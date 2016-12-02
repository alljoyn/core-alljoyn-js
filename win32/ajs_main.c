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
#include "ajs_cmdline.h"
#include "aj_target_nvram.h"


int main(int argc, char* argv[])
{
    AJ_Status status;
    AJS_CmdOptions options;

    if (AJS_CmdlineOptions(argc, argv, &options)) {
        goto Usage;
    }
    if (options.daemonize || options.logFile) {
        goto Usage;
    }

    AJ_SetNVRAM_FilePath(options.nvramFile);
    AJ_Initialize();

    if (options.scriptName) {
        status = AJS_InstallScript(options.scriptName);
        if (status != AJ_OK) {
            AJ_Printf("Failed to install script %s\n", options.scriptName);
            exit(1);
        }
    }

    do {
        status = AJS_Main(options.deviceName);
    } while (status == AJ_ERR_RESTART);

    return (int)status;

Usage:

#ifndef NDEBUG
    AJ_Printf("Usage: %s [--debug] [--name <device-name>] [--nvram-file <nvram-file>] [script_file]\n", argv[0]);
#else
    AJ_Printf("Usage: %s [--name <device-name>] [--nvram-file <nvram-file>] [script_file]\n", argv[0]);
#endif
    exit(1);
}
