/******************************************************************************
 * Copyright (c) 2013 - 2014, AllSeen Alliance. All rights reserved.
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
#include "ajs_propstore.h"
#include "ajs_translations.h"

#include <alljoyn/services_common/PropertyStore.h>
#include <alljoyn/services_common/ServicesCommon.h>
#include <alljoyn/onboarding/OnboardingManager.h>
#include <aj_guid.h>
#include <aj_nvram.h>
#include <aj_creds.h>
#include <aj_config.h>

#define FIRST_PROPERTY_INDEX ((AJSVC_PropertyStoreFieldIndices)0)

static const PropertyStoreEntry propDefs[AJSVC_PROPERTY_STORE_NUMBER_OF_KEYS] =
{
//  { "Key Name            ", W, A, M, I .. . . ., P },
    { "DeviceId",             0, 1, 0, 1, 0, 0, 0, 1 },
    { "AppId",                0, 1, 0, 1, 0, 0, 0, 1 },
    { "DeviceName",           1, 1, 0, 1, 0, 0, 0, 1 },
    { "DefaultLanguage",      1, 1, 0, 0, 0, 0, 0, 1 },
    { "Passcode",             1, 0, 0, 0, 0, 0, 0, 0 },
    { "RealmName",            1, 0, 0, 0, 0, 0, 0, 0 },
// Add other runtime keys above this line
    { "AppName",              0, 1, 0, 0, 0, 0, 0, 1 },
    { "Description",          0, 0, 1, 0, 0, 0, 0, 1 },
    { "Manufacturer",         0, 1, 1, 0, 0, 0, 0, 1 },
    { "ModelNumber",          0, 1, 0, 0, 0, 0, 0, 1 },
    { "DateOfManufacture",    0, 0, 0, 0, 0, 0, 0, 1 },
    { "SoftwareVersion",      0, 0, 0, 0, 0, 0, 0, 1 },
    { "AJSoftwareVersion",    0, 0, 0, 0, 0, 0, 0, 1 },
    { "MaxLength",            0, 1, 0, 0, 0, 0, 0, 1 },
// Add other mandatory about keys above this line
    { "HardwareVersion",      0, 0, 0, 0, 0, 0, 0, 1 },
    { "SupportUrl",           0, 0, 1, 0, 0, 0, 0, 1 },
// Add other optional about keys above this line
};

static duk_context* duktape;
/*
 * TODO - many of these strings need to be provisioned or initialized from JavaScript
 */
static const char deviceManufactureName[] = "Manufacturer Name";
static const char deviceProductName[] = "Product Name";
/*
 * properties array of default values
 */
static const char* DEFAULT_PASSCODE[] = { "303030303030" }; // HEX encoded { '0', '0', '0', '0', '0', '0' }
static const char* DEFAULT_APP_NAME[] = { "AllJoyn.js" };
static const char* DEFAULT_DESCRIPTION[] = { "AllJoyn.js" };
static const char* DEFAULT_MANUFACTURER[] = { "AllSeen" };
static const char* DEFAULT_DEVICE_MODEL[] = { "0.0.1" };
static const char* DEFAULT_DATE_OF_MANUFACTURE[] = { "2014-07-07" };
static const char* DEFAULT_SOFTWARE_VERSION[] = { "0.0.1" };
static const char* DEFAULT_HARDWARE_VERSION[] = { "0.0.1" };
static const char* DEFAULT_SUPPORT_URL[] = { "www.allseenalliance.org" };
static const char* DEFAULT_LANGUAGE[] = { "en" };

