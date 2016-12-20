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
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include "ajs.h"
#include "ajs_util.h"
#include "ajs_debugger.h"
#include <ajtcl/aj_msg_priv.h>

#include <ajtcl/aj_cert.h>
#include <ajtcl/aj_peer.h>
#include <ajtcl/aj_creds.h>
#include <ajtcl/aj_auth_listener.h>
#include <ajtcl/aj_authentication.h>
#include <ajtcl/aj_authorisation.h>
#include <ajtcl/aj_security.h>

typedef enum {
    AJS_SVC_AUTHENTICATED   = 0,    /* Auth has finished but callback has not been called */
    AJS_SVC_NO_AUTH         = 1,    /* No auth is needed for this connection */
    AJS_SVC_AUTHENTICATING  = 2,    /* Currently authenticating */
    AJS_SVC_AUTH_DONE       = 3,    /* Auth has finished and callback has been called */
    AJS_SVC_AUTH_ERROR      = 4     /* An error occurred */
} AuthStatus;

typedef struct {
    uint16_t port;
    uint16_t refCount;
    uint32_t replySerial;
    uint32_t sessionId;
} SessionInfo;

typedef struct {
    char* peer;
    duk_context* ctx;
} PeerInfo;


static AuthStatus GetPeerStatus(duk_context* ctx, const char* peer)
{
    duk_int_t ret;
    AJS_GetGlobalStashObject(ctx, peer);
    duk_get_prop_string(ctx, -1, "authStatus");
    if (!duk_is_number(ctx, -1)) {
        /* No peer found */
        duk_pop_2(ctx);
        return AJS_SVC_AUTH_ERROR;
    }
    ret = duk_get_int(ctx, -1);
    duk_pop_2(ctx);
    return (AuthStatus)ret;
}

/*
 * Set the authStatus on a peer. If no peer object exists, one will be created.
 */
static void SetPeerStatus(duk_context* ctx, const char* peer, AuthStatus status)
{
    AJS_GetGlobalStashObject(ctx, peer);
    duk_get_prop_string(ctx, -1, "name");
    if (!duk_is_string(ctx, -1)) {
        /* Peer doesn't exist */
        duk_pop(ctx);
        duk_push_string(ctx, peer);
        duk_put_prop_string(ctx, -2, "name");
        duk_push_int(ctx, (int)status);
        duk_put_prop_string(ctx, -2, "authStatus");
    } else {
        /* Peer exists, update status */
        duk_pop(ctx);
        duk_push_int(ctx, (int)status);
        duk_put_prop_string(ctx, -2, "authStatus");
    }
    duk_pop(ctx);
}

static void AuthCallback(const void* context, AJ_Status status)
{
    PeerInfo* info = (PeerInfo*)context;
    if (info) {
        /*
         * Update the peers status
         */
        SetPeerStatus(info->ctx, info->peer, (status == AJ_OK) ? AJS_SVC_AUTHENTICATED : AJS_SVC_AUTHENTICATING);
        if (status == AJ_OK) {
            /*
             * Auth is complete, info will never be used again
             */
            if (info->peer) {
                AJ_Free(info->peer);
            }
            AJ_Free(info);
        }
    }
}

static int NativeServiceObjectFinalizer(duk_context* ctx)
{
    const char* peer;
    SessionInfo* sessionInfo;

    AJ_InfoPrintf(("ServiceObjectFinalizer\n"));

    duk_get_prop_string(ctx, 0, "dest");
    if (!duk_is_undefined(ctx, -1)) {
        peer = duk_get_string(ctx, -1);
        if (peer) {
            AJS_GetGlobalStashObject(ctx, "sessions");
            duk_get_prop_string(ctx, -1, peer);
            duk_get_prop_string(ctx, -1, "info");
            sessionInfo = duk_get_buffer(ctx, -1, NULL);
            duk_pop_2(ctx);
            AJ_ASSERT(sessionInfo->refCount != 0);
            if ((--sessionInfo->refCount == 0) && sessionInfo->sessionId) {
                duk_del_prop_string(ctx, -1, peer);
                /*
                 * Only leave the session if AllJoyn is still running
                 */
                if (AJS_IsRunning()) {
                    (void) AJ_BusLeaveSession(AJS_GetBusAttachment(), sessionInfo->sessionId);
                }
                sessionInfo->sessionId = 0;
            }
            duk_pop(ctx);
        }
        /*
         * There is no guarantee that finalizers are only called once. This ensures that the
         * finalizer is idempotent.
         */
        duk_del_prop_string(ctx, 0, "dest");
    }
    duk_pop(ctx);
    return 0;
}

/*
 * Check is a peer is still live
 */
static void CheckPeerIsAlive(duk_context* ctx, const char* peer)
{
    AJS_GetGlobalStashObject(ctx, "sessions");
    if (!duk_has_prop_string(ctx, -1, peer)) {
        duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "Peer has disconnected");
    }
    duk_pop(ctx);
}

/*
 * Looks for an interface that defines a specific member.
 *
 * TODO - check for ambiguity - same member defined on multiple interfaces
 */
