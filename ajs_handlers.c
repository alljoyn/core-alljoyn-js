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
#include "ajs_propstore.h"
#include <alljoyn/services_common/PropertyStore.h>



static AJ_BusAttachment* ajBus;

static int NativeAdvertiseName(duk_context* ctx)
{
    AJ_Status status = AJ_OK;
    size_t numArgs = duk_get_top(ctx);
    const char* name = duk_require_string(ctx, 0);
    uint8_t op = AJ_BUS_START_ADVERTISING;

    if ((numArgs >= 2) && !duk_get_boolean(ctx, 1)) {
        op = AJ_BUS_STOP_ADVERTISING;
    } else {
        status = AJ_BusRequestName(ajBus, name, AJ_NAME_REQ_DO_NOT_QUEUE);
    }
    if (status != AJ_OK) {
        status = AJ_BusAdvertiseName(ajBus, name, AJ_TRANSPORT_ANY, op, 0);
    }
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "advertiseName: %s", AJ_StatusText(status));
    }
    /*
     * Push reply object. The method serial number is one less than the serial number
     */
    AJS_PushReplyObject(ctx, ajBus->serial - 1);
    return 1;
}

static int NativeFindServiceByName(duk_context* ctx)
{
    AJ_Status status;
    const char* name = duk_require_string(ctx, 0);
    uint8_t op;

    AJ_InfoPrintf(("findServiceByName %s\n", name));

    if (duk_is_callable(ctx, 2)) {
        if (!duk_is_object(ctx, 1)) {
            duk_error(ctx, DUK_ERR_TYPE_ERROR, "Second arg must be an object with properties 'interfaces', 'path', and 'port'");
        }
        duk_get_prop_string(ctx, 1, "interfaces");
        if (!duk_is_array(ctx, -1)) {
            duk_error(ctx, DUK_ERR_TYPE_ERROR, "Property 'interfaces' is required and must be an array of strings");
        }
        duk_pop(ctx);
        duk_get_prop_string(ctx, 1, "path");
        if (!duk_is_string(ctx, -1)) {
            duk_error(ctx, DUK_ERR_TYPE_ERROR, "Property 'path' is required and must be a string");
        }
        duk_pop(ctx);
        duk_get_prop_string(ctx, 1, "port");
        if (!duk_is_number(ctx, -1)) {
            duk_error(ctx, DUK_ERR_TYPE_ERROR, "Property 'port' is required and must be an integer");
        }
        duk_pop(ctx);
        op = AJ_BUS_START_FINDING;
    } else if (duk_is_undefined(ctx, 1)) {
        op = AJ_BUS_STOP_FINDING;
    } else {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "findServiceByName requires a callback function");
    }
    status = AJ_BusFindAdvertisedName(ajBus, name, op);
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "findServiceByName: %s", AJ_StatusText(status));
    }
    AJS_GetGlobalStashObject(ctx, "findName");
    if (op == AJ_BUS_START_FINDING) {
        duk_push_object(ctx);
        duk_get_prop_string(ctx, 1, "interfaces");
        duk_put_prop_string(ctx, -2, "interfaces");
        duk_get_prop_string(ctx, 1, "path");
        duk_put_prop_string(ctx, -2, "path");
        duk_get_prop_string(ctx, 1, "port");
        duk_put_prop_string(ctx, -2, "port");
        duk_dup(ctx, 2);
        duk_put_prop_string(ctx, -2, "cb");
        duk_put_prop_string(ctx, -2, name);
    } else {
        duk_del_prop_string(ctx, -1, name);
    }
    duk_pop(ctx);
    /*
     * Push reply object. The method serial number is one less than the serial number
     */
    AJS_PushReplyObject(ctx, ajBus->serial - 1);
    return 1;
}

static int NativeAddMatch(duk_context* ctx)
{
    AJ_Status status;
    const char* iface = duk_require_string(ctx, 0);
    const char* signal = duk_require_string(ctx, 1);
    status = AJ_BusAddSignalRule(ajBus, signal, iface, AJ_BUS_SIGNAL_ALLOW);
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "addMatch: %s", AJ_StatusText(status));
    }
    /*
     * Push reply object. The method serial number is one less than the serial number
     */
    AJS_PushReplyObject(ctx, ajBus->serial - 1);
    return 1;
}