static const char** defaultVals[AJSVC_PROPERTY_STORE_NUMBER_OF_KEYS] =
{
    // "Default Values per language",    "Key Name"
    NULL,                           /*DeviceId*/
    NULL,                           /*AppId*/
    NULL,                           /*DeviceName*/
    DEFAULT_LANGUAGE,               /*DefaultLanguage*/
    DEFAULT_PASSCODE,               /*Passcode*/
    NULL,                           /*RealmName*/
    // Add other runtime or configurable keys above this line
    DEFAULT_APP_NAME,               /*AppName*/
    DEFAULT_DESCRIPTION,            /*Description*/
    DEFAULT_MANUFACTURER,           /*Manufacturer*/
    DEFAULT_DEVICE_MODEL,           /*ModelNumber*/
    DEFAULT_DATE_OF_MANUFACTURE,    /*DateOfManufacture*/
    DEFAULT_SOFTWARE_VERSION,       /*SoftwareVersion*/
    NULL,                           /*AJSoftwareVersion*/
    NULL,                           /*MaxLength*/
    // Add other mandatory about keys above this line
    DEFAULT_HARDWARE_VERSION,       /*HardwareVersion*/
    DEFAULT_SUPPORT_URL,            /*SupportUrl*/
    // Add other optional about keys above this line
};

/**
 * properties array of runtime values' buffers
 */
static char machineIdVar[MACHINE_ID_LENGTH + 1] = { 0 };
static char* machineIdVars[] = { machineIdVar };
static char deviceNameVar[DEVICE_NAME_VALUE_LENGTH + 1] = { 0 };
static char* deviceNameVars[] = { deviceNameVar };
static char defaultLanguageVar[LANG_VALUE_LENGTH + 1] = { 0 };
static char* defaultLanguageVars[] = { defaultLanguageVar };
static char passcodeVar[PASSWORD_VALUE_LENGTH + 1] = { 0 };
static char* passcodeVars[] = { passcodeVar };
static char realmNameVar[KEY_VALUE_LENGTH + 1] = { 0 };
static char* realmNameVars[] = { realmNameVar };

PropertyStoreConfigEntry propVals[AJSVC_PROPERTY_STORE_NUMBER_OF_RUNTIME_KEYS] =
{
    //  {"Buffers for Values per language", "Buffer Size"},                  "Key Name"
    { machineIdVars,             MACHINE_ID_LENGTH + 1 },               /*DeviceId*/
    { machineIdVars,             MACHINE_ID_LENGTH + 1 },               /*AppId*/
    { deviceNameVars,            DEVICE_NAME_VALUE_LENGTH + 1 },        /*DeviceName*/
    { defaultLanguageVars,       LANG_VALUE_LENGTH + 1 },               /*DefaultLanguage*/
    { passcodeVars,              PASSWORD_VALUE_LENGTH + 1 },           /*Passcode*/
    { realmNameVars,             KEY_VALUE_LENGTH + 1 },                /*RealmName*/
};

static const char* defaultLanguagesKeyName = { "SupportedLanguages" };

uint8_t AJSVC_PropertyStore_GetMaxValueLength(AJSVC_PropertyStoreFieldIndices field)
{
    switch (field) {
    case AJSVC_PROPERTY_STORE_DEVICE_NAME:
        return DEVICE_NAME_VALUE_LENGTH;

    case AJSVC_PROPERTY_STORE_DEFAULT_LANGUAGE:
        return LANG_VALUE_LENGTH;

    case AJSVC_PROPERTY_STORE_PASSCODE:
        return PASSWORD_VALUE_LENGTH;

    default:
        return KEY_VALUE_LENGTH;
    }
}

const char* AJSVC_PropertyStore_GetFieldName(AJSVC_PropertyStoreFieldIndices field)
{
    if ((int8_t)field <= (int8_t)AJSVC_PROPERTY_STORE_ERROR_FIELD_INDEX || (int8_t)field >= (int8_t)AJSVC_PROPERTY_STORE_NUMBER_OF_KEYS) {
        return "N/A";
    }
    return propDefs[field].keyName;
}

AJSVC_PropertyStoreFieldIndices AJSVC_PropertyStore_GetFieldIndex(const char* fieldName)
{
    AJSVC_PropertyStoreFieldIndices field = FIRST_PROPERTY_INDEX;
    for (; field < AJSVC_PROPERTY_STORE_NUMBER_OF_KEYS; field++) {
        if (!strcmp(propDefs[field].keyName, fieldName)) {
            return field;
        }
    }
    return AJSVC_PROPERTY_STORE_ERROR_FIELD_INDEX;
}

