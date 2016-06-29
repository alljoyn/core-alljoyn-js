/**
 * @file
 */
/******************************************************************************
 * Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
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
#ifndef _AJS_CONSOLE_H

#include "ajs_console_common.h"
#include <signal.h>
#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/String.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/version.h>
#include <alljoyn/AboutListener.h>

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN"

/*
 * Type of debug messages
 */
typedef enum {
    STATUS_NOTIFICATION     = 0x01,
    PRINT_NOTIFICATION      = 0x02,
    ALERT_NOTIFICATION      = 0x03,
    LOG_NOTIFICATION        = 0x04,
    BASIC_INFO_REQ          = 0x10,
    TRIGGER_STATUS_REQ      = 0x11,
    PAUSE_REQ               = 0x12,
    RESUME_REQ              = 0x13,
    STEP_INTO_REQ           = 0x14,
    STEP_OVER_REQ           = 0x15,
    STEP_OUT_REQ            = 0x16,
    LIST_BREAK_REQ          = 0x17,
    ADD_BREAK_REQ           = 0x18,
    DEL_BREAK_REQ           = 0x19,
    GET_VAR_REQ             = 0x1a,
    PUT_VAR_REQ             = 0x1b,
    GET_CALL_STACK_REQ      = 0x1c,
    GET_LOCALS_REQ          = 0x1d,
    EVAL_REQ                = 0x1e,
    DETACH_REQ              = 0x1f,
    DUMP_HEAP_REQ           = 0x20
} DEBUG_REQUESTS;

/*
 * Elements of debug messages
 */
typedef enum {
    DBG_TYPE_EOM        = 0x00,
    DBG_TYPE_REQ        = 0x01,
    DBG_TYPE_REP        = 0x02,
    DBG_TYPE_ERR        = 0x03,
    DBG_TYPE_NFY        = 0x04,
    DBG_TYPE_INTEGER4   = 0x10,
    DBG_TYPE_STRING4    = 0x11,
    DBG_TYPE_STRING2    = 0x12,
    DBG_TYPE_BUFFER4    = 0x13,
    DBG_TYPE_BUFFER2    = 0x14,
    DBG_TYPE_UNUSED     = 0x15,
    DBG_TYPE_UNDEFINED  = 0x16,
    DBG_TYPE_NULL       = 0x17,
    DBG_TYPE_TRUE       = 0x18,
    DBG_TYPE_FALSE      = 0x19,
    DBG_TYPE_NUMBER     = 0x1a,
    DBG_TYPE_OBJECT     = 0x1b,
    DBG_TYPE_POINTER    = 0x1c,
    DBG_TYPE_LIGHTFUNC  = 0x1d,
    DBG_TYPE_HEAPPTR    = 0x1e,
    DBG_TYPE_STRLOW     = 0x60,
    DBG_TYPE_STRHIGH    = 0x7f,
    DBG_TYPE_INTSMLOW   = 0x80,
    DBG_TYPE_INTSMHIGH  = 0xbf,
    DBG_TYPE_INTLGLOW   = 0xc0,
    DBG_TYPE_INTLGHIGH  = 0xff
} DEBUG_TYPES;

class AJS_Console : public ajn::BusListener, public ajn::SessionListener, public ajn::AboutListener, public ajn::MessageReceiver, public ajn::BusAttachment::JoinSessionAsyncCB {
  public:

    AJS_Console();

    virtual ~AJS_Console();

    QStatus Connect(const char* deviceName, volatile sig_atomic_t* interrupt);

    int8_t Eval(const qcc::String script);

    QStatus Reboot();

    QStatus Install(qcc::String name, const uint8_t* script, size_t len);

    int8_t LockdownConsole(void);

    void SessionLost(ajn::SessionId sessionId, SessionLostReason reason);

    virtual void BusDisconnected();

    virtual void BusStopping();

    virtual void Notification(const ajn::InterfaceDescription::Member* member, const char* sourcePath, ajn::Message& msg);

    virtual void Print(const char* fmt, ...);

    virtual void PrintMsg(const ajn::InterfaceDescription::Member* member, const char* sourcePath, ajn::Message& msg);

    virtual void AlertMsg(const ajn::InterfaceDescription::Member* member, const char* sourcePath, ajn::Message& msg);

    virtual void ThrowMsg(const ajn::InterfaceDescription::Member* member, const char* sourcePath, ajn::Message& msg);

    virtual void DebugNotification(const ajn::InterfaceDescription::Member* member, const char* sourcePath, ajn::Message& msg);

    virtual void EvalResult(const ajn::InterfaceDescription::Member* member, const char* sourcePath, ajn::Message& msg);

    void Announced(const char* busName, uint16_t version, ajn::SessionPort port, const ajn::MsgArg& objectDescriptionArg, const ajn::MsgArg& aboutDataArg);

    /**
     * Start the debugger
     */
    void StartDebugger();

    /**
     * Stop the debugger
     */
    void StopDebugger();

    /**
     * Signal callback for the debug version information. This version string is
     * sent immediately after StartDebugger() is called.
     */
    void DebugVersion(const ajn::InterfaceDescription::Member* member, const char* sourcePath, ajn::Message& msg);

    /**
     * Get the basic information about the debug target
     *
     * @param version[out]      Version number
     * @param describe[out]     Description
     * @param targInfo[out]     Target Information
     * @param endianness[out]   Little or big endian
     */
    void BasicInfo(uint16_t* version, char** describe, char** targInfo, uint8_t* endianness);