static int NativeFindService(duk_context* ctx)
{
    AJ_Status status;
    const char* iface = duk_require_string(ctx, 0);
    uint8_t rule = AJ_BUS_SIGNAL_ALLOW;

    if (duk_is_callable(ctx, 1)) {
        rule = AJ_BUS_SIGNAL_ALLOW;
    } else if (duk_is_undefined(ctx, 1)) {
        rule = AJ_BUS_SIGNAL_DENY;
    } else {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "findService requires a callback function");
    }
    duk_push_sprintf(ctx, "type='signal',sessionless='t',implements='%s',interface='org.alljoyn.About',member='Announce'", iface);
    status = AJ_BusSetSignalRule(ajBus, duk_get_string(ctx, -1), rule);
    if (rule == AJ_BUS_SIGNAL_ALLOW) {
        if (status == AJ_OK) {
            AJS_GetGlobalStashObject(ctx, "serviceCB");
            duk_dup(ctx, 1);
            duk_put_prop_string(ctx, -2, iface);
            duk_pop(ctx);
        }
    } else {
        AJS_GetGlobalStashObject(ctx, "serviceCB");
        duk_del_prop_string(ctx, -1, iface);
        duk_pop(ctx);
    }
    /* TODO - delete "serviceCB" if it is empty */
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "findService: %s", AJ_StatusText(status));
    }
    /*
     * Push reply object. The method serial number is one less than the serial number
     */
    AJS_PushReplyObject(ctx, ajBus->serial - 1);
    return 1;
}

/*
 * The proplist is an object that maps property names to NVRAM identifiers. This function loads the
 * JSON encoded proplist from NVRAM and converts it to a JavaScript object
 */
static void LoadNVRAMPropList(duk_context* ctx)
{
    AJ_NV_DATASET* ds;

    ds = AJ_NVRAM_Open(AJS_PROPSTORE_NVRAM_ID, "r", 0);
    if (!ds) {
        duk_push_object(ctx);
    } else {
        const char* props = AJ_NVRAM_Peek(ds);

        duk_push_string(ctx, props);
        duk_json_decode(ctx, -1);
        /*
         * TODO - must trap errors and close the data set
         */
        AJ_NVRAM_Close(ds);
    }
}

/*
 * The proplist is an object at idx on the duktape stack. This function encodes the object as a JSON
 * string and stores it in NVRAM.
 */
static void StoreNVRAMPropList(duk_context* ctx, duk_idx_t idx)
{
    AJ_NV_DATASET* ds;
    duk_size_t len;
    uint16_t ret;
    const char* str;

    duk_json_encode(ctx, idx);
    str = duk_get_lstring(ctx, idx, &len);
    ds = AJ_NVRAM_Open(AJS_PROPSTORE_NVRAM_ID, "w", len + 1);
    ret = AJ_NVRAM_Write(str, len + 1, ds);
    AJ_NVRAM_Close(ds);
    if (ret != (len + 1)) {
        duk_error(ctx, DUK_ERR_ALLOC_ERROR, "store failed");
    }
}

#define SET_BIT(m, n)   (((m)[(n) / 8]) |= (1 << ((n) % 8)))
#define TEST_BIT(m, n)  (((m)[(n) / 8]) & (1 << ((n) % 8)))

static void StorePropInNVRAM(duk_context* ctx, const char* propName, duk_idx_t idx)
{
    AJ_NV_DATASET* ds;
    const char* val;
    duk_size_t sz;
    uint16_t nvId = 0;

    /*
     * Get the NVRAM id for the property - may need to allocate one
     */
    LoadNVRAMPropList(ctx);
    duk_get_prop_string(ctx, -1, propName);
    if (duk_is_undefined(ctx, -1)) {
        size_t i;
        uint8_t used[(7 + AJS_PROPSTORE_NVRAM_MAX - AJS_PROPSTORE_NVRAM_MIN) / 8];
        memset(used, 0, sizeof(used));
        /*
         * Iterate over the properties to initialize a bit mask of used NVRAM identifiers.
         */
        duk_enum(ctx, -2, DUK_ENUM_OWN_PROPERTIES_ONLY);
        while (duk_next(ctx, -1, 1)) {
            uint16_t bit = duk_get_int(ctx, -1) - AJS_PROPSTORE_NVRAM_MIN;
            duk_pop_2(ctx);
            AJ_ASSERT(bit < (AJS_PROPSTORE_NVRAM_MAX - AJS_PROPSTORE_NVRAM_MIN));
            SET_BIT(used, bit);
        }
        duk_pop(ctx);
        /*
         * Use first free slot.
         */
        for (i = 0; i < sizeof(used) * 8; ++i) {
            if (!TEST_BIT(used, i)) {
                nvId = AJS_PROPSTORE_NVRAM_MIN + i;
                break;
            }
        }
        /*
         * Add the nvId entry to the properties object
         */
        if (nvId) {
            duk_push_number(ctx, nvId);
            duk_put_prop_string(ctx, -3, propName);
            StoreNVRAMPropList(ctx, -2);
        }
    } else {
        nvId = duk_get_int(ctx, -1);
    }
    duk_pop_2(ctx);
    if (!nvId) {
        duk_error(ctx, DUK_ERR_ALLOC_ERROR, "store failed");
    }
    /*
     * Use duktape JX encoding so buffers are appropriately encoded
     */
    duk_get_global_string(ctx, "Duktape");
    duk_get_prop_string(ctx, -1, "enc");
    duk_remove(ctx, -2);
    duk_push_string(ctx, "jx");
    duk_dup(ctx, idx);
    duk_call(ctx, 2);
    val = duk_get_lstring(ctx, -1, &sz);
    ds = AJ_NVRAM_Open(nvId, "w", sz + 1);
    AJ_NVRAM_Write(val, sz + 1, ds);
    AJ_NVRAM_Close(ds);
}