static const char* FindInterfaceForMember(duk_context* ctx, duk_idx_t mbrIdx, const char** member)
{
    const char* iface = NULL;
    uint8_t found = FALSE;
    size_t numInterfaces;
    duk_idx_t listIdx;

    duk_get_prop_string(ctx, -1, "interfaces");
    numInterfaces = duk_get_length(ctx, -1);
    listIdx = AJS_GetAllJoynProperty(ctx, "interfaceDefinition");

    if (duk_is_object(ctx, mbrIdx) && NumProps(ctx, mbrIdx) > 0) {
        /*
         * Expect an object of form { member:"org.foo.interface" }
         */
        duk_enum(ctx, mbrIdx, DUK_ENUM_OWN_PROPERTIES_ONLY);
        if (!duk_next(ctx, -1, 1)) {
            duk_error(ctx, DUK_ERR_TYPE_ERROR, "Require object of form { 'member-name':'interface-name' }");
        }
        iface = duk_require_string(ctx, -1);
        if (!AJ_StringFindFirstOf(iface, ".") == -1) {
            duk_error(ctx, DUK_ERR_TYPE_ERROR, "Interface name '%s' is not a dotted name", iface);
        }
        *member = duk_require_string(ctx, -2);
        duk_get_prop_string(ctx, listIdx, iface);
        if (duk_is_undefined(ctx, -1)) {
            duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "Unknown interface: '%s'", iface);
        }
        found = duk_has_prop_string(ctx, -1, *member);
        duk_pop_n(ctx, 4);
    } else {
        size_t i;
        /*
         * Expect a string
         */
        *member = duk_require_string(ctx, mbrIdx);
        for (i = 0; !found && (i < numInterfaces); ++i) {
            duk_get_prop_index(ctx, -2, i);
            iface = duk_require_string(ctx, -1);
            duk_get_prop_string(ctx, listIdx, iface);
            /*
             * See if the requested member exists on this interface
             */
            found = duk_has_prop_string(ctx, -1, *member);
            duk_pop_2(ctx);
        }
    }
    duk_pop_2(ctx);
    if (!found) {
        duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "Unknown member: '%s'", *member);
    }
    return iface;
}

/*
 * Called with a service object on the top of the stack. Returns with a message object on top of
 * the stack replacing the service object.
 */
static void MessageSetup(duk_context* ctx, const char* iface, const char* member, const char* path, uint8_t msgType)
{
    AJ_Status status;
    AJ_MsgHeader hdr;
    AJ_Message msg;
    AJS_MsgInfo* msgInfo;
    const char* dest;
    uint8_t secure;
    size_t dlen;
    duk_idx_t objIdx = duk_push_object(ctx);

    /*
     * Get the destination from the service object
     */
    duk_get_prop_string(ctx, -2, "dest");
    dest = duk_get_lstring(ctx, -1, &dlen);
    duk_pop(ctx);
    /*
     * If this is not a broadcast message make sure the destination peer is still connected
     */
    if (dlen) {
        CheckPeerIsAlive(ctx, dest);
    }
    /*
     * Initialize a message struct so we can lookup the message id. We do this now because it is
     * alot more efficient if the method object we are creating is used multiple times.
     */
    memset(&msg, 0, sizeof(AJ_Message));
    memset(&hdr, 0, sizeof(AJ_MsgHeader));
    msg.hdr = &hdr;
    msg.signature = "*";
    msg.member = member;
    msg.iface = iface;
    msg.objPath = path ? path : AJS_GetStringProp(ctx, -2, "path");
    /*
     * This allows us to use one object table entry for all messages
     */
    AJS_SetObjectPath(msg.objPath);
    hdr.msgType = msgType;
    status = AJ_LookupMessageId(&msg, &secure);
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "Unknown %s %s", path ? "SIGNAL" : "METHOD", member);
    }
    /*
     * Buffer to caching message information stored in the "info" property on the method object
     */
    msgInfo = duk_push_fixed_buffer(ctx, sizeof(AJS_MsgInfo) + dlen + 1);
    msgInfo->secure = secure;
    msgInfo->session = AJS_GetIntProp(ctx, -2, "session");
    msgInfo->msgId = msg.msgId;
    memcpy(msgInfo->dest, dest, dlen);
    msgInfo->dest[dlen] = 0;
    duk_put_prop_string(ctx, objIdx, "info");

    AJ_ASSERT(duk_get_top_index(ctx) == objIdx);
    /*
     * Remove sessions object and leave the message object on the top of the stack
     */
    duk_remove(ctx, -2);
}

static int NativeMethod(duk_context* ctx)
{
    const char* member;
    const char* iface;

    duk_push_this(ctx);
    iface = FindInterfaceForMember(ctx, 0, &member);

    if (!iface || !member) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Interface or member are null\n");
        return 0;
    }
    AJ_InfoPrintf(("NativeMethod %s\n", member));

    MessageSetup(ctx, iface, member, NULL, AJ_MSG_METHOD_CALL);
    /*
     * Register function for actually calling this method
     */
    duk_push_c_lightfunc(ctx, AJS_MarshalMethodCall, DUK_VARARGS, 0, 0);
    duk_put_prop_string(ctx, -2, "call");
    /* return the method object we just created */
    return 1;
}

