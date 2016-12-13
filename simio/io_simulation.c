/**
 * @file
 */
/******************************************************************************
 * Copyright (c) 2016 Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright 2016 Open Connectivity Foundation and Contributors to
 *    AllSeen Alliance. All rights reserved.
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

#define AJ_MODULE GPIO

#ifdef _WIN32
#include <Windows.h>
#else
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#endif

#include "ajs.h"
#include "ajs_io.h"

/**
 * Controls debug output for this module
 */
#ifndef NDEBUG
uint8_t dbgGPIO;
#endif

extern void AJ_Net_Interrupt();

#ifdef _WIN32

static HANDLE cond;
static HANDLE writeEv;
static HANDLE lock;

#define LOCK(lock)   WaitForSingleObject(lock, INFINITE)
#define UNLOCK(lock) ReleaseMutex(lock)

static HANDLE pipe = INVALID_HANDLE_VALUE;
static const char gui_server[] = "\\\\.\\pipe\\tmp\\ajs_gui";

#else

static pthread_cond_t cond;
static pthread_mutex_t mutex;
static pthread_mutex_t lock;
#define LOCK(lock) pthread_mutex_lock(&lock)
#define UNLOCK(lock) pthread_mutex_unlock(&lock)

static int sock = -1;
static const char gui_server[] = "/tmp/ajs_gui";

#endif

/*
 * Struct for holding the state of a GPIO pin
 */
typedef struct {
    int16_t trigId;
    uint16_t pinId;
} GPIO;

/*
 * Having one buffer for reading IO pin data is fragile but this is not production code
 */
static uint8_t recvBuf[4];
static uint8_t uartBuf[256];
static uint8_t uartPos = 0;
static uint8_t uartLen = 0;

typedef struct {
    GPIO tx;
    GPIO rx;
} UART;

#define MAX_TRIGGERS 32

/*
 * Bit mask of allocated triggers (max 32)
 */
static uint32_t trigSet;

#define BIT_IS_SET(i, b)  ((i) & (1 << (b)))
#define BIT_SET(i, b)     ((i) |= (1 << (b)))
#define BIT_CLR(i, b)     ((i) &= ~(1 << (b)))

static AJS_IO_PinTriggerCondition triggerCondition[MAX_TRIGGERS];

#ifdef _WIN32

