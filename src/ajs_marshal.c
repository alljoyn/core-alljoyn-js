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
#include "ajs_util.h"
#include <ajtcl/aj_msg_priv.h>

static AJ_Status MarshalStringArg(AJ_Message* msg, uint8_t typeId, const char* str)
{
    char sig[2];
    sig[0] = typeId;
    sig[1] = 0;

    AJ_InfoPrintf(("MarshalStringArg\n"));

    return AJ_MarshalArgs(msg, sig, str);
}

static AJ_Status MarshalJSBuffer(duk_context* ctx, AJ_Message* msg, uint8_t typeId, duk_idx_t idx)
{
    AJ_Status status;
    AJ_Arg arg;
    size_t len;

    void* buf = duk_get_buffer(ctx, idx, &len);
    status = AJ_MarshalArg(msg, AJ_InitArg(&arg, typeId, AJ_ARRAY_FLAG, buf, len));
    return status;
}

static AJ_Status MarshalScalarArg(AJ_Message* msg, uint8_t typeId, double n)
{
    AJ_Status status = AJ_ERR_SIGNATURE;
    char sig[2];

    sig[0] = typeId;
    sig[1] = 0;

    AJ_InfoPrintf(("MarshalScalarArg\n"));

    switch (typeId) {
    case AJ_ARG_BYTE:
        status = AJ_MarshalArgs(msg, sig, (uint8_t)n);
        break;

    case AJ_ARG_BOOLEAN:
        status = AJ_MarshalArgs(msg, sig, ((uint32_t)n) != 0);
        break;

    case AJ_ARG_UINT32:
        status = AJ_MarshalArgs(msg, sig, (uint32_t)n);
        break;

    case AJ_ARG_INT32:
        status = AJ_MarshalArgs(msg, sig, (int32_t)n);
        break;

    case AJ_ARG_INT16:
        status = AJ_MarshalArgs(msg, sig, (int16_t)n);
        break;

    case AJ_ARG_UINT16:
        status = AJ_MarshalArgs(msg, sig, (uint16_t)n);
        break;

    case AJ_ARG_DOUBLE:
        status = AJ_MarshalArgs(msg, sig, n);
        break;

    case AJ_ARG_UINT64:
        /* TODO this will lose precision for numbers ~> 2^53*/
        status = AJ_MarshalArgs(msg, sig, (uint64_t)n);
        break;

    case AJ_ARG_INT64:
        /* TODO this will lose precision for numbers ~> 2^53*/
        status = AJ_MarshalArgs(msg, sig, (int64_t)n);
        break;
    }
    return status;
}

static AJ_Status MarshalJSArg(duk_context* ctx, AJ_Message* msg, const char* sig, duk_idx_t idx);

static AJ_Status MarshalVariantArg(duk_context* ctx, AJ_Message* msg, duk_idx_t idx)
{
    AJ_Status status;
    const char* sig;

    duk_enum(ctx, idx, DUK_ENUM_OWN_PROPERTIES_ONLY);
    if (!duk_next(ctx, -1, 1) || duk_next(ctx, -3, 0)) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Variant must have a single signature");
    }
    sig = duk_to_string(ctx, -2);
    status = AJ_MarshalVariant(msg, sig);
    if (status == AJ_OK) {
        status = MarshalJSArg(ctx, msg, sig, -1);
    }
    duk_pop_3(ctx);
    return status;
}

static AJ_Status MarshalJSDict(duk_context* ctx, AJ_Message* msg, duk_idx_t idx)
{
    AJ_Arg container;
    AJ_Status status = AJ_MarshalContainer(msg, &container, AJ_ARG_ARRAY);
    if (status == AJ_OK) {
        duk_enum(ctx, idx, DUK_ENUM_OWN_PROPERTIES_ONLY);
        while ((status == AJ_OK) && duk_next(ctx, -1, 1)) {
            AJ_Arg entry;
            status = AJ_MarshalContainer(msg, &entry, AJ_ARG_DICT_ENTRY);
            if (status == AJ_OK) {
                status = MarshalJSArg(ctx, msg, NULL, -2);
                if (status == AJ_OK) {
                    status = MarshalJSArg(ctx, msg, NULL, -1);
                }
                duk_pop_2(ctx);
                if (status == AJ_OK) {
                    status = AJ_MarshalCloseContainer(msg, &entry);
                }
            }
        }
        duk_pop(ctx);
        if (status == AJ_OK) {
            status = AJ_MarshalCloseContainer(msg, &container);
        }
    }
    return status;
}

