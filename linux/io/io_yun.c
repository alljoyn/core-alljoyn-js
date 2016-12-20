/**
 * @file
 */
/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
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
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "ajs.h"
#include "ajs_io.h"

extern void AJ_Net_Interrupt();


#define SMSG_MAX_MSG_LEN 31
#define RD_TO 500
#define BAUDRATE B230400


#define TTY_DEV "/dev/ttyATH0"

typedef struct {
    uint16_t sum;
    uint8_t pos;
} CheckSum;

static void CheckSumInit(CheckSum*chksum) {
    chksum->sum = 0; chksum->pos = 0;
}
static void AddByte(CheckSum*chksum, uint8_t b) {
    chksum->sum += (++chksum->pos * b);
}
static uint8_t GetSumMSB(const CheckSum*chksum) {
    return (chksum->sum >> 8) & 0xff;
}
static uint8_t GetSumLSB(const CheckSum*chksum) {
    return chksum->sum & 0xff;
}


static int WaitForMsg(int fd, uint32_t timeout)
{
    fd_set rfds;
    struct timeval to = { 0, timeout };
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    return (select(fd + 1, &rfds, NULL, NULL, &to) > 0);
}


static int ReadByte(int fd, uint8_t* buf)
{
    if (WaitForMsg(fd, RD_TO)) {
        ssize_t ret = read(fd, buf, 1);
        if (ret > 0) {
            //printf(" r%02x", *buf); fflush(stdout);
            return 1;
        }
    }
    return 0;
}


static void FlushRead(int fd)
{
    uint8_t buf;

    while (WaitForMsg(fd, RD_TO)) {
        if (fd > 0) {
            ssize_t ret = read(fd, &buf, 1);
            ret++; // suppress compiler warning.
            //printf(" f%02x", buf); fflush(stdout);
        }
    }
}


static int ReadMsg(int fd, uint8_t* buf, uint8_t len)
{
    uint8_t plen;
    uint8_t psumbuf;
    CheckSum sum;
    uint8_t i;

    CheckSumInit(&sum);
    if (!ReadByte(fd, &plen)) {
        return 0;
    }
    if ((plen > SMSG_MAX_MSG_LEN) || (plen > len)) {
        return -1;
    }
    AddByte(&sum, plen);

    for (i = 0; i < plen; ++i) {
        if (!ReadByte(fd, &buf[i])) {
            return -1;
        }
        AddByte(&sum, buf[i]);
    }

    if (!ReadByte(fd, &psumbuf)) {
        return -1;
    }
    if (GetSumMSB(&sum) != psumbuf) {
        return -1;
    }

    if (!ReadByte(fd, &psumbuf)) {
        return -1;
    }
    if (GetSumLSB(&sum) != psumbuf) {
        return -1;
    }

    FlushRead(fd);

    return plen;
}


static int WriteByte(int fd, const uint8_t buf)
{
    if (fd > 0) {
        //printf(" w%02x", buf); fflush(stdout);
        ssize_t ret = write(fd, &buf, 1);
        return (ret > 0);
    }
    return 0;
}


static int WriteMsg(int fd, const uint8_t* buf, uint8_t len)
{
    CheckSum sum;
    uint8_t sumbuf;
    uint8_t i;

    CheckSumInit(&sum);
    if ((len < 1) || (len > SMSG_MAX_MSG_LEN)) {
        return -1;
    }

    if (!WriteByte(fd, len)) {
        return -1;
    }
    AddByte(&sum, len);

    for (i = 0; i < len; ++i) {
        if (!WriteByte(fd, buf[i])) {
            return -3;
        }
        AddByte(&sum, buf[i]);
    }

    sumbuf = GetSumMSB(&sum);
    if (!WriteByte(fd, sumbuf)) {
        return -4;
    }

    sumbuf = GetSumLSB(&sum);
    if (!WriteByte(fd, sumbuf)) {
        return -5;
    }

    usleep(RD_TO * 2);

    return len;
}


/***********************************************************/

/*
 * Bit mask of allocated triggers
 */
static uint32_t trigSet;
static uint32_t trigMode;

#define BIT_IS_SET(i, b)  ((i) & (1 << (b)))
#define BIT_SET(i, b)     ((i) |= (1 << (b)))
#define BIT_CLR(i, b)     ((i) &= ~(1 << (b)))


#define PIN_CONFIG_DIGITAL 0x00
#define PIN_CONFIG_ANALOG  0x10
#define PIN_CONFIG_MASK    0x1f

