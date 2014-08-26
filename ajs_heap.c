/**
 * @file
 */
/******************************************************************************
 * Copyright (c) 2012-2013, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

/**
 * Per-module definition of the current module for debug logging.  Must be defined
 * prior to first inclusion of aj_debug.h
 */
#define AJ_MODULE HEAP

#include <stdio.h>
#include <assert.h>

#include "aj_target.h"
#include "aj_debug.h"
#include "ajs_heap.h"

extern void duk_dump_string_table(void* ctx);

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
    void* endOfPool; /* Address of end of this pool */
    void* freeList;  /* Free list for this pool */
#ifndef NDEBUG
    uint16_t use;    /* Number of entries in use */
    uint16_t hwm;    /* High-water mark */
    uint16_t max;    /* Max allocation from this pool */
#endif
} Pool;

typedef struct _MemBlock {
    struct _MemBlock* next;
    uint8_t mem[sizeof(void*)];
} MemBlock;

typedef struct {
    Pool** heapPools;    /* Array of pointers to pools for each heap */
    uint8_t* numPools;   /* Array of the number of pools per heap */
    uint8_t** heapStart; /* Array of pointers to the start of each heap */
    uint8_t numHeaps;    /* Number of heaps */
    size_t* heapSz;      /* Array of the sizes of each heap */
} PoolConfig;


static const AJS_HeapConfig* heapConfig;
static uint8_t numPools;
static PoolConfig* pools;

void AJS_HeapDump(void);

static int8_t findHeapBySize(size_t sz)
{
    uint8_t i;
    for (i = 0; i < numPools; i++) {
        if (sz <= heapConfig[i].size) {
            return heapConfig[i].heapIndex;
        }
    }
    return -1;
}
static int8_t findHeapByMem(uint8_t* mem)
{
    int8_t i;
    for (i = 0; i < pools->numHeaps; i++) {
        if (((ptrdiff_t)mem >= (ptrdiff_t)pools->heapStart[i]) &&
            ((ptrdiff_t)mem < (ptrdiff_t)pools->heapStart[i] + pools->heapSz[i])) {
            return i;
        }
    }
    return -1;
}

size_t AJS_HeapRequired(const AJS_HeapConfig* poolConfig, uint8_t numPools, uint8_t heapNum)
{
    size_t heapSz = sizeof(Pool) * numPools;
    uint8_t i;

    for (i = 0; i < numPools; ++i) {
        if (poolConfig[i].heapIndex == heapNum) {
            size_t sz = poolConfig[i].size;
            sz += AJS_HEAP_POOL_ROUNDING - (sz & (AJS_HEAP_POOL_ROUNDING - 1));
            heapSz += sz * poolConfig[i].entries;
        }
    }
    return heapSz;
}