static void LoadPropFromNVRAM(duk_context* ctx, const char* propName)
{

    /*
     * Get the NVRAM id for the property
     */
    LoadNVRAMPropList(ctx);
    duk_get_prop_string(ctx, -1, propName);
    if (duk_is_undefined(ctx, -1)) {
        /*
         * Leave the undefined result on the stack
         */
        duk_remove(ctx, -2);
    } else {
        const char* val;
        uint16_t nvId = duk_get_int(ctx, -1);
        duk_int_t ret;
        AJ_NV_DATASET* ds;

        AJ_ASSERT((nvId <= AJS_PROPSTORE_NVRAM_MAX) && (nvId >= AJS_PROPSTORE_NVRAM_MIN));
        /*
         * Pop the prop list and the nvram id
         */
        duk_pop_2(ctx);
        /*
         * Use the duktape JX decoder so buffer objects get decoded.
         */
        duk_get_global_string(ctx, "Duktape");
        duk_get_prop_string(ctx, -1, "dec");
        duk_remove(ctx, -2);
        duk_push_string(ctx, "jx");
        ds = AJ_NVRAM_Open(nvId, "r", 0);
        if (!ds) {
            duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "Inconsistent propstore");
        }
        val = AJ_NVRAM_Peek(ds);
        duk_push_string(ctx, val);
        ret = duk_pcall(ctx, 2);
        AJ_NVRAM_Close(ds);
        if (ret != DUK_EXEC_SUCCESS) {
            duk_throw(ctx);
        }
    }
}

static int NativeLoadProperty(duk_context* ctx)
{
    const char* prop = duk_require_string(ctx, 0);
    AJSVC_PropertyStoreFieldIndices index = AJSVC_PropertyStore_GetFieldIndex(prop);

    /*
     * First check for built-in (config/about) properties
     */
    if (index != AJSVC_PROPERTY_STORE_ERROR_FIELD_INDEX) {
        const char* val = AJSVC_PropertyStore_GetValue(index);
        if (val) {
            duk_push_string(ctx, val);
        } else {
            duk_push_undefined(ctx);
        }
    } else {
        LoadPropFromNVRAM(ctx, prop);
    }
    return 1;
}

static int NativeStoreProperty(duk_context* ctx)
{
    const char* prop = duk_require_string(ctx, 0);
    AJSVC_PropertyStoreFieldIndices index = AJSVC_PropertyStore_GetFieldIndex(prop);

    /*
     * First check for built-in (config/about) properties
     */
    if (index != AJSVC_PROPERTY_STORE_ERROR_FIELD_INDEX) {
        if (!AJS_PropertyStore_IsReadOnly(index)) {
            /*
             * Ignore attempts to set a readonly property
             */
            AJSVC_PropertyStore_SetValue(index, duk_require_string(ctx, -1));
        }
    } else {
        StorePropInNVRAM(ctx, prop, 1);
    }
    return 0;
}

static const duk_function_list_entry aj_native_functions[] = {
    { "addMatch",          NativeAddMatch,          2 },
    { "findService",       NativeFindService,       2 },
    { "findServiceByName", NativeFindServiceByName, 3 },
    { "advertiseName",     NativeAdvertiseName,     1 },
    { "load",              NativeLoadProperty,      1 },
    { "store",             NativeStoreProperty,     2 },
    { NULL }
};

AJ_Status AJS_RegisterHandlers(AJ_BusAttachment* bus, duk_context* ctx, duk_idx_t ajIdx)
{
    ajBus = bus;
    duk_put_function_list(ctx, ajIdx, aj_native_functions);
    return AJ_OK;
}
