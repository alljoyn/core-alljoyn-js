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
#include "ajs_services.h"
#include <aj_msg_priv.h>
#include <aj_link_timeout.h>
#include <alljoyn/controlpanel/ControlPanelService.h>

static uint8_t IsPropAccessor(AJ_Message* msg)
{
    if (strcmp(msg->iface, AJ_PropertiesIface[0] + 1) == 0) {
        return msg->msgId & 0xFF;
    } else {
        return AJS_NOT_ACCESSOR;
    }
}

static AJ_Status SessionBindReply(duk_context* ctx, AJ_Message* msg)
{
    AJ_Status status;
    uint32_t result;
    uint16_t port;

    status = AJ_UnmarshalArgs(msg, "uq", &result, &port);
    if (status != AJ_OK) {
        return status;
    }
    if (port != AJS_APP_PORT) {
        AJ_ResetArgs(msg);
        status = AJS_ConsoleMsgHandler(ctx, msg);
        if (status == AJ_ERR_NO_MATCH) {
            status = AJS_ServicesMsgHandler(msg);
            if (status == AJ_ERR_NO_MATCH) {
                status = AJ_OK;
            }
        }
    }
    return status;
}

static AJ_Status SessionDispatcher(duk_context* ctx, AJ_Message* msg)
{
    AJ_Status status;
    uint32_t sessionId;
    uint16_t port;
    char* joiner;

    status = AJ_UnmarshalArgs(msg, "qus", &port, &sessionId, &joiner);
    if (status != AJ_OK) {
        return status;
    }
    /*
     * Automatically accept sessions requests to the application port
     */
    if (port == AJS_APP_PORT) {
        return AJS_HandleAcceptSession(ctx, msg, port, sessionId, joiner);
    }
    /*
     * See if this is the control panel port
     */
    if (AJCPS_CheckSessionAccepted(port, sessionId, joiner)) {
        return AJ_BusReplyAcceptSession(msg, TRUE);
    }
    status = AJ_ResetArgs(msg);
    if (status != AJ_OK) {
        return status;
    }
    /*
     * JavaScript doesn't accept/reject session so the session is either for the console or perhaps
     * a service if the services bind their own ports.
     */
    status = AJS_ConsoleMsgHandler(ctx, msg);
    if (status == AJ_ERR_NO_MATCH) {
        status = AJS_ServicesMsgHandler(msg);
    }
    if (status == AJ_ERR_NO_MATCH) {
        AJ_ErrPrintf(("SessionDispatcher rejecting attempt to join unbound port %d\n", port));
        status = AJ_BusReplyAcceptSession(msg, FALSE);
    }
    return status;
}

