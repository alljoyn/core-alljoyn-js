#ifndef AJS_DEBUGGER_H_
#define AJS_DEBUGGER_H_
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

#include "duktape.h"
#include "ajs.h"

/*
 * Debug messages (org.allseen.scriptDebugger)
 */
#define DBG_NOTIF_MSGID         AJ_APP_MESSAGE_ID(0, 2, 0)
#define DBG_BASIC_MSGID         AJ_APP_MESSAGE_ID(0, 2, 1)
#define DBG_TRIGGER_MSGID       AJ_APP_MESSAGE_ID(0, 2, 2)
#define DBG_PAUSE_MSGID         AJ_APP_MESSAGE_ID(0, 2, 3)
#define DBG_RESUME_MSGID        AJ_APP_MESSAGE_ID(0, 2, 4)
#define DBG_STEPIN_MSGID        AJ_APP_MESSAGE_ID(0, 2, 5)
#define DBG_STEPOVER_MSGID      AJ_APP_MESSAGE_ID(0, 2, 6)
#define DBG_STEPOUT_MSGID       AJ_APP_MESSAGE_ID(0, 2, 7)
#define DBG_LISTBREAK_MSGID     AJ_APP_MESSAGE_ID(0, 2, 8)
#define DBG_ADDBREAK_MSGID      AJ_APP_MESSAGE_ID(0, 2, 9)
#define DBG_DELBREAK_MSGID      AJ_APP_MESSAGE_ID(0, 2, 10)
#define DBG_GETVAR_MSGID        AJ_APP_MESSAGE_ID(0, 2, 11)
#define DBG_PUTVAR_MSGID        AJ_APP_MESSAGE_ID(0, 2, 12)
#define DBG_GETCALL_MSGID       AJ_APP_MESSAGE_ID(0, 2, 13)
#define DBG_GETLOCALS_MSGID     AJ_APP_MESSAGE_ID(0, 2, 14)
#define DBG_DUMPHEAP_MSGID      AJ_APP_MESSAGE_ID(0, 2, 15)
#define DBG_VERSION_MSGID       AJ_APP_MESSAGE_ID(0, 2, 16)
#define DBG_GETSCRIPT_MSGID     AJ_APP_MESSAGE_ID(0, 2, 17)
#define DBG_DETACH_MSGID        AJ_APP_MESSAGE_ID(0, 2, 18)
#define DBG_EVAL_MSGID          AJ_APP_MESSAGE_ID(0, 2, 19)
#define DBG_BEGIN_MSGID         AJ_APP_MESSAGE_ID(0, 2, 20)
#define DBG_END_MSGID           AJ_APP_MESSAGE_ID(0, 2, 21)
#define DBG_GETSTATUS_MSGID     AJ_APP_MESSAGE_ID(0, 2, 22)
#define DBG_GETSCRIPTNAME_MSGID AJ_APP_MESSAGE_ID(0, 2, 23)
/* Duplicate from console because an Eval can be processed in the debugger as well */
#define EVAL_MSGID              AJ_APP_MESSAGE_ID(0,  1, 3)
#define LOCK_CONSOLE_MSGID      AJ_APP_MESSAGE_ID(0,  1, 10)
/**
 * Put the console output into quiet mode.
 *
 * @param quiet  If non zero enable quiet mode, if zero disable quiet mode
 */
void AJS_ConsoleSetQuiet(uint8_t quiet);

/**
 * Handle a Debugger Start command.
 *
 * @param ctx  An opaque pointer to a duktape context structure
 * @param msg  The message to handle
 *
 * @return - AJ_OK if the message was handled
 *         - Otherwise and error status
 */
AJ_Status AJS_StartDebugger(duk_context* ctx, AJ_Message* msg);

/**
 * Handle a Debugger Stop command.
 *
 * @param ctx  An opaque pointer to a duktape context structure
 * @param msg  The message to handle
 *
 * @return - AJ_OK if the message was handled
 *         - Otherwise and error status
 */
AJ_Status AJS_StopDebugger(duk_context* ctx, AJ_Message* msg);

/**
 * Handle a Debugger GetStatus command.
 *
 * @param ctx  An opaque pointer to a duktape context structure
 * @param msg  The message to handle
 *
 * @return - AJ_OK if the message was handled
 *         - Otherwise and error status
 */
AJ_Status AJS_DebuggerGetStatus(duk_context* ctx, AJ_Message* msg);

/**
 * Check if a debug client is currently attached
 *
 * @return  TRUE if attached
 *          FALSE if detached
 */
uint8_t AJS_DebuggerIsAttached(void);

/**
 * Handle a Debugger GetScriptName command.
 *
 * @param ctx  An opaque pointer to a duktape context structure
 * @param msg  The message to handle
 *
 * @return - AJ_OK if the message was handled
 *         - Otherwise and error status
 */
AJ_Status AJS_DebuggerGetScriptName(duk_context* ctx, AJ_Message* msg);

/**
 * Handle a Debugger GetScript command.
 *
 * @param ctx  An opaque pointer to a duktape context structure
 * @param msg  The message to handle
 *
 * @return - AJ_OK if the message was handled
 *         - Otherwise and error status
 */
AJ_Status AJS_DebuggerGetScript(duk_context* ctx, AJ_Message* msg);

/**
 * Handle a Debugger control command.
 *
 * @param ctx  An opaque pointer to a duktape context structure
 * @param msg  The message to handle
 *
 * @return - AJ_OK if the message was handled
 *         - Otherwise and error status
 */
AJ_Status AJS_DebuggerCommand(duk_context* ctx, AJ_Message* msg);

#endif /* AJS_DEBUGGER_H_ */