/**
 * @file
 */
/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

/**
 * Per-module definition of the current module for debug logging.  Must be defined
 * prior to first inclusion of aj_debug.h
 */
#define AJ_MODULE HEAP

#include <stdio.h>
#include <assert.h>

#include <duktape.h>

#include <ajtcl/aj_target.h>
#include <ajtcl/aj_debug.h>
#include "ajs_heap.h"

/**
 * Turn on per-module debug printing by setting this variable to non-zero value
 * (usually in debugger).
 */
#ifndef NDEBUG
uint8_t dbgHEAP = 0;
uint8_t dbgHEAPDUMP = 0;
#endif

#ifndef AJS_USE_NATIVE_MALLOC

typedef struct {
    void* startOfPool; /* Address of end of this pool */
    void* endOfPool;   /* Address of end of this pool */
    void* freeList;    /* Free list for this pool */
#ifndef NDEBUG
    uint16_t use;      /* Number of entries in use */
    uint16_t hwm;      /* High-water mark */
    uint16_t min;      /* Min allocation from this pool */
    uint16_t max;      /* Max allocation from this pool */
#endif
} Pool;

typedef struct _MemBlock {
    struct _MemBlock* next;
    uint8_t mem[sizeof(void*)];
} MemBlock;

#define MAX_HEAPS  4

typedef struct {
    uint8_t numPools;
    const AJS_HeapConfig* config;
    Pool* pools;
} HeapInfo;

static HeapInfo heapInfo;

size_t AJS_HeapRequired(const AJS_HeapConfig* heapConfig, uint8_t numPools, uint8_t heapNum)
{
    size_t heapSz = 0;
    uint8_t i;

    /*
     * Pool table is allocated from heap 0
     */
    if (heapNum == 0) {
        heapSz += sizeof(Pool) * numPools;
    }
    for (i = 0; i < numPools; ++i) {
        if (heapConfig[i].heapIndex == heapNum) {
            size_t sz = heapConfig[i].size;
            sz += AJS_HEAP_POOL_ROUNDING - (sz & (AJS_HEAP_POOL_ROUNDING - 1));
            heapSz += sz * heapConfig[i].entries;
        }
    }
    return heapSz;
}

void AJS_HeapTerminate(void* heap)
{
    heapInfo.pools = NULL;
}

AJ_Status AJS_HeapInit(void** heap, size_t* heapSz, const AJS_HeapConfig* heapConfig, uint8_t numPools, uint8_t numHeaps)
{
    uint8_t i;
    uint8_t* heapEnd[MAX_HEAPS];
    uint8_t* heapStart[MAX_HEAPS];

    if (numHeaps > MAX_HEAPS) {
        return AJ_ERR_RESOURCES;
    }
    /*
     * Pointer compresson is only supported when there is a single heap smaller than 256K
     */
#ifdef DUK_OPT_HEAPPTR_ENC16
    if (numHeaps > 1) {
        AJ_ErrPrintf(("Pointer compression not allowed with multiple heaps\n"));
        return AJ_ERR_RESOURCES;
    }
    if (*heapSz >= (256 * 1024)) {
        AJ_ErrPrintf(("Pointer compression not allowed for heaps >= 256K\n"));
        return AJ_ERR_RESOURCES;
    }
#endif
    /*
     * Pre-allocate the pool table from the first heap
     */
    heapInfo.pools = (Pool*)heap[0];
    heapEnd[0] = heapStart[0] = (uint8_t*)(&heapInfo.pools[numPools]);
    heapInfo.config = heapConfig;
    heapInfo.numPools = numPools;
    /*
     * Get bounds for other heaps
     */
    for (i = 1; i < numHeaps; ++i) {
        heapEnd[i] = heapStart[i] = heap[i];
    }
    /*
     * Initialize the pool table
     */
    memset(heapInfo.pools, 0, numPools * sizeof(Pool));
    for (i = 0; i < numPools; ++i) {
        uint16_t n;
        Pool* p = &heapInfo.pools[i];
        uint8_t heapIndex = heapConfig[i].heapIndex;
        size_t sz = heapConfig[i].size;
        /*
         * Round up the pool entry size
         */
        sz += AJS_HEAP_POOL_ROUNDING - (sz & (AJS_HEAP_POOL_ROUNDING - 1));
        /*
         * Initialize the free list for this pool
         */
        p->startOfPool = (void*)heapEnd[heapIndex];
        for (n = heapConfig[i].entries; n != 0; --n) {
            MemBlock* block = (MemBlock*)heapEnd[heapIndex];
            block->next = p->freeList;
            p->freeList = (void*)block;
            heapEnd[heapIndex] += sz;
            /*
             * Check there is room for this entry
             */
            if ((heapEnd[heapIndex] - heapStart[heapIndex]) > heapSz[heapIndex]) {
                AJ_ErrPrintf(("Heap %d is too small for the requested pool allocations\n", heapIndex));
                return AJ_ERR_RESOURCES;
            }
        }
        p->endOfPool = (void*)heapEnd[heapIndex];
#ifndef NDEBUG
        p->use = 0;
        p->hwm = 0;
        p->max = 0;
        p->min = 0xFFFF;
#endif
    }
    return AJ_OK;
}

uint8_t AJS_HeapIsInitialized()
{
    return heapInfo.pools != NULL;
}

