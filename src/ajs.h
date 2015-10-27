#ifndef _AJS_H
#define _AJS_H
/**
 * @file
 */
/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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

/**
 * Per-module definition of the current module for debug logging.  Must be defined
 * prior to first inclusion of aj_debug.h
 */
#ifndef AJ_MODULE
#define AJ_MODULE AJS
#endif

#include <ajtcl/aj_target.h>
#include <ajtcl/aj_nvram.h>
#include <ajtcl/alljoyn.h>
#include <duktape.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Controls debug output for this module
 */
#ifndef NDEBUG
extern uint8_t dbgAJS;
#endif

#if DUK_VERSION < 10099
#define duk_push_c_lightfunc(c, f, a, x, y) duk_push_c_function(c, f, a)
#endif

/*
 * These correspond the values in the AJ object - see ajs.c
 */
#define AJS_MEMBER_TYPE_METHOD    0
#define AJS_MEMBER_TYPE_SIGNAL    1
#define AJS_MEMBER_TYPE_PROPERTY  2

/*
 * NVRAM identifiers used by AllJoyn.js
 */
#define AJS_SCRIPT_NAME_NVRAM_ID  (AJ_NVRAM_ID_APPS_BEGIN + 0)
#define AJS_SCRIPT_NVRAM_ID       (AJ_NVRAM_ID_APPS_BEGIN + 1)
#define AJS_SCRIPT_SIZE_ID        (AJ_NVRAM_ID_APPS_BEGIN + 2)
#define AJS_LOCKDOWN_NVRAM_ID     (AJ_NVRAM_ID_APPS_BEGIN + 3)
#define AJS_PROPSTORE_NVRAM_ID    (AJ_NVRAM_ID_APPS_BEGIN + 32)
#define AJS_PROPSTORE_NVRAM_MIN   (AJS_PROPSTORE_NVRAM_ID + 1)
#define AJS_PROPSTORE_NVRAM_MAX   (AJS_PROPSTORE_NVRAM_MIN + 256)

#define AJS_CONSOLE_UNLOCKED    0
#define AJS_CONSOLE_LOCKED      1
#define AJS_CONSOLE_LOCK_ERR    2

/*
 * Default TTL for sessionless signals and notifications
 */
#define AJS_DEFAULT_SLS_TTL       30

/**
 * The internal name for looking up the AllJoyn object
 */
extern const char* AJS_AJObjectName;

/**
 * The internal name for looking up the IO object
 */
extern const char* AJS_IOObjectName;

/*
 * Flags that a message call is not a property accessor (Get/Set/GetAll)
 */
#define AJS_NOT_ACCESSOR 0xFF

/**
 * Private information stuffed into an unmarshalled message.
 */
typedef struct {
    uint8_t flags;
    uint8_t accessor;  /* AJS_NOT_ACCESSOR or AJ_PROP_GET, AJ_PROP_SET, AJ_PROP_GET_ALL */
    uint32_t msgId;
    uint32_t serialNum;
    uint32_t sessionId;
} AJS_ReplyInternal;


/**
 * Type for compactly holding message information
 */
typedef struct {
    int32_t msgId;
    int32_t session;
    uint8_t secure;
    char dest[1];
} AJS_MsgInfo;

/**
 * Is the AllJoyn bus up and running and ready to send/receiver messages.
 */
uint8_t AJS_IsRunning();

/**
 * Get the current lockdown bit from NVRAM.
 *
 * @param state[out]    Current lockdown state
 * @return              AJ_OK upon success
 */
AJ_Status AJS_GetLockdownState(uint8_t* state);

/**
 * Update the lockdown bit to a new value.
 *
 * @param bit       New lockdown setting
 * @return          AJ_OK if bit was set successfully
 */
AJ_Status AJS_SetLockdownState(uint8_t state);

/**
 * Lock out the console and debugger forever
 *
 * @param msg       Lockdown message
 *
 * @return          AJ_OK if successful
 */
AJ_Status AJS_LockConsole(AJ_Message* msg);

/**
 * Gets the size of the script currently in NVRAM
 *
 * @return      Script size
 */
uint32_t AJS_GetScriptSize(void);

/**
 * Get the console bus name once a connection is made
 *
 * @return      Name of the console bus
 */
char* AJS_GetConsoleBusName(void);

/**
 * Get the console's session ID
 *
 * @return      Session ID
 */
uint32_t AJS_GetConsoleSession(void);

/**
 * Get a pointer to the bus attachment
 */
AJ_BusAttachment* AJS_GetBusAttachment();

/**
 * Evaluate an eval message. This must be public to be call-able
 * from the debugger read callback as well as from the console.
 *
 * @param ctx       Duktape context
 * @param msg       Eval message from console
 */