/*
 * Structs and arrays have the same representation in JavaScript. Ideally an AllJoyn struct would be
 * represented as a JavaScript object but properties are not positional so there would be no way to
 * map them to the appropriate struct element.
 */
static AJ_Status MarshalJSArray(duk_context* ctx, AJ_Message* msg, uint8_t typeId, duk_idx_t idx)
{
    AJ_Arg container;
    AJ_Status status = AJ_MarshalContainer(msg, &container, typeId);
    if (status == AJ_OK) {
        int n = 0;
        while (status == AJ_OK) {
            duk_get_prop_index(ctx, idx, n++);
            if (duk_is_undefined(ctx, -1)) {
                duk_pop(ctx);
                break;
            }
            status = MarshalJSArg(ctx, msg, NULL, duk_get_top_index(ctx));
            duk_pop(ctx);
        }
        if (status == AJ_OK) {
            status = AJ_MarshalCloseContainer(msg, &container);
        }
    }
    return status;
}

static AJ_Status MarshalJSArg(duk_context* ctx, AJ_Message* msg, const char* sig, duk_idx_t idx)
{
    AJ_Status status = AJ_ERR_SIGNATURE;
    uint8_t typeId;

    if (!sig) {
        sig = AJ_NextArgSig(msg);
        if (!sig) {
            return status;
        }
    }
    typeId = *sig++;

    if (!typeId) {
        return AJ_ERR_SIGNATURE;
    }
    if (AJ_IsScalarType(typeId)) {
        duk_to_number(ctx, idx);
        if (duk_is_number(ctx, idx)) {
            status = MarshalScalarArg(msg, typeId, duk_get_number(ctx, idx));
        }
    } else if (AJ_IsStringType(typeId)) {
        if (duk_is_string(ctx, idx)) {
            status = MarshalStringArg(msg, typeId, duk_get_string(ctx, idx));
        }
    } else if (typeId == AJ_ARG_VARIANT) {
        if (duk_is_object(ctx, idx)) {
            status = MarshalVariantArg(ctx, msg, idx);
        }
    } else if (typeId == AJ_ARG_STRUCT) {
        if (duk_is_array(ctx, idx)) {
            status = MarshalJSArray(ctx, msg, AJ_ARG_STRUCT, idx);
        }
    } else if (typeId == AJ_ARG_ARRAY) {
        if (duk_is_array(ctx, idx)) {
            status = MarshalJSArray(ctx, msg, AJ_ARG_ARRAY, idx);
        } else if (duk_is_buffer(ctx, idx) && AJ_IsScalarType(*sig)) {
            status = MarshalJSBuffer(ctx, msg, *sig, idx);
        } else if (*sig == AJ_ARG_DICT_ENTRY) {
            status = MarshalJSDict(ctx, msg, idx);
        }
    }
    return status;
}

static AJ_Status MarshalProp(duk_context* ctx, AJ_Message* msg, const char* propSig, duk_idx_t idx)
{
    AJ_Status status = AJ_MarshalVariant(msg, propSig);
    if (status == AJ_OK) {
        status = MarshalJSArg(ctx, msg, propSig, idx);
    }
    return status;
}

static AJ_Status MarshalPropEntry(duk_context* ctx, AJ_Message* msg, const char* propName, const char* propSig, duk_idx_t idx)
{
    AJ_Arg entry;
    AJ_Status status = AJ_MarshalContainer(msg, &entry, AJ_ARG_DICT_ENTRY);

    if (status == AJ_OK) {
        if (status == AJ_OK) {
            AJ_MarshalArgs(msg, "s", propName);
            status = MarshalProp(ctx, msg, propSig, idx);
        }
        if (status == AJ_OK) {
            status = AJ_MarshalCloseContainer(msg, &entry);
        }
    }
    return status;
}

