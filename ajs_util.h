#ifndef _AJS_UTIL_H
#define _AJS_UTIL_H
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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Controls debug output for this module
 */
#ifndef NDEBUG
extern uint8_t dbgAJS;
#endif

/*
 * Watchdog timer for detecting infinite loops
 */
#define AJS_DEFAULT_WATCHDOG_TIMEOUT   (10 * 1000)

/*
 * Prefixing a property name with 0xFF makes it a hidden property
 */
#define AJS_HIDDEN_PROP(n) "\377" n

/**
 * Put a list of functions. This is very similar to the duktape duk_put_function_list() helper API but
 * allows the caller to specify if the functions being pushed are lightfuncs or regular funct.
 *
 * @param ctx      An opaque pointer to a duktape context structure
 * @param objIdx   Index on dulktape stack for the object
 * @param funcList The functions to push
 */
void AJS_PutFunctionList(duk_context* ctx, duk_idx_t objIdx, const duk_function_list_entry*funcList, uint8_t light);

/**
 * Register a finalizer on an object
 *
 * @param ctx      An opaque pointer to a duktape context structure
 * @param objIdx   Index on dulktape stack for the object
 * @param finFunc  Finalizer function to be called when the object is freed
 */
void AJS_RegisterFinalizer(duk_context* ctx, duk_idx_t objIdx, duk_c_function finFunc);

/**
 * This function does the equivalent of the following JavaScript
 *
 * Object.createObject(<protoype>)
 *
 * Leave the newly created object on the top of the duktape stack
 *
 * @param ctx      An opaque pointer to a duktape context structure
 * @param protoIdx Index on dulktape stack for the prototype object
 *
 * @return  Returns the index of newly created object
 */
int AJS_CreateObjectFromPrototype(duk_context* ctx, duk_idx_t protoIdx);

/**
 * This function does the equivalent of the following JavaScript
 *
 * obj.intProp += 1
 *
 * @param ctx      An opaque pointer to a duktape context structure
 * @param intProp  Name of the property to increment
 * @param objIndex Index on dulktape stack for the object
 *
 * @return  Returns the incremented value of the property
 */
int AJS_IncrementProperty(duk_context* ctx, const char* intProp, duk_idx_t objIdx);

/**
 * This function does the equivalent of the following JavaScript
 *
 * Object.defineProperty(<obj>, <prop>, { set: function() { ... }, get: function() { ... } });
 *
 * @param ctx      An opaque pointer to a duktape context structure
 * @param objIdx   Index on duktape stack for the object
 * @param prop     The name of the property
 * @param setter   The native setter function for the property - can be NULL
 * @param getter   The native getter function for the property - can be NULL
 */
void AJS_SetPropertyAccessors(duk_context* ctx, duk_idx_t objIdx, const char* prop, duk_c_function setter, duk_c_function getter);

/**
 * Get a string property from the object on the stack. This is a convenience function that leaves
 * the duktape stack unchanged. Note that the returned string is only guaranteed to be live while
 * the object remains on the stack.
 *
 * @param ctx   An opaque pointer to a duktape context structure
 * @param idx   Index on the duktape stack of object with the required property
 * @param prop  The property to get
 *
 * @return The string property
 */
const char* AJS_GetStringProp(duk_context* ctx, duk_idx_t idx, const char* prop);

/**
 * Get an integer property from the object at the top of the stack. This is a convenience function
 * that leaves the duktape stack unchanged.
 *
 * @param ctx   An opaque pointer to a duktape context structure
 * @param idx   Index on the duktape stack of object with the required property
 * @param prop  The property to get
 *
 * @return The integer property
 */
uint32_t AJS_GetIntProp(duk_context* ctx, duk_idx_t idx, const char* prop);

/**
 * Get a global stash object creating it if needed. Stashed object is on the top of the stack when
 * this function returns.
 *
 * @param ctx   An opaque pointer to a duktape context structure
 * @param name  The name of the object to get
 */
void AJS_GetGlobalStashObject(duk_context* ctx, const char* name);

/**
 * Clear a global stash object
 *
 * @param ctx   An opaque pointer to a duktape context structure
 * @param name  Name of the object to clean
 */