/*
 * Some of the Arduino Yun digital I/O pins support PWM output.  We
 * can't control the frequency of the PWM; only the duty cycle.
 * Therefore, we'll use the analog output pin function specifier since
 * that API matches up with the functionality provided.
 *
 * All the analog input pins can be operated as digital I/O pins too.
 */
static const AJS_IO_Info info[] = {
    {                              AJS_IO_FUNCTION_DIGITAL_IO,    0, "D0",  "",  "Digital Pin 0"  },
    {                              AJS_IO_FUNCTION_DIGITAL_IO,    1, "D1",  "",  "Digital Pin 1"  },
    {                              AJS_IO_FUNCTION_DIGITAL_IO,    2, "D2",  "",  "Digital Pin 2"  },
    { AJS_IO_FUNCTION_ANALOG_OUT | AJS_IO_FUNCTION_DIGITAL_IO,    3, "D3",  "",  "Digital Pin 3"  },
    {                              AJS_IO_FUNCTION_DIGITAL_IO,    4, "D4",  "",  "Digital Pin 4"  },
    { AJS_IO_FUNCTION_ANALOG_OUT | AJS_IO_FUNCTION_DIGITAL_IO,    5, "D5",  "",  "Digital Pin 5"  },
    { AJS_IO_FUNCTION_ANALOG_OUT | AJS_IO_FUNCTION_DIGITAL_IO,    6, "D6",  "",  "Digital Pin 6"  },
    {                              AJS_IO_FUNCTION_DIGITAL_IO,    7, "D7",  "",  "Digital Pin 7"  },
    {                              AJS_IO_FUNCTION_DIGITAL_IO,    8, "D8",  "",  "Digital Pin 8"  },
    { AJS_IO_FUNCTION_ANALOG_OUT | AJS_IO_FUNCTION_DIGITAL_IO,    9, "D9",  "",  "Digital Pin 9"  },
    { AJS_IO_FUNCTION_ANALOG_OUT | AJS_IO_FUNCTION_DIGITAL_IO,   10, "D10", "",  "Digital Pin 10" },
    { AJS_IO_FUNCTION_ANALOG_OUT | AJS_IO_FUNCTION_DIGITAL_IO,   11, "D11", "",  "Digital Pin 11" },
    {                              AJS_IO_FUNCTION_DIGITAL_IO,   12, "D12", "",  "Digital Pin 12" },
    {                              AJS_IO_FUNCTION_DIGITAL_IO,   13, "D13", "",  "Digital Pin 13" },

    { AJS_IO_FUNCTION_ANALOG_IN  | AJS_IO_FUNCTION_DIGITAL_IO, 0xA0, "A0",  "",  "Analog Pin 0"   },
    { AJS_IO_FUNCTION_ANALOG_IN  | AJS_IO_FUNCTION_DIGITAL_IO, 0xA1, "A1",  "",  "Analog Pin 1"   },
    { AJS_IO_FUNCTION_ANALOG_IN  | AJS_IO_FUNCTION_DIGITAL_IO, 0xA2, "A2",  "",  "Analog Pin 2"   },
    { AJS_IO_FUNCTION_ANALOG_IN  | AJS_IO_FUNCTION_DIGITAL_IO, 0xA3, "A3",  "",  "Analog Pin 3"   },
    { AJS_IO_FUNCTION_ANALOG_IN  | AJS_IO_FUNCTION_DIGITAL_IO, 0xA4, "A4",  "",  "Analog Pin 4"   },
    { AJS_IO_FUNCTION_ANALOG_IN  | AJS_IO_FUNCTION_DIGITAL_IO, 0xA5, "A5",  "",  "Analog Pin 5"   }
};

static int arduinoFd = -1;

/*
 * Struct for holding the state of a GPIO pin
 */
typedef struct {
    uint8_t trigMode;
    uint16_t pinId;
} GPIO;

typedef enum {
    RS_IDLE,
    RS_WAITING,
    RS_TIMEDOUT,
    RS_DONE
} ReadState;

static pthread_t threadId;
static pthread_mutex_t mutex;
static pthread_cond_t cond;
static uint8_t recvBuf[4];
static ReadState readState = RS_IDLE;
static struct timespec readTO;

