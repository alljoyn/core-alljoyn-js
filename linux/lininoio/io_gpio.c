/**
 * @file
 */
/******************************************************************************
 *  * Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
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

#define AJ_MODULE GPIO

#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/eventfd.h>
#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>

#include "ajs.h"
#include "ajs_io.h"

/**
 * Controls debug output for this module
 */
#ifndef NDEBUG
uint8_t dbgGPIO;
#endif

extern void AJ_Net_Interrupt();

static const char gpio_root[] = "/sys/class/gpio/";
static const char pwm_root[] = "/sys/class/pwm/pwmchip0/";

static pthread_mutex_t mutex;

/*
 * Struct for holding the state of a GPIO pin
 */
typedef struct {
    int8_t trigId;
    uint8_t pinId;
    uint8_t value;
    uint8_t debounceMillis;
    AJ_Time timer;
    uint32_t pwmPeriod;
    int fd;
} GPIO;

#define MAX_TRIGGERS 16

/*
 * Bit mask of allocated triggers and levels (max 32)
 */
static uint32_t trigSet;
static uint32_t trigLevel;

#define BIT_IS_SET(i, b)  ((i) & (1 << (b)))
#define BIT_SET(i, b)     ((i) |= (1 << (b)))
#define BIT_CLR(i, b)     ((i) &= ~(1 << (b)))

typedef struct {
    uint8_t physicalPin; /* matches up the pin information in the pin info array */
    uint8_t gpioId;      /* id used to export the GPIO function of this pin */
    int8_t pwmId;        /* id used to export the PWM function of this pin */
    const char* gpioDev; /* exported device name for the GPIO function of this pin */
} PIN_Info;

static const PIN_Info pinInfo[] = {
    {  2, 117, -1, "D2" },
    {  3, 116,  0, "D3" },
    {  4, 120, -1, "D4" },
    {  5, 114,  4, "D5" },
    {  6, 123,  5, "D6" },
    {  8, 104, -1, "D8" },
    {  9, 105,  1, "D9" },
    { 10, 106,  2, "D10" },
    { 11, 107,  3, "D11" },
    { 12, 122, -1, "D12" },
    { 13, 115, -1, "D13" },
};

static GPIO* triggers[MAX_TRIGGERS];

/*
 * This is an eventFd for interrupting the trigger thread so the trigger list can be recomputed.
 */
static int resetFd = -1;

static void* TriggerThread(void* arg)
{
    fd_set readFds;
    fd_set exceptFds;

    resetFd = eventfd(0, O_NONBLOCK);  // Use O_NONBLOCK instead of EFD_NONBLOCK due to bug in OpenWrt's uCLibc

    FD_ZERO(&readFds);

    AJ_InfoPrintf(("Started TriggerThread\n"));

    while (TRUE) {
        int ret;
        int i;
        int maxFd = resetFd;

        FD_SET(resetFd, &readFds);
        /*
         * Initialize the except FDs from the trigger list
         */
        FD_ZERO(&exceptFds);
        for (i = 0; i < MAX_TRIGGERS; ++i) {
            if (triggers[i]) {
                FD_SET(triggers[i]->fd, &exceptFds);
                maxFd = max(maxFd, triggers[i]->fd);
            }
        }
        ret = select(maxFd + 1, &readFds, NULL, &exceptFds, NULL);
        if (ret < 0) {
            AJ_ErrPrintf(("Error: select returned %d\n", ret));
            break;
        }
        /*
         * Need to clear the reset event by reading from the file descriptor.
         */
        if (FD_ISSET(resetFd, &readFds)) {
            uint64_t u64;
            read(resetFd, &u64, sizeof(u64));
        }
        /*
         * Figure out which GPIOs were triggered
         */
        pthread_mutex_lock(&mutex);
        for (i = 0; i < MAX_TRIGGERS; ++i) {
            GPIO* gpio = triggers[i];
            if (gpio && FD_ISSET(gpio->fd, &exceptFds)) {
                char buf[2];
                /*
                 * Consume the value
                 */
                pread(gpio->fd, buf, sizeof(buf), 0);
                /*
                 *
                 */
                if (gpio->debounceMillis) {
                    if (!BIT_IS_SET(trigSet, i)) {
                        AJ_InitTimer(&gpio->timer);
                    } else if (AJ_GetElapsedTime(&gpio->timer, TRUE) < gpio->debounceMillis) {
                        continue;
                    }
                }
                AJ_InfoPrintf(("Trigger on pin %d\n", gpio->pinId));
                /*
                 * Record which trigger fired and the leve
                 */
                BIT_SET(trigSet, i);
                if (buf[0] == '0') {
                    BIT_CLR(trigLevel, i);
                } else {
                    BIT_SET(trigLevel, i);
                }
            }
        }
        pthread_mutex_unlock(&mutex);
        if (trigSet) {
            AJ_Net_Interrupt();
        }
    }
    close(resetFd);
    pthread_mutex_destroy(&mutex);
    resetFd = -1;

    return NULL;
}