/*
 * Signals require an object path, interface, and member
 */
static void InitSignal(duk_context* ctx, const char* dest, uint32_t session)
{
    const char* path = duk_require_string(ctx, 0);
    const char* iface;
    const char* member;

    /*
     * Build up a dummy service object
     */
    duk_push_object(ctx);
    /*
     * Get the interfaces from the object path
     */
    AJS_GetAllJoynProperty(ctx, "objectDefinition");
    duk_get_prop_string(ctx, -1, path);
    if (duk_is_undefined(ctx, -1)) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Unknown object path '%s'", path);
    }
    duk_get_prop_string(ctx, -1, "interfaces");
    duk_put_prop_string(ctx, -4, "interfaces");
    duk_pop_2(ctx);
    /*
     * Set the endpoint information
     */
    duk_push_string(ctx, dest);
    duk_put_prop_string(ctx, -2, "dest");
    duk_push_number(ctx, session);
    duk_put_prop_string(ctx, -2, "session");
    /*
     * Resolve the interface name
     */
    iface = FindInterfaceForMember(ctx, 1, &member);
    MessageSetup(ctx, iface, member, path, AJ_MSG_SIGNAL);
    duk_dup(ctx, 0);
    duk_put_prop_string(ctx, -2, "path");
    duk_push_c_lightfunc(ctx, AJS_MarshalSignal, DUK_VARARGS, 0, 0);
    duk_put_prop_string(ctx, -2, "send");
}

static int NativeSignal(duk_context* ctx)
{
    const char* dest;
    uint32_t session;

    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "dest");
    dest = duk_get_string(ctx, -1);
    duk_pop(ctx);
    duk_get_prop_string(ctx, -1, "session");
    session = duk_get_int(ctx, -1);
    duk_pop_2(ctx);
    InitSignal(ctx, dest, session);
    return 1;
}

static int NativeBroadcastSignal(duk_context* ctx)
{
    /*
     * Broadcast signals have an empty destination and no session
     */
    InitSignal(ctx, NULL, 0);
    return 1;
}

static int NativeSetProp(duk_context* ctx)
{
    const char* prop;
    const char* iface;

    if (!duk_is_string(ctx, 0)) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "First argument must be the property name");
    }
    duk_push_c_lightfunc(ctx, AJS_MarshalMethodCall, 3, 0, 0);
    prop = duk_get_string(ctx, 0);
    duk_push_this(ctx);
    iface = FindInterfaceForMember(ctx, 0, &prop);
    MessageSetup(ctx, &AJ_PropertiesIface[0][1], "Set", NULL, AJ_MSG_METHOD_CALL);
    duk_push_string(ctx, iface);
    duk_push_string(ctx, prop);
    duk_dup(ctx, 1);
    duk_call_method(ctx, 3);
    return 1;
}

static int NativeGetProp(duk_context* ctx)
{
    const char* prop;
    const char* iface;

    if (!duk_is_string(ctx, 0)) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Lone argument must be the property name");
    }
    duk_push_c_lightfunc(ctx, AJS_MarshalMethodCall, 2, 0, 0);
    prop = duk_get_string(ctx, 0);
    duk_push_this(ctx);
    iface = FindInterfaceForMember(ctx, 0, &prop);
    MessageSetup(ctx, &AJ_PropertiesIface[0][1], "Get", NULL, AJ_MSG_METHOD_CALL);
    duk_push_string(ctx, iface);
    duk_push_string(ctx, prop);
    duk_call_method(ctx, 2);
    return 1;
}

static int NativeGetAllProps(duk_context* ctx)
{
    if (!duk_is_string(ctx, 0)) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "First argument must be the interface name");
    }
    duk_push_c_lightfunc(ctx, AJS_MarshalMethodCall, 1, 0, 0);
    duk_push_this(ctx);
    MessageSetup(ctx, &AJ_PropertiesIface[0][1], "GetAll", NULL, AJ_MSG_METHOD_CALL);
    duk_dup(ctx, 0);
    duk_call_method(ctx, 1);
    return 1;
}

