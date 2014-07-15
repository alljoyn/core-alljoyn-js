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
#include "ajs_util.h"

void AJS_RegisterFinalizer(duk_context* ctx, duk_idx_t objIdx, duk_c_function finFunc)
{
    objIdx = duk_normalize_index(ctx, objIdx);
    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, "Duktape");
    if (duk_is_object(ctx, -1)) {
        duk_get_prop_string(ctx, -1, "fin");
        duk_dup(ctx, objIdx);
        duk_push_c_function(ctx, finFunc, 1);
        if (duk_pcall(ctx, 2) != DUK_EXEC_SUCCESS) {
            AJ_ErrPrintf(("Failed to set finalizer %s\n", duk_to_string(ctx, -1)));
        }
        duk_pop(ctx);
    } else {
        AJ_ErrPrintf(("Failed to get Duktape object\n"));
    }
    duk_pop_2(ctx);
}

/*
 * This function does the equivalent of the following JavaScript
 *
 * Object.createObject(<protoype>)
 *
 */
int AJS_CreateObjectFromPrototype(duk_context* ctx, duk_idx_t protoIdx)
{
    /*
     * In case the protoype object index is relative
     */
    protoIdx = duk_normalize_index(ctx, protoIdx);

    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, "Object");
    duk_get_prop_string(ctx, -1, "create");
    duk_dup(ctx, protoIdx);
    duk_call(ctx, 1);
    /*
     * Leave the newly created object returnd by the call on the stack
     */
    duk_swap(ctx, -1, -3);
    duk_pop_2(ctx);
    return duk_get_top_index(ctx);
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
AJ_Status AJS_SetPropertyAccessors(duk_context* ctx, duk_idx_t objIdx, const char* prop, duk_c_function setter, duk_c_function getter)
{
    /*
     * Must have a least one of these
     */
    if (!setter && !getter) {
        return AJ_ERR_NULL;
    }
    /*
     * In case the object index is relative
     */
    objIdx = duk_normalize_index(ctx, objIdx);

    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, "Object");
    duk_get_prop_string(ctx, -1, "defineProperty");
    duk_dup(ctx, objIdx);
    duk_push_string(ctx, prop);
    /*
     * Create the set/get object
     */
    duk_push_object(ctx);
    if (setter) {
        duk_push_c_function(ctx, setter, 1);
        duk_put_prop_string(ctx, -2, "set");
    }
    if (getter) {
        duk_push_c_function(ctx, getter, 0);
        duk_put_prop_string(ctx, -2, "get");
    }
    duk_call(ctx, 3);
    duk_pop_3(ctx);
    return AJ_OK;
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
    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, AJS_AllJoynObject);
    duk_get_prop_string(ctx, -1, prop);
    duk_remove(ctx, -2);
    duk_remove(ctx, -2);
    return duk_get_top_index(ctx);
}

size_t AJS_NumProps(duk_context* ctx, duk_idx_t idx)
{
    size_t num = 0;
    AJ_ASSERT(duk_is_object(ctx, idx));
    duk_enum(ctx, idx, DUK_ENUM_OWN_PROPERTIES_ONLY);
    while (duk_next(ctx, -1, 0)) {
        duk_pop(ctx);
        ++num;
    }
    duk_pop(ctx); // enum
    return num;
}

void AJS_DumpJX(duk_context* ctx, const char* tag, duk_idx_t idx)
{
    if (dbgAJS) {
        if (tag) {
            AJ_AlwaysPrintf(("%s\n", tag));
        }
        idx = duk_normalize_index(ctx, idx);
        duk_push_global_object(ctx);
        duk_get_prop_string(ctx, -1, "Duktape");
        duk_get_prop_string(ctx, -1, "enc");
        duk_push_string(ctx, "jx");
        duk_dup(ctx, idx);
        duk_pcall(ctx, 2);
        AJ_AlwaysPrintf(("%s\n", duk_get_string(ctx, -1)));
        duk_pop_3(ctx);
    }
}

void AJS_DumpStack(duk_context* ctx)
{
    if (dbgAJS) {
        duk_idx_t top = duk_get_top_index(ctx);

        AJS_DumpJX(ctx, "===== Duktape Stack =====", top);
        while (--top) {
            AJS_DumpJX(ctx, NULL, top);
        }
        AJ_AlwaysPrintf(("=========================\n"));
    }
}
