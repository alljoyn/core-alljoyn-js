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

#include "ajs.h"
#include "ajs_target.h"
#include "ajs_cmdline.h"
#include "ajs_storage.h"

#include <stdio.h>
#include <errno.h>

typedef struct {
    FILE* file;
    uint8_t* buf;
    size_t len;
} SCRIPTF;

static uint8_t OpenScript(SCRIPTF* sf, const char* scriptName)
{
    sf->file = fopen(scriptName, "rb");
    if (!sf->file) {
        return FALSE;
    } else {
        sf->len = 0;
        sf->buf = NULL;
        return TRUE;
    }
}

static AJ_Status ReadScript(SCRIPTF* sf, const uint8_t** data, uint32_t* len)
{
    if (fseek(sf->file, 0, SEEK_END) == 0) {
        sf->len = ftell(sf->file);
        sf->buf = malloc(sf->len);
        if (sf->buf) {
            fseek(sf->file, 0, SEEK_SET);
            fread(sf->buf, sf->len, 1, sf->file);
            *data = sf->buf;
            *len = (uint32_t)sf->len;
            return AJ_OK;
        } else {
            return AJ_ERR_RESOURCES;
        }
    } else {
        return AJ_ERR_FAILURE;
    }
}

static AJ_Status CloseScript(SCRIPTF* sf)
{
    fclose(sf->file);
    if (sf->buf) {
        free(sf->buf);
    }
    return AJ_OK;
}

AJ_Status AJS_InstallScript(const char* fn)
{
    AJ_Status status;
    SCRIPTF sf;

    if (!OpenScript(&sf, fn)) {
        AJ_Printf("Cannot open script file %s\n", fn);
        status = AJ_ERR_UNKNOWN;
    } else {
        void* ctx;
        const uint8_t* data;
        uint32_t len;
        status = ReadScript(&sf, &data, &len);
        if (status == AJ_OK) {
            AJ_NV_DATASET* ds = NULL;
            status = AJS_OpenScript(len, &ctx);
            if (status == AJ_ERR_RESOURCES) {
                AJ_ErrPrintf(("AJS_InstallScript(): Script is too large\n"));
                return status;
            } else if (status != AJ_OK) {
                AJ_ErrPrintf(("AJS_InstallScript(): Error opening script\n"));
                return status;
            }
            status = AJS_WriteScript((uint8_t*)data, len, ctx);
            if (status != AJ_OK) {
                AJ_ErrPrintf(("AJS_InstallScript(): Error writing script to storage\n"));
                return status;
            }
            AJS_CloseScript(ctx);
            /*
             * Store the scripts length
             */
            ds = AJ_NVRAM_Open(AJS_SCRIPT_SIZE_ID, "w", sizeof(uint32_t));
            if (!ds) {
                status = AJ_ERR_NO_MATCH;
                goto NVRAM_Cleanup;
            }
            if (AJ_NVRAM_Write(&len, sizeof(len), ds) != sizeof(len)) {
                status = AJ_ERR_RESOURCES;
                goto NVRAM_Cleanup;
            }
            AJ_NVRAM_Close(ds);
            /*
             * Now store the script name
             */
            len = strlen(fn) + 1;
            ds = AJ_NVRAM_Open(AJS_SCRIPT_NAME_NVRAM_ID, "w", len);
            if (!ds) {
                status = AJ_ERR_NO_MATCH;
                goto NVRAM_Cleanup;
            }
            if (AJ_NVRAM_Write(fn, len, ds) != len) {
                status = AJ_ERR_RESOURCES;
                goto NVRAM_Cleanup;
            }
        NVRAM_Cleanup:
            if (ds) {
                AJ_NVRAM_Close(ds);
            }
        }
        CloseScript(&sf);
    }
    return status;
}

#ifndef NDEBUG
extern uint8_t dbgMSG;
extern uint8_t dbgHELPER;
extern uint8_t dbgBUS;
extern uint8_t dbgABOUT;
extern uint8_t dbgINTROSPECT;
#ifdef CONTROLPANEL_SERVICE
extern uint8_t dbgAJCPS;
#endif
extern uint8_t dbgAJS;
extern uint8_t dbgHEAP;
extern uint8_t dbgNET;
extern uint8_t dbgHEAPDUMP;
#if !defined(AJS_CONSOLE_LOCKDOWN)
extern uint8_t dbgCONSOLE;
extern uint8_t dbgDEBUGGER;
#endif
extern uint8_t dbgGPIO;
#endif

int AJS_CmdlineOptions(int argc, char* argv[], AJS_CmdOptions* options)
{
    int argn = 1;

#ifndef NDEBUG
    AJ_DbgLevel = 2;
    dbgMSG = 0;
    dbgHELPER = 0;
    dbgABOUT = 0;
    dbgBUS = 0;
    dbgINTROSPECT = 0;
#ifdef CONTROLPANEL_SERVICE
    dbgAJCPS = 0;
#endif
    dbgAJS = 0;
    dbgHEAP = 0;
    dbgNET = 0;
    dbgHEAPDUMP = 0;
    dbgGPIO = 0;
#if !defined(AJS_CONSOLE_LOCKDOWN)
    dbgCONSOLE = 0;
    dbgDEBUGGER = 0;
#endif
#endif

    memset(options, 0, sizeof(*options));

    while (argn < argc) {
        if (strcmp(argv[argn], "--debug") == 0) {
#ifndef NDEBUG
            AJ_DbgLevel = 4;
            dbgAJS = 1;
#if !defined(AJS_CONSOLE_LOCKDOWN)
            dbgCONSOLE = 4;
            dbgDEBUGGER = 4;
#endif
            ++argn;
            continue;
#else
            AJ_Printf("Not built with debugging\n");
            return 1;
#endif
        }
        if (strcmp(argv[argn], "--nvram-file") == 0) {
            if (++argn >= argc) {
                return 1;
            }
            options->nvramFile = argv[argn++];
            continue;
        }
        if (strcmp(argv[argn], "--log-file") == 0) {
            if (++argn >= argc) {
                return 1;
            }
            options->logFile = argv[argn++];
            continue;
        }
        if (strcmp(argv[argn], "--daemon") == 0) {
            options->daemonize = TRUE;
            ++argn;
            continue;
        }
        if (strcmp(argv[argn], "--name") == 0) {
            if (++argn >= argc) {
                return 1;
            }
            options->deviceName = argv[argn++];
            continue;
        }
        if (argv[argn][0] == '-') {
            return 1;
        }
        if (argc > (argn + 1)) {
            return 1;
        }
        options->scriptName = argv[argn];
        ++argn;
    }
    return 0;
}