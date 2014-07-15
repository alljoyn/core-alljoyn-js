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

AJ_Status AJS_RegisterHandlers(AJ_BusAttachment* bus, duk_context* ctx, duk_idx_t ajIdx)
{
    ajBus = bus;

    duk_push_c_function(ctx, NativeAddMatch, 2);
    duk_put_prop_string(ctx, ajIdx, "addMatch");
    duk_push_c_function(ctx, NativeFindService, 2);
    duk_put_prop_string(ctx, ajIdx, "findService");
    duk_push_c_function(ctx, NativeFindServiceByName, 3);
    duk_put_prop_string(ctx, ajIdx, "findServiceByName");
    duk_push_c_function(ctx, NativeAdvertiseName, 1);
    duk_put_prop_string(ctx, ajIdx, "advertiseName");
    return AJ_OK;
}