AJ_Status AJS_Eval(duk_context* ctx, AJ_Message* msg);

/**
 * Attach to an AllJoyn routing node. This may trigger on-boarding.
 *
 * @param aj  Pointer to a bus attachment
 */
AJ_Status AJS_AttachAllJoyn(AJ_BusAttachment* aj);

/**
 * Detach from an AllJoyn routing node.
 *
 * @param aj      Pointer to a bus attachment
 * @param status  Status code indicating the reason for detaching
 */
AJ_Status AJS_DetachAllJoyn(AJ_BusAttachment* aj, AJ_Status reason);

/**
 * Initialize object and interface tables from the JavaScript
 *
 * @param ctx    An opaque pointer to a duktape context structure
 * @param ajIdx  Index on the duktape stack for the AllJoyn object. If 0 means the AllJoyn object
 *               was not loaded.
 */
AJ_Status AJS_InitTables(duk_context* ctx, duk_idx_t ajIdx);

/**
 * Reset the object and interface tables
 *
 * @param ctx        An opaque pointer to a duktape context structure
 */
void AJS_ResetTables(duk_context* ctx);

/**
 * Start processing messages until an error occurs.
 *
 * @param ctx    An opaque pointer to a duktape context structure
 * @param bus    Pointer to a bus attachment to receive messages on
 * @param ajIdx  Index on the duktape stack for the AllJoyn object. If 0 means the AllJoyn object
 *               was not loaded.
 *
 * @return   Always returns an error status code.
 */
AJ_Status AJS_MessageLoop(duk_context* ctx, AJ_BusAttachment* bus, duk_idx_t ajIdx);

/**
 * Entry point for AllJoyn
 *
 * @param deviceName  An optional device name that some platforms are able to provide a launch time.
 *
 * @return   - AJ_OK if everything is ok
 */
AJ_Status AJS_Main(const char* deviceName);

/**
 * Start running the scripting environment
 *
 * @param bus  Pointer to a bus attachment
 * @param ctx  An opaque pointer to a duktape context structure
 *
 * @return   - AJ_OK if the JavaScript was compiled and ran
 */
AJ_Status AJS_Run(AJ_BusAttachment* bus, duk_context* ctx);

/**
 * Register native 'C' functions so they can be called from JavaScript
 *
 * @param bus     Pointer to a bus attachment to receive messages on
 * @param ctx     An opaque pointer to a duktape context structure
 * @param ajIdx Index in the duktape value stack for the AJ JavaScript object
 */
AJ_Status AJS_RegisterHandlers(AJ_BusAttachment* bus, duk_context* ctx, duk_idx_t ajIdx);

/**
 * Registration and initialization for notification support
 *
 * @param bus     Pointer to a bus attachment to receive messages on
 * @param ctx     An opaque pointer to a duktape context structure
 * @param ajIdx   Index in the duktape value stack for the AJ JavaScript object
 */
AJ_Status AJS_RegisterNotifHandlers(AJ_BusAttachment* bus, duk_context* ctx, duk_idx_t ajIdx);

/**
 * Registration and initialization for control panel support
 *
 * @param bus     Pointer to a bus attachment to receive messages on
 * @param ctx     An opaque pointer to a duktape context structure
 * @param ajIdx   Index in the duktape value stack for the AJ JavaScript object
 */
AJ_Status AJS_RegisterControlPanelHandlers(AJ_BusAttachment* bus, duk_context* ctx, duk_idx_t ajIdx);

/**
 * Register native 'C' function for signals and method calls
 *
 * @param bus     Pointer to a bus attachment to receive messages on
 * @param ctx     An opaque pointer to a duktape context structure
 * @param ajIdx   Index in the duktape value stack for the AJ JavaScript object
 */
void AJS_RegisterMsgFunctions(AJ_BusAttachment* bus, duk_context* ctx, duk_idx_t ajIdx);

/**
 * Register support for translation
 *
 * @param ctx     An opaque pointer to a duktape context structure
 * @param ajIdx   Index in the duktape value stack for the AJ JavaScript object
 */
void AJS_RegisterTranslations(duk_context* ctx, duk_idx_t ajIdx);

/**
 * Get the current language.
 *
 * @return The index for the current language
 */
uint8_t AJS_GetCurrentLanguage();

/**
 * Get the current language name
 *
 * @return The current language name string
 */
const char* AJS_GetCurrentLanguageName();

/**
 * Function registered on method call for sending a reply
 *
 * @param ctx     An opaque pointer to a duktape context structure
 */
int AJS_MethodCallReply(duk_context* ctx);

/**
 * Function registered on method call for sending a error reply
 *
 * @param ctx     An opaque pointer to a duktape context structure
 */