    /**
     * Sends a trigger request to signal a notification message
     */
    void Trigger(void);

    /**
     * Attach to an already running script
     * (Only way to re-attach once detached without restarting the console)
     */
    void Attach(void);

    /**
     * Detach the console from the debugger
     * Note: The script will continue to run
     */
    void Detach(void);

    /**
     * Step into debug request
     */
    void StepIn(void);

    /**
     * Step out debug request
     */
    void StepOut(void);

    /**
     * Step over debug request
     */
    void StepOver(void);

    /**
     * Send the resume (run/continue) command to the debugger
     */
    void Resume(void);

    /**
     * Pause the debugger
     */
    void Pause(void);

    /**
     * Add a new breakpoint
     *
     * @param file      Script file to add the breakpoint in
     * @param line      Line to add the breakpoint at
     */
    void AddBreak(char* file, uint16_t line);

    /**
     * Delete a breakpoint
     *
     * @param index     Breakpoint index, 0 = first added, 1 = second etc.
     */
    void DelBreak(uint8_t index);

    /**
     * Get all breakpoints
     *
     * @param breakpoints[out]  Array of BreakPoint structures
     * @param count[out]        Number of breakpoints in param 1's array
     */
    void ListBreak(AJS_BreakPoint** breakpoints, uint8_t* count);

    /**
     * Frees a list of breakpoints generated from ListBreak
     *
     * @param breakpoints       Array of BreakPoint structures
     * @param num               Number of breakpoint structures
     */
    void FreeBreakpoints(AJS_BreakPoint* breakpoints, uint8_t num);

    /**
     * Get a variable
     *
     * @param var        Variable name
     * @param value[out] Pointer that will contain the variables value
     * @param size[out]  Size of the variable
     * @param type[out]    Contains the type of variable
     * @return           True if the variable existed, false if not
     */
    bool GetVar(char* var, uint8_t** value, uint32_t* size, uint8_t* type);

    /**
     * Put a value into a variable
     *
     * @param var       Name of the variable
     * @param value     Value to set the variable to
     * @param size      Size of the values data
     *
     * @return          True if the variable was successfully set
     *                  False if the variable did not exist or the type/size was invalid.
     */
    bool PutVar(char* var, uint8_t* value, uint32_t size, uint8_t type);

    /**
     * Get call stack.
     *
     * @param stack[out]    Array of call stack entries
     * @param size[out]     Number of call stack entires
     */
    void GetCallStack(AJS_CallStack** stack, uint8_t* size);

    /**
     * Free the call stack populated by GetCallStack. This function must be called
     * after GetCallStack or it will cause a memory leak
     *
     * @param stack         Call stack array
     * @param size          Depth of the call stack
     */
    void FreeCallStack(AJS_CallStack* stack, uint8_t size);

    /**
     * Get all local variables.
     *
     * @param list[out]     Array of Locals structure containing all local variables
     * @param size[out]     Number of local variables found
     */
    void GetLocals(AJS_Locals** list, uint16_t* size);

    /**
     * Free local variable array populated by GetLocals
     *
     * @param list          Array of local variables
     * @param size          Number of local variables
     */
    void FreeLocals(AJS_Locals* list, uint16_t size);

    /**
     * Do an eval while in the debugger
     *
     * @param str           Eval string
     * @param value[out]    Returned evaluation data
     * @param size[out]     Size of the data
     * @param type[out]     Type of the data (tval/dval)
     */
    void DebugEval(char* str, uint8_t** value, uint32_t* size, uint8_t* type);

    /**
     * Dump the duktape heap
     */
    void DumpHeap(void);
    /**
     * Get a script from the debug target. Opposite direction as Install.
     *
     * @param script[out]   Pointer to the scripts data
     * @param length[out]   Size of the script
     *
     * @return              True if getting the script was successful
     */
    bool GetScript(uint8_t** script, uint32_t* length);

    /**
     * Get the debug targets current status
     *
     * @return              status: running, paused, busy, or disconnected
     *                      -1 on error
     */
    AJS_DebugStatus GetDebugStatus(void);

    /**
     * Get the installed scripts name
     *
     * @return              Name of the script that is installed
     */
    char* GetScriptName(void);

    void SetVerbose(bool newValue) {
        verbose = newValue;
    }

    bool GetVerbose() {
        return verbose;
    }

    AJS_DebugStatus GetDebugState(void)
    {
        return debugState;
    }

    void SetDebugState(AJS_DebugStatus state)
    {
        debugState = state;
    }

    class Event;
    AJS_DebugStatus debugState;
    bool activeDebug;
    bool quiet;
    SignalRegistration* handlers;
    bool verbose;
  private:

    /*
     * Copying and assignment not supported
     */
    AJS_Console(const AJS_Console&);
    const AJS_Console& operator=(const AJS_Console&);

    virtual void RegisterHandlers(ajn::BusAttachment* ajb);
    virtual void JoinSessionCB(QStatus status, ajn::SessionId sessionId, const ajn::SessionOpts& opts, void* context);

    ajn::SessionId sessionId;
    ajn::ProxyBusObject* proxy;
    char* connectedBusName;
    ajn::BusAttachment* aj;
    Event* ev;
    qcc::String deviceName;
    static const size_t printBufLen = 1024;
    char printBuf[printBufLen];
};

#endif