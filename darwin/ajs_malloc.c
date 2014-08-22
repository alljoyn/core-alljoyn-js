/**
 * @file
 */
/******************************************************************************
 * Copyright (c) 2013, 2014, AllSeen Alliance. All rights reserved.
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

#include "ajs.h"
#include "ajs_heap.h"

#ifndef AJS_USE_NATIVE_MALLOC

static const AJS_HeapConfig heapConfig[] = {
    { 16,     200 },
    { 24,     800 },
    { 32,     800 },
    { 48,     800 },
    { 64,     800 },
    { 96,     800 },
    { 128,     64 },
    { 256,     64 },
    { 512,     16 },
    { 1024,    16 },
    { 2048,     8 },
    { 3000,     8 },
    { 4096,     2 },
    { 16384,    2 }
};

static uint32_t heap[500000 / 4];

AJ_Status AJS_HeapCreate()
{
    size_t heapSz;
    /*
     * Allocate the heap pools
     */
    heapSz = AJS_HeapRequired(heapConfig, ArraySize(heapConfig));
    if (heapSz > sizeof(heap)) {
        AJ_ErrPrintf(("Heap space is too small %d required %d\n", (int)sizeof(heap), (int)heapSz));
        return AJ_ERR_RESOURCES;
    }
    AJ_Printf("Allocated heap %d bytes\n", (int)heapSz);
    AJS_HeapInit(heap, heapSz, heapConfig, ArraySize(heapConfig));
    return AJ_OK;
}

void AJS_HeapDestroy()
{
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
void AJS_HeapDump()
{
}
#endif

#endif