static int8_t GetLanguageIndexForProperty(int8_t lang, AJSVC_PropertyStoreFieldIndices field)
{
    if (propDefs[field].mode2MultiLng) {
        return lang;
    }
    return AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX;
}

const char* AJSVC_PropertyStore_GetValueForLang(AJSVC_PropertyStoreFieldIndices field, int8_t lang)
{
    if ((int8_t)field <= (int8_t)AJSVC_PROPERTY_STORE_ERROR_FIELD_INDEX || (int8_t)field >= (int8_t)AJSVC_PROPERTY_STORE_NUMBER_OF_KEYS) {
        return NULL;
    }
    lang = GetLanguageIndexForProperty(lang, field);
    if (lang <= AJSVC_PROPERTY_STORE_ERROR_LANGUAGE_INDEX || lang >= AJS_GetNumberOfLanguages(duktape)) {
        return NULL;
    }
    if (field < AJSVC_PROPERTY_STORE_NUMBER_OF_RUNTIME_KEYS &&
        (propDefs[field].mode0Write || propDefs[field].mode3Init) &&
        propVals[field].value != NULL &&
        (propVals[field].value[lang]) != NULL &&
        (propVals[field].value[lang])[0] != '\0') {
        AJ_InfoPrintf(("Has key [%s] runtime Value [%s]\n", propDefs[field].keyName, propVals[field].value[lang]));
        return propVals[field].value[lang];
    } else if (defaultVals[field] != NULL && (defaultVals[field])[lang] != NULL) {
        AJ_InfoPrintf(("Has key [%s] default Value [%s]\n", propDefs[field].keyName, (defaultVals[field])[lang]));
        return (defaultVals[field])[lang];
    }

    return NULL;
}

const char* AJSVC_PropertyStore_GetValue(AJSVC_PropertyStoreFieldIndices field)
{
    return AJSVC_PropertyStore_GetValueForLang(field, AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX);
}

const char* AJSVC_PropertyStore_GetLanguageName(int8_t lang)
{
    return AJS_GetLanguageName(duktape, lang);
}

int8_t AJSVC_PropertyStore_GetLanguageIndex(const char* const language)
{
    return (int8_t)AJS_GetLanguageIndex(duktape, language);
}

uint8_t AJSVC_PropertyStore_SetValueForLang(AJSVC_PropertyStoreFieldIndices field, int8_t lang, const char* value)
{
    size_t var_size;
    if ((int8_t)field <= (int8_t)AJSVC_PROPERTY_STORE_ERROR_FIELD_INDEX ||
        (int8_t)field >= (int8_t)AJSVC_PROPERTY_STORE_NUMBER_OF_RUNTIME_KEYS) {
        return FALSE;
    }
    lang = GetLanguageIndexForProperty(lang, field);
    if (lang <= AJSVC_PROPERTY_STORE_ERROR_LANGUAGE_INDEX || lang >= AJS_GetNumberOfLanguages(duktape)) {
        return FALSE;
    }
    AJ_InfoPrintf(("Set key [%s] defaultValue [%s]\n", propDefs[field].keyName, value));
    var_size = propVals[field].size;
    strncpy(propVals[field].value[lang], value, var_size - 1);
    (propVals[field].value[lang])[var_size - 1] = '\0';

    return TRUE;
}

uint8_t AJSVC_PropertyStore_SetValue(AJSVC_PropertyStoreFieldIndices field, const char* value)
{
    return AJSVC_PropertyStore_SetValueForLang(field, AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX, value);
}

int8_t AJSVC_PropertyStore_GetCurrentDefaultLanguageIndex()
{
    const char* curr = AJSVC_PropertyStore_GetValue(AJSVC_PROPERTY_STORE_DEFAULT_LANGUAGE);
    int8_t index = AJSVC_PropertyStore_GetLanguageIndex(curr);
    if (index == AJSVC_PROPERTY_STORE_ERROR_LANGUAGE_INDEX) {
        index = AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX;
        AJ_WarnPrintf(("Failed to find default language %s defaulting to %s", (curr ? curr : "NULL"), AJS_GetLanguageName(duktape, AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX)));
    }
    return index;
}

