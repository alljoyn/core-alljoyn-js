/**
 * @file
 */
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

#include "ajs.h"
#include "ajs_target.h"

/**
 * Install a script as specified on the command line
 */
AJ_Status AJS_InstallScript(const char* fn);

typedef struct {
    const char* deviceName;
    const char* scriptName;
    const char* nvramFile;
    const char* logFile;
    uint32_t daemonize;
} AJS_CmdOptions;

int AJS_CmdlineOptions(int argc, char* argv[], AJS_CmdOptions* options);