static void* ReadThread(void* context)
{
    int fd;

    pthread_mutex_lock(&mutex);
    fd = arduinoFd;
    pthread_mutex_unlock(&mutex);

    while (fd != -1) {
        uint8_t buf[4];
        int ret;

        while (WaitForMsg(fd, 10000) == 0) {
            pthread_mutex_lock(&mutex);
            if (readState == RS_WAITING) {
                struct timespec now;
                clock_gettime(CLOCK_MONOTONIC, &now);
                if ((now.tv_sec > readTO.tv_sec) ||
                    ((now.tv_sec == readTO.tv_sec) && (now.tv_nsec > readTO.tv_nsec))) {
                    readState = RS_TIMEDOUT;
                    pthread_cond_signal(&cond);
                }
            }
            pthread_mutex_unlock(&mutex);
        }
        ret = ReadMsg(fd, buf, sizeof(buf));
        if (ret == sizeof(buf)) {
            if (buf[0] == 'r') {
                pthread_mutex_lock(&mutex);
                memcpy(recvBuf, buf, ret);
                readState = RS_DONE;
                pthread_cond_signal(&cond);
                pthread_mutex_unlock(&mutex);
            } else if (buf[0] == 'i') {
                /*
                 * Record which trigger fired
                 */
                uint8_t pin;
                for (pin = 0; pin < ArraySize(info); ++pin) {
                    if (info[pin].physicalPin == buf[1]) {
                        break;
                    }
                }
                if (pin < ArraySize(info)) {
                    pthread_mutex_lock(&mutex);
                    BIT_SET(trigSet, pin);
                    if (buf[3] == AJS_IO_PIN_TRIGGER_ON_FALL) {
                        BIT_SET(trigMode, pin);
                    } else {
                        BIT_CLR(trigMode, pin);
                    }
                    pthread_mutex_unlock(&mutex);
                    AJ_Net_Interrupt();
                }
            }
        }
        pthread_mutex_lock(&mutex);
        fd = arduinoFd;
        pthread_mutex_unlock(&mutex);
    }
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
    return NULL;
}

static uint16_t GetResponse(const GPIO* gpio, uint32_t timeout)
{
    uint16_t resp = 0;

    pthread_mutex_lock(&mutex);
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    readTO.tv_sec = now.tv_sec + (timeout + now.tv_nsec / 1000000) / 1000;
    readTO.tv_nsec = (now.tv_nsec + timeout * 1000000) % 1000000000;

    readState = RS_WAITING;
    while (readState == RS_WAITING) {
        pthread_cond_wait(&cond, &mutex);
    }
    if (readState == RS_DONE) {
        if ((recvBuf[0] == 'r') && (recvBuf[1] == info[gpio->pinId].physicalPin)) {
            resp = ((uint16_t)recvBuf[2] << 8) | recvBuf[3];
        }
    }
    readState = RS_IDLE;
    pthread_mutex_unlock(&mutex);
    return resp;
}

static int SMsgOpen(void)
{
    int fd;
    int ret;

    fd = open(TTY_DEV, O_RDWR | O_NONBLOCK | O_NOCTTY);
    if (fd < 0) {
        perror("opening " TTY_DEV);
        goto exit;
    } else {
        struct termios tio;
        memset(&tio, 0, sizeof(tio));
        tio.c_iflag = 0;
        tio.c_oflag = 0;
        tio.c_cflag = CS8 | CREAD | CLOCAL;
        tio.c_lflag = 0;
        tio.c_cc[VMIN] = 0;
        tio.c_cc[VTIME] = 5;
        cfsetspeed(&tio, BAUDRATE);
        tcsetattr(fd, TCSANOW, &tio);
    }

    while (WaitForMsg(fd, 10 * RD_TO)) {
        FlushRead(fd);
    }

    arduinoFd = fd;

    ret = pthread_create(&threadId, NULL, ReadThread, NULL);
    if (ret) {
        AJ_ErrPrintf(("Failed to create read thread\n"));
        close(arduinoFd);
        arduinoFd = -1;
    } else {
        pthread_cond_init(&cond, NULL);
        pthread_mutex_init(&mutex, NULL);
    }

exit:
    return fd;
}

static int SendCmd(uint8_t op, GPIO* gpio, uint16_t arg)
{
    int fd;
    int success = 1;
    pthread_mutex_lock(&mutex);
    if (arduinoFd == -1) {
        arduinoFd = SMsgOpen();
    }
    fd = arduinoFd;
    pthread_mutex_unlock(&mutex);

    if (fd != -1) {
        uint8_t buf[4];
        int ret;
        buf[0] = op;
        buf[1] = info[gpio->pinId].physicalPin;
        buf[2] = arg >> 8;
        buf[3] = arg & 0xff;
        ret = WriteMsg(fd, buf, sizeof(buf));
        if (ret < 0) {
            AJ_ErrPrintf(("Failed to send cmd - closing socket\n"));
            close(fd);
            pthread_mutex_lock(&mutex);
            arduinoFd = -1;
            pthread_mutex_unlock(&mutex);
            success = 0;
        }
    }
    return success;
}

