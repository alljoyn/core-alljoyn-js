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

#define AJ_MODULE GPIO

#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>

#include "ajs.h"
#include "ajs_io.h"

extern uint8_t dbgGPIO;

static const char adc_dev[] = "/sys/bus/iio/devices/iio:device0/";

typedef struct {
    uint8_t addr;
    int fd;
} ADC_CONTEXT;

typedef struct {
    uint8_t physicalPin; /* matches up the pin information in the pin info array */
    const char* dev;     /* device name for the adc */
} PIN_Info;

static const PIN_Info pinInfo[] = {
    {  0xA0, "in_voltage_A0_raw" },
    {  0xA1, "in_voltage_A1_raw" },
    {  0xA2, "in_voltage_A2_raw" },
    {  0xA3, "in_voltage_A3_raw" },
    {  0xA4, "in_voltage_A4_raw" },
    {  0xA5, "in_voltage_A5_raw" },
};

static int OpenDeviceFile(const char* fn, int mode)
{
    int fd;
    char buf[sizeof(adc_dev) + 24];

    snprintf(buf, sizeof(buf), "%s%s", adc_dev, fn);
    fd = open(buf, mode);
    if (fd >= 0) {
        AJ_InfoPrintf(("Opened %s\n", buf));
    } else {
        AJ_ErrPrintf(("Failed to open %s\n", buf));
    }
    return fd;
}

static AJ_Status EnableIfNeeded()
{
    int ret;
    char buf[2];
    int fd = OpenDeviceFile("enable", O_RDWR);
    if (fd < 0) {
        return AJ_ERR_DRIVER;
    }
    ret = pread(fd, buf, sizeof(buf), 0);
    if (ret > 0) {
        if (buf[0] == '0') {
            buf[0] = '1';
            buf[1] = 0;
            ret = write(fd, buf, sizeof(buf));
        }
    }
    close(fd);
    if (ret <= 0) {
        AJ_ErrPrintf(("Unable to enable ADC ret=%d errno=%d\n", ret, errno));
        return AJ_ERR_DRIVER;
    }
    return AJ_OK;
}

AJ_Status AJS_TargetIO_AdcOpen(uint16_t pinIndex, void** adcCtx)
{
    AJ_Status status;
    size_t pin;
    uint16_t physicalPin = AJS_TargetIO_GetInfo(pinIndex)->physicalPin;

    /*
     * Locate ADC associated with the physical pin
     */
    for (pin = 0; pin < ArraySize(pinInfo); ++pin) {
        if (pinInfo[pin].physicalPin == physicalPin) {
            break;
        }
    }
    if (pin >= ArraySize(pinInfo)) {
        return AJ_ERR_INVALID;
    }
    status = EnableIfNeeded();
    if (status == AJ_OK) {
        int fd = OpenDeviceFile(pinInfo[pin].dev, O_RDONLY);
        if (fd <= 0) {
            status = AJ_ERR_DRIVER;
        } else {
            *adcCtx = (void*)fd;
        }
    }
    return status;
}

AJ_Status AJS_TargetIO_AdcClose(void* adcCtx)
{
    int fd = (int)adcCtx;
    close(fd);
    return AJ_OK;
}

uint32_t AJS_TargetIO_AdcRead(void* adcCtx)
{
    char buf[8];
    int fd = (int)adcCtx;
    int ret = pread(fd, buf, sizeof(buf), 0);
    if (ret <= 0) {
        AJ_ErrPrintf(("Unable to read ADC value ret=%d errno=%d\n", ret, errno));
        return 0;
    }
    buf[sizeof(buf) - 1] = 0;
    return strtoul(buf, NULL, 10);
}

