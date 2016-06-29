#ifndef _AJS_TRANSLATIONS_H
#define _AJS_TRANSLATIONS_H
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
 * Lookup a string and find a translation if one exists
 *
 * @param ctx  An opaque pointer to a duktape context structure
 * @param idx  The index on the stack for the string to be translated
 * @param lang The chosen language
 */
const char* AJS_GetTranslatedString(duk_context* ctx, duk_idx_t idx, uint16_t lang);

/**
 * Lookup a string and find a translation to the current default language if one exists
 *
 * @param ctx  An opaque pointer to a duktape context structure
 * @param idx  The index on the stack for the string to be translated
 */
const char* AJS_GetDefaultTranslatedString(duk_context* ctx, duk_idx_t idx);

/**
 * Get the language name for a give language index
 *
 * @param ctx        An opaque pointer to a duktape context structure
 * @param langIndex  The index to lookup.
 *
 * @return  The language name or the default language
 */
const char* AJS_GetLanguageName(duk_context* ctx, uint8_t langIndex);

/**
 * Get the language index for a give language name
 *
 * @param ctx       An opaque pointer to a duktape context structure
 * @param langName  The name to lookup.
 *
 * @return The language index or 0 if the language is unknown.
 */
uint8_t AJS_GetLanguageIndex(duk_context* ctx, const char* langName);

/**
 * Get the number of languages supported
 *
 * @param  ctx  An opaque pointer to a duktape context structure
 * @return The number of supported languages - never less than 1
 */
uint8_t AJS_GetNumberOfLanguages(duk_context* ctx);

#ifdef __cplusplus
}
#endif

#endif