void AJS_HeapTerminate(void* heap)
{
    if (pools && pools->heapPools) {
        AJ_Free(pools->heapPools);
    }
    if (pools && pools->heapStart) {
        AJ_Free(pools->heapStart);
    }
    if (pools && pools->numPools) {
        AJ_Free(pools->numPools);
    }
    if (pools) {
        AJ_Free(pools);
    }
    pools = NULL;
}
AJ_Status AJS_HeapInit(void** heap, size_t* heapSz, const AJS_HeapConfig* poolConfig, uint8_t num, uint8_t numHeaps)
{
    uint8_t i, j, k;
    uint16_t n;
    Pool* p;
    /* Allocate the poolConfig structure */
    pools = AJ_Malloc(sizeof(PoolConfig));
    pools->heapPools = AJ_Malloc(numHeaps * sizeof(Pool*));
    pools->numPools = AJ_Malloc(numHeaps * sizeof(uint8_t));
    pools->heapStart = AJ_Malloc(numHeaps * sizeof(uint8_t*));
    pools->heapSz = AJ_Malloc(numHeaps * sizeof(size_t));
    pools->numHeaps = numHeaps;
    numPools = num;
    heapConfig = poolConfig;

    /* Go through each heap and setup, 'j' is the heap number for this outer loop */
    for (j = 0; j < pools->numHeaps; j++) {
        uint8_t* heapStart;
        uint8_t* heapEnd;
        uint8_t poolsInHeap;
        /* Calculate the end for this heap */
        poolsInHeap = 0;
        for (k = 0;k < num;k++) {
            if (poolConfig[k].heapIndex == j) {
                poolsInHeap++;
            }
        }
        heapEnd = (uint8_t*)heap[j] + sizeof(Pool) * poolsInHeap;

        /* Set the static values for this heap */
        pools->heapSz[j] = heapSz[j];
        pools->heapPools[j] = (Pool*)heap[j];
        pools->heapStart[j] = (uint8_t*)heapEnd;
        pools->numPools[j] = poolsInHeap;

        p = (Pool*)heap[j];
        /*
         * Clear the heap pool info  block
         */
        memset(pools->heapPools[j], 0, heapEnd - (uint8_t*)heap[j]);
        /* Go through the pools for each heap, 'i' is the pool iterator */
        for (i = 0; i < numPools; ++i) {
            if (poolConfig[i].heapIndex == j) {
                size_t sz = poolConfig[i].size;
                size_t heapSize = heapSz[j];
                sz += AJS_HEAP_POOL_ROUNDING - (sz & (AJS_HEAP_POOL_ROUNDING - 1));
                /*
                 * Add all blocks to the pool free list
                 */
                for (n = poolConfig[i].entries; n != 0; --n) {
                    MemBlock* block = (MemBlock*)heapEnd;
                    if (sz > heapSize) {
                        return AJ_ERR_RESOURCES;
                    }

                    block->next = p->freeList;
                    p->freeList = (void*)block;
                    heapEnd += sz;
                    heapSize -= sz;

                }
                /*
                 * Save end of pool pointer for use by AJ_PoolFree
                 */
                p->endOfPool = (void*)heapEnd;
#ifndef NDEBUG
                p->use = 0;
                p->hwm = 0;
                p->max = 0;
#endif
                ++p;
            }
        }
    }
    return AJ_OK;
}

uint8_t AJS_HeapIsInitialized()
{
    return pools->heapPools != NULL;
}

void* AJS_Alloc(void* userData, size_t sz)
{
    Pool* p;
    uint8_t i;
    uint8_t heapNum;
    /*
     * Find the right heap to allocate from
     */
    heapNum = findHeapBySize(sz);
    if (sz == -1) {
        AJ_ErrPrintf(("No heap with size %d\n", sz));
        return NULL;
    }
    p = pools->heapPools[heapNum];
    assert(p->freeList != p);
    if (!p) {
        AJ_ErrPrintf(("Heap not initialized\n"));
        return NULL;
    }
    /*
     * Find pool that can satisfy the allocation
     */
    for (i = 0; i < numPools; ++i) {
        if ((sz <= heapConfig[i].size) && (heapConfig[i].heapIndex == heapNum)) {
            MemBlock* block = (MemBlock*)p->freeList;
            if (!block || (uint8_t*)block >= (uint8_t*)p->endOfPool) {
                /*
                 * Are we allowed to borrowing from next pool?
                 */
                if (heapConfig[i].borrow) {
                    continue;
                }
                break;
            }
            AJ_InfoPrintf(("AJ_PoolAlloc pool[%d] allocated %d\n", heapConfig[i].size, (int)sz));
            p->freeList = block->next;
            assert(p->freeList != p);
#ifndef NDEBUG
            ++p->use;
            p->hwm = max(p->use, p->hwm);
            p->max = max(p->max, sz);
#endif
            return (void*)block;
        }
        if (heapConfig[i].heapIndex == heapNum) {
            ++p;
        }
    }
    AJ_ErrPrintf(("AJS_PoolAlloc of %d bytes failed\n", (int)sz));
    AJS_HeapDump();
    return NULL;
}

