/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#include <alljoyn/Init.h>
#include "ajs_console_common.h"
#include "ajs_console.h"

extern "C" {

AJS_ConsoleCtx* AJS_ConsoleInit(void)
{
    AJS_ConsoleCtx* ctx = (AJS_ConsoleCtx*)malloc(sizeof(AJS_ConsoleCtx));
    if (ctx) {
        AllJoynInit();
        AllJoynRouterInit();
        AJS_Console* console = new AJS_Console;
        if (console) {
            ctx->console = (void*)console;
            ctx->version = NULL;
            console->handlers = (SignalRegistration*)malloc(sizeof(SignalRegistration));
            if (!console->handlers) {
                free(ctx);
                return NULL;
            }
            memset(console->handlers, 0, sizeof(SignalRegistration));
        } else {
            free(ctx);
            ctx = NULL;
        }
    }
    return ctx;
}

void AJS_ConsoleDeinit(AJS_ConsoleCtx* ctx)
{
    if (ctx) {
        if (ctx->console) {
            delete static_cast<AJS_Console*>(ctx->console);
        }
        free(ctx);
    }
    AllJoynShutdown();
    AllJoynRouterShutdown();
}

void AJS_ConsoleRegisterSignalHandlers(AJS_ConsoleCtx* ctx, SignalRegistration* handlers)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return;
    }
    console->handlers->notification = handlers->notification;
    console->handlers->print = handlers->print;
    console->handlers->alert = handlers->alert;
    console->handlers->dbgVersion = handlers->dbgVersion;
    console->handlers->dbgNotification = handlers->dbgNotification;
}

int AJS_ConsoleConnect(AJS_ConsoleCtx* ctx, const char* deviceName, volatile sig_atomic_t* interrupt)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return 0;
    }
    console->Connect(deviceName, interrupt);
    return 1;
}

int8_t AJS_ConsoleEval(AJS_ConsoleCtx* ctx, const char* script)
{
    AJS_Console* console;
    qcc::String scriptStr;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return 0;
    }
    scriptStr.assign(script, strlen(script));
    return console->Eval(scriptStr);
}

int AJS_ConsoleReboot(AJS_ConsoleCtx* ctx)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return 0;
    }
    if (console->Reboot() == ER_OK) {
        return 1;
    }
    return 0;
}

int AJS_ConsoleInstall(AJS_ConsoleCtx* ctx, const char* name, const uint8_t* script, size_t len)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return 0;
    }
    if (script) {
        if (console->Install(qcc::String(name), script, len) == ER_OK) {
            return 1;
        }
    }
    return 0;
}

int AJS_ConsoleLockdown(AJS_ConsoleCtx* ctx)
{
    AJS_Console* console;
    if (ctx && ctx->console) {
        console = static_cast<AJS_Console*>(ctx->console);
    } else {
        return 0;
    }
    return console->LockdownConsole();
}

void AJS_ConsoleSetVerbose(AJS_ConsoleCtx* ctx, uint8_t newValue)
{
    ctx->verbose = newValue;
}

uint8_t AJS_ConsoleGetVerbose(AJS_ConsoleCtx* ctx)
{
    return ctx->verbose;
}

void AJS_Debug_SetQuiet(AJS_ConsoleCtx* ctx, uint8_t quiet)
{
    ctx->quiet = quiet;
}
} // End of extern "C"