int AJS_MethodCallError(duk_context* ctx);

/**
 * Register native 'C' functions for target-specific I/O
 *
 * @param ctx     An opaque pointer to a duktape context structure
 * @param ioIdx   Index for the IO object
 */
AJ_Status AJS_RegisterIO(duk_context* ctx, duk_idx_t ioIdx);

/**
 * Called to check if there are any asynchronous I/O events to service
 *
 * @param ctx   An opaque pointer to a duktape context structure
 */
AJ_Status AJS_ServiceIO(duk_context* ctx);

/**
 * Load any target specific modules (from require())
 *
 * @param ctx   An opaque pointer to a duktape context structure
 * @param idx   Index of the target specific object
 * @param id    Name of the target specific module
 */
AJ_Status AJS_TargetModuleLoad(duk_context* ctx, duk_idx_t idx, const char* id);

/*
 * Check to see if any external modules have asynchronous events (platform specific)
 *
 * @param ctx   An opaque pointer to a duktape context structure
 *
 * @return      AJ_OK on success
 */
AJ_Status AJS_ServiceExtModules(duk_context* ctx);

/**
 * Register native 'C' functions for the setInterval/setTimeout functions
 *
 * @param ctx     An opaque pointer to a duktape context structure
 */
AJ_Status AJS_RegisterTimerFuncs(duk_context* ctx);

/**
 * Process any timer callbacks and return the number of milliseconds until the deadline.
 *
 * @param ctx       An opaque pointer to a duktape context structure
 * @param clock     Tracks the elapsed time between calls to this function
 * @param deadline  Returns the deadline for calling this function again.
 */
AJ_Status AJS_RunTimers(duk_context* ctx, AJ_Time* clock, uint32_t* deadline);

#define AJS_APP_PORT            2

/**
 * Handle messages sent to the ScriptConsole object.
 *
 * @param ctx  An opaque pointer to a duktape context structure
 * @param msg  The message to handle
 *
 * @return - AJ_OK if the message was handled
 *         - Otherwise and error status
 */
AJ_Status AJS_ConsoleMsgHandler(duk_context* ctx, AJ_Message* msg);

/**
 * Report an error to the console there is one attached. It is expected that there is an error
 * string on the duktape stack.
 *
 * @param ctx  An opaque pointer to a duktape context structure
 */
#if !defined(AJS_CONSOLE_LOCKDOWN)
void AJS_ConsoleSignalError(duk_context* ctx);
#else
#define AJS_ConsoleSignalError(ctx) do { } while (0)
#endif

/**
 * Initialize the console service
 *
 * @param aj     Pointer to a bus attachment
 */
AJ_Status AJS_ConsoleInit(AJ_BusAttachment* aj);

/**
 * Shutdown the console service
 */
void AJS_ConsoleTerminate();

/**
 * Services any pending sessions to join (when authentication is pending)
 */
AJ_Status AJS_ServiceSessions(duk_context* ctx);

/**
 * Set the object list for About
 *
 * @param objList  The list of objects to announce
 */
AJ_Status AJS_SetAnnounceObjects(const AJ_Object* objList);

/**
 * Set the object path in the object table. This allows a single entry to in the object table to be
 * used for all method calls and signals.
 *
 * @param path  The object path to send.
 */
void AJS_SetObjectPath(const char* path);

/*
 * Same functionality as AJS_SetObjectPath() except the object table is registered by
 * the authenticatio code.
 *
 * @param path  The object path to register
 * @param index The object index
 */
void AJS_AuthRegisterObject(const char* path, uint8_t index);

/**
 * Handle an incoming About announcement signal
 *
 * @param ctx  An opaque pointer to a duktape context structure
 * @param msg  The About announcement signal
 */
AJ_Status AJS_AboutAnnouncement(duk_context* ctx, AJ_Message* msg);

/**
 * Handle an incoming FoundAdvertisedName signal
 *
 * @param ctx  An opaque pointer to a duktape context structure
 * @param msg  The FoundAdvertisedName signal
 */
AJ_Status AJS_FoundAdvertisedName(duk_context* ctx, AJ_Message* msg);

/**
 * Handle a response to a JOIN_SESSION for an internally generated JOIN_SESSION method call
 *
 * @param ctx  An opaque pointer to a duktape context structure
 * @param msg  The JOIN_SESSION response
 */
AJ_Status AJS_HandleJoinSessionReply(duk_context* ctx, AJ_Message* msg);

/**
 * Handle an ACCEPT_SESSION call
 *
 * @param ctx       An opaque pointer to a duktape context structure
 * @param msg       The ACCEPT_SESSION method call
 * @param port      The port
 * @param sessionId The sessionId for the new session
 * @param joiner    The requesting peer
 */