uint16_t AJS_TargetIO_GetNumPins()
{
    return ArraySize(info);
}

const AJS_IO_Info* AJS_TargetIO_GetInfo(uint16_t pin)
{
    if (pin < ArraySize(info)) {
        return &info[pin];
    } else {
        return NULL;
    }
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
    gpio->trigMode = AJS_IO_PIN_NO_TRIGGER;
    *pinCtx = gpio;
    if (SendCmd('c', gpio, PIN_CONFIG_DIGITAL | (uint16_t)config)) {
        return AJ_OK;
    }
    return AJ_ERR_DRIVER;
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
    if (!SendCmd('w', gpio, (uint16_t)val)) {
        AJ_ErrPrintf(("AJS_TargetIO_PinSet(%d, %d)\n", gpio->pinId, val));
    }
}

uint32_t AJS_TargetIO_PinGet(void* pinCtx)
{
    GPIO* gpio = (GPIO*)pinCtx;
    uint16_t resp = 0;
    if (SendCmd('r', gpio, 0)) {
        resp = GetResponse(gpio, 200);
    } else {
        AJ_ErrPrintf(("AJS_TargetIO_PinGet(%d) read cmd fail\n", gpio->pinId));
    }
    return resp;
}

int32_t AJS_TargetIO_PinTrigId()
{
    if (trigSet == 0) {
        return AJS_IO_PIN_NO_TRIGGER;
    } else {
        /*
         * This is static so triggers are returned round-robin to ensure fairness
         */
        static uint8_t id = 0;
        uint8_t mode;

        pthread_mutex_lock(&mutex);
        while (!BIT_IS_SET(trigSet, id)) {
            ++id;
            if (id > ArraySize(info)) {
                id = 0;
            }
        }
        mode = BIT_IS_SET(trigMode, id) ? AJS_IO_PIN_TRIGGER_ON_FALL : AJS_IO_PIN_TRIGGER_ON_RISE;
        BIT_CLR(trigSet, id);
        BIT_CLR(trigMode, id);
        pthread_mutex_unlock(&mutex);
        return (id << 4) | mode;
    }
}

AJ_Status AJS_TargetIO_PinPWM(void* pinCtx, double dutyCycle, uint32_t freq)
{
    return AJ_ERR_DRIVER;
}

AJ_Status AJS_TargetIO_PinEnableTrigger(void* pinCtx, AJS_IO_PinTriggerMode trigger, int32_t* trigId, uint8_t debounce)
{
    GPIO* gpio = (GPIO*)pinCtx;

    if ((gpio->trigMode == trigger) || (trigger & gpio->trigMode)) {
        if (trigger == AJS_IO_PIN_TRIGGER_DISABLE) {
            *trigId = AJS_IO_PIN_NO_TRIGGER;
        } else {
            *trigId = (gpio->pinId << 4) | (trigger & 0x0f);
        }
    } else {
        uint16_t arg = 0;
        uint16_t ltid;
        if (trigger == AJS_IO_PIN_TRIGGER_DISABLE) {
            ltid = (gpio->pinId << 4) | (gpio->trigMode & 0x0f);
            arg = 0;
        } else {
            ltid = (gpio->pinId << 4) | (trigger & 0x0f);
            arg = (debounce << 8) | trigger;
        }
        *trigId = ltid;
        BIT_CLR(trigSet, gpio->pinId);
        BIT_CLR(trigMode, gpio->pinId);
        gpio->trigMode = trigger;
        if (!SendCmd('i', gpio, arg)) {
            return AJ_ERR_DRIVER;
        }
    }
    return AJ_OK;
}

AJ_Status AJS_TargetIO_AdcOpen(uint16_t pin, void** adcCtx)
{
    AJ_ErrPrintf(("AJS_TargetIO_AdcOpen(%d)\n", pin));
    GPIO* gpio;
    gpio = malloc(sizeof(GPIO));
    if (!gpio) {
        AJ_ErrPrintf(("AJS_TargetIO_PinOpen(): Malloc failed to allocate %d bytes\n", sizeof(GPIO)));
        return AJ_ERR_RESOURCES;
    }
    gpio->pinId = pin;
    gpio->trigMode = AJS_IO_PIN_TRIGGER_DISABLE; // ignored
    *adcCtx = gpio;
    if (SendCmd('c', gpio, PIN_CONFIG_ANALOG | AJS_IO_PIN_INPUT)) {
        return AJ_OK;
    }
    return AJ_ERR_DRIVER;
}

