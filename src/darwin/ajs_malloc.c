/**
 * @file
 */
/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#include "../ajs.h"
#include "../ajs_heap.h"

#ifndef AJS_USE_NATIVE_MALLOC

static const AJS_HeapConfig heapConfig[] = {
    { 16,     200, 0, 0 },
    { 24,     800, 0, 0 },
    { 32,     800, 0, 0 },
    { 48,     800, 0, 0 },
    { 64,     800, 0, 0 },
    { 96,     800, 0, 0 },
    { 128,     64, 0, 0 },
    { 256,     64, 0, 0 },
    { 512,     16, 0, 0 },
    { 1024,    16, 0, 0 },
    { 2048,     8, 0, 0 },
    { 3000,     8, 0, 0 },
    { 4096,     2, 0, 0 },
    { 16384,    2, 0, 0 }
};

static uint32_t heap[500000 / 4];

AJ_Status AJS_HeapCreate()
{
    size_t heapSz[1];
    uint8_t* heapPtr[1];
    heapPtr[0] = &heap;
    /*
     * Allocate the heap pools
     */
    heapSz[0] = AJS_HeapRequired(heapConfig, ArraySize(heapConfig), 0);
    if (heapSz[0] > sizeof(heap)) {
        AJ_ErrPrintf(("Heap space is too small %d required %d\n", (int)sizeof(heap), (int)heapSz[0]));
        return AJ_ERR_RESOURCES;
    }
    AJ_Printf("Allocated heap %d bytes\n", (int)heapSz);
    AJS_HeapInit(heapPtr, heapSz, heapConfig, ArraySize(heapConfig), 1);
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