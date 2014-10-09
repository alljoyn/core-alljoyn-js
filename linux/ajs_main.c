/**
 * @file
 */
/******************************************************************************
 * Copyright (c) 2013, 2014, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include "ajs.h"
#include "ajs_target.h"

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
        fseek(sf->file, 0, SEEK_SET);
        fread(sf->buf, sf->len, 1, sf->file);
        *data = sf->buf;
        *len = (uint32_t)sf->len;
        return AJ_OK;
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

static AJ_Status InstallScript(const char* fn)
{
    AJ_Status status = AJ_OK;
    SCRIPTF sf;

    if (!OpenScript(&sf, fn)) {
        AJ_Printf("Cannot open script file %s\n", fn);
        status = AJ_ERR_UNKNOWN;
    } else {
        const uint8_t* data;
        uint32_t len;
        status = ReadScript(&sf, &data, &len);
        if (status == AJ_OK) {
            AJ_NV_DATASET* ds = AJ_NVRAM_Open(AJS_SCRIPT_NVRAM_ID, "w", sizeof(len) + len);
            if (ds) {
                AJ_NVRAM_Write(&len, sizeof(len), ds);
                AJ_NVRAM_Write(data, len, ds);
                AJ_NVRAM_Close(ds);
            }
            /*
             * Now store the script name
             */
            len = strlen(fn) + 1;
            ds = AJ_NVRAM_Open(AJS_SCRIPT_NAME_NVRAM_ID, "w", len);
            if (ds) {
                AJ_NVRAM_Write(fn, len, ds);
                AJ_NVRAM_Close(ds);
            }
        }
        CloseScript(&sf);
    }
    return status;
}

extern uint8_t dbgMSG;
extern uint8_t dbgHELPER;
extern uint8_t dbgBUS;
extern uint8_t dbgABOUT;
extern uint8_t dbgINTROSPECT;
extern uint8_t dbgAJCPS;
extern uint8_t dbgAJS;
extern uint8_t dbgHEAP;
extern uint8_t dbgNET;
extern uint8_t dbgHEAPDUMP;
extern uint8_t dbgCONSOLE;
extern uint8_t dbgGPIO;

int main(int argc, char* argv[])
{
    const char* deviceName = NULL;
    int argn = 1;
    AJ_Status status = AJ_OK;
#ifndef NDEBUG
    AJ_DbgLevel = 4;
    dbgMSG = 0;
    dbgHELPER = 0;
    dbgABOUT = 0;
    dbgBUS = 0;
    dbgINTROSPECT = 0;
    dbgAJCPS = 0;
    dbgAJS = 0;
    dbgHEAP = 0;
    dbgNET = 0;
    dbgHEAPDUMP = 0;
    dbgCONSOLE = 0;
    dbgGPIO = 1;
#endif

    AJ_Initialize();

    if (argc > argn) {
        if (strcmp(argv[argn], "--name") == 0) {
            ++argn;
            if (argn >= argc) {
                goto Usage;
            }
            deviceName = argv[argn];
            ++argn;
        }
        if (argc > argn) {
            if (argc > (argn + 1)) {
                goto Usage;
            }
            status = InstallScript(argv[argn]);
            if (status != AJ_OK) {
                AJ_Printf("Failed to install script %s\n", argv[argn]);
                exit(-1);
            }
        }
    }

    status = AJS_Main(deviceName);

    return -((int)status);

Usage:

    AJ_Printf("Usage: %s [--name <device-name>] [script_file]\n", argv[0]);
    exit(-1);
}