static AJ_Status MarshalProperties(duk_context* ctx, AJ_Message* msg, const char* iface, duk_idx_t idx)
{
    AJ_Arg propList;
    AJ_Status status = AJ_MarshalContainer(msg, &propList, AJ_ARG_ARRAY);

    if (status != AJ_OK) {
        goto ExitProps;
    }
    /*
     * Ok if nothing was returned
     */
    if (duk_is_undefined(ctx, idx)) {
        goto ExitProps;
    }
    /*
     * Not ok if the value is not an object
     */
    if (!duk_is_object(ctx, idx)) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Requires object with values for each property to be returned");
    }
    /*
     * Ok if there are no interfaces
     */
    AJS_GetAllJoynProperty(ctx, "interfaceDefinition");
    if (!duk_is_object(ctx, -1) || NumProps(ctx, -1) == 0) {
        duk_pop(ctx);
        goto ExitProps;
    }
    /*
     * Ok if the interface doesn't exsit
     */
    duk_get_prop_string(ctx, -1, iface);
    if (!duk_is_object(ctx, -1)) {
        duk_pop_2(ctx);
        goto ExitProps;
    }
    /*
     * Iterate over the properties in the interface
     */
    duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
    while (duk_next(ctx, -1, 1) && (status == AJ_OK)) {
        if (duk_is_object(ctx, -1)) {
            if (AJS_GetIntProp(ctx, -1, "type") == AJS_MEMBER_TYPE_PROPERTY) {
                /*
                 * Skip write-only properties
                 */
                if (AJS_GetPropMemberAccess(ctx, -1) != AJS_PROP_ACCESS_W) {
                    const char* prop = duk_get_string(ctx, -2);
                    /*
                     * Is there is a value for this property
                     */
                    duk_get_prop_string(ctx, idx, prop);
                    if (!duk_is_undefined(ctx, -1)) {
                        const char* sig = AJS_GetStringProp(ctx, -2, "signature");
                        status = MarshalPropEntry(ctx, msg, prop, sig, -1);
                    }
                    duk_pop(ctx);

                }
            }
        }
        duk_pop_2(ctx);
    }
    duk_pop(ctx);

ExitProps:

    if (status == AJ_OK) {
        AJ_MarshalCloseContainer(msg, &propList);
    }
    return status;
}

static AJ_Status MarshalPropSet(duk_context* ctx, AJ_Message* msg)
{
    AJ_Status status = AJ_ERR_SIGNATURE;
    const char* iface = duk_require_string(ctx, 0);
    const char* prop = duk_require_string(ctx, 1);

    status = AJ_MarshalArgs(msg, "ss", iface, prop);
    if (status == AJ_OK) {
        uint8_t secure;
        uint32_t propId;
        const char* propSig;
        /*
         * This will get the type signature for the property
         */
        status = AJ_IdentifyProperty(msg, iface, prop, &propId, &propSig, &secure);
        if (status == AJ_OK) {
            if (secure) {
                msg->hdr->flags |=  AJ_FLAG_ENCRYPTED;
            }
            status = MarshalProp(ctx, msg, propSig, 2);
        }
    }
    return status;
}

static uint8_t IsSetProp(AJ_Message* msg)
{
    return (msg->hdr->msgType == AJ_MSG_METHOD_CALL) && (strcmp(msg->iface, AJ_PropertiesIface[0] + 1) == 0) && ((msg->msgId & 0xFF) == AJ_PROP_SET);
}

/*
 * Unpack the message header fields for a SIGNAL or METHOD call
 */
/*
 * Called with a list of args
 */