void AJS_ClearGlobalStashObject(duk_context* ctx, const char* name);

/**
 * Get a global stash array creating it if needed. Stashed array is on the top of the stack when
 * this function returns.
 *
 * @param ctx   An opaque pointer to a duktape context structure
 * @param name  The name of the array to get
 */

void AJS_GetGlobalStashArray(duk_context* ctx, const char* name);

/**
 * Temporarily stabilize a string by storing it in the global stash. The returned string pointer
 * will remain stable until AJS_ClearPinnedObjects() is called.
 *
 * @param ctx   An opaque pointer to a duktape context structure
 * @param idx   Duktape stack index for the string to be stabilized
 *
 * @return  Returns a pointer to the stable string.
 */
const char* AJS_PinString(duk_context* ctx, duk_idx_t idx);

/**
 * Temporarily stabilize a buffer by storing it in the global stash. The returned pointer
 * will remain stable until AJS_ClearPinnedObjects() is called.
 *
 * @param ctx   An opaque pointer to a duktape context structure
 * @param idx   Duktape stack index for the buffer to be stabilized
 *
 * @return  Returns a pointer to the stable buffer pointer.
 */
void* AJS_PinBuffer(duk_context* ctx, duk_idx_t idx);

/**
 * Clear the all pinned strings and buffers. Pointers to previous pinned items become invalid.
 *
 * @param ctx   An opaque pointer to a duktape context structure
 */
void AJS_ClearPins(duk_context* ctx);

/**
 * Get a property of the global AllJoyn object.
 *
 * @param ctx   An opaque pointer to a duktape context structure
 * @param prop  The name of the property to get
 *
 * @return  The absolute stack index of the property
 */
duk_idx_t AJS_GetAllJoynProperty(duk_context* ctx, const char* prop);

/**
 * This function does the equivalent of the following JavaScript
 *
 * Object.freeze(<obj>);
 *
 * @param ctx      An opaque pointer to a duktape context structure
 * @param objIdx   Index on duktape stack for the object
 */
void AJS_ObjectFreeze(duk_context* ctx, duk_idx_t objIdx);

/**
 * Enable the watchdog timer
 */
void AJS_EnableWatchdogTimer(void);

/**
 * Disable the watchdog timer.
 * Warning: This disables protection against malicious scripts
 * that could cause infinite loops.
 */
void AJS_DisableWatchdogTimer(void);

/**
 * Set the watchdog timer to trap infinite loops or use of blocking calls.
 *
 * @param timeout  Watchdog timer timeout value in milliseconds.
 */
void AJS_SetWatchdogTimer(uint32_t timeout);

/**
 * Clear the current watchdog timer.
 */
void AJS_ClearWatchdogTimer();

/**
 * Clones the parts of a message needed for a reply and closes the message. The returned message
 * pointer is "pinned" so is valid for until control returns the message loop level.
 *
 * @param ctx   An opaque pointer to a duktape context structure
 * @param msg   Message to be cloned and closed
 * @return      Partial message structure
 */
AJ_Message* AJS_CloneAndCloseMessage(duk_context* ctx, AJ_Message* msg);

/**
 * Prints out the JSON for an object on the duktape stack
 *
 * @param ctx   An opaque pointer to a duktape context structure
 * @param tag   Debug tag
 * @param idx   An index on the duktape stack
 */
void AJS_DumpJX(duk_context* ctx, const char* tag, duk_idx_t idx);

/**
 * Dump the current duktape stack
 *
 * @param ctx   An opaque pointer to a duktape context structure
 */
void AJS_DumpStack(duk_context* ctx);

/**
 * Dump the global object
 *
 * @param ctx   An opaque pointer to a duktape context structure
 */
#define AJS_DumpGlobalObject(ctx) \
    do { \
        duk_push_global_object(ctx); \
        AJS_DumpJX(ctx, "GlobalObject", -1); \
        duk_pop(ctx); \
    } while (0)

/**
 * Dump the global stash
 *
 * @param ctx   An opaque pointer to a duktape context structure
 */
#define AJS_DumpGlobalStash(ctx) \
    do { \
        duk_push_global_stash(ctx); \
        AJS_DumpJX(ctx, "GlobalStash", -1); \
        duk_pop(ctx); \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif