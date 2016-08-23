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
#include "ajs_services.h"
#include "ajs_debugger.h"

static uint32_t hasPolicyChanged = FALSE;

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
#if !defined(AJS_CONSOLE_LOCKDOWN)
    uint8_t ldstate;
#endif

    status = AJ_UnmarshalArgs(msg, "uq", &result, &port);
    if (status != AJ_OK) {
        return status;
    }
    if (port != AJS_APP_PORT) {
        AJ_ResetArgs(msg);
#if !defined(AJS_CONSOLE_LOCKDOWN)
        status = AJS_GetLockdownState(&ldstate);
        if (status == AJ_OK && ldstate == AJS_CONSOLE_UNLOCKED) {
            status = AJS_ConsoleMsgHandler(ctx, msg);
        }
#endif
        if (status == AJ_ERR_NO_MATCH) {
            status = AJS_ServicesMsgHandler(msg);
            if (status == AJ_ERR_NO_MATCH) {
                status = AJ_OK;
            }
        }
    }
    return status;
}

void AJS_QueueNotifPolicyChanged()
{
    hasPolicyChanged += 1;
}

static AJ_Status SessionDispatcher(duk_context* ctx, AJ_Message* msg)
{
    AJ_Status status;
    uint32_t sessionId;
    uint16_t port;
    char* joiner;
#if !defined(AJS_CONSOLE_LOCKDOWN)
    uint8_t ldstate;
#endif

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
#ifdef CONTROLPANEL_SERVICE
    if (AJCPS_CheckSessionAccepted(port, sessionId, joiner)) {
        return AJ_BusReplyAcceptSession(msg, TRUE);
    }
#endif
    status = AJ_ResetArgs(msg);
    if (status != AJ_OK) {
        return status;
    }
    /*
     * JavaScript doesn't accept/reject session so the session is either for the console or perhaps
     * a service if the services bind their own ports.
     */
#if !defined(AJS_CONSOLE_LOCKDOWN)
    status = AJS_GetLockdownState(&ldstate);
    if (status == AJ_OK && ldstate == AJS_CONSOLE_UNLOCKED) {
        status = AJS_ConsoleMsgHandler(ctx, msg);
    }
#endif
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
#if !defined(AJS_CONSOLE_LOCKDOWN)
    uint8_t ldstate;

    status = AJS_GetLockdownState(&ldstate);
    if (status == AJ_OK && ldstate == AJS_CONSOLE_UNLOCKED) {
        status = AJS_ConsoleMsgHandler(ctx, msg);
        if (status != AJ_ERR_NO_MATCH) {
            if (status != AJ_OK) {
                AJ_WarnPrintf(("AJS_ConsoleMsgHandler returned %s\n", AJ_StatusText(status)));
            }
            return status;
        }
    }
#endif
    /*
     * JOIN_SESSION replies are handled in the AllJoyn.js layer and don't get passed to JavaScript
     */
    if (msg->msgId == AJ_REPLY_ID(AJ_METHOD_JOIN_SESSION)) {
        return AJS_HandleJoinSessionReply(ctx, msg);
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
     * Nothing more to do if the AllJoyn module was not loaded
     */
    if (ajIdx < 0) {
        return AJ_OK;
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
#if !defined(AJS_CONSOLE_LOCKDOWN)
            if (AJS_DebuggerIsAttached()) {
                msg = AJS_CloneAndCloseMessage(ctx, msg);
            }
#endif
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
    /*
     * Opens up a stack entry above the function
     */
    duk_dup_top(ctx);
    msgIdx = AJS_UnmarshalMessage(ctx, msg, accessor);
    AJ_ASSERT(msgIdx == (ajIdx + 3));
    /*
     * Save the message object on the stack
     */
    duk_copy(ctx, msgIdx, -3);
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
        /*
         * If attached, the debugger will begin to unmarshal a message when the
         * method handler is called, therefore it must be cloned-and-closed now.
         */
#if !defined(AJS_CONSOLE_LOCKDOWN)
        if (AJS_DebuggerIsAttached()) {
            msg = AJS_CloneAndCloseMessage(ctx, msg);
        }
#endif
        if (duk_pcall_method(ctx, numArgs) != DUK_EXEC_SUCCESS) {
            AJ_ErrPrintf(("%s: %s\n", func, duk_safe_to_string(ctx, -1)));
#if !defined(AJS_CONSOLE_LOCKDOWN)
            AJS_ThrowHandler(ctx);
#endif
            /*
             * Generate an error reply if this was a method call
             */
            if (msg->hdr->msgType == AJ_MSG_METHOD_CALL) {
                duk_push_c_lightfunc(ctx, AJS_MethodCallError, 1, 0, 0);
                duk_insert(ctx, -3);
                (void)duk_pcall_method(ctx, 1);
            }
        }
    }
    /*
     * Cleanup stack back to the AJ object
     */
    duk_set_top(ctx, ajIdx + 1);
    return status;
}

void ProcessPolicyNotifications(duk_context* ctx, duk_idx_t ajIdx)
{
    if (!hasPolicyChanged) {
        return;
    }

    hasPolicyChanged -= 1;

    duk_get_prop_string(ctx, ajIdx, "onPolicyChanged");
    if (!duk_is_function(ctx, -1)) {
        AJ_WarnPrintf(("onPolicyChanged: not registered - ignoring message\n"));
        duk_pop_2(ctx);
        return;
    }

    if (duk_pcall_method(ctx, 0) != DUK_EXEC_SUCCESS) {
        AJ_ErrPrintf(("onPolicyChanged: %s\n", duk_safe_to_string(ctx, -1)));
    }

    duk_pop_2(ctx);
}

static AJS_DEFERRED_OP deferredOp = AJS_OP_NONE;

void AJS_DeferredOperation(duk_context* ctx, AJS_DEFERRED_OP op)
{
    deferredOp = op;
}

static AJ_Status DoDeferredOperation(duk_context* ctx)
{
    AJ_Status status = AJ_ERR_RESTART;

    switch (deferredOp) {
#ifdef ONBOARDING_SERVICE
    case AJS_OP_OFFBOARD:
        AJOBS_ClearInfo();
        AJS_DetachAllJoyn(AJS_GetBusAttachment(), AJ_ERR_RESTART);
        break;
#endif

    case AJS_OP_FACTORY_RESET:
        AJS_FactoryReset();
        break;

    default:
        status = AJ_OK;
        break;
    }
    deferredOp = AJS_OP_NONE;
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
    uint8_t ldstate;

    AJ_InfoPrintf(("AJS_MessageLoop top=%d\n", (int)top));

    deferredOp = AJS_OP_NONE;

    if (ajIdx >= 0) {
        /*
         * Read the link timeout property from the config set
         */
        duk_get_prop_string(ctx, ajIdx, "config");
        duk_get_prop_string(ctx, ajIdx, "linkTimeout");
        linkTO = duk_get_int(ctx, -1);
        duk_pop_2(ctx);
        AJ_SetBusLinkTimeout(aj, linkTO);
    }
    AJ_ASSERT(duk_get_top_index(ctx) == top);

    /*
     * Initialize About we can start sending announcements
     */
    AJ_AboutInit(aj, AJS_APP_PORT);
    /*
     * timer clock must be initialized
     */
    AJ_InitTimer(&timerClock);


    while (status == AJ_OK) {
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
         * Check we are cleaning up the duktape stack correctly.
         */
        if (duk_get_top_index(ctx) != top) {
            AJ_ErrPrintf(("!!!AJS_MessageLoop top=%d expected %d\n", (int)duk_get_top_index(ctx), (int)top));
            AJS_DumpStack(ctx);
            AJ_ASSERT(duk_get_top_index(ctx) == top);
        }
        AJS_SetWatchdogTimer(AJS_DEFAULT_WATCHDOG_TIMEOUT);
        /*
         * Pinned items (strings/buffers) are only valid while running script
         */
        AJS_ClearPins(ctx);
        /*
         * Check if there are any pending I/O operations to perform.
         */
        status = AJS_ServiceIO(ctx);
        if (status != AJ_OK) {
            AJ_ErrPrintf(("Error servicing I/O functions\n"));
            break;
        }
        /*
         * Check if any external modules have operations to perform
         */
        status = AJS_ServiceExtModules(ctx);
        if (status != AJ_OK) {
            AJ_ErrPrintf(("Error servicing external modules\n"));
            break;
        }
        /*
         * Service any pending session joining
         */
        status = AJS_ServiceSessions(ctx);
        if (status != AJ_OK) {
            AJ_ErrPrintf(("Error servicing sessions\n"));
            break;
        }

        /*
         * Do any announcing required
         */
        status = AJS_GetLockdownState(&ldstate);
        if (status == AJ_OK && ldstate == AJS_CONSOLE_UNLOCKED) {
            status = AJ_AboutAnnounce(aj);
        }
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

        ProcessPolicyNotifications(ctx, ajIdx);

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
        case AJ_METHOD_SEND_MANIFESTS:
        case AJ_REPLY_ID(AJ_METHOD_SEND_MANIFESTS):
        case AJ_METHOD_SEND_MEMBERSHIPS:
        case AJ_REPLY_ID(AJ_METHOD_SEND_MEMBERSHIPS):
        case AJ_METHOD_SECURITY_GET_PROP:
        case AJ_REPLY_ID(AJ_METHOD_SECURITY_GET_PROP):
        case AJ_METHOD_CLAIMABLE_CLAIM:
        case AJ_REPLY_ID(AJ_METHOD_CLAIMABLE_CLAIM):
        case AJ_METHOD_MANAGED_RESET:
        case AJ_REPLY_ID(AJ_METHOD_MANAGED_RESET):
        case AJ_METHOD_MANAGED_UPDATE_IDENTITY:
        case AJ_REPLY_ID(AJ_METHOD_MANAGED_UPDATE_IDENTITY):
        case AJ_METHOD_MANAGED_UPDATE_POLICY:
        case AJ_REPLY_ID(AJ_METHOD_MANAGED_UPDATE_POLICY):
        case AJ_METHOD_MANAGED_RESET_POLICY:
        case AJ_REPLY_ID(AJ_METHOD_MANAGED_RESET_POLICY):
        case AJ_METHOD_MANAGED_INSTALL_MEMBERSHIP:
        case AJ_REPLY_ID(AJ_METHOD_MANAGED_INSTALL_MEMBERSHIP):
        case AJ_METHOD_MANAGED_REMOVE_MEMBERSHIP:
        case AJ_REPLY_ID(AJ_METHOD_MANAGED_REMOVE_MEMBERSHIP):
        case AJ_METHOD_MANAGED_START_MANAGEMENT:
        case AJ_REPLY_ID(AJ_METHOD_MANAGED_START_MANAGEMENT):
        case AJ_METHOD_MANAGED_END_MANAGEMENT:
        case AJ_REPLY_ID(AJ_METHOD_MANAGED_END_MANAGEMENT):
        case AJ_METHOD_MANAGED_INSTALL_MANIFESTS:
        case AJ_REPLY_ID(AJ_METHOD_MANAGED_INSTALL_MANIFESTS):
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
        /*
         * Perform any deferred operations. These are operations such as factory reset that cannot
         * be cleanly performed from inside duktape.
         */
        if (status == AJ_OK) {
            status = DoDeferredOperation(ctx);
        }
    }
    AJS_ClearPins(ctx);
    AJS_ClearWatchdogTimer();
    return status;
}