static void ResetTriggerThread()
{
    if (resetFd != -1) {
        uint64_t u64;
        write(resetFd, &u64, sizeof(u64));
    }
}

static void InitTriggerThread()
{
    if (resetFd == -1) {
        int ret;
        pthread_t threadId;

        pthread_mutex_init(&mutex, NULL);
        ret = pthread_create(&threadId, NULL, TriggerThread, NULL);
        if (ret) {
            AJ_ErrPrintf(("Failed to create trigger thread\n"));
        } else {
            pthread_detach(threadId);
        }
    }
}

static AJ_Status ExportIfNeeded(const char* dev, int deviceId, const char* root)
{
    int fd;
    DIR* dir;
    char buf[256];

    snprintf(buf, sizeof(buf), "%s%s", root, dev);
    dir = opendir(buf);
    if (dir) {
        closedir(dir);
        return AJ_OK;
    }
    if (errno != ENOENT) {
        AJ_ErrPrintf(("Failed opendir %d %s\n", errno, buf));
        return AJ_ERR_INVALID;
    }
    /*
     * Try to create the device entry
     */
    snprintf(buf, sizeof(buf), "%s%s", root, "export");
    fd = open(buf, O_WRONLY);
    if (fd >= 0) {
        size_t sz = snprintf(buf, sizeof(buf), "%d", deviceId);
        write(fd, buf, sz + 1);
        close(fd);
        return AJ_OK;
    } else {
        AJ_ErrPrintf(("Failed to open %s\n", buf));
        return AJ_ERR_INVALID;
    }
}

static int OpenDeviceProp(const char* dev, const char* root, const char* prop, int mode)
{
    int fd;
    char buf[256];

    snprintf(buf, sizeof(buf), "%s%s/%s", root, dev, prop);
    fd = open(buf, mode);
    if (fd >= 0) {
        AJ_InfoPrintf(("Opened %s\n", buf));
    } else {
        AJ_ErrPrintf(("Failed to open %s\n", buf));
    }
    return fd;
}

static AJ_Status SetDeviceProp(const char* dev, const char* root, const char* prop, const char* val)
{
    AJ_Status status = AJ_OK;
    size_t sz = strlen(val) + 1;
    int fd = OpenDeviceProp(dev, root, prop, O_WRONLY);
    if (fd < 0) {
        return AJ_ERR_DRIVER;
    }
    if (write(fd, val, sz) != sz) {
        AJ_ErrPrintf(("Failed to write prop %s to %s value=%s\n", prop, dev, val));
        status = AJ_ERR_DRIVER;
    }
    close(fd);
    AJ_InfoPrintf(("Set device %s prop %s to %s\n", dev, prop, val));
    return status;
}

static AJ_Status GetDeviceProp(const char* dev, const char* root, const char* prop, char* buf, size_t* bufSz)
{
    AJ_Status status = AJ_OK;

    int fd = OpenDeviceProp(dev, root, prop, O_RDONLY);
    if (fd < 0) {
        return AJ_ERR_DRIVER;
    }
    *bufSz = read(fd, buf, *bufSz);
    if (*bufSz <= 0) {
        AJ_ErrPrintf(("Failed to read prop %s from %s\n", dev, prop));
        status = AJ_ERR_DRIVER;
    }
    close(fd);
    AJ_InfoPrintf(("Got device %s prop %s = %s\n", dev, prop, buf));
    return status;
}