AJ_Status AJS_TargetIO_AdcClose(void* adcCtx)
{
    GPIO* gpio = (GPIO*)adcCtx;
    AJ_ErrPrintf(("AJS_TargetIO_AdcClose(%d)\n", gpio->pinId));
    free(gpio);
    return AJ_OK;
}

uint32_t AJS_TargetIO_AdcRead(void* adcCtx)
{
    GPIO* gpio = (GPIO*)adcCtx;
    uint16_t resp = 0;
    if (SendCmd('r', gpio, 0)) {
        resp = GetResponse(gpio, 200);
    } else {
        AJ_ErrPrintf(("AJS_TargetIO_AdcRead(%d)\n", gpio->pinId));
    }
    return resp;
}

AJ_Status AJS_TargetIO_DacOpen(uint16_t pin, void** dacCtx)
{
    GPIO* gpio;
    AJ_ErrPrintf(("AJS_TargetIO_DacOpen(%d)\n", pin));
    gpio = malloc(sizeof(GPIO));
    if (!gpio) {
        AJ_ErrPrintf(("AJS_TargetIO_PinOpen(): Malloc failed to allocate %d bytes\n", sizeof(GPIO)));
        return AJ_ERR_RESOURCES;
    }
    gpio->pinId = pin;
    gpio->trigMode = AJS_IO_PIN_TRIGGER_DISABLE; // ignored
    *dacCtx = gpio;
    if (SendCmd('c', gpio, PIN_CONFIG_ANALOG | AJS_IO_PIN_OUTPUT)) {
        return AJ_OK;
    }
    return AJ_ERR_DRIVER;
}

AJ_Status AJS_TargetIO_DacClose(void* dacCtx)
{
    GPIO* gpio = (GPIO*)dacCtx;
    AJ_ErrPrintf(("AJS_TargetIO_DacClose(%d)\n", gpio->pinId));
    free(gpio);
    return AJ_OK;
}

void AJS_TargetIO_DacWrite(void* dacCtx, uint32_t val)
{
    GPIO* gpio = (GPIO*)dacCtx;
    if (val > 255) {
        val = 255;
    }
    if (!SendCmd('w', gpio, (uint16_t)val)) {
        AJ_ErrPrintf(("AJS_TargetIO_DacWrite(%d, %d)\n", gpio->pinId, val));
    }
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

AJ_Status AJS_TargetIO_SpiRead(void* ctx, uint32_t length, uint8_t* buffer)
{
    return AJ_ERR_UNEXPECTED;
}

void AJS_TargetIO_SpiWrite(void* ctx, uint8_t* data, uint32_t length)
{
    return;
}

AJ_Status AJS_TargetIO_SpiOpen(uint8_t mosi, uint8_t miso, uint8_t cs, uint8_t clk, uint32_t prescaler,
                               uint8_t master, uint8_t cpol, uint8_t cpha, uint8_t data, void** spiCtx)
{
    return AJ_ERR_UNEXPECTED;
}

AJ_Status AJS_TargetIO_SpiClose(void* spiCtx)
{
    return AJ_ERR_UNEXPECTED;
}

uint32_t AJS_TargetIO_UartRead(void* uartCtx, uint8_t* buf, uint32_t length)
{
    return 0;
}

AJ_Status AJS_TargetIO_UartWrite(void* uartCtx, uint8_t* data, uint32_t length)
{
    return AJ_ERR_UNEXPECTED;
}

AJ_Status AJS_TargetIO_UartOpen(uint8_t txPin, uint8_t rxPin, uint32_t baud, void** uartCtx)
{
    return AJ_ERR_UNEXPECTED;
}

AJ_Status AJS_TargetIO_UartClose(void* uartCtx)
{
    return AJ_ERR_UNEXPECTED;
}
void AJS_TargetIO_I2cStart(void* ctx, uint8_t addr)
{
}
void AJS_TargetIO_I2cStop(void* ctx)
{
}
uint8_t AJS_TargetIO_I2cRead(void* ctx)
{
    return 0;
}
void AJS_TargetIO_I2cWrite(void* ctx, uint8_t data)
{
}
AJ_Status AJS_TargetIO_I2cOpen(uint8_t sda, uint8_t scl, uint32_t clock, uint8_t mode, uint8_t ownAddress, void** ctx)
{
    return AJ_ERR_UNEXPECTED;
}
AJ_Status AJS_TargetIO_I2cClose(void* ctx)
{
    return AJ_ERR_UNEXPECTED;
}