void* AJS_Alloc(void* userData, size_t sz)
{
    Pool* p = heapInfo.pools;
    uint8_t i;

    if (!p) {
        AJ_ErrPrintf(("Heap not initialized\n"));
        return NULL;
    }
    /*
     * Find pool that can satisfy the allocation
     */
    for (i = 0; i < heapInfo.numPools; ++i, ++p) {
        if (sz <= heapInfo.config[i].size) {
            MemBlock* block = (MemBlock*)p->freeList;
            if (!block) {
                /*
                 * Are we allowed to borrow from next pool?
                 */
                if (heapInfo.config[i].borrow) {
                    continue;
                }
                break;
            }
            AJ_InfoPrintf(("AJS_Alloc pool[%d] allocated %d\n", heapInfo.config[i].size, (int)sz));
            p->freeList = block->next;
#ifndef NDEBUG
            ++p->use;
            p->hwm = max(p->use, p->hwm);
            p->max = max(p->max, sz);
            p->min = min(p->min, sz);
#endif
            return (void*)block;
        }
    }
    AJ_ErrPrintf(("AJS_Alloc of %d bytes failed\n", (int)sz));
    AJS_HeapDump();
    return NULL;
}

duk_uint16_t AJS_EncodePtr16(void* userData, const void* ptr)
{
    if (ptr) {
        ptrdiff_t offset = (ptrdiff_t)ptr - (ptrdiff_t)heapInfo.pools;
        return (duk_uint16_t)(offset >> 2);
    } else {
        return 0;
    }
}

void* AJS_DecodePtr16(void* userData, duk_uint16_t enc)
{
    if (enc) {
        ptrdiff_t offset = ((ptrdiff_t)enc) << 2;
        return (void*)(offset + (ptrdiff_t)heapInfo.pools);
    } else {
        return NULL;
    }
}

void AJS_Free(void* userData, void* mem)
{
    if (mem) {
        Pool* p = heapInfo.pools;
        uint8_t i;
        /*
         * Locate the pool from which the released memory was allocated
         */
        for (i = 0; i < heapInfo.numPools; ++i, ++p) {
            if (((ptrdiff_t)mem >= (ptrdiff_t)p->startOfPool) && ((ptrdiff_t)mem < (ptrdiff_t)p->endOfPool)) {
                MemBlock* block = (MemBlock*)mem;
                block->next = p->freeList;
                p->freeList = block;
                AJ_InfoPrintf(("AJS_Free pool[%d]\n", heapInfo.config[i].size));
#ifndef NDEBUG
                --p->use;
#endif
                return;
            }
        }
        AJ_ErrPrintf(("AJS_Free invalid heap pointer %0x\n", (size_t)mem));
        AJ_ASSERT(0);
    }
}

void* AJS_Realloc(void* userData, void* mem, size_t newSz)
{
    Pool* p = heapInfo.pools;
    uint8_t i;

    if (mem) {
        /*
         * Locate the pool from which the released memory was allocated
         */
        for (i = 0; i < heapInfo.numPools; ++i, ++p) {
            if (((ptrdiff_t)mem >= (ptrdiff_t)p->startOfPool) && ((ptrdiff_t)mem < (ptrdiff_t)p->endOfPool)) {
                size_t oldSz = heapInfo.config[i].size;
                /*
                 * Don't need to do anything if the same block would be reused
                 */
                if ((newSz <= oldSz) && ((i == 0) || (newSz > heapInfo.config[i - 1].size))) {
                    AJ_InfoPrintf(("AJS_Realloc pool[%d] %d bytes in place\n", (int)oldSz, (int)newSz));
#ifndef NDEBUG
                    p->max = max(p->max, newSz);
#endif
                } else {
                    MemBlock* block = (MemBlock*)mem;
                    AJ_InfoPrintf(("AJS_Realloc pool[%d] by AJS_Alloc(%d)\n", (int)oldSz, (int)newSz));
                    mem = AJS_Alloc(userData, newSz);
                    if (mem) {
                        memcpy(mem, (void*)block, min(oldSz, newSz));
                        /*
                         * Put old block on the free list
                         */
                        block->next = p->freeList;
                        p->freeList = block;
#ifndef NDEBUG
                        --p->use;
#endif
                    }
                }
                return mem;
            }
        }
        AJ_ErrPrintf(("AJS_Realloc invalid heap pointer %0x\n", (size_t)mem));
        AJ_ASSERT(0);
    } else {
        return AJS_Alloc(userData, newSz);
    }
    AJ_ErrPrintf(("AJS_Realloc of %d bytes failed\n", (int)newSz));
    AJS_HeapDump();
    return NULL;
}

#ifndef NDEBUG
void AJS_HeapDump(void)
{
    if (dbgHEAPDUMP) {
        uint8_t i;
        size_t memUse = 0;
        size_t memHigh = 0;
        size_t memTotal = 0;
        Pool* p = heapInfo.pools;

        AJ_AlwaysPrintf(("======= dump of %d heap pools ======\n", heapInfo.numPools));
        for (i = 0; i < heapInfo.numPools; ++i, ++p) {
            if (p->hwm == 0) {
                AJ_AlwaysPrintf(("heap[%d] pool[%d] unused\n", heapInfo.config[i].heapIndex, heapInfo.config[i].size));
            } else {
                AJ_AlwaysPrintf(("heap[%d] pool[%d] used=%d free=%d high-water=%d min-alloc=%d max-alloc=%d\n",
                                 heapInfo.config[i].heapIndex, heapInfo.config[i].size, p->use, heapInfo.config[i].entries - p->use, p->hwm, p->min, p->max));
            }
            memUse += p->use * heapInfo.config[i].size;
            memHigh += p->hwm * heapInfo.config[i].size;
            memTotal += p->hwm * p->max;
        }
        AJ_AlwaysPrintf(("======= heap hwm = %d use = %d waste = %d ======\n", (int)memHigh, (int)memUse, (int)(memHigh - memTotal)));
    }
}
#endif

#endif // AJS_USE_NATIVE_MALLOC