static AJ_Status HandleMessage(duk_context* ctx, duk_idx_t ajIdx, AJ_Message* msg)
{
    duk_idx_t msgIdx;
    uint8_t accessor = AJS_NOT_ACCESSOR;
    const char* func;
    AJ_Status status;

    status = AJS_ConsoleMsgHandler(ctx, msg);
    if (status != AJ_ERR_NO_MATCH) {
        if (status != AJ_OK) {
            AJ_WarnPrintf(("AJS_ConsoleMsgHandler returned %s\n", AJ_StatusText(status)));
        }
        return status;
    }
    /*
     * JOIN_SESSION replies are handled in the AllJoyn.js layer and don't get passed to JavaScript
     */
    if (msg->msgId == AJ_REPLY_ID(AJ_METHOD_JOIN_SESSION)) {
        return AJS_HandleJoinSessionReply(ctx, msg);
    }
    /*
     * Nothing more to do if the AllJoyn module was not loaded
     */
    if (ajIdx < 0) {
        return AJ_OK;
    }
    /*
     * Let the bases services layer take a look at the message
     */
    status = AJS_ServicesMsgHandler(msg);
    if (status != AJ_ERR_NO_MATCH) {
        if (status != AJ_OK) {
            AJ_WarnPrintf(("AJS_ServicesMsgHandler returned %s\n", AJ_StatusText(status)));
        }
        return status;
    }
    /*
     * Push the appropriate callback function onto the duktape stack
     */
    if (msg->hdr->msgType == AJ_MSG_SIGNAL) {
        /*
         * About announcements and found name signal get special handling
         */
        if (msg->msgId == AJ_SIGNAL_ABOUT_ANNOUNCE) {
            return AJS_AboutAnnouncement(ctx, msg);
        }
        if (msg->msgId == AJ_SIGNAL_FOUND_ADV_NAME) {
            return AJS_FoundAdvertisedName(ctx, msg);
        }
        if ((msg->msgId == AJ_SIGNAL_SESSION_LOST) || (msg->msgId == AJ_SIGNAL_SESSION_LOST_WITH_REASON)) {
            return AJS_SessionLost(ctx, msg);
        }
        func = "onSignal";
        duk_get_prop_string(ctx, ajIdx, func);
    } else if (msg->hdr->msgType == AJ_MSG_METHOD_CALL) {
        accessor = IsPropAccessor(msg);
        switch (accessor) {
        case AJS_NOT_ACCESSOR:
            func = "onMethodCall";
            break;

        case AJ_PROP_GET:
            func = "onPropGet";
            break;

        case AJ_PROP_SET:
            func = "onPropSet";
            break;

        case AJ_PROP_GET_ALL:
            func = "onPropGetAll";
            break;

        default:
            return AJ_ERR_INVALID;
        }
        duk_get_prop_string(ctx, ajIdx, func);
    } else {
        func = "onReply";
        AJS_GetGlobalStashObject(ctx, func);
        if (duk_is_object(ctx, -1)) {
            duk_get_prop_index(ctx, -1, msg->replySerial);
            duk_swap_top(ctx, -2);
            /*
             * Clear the onReply entry
             */
            duk_del_prop_index(ctx, -1, msg->replySerial);
            duk_pop(ctx);
        }
    }
    /*
     * Skip if there is no function to call.
     */
    if (!duk_is_function(ctx, -1)) {
        if (msg->hdr->msgType == AJ_MSG_METHOD_CALL) {
            AJ_Message error;
            AJ_WarnPrintf(("%s: not registered - rejecting message\n", func));
            AJ_MarshalErrorMsg(msg, &error, AJ_ErrRejected);
            status = AJ_DeliverMsg(&error);
        } else {
            AJ_WarnPrintf(("%s: not registered - ignoring message\n", func));
            status = AJ_OK;
        }
        duk_pop(ctx);
        return status;
    }
    msgIdx = AJS_UnmarshalMessage(ctx, msg, accessor);
    AJ_ASSERT(msgIdx == (ajIdx + 2));
    /*
     * Special case for GET prop so we can get the signature for marshalling the result
     */
    if (accessor == AJS_NOT_ACCESSOR) {
        status = AJS_UnmarshalMsgArgs(ctx, msg);
    } else {
        status = AJS_UnmarshalPropArgs(ctx, msg, accessor, msgIdx);
    }
    if (status == AJ_OK) {
        duk_idx_t numArgs = duk_get_top(ctx) - msgIdx - 1;
        if (duk_pcall_method(ctx, numArgs) != DUK_EXEC_SUCCESS) {
            AJ_ErrPrintf(("%s: %s\n", func, duk_safe_to_string(ctx, -1)));
        }
        duk_pop(ctx);
    } else {
        /*
         * Cleanup stack back to the AJ object
         */
        duk_set_top(ctx, ajIdx);
    }
    return status;
}