static int NativeEnableSecurity(duk_context* ctx)
{
    const char* peer;
    const char* objPath;
    AJ_Status status = AJ_OK;
    PeerInfo* info = (PeerInfo*)AJ_Malloc(sizeof(PeerInfo));

    AJ_InfoPrintf(("NativeEnableSecurity()\n"));
    if (!info) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Could not allocate PeerInfo, AJ_ERR_RESOURCES");
    }
    if (!duk_is_function(ctx, 0)) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Lone argument must be a function");
    }
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "path");
    objPath = duk_get_string(ctx, -1);
    /*
     * Object path must be set here, before making any method/signal/prop calls
     */
    AJS_AuthRegisterObject(objPath, AJ_PRX_ID_FLAG);
    duk_pop(ctx);

    duk_get_prop_string(ctx, -1, "dest");
    peer = duk_get_string(ctx, -1);
    /*
     * Set the peers status to AJS_SVC_AUTHENTICATING (not authenticated)
     */
    SetPeerStatus(ctx, peer, AJS_SVC_AUTHENTICATING);
    /*
     * Set the callback function for this peer
     */
    AJS_GetGlobalStashObject(ctx, peer);
    duk_dup(ctx, 0);
    duk_put_prop_string(ctx, -2, "callback");
    /*
     * Save the service object by putting it into the peer object
     */
    duk_dup(ctx, -3);
    duk_put_prop_string(ctx, -2, "service");
    /*
     * Save the peer info so AuthCallback can access it
     */
    info->peer = (char*)AJ_Malloc(strlen(peer) + 1);
    if (!info->peer) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Could not allocate peer string, AJ_ERR_RESOURCES");
    }
    memcpy(info->peer, peer, strlen(peer));
    info->peer[strlen(peer)] = '\0';
    info->ctx = ctx;

    AJ_InfoPrintf(("NativeEnableSecurity(): Authenticating peer %s\n", info->peer));
    status = AJ_BusAuthenticatePeer(AJS_GetBusAttachment(), info->peer, AuthCallback, (void*)info);
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "AJ_BusAuthenticatePeer() failed\n");
    }
    duk_pop_3(ctx);

    return 0;
}

static const duk_function_list_entry peer_native_functions[] = {
    { "method",         NativeMethod,           1 },
    { "signal",         NativeSignal,           2 },
    { "getAllProps",    NativeGetAllProps,      1 },
    { "getProp",        NativeGetProp,          1 },
    { "setProp",        NativeSetProp,          2 },
    { "enableSecurity", NativeEnableSecurity,   1 },
    { NULL }
};

static const duk_number_list_entry peer_native_numbers[] = {
    { "session", 0 },
    { NULL }
};

void AJS_RegisterMsgFunctions(AJ_BusAttachment* bus, duk_context* ctx, duk_idx_t ajIdx)
{
    duk_idx_t objIdx;

    duk_push_c_function(ctx, NativeBroadcastSignal, 2);
    duk_put_prop_string(ctx, ajIdx, "signal");

    duk_push_global_stash(ctx);
    /*
     * Push and initialized the peerProto object
     */
    objIdx = duk_push_object(ctx);
    duk_put_number_list(ctx, objIdx, peer_native_numbers);
    AJS_PutFunctionList(ctx, objIdx, peer_native_functions, TRUE);
    /*
     * Finalizer function called when the object is deleted
     */
    AJS_RegisterFinalizer(ctx, objIdx, NativeServiceObjectFinalizer);
    /*
     * Give the object a name
     */
    duk_put_prop_string(ctx, -2, "peerProto");
    duk_pop(ctx);
}

/*
 * If there is a callback registered for this interface pushes the function onto the stack
 */
static int PushServiceCallback(duk_context* ctx, const char* iface)
{
    AJS_GetGlobalStashObject(ctx, "serviceCB");

    duk_get_prop_string(ctx, -1, iface);
    duk_remove(ctx, -2);
    if (duk_is_callable(ctx, -1)) {
        return 1;
    } else {
        duk_pop(ctx);
        return 0;
    }
}

static int HasServiceCallback(duk_context* ctx, const char* iface)
{
    int result;
    AJS_GetGlobalStashObject(ctx, "serviceCB");
    result = duk_has_prop_string(ctx, -1, iface);
    duk_pop(ctx);
    return result;
}

/*
 * Called with "sessions" at the top of the stack.
 *
 * Iterate over the accumulated announcents and make callbacks into JavaScript
 */
static void AnnouncementCallbacks(duk_context* ctx, const char* peer, SessionInfo* sessionInfo)
{
    size_t i;
    size_t numSvcs;
    AJ_ASSERT(sessionInfo->sessionId);
    /*
     * Get the session object (indexed by the peer string) and from this get the announcements array
     */
    duk_get_prop_string(ctx, -1, peer);
    duk_get_prop_string(ctx, -1, "anno");
    /*
     * Iterate over the services implemented by this peer
     */
    numSvcs = duk_get_length(ctx, -1);

    for (i = 0; i < numSvcs; ++i) {
        size_t j;
        size_t numIfaces;
        duk_idx_t svcIdx;

        duk_get_prop_index(ctx, -1, i);
        if (duk_is_undefined(ctx, -1)) {
            /*
             * Undefined means the iface has been deleted so pop "interfaces"
             */
            duk_pop(ctx);
            /*
             * Delete the announcement entry and continue
             */
            duk_del_prop_index(ctx, -1, i);
            continue;
        }
        svcIdx = duk_get_top_index(ctx);
        /*
         * Iterate over the interfaces for this service
         */
        duk_get_prop_string(ctx, svcIdx, "interfaces");
        numIfaces = duk_get_length(ctx, -1);
        for (j = 0; j < numIfaces; ++j) {
            const char* iface;
            duk_get_prop_index(ctx, -1, j);
            iface = duk_get_string(ctx, -1);
            duk_pop(ctx);
            /*
             * If there is a callback registered for this interface call it
             *
             * TODO - should we really make a callback for each interface or just call the first one
             * that has an callback referenced. If the same callback function has been registered
             * for multiple interfaces we definitely don't want to call it multiple times.
             */
            if (PushServiceCallback(ctx, iface)) {
                /*
                 * Set the session information on the service
                 */
                duk_push_int(ctx, sessionInfo->sessionId);
                duk_put_prop_string(ctx, svcIdx, "session");
                duk_push_string(ctx, peer);
                duk_put_prop_string(ctx, svcIdx, "dest");
                /*
                 * Set the peer status for this new peer
                 */
                SetPeerStatus(ctx, peer, AJS_SVC_NO_AUTH);
                /*
                 * Call the callback function
                 */
                duk_dup(ctx, svcIdx);
                if (duk_pcall(ctx, 1) != DUK_EXEC_SUCCESS) {
                    AJS_ConsoleSignalError(ctx);
                }
                duk_pop(ctx);
            }
        }
        /*
         * Pop "interfaces" and the service object
         */
        duk_pop_2(ctx);
        /*
         * Delete the announcement entry
         */
        duk_del_prop_index(ctx, -1, i);
    }
    /* Pop "anno" and the session object */
    duk_pop_2(ctx);
}

