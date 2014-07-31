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

static const char installedScript[] = "ajs_tmp.js";
static const char* scriptName = installedScript;

size_t AJS_GetMaxScriptLen()
{
    return 64 * 1024;
}

uint8_t AJS_IsScriptInstalled()
{
    FILE* file = fopen(scriptName, "rb");
    if (file) {
        fclose(file);
        return TRUE;
    } else {
        return FALSE;
    }
}

typedef struct {
    FILE* file;
    uint8_t* buf;
    size_t len;
    uint8_t mode;
} SCRIPTF;

void* AJS_OpenScript(uint8_t mode)
{
    SCRIPTF* sf;

    sf = malloc(sizeof(SCRIPTF));
    if (mode == AJS_SCRIPT_READ) {
        sf->file = fopen(scriptName, "rb");
    } else {
        sf->file = fopen(scriptName, "wb");
    }
    if (!sf->file) {
        free(sf);
        return NULL;
    } else {
        sf->len = 0;
        sf->buf = NULL;
        sf->mode = mode;
        return sf;
    }
}

AJ_Status AJS_ReadScript(void* scriptf, const uint8_t** data, size_t* len)
{
    SCRIPTF* sf = scriptf;

    if (sf->mode != AJS_SCRIPT_READ) {
        return AJ_ERR_INVALID;
    }
    if (fseek(sf->file, 0, SEEK_END) == 0) {
        sf->len = ftell(sf->file);
        sf->buf = malloc(sf->len);
        fseek(sf->file, 0, SEEK_SET);
        fread(sf->buf, sf->len, 1, sf->file);
        *data = sf->buf;
        *len = sf->len;
        return AJ_OK;
    } else {
        return AJ_ERR_READ;
    }
}

AJ_Status AJS_WriteScript(void* scriptf, const uint8_t* data, size_t len)
{
    SCRIPTF* sf = scriptf;

    if (sf->mode != AJS_SCRIPT_WRITE) {
        return AJ_ERR_INVALID;
    }
    if (fwrite(data, len, 1, sf->file) == 1) {
        sf->len += len;
        return AJ_OK;
    } else {
        return AJ_ERR_WRITE;
    }
}

AJ_Status AJS_CloseScript(void* scriptf)
{
    SCRIPTF* sf = scriptf;

    fclose(sf->file);
    if (sf->buf) {
        free(sf->buf);
    }
    free(sf);
    return AJ_OK;
}

AJ_Status AJS_DeleteScript()
{
    _unlink(scriptName);
    return AJ_OK;
}

static AJ_Status InstallScript(const char* fn)
{
    AJ_Status status = AJ_OK;
    void* scriptIn;

    scriptName = fn;
    scriptIn = AJS_OpenScript(AJS_SCRIPT_READ);
    if (!scriptIn) {
        AJ_Printf("Cannot open script file %s\n", scriptName);
        status = AJ_ERR_READ;
    } else {
        const uint8_t* data;
        size_t len;
        status = AJS_ReadScript(scriptIn, &data, &len);
        if (status == AJ_OK) {
            void* scriptOut = NULL;
            scriptName = installedScript;
            scriptOut = AJS_OpenScript(AJS_SCRIPT_WRITE);
            if (!scriptOut) {
                status = AJ_ERR_WRITE;
            } else {
                status = AJS_WriteScript(scriptOut, data, len);
                AJS_CloseScript(scriptOut);
            }
        }
        AJS_CloseScript(scriptIn);
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

int main(int argc, char*argv[])
{
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
#endif

    if (argc >= 2) {
        if (argc != 2) {
            AJ_Printf("Usage: %s [script_file]\n", argv[0]);
            exit(-1);
        }
        status = InstallScript(argv[1]);
        if (status != AJ_OK) {
            AJ_Printf("Failed to install script %s\n", argv[1]);
            exit(-1);
        }
    }

    AJ_Initialize();
    status = AJS_Main();

    return -((int)status);
}

