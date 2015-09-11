/**
 * @file
 */
/******************************************************************************
 *  * Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
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

#include "ajs.h"
#include "ajs_heap.h"

#ifndef AJS_USE_NATIVE_MALLOC

static const AJS_HeapConfig heapConfig[] = {
    { 8,      100, AJS_POOL_BORROW, 0 },
    { 16,     600, AJS_POOL_BORROW, 0 },
    { 24,     400, AJS_POOL_BORROW, 0 },
    { 32,     400, AJS_POOL_BORROW, 0 },
    { 48,     300, 0, 0 },
    { 64,     100, 0, 0 },
    { 128,     64, 0, 0 },
    { 256,     32, 0, 0 },
    { 512,     16, 0, 0 },
    { 1024,     8, 0, 0 },
    { 2048,     4, AJS_POOL_BORROW, 0 },
    { 6000,     2, 0, 0 }
};

#define NUM_HEAPS 1

static void* heap[NUM_HEAPS];

AJ_Status AJS_HeapCreate()
{
    int i;
    size_t heapSz[NUM_HEAPS];

    /*
     * Allocate the heap pools
     */
    for (i = 0; i < NUM_HEAPS; ++i) {
        heapSz[i] = AJS_HeapRequired(heapConfig, ArraySize(heapConfig), i);
        heap[i] = malloc(heapSz[i]);
        if (!heap[i]) {
            AJ_ErrPrintf(("AJS_HeapCreate(): Malloc failed to allocate %d bytes\n", heapSz[i]));
            return AJ_ERR_RESOURCES;
        }
        AJ_Printf("Allocated heap[%d] %d bytes\n", i, (int)heapSz[i]);
    }
    return AJS_HeapInit(heap, heapSz, heapConfig, ArraySize(heapConfig), NUM_HEAPS);
}

void AJS_HeapDestroy()
{
    int i;
    for (i = 0; i < NUM_HEAPS; ++i) {
        free(heap[i]);
    }
}

#else

AJ_Status AJS_HeapCreate()
{
    return AJ_OK;
}

void AJS_HeapDestroy()
{
}

void* AJS_Alloc(void* userData, size_t sz)
{
    return malloc(sz);
}

void AJS_Free(void* userData, void* mem)
{
    return free(mem);
}

void* AJS_Realloc(void* userData, void* mem, size_t newSz)
{
    return realloc(mem, newSz);
}

#ifndef NDEBUG
void AJS_HeapDump(void)
{
}
#endif

#endif