/*
 * Called with and array of interfaces at the top of the stack. Replaces the interfaces array with
 * the service object on returning.
 */
static void AddServiceObject(duk_context* ctx, SessionInfo* sessionInfo, const char* path, const char* dest)
{
    /*
     * Create a service object from the prototype and customize it
     */
    AJS_GetGlobalStashObject(ctx, "peerProto");
    AJS_CreateObjectFromPrototype(ctx, -1);
    duk_remove(ctx, -2);
    /*
     * Set properties on the service object
     */
    duk_push_string(ctx, dest);
    duk_put_prop_string(ctx, -2, "dest");
    duk_push_string(ctx, path);
    duk_put_prop_string(ctx, -2, "path");
    duk_swap_top(ctx, -2);
    duk_put_prop_string(ctx, -2, "interfaces");
    /*
     * Increment the peer session refCount. It will be decremented when the service object
     * finalizer is called. If the refCount goes to zero and a session was established when
     * session will be closed.
     */
    ++sessionInfo->refCount;
}

AJ_Status AJS_FoundAdvertisedName(duk_context* ctx, AJ_Message* msg)
{
    AJ_Status status;
    SessionInfo* sessionInfo;
    const char* name;
    const char* prefix;
    uint16_t port;
    uint16_t mask;
    duk_idx_t fnmeIdx;
    duk_idx_t sessIdx;

    AJ_UnmarshalArgs(msg, "sqs", &name, &mask, &prefix);

    AJ_InfoPrintf(("AJS_FoundAdvertisedName %s %s\n", prefix, name));

    AJS_GetGlobalStashObject(ctx, "sessions");
    sessIdx = duk_get_top_index(ctx);
    /*
     * If the name is already listed we can ignore this signal
     */
    if (duk_get_prop_string(ctx, -1, name)) {
        duk_pop_2(ctx);
        return AJ_OK;
    }
    /*
     * Check if there is a callback registered for this name (actually the prefix)
     */
    AJS_GetGlobalStashObject(ctx, "findName");
    duk_get_prop_string(ctx, -1, prefix);
    if (duk_is_undefined(ctx, -1)) {
        duk_pop_3(ctx);
        return AJ_OK;
    }
    fnmeIdx = duk_get_top_index(ctx);
    /*
     * From here on we make it look like we received an announcement
     */
    port = AJS_GetIntProp(ctx, fnmeIdx, "port");
    status = AJ_BusJoinSession(msg->bus, name, port, NULL);
    if (status == AJ_OK) {
        duk_idx_t anIdx;
        duk_idx_t svcIdx;
        /*
         * Create service object and add it to the sessions array
         */
        svcIdx = duk_push_object(ctx);
        /*
         * Set the "info" and "anno" properties on the service object
         */
        sessionInfo = duk_push_fixed_buffer(ctx, sizeof(SessionInfo));
        sessionInfo->port = port;
        duk_put_prop_string(ctx, svcIdx, "info");
        anIdx = duk_push_array(ctx);
        /*
         * Push array of interfaces
         */
        duk_get_prop_string(ctx, fnmeIdx, "interfaces");
        /*
         * Set the callback to be called when the JOIN_SESSION reply is received
         */
        AJS_GetGlobalStashObject(ctx, "serviceCB");
        duk_get_prop_index(ctx, -2, 0);           /* key - first interface in "interfaces" array */
        duk_get_prop_string(ctx, fnmeIdx, "cb");  /* val - callback function */
        duk_put_prop(ctx, -3);
        duk_pop(ctx);
        /* Interfaces array is at the top of the stack */
        AddServiceObject(ctx, sessionInfo, AJS_GetStringProp(ctx, fnmeIdx, "path"), name);
        /*
         * Append service object to the announcements array for processing later
         */
        duk_put_prop_index(ctx, anIdx, duk_get_length(ctx, anIdx));

        AJ_ASSERT(duk_get_top_index(ctx) == anIdx);
        duk_put_prop_string(ctx, svcIdx, "anno");
        AJ_ASSERT(duk_get_top_index(ctx) == svcIdx);
        duk_put_prop_string(ctx, sessIdx, name);
        /*
         * Store the reply serial number for the JOIN_SESSION reply
         */
        sessionInfo->replySerial = msg->bus->serial - 1;
    } else {
        duk_del_prop_string(ctx, sessIdx, prefix);
    }
    /* Clean up the stack */
    duk_pop_n(ctx, 4);
    return status;
}

