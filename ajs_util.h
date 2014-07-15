#ifndef _AJS_UTIL_H
#define _AJS_UTIL_H
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

/**
 * Controls debug output for this module
 */
#ifndef NDEBUG
extern uint8_t dbgAJS;
#endif

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
 *
 * @return  AJ_OK or an error status code
 */
AJ_Status AJS_SetPropertyAccessors(duk_context* ctx, duk_idx_t objIdx, const char* prop, duk_c_function setter, duk_c_function getter);

/**
 * Get a string property from the object at the top of the stack. This is a convenience function
 * that leaves the duktape stack unchanged.
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
 * Get a global stash array creating it if needed. Stashed array is on the top of the stack when
 * this function returns.
 *
 * @param ctx   An opaque pointer to a duktape context structure
 * @param name  The name of the array to get
 */

void AJS_GetGlobalStashArray(duk_context* ctx, const char* name);

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
 * Count the number of enumerable properties on the object on the duktape stack
 *
 * @param ctx   An opaque pointer to a duktape context structure
 * @param idx   An index on the duktape stack
 */
size_t AJS_NumProps(duk_context* ctx, duk_idx_t idx);

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

#endif
