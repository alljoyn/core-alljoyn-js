/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
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

#ifndef AJS_CONSOLE_C_H_
#define AJS_CONSOLE_C_H_

#include <signal.h>
#include <stdint.h>
#include "ajs_console_common.h"

/**
 * Register any desired handler functions
 *
 * @param ctx       Console context
 * @param handlers  List of functions to be registered (any element can be NULL)
 */
void AJS_ConsoleRegisterSignalHandlers(AJS_ConsoleCtx* ctx, SignalRegistration* handlers);

/**
 * Set the verbose level for the console
 *
 * @param ctx           Console context
 * @param verbose       Set verbose on or off
 */
void AJS_ConsoleSetVerbose(AJS_ConsoleCtx* ctx, uint8_t verbose);

/**
 * Initialize the console. This must be called before any other
 * console/debug commands.
 *
 * @return              Context pointer needed for any subsequent console/debug commands
 */
AJS_ConsoleCtx* AJS_ConsoleInit(void);

/**
 * De-initialize the console
 *
 * @param ctx           Console context pointer
 */
void AJS_ConsoleDeinit(AJS_ConsoleCtx* ctx);

/**
 * Connect the console to an AllJoyn.js target
 *
 * @param ctx           Console context
 * @param deviceName    Name of the device your connecting to (can be NULL)
 * @param interrupt     Interrupt pointer (sigint handler, can be NULL)
 * @return              1 on success, 0 on failure
 */
int AJS_ConsoleConnect(AJS_ConsoleCtx* ctx, const char* deviceName, volatile sig_atomic_t* interrupt);

/**
 * Eval a JavaScript string
 *
 * @param ctx           Console context
 * @param script        Eval string
 * @return              Result of AJSConsole.Eval()
 */
int8_t AJS_ConsoleEval(AJS_ConsoleCtx* ctx, const char* script);

/**
 * Signal the AllJoyn.js target to reboot
 *
 * @param ctx           Console context
 * @return              1 on success, 0 on failure
 */
int AJS_ConsoleReboot(AJS_ConsoleCtx* ctx);

/**
 * Install a script
 *
 * @param ctx           Console context
 * @param name          Device name
 * @param script        Pointer to the script to be installed
 * @param len           Byte length of the script
 * @return              1 on success, 0 on failure.
 */
int AJS_ConsoleInstall(AJS_ConsoleCtx* ctx, const char* name, const uint8_t* script, size_t len);

#endif /* AJS_CONSOLE_C_H_ */