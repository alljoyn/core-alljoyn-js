/**
 * @file
 */
/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

static const char** languagesList;

static const char* GetTranslatedString(duk_context* ctx, duk_idx_t idx, uint16_t langNum, const char* langName)
{
    const char* translation = NULL;
    const char* original = duk_get_string(ctx, idx);

    if (!original) {
        return "???";
    }

    AJ_InfoPrintf(("Looking up translation for %s\n", original));

    AJS_GetGlobalStashObject(ctx, "translations");
    if (duk_is_object(ctx, -1)) {
        /*
         * Find out which language and then look up a translation
         */
        if (langName) {
            duk_push_string(ctx, langName);
        } else {
            AJS_GetAllJoynProperty(ctx, "languages");
            if (duk_is_array(ctx, -1)) {
                duk_get_prop_index(ctx, -1, langNum);
                duk_remove(ctx, -2);
            }
        }
        /*
         * If we got a language see if there is a translation
         */
        if (duk_is_string(ctx, -1)) {
            duk_get_prop(ctx, -2);
            if (duk_is_object(ctx, -1)) {
                duk_get_prop_string(ctx, -1, original);
                translation = duk_get_string(ctx, -1);
                duk_pop(ctx);
            }
            duk_pop(ctx);
        }
        duk_pop(ctx);
    }
    duk_pop(ctx);

    if (translation && *translation) {
        return translation;
    } else {
        return original;
    }
}

const char* AJS_GetTranslatedString(duk_context* ctx, duk_idx_t idx, uint16_t langNum)
{
    return GetTranslatedString(ctx, idx, langNum, NULL);
}

/*
 * Translations is an object where the properties are arrays of translation strings. The property
 * names are the respective languages.
 */
static int NativeTranslationsSetter(duk_context* ctx)
{
    duk_uarridx_t numLangs = 0;

    if (!duk_is_object(ctx, 0)) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Translations requires an object");
    }
    /*
     * Global languages list to be registered with AJ_RegisterDescriptionLanguages()
     */
    languagesList = duk_alloc(ctx, sizeof(const char*) * 2);
    /*
     * Array for languages
     */
    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, "AJ");
    duk_push_array(ctx);
    /*
     * Count the languages and accumulate them in an array
     */
    duk_enum(ctx, 0, DUK_ENUM_OWN_PROPERTIES_ONLY);
    while (duk_next(ctx, -1, 0)) {
        languagesList = duk_realloc(ctx, languagesList, sizeof(const char*) * (numLangs + 1));
        languagesList[numLangs] = duk_get_string(ctx, -1);
        duk_put_prop_index(ctx, -3, numLangs);
        ++numLangs;
    }
    languagesList[numLangs] = NULL;
    duk_pop(ctx); // enum
    /*
     * Add the languages array to the AllJoyn object
     */
    duk_put_prop_string(ctx, -2, "languages");
    duk_pop_2(ctx);
    /*
     * The translation array is stored in the global stash
     */
    duk_push_global_stash(ctx);
    duk_dup(ctx, 0);
    duk_put_prop_string(ctx, -2, "translations");
    duk_pop(ctx);

    AJ_RegisterDescriptionLanguages(languagesList);
    return 0;
}

static int NativeTranslate(duk_context* ctx)
{
    const char* trans = NULL;

    if (duk_is_number(ctx, 1)) {
        trans = GetTranslatedString(ctx, 0, duk_get_int(ctx, 1), NULL);
    } else if (duk_is_string(ctx, 1)) {
        trans = GetTranslatedString(ctx, 0, 0, duk_get_string(ctx, 1));
    }
    duk_push_string(ctx, trans);
    return 1;
}

void AJS_RegisterTranslations(duk_context* ctx, duk_idx_t ajIdx)
{
    /*
     * Setter for handling language translations
     */
    AJS_SetPropertyAccessors(ctx, ajIdx, "translations", NativeTranslationsSetter, NULL);
    duk_push_c_function(ctx, NativeTranslate, 2);
    duk_put_prop_string(ctx, ajIdx, "translate");
}

const char* AJS_GetLanguageName(duk_context* ctx, uint8_t langIndex)
{
    const char* langName = NULL;

    AJS_GetAllJoynProperty(ctx, "languages");
    if (duk_is_array(ctx, -1)) {
        duk_get_prop_index(ctx, -1, langIndex);
        langName = duk_get_string(ctx, -1);
        duk_pop(ctx);
    }
    duk_pop(ctx);
    if (!langName) {
        AJS_GetAllJoynProperty(ctx, "defaultLanguage");
        langName = duk_get_string(ctx, -1);
        duk_pop(ctx);
    }
    return langName;
}

uint8_t AJS_GetLanguageIndex(duk_context* ctx, const char* langName)
{
    uint8_t i;

    AJS_GetAllJoynProperty(ctx, "languages");
    if (duk_is_array(ctx, -1)) {
        i = 0;
    } else {
        duk_uarridx_t num = duk_get_length(ctx, -1);
        for (i = 0; i < num; ++i) {
            const char* str;
            duk_get_prop_index(ctx, -1, i);
            str = duk_get_string(ctx, -1);
            duk_pop(ctx);
            if (strcmp(langName, str) == 0) {
                break;
            }
        }
        if (i == num) {
            i = 0;
        }
    }
    duk_pop(ctx);
    return i;
}

uint8_t AJS_GetNumberOfLanguages(duk_context* ctx)
{
    uint8_t num;
    AJS_GetAllJoynProperty(ctx, "languages");
    num = duk_get_length(ctx, -1);
    duk_pop(ctx);
    return num ? num : 1;
}
