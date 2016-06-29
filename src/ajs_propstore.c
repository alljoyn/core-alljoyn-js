/******************************************************************************
 *  *    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
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
#include "ajs_util.h"
#include "ajs_propstore.h"
#include "ajs_translations.h"
#include <ajtcl/services/PropertyStore.h>
#include <ajtcl/services/ServicesCommon.h>
#ifdef ONBOARDING_SERVICE
#include <ajtcl/services/OnboardingManager.h>
#endif
#include <ajtcl/aj_guid.h>
#include <ajtcl/aj_nvram.h>
#include <ajtcl/aj_creds.h>
#include <ajtcl/aj_config.h>

#ifndef CONFIG_SERVICE
#error CONFIG_SERVICE is required
#endif

/*
 * Compute NVRAM id for a given property key
 */
#define NVRAM_ID(key)  ((uint16_t)(key) + AJ_PROPERTIES_NV_ID_BEGIN)

/*
 * Maximum length of a predefined property string
 */
#define MAX_PROP_LENGTH     32

#define  P_READONLY  0x01
#define  P_ANNOUNCE  0x02
#define  P_LOCALIZED 0x04
#define  P_PRIVATE   0x08

/*
 * property structure
 */
typedef struct {
    const char* keyName;      /* The property key name as shown in About and Config documentation */
    uint8_t flags;
    const char* initialValue;
} PropertyStoreEntry;

#define FIRST_PROPERTY_INDEX (0)

static duk_context* duktape;

/*
 * Default property strings
 */
static const char DEFAULT_PASSCODE_CLEAN[]      = "000000";
static const char DEFAULT_PASSCODE[]            = "303030303030"; // HEX encoded { '0', '0', '0', '0', '0', '0' }
static const char DEFAULT_APP_NAME[]            = "AllJoyn.js";
static const char DEFAULT_DESCRIPTION[]         = "AllJoyn.js";
static const char DEFAULT_MANUFACTURER[]        = "AllSeen Alliance";
static const char DEFAULT_DEVICE_MODEL[]        = "0.0.1";
static const char DEFAULT_DATE_OF_MANUFACTURE[] = "2014-09-03";
static const char DEFAULT_SOFTWARE_VERSION[]    = "0.0.1";
static const char DEFAULT_HARDWARE_VERSION[]    = "0.0.1";
static const char DEFAULT_SUPPORT_URL[]         = "www.allseenalliance.org";
static const char DEFAULT_LANGUAGE[]            = "en";

#define IS_VALID_FIELD_ID(f) \
    (((f) > AJSVC_PROPERTY_STORE_ERROR_FIELD_INDEX) && ((f) < AJSVC_PROPERTY_STORE_NUMBER_OF_KEYS))

static PropertyStoreEntry propDefs[AJSVC_PROPERTY_STORE_NUMBER_OF_KEYS] =
{
    /* config keys */
    { "DeviceId",          P_READONLY | P_ANNOUNCE,                NULL },
    { "AppId",             P_READONLY | P_ANNOUNCE,                NULL },
    { "DeviceName",        P_ANNOUNCE | P_LOCALIZED,               NULL },
    { "DefaultLanguage",   P_ANNOUNCE,                             DEFAULT_LANGUAGE },
    { "Passcode",          P_PRIVATE,                              DEFAULT_PASSCODE },
    { "RealmName",         P_PRIVATE,                              NULL },
    /* regular keys */
    { "AppName",           P_ANNOUNCE,                             DEFAULT_APP_NAME },
    { "Description",       P_LOCALIZED,                            DEFAULT_DESCRIPTION },
    { "Manufacturer",      P_ANNOUNCE | P_LOCALIZED,               DEFAULT_MANUFACTURER },
    { "ModelNumber",       P_ANNOUNCE,                             DEFAULT_DEVICE_MODEL },
    { "DateOfManufacture", P_READONLY,                             DEFAULT_DATE_OF_MANUFACTURE },
    { "SoftwareVersion",   P_READONLY,                             DEFAULT_SOFTWARE_VERSION },
    { "AJSoftwareVersion", P_READONLY,                             NULL },
    { "MaxLength",         P_READONLY | P_ANNOUNCE,                NULL },
    /* optional keys */
    { "HardwareVersion",   P_READONLY,                             DEFAULT_HARDWARE_VERSION },
    { "SupportUrl",        P_LOCALIZED,                            DEFAULT_SUPPORT_URL },
};

