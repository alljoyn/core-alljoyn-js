/**
 * @file
 */
/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
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
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
******************************************************************************/

#include "../ajs.h"
#include "../ajs_util.h"
#include "../ajs_target.h"
#include "../ajs_ctrlpanel.h"
#include <ajtcl/aj_util.h>
#include "../ajs_heap.h"

/**
 * Turn on per-module debug printing by setting this variable to non-zero value
 * (usually in debugger).
 */
#ifndef NDEBUG
uint8_t dbgAJS_MALLOC = 0;
#endif

static const AJS_HeapConfig heapConfig[] = {
    { 12,      20, AJS_POOL_BORROW, 0 },
    { 20,     100, AJS_POOL_BORROW, 0 },
    { 24,     100, AJS_POOL_BORROW, 0 },
    { 32,     700, AJS_POOL_BORROW, 0 },
    { 40,     250, 0, 0 },
    { 48,     400, 0, 0 },
    { 128,    80, 0, 0 },
    { 256,    30, 0, 1 },
    { 512,    16, 0, 1 },
    { 1024,   6, 0, 1 },
    { 2048,   6, 0, 1 },
#ifdef DUK_FIXED_SIZE_ST
    { DUK_FIXED_SIZE_ST* sizeof(void*), 1, 0, 1 }
#else
    { 4296,     3, 0, 1 }
#endif
};
/*
 * Special section of RAM on the STM32F4. It is not in a continuous address space
 * as normal ram so it must have its own heap in the pool allocator
 */
static uint32_t heapCCM[63928 / 4] __attribute__ ((section(".ccm")));
/*
 * Regular RAM section. This can be dynamically allocated out of the FreeRTOS heap
 */
static uint32_t* heapRAM;

AJ_Status AJS_HeapCreate()
{
    size_t heapSz[2];
    uint8_t* heapArray[2];
    /*
     * Calculate the sizes for each heap
     */
    heapSz[0] = AJS_HeapRequired(heapConfig, ArraySize(heapConfig), 0); //RAM
    heapSz[1] = AJS_HeapRequired(heapConfig, ArraySize(heapConfig), 1); //CCM
    heapRAM = AJ_Malloc(heapSz[0]);
    if (!heapRAM) {
        AJ_ErrPrintf(("Not enough memory to allocate %u bytes\n", heapSz[0]));
        return AJ_ERR_RESOURCES;
    }

    if (heapSz[1] > sizeof(heapCCM)) {
        AJ_ErrPrintf(("Heap space is too small %d required %d\n", (int)sizeof(heapCCM), (int)heapSz[1]));
        return AJ_ERR_RESOURCES;
    }

    AJ_Printf("Allocated heap %d bytes\n", (int)heapSz[0]);

    heapArray[0] = heapRAM;
    heapArray[1] = &heapCCM;

    AJS_HeapInit(heapArray, heapSz, heapConfig, ArraySize(heapConfig), 2);
}

void AJS_HeapDestroy()
{
    AJ_Free(heapRAM);
}