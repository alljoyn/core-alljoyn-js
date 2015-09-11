/**
 * @file
 */
/******************************************************************************
 * Copyright (c) 2016 Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright 2016 Open Connectivity Foundation and Contributors to
 *    AllSeen Alliance. All rights reserved.
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
#include "ajs_util.h"
#include "ajs_target.h"
#include "ajs_ctrlpanel.h"
#include "aj_util.h"
#include "ajs_heap.h"

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
    { 128,    100, 0, 0 },
    { 256,    30, 0, 0 },
    { 512,    16, 0, 0 },
    { 1024,   8, 0, 0 },
    { 2048,   6, 0, 0 },
#ifdef DUK_FIXED_SIZE_ST
    { DUK_FIXED_SIZE_ST* sizeof(void*), 1, 0, 0 }
#else
    { 4296,     3, 0, 0 }
#endif
};
/*
 * Static heap for the pool allocator.
 * TODO: Tune this heap, 128k is much bigger than is needed.
 */
static uint8_t heapRAM[128756];

AJ_Status AJS_HeapCreate()
{
    size_t heapSz[1];
    uint8_t* heapArray[1];
    /*
     * Calculate the sizes for each heap
     */
    heapSz[0] = AJS_HeapRequired(heapConfig, ArraySize(heapConfig), 0); //RAM
    if (heapSz[0] > sizeof(heapRAM)) {
        AJ_ErrPrintf(("Not enough memory to allocate %u bytes\n", heapSz[0]));
        return AJ_ERR_RESOURCES;
    }

    AJ_InfoPrintf(("Allocated heap %d bytes\n", (int)heapSz[0]));

    heapArray[0] = (uint8_t*)&heapRAM;

    AJS_HeapInit((void**)heapArray, heapSz, heapConfig, ArraySize(heapConfig), 1);

    return AJ_OK;
}

void AJS_HeapDestroy()
{
    return;
}