AJ_Status AJS_GetAllJoynAboutProps(duk_context* ctx, duk_idx_t aboutIdx)
{
    duk_enum(ctx, aboutIdx, DUK_ENUM_OWN_PROPERTIES_ONLY);
    const char* value = NULL;
    const char* aboutProp = NULL;
    int8_t fieldIndex = AJSVC_PROPERTY_STORE_ERROR_FIELD_INDEX;
    while (duk_next(ctx, -1, 1)) {
        aboutProp = duk_require_string(ctx, -2);
        fieldIndex =  AJSVC_PropertyStore_GetFieldIndex(aboutProp);
        if (fieldIndex != AJSVC_PROPERTY_STORE_ERROR_FIELD_INDEX) {
            if (duk_is_string(ctx, -1)) {
                propDefs[fieldIndex].initialValue = duk_require_string(ctx, -1);
                duk_pop(ctx);
            } else {
                duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
                if (duk_has_prop_string(ctx, -1, "access")) {
                    propDefs[fieldIndex].flags = AJS_GetIntProp(ctx, -2, "access");
                }
                propDefs[fieldIndex].initialValue = AJS_GetStringProp(ctx, -2, "value");
                duk_pop_2(ctx);
            }
        }
        duk_pop(ctx);
    }
    duk_pop(ctx);
}

static const char* SupportedLanguagesKey = "SupportedLanguages";

uint8_t AJSVC_PropertyStore_GetMaxValueLength(int8_t field)
{
    return MAX_PROP_LENGTH;
}

const char* AJSVC_PropertyStore_GetFieldName(int8_t field)
{
    return IS_VALID_FIELD_ID(field) ? propDefs[field].keyName : "N/A";
}

int8_t AJSVC_PropertyStore_GetFieldIndex(const char* fieldName)
{
    int8_t field;

    for (field = FIRST_PROPERTY_INDEX; field < AJSVC_PROPERTY_STORE_NUMBER_OF_KEYS; field++) {
        if (strcmp(propDefs[field].keyName, fieldName) == 0) {
            return field;
        }
    }
    return AJSVC_PROPERTY_STORE_ERROR_FIELD_INDEX;
}

static const char* PeekProp(int8_t field)
{
    duk_context* ctx = duktape;
    const char* prop;
    AJ_NV_DATASET* handle = AJ_NVRAM_Open(NVRAM_ID(field), "r", 0);
    if (handle) {
        duk_push_string(ctx, AJ_NVRAM_Peek(handle));
        prop = AJS_PinString(ctx, -1);
        duk_pop(ctx);
        AJ_NVRAM_Close(handle);
        return prop;
    } else {
        return NULL;
    }
}

static uint8_t PropChanged(int8_t field, const char* str)
{
    uint8_t changed = TRUE;
    AJ_NV_DATASET* handle = AJ_NVRAM_Open(NVRAM_ID(field), "r", 0);
    if (handle) {
        const char* prop = AJ_NVRAM_Peek(handle);
        if (prop && (strcmp(prop, str) == 0)) {
            changed = FALSE;
        }
        AJ_NVRAM_Close(handle);
    }
    return changed;
}

const char* AJSVC_PropertyStore_GetLanguageName(int8_t index)
{
    duk_context* ctx = duktape;
    const char* language = NULL;

    if (index < AJS_GetNumberOfLanguages(ctx)) {
        language = AJS_GetLanguageName(ctx, index);
    } else {
        language = PeekProp(AJSVC_PROPERTY_STORE_DEFAULT_LANGUAGE);
    }
    return language ? language : DEFAULT_LANGUAGE;
}

const char* AJSVC_PropertyStore_GetValueForLang(int8_t field, int8_t index)
{
    const char* val = NULL;
    const char* lang;
    duk_context* ctx = duktape;
    AJ_NV_DATASET* handle;

    if (!IS_VALID_FIELD_ID(field)) {
        return NULL;
    }
    if (propDefs[field].flags & P_PRIVATE) {
        return NULL;
    }
    if (!(propDefs[field].flags & P_LOCALIZED)) {
        return PeekProp(field);
    }
    lang = AJSVC_PropertyStore_GetLanguageName(index);
    /*
     * Expect JSON of form { <lang>:<string>, <lang>:<string>, ... }
     */
    handle = AJ_NVRAM_Open(NVRAM_ID(field), "r", 0);
    if (handle) {
        duk_push_string(ctx, AJ_NVRAM_Peek(handle));
        AJ_NVRAM_Close(handle);
        duk_json_decode(ctx, -1);
        /*
         * First try lookup the language requested
         * Second try lookup the current default language
         * Third try lookup the compile time default language
         */
        duk_get_prop_string(ctx, -1, lang);
        if (!duk_is_string(ctx, -1)) {
            duk_pop(ctx);
            lang = PeekProp(AJSVC_PROPERTY_STORE_DEFAULT_LANGUAGE);
            duk_get_prop_string(ctx, -1, lang);
            if (!duk_is_string(ctx, -1)) {
                duk_pop(ctx);
                lang = DEFAULT_LANGUAGE;
                duk_get_prop_string(ctx, -1, lang);
            }
        }
        if (duk_is_string(ctx, -1)) {
            /*
             * Stabilize the string
             */
            val = AJS_PinString(ctx, -1);
        }
        duk_pop_2(ctx);
    }
    return val;
}

