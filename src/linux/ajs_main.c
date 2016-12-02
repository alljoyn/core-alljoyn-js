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

#include "../ajs.h"
#include "../ajs_target.h"
#include "../ajs_cmdline.h"
#include <ajtcl/aj_nvram.h>

#include <unistd.h>
#include <errno.h>

/*
 * Approximate limit on number of bytes to accumulate in the log file.
 */
#define MAX_LOG_FILE_SIZE 8192
#define DEFAULT_LOG_FILE  "/tmp/AJS.log"

int main(int argc, char* argv[])
{
    AJ_Status status;
    AJS_CmdOptions options;

    if (AJS_CmdlineOptions(argc, argv, &options)) {
        goto Usage;
    }

    AJ_Initialize();

    if (options.daemonize) {
        int ret = daemon(1, 0);
        if (ret < 0) {
            AJ_Printf("Failed to launch daemon errno=%d\n", errno);
            exit(1);
        }
        if (!options.logFile) {
            options.logFile = DEFAULT_LOG_FILE;
        }
    }
    if (options.logFile) {
        AJ_SetLogFile(options.logFile, MAX_LOG_FILE_SIZE);
    }

    if (options.scriptName) {
        status = AJS_InstallScript(options.scriptName);
        if (status != AJ_OK) {
            AJ_Printf("Failed to install script %s\n", options.scriptName);
            exit(1);
        }
    }
    /*
     * If running as a daemon keep restarting
     */
    do {
        status = AJS_Main(options.deviceName);
    } while (options.daemonize || (status == AJ_ERR_RESTART));

    return (int)status;

Usage:

#ifndef NDEBUG
    AJ_Printf("Usage: %s [--debug] [--daemon] [--log-file <log_file>] [--name <device_name>] [script_file]\n", argv[0]);
#else
    AJ_Printf("Usage: %s [--daemon] [--log-file <log_file>] [--name <device_name>] [script_file]\n", argv[0]);
#endif
    exit(1);
}
