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
#include <alljoyn/notification/NotificationProducer.h>


static int8_t EnumKVPairs(duk_context* ctx, const char* prop, AJNS_DictionaryEntry** kvOut)
{
    int num = 0;
    AJNS_DictionaryEntry* kv = NULL;

    duk_get_prop_string(ctx, -1, prop);
    if (!duk_is_object(ctx, -1)) {
        duk_pop(ctx);
        return 0;
    }
    duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
    while (duk_next(ctx, -1, 1)) {
        if (!kv) {
            *kvOut = kv = duk_alloc(ctx, sizeof(AJNS_DictionaryEntry));
        } else {
            *kvOut = kv = duk_realloc(ctx, kv, (num + 1) * sizeof(AJNS_DictionaryEntry));
        }
        if (!kv) {
            duk_error(ctx, DUK_ERR_ALLOC_ERROR, "alloc failed");
        }
        kv[num].key = duk_require_string(ctx, -2);
        kv[num].value = duk_require_string(ctx, -1);
        duk_pop_2(ctx);
        ++num;
    }
    duk_pop_2(ctx); /* enum */
    /*
     * Count is an 8 bit signed int
     */
    if (num > 127) {
        duk_error(ctx, DUK_ERR_RANGE_ERROR, "Too many entries");
    }
    return (int8_t)num;
}

static int SafeSendNotification(duk_context* ctx)
{
    AJNS_NotificationContent* notif = duk_require_pointer(ctx, -2);

    notif->numTexts = EnumKVPairs(ctx, "text", &notif->texts);
    notif->numCustomAttributes = EnumKVPairs(ctx, "attributes", &notif->customAttributes);
    notif->numAudioUrls = EnumKVPairs(ctx, "audioUrls", &notif->richAudioUrls);
    notif->richIconUrl = AJS_GetStringProp(ctx, -1, "iconUrl");
    notif->richIconObjectPath = AJS_GetStringProp(ctx, -1, "iconPath");
    notif->richAudioObjectPath = AJS_GetStringProp(ctx, -1, "audioPath");
    notif->controlPanelServiceObjectPath = AJS_GetStringProp(ctx, -1, "controlPanelPath");

    return 0;
}

static int NativeSendNotification(duk_context* ctx)
{
    int ret;
    AJ_Status status = AJ_OK;
    AJNS_NotificationContent notif;
    AJ_BusAttachment* aj = AJS_GetBusAttachment();
    uint16_t ttl = duk_is_number(ctx, 0) ? (uint16_t)duk_get_int(ctx, 0) : AJS_DEFAULT_SLS_TTL;
    uint16_t notifType;

    if (!AJS_IsRunning()) {
        duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "notification.send: not attached to AllJoyn");
    }

    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "type");
    notifType = (uint16_t)duk_require_int(ctx, -1);
    duk_pop(ctx);

    memset(&notif, 0, sizeof(notif));
    duk_push_pointer(ctx, &notif);
    duk_push_this(ctx);
    ret = duk_safe_call(ctx, SafeSendNotification, 2, 0);
    if (ret == DUK_EXEC_SUCCESS) {
        uint32_t serialNum;
        status = AJNS_Producer_SendNotification(aj, &notif, notifType, ttl, &serialNum);
    }
    duk_free(ctx, notif.texts);
    duk_free(ctx, notif.customAttributes);
    duk_free(ctx, notif.richAudioUrls);
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "notification.send: %s", AJ_StatusText(status));
    }
    return (ret == DUK_EXEC_SUCCESS) ? 1 : -1;
}

static int NativeCancelNotification(duk_context* ctx)
{
    AJ_Status status;
    uint16_t notifType;
    AJ_BusAttachment* aj = AJS_GetBusAttachment();

    if (!AJS_IsRunning()) {
        duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "notification.send: not attached to AllJoyn");
    }
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "type");
    notifType = (uint16_t)duk_require_int(ctx, -1);
    status = AJNS_Producer_DeleteLastNotification(aj, notifType);
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "notifcation.cancel: %s", AJ_StatusText(status));
    }
    return 1;
}

static void AddObjProp(duk_context* ctx, const char* prop)
{
    duk_push_object(ctx);
    duk_put_prop_string(ctx, -2, prop);
}

static void AddStringProp(duk_context* ctx, const char* prop)
{
    duk_push_null(ctx);
    duk_put_prop_string(ctx, -2, prop);
}

static int NativeNotification(duk_context* ctx)
{
    uint16_t notifType;
    duk_idx_t numArgs = duk_get_top(ctx);
    const char* text = NULL;
    const char* lang;

    if (numArgs == 0) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "notification: requires type");
    }
    notifType = duk_require_int(ctx, 0);
    if (numArgs > 1) {
        text = duk_require_string(ctx, 1);
    }
    if (numArgs > 2) {
        lang = duk_require_string(ctx, 2);
    } else {
        lang = AJS_GetCurrentLanguageName();
    }
    /*
     * Create the actual notification object. We could have done this in JavaScript but setting the
     * send and cancel functions would have been messy.
     */
    duk_push_object(ctx);
    duk_push_int(ctx, notifType);
    duk_put_prop_string(ctx, -2, "type");
    if (text) {
        duk_push_object(ctx);
        duk_push_string(ctx, text);
        duk_put_prop_string(ctx, -2, lang);
        duk_put_prop_string(ctx, -2, "text");
    } else {
        AddObjProp(ctx, "text");
    }
    AddObjProp(ctx, "attributes");
    AddObjProp(ctx, "audioUrls");
    AddStringProp(ctx, "iconUrl");
    AddStringProp(ctx, "iconPath");
    AddStringProp(ctx, "audioPath");
    AddStringProp(ctx, "controlPanelPath");
    duk_push_c_function(ctx, NativeSendNotification, 1);
    duk_put_prop_string(ctx, -2, "send");
    duk_push_c_function(ctx, NativeCancelNotification, 0);
    duk_put_prop_string(ctx, -2, "cancel");
    /*
     * The new object is the return value - leave it on the stack
     */
    return 1;
}

AJ_Status AJS_RegisterNotifHandlers(AJ_BusAttachment* bus, duk_context* ctx, duk_idx_t ajIdx)
{
    duk_idx_t notifIdx;

    notifIdx = duk_push_c_function(ctx, NativeNotification, DUK_VARARGS);
    /*
     * Notification type consts
     */
    duk_push_int(ctx, 0);
    duk_put_prop_string(ctx, notifIdx, "Emergency");
    duk_push_int(ctx, 1);
    duk_put_prop_string(ctx, notifIdx, "Warning");
    duk_push_int(ctx, 2);
    duk_put_prop_string(ctx, notifIdx, "Info");

    duk_put_prop_string(ctx, ajIdx, "notification");

    return AJ_OK;
}