/*
 * Create a session object if we don't have one for this peer. Leave the session object on the top
 * of the stack when this function returns.
 */
static SessionInfo* AllocSessionObject(duk_context* ctx, const char* name)
{
    SessionInfo* sessionInfo;

    if (duk_get_prop_string(ctx, -1, name)) {
        duk_get_prop_string(ctx, -1, "info");
        sessionInfo = duk_get_buffer(ctx, -1, NULL);
        duk_pop(ctx);
    } else {
        /*
         * Replace whatever we got with the object we need
         */
        duk_pop(ctx);
        duk_push_object(ctx);
        duk_dup(ctx, -1);
        duk_put_prop_string(ctx, -3, name);
        sessionInfo = duk_push_fixed_buffer(ctx, sizeof(SessionInfo));
        duk_put_prop_string(ctx, -2, "info");
        duk_push_array(ctx);
        duk_put_prop_string(ctx, -2, "anno");
    }
    return sessionInfo;
}

/*
 * Announcements are processed as follows:
 *
 * 1) First check if there is an existing session for the announcement sender. If not create and
 *    initialize an empty session object. A session object has two properties "info" which is a buffer
 *    that holds state information about the session and an array "anno" which is used to accumulate
 *    announcements from the same sender.
 * 2) Unmarshal the announcement signal and if there is a callback registered for any of the
 *    interfaces add a service object to announcements array. The "interfaces" property on the
 *    service object is an array of all interfaces in the announcement.
 * 3) If there is already a session with the announcement sender make callbacks now, otherwise send
 *    a JOIN_SESSION method call to join the session with the announcement sender. The callbacks are
 *    reevaluated when the JOIN_SESSION reply is received.
 */
AJ_Status AJS_AboutAnnouncement(duk_context* ctx, AJ_Message* msg)
{
    AJ_BusAttachment* aj = msg->bus;
    AJ_Status status;
    uint16_t version;
    duk_idx_t anIdx;
    SessionInfo* sessionInfo;
    AJ_Arg objList;
    const char* sender;

    AJ_InfoPrintf(("AJS_AboutAnnouncement\n"));

    /*
     * Ignore our own announcements
     */
    if (strcmp(msg->sender, AJ_GetUniqueName(msg->bus)) == 0) {
        AJ_InfoPrintf(("Ignoring our own announcement\n"));
        return AJ_OK;
    }
    /*
     * Push the sender string on the stack to stabilize it.
     */
    sender = duk_push_string(ctx, msg->sender);
    /*
     * Announcements are accumulated in a global stash object indexed by peer name
     */
    AJS_GetGlobalStashObject(ctx, "sessions");
    sessionInfo = AllocSessionObject(ctx, sender);
    /*
     * Get the announcements array
     */
    duk_get_prop_string(ctx, -1, "anno");
    anIdx = duk_get_top_index(ctx);
    /*
     * Find out if there are JavaScript handlers for this announcement
     */
    AJ_UnmarshalArgs(msg, "qq", &version, &sessionInfo->port);
    status = AJ_UnmarshalContainer(msg, &objList, AJ_ARG_ARRAY);
    while (status == AJ_OK) {
        int ifcCount = 0;
        uint8_t hasCB = FALSE;
        AJ_Arg obj;
        AJ_Arg interfaces;
        const char* path;

        status = AJ_UnmarshalContainer(msg, &obj, AJ_ARG_STRUCT);
        if (status != AJ_OK) {
            break;
        }
        AJ_UnmarshalArgs(msg, "o", &path);
        /*
         * Array to accumulate interfaces
         */
        duk_push_array(ctx);
        status = AJ_UnmarshalContainer(msg, &interfaces, AJ_ARG_ARRAY);
        while (status == AJ_OK) {
            /*
             * Accumulate interfaces checking
             */
            const char* iface;
            status = AJ_UnmarshalArgs(msg, "s", &iface);
            if (status == AJ_OK) {
                duk_push_string(ctx, iface);
                duk_put_prop_index(ctx, -2, ifcCount++);
            }
            /*
             * We will disard the interface array if no callbacks are registered.
             */
            if (!hasCB && HasServiceCallback(ctx, iface)) {
                hasCB = TRUE;
            }
        }
        if (hasCB) {
            AddServiceObject(ctx, sessionInfo, path, sender);
            /*
             * Append service object to the announcements array for processing later
             */
            duk_put_prop_index(ctx, anIdx, duk_get_length(ctx, anIdx));
        } else {
            /* discard the interface array */
            duk_pop(ctx);
        }
        if (status == AJ_ERR_NO_MORE) {
            status = AJ_UnmarshalCloseContainer(msg, &interfaces);
        }
        if (status == AJ_OK) {
            status = AJ_UnmarshalCloseContainer(msg, &obj);
        }
    }
    if (status == AJ_ERR_NO_MORE) {
        status = AJ_UnmarshalCloseContainer(msg, &objList);
    }
    /*
     * All done with this message - we need to close it now to free up the output buffer before we
     * attempt to call AJ_BusJoinSession() below.
     */
    AJ_CloseMsg(msg);

    /* Pop "anno" and the session object */
    duk_pop_2(ctx);

    if (status != AJ_OK) {
        goto Exit;
    }
    /*
     * If we already have a session with this peer callback with the new service objects
     */
    if (sessionInfo->sessionId) {
        AnnouncementCallbacks(ctx, sender, sessionInfo);
        goto Exit;
    }
    /*
     * If there is a JOIN_SESSION in progress we have nothing more to do
     */
    if (sessionInfo->replySerial != 0) {
        goto Exit;
    }
    /*
     * If announcements were registered attempt to join the session
     */
    if (sessionInfo->refCount) {
        status = AJ_BusJoinSession(aj, sender, sessionInfo->port, NULL);
        if (status == AJ_OK) {
            sessionInfo->replySerial = aj->serial - 1;
        }
    }
    /*
     * At this point if we don't have a JOIN_SESSION in progress we must delete the session object
     */
    if (sessionInfo->replySerial == 0) {
        duk_del_prop_string(ctx, -2, sender);
    }

Exit:

    /* Pop "sessions" and sender string */
    duk_pop_2(ctx);
    return status;
}

