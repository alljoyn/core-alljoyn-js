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
#include "ajs_storage.h"

/*
 * This file includes a sample implementation for storing and loading AJS
 * scripts into persistent storage. It uses the AllJoyn Thin Client NVRAM API's
 * to implement the required target specific functionality. This implementation
 * can be used for any target but can also be left out, allowing the target to
 * provide its own implementation.
 */

uint32_t AJS_MaxScriptLen()
{
    return (3 * AJ_NVRAM_GetSizeRemaining()) / 4;
}

AJ_Status AJS_OpenScript(uint32_t length, void** ctx)
{
    AJ_NV_DATASET* ds;
    if (length > AJS_MaxScriptLen()) {
        return AJ_ERR_RESOURCES;
    }
    ds = AJ_NVRAM_Open(AJS_SCRIPT_NVRAM_ID, length ? "w" : "r", length);
    if (!ds) {
        return AJ_ERR_FAILURE;
    }
    *ctx = (void*)ds;
    return AJ_OK;
}

AJ_Status AJS_WriteScript(uint8_t* script, uint32_t length, void* ctx)
{
    AJ_NV_DATASET* ds = (AJ_NV_DATASET*)ctx;
    if (ds) {
        if (AJ_NVRAM_Write(script, length, ds) != length) {
            goto WriteFailure;
        }
        return AJ_OK;
    }
WriteFailure:
    AJ_ErrPrintf(("AJS_WriteScript(): Could not write script to NVRAM\n"));
    return AJ_ERR_FAILURE;
}

AJ_Status AJS_ReadScript(uint8_t** script, uint32_t* length, void* ctx)
{
    AJ_NV_DATASET* ds = (AJ_NV_DATASET*)ctx;
    AJ_NV_DATASET* l;
    if (ds) {
        *script = (uint8_t*)AJ_NVRAM_Peek(ds);
        l = AJ_NVRAM_Open(AJS_SCRIPT_SIZE_ID, "r", 0);
        if (l) {
            if (AJ_NVRAM_Read(length, sizeof(uint32_t), l) != sizeof(uint32_t)) {
                goto ReadFailure;
            }
            AJ_NVRAM_Close(l);
            return AJ_OK;
        }
    }
ReadFailure:
    AJ_ErrPrintf(("AJS_ReadScript(): Could not read script\n"));
    if (l) {
        AJ_NVRAM_Close(l);
    }
    return AJ_ERR_FAILURE;
}

AJ_Status AJS_CloseScript(void* ctx)
{
    AJ_NV_DATASET* ds = (AJ_NV_DATASET*)ctx;
    if (ds) {
        AJ_NVRAM_Close(ds);
    }
    return AJ_OK;
}

AJ_Status AJS_DeleteScript(void)
{
    AJ_NVRAM_Delete(AJS_SCRIPT_NVRAM_ID);
    return AJ_OK;
}