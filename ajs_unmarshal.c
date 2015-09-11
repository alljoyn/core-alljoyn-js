/**
 * @file
 */
/******************************************************************************
 * Copyright (c) 2016 Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright 2016 Open Connectivity Foundation and Contributors to
 *    AllSeen Alliance. All rights reserved.
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
#include <aj_msg_priv.h>


duk_idx_t AJS_UnmarshalMessage(duk_context* ctx, AJ_Message* msg, uint8_t accessor)
{
    duk_idx_t objIndex = duk_push_object(ctx);

    duk_push_string(ctx, msg->sender);
    duk_put_prop_string(ctx, objIndex, "sender");

    if ((msg->hdr->msgType == AJ_MSG_METHOD_CALL) || (msg->hdr->msgType == AJ_MSG_SIGNAL)) {
        duk_push_string(ctx, msg->member);
        duk_put_prop_string(ctx, objIndex, "member");
        duk_push_string(ctx, msg->iface);
        duk_put_prop_string(ctx, objIndex, "iface");
        duk_push_string(ctx, msg->objPath);
        duk_put_prop_string(ctx, objIndex, "path");
        /* If sender == unique name then true, otherwise false */
        duk_push_boolean(ctx, !abs(strcmp(msg->sender, AJS_GetBusAttachment()->uniqueName)));
        duk_put_prop_string(ctx, objIndex, "fromSelf");

        if (msg->hdr->msgType == AJ_MSG_METHOD_CALL) {
            AJS_ReplyInternal* msgReply;
            /*
             * Private read-only information needed for composing the reply
             */
            duk_push_string(ctx, AJS_HIDDEN_PROP("reply"));
            msgReply = duk_push_fixed_buffer(ctx, sizeof(AJS_ReplyInternal));
            msgReply->msgId = msg->msgId;
            msgReply->flags = msg->hdr->flags;
            msgReply->serialNum = msg->hdr->serialNum;
            msgReply->sessionId = msg->sessionId;
            msgReply->accessor = accessor;
            duk_def_prop(ctx, objIndex, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_HAVE_WRITABLE);
            /*
             * Register the reply functions
             */
            duk_push_c_lightfunc(ctx, AJS_MethodCallReply, DUK_VARARGS, 0, 0);
            duk_put_prop_string(ctx, objIndex, "reply");
            duk_push_c_lightfunc(ctx, AJS_MethodCallError, DUK_VARARGS, 0, 0);
            duk_put_prop_string(ctx, objIndex, "errorReply");
        }
    } else {
        int isError = (msg->hdr->msgType == AJ_MSG_ERROR);

        AJ_InfoPrintf(("Reply serial %d\n", msg->replySerial));
        duk_push_int(ctx, msg->replySerial);
        duk_put_prop_string(ctx, objIndex, "replySerial");
        if (isError) {
            duk_push_string(ctx, msg->error);
            duk_put_prop_string(ctx, objIndex, "error");
        }
        duk_push_boolean(ctx, isError);
        duk_put_prop_string(ctx, objIndex, "isErrorReply");
    }
    return objIndex;
}

/*
 * Push a scalar array as a buffer.
 */
static AJ_Status PushScalarArrayArg(duk_context* ctx, uint8_t typeId, AJ_Message* msg)
{
    AJ_Arg arg;
    AJ_Status status = AJ_UnmarshalArg(msg, &arg);
    void* buf = duk_push_fixed_buffer(ctx, arg.len);

    if (buf) {
        memcpy(buf, arg.val.v_data, arg.len);
    } else {
        status = AJ_ERR_RESOURCES;
    }
    return status;
}

static AJ_Status PushScalarArg(duk_context* ctx, AJ_Message* msg)
{
    AJ_Arg arg;
    AJ_Status status = AJ_UnmarshalArg(msg, &arg);

    if (status == AJ_OK) {
        switch (arg.typeId) {
        case AJ_ARG_BYTE:
            duk_push_uint(ctx, (uint32_t)(*arg.val.v_byte));
            break;

        case AJ_ARG_BOOLEAN:
            duk_push_boolean(ctx, *arg.val.v_bool);
            break;

        case AJ_ARG_UINT32:
            duk_push_uint(ctx, *arg.val.v_uint32);
            break;

        case AJ_ARG_INT32:
            duk_push_int(ctx, *arg.val.v_int32);
            break;

        case AJ_ARG_UINT16:
            duk_push_uint(ctx, *arg.val.v_uint16);
            break;

        case AJ_ARG_INT16:
            duk_push_int(ctx, *arg.val.v_int16);
            break;

        case AJ_ARG_DOUBLE:
            duk_push_number(ctx, *arg.val.v_double);
            break;

        case AJ_ARG_UINT64:
            /* TODO this will lose precision for numbers ~> 2^52*/
            duk_push_number(ctx, (double)*arg.val.v_uint64);
            break;

        case AJ_ARG_INT64:
            /* TODO this will lose precision for numbers ~> 2^52*/
            duk_push_number(ctx, (double)*arg.val.v_int64);
            break;
        }
    }
    return status;
}

static AJ_Status PushArg(duk_context* ctx, AJ_Message* msg);

/*
 * An AllJoyn dictionary entry maps cleanly to a JavaScript object property
 */