static DWORD __stdcall pipeRead(void* arg)
{
    HANDLE readEv = CreateEvent(NULL, FALSE, FALSE, NULL);

    while (pipe != INVALID_HANDLE_VALUE) {
        OVERLAPPED ov;
        DWORD read;
        uint8_t buf[4];

        memset(&ov, 0, sizeof(ov));
        ov.hEvent = readEv;
        ReadFile(pipe, buf, sizeof(buf), NULL, &ov);
        if (!GetOverlappedResult(pipe, &ov, &read, TRUE)) {
            goto exit;
        }
        if (buf[0] == 'r') {
            memcpy(recvBuf, buf, read);
            SetEvent(cond);
        } else if (buf[0] == 'i') {
            size_t idx = buf[1] - 1;
            if (idx < MAX_TRIGGERS) {
                /*
                 * Condition that caused the trigger
                 */
                triggerCondition[idx] = buf[2];
                /*
                 * Check trigger - we might have data to read
                 */
                if (triggerCondition[idx] == AJS_IO_PIN_TRIGGER_ON_RX_READY) {
                    DWORD readLen;
                    DWORD readMax;
                    DWORD sz = buf[3];
                    while (sz) {
                        LOCK(lock);
                        if (uartPos > 0) {
                            /*
                             * Shuffle data to front of the buffer
                             */
                            uartLen -= uartPos;
                            memmove(uartBuf, uartBuf + uartPos, uartLen);
                            uartPos = 0;
                        }
                        readMax = sizeof(uartBuf) - uartLen;
                        UNLOCK(lock);
                        readLen = min(readMax, sz);
                        if (!readLen) {
                            break;
                        }
                        memset(&ov, 0, sizeof(ov));
                        ov.hEvent = readEv;
                        ReadFile(pipe, uartBuf + uartLen, readLen, NULL, &ov);
                        if (!GetOverlappedResult(pipe, &ov, &read, TRUE)) {
                            goto exit;
                        }
                        LOCK(lock);
                        uartLen += (uint8_t)read;
                        UNLOCK(lock);
                        sz -= read;
                    }
                    if (sz) {
                        AJ_ErrPrintf(("UART buffer overflow - discarding %d bytes\n", sz));
                        /*
                         * Discard remaining data
                         */
                        while (sz) {
                            uint8_t tmp[32];
                            memset(&ov, 0, sizeof(ov));
                            ov.hEvent = readEv;
                            ReadFile(pipe, tmp, min(sz, sizeof(tmp)), NULL, &ov);
                            if (!GetOverlappedResult(pipe, &ov, &read, TRUE)) {
                                break;
                            }
                            sz -= read;
                        }
                    }
                }
                /*
                 * Record which trigger fired
                 */
                BIT_SET(trigSet, idx);
                AJ_Net_Interrupt();
            } else {
                AJ_ErrPrintf(("Invalid trigger index %d\n", idx));
            }
        }
    }
exit:
    if (pipe != INVALID_HANDLE_VALUE) {
        AJ_ErrPrintf(("Read failed error=%d - closing pipe\n", GetLastError()));
        CloseHandle(pipe);
        pipe = INVALID_HANDLE_VALUE;
    }
    CloseHandle(readEv);
    SetEvent(cond);
    CloseHandle(cond);
    CloseHandle(lock);
    return 0;
}

static AJ_Status WaitForData(uint32_t ms)
{
    DWORD ret = WaitForSingleObject(cond, ms);
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

    if (pipe != INVALID_HANDLE_VALUE) {
        return;
    }
    pipe = CreateFileA(gui_server, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
    if (pipe == INVALID_HANDLE_VALUE) {
        AJ_ErrPrintf(("Failed to connect to \"%s\"\n", gui_server));
        return;
    }
    AJ_ErrPrintf(("Connected to \"%s\"\n", gui_server));
    lock = CreateMutex(NULL, FALSE, NULL);
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
        CloseHandle(lock);
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

static int SendBytes(uint8_t* data, size_t len)
{
    OpenSimIO();
    if (pipe != INVALID_HANDLE_VALUE) {
        OVERLAPPED ov;
        DWORD wrote;
        memset(&ov, 0, sizeof(ov));
        ov.hEvent = writeEv;
        WriteFile(pipe, data, len, NULL, &ov);
        if (!GetOverlappedResult(pipe, &ov, &wrote, TRUE)) {
            AJ_ErrPrintf(("Failed to send data - closing pipe\n"));
            CloseHandle(pipe);
            pipe = INVALID_HANDLE_VALUE;
        }
    }
    return pipe != INVALID_HANDLE_VALUE;
}

AJ_Status AJS_TargetIO_System(const char* cmd, char* output, uint16_t length)
{
    int ret = system(cmd);
    if (ret == -1) {
        return AJ_ERR_FAILURE;
    } else {
        return AJ_OK;
    }
}

#else

static void* SockRead(void* arg)
{
    while (sock != -1) {
        uint8_t buf[3];
        int ret = recv(sock, buf, sizeof(buf), MSG_WAITALL);
        if (ret != 3) {
            goto exit;
        }
        if (buf[0] == 'r') {
            memcpy(recvBuf, buf, ret);
            pthread_mutex_lock(&mutex);
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mutex);
        } else if (buf[0] == 'i') {
            size_t idx = buf[1] - 1;
            if (idx < MAX_TRIGGERS) {
                /*
                 * Condition that caused the trigger
                 */
                triggerCondition[idx] = buf[2];
                /*
                 * Check trigger - we might have data to read
                 */
                if (triggerCondition[idx] == AJS_IO_PIN_TRIGGER_ON_RX_READY) {
                    size_t readLen;
                    size_t readMax;
                    uint8_t sz;
                    ret = recv(sock, &sz, sizeof(sz), MSG_WAITALL);
                    while (sz) {
                        LOCK(lock);
                        if (uartPos > 0) {
                            /*
                             * Shuffle data to front of the buffer
                             */
                            uartLen -= uartPos;
                            memmove(uartBuf, uartBuf + uartPos, uartLen);
                            uartPos = 0;
                        }
                        readMax = sizeof(uartBuf) - uartLen;
                        UNLOCK(lock);
                        readLen = min(readMax, sz);
                        if (!readLen) {
                            break;
                        }
                        ret = recv(sock, uartBuf + uartLen, readLen, MSG_WAITALL);
                        if (ret != readLen) {
                            goto exit;
                        }
                        LOCK(lock);
                        uartLen += (uint8_t)readLen;
                        UNLOCK(lock);
                        sz -= readLen;
                    }

                    if (sz) {
                        AJ_ErrPrintf(("UART buffer overflow - discarding %d bytes\n", sz));
                        /*
                         * Discard remaining data
                         */
                        while (sz--) {
                            uint8_t tmp;
                            ret = recv(sock, &tmp, 1, MSG_WAITALL);
                        }
                    }
                }
                /*
                 * Record which trigger fired
                 */
                BIT_SET(trigSet, buf[1] - 1);
                AJ_Net_Interrupt();
            } else {
                AJ_ErrPrintf(("Invalid trigger index %d\n", idx));
            }
        }
    }
exit:
    AJ_ErrPrintf(("Recv failed - closing socket\n"));
    close(sock);
    sock = -1;
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&lock);
    return NULL;
}