AJ_Status AJS_HandleAcceptSession(duk_context* ctx, AJ_Message* msg, uint16_t port, uint32_t sessionId, const char* joiner);

/**
 * Handle a SESSION_LOST signal
 *
 * @param ctx  An opaque pointer to a duktape context structure
 * @param msg  The SESSION_LOST signal
 */
AJ_Status AJS_SessionLost(duk_context* ctx, AJ_Message* msg);

/**
 * End all application sessions that are currently active
 *
 * @param ctx   An opaque pointer to a duktape context structure
 */
AJ_Status AJS_EndSessions(duk_context* ctx);

/**
 * Native function called from JavaScript to marshal and send a method call message
 *
 * @param ctx  An opaque pointer to a duktape context structure
 */
int AJS_MarshalMethodCall(duk_context* ctx);

/**
 * Native function called from JavaScript to marshal and send a signal message
 *
 * @param ctx  An opaque pointer to a duktape context structure
 */
int AJS_MarshalSignal(duk_context* ctx);

/**
 * Unmarshals a message from C to JavaScript pushing the resultant JavaScript object onto the
 * duktape stack. Returns the index of the message object.
 *
 * @param ctx          An opaque pointer to a duktape context structure
 * @param msg          The message to unmarshal
 * @param accessor     Indicates if the message is an accessor (PropGet/PropSet/PropGetAll method call)
 *
 * @return   Returns the absolute index on the duktape stack for the pushed object.
 */
duk_idx_t AJS_UnmarshalMessage(duk_context* ctx, AJ_Message* msg, uint8_t accessor);

/**
 * Unmarshals message arguments from C to JavaScript pushing the resultant JavaScript objects onto
 * the duktape stack.
 *
 * @param ctx  An opaque pointer to a duktape context structure
 * @param msg  The message to unmarshal
 *
 * @return   Returns AJ_OK if the unmarshaling was succesful
 */
AJ_Status AJS_UnmarshalMsgArgs(duk_context* ctx, AJ_Message* msg);

/**
 * Unmarshals message arguments for a property accessor call from C to JavaScript pushing the
 * resultant JavaScript objects onto the duktape stack.
 *
 * @param ctx  An opaque pointer to a duktape context structure
 * @param msg  The property accessor message to unmarshal
 *
 * @return   Returns AJ_OK if the unmarshaling was succesful
 */
AJ_Status AJS_UnmarshalPropArgs(duk_context* ctx, AJ_Message* msg, uint8_t accessor, duk_idx_t msgIdx);

/*
 * Access permissions on properties
 */
#define AJS_PROP_ACCESS_R      '>'
#define AJS_PROP_ACCESS_W      '<'
#define AJS_PROP_ACCESS_RW     '='

/**
 * Gets the access permission defined on a property member.
 *
 * @param ctx  An opaque pointer to a duktape context structure
 * @param idx  Index for the property object
 */
char AJS_GetPropMemberAccess(duk_context* ctx, duk_idx_t idx);

/**
 * This function is used to by all native functions that make a method call to push a reply object
 * that can be used to register a callback function to be called when a method reply is received.
 *
 * @param ctx         An opaque pointer to a duktape context structure
 * @param replySerial The method call serial number that will be used to match up the reply
 */
void AJS_PushReplyObject(duk_context* ctx, uint32_t replySerial);

/**
 * @param ctx    An opaque pointer to a duktape context structure
 * @param alert  Set this to 1 for an alert, 0 for print. Print and alert correspond roughly to
 *               stdout and stderr respectively.
 *
 */
void AJS_AlertHandler(duk_context* ctx, uint8_t alert);

/**
 * Signal the console (or print locally) when a throw is called from JS
 *
 * @param ctx   An opaque pointer to a duktape context structure
 */
void AJS_ThrowHandler(duk_context* ctx);

/**
 * @param ctx         An opaque pointer to a duktape context structure
 * @param deviceName  If not NULL the device name to set in the property store.
 */
AJ_Status AJS_PropertyStoreInit(duk_context* ctx, const char* deviceName);

/**
 * Reset the property store to defaults and offboard the device
 */
AJ_Status AJS_FactoryReset();

typedef enum {
    AJS_OP_NONE          = 0,
    AJS_OP_OFFBOARD      = 1,
    AJS_OP_FACTORY_RESET = 2
} AJS_DEFERRED_OP;

/**
 * Set up for an operation that can only be performed at the msg loop level.
 */
void AJS_DeferredOperation(duk_context* ctx, AJS_DEFERRED_OP op);

#ifdef __cplusplus
}
#endif
#endif
