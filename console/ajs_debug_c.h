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

#ifndef AJS_DEBUG_H_
#define AJS_DEBUG_H_

#include <stdint.h>
#include "ajs_console_c.h"
#include "ajs_console_common.h"

/**
 * Set the active debug flag meaning the console has started in
 * debug mode.
 *
 * @param ctx       Console context
 * @param active    New state to be set
 */
void AJS_Debug_SetActiveDebug(AJS_ConsoleCtx* ctx, uint8_t active);

/**
 * Get the active debug state
 *
 * @param ctx       Console context
 * @return          Value of the active debug flag
 */
uint8_t AJS_Debug_GetActiveDebug(AJS_ConsoleCtx* ctx);

/**
 * Set the quiet flag
 *
 * @param ctx       Console context
 * @param quiet     New value of quiet flag
 */
void AJS_Debug_SetQuiet(AJS_ConsoleCtx* ctx, uint8_t quiet);

/**
 * Set the debuggers current state
 *
 * @param ctx       Console context
 * @param state     State
 * @return          DBG_OK if the state was set successfully
 */
AJS_DebugStatusCode AJS_Debug_SetDebugState(AJS_ConsoleCtx* ctx, AJS_DebugStatus state);

/**
 * Get the debuggers current state
 *
 * @param ctx       Console context
 * @return          Debuggers state
 */
AJS_DebugStatus AJS_Debug_GetDebugState(AJS_ConsoleCtx* ctx);

/**
 * Start the debugger
 *
 * @param ctx       Console context
 * @return          DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_StartDebugger(AJS_ConsoleCtx* ctx);

/**
 * Stop the debugger
 *
 * @param ctx       Console context
 * @return          DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_StopDebugger(AJS_ConsoleCtx* ctx);

/**
 * Get the debugger version
 *
 * @param ctx       Console context
 * @return          Debug version string
 */
char* AJS_Debug_GetVersion(AJS_ConsoleCtx* ctx);

/**
 * Get the basic information about the debug target
 *
 * @param ctx               Console context
 * @param version[out]      Version number
 * @param describe[out]     Description
 * @param targInfo[out]     Target Information
 * @param endianness[out]   Little or big endian
 * @return                  DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_BasicInfo(AJS_ConsoleCtx* ctx, uint16_t* version, char** description, char** targInfo, uint8_t* endianness);

/**
 * Sends a trigger request which results in a debug notification.
 *
 * @param ctx               Console context
 * @return                  DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_Trigger(AJS_ConsoleCtx* ctx);

/**
 * Step into request.
 *
 * @param ctx               Console context
 * @return                  DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_StepIn(AJS_ConsoleCtx* ctx);

/**
 * Step out request.
 *
 * @return                  DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_StepOut(AJS_ConsoleCtx* ctx);

/**
 * Step over request.
 *
 * @param ctx               Console context
 * @return                  DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_StepOver(AJS_ConsoleCtx* ctx);

/**
 * Resume request. Only has an effect when the debugger is in a paused state.
 *
 * @param ctx               Console context
 * @return                  DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_Resume(AJS_ConsoleCtx* ctx);

/**
 * Pause request. Only has an effect when the debugger is in a running state.
 *
 * @param ctx               Console context
 * @return                  DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_Pause(AJS_ConsoleCtx* ctx);

/**
 * Attach to an already running debug target
 * (Only way to re-attach once detached)
 *
 * @param ctx               Console context
 * @return                  DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_Attach(AJS_ConsoleCtx* ctx);

/**
 * Detach the console from the debugger
 * Note: The script will continue to run
 *
 * @param ctx               Console context
 * @return                  DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_Detach(AJS_ConsoleCtx* ctx);

/**
 * Add a new breakpoint
 *
 * @param ctx       Console context
 * @param file      Script file to add the breakpoint in
 * @param line      Line to add the breakpoint at
 * @return          DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_AddBreakpoint(AJS_ConsoleCtx* ctx, char* file, uint16_t line);

/**
 * Delete a breakpoint
 *
 * @param ctx       Console context
 * @param index     Breakpoint index, 0 = first added, 1 = second etc.
 * @return          DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_DelBreakpoint(AJS_ConsoleCtx* ctx, uint8_t index);

/**
 * Get a variable
 *
 * @param ctx       Console context
 * @param var        Variable name
 * @param value[out] Pointer that will contain the variables value
 * @param size[out]  Size of the variable
 * @param type[out]  Contains the type of variable
 * @return           DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_GetVar(AJS_ConsoleCtx* ctx, char* name, uint8_t** value, uint32_t* size, uint8_t* type);

/**
 * Put a value into a variable
 *
 * @param ctx       Console context
 * @param name      Variable name
 * @param value     Pointer to the desired value
 * @param size      Size of the value
 * @param type      Tval type
 * @return          DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_PutVar(AJS_ConsoleCtx* ctx, char* name, uint8_t* value, uint32_t size, uint8_t type);

/**
 * Get call stack.
 *
 * @param ctx           Console context
 * @param stack[out]    Array of call stack entries
 * @param size[out]     Number of call stack entires
 * @return              DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_GetCallStack(AJS_ConsoleCtx* ctx, AJS_CallStack** stack, uint8_t* size);

/**
 * Free the call stack populated by GetCallStack. This function must be called
 * after GetCallStack or it will cause a memory leak
 *
 * @param ctx           Console context
 * @param stack         Call stack array
 * @param size          Depth of the call stack
 * @return              DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_FreeCallStack(AJS_ConsoleCtx* ctx, AJS_CallStack* stack, uint8_t size);

/**
 * Get all local variables.
 *
 * @param ctx           Console context
 * @param list[out]     Array of Locals structure containing all local variables
 * @param size[out]     Number of local variables found
 * @return              DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_GetLocals(AJS_ConsoleCtx* ctx, AJS_Locals** locals, uint16_t* num);

/**
 * Free local variable array populated by GetLocals
 *
 * @param ctx           Console context
 * @param list          Array of local variables
 * @param size          Number of local variables
 * @return              DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_FreeLocals(AJS_ConsoleCtx* ctx, AJS_Locals* locals, uint8_t num);

/**
 * Get all breakpoints
 *
 * @param ctx               Console context
 * @param breakpoints[out]  Array of BreakPoint structures
 * @param count[out]        Number of breakpoints in first 'breakpoints' array
 * @return                  DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_ListBreakpoints(AJS_ConsoleCtx* ctx, AJS_BreakPoint** breakpoints, uint8_t* num);

/**
 * Frees a list of breakpoints generated from ListBreak
 *
 * @param ctx           Console context
 * @param breakpoints   Array of BreakPoint structures
 * @param num           Number of breakpoint structures
 * @return              DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_FreeBreakpoints(AJS_ConsoleCtx* ctx, AJS_BreakPoint* breakpoints, uint8_t num);

/**
 * Do an eval while debugging
 *
 * @param ctx           Console context
 * @param str           Eval string
 * @param value[out]    Returned evaluation data
 * @param size[out]     Size of the data
 * @param type[out]     Type of the data (tval/dval)
 * @return              DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_Eval(AJS_ConsoleCtx* ctx, char* str, uint8_t** ret, uint32_t* size, uint8_t* type);

/**
 * Get a script from the debug target. Opposite direction as Install.
 *
 * @param ctx           Console context
 * @param script[out]   Pointer to the scripts data
 * @param length[out]   Size of the script
 * @return              DBG_OK on success
 */
AJS_DebugStatusCode AJS_Debug_GetScript(AJS_ConsoleCtx* ctx, uint8_t** script, uint32_t* size);

#endif /* AJS_DEBUG_H_ */