static AJ_Status WaitForData(uint32_t ms)
{
    struct timespec ts;
    int ret;

#ifdef __MACH__
#else
    clock_gettime(CLOCK_REALTIME, &ts);
#endif
    ts.tv_nsec += (ms % 1000) * 1000000;
    ts.tv_sec += ms / 1000 + ts.tv_nsec / 1000000000;
    ts.tv_nsec %= 1000000000;
    ret = pthread_cond_timedwait(&cond, &mutex, &ts);
    if (ret) {
        AJ_ErrPrintf(("WaitForData ret=%d\n", ret));
        return ret == ETIMEDOUT ? AJ_ERR_TIMEOUT : AJ_ERR_DRIVER;
    }
    return AJ_OK;
}

static void OpenSimIO()
{
    struct sockaddr_un sa;

    if (sock != -1) {
        return;
    }
    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    memcpy(sa.sun_path, gui_server, sizeof(gui_server));

    sock = socket(AF_UNIX,  SOCK_STREAM, 0);
    if (sock >= 0) {
        pthread_t threadId;
        int ret = connect(sock, (struct sockaddr*)&sa, sizeof(sa));
        if (ret == -1) {
            AJ_ErrPrintf(("Failed to connect to I/O gui server\n"));
            close(sock);
            sock = -1;
        }
        /*
         * Create thread to handle reads
         */
        ret = pthread_create(&threadId, NULL, SockRead, NULL);
        if (ret) {
            AJ_ErrPrintf(("Failed to create read thread\n"));
            close(sock);
            sock = -1;
        } else {
            pthread_detach(threadId);
            pthread_cond_init(&cond, NULL);
            pthread_mutex_init(&mutex, NULL);
            pthread_mutex_init(&lock, NULL);
        }
    }
}

static int SendBytes(uint8_t* data, size_t len)
{
    OpenSimIO();
    if (sock != -1) {
        int ret;
        ret = send(sock, data, len, 0);
        if (ret == -1) {
            AJ_ErrPrintf(("Failed to send cmd - closing socket\n"));
            close(sock);
            sock = -1;
        }
    }
    return sock != -1;
}