void AJS_Free(void* userData, void* mem)
{
    Pool* p;
    uint8_t i;
    uint8_t heapIdx;

    if (mem) {
        /*
         * Locate the heap that contains the memory
         */
        heapIdx = findHeapByMem(mem);
        if (heapIdx == -1) {
            AJ_ErrPrintf(("No heap memory contains 0x%x\n", mem));
        }
        assert((ptrdiff_t)mem >= (ptrdiff_t)pools->heapStart[heapIdx]);
        p = pools->heapPools[heapIdx];
        /*
         * Locate the pool from which the released memory was allocated
         */
        for (i = 0; i < numPools; ++i, ++p) {
            if ((ptrdiff_t)mem < (ptrdiff_t)p->endOfPool) {
                MemBlock* block = (MemBlock*)mem;
                block->next = p->freeList;
                p->freeList = block;
                AJ_InfoPrintf(("AJ_PoolFree pool[%d]\n", heapConfig[i].size));
#ifndef NDEBUG
                --p->use;
#endif
                break;
            }
            assert(i < numPools);
        }

    }
}

void* AJS_Realloc(void* userData, void* mem, size_t newSz)
{
    Pool* pOld;
    uint8_t i;
    uint8_t oldHeap;

    if (mem) {
        /*
         * Find the current heap this memory exists in
         */
        oldHeap = findHeapByMem(mem);
        assert(oldHeap != -1);
        assert((ptrdiff_t)mem >= (ptrdiff_t)pools->heapStart[oldHeap]);
        pOld = pools->heapPools[oldHeap];
        /*
         * Locate the pool from which the released memory was allocated
         */
        for (i = 0; i < numPools; ++i) {
            if (((ptrdiff_t)mem < (ptrdiff_t)pOld->endOfPool) && (heapConfig[i].heapIndex == oldHeap)) {
                size_t oldSz = heapConfig[i].size;
                /*
                 * Don't need to do anything if the same block would be reused
                 */
                if ((newSz <= oldSz) && ((i == 0) || (newSz > heapConfig[i - 1].size))) {
                    AJ_InfoPrintf(("AJ_Realloc pool[%d] %d bytes in place\n", (int)oldSz, (int)newSz));
                } else {
                    MemBlock* block = (MemBlock*)mem;
                    AJ_InfoPrintf(("AJ_Realloc pool[%d] by AJ_Alloc(%d)\n", (int)oldSz, (int)newSz));
                    mem = AJS_Alloc(NULL, newSz);
                    if (mem) {
                        memcpy(mem, (void*)block, min(oldSz, newSz));
                        /*
                         * Put old block on the free list
                         */
                        block->next = pOld->freeList;
                        pOld->freeList = block;
#ifndef NDEBUG
                        --pOld->use;
#endif
                    }
                }
                return mem;
            }
            if (heapConfig[i].heapIndex == oldHeap) {
                ++pOld;
            }
        }
    } else {
        return AJS_Alloc(NULL, newSz);
    }
    AJ_ErrPrintf(("AJ_Realloc of %d bytes failed\n", (int)newSz));
    AJS_HeapDump();
    return NULL;
}

#ifndef NDEBUG
void AJS_HeapDump(void)
{
    if (dbgHEAPDUMP) {
        int k;
        Pool* p;

        size_t memUse = 0;
        size_t memHigh = 0;
        size_t memTotal = 0;
        AJ_AlwaysPrintf(("================= Dump of %d heap pools ==============\n", numPools));
        for (k = 0; k < pools->numHeaps; k++) {
            uint8_t i;
            p = *(pools->heapPools + k);
            for (i = 0; i < numPools; ++i) {
                if (k == heapConfig[i].heapIndex) {
                    AJ_AlwaysPrintf(("pool[%4d] used=%4d free=%4d high-water=%4d max-alloc=%4d heap=%2d\n",
                            heapConfig[i].size, p->use, heapConfig[i].entries - p->use, p->hwm, p->max, heapConfig[i].heapIndex));
                    memUse += p->use * heapConfig[i].size;
                    memHigh += p->hwm * heapConfig[i].size;
                    memTotal += p->hwm * p->max;
                    ++p;
                }
            }
        }
        AJ_AlwaysPrintf(("======= heap hwm = %d use = %d waste = %d ======\n", (int)memHigh, (int)memUse, (int)(memHigh - memTotal)));
    }
}
#endif

#endif // AJS_USE_NATIVE_MALLOC
