/**
 * @file
 */
/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include <Windows.h>

#include "ajs.h"
#include "ajs_io.h"

extern void AJ_Net_Interrupt();

static HANDLE pipe = INVALID_HANDLE_VALUE;

static const char gui_server[] = "\\\\.\\pipe\\tmp\\ajs_gui";

/*
 * Struct for holding the state of a GPIO pin
 */
typedef struct {
    int16_t trigId;
    uint16_t pinId;
} GPIO;

static HANDLE cond;
static HANDLE writeEv;
static uint8_t recvBuf[3];

#define MAX_TRIGGERS 32

/*
 * Bit mask of allocated triggers (max 32)
 */
static uint32_t trigSet;

#define BIT_IS_SET(i, b)  ((i) & (1 << (b)))
#define BIT_SET(i, b)     ((i) |= (1 << (b)))
#define BIT_CLR(i, b)     ((i) &= ~(1 << (b)))

static DWORD __stdcall pipeRead(void* arg)
{
    HANDLE readEv = CreateEvent(NULL, FALSE, FALSE, NULL);

    while (pipe != INVALID_HANDLE_VALUE) {
        OVERLAPPED ov;
        DWORD read;
        uint8_t buf[3];

        memset(&ov, 0, sizeof(ov));
        ov.hEvent = readEv;
        ReadFile(pipe, buf, sizeof(buf), NULL, &ov);
        if (!GetOverlappedResult(pipe, &ov, &read, TRUE) || (read != 3)) {
            break;
        }
        if (buf[0] == 'r') {
            memcpy(recvBuf, buf, read);
            SetEvent(cond);
        } else if (buf[0] == 'i') {
            /*
             * Record which trigger fired
             */
            BIT_SET(trigSet, buf[1] - 1);
            AJ_Net_Interrupt();
        }
    }
    if (pipe != INVALID_HANDLE_VALUE) {
        AJ_ErrPrintf(("Read failed - closing pipe\n"));
        CloseHandle(pipe);
        pipe = INVALID_HANDLE_VALUE;
    }
    CloseHandle(readEv);
    SetEvent(cond);
    CloseHandle(cond);
    return 0;
}

static AJ_Status WaitForData(uint32_t ms)
{
    DWORD ret;

    ret = WaitForSingleObject(cond, ms);
    ResetEvent(cond);
    if (ret) {
        return ret == WAIT_TIMEOUT ? AJ_ERR_TIMEOUT : AJ_ERR_DRIVER;
    } else {
        return AJ_OK;
    }
}

