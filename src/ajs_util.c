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
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include "ajs.h"
#include "ajs_util.h"

void AJS_PutFunctionList(duk_context* ctx, duk_idx_t objIdx, const duk_function_list_entry*funcList, uint8_t light)
{
    if (light) {
        objIdx = duk_normalize_index(ctx, objIdx);
        while (funcList->key) {
            duk_push_c_lightfunc(ctx, funcList->value, funcList->nargs, 0, 0);
            duk_put_prop_string(ctx, objIdx, funcList->key);
            ++funcList;
        }
    } else {
        duk_put_function_list(ctx, objIdx, funcList);
    }
}

void AJS_RegisterFinalizer(duk_context* ctx, duk_idx_t objIdx, duk_c_function finFunc)
{
    objIdx = duk_normalize_index(ctx, objIdx);
    duk_push_c_function(ctx, finFunc, 1);
    duk_set_finalizer(ctx, objIdx);
}

/*
 * This function does the equivalent of the following JavaScript
 *
 * Object.createObject(<protoype>)
 */
int AJS_CreateObjectFromPrototype(duk_context* ctx, duk_idx_t protoIdx)
{
    protoIdx = duk_normalize_index(ctx, protoIdx);
    duk_push_object(ctx);
    duk_dup(ctx, protoIdx);
    duk_set_prototype(ctx, -2);
    return duk_get_top_index(ctx);
}

/*
 * This function does the equivalent of the following JavaScript
 *
 * Object.freeze(<object>)
 */
void AJS_ObjectFreeze(duk_context* ctx, duk_idx_t objIdx)
{
    objIdx = duk_normalize_index(ctx, objIdx);

    duk_get_global_string(ctx, "Object");
    duk_get_prop_string(ctx, -1, "freeze");
    duk_dup(ctx, objIdx);
    duk_call(ctx, 1);
    duk_pop_2(ctx);
}

int AJS_IncrementProperty(duk_context* ctx, const char* intProp, duk_idx_t objIdx)
{
    int val;

    objIdx = duk_normalize_index(ctx, objIdx);
    duk_get_prop_string(ctx, -1, intProp);
    val = duk_require_int(ctx, -1) + 1;
    duk_pop(ctx);
    duk_push_int(ctx, val);
    duk_put_prop_string(ctx, objIdx, intProp);
    return val;
}

/*
 * This function does the equivalent of the following JavaScript
 *
 * Object.defineProperty(<obj>, <prop>, { set: function() { ... }, get: function() { ... } });
 *
 */
void AJS_SetPropertyAccessors(duk_context* ctx, duk_idx_t objIdx, const char* prop, duk_c_function setter, duk_c_function getter)
{
    duk_uint_t flags = 0;

    /*
     * Must have a least one of these
     */
    if (!setter && !getter) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "setter and getter cannot both be NULL");
    }
    /*
     * In case the object index is relative
     */
    objIdx = duk_normalize_index(ctx, objIdx);
    duk_push_string(ctx, prop);
    if (getter) {
        flags |= DUK_DEFPROP_HAVE_GETTER;
        duk_push_c_function(ctx, getter, 0);
    }
    if (setter) {
        flags |= DUK_DEFPROP_HAVE_SETTER;
        duk_push_c_function(ctx, setter, 1);
    }
    duk_def_prop(ctx, objIdx, flags);
}

size_t NumProps(duk_context* ctx, duk_idx_t enumIdx)
{
    size_t num = 0;
    AJ_ASSERT(duk_is_object(ctx, enumIdx));
    duk_enum(ctx, enumIdx, DUK_ENUM_OWN_PROPERTIES_ONLY);
    while (duk_next(ctx, -1, 0)) {
        duk_pop(ctx);
        ++num;
    }
    duk_pop(ctx); // enum
    return num;
}

const char* AJS_GetStringProp(duk_context* ctx, duk_idx_t idx, const char* prop)
{
    const char* str;

    duk_get_prop_string(ctx, idx, prop);
    str = duk_get_string(ctx, -1);
    duk_pop(ctx);
    return str;
}

uint32_t AJS_GetIntProp(duk_context* ctx, duk_idx_t idx, const char* prop)
{
    uint32_t val;

    duk_get_prop_string(ctx, idx, prop);
    val = duk_get_int(ctx, -1);
    duk_pop(ctx);
    return val;
}

void AJS_GetGlobalStashObject(duk_context* ctx, const char* name)
{
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, name);
    if (!duk_is_object(ctx, -1)) {
        duk_pop(ctx);
        duk_push_object(ctx);
        duk_dup_top(ctx);
        duk_put_prop_string(ctx, -3, name);
    }
    /*
     * Remove the global stash leaving the object on the top of the stack
     */
    duk_remove(ctx, -2);
}

void AJS_ClearGlobalStashObject(duk_context* ctx, const char* name)
{
    duk_push_global_stash(ctx);
    duk_push_object(ctx);
    duk_put_prop_string(ctx, -2, name);
    duk_pop(ctx);
}