AJ_Status AJS_TargetIO_PinOpen(uint16_t pinIndex, AJS_IO_PinConfig config, void** pinCtx)
{
    AJ_Status status = AJ_OK;
    int fd;
    uint16_t physicalPin = AJS_TargetIO_GetInfo(pinIndex)->physicalPin;
    size_t pin;
    GPIO* gpio;
    const char* dev;

    /*
     * Locate GPIO associated with the physical pin
     */
    for (pin = 0; pin < ArraySize(pinInfo); ++pin) {
        if (pinInfo[pin].physicalPin == physicalPin) {
            break;
        }
    }
    if (pin >= ArraySize(pinInfo)) {
        return AJ_ERR_INVALID;
    }
    dev = pinInfo[pin].gpioDev;
    status = ExportIfNeeded(dev, pinInfo[pin].gpioId, gpio_root);
    if (status != AJ_OK) {
        return status;
    }
    /*
     * Set the pin direction
     */
    status = SetDeviceProp(dev, gpio_root, "direction", (config == AJS_IO_PIN_OUTPUT) ? "out" : "in");
    if (status != AJ_OK) {
        return status;
    }
    /*
     * Save the open file handle
     */
    fd = OpenDeviceProp(dev, gpio_root, "value", (config == AJS_IO_PIN_OUTPUT) ? O_RDWR : O_RDONLY);
    if (fd >= 0) {
        gpio = malloc(sizeof(GPIO));
        if (!gpio) {
            AJ_ErrPrintf(("AJS_TargetIO_PinOpen(): Malloc failed to allocate %d bytes\n", sizeof(GPIO)));
            return AJ_ERR_RESOURCES;
        }
        gpio->fd = fd;
        gpio->pinId = pin;
        gpio->trigId = AJS_IO_PIN_NO_TRIGGER;
        gpio->pwmPeriod = 0;
        AJS_TargetIO_PinGet(gpio);
    } else {
        status = AJ_ERR_DRIVER;
    }
    *pinCtx = gpio;
    return status;
}

AJ_Status AJS_TargetIO_PinClose(void* pinCtx)
{
    GPIO* gpio = (GPIO*)pinCtx;
    AJ_InfoPrintf(("AJS_TargetIO_PinClose(%d)\n", gpio->pinId));
    /*
     * Might need to remove the pin from the triggers list
     */
    if (gpio->trigId != AJS_IO_PIN_NO_TRIGGER) {
        pthread_mutex_lock(&mutex);
        triggers[gpio->trigId] = NULL;
        pthread_mutex_unlock(&mutex);
    }
    close(gpio->fd);
    free(gpio);
    return AJ_OK;
}

void AJS_TargetIO_PinToggle(void* pinCtx)
{
    GPIO* gpio = (GPIO*)pinCtx;
    AJS_TargetIO_PinSet(pinCtx, !gpio->value);
}

void AJS_TargetIO_PinSet(void* pinCtx, uint32_t val)
{
    GPIO* gpio = (GPIO*)pinCtx;

    /*
     * Setting an explicit value disables PWM if it was enabled
     */
    if (gpio->pwmPeriod != 0) {
        (void)SetDeviceProp(pinInfo[gpio->pinId].gpioDev, pwm_root, "enable", "0");
        (void)SetDeviceProp(pinInfo[gpio->pinId].gpioDev, gpio_root, "direction", "out");
        gpio->pwmPeriod = 0;
    }
    if (write(gpio->fd, val ? "1" : "0", 2) != 2) {
        AJ_ErrPrintf(("AJS_TargetIO_PinSet write unsuccessful errno=%d\n", errno));
    } else {
        gpio->value = val ? 1 : 0;
    }
}

uint32_t AJS_TargetIO_PinGet(void* pinCtx)
{
    int ret;
    GPIO* gpio = (GPIO*)pinCtx;
    char buf[2];

    ret = pread(gpio->fd, buf, sizeof(buf), 0);
    if (ret <= 0) {
        AJ_ErrPrintf(("AJS_TargetIO_PinGet unsuccessful ret=%d errno=%d\n", ret, errno));
    } else {
        gpio->value = (buf[0] == '1');
    }
    return gpio->value;
}

int32_t AJS_TargetIO_PinTrigId(uint32_t* level)
{
    pthread_mutex_lock(&mutex);
    if (trigSet == 0) {
        pthread_mutex_unlock(&mutex);
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
        *level = BIT_IS_SET(trigLevel, id % MAX_TRIGGERS);
        pthread_mutex_unlock(&mutex);
        return (id % MAX_TRIGGERS);
    }
}