const char* AJSVC_PropertyStore_GetValue(int8_t field)
{
    return AJSVC_PropertyStore_GetValueForLang(field, AJS_GetCurrentLanguage());
}

int8_t AJSVC_PropertyStore_GetLanguageIndex(const char* const language)
{
    return (int8_t)AJS_GetLanguageIndex(duktape, language);
}

uint8_t AJSVC_PropertyStore_SetValue(int8_t field, const char* value)
{
    return AJSVC_PropertyStore_SetValueForLang(field, AJS_GetCurrentLanguage(), value);
}

uint8_t AJS_PropertyStore_SetPasscode(const char* passcode)
{
    return AJSVC_PropertyStore_SetValue("Passcode", passcode);
}

/*
 * If mandatory properties have not been initialize set them to hardcoded default values
 */
static void InitProperties(const char* deviceName, uint8_t force)
{
    AJ_GUID guid;
    char buffer[MAX_PROP_LENGTH + 1];
    int8_t field;
    AJ_GetLocalGUID(&guid);
    AJ_GUID_ToString(&guid, buffer, sizeof(buffer));

    /*
     * Special case initialization of device name
     */
    if (deviceName) {
        AJSVC_PropertyStore_SetValue(AJSVC_PROPERTY_STORE_DEVICE_NAME, deviceName);
    }

    /*
     * Set the default passcode for the auth listener
     */
    AJS_SetDefaultPasscode(DEFAULT_PASSCODE_CLEAN);
    /*
     * Initialize properties that have not been set
     */
    for (field = FIRST_PROPERTY_INDEX; field < AJSVC_PROPERTY_STORE_NUMBER_OF_KEYS; ++field) {
        uint8_t ok = TRUE;
        /*
         * Don't overwrite existing value unless force is TRUE
         */
        if (!force && AJ_NVRAM_Exist(NVRAM_ID(field))) {
            continue;
        }
        switch (field) {
        case AJSVC_PROPERTY_STORE_APP_ID:
            ok = AJSVC_PropertyStore_SetValue(AJSVC_PROPERTY_STORE_APP_ID, buffer);
            break;

        case AJSVC_PROPERTY_STORE_DEVICE_ID:
            ok = AJSVC_PropertyStore_SetValue(AJSVC_PROPERTY_STORE_DEVICE_ID, buffer);
            break;

        case AJSVC_PROPERTY_STORE_DEVICE_NAME:
            memcpy(buffer, "AllJoyn.js.", 11);
            buffer[18] = 0;
            ok = AJSVC_PropertyStore_SetValue(AJSVC_PROPERTY_STORE_DEVICE_NAME, buffer);
            break;

        default:
            if (propDefs[field].initialValue) {
                ok = AJSVC_PropertyStore_SetValue(field, propDefs[field].initialValue);
            }
            break;
        }
        if (!ok) {
            AJ_WarnPrintf(("Failed to initialize property %s\n", AJSVC_PropertyStore_GetFieldName(field)));
        }
    }
}

/*
 * This function is registered with About and handles property store read requests
 */
static AJ_Status AboutPropGetter(AJ_Message* msg, const char* language)
{
    AJ_Status status = AJ_ERR_INVALID;
    int8_t lang;
    AJSVC_PropertyStoreCategoryFilter filter;

    memset(&filter, 0, sizeof(AJSVC_PropertyStoreCategoryFilter));

    if (msg->msgId == AJ_SIGNAL_ABOUT_ANNOUNCE) {
        filter.bit2Announce = TRUE;
        lang = AJSVC_PropertyStore_GetLanguageIndex(language);
        status = AJ_OK;
    } else if (msg->msgId == AJ_REPLY_ID(AJ_METHOD_ABOUT_GET_ABOUT_DATA)) {
        filter.bit0About = TRUE;
        lang = AJSVC_PropertyStore_GetLanguageIndex(language);
        status = (lang == AJSVC_PROPERTY_STORE_ERROR_LANGUAGE_INDEX) ? AJ_ERR_UNKNOWN : AJ_OK;
    }
    if (status == AJ_OK) {
        status = AJSVC_PropertyStore_ReadAll(msg, filter, lang);
    }
    return status;
}