AJ_Status AJS_MessageLoop(duk_context* ctx, AJ_BusAttachment* aj, duk_idx_t ajIdx)
{
    AJ_Status status = AJ_OK;
    AJ_Message msg;
    AJ_Time timerClock;
    uint32_t linkTO;
    uint32_t msgTO = 0x7FFFFFFF;
    duk_idx_t top = duk_get_top_index(ctx);

    AJ_InfoPrintf(("AJS_MessageLoop top=%d\n", (int)top));

    if (ajIdx >= 0) {
        /*
         * Read the link timeout property from the config set
         */
        duk_get_prop_string(ctx, -1, "config");
        duk_get_prop_string(ctx, -1, "linkTimeout");
        linkTO = duk_get_int(ctx, -1);
        duk_pop_2(ctx);
        AJ_SetBusLinkTimeout(aj, linkTO);
    }
    /*
     * timer clock must be initialized
     */
    AJ_GetElapsedTime(&timerClock, FALSE);

    AJ_ASSERT(duk_get_top_index(ctx) == top);

    /*
     * Initialize About we can start sending announcements
     */
    AJ_AboutInit(aj, AJS_APP_PORT);

    while (status == AJ_OK) {
        /*
         * Check we are cleaning up the duktape stack correctly.
         */
        if (duk_get_top_index(ctx) != top) {
            AJ_ErrPrintf(("!!!AJS_MessageLoop top=%d expected %d\n", (int)duk_get_top_index(ctx), (int)top));
            AJS_DumpStack(ctx);
            AJ_ASSERT(duk_get_top_index(ctx) == top);
        }
        /*
         * Pinned strings are only valid while running script
         */
        AJS_ClearPinnedStrings(ctx);
        /*
         * Check if there are any pending I/O operations to perform.
         */
        status = AJS_ServiceIO(ctx);
        if (status != AJ_OK) {
            AJ_ErrPrintf(("Error servicing I/O functions\n"));
            break;
        }
        /*
         * Services the internal and timeout timers and updates the timeout value for any new
         * timers that have been registered since this function was last called.
         */
        status = AJS_RunTimers(ctx, &timerClock, &msgTO);
        if (status != AJ_OK) {
            AJ_ErrPrintf(("Error servicing timer functions\n"));
            break;
        }
        /*
         * Do any announcing required
         */
        status = AJ_AboutAnnounce(aj);
        if (status != AJ_OK) {
            break;
        }
        /*
         * This special wildcard allows us to unmarshal signals with any source path
         */
        AJS_SetObjectPath("!");
        /*
         * Block until a message is received, the timeout expires, or the operation is interrupted.
         */
        status = AJ_UnmarshalMsg(aj, &msg, msgTO);
        if (status != AJ_OK) {
            if ((status == AJ_ERR_INTERRUPTED) || (status == AJ_ERR_TIMEOUT)) {
                status = AJ_OK;
                continue;
            }
            if (status == AJ_ERR_NO_MATCH) {
                status = AJ_OK;
            }
            AJ_CloseMsg(&msg);
            continue;
        }

        switch (msg.msgId) {
        case 0:
            /* If a message was parsed but is unrecognized and should be ignored */
            break;

        /* Introspection messages */
        case AJ_METHOD_PING:
        case AJ_METHOD_BUS_PING:
        case AJ_METHOD_GET_MACHINE_ID:
        case AJ_METHOD_INTROSPECT:
        case AJ_METHOD_GET_DESCRIPTION_LANG:
        case AJ_METHOD_INTROSPECT_WITH_DESC:
        /* About messages */
        case AJ_METHOD_ABOUT_GET_PROP:
        case AJ_METHOD_ABOUT_SET_PROP:
        case AJ_METHOD_ABOUT_GET_ABOUT_DATA:
        case AJ_METHOD_ABOUT_GET_OBJECT_DESCRIPTION:
        case AJ_METHOD_ABOUT_ICON_GET_PROP:
        case AJ_METHOD_ABOUT_ICON_SET_PROP:
        case AJ_METHOD_ABOUT_ICON_GET_URL:
        case AJ_METHOD_ABOUT_ICON_GET_CONTENT:
        /* Authentication messages and replies */
        case AJ_METHOD_EXCHANGE_GUIDS:
        case AJ_REPLY_ID(AJ_METHOD_EXCHANGE_GUIDS):
        case AJ_METHOD_EXCHANGE_SUITES:
        case AJ_REPLY_ID(AJ_METHOD_EXCHANGE_SUITES):
        case AJ_METHOD_AUTH_CHALLENGE:
        case AJ_REPLY_ID(AJ_METHOD_AUTH_CHALLENGE):
        case AJ_METHOD_GEN_SESSION_KEY:
        case AJ_REPLY_ID(AJ_METHOD_GEN_SESSION_KEY):
        case AJ_METHOD_EXCHANGE_GROUP_KEYS:
        case AJ_REPLY_ID(AJ_METHOD_EXCHANGE_GROUP_KEYS):
        case AJ_METHOD_KEY_EXCHANGE:
        case AJ_REPLY_ID(AJ_METHOD_KEY_EXCHANGE):
        case AJ_METHOD_KEY_AUTHENTICATION:
        case AJ_REPLY_ID(AJ_METHOD_KEY_AUTHENTICATION):
        /* Replies the app ignores */
        case AJ_REPLY_ID(AJ_METHOD_ADD_MATCH):
        case AJ_REPLY_ID(AJ_METHOD_REMOVE_MATCH):
        case AJ_REPLY_ID(AJ_METHOD_PING):
        case AJ_REPLY_ID(AJ_METHOD_BUS_PING):
        /* Signals the app ignores */
        case AJ_SIGNAL_PROBE_ACK:
        case AJ_SIGNAL_PROBE_REQ:
        case AJ_SIGNAL_NAME_OWNER_CHANGED:
            status = AJ_BusHandleBusMessage(&msg);
            break;

        case AJ_METHOD_ACCEPT_SESSION:
            status = SessionDispatcher(ctx, &msg);
            break;

        case AJ_REPLY_ID(AJ_METHOD_BIND_SESSION_PORT):
            status = SessionBindReply(ctx, &msg);
            break;

        default:
            status = HandleMessage(ctx, ajIdx, &msg);
            break;
        }
        /*
         * Free message resources
         */
        AJ_CloseMsg(&msg);
        /*
         * Decide which messages should cause us to exit
         */
        switch (status) {
        case AJ_OK:
            break;

        case AJ_ERR_READ:
        case AJ_ERR_WRITE:
        case AJ_ERR_RESTART:
        case AJ_ERR_RESTART_APP:
            break;

        default:
            AJ_ErrPrintf(("Got error %s - continuing anyway\n", AJ_StatusText(status)));
            status = AJ_OK;
        }
        /*
         * Let link monitor know we are getting messages
         */
        AJ_NotifyLinkActive();
    }
    return status;
}