static void OpenSimIO()
{
    HANDLE thread = INVALID_HANDLE_VALUE;

    pipe = CreateFileA(gui_server, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
    if (pipe == INVALID_HANDLE_VALUE) {
        return;
    }
    writeEv = CreateEvent(NULL, FALSE, FALSE, NULL);
    cond = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (cond != INVALID_HANDLE_VALUE) {
        /*
         * Create thread to handle reads
         */
        thread = CreateThread(NULL, 0, pipeRead, NULL, 0, NULL);
    }
    if (thread == INVALID_HANDLE_VALUE) {
        AJ_ErrPrintf(("Failed to initialize SimIO\n"));
        CloseHandle(pipe);
        CloseHandle(writeEv);
        CloseHandle(cond);
        pipe = INVALID_HANDLE_VALUE;
    } else {
        /*
         * This lets the thread run free
         */
        CloseHandle(thread);
    }
}

static int SendCmd(uint8_t op, GPIO* gpio, uint8_t arg)
{
    if (pipe == INVALID_HANDLE_VALUE) {
        OpenSimIO();
    }
    if (pipe != INVALID_HANDLE_VALUE) {
        OVERLAPPED ov;
        DWORD wrote;
        uint8_t buf[3];
        buf[0] = op;
        buf[1] = (uint8_t)gpio->pinId;
        buf[2] = arg;
        memset(&ov, 0, sizeof(ov));
        ov.hEvent = writeEv;
        WriteFile(pipe, buf, sizeof(buf), NULL, &ov);
        if (!GetOverlappedResult(pipe, &ov, &wrote, TRUE)) {
            AJ_ErrPrintf(("Failed to send cmd - closing pipe\n"));
            CloseHandle(pipe);
            pipe = INVALID_HANDLE_VALUE;
        }
    }
    return pipe != INVALID_HANDLE_VALUE;
}

AJ_Status AJS_TargetIO_PinOpen(uint16_t pin, AJS_IO_PinConfig config, void** pinCtx)
{
    GPIO* gpio;

    AJ_ErrPrintf(("AJS_TargetIO_PinOpen(%d, %02x)\n", pin, config));
    gpio = malloc(sizeof(GPIO));
    gpio->pinId = pin;
    gpio->trigId = AJS_IO_PIN_NO_TRIGGER;
    if (pipe == INVALID_HANDLE_VALUE) {
        OpenSimIO();
    }
    *pinCtx = gpio;
    return AJ_OK;
}

AJ_Status AJS_TargetIO_PinClose(void* pinCtx)
{
    GPIO* gpio = (GPIO*)pinCtx;
    AJ_ErrPrintf(("AJS_TargetIO_PinClose(%d)\n", gpio->pinId));
    free(gpio);
    return AJ_OK;
}

void AJS_TargetIO_PinToggle(void* pinCtx)
{
    GPIO* gpio = (GPIO*)pinCtx;
    if (!SendCmd('t', gpio, 0)) {
        AJ_ErrPrintf(("AJS_TargetIO_PinToggle(%d)\n", gpio->pinId));
    }
}

void AJS_TargetIO_PinSet(void* pinCtx, uint32_t val)
{
    GPIO* gpio = (GPIO*)pinCtx;
    if (!SendCmd('w', gpio, (uint8_t)val)) {
        AJ_ErrPrintf(("AJS_TargetIO_PinSet(%d, %d)\n", gpio->pinId, val));
    }
}

uint32_t AJS_TargetIO_PinGet(void* pinCtx)
{
    GPIO* gpio = (GPIO*)pinCtx;
    if (SendCmd('r', gpio, 0)) {
        if (WaitForData(200) == AJ_OK) {
            if ((recvBuf[0] == 'r') && (recvBuf[1] == (int)gpio->pinId)) {
                return recvBuf[2];
            }
        }
    } else {
        AJ_ErrPrintf(("AJS_TargetIO_PinGet(%d)\n", gpio->pinId));
    }
    return 0;
}

int32_t AJS_TargetIO_PinTrigId()
{
    if (trigSet == 0) {
        return AJS_IO_PIN_NO_TRIGGER;
    } else {
        /*
         * This is static so triggers are returned round-robin to ensure fairness
         */
        static uint32_t id = 0;
        while (!BIT_IS_SET(trigSet, id % MAX_TRIGGERS)) {
            ++id;
        }
        BIT_CLR(trigSet, id % MAX_TRIGGERS);
        return (id % MAX_TRIGGERS);
    }
}

AJ_Status AJS_TargetIO_PinPWM(void* pinCtx, double dutyCycle, uint32_t freq)
{
    return AJ_ERR_DRIVER;
}

AJ_Status AJS_TargetIO_PinEnableTrigger(void* pinCtx, AJS_IO_PinTriggerMode trigger, int32_t* trigId, uint8_t debounce)
{
    GPIO* gpio = (GPIO*)pinCtx;

    if ((trigger != AJS_IO_PIN_TRIGGER_ON_RISE) && (trigger != AJS_IO_PIN_TRIGGER_ON_FALL)) {
        /*
         * Disable triggers for this pin
         */
        if (gpio->trigId != AJS_IO_PIN_NO_TRIGGER) {
            SendCmd('i', gpio, 0);
            *trigId = gpio->trigId;
            BIT_CLR(trigSet, gpio->trigId);
            gpio->trigId = AJS_IO_PIN_NO_TRIGGER;
        } else {
            *trigId = AJS_IO_PIN_NO_TRIGGER;
        }
        return AJ_OK;
    }
    SendCmd('i', gpio, (trigger == AJS_IO_PIN_TRIGGER_ON_RISE) ? 1 : 2);
    gpio->trigId = gpio->pinId;
    *trigId = gpio->trigId;

    AJ_ErrPrintf(("AJS_TargetIO_PinEnableTrigger pinId %d\n", gpio->pinId));
    return AJ_OK;
}

AJ_Status AJS_TargetIO_System(const char* cmd)
{
    int ret = system(cmd);
    if (ret == -1) {
        return AJ_ERR_FAILURE;
    } else {
        return AJ_OK;
    }
}