AJ_Status AJS_TargetIO_System(const char* cmd, char* output, uint16_t length)
{
    FILE* fp = popen(cmd, "r");
    if (fp == NULL) {
        AJ_ErrPrintf(("System call \"%s\" failed\n"));
        return AJ_ERR_FAILURE;
    }
    if (output) {
        fread(output, length, 1, fp);
        output[length - 1] = '\0';
    }
    pclose(fp);
    return AJ_OK;
}
#endif

static int SendCmd(uint8_t op, GPIO* gpio, uint8_t arg1, uint8_t arg2)
{
    uint8_t buf[4];
    buf[0] = op;
    buf[1] = (uint8_t)gpio->pinId;
    buf[2] = arg1;
    buf[3] = arg2;
    return SendBytes(buf, sizeof(buf));
}

AJ_Status AJS_TargetIO_PinOpen(uint16_t pin, AJS_IO_PinConfig config, void** pinCtx)
{
    GPIO* gpio;

    AJ_ErrPrintf(("AJS_TargetIO_PinOpen(%d, %02x)\n", pin, config));
    gpio = malloc(sizeof(GPIO));
    if (!gpio) {
        AJ_ErrPrintf(("AJS_TargetIO_PinOpen(): Malloc failed to allocate %d bytes\n", sizeof(GPIO)));
        return AJ_ERR_RESOURCES;
    }
    gpio->pinId = pin;
    gpio->trigId = AJS_IO_PIN_NO_TRIGGER;
    OpenSimIO();
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
    if (!SendCmd('t', gpio, 0, 0)) {
        AJ_ErrPrintf(("AJS_TargetIO_PinToggle(%d)\n", gpio->pinId));
    }
}

void AJS_TargetIO_PinSet(void* pinCtx, uint32_t val)
{
    GPIO* gpio = (GPIO*)pinCtx;
    if (!SendCmd('w', gpio, (uint8_t)val, 0)) {
        AJ_ErrPrintf(("AJS_TargetIO_PinSet(%d, %d)\n", gpio->pinId, val));
    }
}

uint32_t AJS_TargetIO_PinGet(void* pinCtx)
{
    GPIO* gpio = (GPIO*)pinCtx;
    if (SendCmd('r', gpio, 0, 0)) {
        AJ_Status status = WaitForData(200);
        if (status == AJ_OK) {
            if ((recvBuf[0] == 'r') && (recvBuf[1] == (int)gpio->pinId)) {
                return recvBuf[2];
            }
        } else {
            AJ_ErrPrintf(("AJS_TargetIO_PinGet %s\n", AJ_StatusText(status)));
        }
    } else {
        AJ_ErrPrintf(("AJS_TargetIO_PinGet(%d)\n", gpio->pinId));
    }
    return 0;
}

int32_t AJS_TargetIO_PinTrigId(AJS_IO_PinTriggerCondition* condition)
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
        id %= MAX_TRIGGERS;
        BIT_CLR(trigSet, id);
        *condition = triggerCondition[id];
        return id;
    }
}

AJ_Status AJS_TargetIO_PinPWM(void* pinCtx, double dutyCycle, uint32_t freq)
{
    GPIO* gpio = (GPIO*)pinCtx;
    if (!SendCmd('p', gpio, (uint8_t)freq, (uint8_t)(dutyCycle * 255.0))) {
        AJ_ErrPrintf(("AJS_TargetIO_PinPWM(%d, %d, %d)\n", gpio->pinId, dutyCycle, freq));
        return AJ_ERR_DRIVER;
    }
    return AJ_OK;
}