AJ_Status AJS_PropertyStoreInit(duk_context* ctx, const char* deviceName)
{
    duktape = ctx;
    AJS_GetAllJoynProperty(ctx, "aboutDefinition");
    if (!duk_is_undefined(ctx, -1) && NumProps(ctx, -1) > 0) {
        AJS_GetAllJoynAboutProps(ctx, duk_get_top_index(ctx));
        duk_pop(ctx);
    }
    InitProperties(deviceName, FALSE);
    AJ_AboutRegisterPropStoreGetter(AboutPropGetter);
    return AJ_OK;
}

AJ_Status AJSVC_PropertyStore_LoadAll()
{
    return AJ_OK;
}

AJ_Status AJSVC_PropertyStore_SaveAll()
{
    AJ_AboutSetShouldAnnounce();
    return AJ_OK;
}

uint8_t AJS_PropertyStore_IsReadOnly(int8_t field)
{
    return !IS_VALID_FIELD_ID(field) || (propDefs[field].flags & (P_READONLY | P_PRIVATE));
}

AJ_Status AJSVC_PropertyStore_ReadAll(AJ_Message* msg, AJSVC_PropertyStoreCategoryFilter filter, int8_t lang)
{
    AJ_Status status;
    AJ_Arg array;
    AJ_Arg dict;
    int8_t field;

    AJ_InfoPrintf(("PropertyStore_ReadAll()\n"));

    status = AJ_MarshalContainer(msg, &array, AJ_ARG_ARRAY);
    if (status != AJ_OK) {
        return status;
    }

    for (field = FIRST_PROPERTY_INDEX; field < AJSVC_PROPERTY_STORE_NUMBER_OF_KEYS; field++) {

        if (propDefs[field].flags & P_PRIVATE) {
            continue;
        }
        if (filter.bit0About || (filter.bit1Config && !(propDefs[field].flags & P_READONLY)) || (filter.bit2Announce && (propDefs[field].flags & P_ANNOUNCE))) {
            const char* value;

            switch (field) {
            case AJSVC_PROPERTY_STORE_APP_ID:
                status = AJ_MarshalContainer(msg, &dict, AJ_ARG_DICT_ENTRY);
                if (status != AJ_OK) {
                    goto ExitReadAll;
                }
                status = AJ_MarshalArgs(msg, "s", propDefs[field].keyName);
                if (status != AJ_OK) {
                    goto ExitReadAll;
                }
                value = AJSVC_PropertyStore_GetValueForLang(field, lang);
                if (!value) {
                    status = AJ_ERR_INVALID;
                    goto ExitReadAll;
                }
                status = AJSVC_MarshalAppIdAsVariant(msg, value);
                if (status != AJ_OK) {
                    goto ExitReadAll;
                }
                status = AJ_MarshalCloseContainer(msg, &dict);
                break;

            case AJSVC_PROPERTY_STORE_MAX_LENGTH:
                status = AJ_MarshalArgs(msg, "{sv}", propDefs[field].keyName, "q", MAX_PROP_LENGTH);
                break;

            case AJSVC_PROPERTY_STORE_AJ_SOFTWARE_VERSION:
                status = AJ_MarshalArgs(msg, "{sv}", propDefs[field].keyName, "s", AJ_GetVersion());
                break;

            default:
                value = AJSVC_PropertyStore_GetValueForLang(field, lang);
                if (!value) {
                    AJ_WarnPrintf(("PropertyStore_ReadAll - No value for field[%d] %s lang:%s\n", field, propDefs[field].keyName, AJSVC_PropertyStore_GetLanguageName(lang)));
                } else {
                    status = AJ_MarshalArgs(msg, "{sv}", propDefs[field].keyName, "s", value);
                }
            }
            if (status != AJ_OK) {
                goto ExitReadAll;
            }
        }
    }
    if (status != AJ_OK) {
        goto ExitReadAll;
    }
    /*
     * About returns the array of supported languages
     */
    if (filter.bit0About) {
        uint8_t i;
        AJ_Arg langs;
        uint8_t numLangs = AJS_GetNumberOfLanguages(duktape);
        status = AJ_MarshalContainer(msg, &dict, AJ_ARG_DICT_ENTRY);
        if (status != AJ_OK) {
            goto ExitReadAll;
        }
        status = AJ_MarshalArgs(msg, "s", SupportedLanguagesKey);
        if (status != AJ_OK) {
            goto ExitReadAll;
        }
        status = AJ_MarshalVariant(msg, "as");
        if (status != AJ_OK) {
            goto ExitReadAll;
        }
        status = AJ_MarshalContainer(msg, &langs, AJ_ARG_ARRAY);
        if (status != AJ_OK) {
            goto ExitReadAll;
        }
        for (i = 0; i < numLangs; ++i) {
            status = AJ_MarshalArgs(msg, "s", AJS_GetLanguageName(duktape, i));
            if (status != AJ_OK) {
                goto ExitReadAll;
            }
        }
        status = AJ_MarshalCloseContainer(msg, &langs);
        if (status != AJ_OK) {
            goto ExitReadAll;
        }
        status = AJ_MarshalCloseContainer(msg, &dict);
        if (status != AJ_OK) {
            goto ExitReadAll;
        }
    }
    status = AJ_MarshalCloseContainer(msg, &array);

ExitReadAll:

    return status;
}