void AJS_GetGlobalStashArray(duk_context* ctx, const char* name)
{
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, name);
    if (!duk_is_array(ctx, -1)) {
        duk_pop(ctx);
        duk_push_array(ctx);
        duk_dup_top(ctx);
        duk_put_prop_string(ctx, -3, name);
    }
    /*
     * Remove the global stash leaving the array on the top of the stack
     */
    duk_remove(ctx, -2);
}

duk_idx_t AJS_GetAllJoynProperty(duk_context* ctx, const char* prop)
{
    if (duk_get_global_string(ctx, AJS_AJObjectName)) {
        duk_get_prop_string(ctx, -1, prop);
        duk_remove(ctx, -2);
    }
    return duk_get_top_index(ctx);
}

static uint8_t pins = FALSE;

void* AJS_PinBuffer(duk_context* ctx, duk_idx_t idx)
{
    void* stablePtr;

    idx = duk_normalize_index(ctx, idx);
    AJS_GetGlobalStashArray(ctx, "pins");
    duk_dup(ctx, idx);
    stablePtr = duk_require_buffer(ctx, -1, NULL);
    duk_put_prop_index(ctx, -2, duk_get_length(ctx, -2));
    duk_pop(ctx);
    pins = TRUE;
    return stablePtr;
}

const char* AJS_PinString(duk_context* ctx, duk_idx_t idx)
{
    const char* stableStr;

    idx = duk_normalize_index(ctx, idx);
    AJS_GetGlobalStashArray(ctx, "pins");
    duk_dup(ctx, idx);
    stableStr = duk_require_string(ctx, -1);
    duk_put_prop_index(ctx, -2, duk_get_length(ctx, -2));
    duk_pop(ctx);
    pins = TRUE;
    return stableStr;
}

void AJS_ClearPins(duk_context* ctx)
{
    if (pins) {
        duk_push_global_stash(ctx);
        duk_del_prop_string(ctx, -1, "pins");
        duk_pop(ctx);
        pins = FALSE;
    }
}

static AJ_Time watchdogTimer;
static uint32_t watchdogTimeout;
static uint8_t watchdogEnabled = TRUE;

void AJS_EnableWatchdogTimer(void)
{
    watchdogEnabled = TRUE;
}

void AJS_DisableWatchdogTimer(void)
{
    watchdogEnabled = FALSE;
}

void AJS_SetWatchdogTimer(uint32_t timeout)
{
    AJ_InitTimer(&watchdogTimer);
    watchdogTimeout = timeout;
}

void AJS_ClearWatchdogTimer()
{
    watchdogTimeout = 0;
}

/*
 * This function gets called periodically by the duktape bytecode executor.
 */
int AJS_ExecTimeoutCheck(const void*udata)
{
    if (watchdogEnabled) {
        if (watchdogTimeout) {
            if (AJ_GetElapsedTime(&watchdogTimer, TRUE) > watchdogTimeout) {
                return 1;
            }
        }
        return 0;
    }
    watchdogTimeout = 0;
    return 0;
}

AJ_Message* AJS_CloneAndCloseMessage(duk_context* ctx, AJ_Message* msg)
{
    size_t len = strlen(msg->sender);
    struct {
        AJ_Message msg;
        AJ_MsgHeader hdr;
        char sender[1];
    }* buffer;

    duk_push_fixed_buffer(ctx, (sizeof(*buffer) + len));
    buffer = AJS_PinBuffer(ctx, -1);
    duk_pop(ctx);

    memcpy(buffer->sender, msg->sender, len + 1);
    memcpy(&buffer->hdr, msg->hdr, sizeof(AJ_MsgHeader));
    memset(&buffer->msg, 0, sizeof(AJ_Message));
    buffer->msg.hdr = &buffer->hdr;
    buffer->msg.sender = buffer->sender;
    buffer->msg.msgId = msg->msgId;
    buffer->msg.sessionId = msg->sessionId;
    buffer->msg.bus = msg->bus;
    AJ_CloseMsg(msg);

    return (AJ_Message*)buffer;
}

void AJS_DumpJX(duk_context* ctx, const char* tag, duk_idx_t idx)
{
#ifndef NDEBUG
    if (dbgAJS) {
        if (tag) {
            AJ_AlwaysPrintf(("%s\n", tag));
        }
        idx = duk_normalize_index(ctx, idx);
        duk_get_global_string(ctx, "Duktape");
        duk_get_prop_string(ctx, -1, "enc");
        duk_push_string(ctx, "jx");
        duk_dup(ctx, idx);
        duk_pcall(ctx, 2);
        AJ_AlwaysPrintf(("%s\n", duk_get_string(ctx, -1)));
        duk_pop_2(ctx);
    }
#endif
}

void AJS_DumpStack(duk_context* ctx)
{
#ifndef NDEBUG
    if (dbgAJS) {
        duk_idx_t top = duk_get_top_index(ctx);

        AJS_DumpJX(ctx, "===== Duktape Stack =====", top);
        while (top--) {
            AJS_DumpJX(ctx, NULL, top);
        }
        AJ_AlwaysPrintf(("=========================\n"));
    }
#endif
}