static AJ_Status PushDictionaryArg(duk_context* ctx, AJ_Message* msg)
{
    AJ_Arg arg;
    AJ_Status status = AJ_UnmarshalContainer(msg, &arg, AJ_ARG_ARRAY);
    if (status == AJ_OK) {
        int objIndex = duk_push_object(ctx);
        while (status == AJ_OK) {
            AJ_Arg entry;
            status = AJ_UnmarshalContainer(msg, &entry, AJ_ARG_DICT_ENTRY);
            if (status == AJ_OK) {
                status = PushArg(ctx, msg); // key
                if (status == AJ_OK) {
                    status = PushArg(ctx, msg); // value
                }
                if (status == AJ_OK) {
                    duk_put_prop(ctx, objIndex);
                }
                AJ_UnmarshalCloseContainer(msg, &entry);
            }
        }
        if (status == AJ_ERR_NO_MORE) {
            status = AJ_OK;
        }
        AJ_UnmarshalCloseContainer(msg, &arg);
    }
    return status;
}

/*
 * Structs and arrays have the same representation in JavaScript. Ideally an AllJoyn struct would be
 * represented as a JavaScript object but struct elements are not named in the wire protocol so
 * there is no way to map them into object properties.
 */
static AJ_Status PushContainerArg(duk_context* ctx, uint8_t typeId, AJ_Message* msg)
{
    AJ_Arg arg;
    AJ_Status status = AJ_UnmarshalContainer(msg, &arg, typeId);
    if (status == AJ_OK) {
        int elem = 0;
        int arrayIndex = duk_push_array(ctx);
        while (status == AJ_OK) {
            status = PushArg(ctx, msg);
            if (status == AJ_OK) {
                duk_put_prop_index(ctx, arrayIndex, elem++);
            }
        }
        if (status == AJ_ERR_NO_MORE) {
            status = AJ_OK;
        }
        AJ_UnmarshalCloseContainer(msg, &arg);
    }
    return status;
}

static AJ_Status PushArg(duk_context* ctx, AJ_Message* msg)
{
    AJ_Status status = AJ_ERR_DISALLOWED;
    AJ_Arg arg;
    const char* sig = AJ_NextArgSig(msg);
    uint8_t typeId;

    if (!sig || !*sig) {
        return AJ_ERR_NO_MORE;
    }
    typeId = *sig;

    /*
     * De-variant the argument
     */
    while (typeId == AJ_ARG_VARIANT) {
        status = AJ_UnmarshalVariant(msg, &sig);
        if (status != AJ_OK) {
            return status;
        }
        typeId = *sig;
    }
    if (typeId == ')') {
        status = AJ_ERR_NO_MORE;
    } else if (AJ_IsScalarType(typeId)) {
        status = PushScalarArg(ctx, msg);
    } else if (AJ_IsStringType(typeId)) {
        status = AJ_UnmarshalArg(msg, &arg);
        if (status == AJ_OK) {
            duk_push_string(ctx, arg.val.v_string);
        }
    } else if (typeId == AJ_ARG_STRUCT) {
        status = PushContainerArg(ctx, AJ_ARG_STRUCT, msg);
    } else if (typeId == AJ_ARG_ARRAY) {
        if (sig[1] == AJ_ARG_DICT_ENTRY) {
            status = PushDictionaryArg(ctx, msg);
        } else if (sig[1] == AJ_ARG_BYTE) {
            status = PushScalarArrayArg(ctx, sig[1], msg);
        } else {
            status = PushContainerArg(ctx, AJ_ARG_ARRAY, msg);
        }
    }
    return status;
}

AJ_Status AJS_UnmarshalPropArgs(duk_context* ctx, AJ_Message* msg, uint8_t accessor, duk_idx_t msgIdx)
{
    AJ_Status status;
    const char* iface;
    const char* prop;
    const char* signature;
    uint32_t propId;
    uint8_t secure = FALSE;

    AJ_InfoPrintf(("PushPropArgs\n"));

    if (accessor == AJ_PROP_GET_ALL) {
        status = AJ_UnmarshalArgs(msg, "s", &iface);
        if (status == AJ_OK) {
            duk_push_string(ctx, iface);
            /*
             * Save interface (read-only) so we know how to marshal the reply values
             */
            duk_push_string(ctx, AJS_HIDDEN_PROP("propIface"));
            duk_dup(ctx, -2);
            duk_def_prop(ctx, msgIdx, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_HAVE_WRITABLE);
        }
        /*
         * This call always returns an error status because the interface name we are passing in is
         * invalid but it will indicate if security is required so we can perform the check below
         */
        AJ_IdentifyProperty(msg, iface, "", &propId, &signature, &secure);
    } else {
        status = AJ_UnmarshalArgs(msg, "ss", &iface, &prop);
        if (status == AJ_OK) {
            status = AJ_IdentifyProperty(msg, iface, prop, &propId, &signature, &secure);
            if (status == AJ_OK) {
                duk_push_string(ctx, iface);
                duk_push_string(ctx, prop);
                if (accessor == AJ_PROP_GET) {
                    /*
                     * If we are getting a property save the signature so we know how to marshal the
                     * value in the reply.
                     */
                    duk_push_string(ctx, AJS_HIDDEN_PROP("propSig"));
                    duk_push_string(ctx, signature);
                    duk_def_prop(ctx, msgIdx, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_HAVE_WRITABLE);
                } else {
                    /*
                     * Push the value to set
                     */
                    status = PushArg(ctx, msg);
                }
            }
        }
    }
    /*
     * If the interface is secure check the message is encrypted
     */
    if ((status == AJ_OK) && secure && !(msg->hdr->flags & AJ_FLAG_ENCRYPTED)) {
        status = AJ_ERR_SECURITY;
        AJ_WarnPrintf(("Security violation accessing property\n"));
    }
    return status;
}

AJ_Status AJS_UnmarshalMsgArgs(duk_context* ctx, AJ_Message* msg)
{
    AJ_Status status = AJ_OK;

    while (status == AJ_OK) {
        status = PushArg(ctx, msg);
    }
    if (status == AJ_ERR_NO_MORE) {
        status = AJ_OK;
    }
    return status;
}