AJ_Status AJSVC_PropertyStore_Update(const char* key, int8_t lang, const char* value)
{
    int8_t field = AJSVC_PropertyStore_GetFieldIndex(key);
    return AJSVC_PropertyStore_SetValueForLang(field, lang, value) ? AJ_OK : AJ_ERR_INVALID;
}

uint8_t AJSVC_PropertyStore_SetValueForLang(int8_t field, int8_t lang, const char* value)
{
    duk_context* ctx = duktape;
    const char* str = value;

    if (field == AJSVC_PROPERTY_STORE_ERROR_FIELD_INDEX) {
        return FALSE;
    }
    if (propDefs[field].flags & P_LOCALIZED) {
        AJ_NV_DATASET* handle = AJ_NVRAM_Open(NVRAM_ID(field), "r", 0);
        if (handle) {
            duk_push_string(ctx, AJ_NVRAM_Peek(handle));
            duk_json_decode(ctx, -1);
            AJ_NVRAM_Close(handle);
        } else {
            duk_push_object(ctx);
        }
        duk_push_string(ctx, value);
        duk_put_prop_string(ctx, -2, AJSVC_PropertyStore_GetLanguageName(lang));
        duk_json_encode(ctx, -1);
        str = duk_get_string(ctx, -1);
    }
    /*
     * Avoid setting a property to the same value
     */
    if (PropChanged(field, str)) {
        uint16_t len = strlen(str) + 1;
        AJ_NV_DATASET* handle = AJ_NVRAM_Open(NVRAM_ID(field), "w", len);
        if (handle) {
            AJ_NVRAM_Write(str, len, handle);
            AJ_NVRAM_Close(handle);
        } else {
            AJ_ErrPrintf(("Failed to write property to NVRAM\n"));
        }
    }
    if (str != value) {
        duk_pop(ctx);
    }
    return TRUE;
}

AJ_Status AJSVC_PropertyStore_Reset(const char* key, int8_t lang)
{
    int8_t field = AJSVC_PropertyStore_GetFieldIndex(key);

    if (IS_VALID_FIELD_ID(field)) {
        /*
         * TODO = handle language deletions
         */
        if (!AJ_NVRAM_Delete(field)) {
            return AJ_ERR_FAILURE;
        }
    }
    InitProperties(NULL, FALSE);
    return AJ_OK;
}

AJ_Status AJSVC_PropertyStore_ResetAll()
{
    InitProperties(NULL, TRUE);
    AJ_AboutSetShouldAnnounce();
    return AJ_OK;
}

const char* AJS_GetCurrentLanguageName()
{
    return PeekProp(AJSVC_PROPERTY_STORE_DEFAULT_LANGUAGE);
}

uint8_t AJS_GetCurrentLanguage()
{
    duk_context* ctx = duktape;
    uint8_t langNum = 0;
    const char* lang = PeekProp(AJSVC_PROPERTY_STORE_DEFAULT_LANGUAGE);
    if (lang) {
        langNum = AJS_GetLanguageIndex(ctx, lang);
    }
    return langNum;
}