AJ_Status AJS_TargetIO_PinEnableTrigger(void* pinCtx, int pinFunction, AJS_IO_PinTriggerCondition condition, int32_t* trigId, uint8_t debounce)
{
    GPIO* gpio;

    if (pinFunction == AJS_IO_FUNCTION_UART) {
        UART* uart = (UART*)pinCtx;
        gpio = (condition == AJS_IO_PIN_TRIGGER_ON_TX_READY) ? &uart->tx : &uart->rx;
    } else {
        gpio = (GPIO*)pinCtx;
    }
    AJ_InfoPrintf(("AJS_TargetIO_PinEnableTrigger pinId %d -> %02x\n", gpio->pinId, condition));

    if (gpio->trigId != condition) {
        SendCmd('i', gpio, condition, debounce);
    }
    gpio->trigId = gpio->pinId;
    if ((condition == AJS_IO_PIN_TRIGGER_ON_RX_READY) && uartLen) {
        BIT_SET(trigSet, gpio->trigId);
        triggerCondition[gpio->trigId] = AJS_IO_PIN_TRIGGER_ON_RX_READY;
    }
    if (trigId) {
        *trigId = gpio->trigId;
    }
    return AJ_OK;
}

AJ_Status AJS_TargetIO_PinDisableTrigger(void* pinCtx, int pinFunction, AJS_IO_PinTriggerCondition condition, int32_t* trigId)
{
    GPIO* gpio;

    if (pinFunction == AJS_IO_FUNCTION_UART) {
        UART* uart = (UART*)pinCtx;
        gpio = (condition == AJS_IO_PIN_TRIGGER_ON_TX_READY) ? &uart->tx : &uart->rx;
    } else {
        gpio = (GPIO*)pinCtx;
    }
    AJ_InfoPrintf(("AJS_TargetIO_PinDisableTrigger pinId %d -> %02x\n", gpio->pinId, condition));

    if (gpio->trigId != condition) {
        SendCmd('i', gpio, AJS_IO_PIN_TRIGGER_DISABLE, 0);
    }
    if (trigId) {
        *trigId = gpio->trigId;
    }
    BIT_CLR(trigSet, gpio->trigId);
    gpio->trigId = AJS_IO_PIN_NO_TRIGGER;
    return AJ_OK;
}

AJ_Status AJS_TargetIO_UartOpen(uint8_t txPin, uint8_t rxPin, uint32_t baud, void** uartCtx, int32_t* rxTrigId)
{
    UART* uart;

    uart = malloc(sizeof(UART));
    if (!uart) {
        return AJ_ERR_RESOURCES;
    }
    uart->tx.pinId = txPin;
    uart->rx.pinId = rxPin;
    uartPos = 0;
    uartLen = 0;
    *uartCtx = uart;
    return AJ_OK;
}

AJ_Status AJS_TargetIO_UartClose(void* uartCtx)
{
    free(uartCtx);
    return AJ_OK;
}

uint32_t AJS_TargetIO_UartRead(void* uartCtx, uint8_t* buffer, uint32_t length)
{
    UART* uart = (UART*)uartCtx;

    AJ_InfoPrintf(("AJS_TargetIO_UartRead()"));
    LOCK(lock);
    AJ_ASSERT(uartLen >= uartPos);
    length = min(length, (uint32_t)(uartLen - uartPos));
    memcpy(buffer, uartBuf + uartPos, length);
    /*
     * Consume data we copied
     */
    uartPos += length;
    if (uartLen > uartPos) {
        /*
         * There is more data in the buffer so retrigger
         */
        BIT_SET(trigSet, uart->rx.trigId);
        triggerCondition[uart->rx.trigId] = AJS_IO_PIN_TRIGGER_ON_RX_READY;
    }
    UNLOCK(lock);
    return length;
}

AJ_Status AJS_TargetIO_UartWrite(void* uartCtx, uint8_t* buffer, uint32_t length)
{
    UART* uart = (UART*)uartCtx;

    while (length--) {
        if (!SendCmd('w', &uart->tx, *buffer++, 0)) {
            AJ_ErrPrintf(("AJS_TargetIO_UartWrite\n"));
            return AJ_ERR_FAILURE;
        }
    }
    if (!SendCmd('w', &uart->tx, '\n', 0)) {
        return AJ_ERR_FAILURE;
    }
    return AJ_OK;
}