AJ_Status AJS_TargetIO_PinPWM(void* pinCtx, double dutyCycle, uint32_t freq)
{
    AJ_Status status;
    GPIO* gpio = (GPIO*)pinCtx;
    int8_t pwmId = pinInfo[gpio->pinId].pwmId;
    uint32_t period;
    uint32_t dutyNS;
    const char* dev;
    char val[16];

    AJ_InfoPrintf(("AJS_TargetIO_PinPWM(%d, %f, %d)\n", gpio->pinId, dutyCycle, freq));

    if (pwmId < 0) {
        return AJ_ERR_INVALID;
    }
    dev = pinInfo[gpio->pinId].gpioDev;
    /*
     * Handle limit cases
     */
    if (dutyCycle == 0.0) {
        AJS_TargetIO_PinSet(pinCtx, 0);
        return AJ_OK;
    }
    if (dutyCycle == 1.0) {
        AJS_TargetIO_PinSet(pinCtx, 1);
        return AJ_OK;
    }
    /*
     * Check if we need to enable PWM
     */
    if (gpio->pwmPeriod == 0) {
        status = ExportIfNeeded(dev, pwmId, pwm_root);
        if (status != AJ_OK) {
            return status;
        }
        status = SetDeviceProp(dev, pwm_root, "enable", "1");
        if (status != AJ_OK) {
            return status;
        }
    }
    if (freq == 0) {
        /*
         * If we don't already have a value for period read the default from the device
         */
        if (gpio->pwmPeriod) {
            period = gpio->pwmPeriod;
        } else {
            char buf[16];
            size_t sz = sizeof(buf) - 1;
            status = GetDeviceProp(dev, pwm_root, "period", buf, &sz);
            if (status != AJ_OK) {
                return status;
            }
            buf[sz] = 0;
            period = strtoul(buf, NULL, 10);
            gpio->pwmPeriod = period;
        }
    } else {
        /*
         * Convert frequency to period time in nanoseconds
         */
        period = 1000000000 / freq;
        /*
         * Check if the period needs to be updated
         */
        if (period != gpio->pwmPeriod) {
            snprintf(val, sizeof(val), "%u", period);
            status = SetDeviceProp(dev, pwm_root, "period", val);
            if (status != AJ_OK) {
                return status;
            }
            gpio->pwmPeriod = period;
        }
    }
    dutyNS = (uint32_t)((double)period * dutyCycle);
    AJ_InfoPrintf(("period = %u duty_time = %u\n", period, dutyNS));
    snprintf(val, sizeof(val), "%u", dutyNS);
    return SetDeviceProp(dev, pwm_root, "duty_cycle", val);
}

AJ_Status AJS_TargetIO_PinDisableTrigger(void* pinCtx, int pinFunction, AJS_IO_PinTriggerCondition condition, int32_t* trigId)
{
    AJ_Status status;
    GPIO* gpio = (GPIO*)pinCtx;
    const char* dev = pinInfo[gpio->pinId].gpioDev;

    if (trigId) {
        *trigId = gpio->trigId;
    }
    status = SetDeviceProp(dev, gpio_root, "edge", "none");
    if (gpio->trigId != AJS_IO_PIN_NO_TRIGGER) {
        pthread_mutex_lock(&mutex);
        AJ_ASSERT(gpio->trigId < MAX_TRIGGERS);
        triggers[gpio->trigId] = NULL;
        BIT_CLR(trigSet, gpio->trigId);
        gpio->trigId = AJS_IO_PIN_NO_TRIGGER;
        pthread_mutex_unlock(&mutex);
        /*
         * Let the trigger thread know there has been a change
         */
        ResetTriggerThread();
    }
    return status;
}

AJ_Status AJS_TargetIO_PinEnableTrigger(void* pinCtx, int pinFunction, AJS_IO_PinTriggerCondition condition, int32_t* trigId, uint8_t debounce)
{
    int i;
    AJ_Status status;
    GPIO* gpio = (GPIO*)pinCtx;
    const char* dev = pinInfo[gpio->pinId].gpioDev;
    const char* val;

    InitTriggerThread();

    if (condition == AJS_IO_PIN_TRIGGER_ON_RISE) {
        val = "rising";
    } else if (condition == AJS_IO_PIN_TRIGGER_ON_FALL) {
        val = "falling";
    } else {
        val = "both";
    }
    status = SetDeviceProp(dev, gpio_root, "edge", val);
    if (status == AJ_OK) {
        pthread_mutex_lock(&mutex);
        for (i = 0; i < MAX_TRIGGERS; ++i) {
            if (!triggers[i]) {
                triggers[i] = gpio;
                gpio->trigId = i;
                break;
            }
        }
        gpio->debounceMillis = debounce;
        AJ_InitTimer(&gpio->timer);
        pthread_mutex_unlock(&mutex);
        if (i == MAX_TRIGGERS) {
            (void)SetDeviceProp(dev, gpio_root, "edge", "node");
            status = AJ_ERR_RESOURCES;
        }
        *trigId = gpio->trigId;
    }
    if (status == AJ_OK) {
        /*
         * Let the trigger thread know there has been a change
         */
        ResetTriggerThread();
        AJ_InfoPrintf(("AJS_TargetIO_PinEnableTrigger pinId %d\n", gpio->pinId));
    }
    return status;
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