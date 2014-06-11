/**
 * @file
 */
/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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

#define AJ_MODULE GPIO

#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>

#include "../../ajs.h"
#include "../../ajs_io.h"

/**
 * Controls debug output for this module
 */
#ifndef NDEBUG
uint8_t dbgGPIO;
#endif

extern void AJ_Net_Interrupt();

static int sock = -1;

static const char gui_server[] = "/tmp/ajs_gui";

/*
 * Struct for holding the state of a GPIO pin
 */
typedef struct {
    int8_t trigId;
    uint8_t pinId;
} GPIO;

static pthread_cond_t cond;
static pthread_mutex_t mutex;
static uint8_t recvBuf[3];

#define MAX_TRIGGERS 32

/*
 * Bit mask of allocated triggers (max 32)
 */
static uint32_t trigSet;

#define BIT_IS_SET(i, b)  ((i)& (1 << (b)))
#define BIT_SET(i, b)     ((i) |= (1 << (b)))
#define BIT_CLR(i, b)     ((i) &= ~(1 << (b)))

static void* SockRead(void* arg)
{
    while (sock != -1) {
        uint8_t buf[3];
        int ret = recv(sock, buf, sizeof(buf), MSG_WAITALL);
        if (ret != 3) {
            AJ_ErrPrintf(("Recv failed - closing socket\n"));
            close(sock);
            sock = -1;
            break;
        }
        if (buf[0] == 'r') {
            memcpy(recvBuf, buf, ret);
            pthread_mutex_lock(&mutex);
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mutex);
        } else if (buf[0] == 'i') {
            /*
             * Record which trigger fired
             */
            BIT_SET(trigSet, buf[1] - 1);
            AJ_Net_Interrupt();
        }
    }
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
    return NULL;
}

static AJ_Status WaitForData(uint32_t ms)
{
    struct timespec ts;
    int ret;

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += ms / 1000;
    ts.tv_nsec += (ms % 1000) * 1000000;

    pthread_mutex_lock(&mutex);
    ret = pthread_cond_timedwait(&cond, &mutex, &ts);
    pthread_mutex_unlock(&mutex);
    if (ret) {
        return ret == ETIMEDOUT ? AJ_ERR_TIMEOUT : AJ_ERR_DRIVER;
    } else {
        return AJ_OK;
    }
}

static void OpenSimIO()
{
    pthread_attr_t attr;
    struct sockaddr_un sa;

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
        pthread_attr_init(&attr);
        ret = pthread_create(&threadId, NULL, SockRead, NULL);
        if (ret) {
            AJ_ErrPrintf(("Failed to create read thread\n"));
            close(sock);
            sock = -1;
        } else {
            pthread_detach(threadId);
            pthread_cond_init(&cond, NULL);
            pthread_mutex_init(&mutex, NULL);
        }
    }
}

static int SendCmd(uint8_t op, GPIO* gpio, uint8_t arg1, uint8_t arg2)
{
    if (sock == -1) {
        OpenSimIO();
    }
    if (sock != -1) {
        int ret;
        uint8_t buf[4];
        buf[0] = op;
        buf[1] = gpio->pinId;
        buf[2] = arg1;
        buf[3] = arg2;
        ret = send(sock, buf, sizeof(buf), 0);
        if (ret == -1) {
            AJ_ErrPrintf(("Failed to send cmd - closing socket\n"));
            close(sock);
            sock = -1;
        }
    }
    return sock != -1;
}

AJ_Status AJS_TargetIO_PinOpen(uint16_t pin, AJS_IO_PinConfig config, void** pinCtx)
{
    GPIO* gpio;

    AJ_ErrPrintf(("AJS_TargetIO_PinOpen(%d, %02x)\n", pin, config));
    gpio = malloc(sizeof(GPIO));
    gpio->pinId = pin;
    gpio->trigId = AJS_IO_PIN_NO_TRIGGER;
    if (sock == -1) {
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

int32_t AJS_TargetIO_PinTrigId(uint32_t* level)
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
    GPIO* gpio = (GPIO*)pinCtx;
    if (!SendCmd('p', gpio, (uint8_t)freq, (uint8_t)(dutyCycle * 255.0))) {
        AJ_ErrPrintf(("AJS_TargetIO_PinPWM(%d, %f, %d)\n", gpio->pinId, dutyCycle, freq));
        return AJ_ERR_DRIVER;
    }
    return AJ_OK;
}

AJ_Status AJS_TargetIO_PinEnableTrigger(void* pinCtx, AJS_IO_PinTriggerMode trigger, int32_t* trigId, uint8_t debounce)
{
    GPIO* gpio = (GPIO*)pinCtx;

    if ((trigger != AJS_IO_PIN_TRIGGER_ON_RISE) && (trigger != AJS_IO_PIN_TRIGGER_ON_FALL)) {
        /*
         * Disable triggers for this pin
         */
        if (gpio->trigId != AJS_IO_PIN_NO_TRIGGER) {
            SendCmd('i', gpio, 0, 0);
            *trigId = gpio->trigId;
            BIT_CLR(trigSet, gpio->trigId);
            gpio->trigId = AJS_IO_PIN_NO_TRIGGER;
        } else {
            *trigId = AJS_IO_PIN_NO_TRIGGER;
        }
        return AJ_OK;
    }
    SendCmd('i', gpio, (trigger == AJS_IO_PIN_TRIGGER_ON_RISE) ? 1 : 2, 0);
    gpio->trigId = gpio->pinId;
    *trigId = gpio->trigId;

    AJ_ErrPrintf(("AJS_TargetIO_PinEnableTrigger pinId %d\n", gpio->pinId));
    return AJ_OK;
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