int AJS_MarshalSignal(duk_context* ctx)
{
    AJ_Status status = AJ_OK;
    AJS_MsgInfo* msgInfo;
    AJ_Message msg;
    duk_idx_t numArgs = duk_get_top(ctx);
    AJ_BusAttachment* aj = AJS_GetBusAttachment();
    duk_idx_t idx;
    const char* dest;
    uint8_t flags = 0;
    uint16_t ttl = 0;

    if (!AJS_IsRunning()) {
        duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "signal.send: not attached to AllJoyn");
    }
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "info");
    msgInfo = duk_get_buffer(ctx, -1, NULL);
    if (!msgInfo) {
        duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "info == NULL");
    }
    duk_pop(ctx);
    /*
     * Signal-specific properties
     */
    duk_get_prop_string(ctx, -1, "timeToLive");
    if (!duk_is_undefined(ctx, -1)) {
        ttl = duk_require_int(ctx, -1);
    }
    duk_pop(ctx);
    duk_get_prop_string(ctx, -1, "sessionless");
    if (duk_get_boolean(ctx, -1)) {
        flags |= AJ_FLAG_SESSIONLESS;
        if (!ttl) {
            ttl = AJS_DEFAULT_SLS_TTL;
        }
    }
    duk_pop(ctx);
    /*
     * We need to set the path for AJS_MarshalSignl to use
     */
    AJS_SetObjectPath(AJS_GetStringProp(ctx, -1, "path"));

    duk_pop(ctx);

    AJ_ASSERT(numArgs == duk_get_top(ctx));

    if (msgInfo->secure) {
        flags |= AJ_FLAG_ENCRYPTED;
    }
    if (msgInfo->dest[0] == 0) {
        /*
         * A signal with no destination and no session is as a global broadcast signal
         */
        if (!msgInfo->session) {
            flags |= AJ_FLAG_GLOBAL_BROADCAST;
        }
        dest = NULL;
    } else {
        dest = msgInfo->dest;
    }
    status = AJ_MarshalSignal(aj, &msg, msgInfo->msgId, dest, msgInfo->session, flags, ttl);
    for (idx = 0; (idx < numArgs) && (status == AJ_OK); ++idx) {
        status = MarshalJSArg(ctx, &msg, NULL, idx);
    }
    if (status == AJ_OK) {
        status = AJ_DeliverMsg(&msg);
    }
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "signal.send: %s", AJ_StatusText(status));
    }
    return 1;
}

static int NativeReplySetter(duk_context* ctx)
{
    uint32_t replySerial;

    if (!duk_is_callable(ctx, 0)) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "onReply requires a function");
    }
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "replySerial");
    replySerial = duk_get_int(ctx, -1);
    AJS_GetGlobalStashObject(ctx, "onReply");
    duk_dup(ctx, 0);
    duk_put_prop_index(ctx, -2, replySerial);
    duk_pop_3(ctx);
    return 0;
}

void AJS_PushReplyObject(duk_context* ctx, uint32_t replySerial)
{
    duk_push_object(ctx);
    duk_push_int(ctx, replySerial);
    duk_put_prop_string(ctx, -2, "replySerial");
    AJS_SetPropertyAccessors(ctx, -1, "onReply", NativeReplySetter, NULL);
}

/*
 * Called with a list of args
 */
int AJS_MarshalMethodCall(duk_context* ctx)
{
    AJ_Status status = AJ_OK;
    AJS_MsgInfo* msgInfo;
    AJ_Message msg;
    duk_idx_t numArgs = duk_get_top(ctx);
    AJ_BusAttachment* aj = AJS_GetBusAttachment();
    uint8_t flags = 0;
    uint16_t timeout = 0;
    uint32_t replySerial = 0;

    if (!AJS_IsRunning()) {
        duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "method.call: not attached to AllJoyn");
    }
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "info");
    msgInfo = duk_get_buffer(ctx, -1, NULL);
    if (!msgInfo) {
        duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "info == NULL");
    }
    duk_pop(ctx);
    /*
     * Method-specific properties
     */
    duk_get_prop_string(ctx, -1, "timeout");
    if (duk_is_number(ctx, -1)) {
        timeout = duk_get_int(ctx, -1);
    } else {
        AJS_GetAllJoynProperty(ctx, "config");
        duk_get_prop_string(ctx, -1, "callTimeout");
        timeout = duk_get_int(ctx, -1);
        duk_pop_2(ctx);
    }
    duk_pop(ctx);
    duk_get_prop_string(ctx, -1, "noReply");
    if (duk_get_boolean(ctx, -1)) {
        flags |= AJ_FLAG_NO_REPLY_EXPECTED;
    }
    duk_pop_2(ctx);

    AJ_ASSERT(numArgs == duk_get_top(ctx));

    if (msgInfo->secure) {
        flags |= AJ_FLAG_ENCRYPTED;
    }
    status = AJ_MarshalMethodCall(aj, &msg, msgInfo->msgId, msgInfo->dest, msgInfo->session, flags, timeout);
    if (IsSetProp(&msg)) {
        status = MarshalPropSet(ctx, &msg);
    } else {
        duk_idx_t idx;
        for (idx = 0; (idx < numArgs) && (status == AJ_OK); ++idx) {
            status = MarshalJSArg(ctx, &msg, NULL, idx);
        }
    }
    if (status == AJ_OK) {
        AJ_ASSERT(numArgs == duk_get_top(ctx));
        replySerial = msg.hdr->serialNum;
        status = AJ_DeliverMsg(&msg);
    }
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "method.call: %s", AJ_StatusText(status));
    }
    /*
     * Push the reply object - this is the return value from this function and used to register a reply handler.
     */
    if (flags & AJ_FLAG_NO_REPLY_EXPECTED) {
        duk_push_undefined(ctx);
    } else {
        AJS_PushReplyObject(ctx, replySerial);
    }
    return 1;
}

