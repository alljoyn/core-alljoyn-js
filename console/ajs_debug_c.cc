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

#include "ajs_console.h"
#include "ajs_console_c.h"
#include "ajs_console_common.h"
#include <stdlib.h>

extern "C" {

AJS_DebugStatusCode AJS_Debug_StartDebugger(AJS_ConsoleCtx* ctx)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->StartDebugger();
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_StopDebugger(AJS_ConsoleCtx* ctx)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->StopDebugger();
    return DBG_OK;
}

char* AJS_Debug_GetVersion(AJS_ConsoleCtx* ctx)
{
    if (ctx) {
        if (ctx->version) {
            return ctx->version;
        }
    }
    return NULL;
}

void AJS_Debug_SetActiveDebug(AJS_ConsoleCtx* ctx, uint8_t active)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return;
    }
    console->activeDebug = active;
}

uint8_t AJS_Debug_GetActiveDebug(AJS_ConsoleCtx* ctx)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    return console->activeDebug;
}

AJS_DebugStatus AJS_Debug_GetDebugState(AJS_ConsoleCtx* ctx)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return AJS_DEBUG_DETACHED;
    }
    return console->GetDebugState();
}

AJS_DebugStatusCode AJS_Debug_SetDebugState(AJS_ConsoleCtx* ctx, AJS_DebugStatus state)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->SetDebugState(state);
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_BasicInfo(AJS_ConsoleCtx* ctx, uint16_t* version, char** description, char** targInfo, uint8_t* endianness)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->BasicInfo(version, description, targInfo, endianness);
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_Trigger(AJS_ConsoleCtx* ctx)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->Trigger();
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_StepIn(AJS_ConsoleCtx* ctx)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->StepIn();
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_StepOut(AJS_ConsoleCtx* ctx)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->StepOut();
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_StepOver(AJS_ConsoleCtx* ctx)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->StepOver();
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_Resume(AJS_ConsoleCtx* ctx)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->Resume();
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_Pause(AJS_ConsoleCtx* ctx)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->Pause();
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_Attach(AJS_ConsoleCtx* ctx)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->Attach();
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_Detach(AJS_ConsoleCtx* ctx)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->Detach();
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_AddBreakpoint(AJS_ConsoleCtx* ctx, char* file, uint16_t line)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->AddBreak(file, line);
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_DelBreakpoint(AJS_ConsoleCtx* ctx, uint8_t index)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->DelBreak(index);
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_GetVar(AJS_ConsoleCtx* ctx, char* var, uint8_t** value, uint32_t* size, uint8_t* type)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->GetVar(var, value, size, type);
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_PutVar(AJS_ConsoleCtx* ctx, char* name, uint8_t* value, uint32_t size, uint8_t type)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->PutVar(name, value, size, type);
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_GetCallStack(AJS_ConsoleCtx* ctx, AJS_CallStack** stack, uint8_t* size)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->GetCallStack(stack, size);
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_FreeCallStack(AJS_ConsoleCtx* ctx, AJS_CallStack* stack, uint8_t size)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->FreeCallStack(stack, size);
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_GetLocals(AJS_ConsoleCtx* ctx, AJS_Locals** locals, uint16_t* num)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->GetLocals(locals, num);
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_FreeLocals(AJS_ConsoleCtx* ctx, AJS_Locals* locals, uint8_t num)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->FreeLocals(locals, num);
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_ListBreakpoints(AJS_ConsoleCtx* ctx, AJS_BreakPoint** breakpoints, uint8_t* num)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->ListBreak(breakpoints, num);
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_FreeBreakpoints(AJS_ConsoleCtx* ctx, AJS_BreakPoint* breakpoints, uint8_t num)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->FreeBreakpoints(breakpoints, num);
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_Eval(AJS_ConsoleCtx* ctx, char* str, uint8_t** ret, uint32_t* size, uint8_t* type)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->DebugEval(str, ret, size, type);
    return DBG_OK;
}

AJS_DebugStatusCode AJS_Debug_GetScript(AJS_ConsoleCtx* ctx, uint8_t** script, uint32_t* size)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return DBG_ERR;
    }
    console->GetScript(script, size);
    return DBG_OK;
}
}