AJ_Status AJS_ServiceSessions(duk_context* ctx)
{
    /*
     * Loop through pending sessions and determine if the service callback should be called
     */
    AuthStatus peerStatus;
    const char* peer = NULL;
    SessionInfo* sessionInfo = NULL;
    uint8_t call = FALSE;

    AJ_InfoPrintf(("AJS_ServiceSessions()\n"));

    AJS_GetGlobalStashObject(ctx, "sessions");

    duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
    while (duk_next(ctx, -1, 1)) {
        peer = duk_get_string(ctx, -2);
        AJ_ASSERT(duk_is_object(ctx, -1));
        duk_pop_2(ctx);
        peerStatus = GetPeerStatus(ctx, peer);
        if (peerStatus == AJS_SVC_AUTHENTICATED) {
            AJ_InfoPrintf(("AJS_ServiceSessions(): Peer %s status = %u\n", peer, peerStatus));
            call = TRUE;
            break;
        }
    }
    duk_pop(ctx); /* Pop the enum */
    if (call) {
        AJS_GetGlobalStashObject(ctx, peer);
        duk_get_prop_string(ctx, -1, "callback");
        duk_get_prop_string(ctx, -2, "service");
        duk_push_int(ctx, TRUE);
        /*
         * Set status to AJS_SVC_AUTH_DONE to not continually call the service callback
         */
        SetPeerStatus(ctx, peer, AJS_SVC_AUTH_DONE);
        AJ_InfoPrintf(("AJS_ServiceSessions(): Authenticated, calling service callback\n"));
        AJS_DumpStack(ctx);
        if (duk_pcall(ctx, 2) != DUK_EXEC_SUCCESS) {
            AJS_ConsoleSignalError(ctx);
        }
        duk_pop_2(ctx);
#ifndef NDEBUG
    } else if (peerStatus == AJS_SVC_AUTHENTICATING) {
        AJ_InfoPrintf(("AJS_ServiceSessions(): Authenticating peer %s\n", peer));
    }
#else
    }
#endif
    /* Pop "sessions" */
    duk_pop(ctx);

    return AJ_OK;
}

AJ_Status AJS_HandleJoinSessionReply(duk_context* ctx, AJ_Message* msg)
{
    const char* peer = NULL;
    SessionInfo* sessionInfo = NULL;
    uint32_t replySerial = msg->replySerial;
    uint8_t joined = FALSE;

    AJS_GetGlobalStashObject(ctx, "sessions");
    duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
    while (duk_next(ctx, -1, 1)) {
        peer = duk_get_string(ctx, -2);
        AJ_ASSERT(duk_is_object(ctx, -1));
        duk_get_prop_string(ctx, -1, "info");
        sessionInfo = duk_get_buffer(ctx, -1, NULL);
        AJ_ASSERT(sessionInfo);
        duk_pop_3(ctx);
        if (sessionInfo->replySerial == replySerial) {
            uint32_t sessionId;
            uint32_t replyStatus;
            /*
             * Check if the join was successful
             */
            AJ_UnmarshalArgs(msg, "uu", &replyStatus, &sessionId);
            if (replyStatus == AJ_JOINSESSION_REPLY_SUCCESS) {
                /*
                 * TODO - if we have a well-known name send a ping to get the unique name
                 */
                sessionInfo->sessionId = sessionId;
                joined = TRUE;
            }
            sessionInfo->replySerial = 0;
            break;
        }
    }
    duk_pop(ctx); /* Pop the enum */
    if (joined) {
        /*
         * TODO - we may need to initiate authentication with the remote peer
         */
        AnnouncementCallbacks(ctx, peer, sessionInfo);
    }
    /* Pop "sessions" */
    duk_pop(ctx);
    return AJ_OK;
}