static int HandleReply(duk_context* ctx, const char* error)
{
    AJ_Status status = AJ_OK;
    AJ_MsgHeader header;
    AJ_Message msg;
    duk_idx_t numArgs = duk_get_top(ctx);
    AJS_ReplyInternal* msgReply;
    AJ_Message call;

    duk_push_this(ctx);
    /*
     * We stored information about the call in the message object
     */
    duk_get_prop_string(ctx, -1, AJS_HIDDEN_PROP("reply"));
    msgReply = duk_require_buffer(ctx, -1, NULL);
    duk_pop(ctx);
    /*
     * A zero indicates the reply has already been sent
     */
    if (msgReply->serialNum == 0) {
        duk_error(ctx, DUK_ERR_RANGE_ERROR, "reply already sent");
    }
    AJ_InfoPrintf(("Reply for serial %d\n", msgReply->serialNum));
    /*
     * Initialze a phoney header for the call
     */
    memset(&header, 0, sizeof(header));
    header.serialNum = msgReply->serialNum;
    header.flags = msgReply->flags;
    header.msgType = AJ_MSG_METHOD_CALL;
    /*
     * Zero out the serial number to indicate that the reply has been sent
     */
    msgReply->serialNum = 0;

    duk_get_prop_string(ctx, -1, "sender");
    call.sender = duk_require_string(ctx, -1);
    duk_pop(ctx);

    call.sessionId = msgReply->sessionId;
    call.msgId = msgReply->msgId;
    call.hdr = &header;
    call.bus = AJS_GetBusAttachment();

    if (error) {
        status = AJ_MarshalErrorMsgWithInfo(&call, &msg, AJ_ErrRejected, error);
    } else {
        status = AJ_MarshalReplyMsg(&call, &msg);
        if (status == AJ_OK) {
            /*
             * Property GET messages are a special case because we need the property signature to
             * figure out how to marshal the property value.
             */
            if (msgReply->accessor == AJ_PROP_GET) {
                duk_get_prop_string(ctx, -1, AJS_HIDDEN_PROP("propSig"));
                status = MarshalProp(ctx, &msg, duk_require_string(ctx, -1), 0);
                duk_pop(ctx);
            } else if (msgReply->accessor == AJ_PROP_GET_ALL) {
                duk_get_prop_string(ctx, -1, AJS_HIDDEN_PROP("propIface"));
                status = MarshalProperties(ctx, &msg, duk_require_string(ctx, -1), 0);
            } else {
                duk_idx_t idx;
                for (idx = 0; (idx < numArgs) && (status == AJ_OK); ++idx) {
                    status = MarshalJSArg(ctx, &msg, NULL, idx);
                }
            }
        }
    }
    /*
     * Pop the message (this) object
     */
    duk_pop(ctx);
    if (status == AJ_OK) {
        status = AJ_DeliverMsg(&msg);
    }
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "method reply: %s", AJ_StatusText(status));
    }
    return 1;
}

int AJS_MethodCallReply(duk_context* ctx)
{
    HandleReply(ctx, NULL);
    return 1;
}

int AJS_MethodCallError(duk_context* ctx)
{
    HandleReply(ctx, duk_require_string(ctx, 0));
    return 1;
}