static void ClearPropertiesInRAM()
{
    uint8_t lang;
    char* buf;
    AJSVC_PropertyStoreFieldIndices field = FIRST_PROPERTY_INDEX;
    for (; field < AJSVC_PROPERTY_STORE_NUMBER_OF_RUNTIME_KEYS; field++) {
        if (propVals[field].value) {
            uint8_t numLangs = AJS_GetNumberOfLanguages(duktape);
            lang = AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX;
            for (; lang < numLangs; lang++) {
                if (propDefs[field].mode2MultiLng || lang == AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX) {
                    buf = propVals[field].value[lang];
                    if (buf) {
                        memset(buf, 0, propVals[field].size);
                    }
                }
            }
        }
    }
}

static void InitMandatoryPropertiesInRAM()
{
    char* machineIdValue = propVals[AJSVC_PROPERTY_STORE_APP_ID].value[AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX];
    const char* currentAppIdValue = AJSVC_PropertyStore_GetValue(AJSVC_PROPERTY_STORE_APP_ID);
    const char* currentDeviceIdValue = AJSVC_PropertyStore_GetValue(AJSVC_PROPERTY_STORE_DEVICE_ID);
    const char* currentDeviceNameValue = AJSVC_PropertyStore_GetValue(AJSVC_PROPERTY_STORE_DEVICE_NAME);
    size_t serialIdLen = 0;
    size_t machineIdLen = 0;
    AJ_GUID theAJ_GUID;
    AJ_Status status;
    char deviceName[DEVICE_NAME_VALUE_LENGTH + 1] = { 0 };
    if (currentAppIdValue == NULL || currentAppIdValue[0] == '\0') {
        status = AJ_GetLocalGUID(&theAJ_GUID);
        if (status == AJ_OK) {
            AJ_GUID_ToString(&theAJ_GUID, machineIdValue, propVals[AJSVC_PROPERTY_STORE_APP_ID].size);
        }
    }
    if (currentDeviceIdValue == NULL || currentDeviceIdValue[0] == '\0') {
        AJSVC_PropertyStore_SetValue(AJSVC_PROPERTY_STORE_DEVICE_ID, machineIdValue);
    }
    if (currentDeviceNameValue == NULL || currentDeviceNameValue[0] == '\0') {
        serialIdLen = AJOBS_DEVICE_SERIAL_ID_LEN;
        machineIdLen = strlen(machineIdValue);
#ifdef _WIN32
        _snprintf(deviceName, DEVICE_NAME_VALUE_LENGTH + 1, "%s %s %s", deviceManufactureName, deviceProductName, &machineIdValue[machineIdLen - min(serialIdLen, machineIdLen)]);
#else
        snprintf(deviceName, DEVICE_NAME_VALUE_LENGTH + 1, "%s %s %s", deviceManufactureName, deviceProductName, &machineIdValue[machineIdLen - min(serialIdLen, machineIdLen)]);
#endif
        AJSVC_PropertyStore_SetValue(AJSVC_PROPERTY_STORE_DEVICE_NAME, deviceName);
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

AJ_Status AJS_PropertyStoreInit(duk_context* ctx)
{
    AJ_Status status;

    duktape = ctx;

    status = AJSVC_PropertyStore_LoadAll();
    if (status == AJ_OK) {
        duktape = ctx;
        InitMandatoryPropertiesInRAM();
        /*
         * About needs to get values from the property store
         */
        AJ_AboutRegisterPropStoreGetter(AboutPropGetter);
    }
    return status;
}

static AJ_Status PropertyStore_ReadConfig(uint16_t index, void* ptr, uint16_t size)
{
    AJ_Status status = AJ_OK;
    uint16_t sizeRead = 0;

    AJ_NV_DATASET* nvramHandle = AJ_NVRAM_Open(index, "r", 0);
    if (nvramHandle != NULL) {
        sizeRead = AJ_NVRAM_Read(ptr, size, nvramHandle);
        status = AJ_NVRAM_Close(nvramHandle);
        if (sizeRead != sizeRead) {
            status = AJ_ERR_RANGE;
        }
    }

    return status;
}

static AJ_Status PropertyStore_WriteConfig(uint16_t index, void* ptr, uint16_t size, char* mode)
{
    AJ_Status status = AJ_OK;
    uint16_t sizeWritten = 0;

    AJ_NV_DATASET* nvramHandle = AJ_NVRAM_Open(index, mode, size);
    if (nvramHandle != NULL) {
        sizeWritten = AJ_NVRAM_Write(ptr, size, nvramHandle);
        status = AJ_NVRAM_Close(nvramHandle);
        if (sizeWritten != size) {
            status = AJ_ERR_RANGE;
        }
    }

    return status;
}

AJ_Status AJSVC_PropertyStore_LoadAll()
{
    AJ_Status status = AJ_OK;
    void* buf = NULL;
    uint16_t size = 0;
    uint16_t entry;

    AJSVC_PropertyStoreFieldIndices lang = (AJSVC_PropertyStoreFieldIndices)AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX;
    uint8_t numLangs = AJS_GetNumberOfLanguages(duktape);

    for (; lang < numLangs; lang++) {
        AJSVC_PropertyStoreFieldIndices field = FIRST_PROPERTY_INDEX;
        for (; field < AJSVC_PROPERTY_STORE_NUMBER_OF_RUNTIME_KEYS; field++) {
            if (propVals[field].value == NULL ||
                !propDefs[field].mode0Write ||
                (lang != AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX && !propDefs[field].mode2MultiLng)) {
                continue;
            }
            buf = propVals[field].value[lang];
            if (buf) {
                size = propVals[field].size;
                entry = (int)field + (int)lang * (int)AJSVC_PROPERTY_STORE_NUMBER_OF_RUNTIME_KEYS;
                status = PropertyStore_ReadConfig(AJ_PROPERTIES_NV_ID_BEGIN + entry, buf, size);
                AJ_InfoPrintf(("nvram read field=%d [%s] lang=%d [%s] entry=%d val=%s size=%u status=%s\n", (int)field, propDefs[field].keyName,
                               (int)lang, AJS_GetLanguageName(duktape, lang), (int)entry, propVals[field].value[lang], (int)size, AJ_StatusText(status)));
            }
        }
    }

    return status;
}

AJ_Status AJSVC_PropertyStore_SaveAll()
{
    AJ_Status status = AJ_OK;
    void* buf = NULL;
    uint16_t size = 0;
    uint16_t entry;

    AJSVC_PropertyStoreFieldIndices lang = (AJSVC_PropertyStoreFieldIndices)AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX;
    uint8_t numLangs = AJS_GetNumberOfLanguages(duktape);

    for (; lang < numLangs; lang++) {
        AJSVC_PropertyStoreFieldIndices field = FIRST_PROPERTY_INDEX;
        for (; field < AJSVC_PROPERTY_STORE_NUMBER_OF_RUNTIME_KEYS; field++) {
            if (propVals[field].value == NULL ||
                !propDefs[field].mode0Write ||
                (lang != AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX && !propDefs[field].mode2MultiLng)) {
                continue;
            }
            buf = propVals[field].value[lang];
            if (buf) {
                size = propVals[field].size;
                entry = (int)field + (int)lang * (int)AJSVC_PROPERTY_STORE_NUMBER_OF_RUNTIME_KEYS;
                status = PropertyStore_WriteConfig(AJ_PROPERTIES_NV_ID_BEGIN + entry, buf, size, "w");
                AJ_InfoPrintf(("nvram write field=%d [%s] lang=%d [%s] entry=%d val=%s size=%u status=%s\n", (int)field, propDefs[field].keyName, (int)lang, AJS_GetLanguageName(duktape, lang), (int)entry, propVals[field].value[lang], (int)size, AJ_StatusText(status)));
            }
        }
    }
    AJ_AboutSetShouldAnnounce(); // Set flag for sending an updated Announcement

    return status;
}

static uint8_t UpdateFieldInRAM(AJSVC_PropertyStoreFieldIndices field, int8_t lang, const char* fieldValue)
{
    uint8_t ret = FALSE;

    if (propDefs[field].mode0Write && propDefs[field].mode7Public) {
        ret = AJSVC_PropertyStore_SetValueForLang(field, lang, fieldValue);
    } else {
        AJ_ErrPrintf(("UpdateFieldInRAM ERROR - field %s has read only attribute or is private\n", propDefs[field].keyName));
    }

    return ret;
}

static uint8_t DeleteFieldFromRAM(AJSVC_PropertyStoreFieldIndices field, int8_t lang)
{
    return UpdateFieldInRAM(field, lang, "");
}

AJ_Status AJSVC_PropertyStore_ReadAll(AJ_Message* msg, AJSVC_PropertyStoreCategoryFilter filter, int8_t lang)
{
    AJ_Status status = AJ_OK;
    AJ_Arg array;
    AJ_Arg array2;
    AJ_Arg dict;
    const char* value;
    uint8_t index;
    const char* ajVersion;
    AJSVC_PropertyStoreFieldIndices field = FIRST_PROPERTY_INDEX;

    AJ_InfoPrintf(("PropertyStore_ReadAll()\n"));

    status = AJ_MarshalContainer(msg, &array, AJ_ARG_ARRAY);
    if (status != AJ_OK) {
        return status;
    }

    for (; field < AJSVC_PROPERTY_STORE_NUMBER_OF_KEYS; field++) {
        if (propDefs[field].mode7Public && (filter.bit0About || (filter.bit1Config && propDefs[field].mode0Write) || (filter.bit2Announce && propDefs[field].mode1Announce))) {
            value = AJSVC_PropertyStore_GetValueForLang(field, lang);

            if (value == NULL && (int8_t)field >= (int8_t)AJSVC_PROPERTY_STORE_NUMBER_OF_MANDATORY_KEYS) {     // Non existing values are skipped!
                AJ_WarnPrintf(("PropertyStore_ReadAll - Failed to get value for field=(name=%s, index=%d) and language=(name=%s, index=%d), skipping.\n", AJSVC_PropertyStore_GetFieldName(field), (int)field, AJSVC_PropertyStore_GetLanguageName(lang), (int)lang));
            } else {
                if (field == AJSVC_PROPERTY_STORE_APP_ID) {
                    if (value == NULL) {
                        AJ_ErrPrintf(("PropertyStore_ReadAll - Failed to get value for mandatory field=(name=%s, index=%d) and language=(name=%s, index=%d), aborting.\n", AJSVC_PropertyStore_GetFieldName(field), (int)field, AJSVC_PropertyStore_GetLanguageName(lang), (int)lang));
                        return AJ_ERR_NULL;
                    }
                    status = AJ_MarshalContainer(msg, &dict, AJ_ARG_DICT_ENTRY);
                    if (status != AJ_OK) {
                        return status;
                    }
                    status = AJ_MarshalArgs(msg, "s", propDefs[field].keyName);
                    if (status != AJ_OK) {
                        return status;
                    }
                    status = AJSVC_MarshalAppIdAsVariant(msg, value);
                    if (status != AJ_OK) {
                        return status;
                    }
                    status = AJ_MarshalCloseContainer(msg, &dict);
                    if (status != AJ_OK) {
                        return status;
                    }
                } else if (field == AJSVC_PROPERTY_STORE_MAX_LENGTH) {
                    status = AJ_MarshalArgs(msg, "{sv}", propDefs[field].keyName, "q", DEVICE_NAME_VALUE_LENGTH);
                    if (status != AJ_OK) {
                        return status;
                    }
                    AJ_InfoPrintf(("Has key [%s] runtime Value [%d]\n", propDefs[AJSVC_PROPERTY_STORE_MAX_LENGTH].keyName, DEVICE_NAME_VALUE_LENGTH));
                } else if (field == AJSVC_PROPERTY_STORE_AJ_SOFTWARE_VERSION) {
                    ajVersion = AJ_GetVersion();
                    if (ajVersion == NULL) {
                        AJ_ErrPrintf(("PropertyStore_ReadAll - Failed to get value for mandatory field=(name=%s, index=%d) and language=(name=%s, index=%d), aborting.\n", AJSVC_PropertyStore_GetFieldName(field), (int)field, AJSVC_PropertyStore_GetLanguageName(lang), (int)lang));
                        return AJ_ERR_NULL;
                    }
                    status = AJ_MarshalArgs(msg, "{sv}", propDefs[field].keyName, "s", ajVersion);
                    if (status != AJ_OK) {
                        return status;
                    }
                    AJ_InfoPrintf(("Has key [%s] runtime Value [%s]\n", propDefs[AJSVC_PROPERTY_STORE_AJ_SOFTWARE_VERSION].keyName, ajVersion));
                } else {
                    if (value == NULL) {
                        AJ_ErrPrintf(("PropertyStore_ReadAll - Failed to get value for mandatory field=(name=%s, index=%d) and language=(name=%s, index=%d), aborting.\n", AJSVC_PropertyStore_GetFieldName(field), (int)field, AJSVC_PropertyStore_GetLanguageName(lang), (int)lang));
                        return AJ_ERR_NULL;
                    }
                    status = AJ_MarshalArgs(msg, "{sv}", propDefs[field].keyName, "s", value);
                    if (status != AJ_OK) {
                        return status;
                    }
                }
            }
        }
    }

    if (filter.bit0About) {
        uint8_t numLangs = AJS_GetNumberOfLanguages(duktape);

        // Add supported languages
        status = AJ_MarshalContainer(msg, &dict, AJ_ARG_DICT_ENTRY);
        if (status != AJ_OK) {
            return status;
        }
        status = AJ_MarshalArgs(msg, "s", defaultLanguagesKeyName);
        if (status != AJ_OK) {
            return status;
        }
        status = AJ_MarshalVariant(msg, "as");
        if (status != AJ_OK) {
            return status;
        }
        status = AJ_MarshalContainer(msg, &array2, AJ_ARG_ARRAY);
        if (status != AJ_OK) {
            return status;
        }

        index = AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX;
        for (; index < numLangs; index++) {
            status = AJ_MarshalArgs(msg, "s", AJS_GetLanguageName(duktape, index));
            if (status != AJ_OK) {
                return status;
            }
        }

        status = AJ_MarshalCloseContainer(msg, &array2);
        if (status != AJ_OK) {
            return status;
        }
        status = AJ_MarshalCloseContainer(msg, &dict);
        if (status != AJ_OK) {
            return status;
        }
    }
    status = AJ_MarshalCloseContainer(msg, &array);
    if (status != AJ_OK) {
        return status;
    }

    return status;
}

AJ_Status AJSVC_PropertyStore_Update(const char* key, int8_t lang, const char* value)
{
    AJSVC_PropertyStoreFieldIndices field = AJSVC_PropertyStore_GetFieldIndex(key);
    if (field == AJSVC_PROPERTY_STORE_ERROR_FIELD_INDEX || field >= AJSVC_PROPERTY_STORE_NUMBER_OF_RUNTIME_KEYS) {
        return AJ_ERR_INVALID;
    }
    if (!UpdateFieldInRAM(field, lang, value)) {
        return AJ_ERR_FAILURE;
    }
    return AJ_OK;
}

AJ_Status AJSVC_PropertyStore_Reset(const char* key, int8_t lang)
{
    AJSVC_PropertyStoreFieldIndices field = AJSVC_PropertyStore_GetFieldIndex(key);
    if (field == AJSVC_PROPERTY_STORE_ERROR_FIELD_INDEX || field >= AJSVC_PROPERTY_STORE_NUMBER_OF_RUNTIME_KEYS) {
        return AJ_ERR_INVALID;
    }
    if (!DeleteFieldFromRAM(field, lang)) {
        return AJ_ERR_FAILURE;
    }
    InitMandatoryPropertiesInRAM();
    return AJ_OK;
}

AJ_Status AJSVC_PropertyStore_ResetAll()
{
    ClearPropertiesInRAM();
    InitMandatoryPropertiesInRAM();
    return AJSVC_PropertyStore_SaveAll();
}
