#ifndef _AJS_DUK_H
#define _AJS_DUK_H
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

/**
 * Function to compress pointers to 16 bits on constrained memory devices with less that 256K heaps.
 *
 * @param userData  The user data value associated with the memory allocator for this heap
 * @param ptr       The pointer to compress
 *
 * @return  A 16bit compressed pointer value.
 */
duk_uint16_t AJS_EncodePtr16(void* userData, const void*ptr);

/**
 * Partner function to AJS_EncodePtr16 to decode a compressed pointer value and return the actuall
 * memory address.
 *
 * @param userData  The user data value associated with the memory allocator for this heap
 * @param enc       The encoded value to decode.
 *
 * @return  The decode pointer.
 */
void* AJS_DecodePtr16(void* userData, duk_uint16_t enc);

/**
 * Check is a string literal is stored externally. This is an optimization for memory constrained
 * devices where built-in and other strings can be stored in read-only memory.
 *
 * @param userData  The user data value associated with the memory allocator for this heap
 * @param ptr   Pointer to start of string value being checked - string is not NUL terminated.
 * @param len   Length of the string.
 *
 * @return    Returns a pointer to the external string or NULL if the string is not externally
 *            available.
 */
const void* AJS_ExternalStringCheck(void* userData, const void* ptr, duk_size_t len);

/**
 * Releases an externally stored string. This function will usually be a no-op.
 *
 * @param userData  The user data value associated with the memory allocator for this heap
 * @param extStr    Pointer to an externally stored string previously returned by
 *                  AJS_ExternalStringCheck()
 */
void AJS_ExternalStringFree(void* userData, const void* extStr);

/**
 * If execution timeout checks are enabled this function is called periodically by the duktape
 * bytecode engine to allow the C application environment to break out of infinite loops or other
 * code that is executing longer than expected by throwing an exception.
 *
 * @param udata  Pointer to the user data for the heap that the code is being executed on.
 *
 * @return  Return 0 if execution can proceed, returning a non-zero value will cause the currently
 * executing JS codeo to throw a RANGE_ERROR exception.
 */
int AJS_ExecTimeoutCheck(const void*udata);

#endif