/*
 * Delete session info object pointed to by sessionId. If sessionId
 * is zero then delete all session info objects.
 */
static AJ_Status RemoveSessions(duk_context* ctx, uint32_t sessionId)
{
    AJ_Status status = AJ_OK;
    AJS_GetGlobalStashObject(ctx, "sessions");
    duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
    while (duk_next(ctx, -1, 1)) {
        SessionInfo* sessionInfo;
        const char* peer = duk_get_string(ctx, -2);
        duk_get_prop_string(ctx, -1, "info");
        sessionInfo = duk_get_buffer(ctx, -1, NULL);
        duk_pop_3(ctx);
        if (sessionId == 0) {
            AJ_InfoPrintf(("RemoveSessions(): Leaving session: %u\n", sessionInfo->sessionId));
            status = AJ_BusLeaveSession(AJS_GetBusAttachment(), sessionInfo->sessionId);
        } else if (sessionInfo->sessionId == sessionId) {
            status = AJ_BusLeaveSession(AJS_GetBusAttachment(), sessionInfo->sessionId);
            duk_del_prop_string(ctx, -2, peer);
            break;
        }
    }
    duk_pop_2(ctx);
    /*
     * TODO - this is not all that useful because it only indicates that a peer has gone away
     * without being able to specify exactly which services are affected. The problem is we cannot
     * hold a reference to the service object because we a relying on the service object finalizer
     * to clean up sessions that are no longer in use. If we hold a reference the finalizer will
     * never get called.
     */
    if (sessionId != 0) {
        AJS_GetAllJoynProperty(ctx, "onPeerDisconnected");
        if (duk_is_callable(ctx, -1)) {
            if (duk_pcall(ctx, 0) != DUK_EXEC_SUCCESS) {
                AJS_ConsoleSignalError(ctx);
            }
        }
        duk_pop(ctx);
    } else {
        AJS_ClearGlobalStashObject(ctx, "sessions");
    }
    return status;
}

AJ_Status AJS_EndSessions(duk_context* ctx)
{
    return RemoveSessions(ctx, 0);
}

AJ_Status AJS_SessionLost(duk_context* ctx, AJ_Message* msg)
{
    uint32_t sessionId;
    AJ_UnmarshalArgs(msg, "u", &sessionId);

    AJ_InfoPrintf(("AJS_SessionLost(): Removing session: %u\n", sessionId));
    return RemoveSessions(ctx, sessionId);
}

static int NativeAuthenticatePeer(duk_context* ctx)
{
    const char* path;
    AJ_InfoPrintf(("NativeAuthenticatePeer\n"));
    if (!duk_is_string(ctx, 0)) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Lone argument must be an object path (string)");
    }
    AJS_AuthRegisterObject(duk_require_string(ctx, 0), AJ_APP_ID_FLAG);
    duk_pop(ctx);
    return 0;
}

AJ_Status AJS_HandleAcceptSession(duk_context* ctx, AJ_Message* msg, uint16_t port, uint32_t sessionId, const char* joiner)
{
    uint32_t accept = TRUE;
    SessionInfo* sessionInfo;

    /*
     * Create an entry in the sessions table so we can track this peer
     */
    AJS_GetGlobalStashObject(ctx, "sessions");
    sessionInfo = AllocSessionObject(ctx, joiner);
    /*
     * If there is no handler automatically accept the connection
     */
    AJS_GetAllJoynProperty(ctx, "onPeerConnected");
    if (duk_is_callable(ctx, -1)) {
        /* Empty interface array */
        duk_push_array(ctx);
        AddServiceObject(ctx, sessionInfo, "/", joiner);
#if !defined(AJS_CONSOLE_LOCKDOWN)
        if (AJS_DebuggerIsAttached()) {
            msg = AJS_CloneAndCloseMessage(ctx, msg);
        }
#endif
        duk_push_c_function(ctx, NativeAuthenticatePeer, 1);
        duk_put_prop_string(ctx, -2, "authenticate");
        if (duk_pcall(ctx, 1) != DUK_EXEC_SUCCESS) {
            AJS_ConsoleSignalError(ctx);
            accept = FALSE;
        } else {
            accept = duk_get_boolean(ctx, -1);
        }
    }
    duk_pop_2(ctx);
    /*
     * It is possible that we already have an outbound session to this peer so if we are not
     * accepting the session we can only delete the entry if the refCount is zero.
     */
    if (accept) {
        ++sessionInfo->refCount;
        sessionInfo->port = port;
        sessionInfo->sessionId = sessionId;
    } else if (sessionInfo->refCount == 0) {
        duk_del_prop_string(ctx, -1, joiner);
    }
    /* Pop sessions object */
    duk_pop(ctx);
    return AJ_BusReplyAcceptSession(msg, accept);
}