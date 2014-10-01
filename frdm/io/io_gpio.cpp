/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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

#include "mbed.h"
#include "ajs.h"
#include "ajs_io.h"
#include "ajs_heap.h"
#include "aj_net.h"

typedef void (*intFunc)(void);
typedef void* digitalObj;

#define DOUT    0
#define DIN     1
#define DBOTH   2

typedef struct {
    uint8_t pinNum;     //Pin number from io_info.c
    uint16_t pinId;     //mbed pin ID (PTxxx)
    intFunc func;       //Interrupt function pointer
}GPIO_Info;

typedef struct {
    uint8_t type;
    union {
        DigitalOut* out;
        DigitalIn* in;
        DigitalInOut* inOut;
    };
    InterruptIn* interrupt;     //Interrupt object (must hold on so we can delete it later)
    int8_t trigId;              //Trigger identifier
    uint16_t pinIdx;
    struct {
        //PwmOut* object;       //MBED PWM object
        Ticker* ticker;
        uint8_t dutyCycle;
        uint8_t count;
    } pwm;
}GPIO;

/*
 * Every pin that needs an interrupt has to have a unique function
 * because there is no way to associate the interrupt call to a
 * pin without it being unique.
 */
void AJS_InterruptFunc(uint8_t pin);
void interruptFunc0(void) {
    AJS_InterruptFunc(0);
}
void interruptFunc1(void) {
    AJS_InterruptFunc(1);
}
void interruptFunc2(void) {
    AJS_InterruptFunc(2);
}
void interruptFunc3(void) {
    AJS_InterruptFunc(3);
}
void interruptFunc4(void) {
    AJS_InterruptFunc(4);
}
void interruptFunc5(void) {
    AJS_InterruptFunc(5);
}
void interruptFunc6(void) {
    AJS_InterruptFunc(6);
}
void interruptFunc7(void) {
    AJS_InterruptFunc(7);
}
void interruptFunc8(void) {
    AJS_InterruptFunc(8);
}
void interruptFunc9(void) {
    AJS_InterruptFunc(9);
}
void interruptFunc10(void) {
    AJS_InterruptFunc(10);
}
void interruptFunc11(void) {
    AJS_InterruptFunc(11);
}
void interruptFunc12(void) {
    AJS_InterruptFunc(12);
}
void interruptFunc13(void) {
    AJS_InterruptFunc(13);
}
void interruptFunc14(void) {
    AJS_InterruptFunc(14);
}

GPIO_Info pinInfo[] = {
    { 1,  PTC16, interruptFunc0 },
    { 2,  PTC17, interruptFunc1 },
    { 3,  PTB9,  interruptFunc2 },
    { 5,  PTB23, interruptFunc3 },
    { 6,  PTA2,  interruptFunc4 },
    { 7,  PTC2,  interruptFunc5 },
    { 9,  PTA0,  interruptFunc6 },
    { 10, PTC4,  interruptFunc7 },
    { 12, PTD2,  interruptFunc8 },
    { 13, PTD3,  interruptFunc9 },
    { 14, PTD1,  interruptFunc10 },
    { 17, PTE25, interruptFunc11 },
    { 18, PTE24, interruptFunc12 },
    { 27, PTB2,  NULL },
    { 28, PTB3,  NULL },
    { 29, PTB10, NULL },
    { 30, PTB11, NULL },
    { 31, PTC11, NULL },
    { 32, PTC10, NULL },
    { 33, PTB22, NULL },
    { 34, PTE26, NULL },
    { 35, PTB21, NULL },
    { 36, PTC6,  interruptFunc13 },
    { 37, PTA4,  interruptFunc14 }
};

#define MAX_TRIGGERS 15

static uint8_t pinIdToSource[15];
/*
 * Bit mask of allocated triggers (max 15)
 */
static uint32_t trigUse;
static uint32_t trigSet;

#define BIT_IS_SET(i, b)  ((i) & (1 << (b)))
#define BIT_SET(i, b)     ((i) |= (1 << (b)))
#define BIT_CLR(i, b)    ((i) &= ~(1 << (b)))

static int8_t AllocTrigId(uint8_t pinNum)
{
    int8_t id = 0;
    while (BIT_IS_SET(trigUse, id)) {
        ++id;
    }
    if (id == MAX_TRIGGERS) {
        return AJS_IO_PIN_NO_TRIGGER;
    } else {
        BIT_SET(trigUse, id);
        pinIdToSource[id] = pinNum;
        return id;
    }
}

extern "C" int8_t findTrigId(uint8_t pinSource)
{
    int8_t i;
    for (i = 0; i < MAX_TRIGGERS; i++) {
        if (pinIdToSource[i] == pinSource) {
            return i;
        }
    }
    return -1;
}

extern void AJ_Net_Interrupt(void);

void AJS_InterruptFunc(uint8_t pin)
{
    int8_t trigId;
    trigId = findTrigId(pin);
    BIT_SET(trigSet, trigId);
    AJ_Net_Interrupt();
    AJ_Printf("Pin %u fired and interrupt\n", pin);
}

extern "C" AJ_Status AJS_TargetIO_PinEnableTrigger(void* pinCtx, AJS_IO_PinTriggerMode trigger, int32_t* trigId, uint8_t debounce)
{
    GPIO* gpio = (GPIO*)pinCtx;
    size_t pin;
    uint16_t pinId;
    uint16_t physicalPin = AJS_TargetIO_GetInfo(gpio->pinIdx)->physicalPin;
    for (pin = 0; pin < ArraySize(pinInfo); ++pin) {
        if (pinInfo[pin].pinNum == physicalPin) {
            pinId = pinInfo[pin].pinId;
            break;
        }
    }
    if (!pinInfo[pin].func) {
        return AJ_ERR_INVALID;
    }
    gpio->trigId = AllocTrigId(pin);
    if (gpio->trigId == AJS_IO_PIN_NO_TRIGGER) {
        return AJ_ERR_RESOURCES;
    }
    gpio->interrupt = new InterruptIn((PinName)pinId);
    if (trigger == AJS_IO_PIN_TRIGGER_ON_RISE) {
        gpio->interrupt->rise(pinInfo[pin].func);
    } else if (trigger == AJS_IO_PIN_TRIGGER_ON_FALL) {
        gpio->interrupt->fall(pinInfo[pin].func);
    }
    *trigId = gpio->trigId;
    return AJ_OK;
}

extern "C" int32_t AJS_TargetIO_PinTrigId(uint32_t* level)
{
    if (trigSet == 0) {
        return AJS_IO_PIN_NO_TRIGGER;
    } else {
        static uint32_t id = 0;
        while (!BIT_IS_SET(trigSet, id % MAX_TRIGGERS)) {
            ++id;
        }
        BIT_CLR(trigSet, id % MAX_TRIGGERS);
        return (id % MAX_TRIGGERS);
    }
}

extern "C" uint32_t AJS_TargetIO_PinGet(void* pinCtx)
{
    GPIO* gpio = (GPIO*)pinCtx;
    switch (gpio->type) {
    case (DOUT):
        return gpio->out->read();
        break;

    case (DBOTH):
        return gpio->inOut->read();
        break;

    case (DIN):
        return gpio->in->read();

    default:
        break;
    }
    return -1;
}

extern "C" void AJS_TargetIO_PinSet(void* pinCtx, uint32_t val)
{
    GPIO* gpio = (GPIO*)pinCtx;
    switch (gpio->type) {
    case (DOUT):
        gpio->out->write(val ? 0 : 1);
        break;

    case (DBOTH):
        gpio->inOut->write(val ? 0 : 1);
        break;

    case (DIN):
    default:
        break;
    }
    return;
}

extern "C" void AJS_TargetIO_PinToggle(void* pinCtx)
{
    GPIO* gpio = (GPIO*)pinCtx;
    switch (gpio->type) {
    case (DOUT):
        *(gpio->out) = !*(gpio->out);
        break;

    case (DBOTH):
        *(gpio->inOut) = !*(gpio->inOut);
        break;

    case (DIN):
    default:
        break;
    }
    return;
}
/*
 * No implementation for this on the Freedom board
 */
extern "C" AJ_Status AJS_TargetIO_System(const char* cmd, char* output, uint16_t length)
{
    return AJ_ERR_FAILURE;
}

extern "C" AJ_Status AJS_TargetIO_PinClose(void* pinCtx)
{
    GPIO* gpio = (GPIO*)pinCtx;
    if (gpio) {
        /* Check which type of digital pin and delete accordingly */
        if      (gpio->type == DOUT  && gpio->out) {
            delete gpio->out;
        } else if (gpio->type == DIN   && gpio->in) {
            delete gpio->in;
        } else if (gpio->type == DBOTH && gpio->inOut) {
            delete gpio->inOut;
        }

        if (gpio->trigId != -1) {
            delete gpio->interrupt;
        }
        AJS_Free(NULL, gpio);
    }
    return AJ_OK;
}

extern "C" AJ_Status AJS_TargetIO_PinOpen(uint16_t pinIndex, AJS_IO_PinConfig config, void** pinCtx)
{
    GPIO* gpio;
    size_t pin;
    uint16_t physicalPin = AJS_TargetIO_GetInfo(pinIndex)->physicalPin;
    for (pin = 0; pin < ArraySize(pinInfo); ++pin) {
        if (pinInfo[pin].pinNum == physicalPin) {
            break;
        }
    }
    if (pin >= ArraySize(pinInfo)) {
        return AJ_ERR_INVALID;
    }
    gpio = (GPIO*)AJS_Alloc(NULL, sizeof(GPIO));
    memset(gpio, 0, sizeof(GPIO));

    /*
     * Find if this is a digitalOut, digitalIn or both
     */
    uint32_t direction = AJS_TargetIO_GetInfo(pinIndex)->functions;

    if (direction & AJS_IO_FUNCTION_DIGITAL_OUT) {
        gpio->out = new DigitalOut((PinName)pinInfo[pin].pinId, 0);
        *(gpio->out) = 0;
    } else if ((direction & AJS_IO_FUNCTION_DIGITAL_IN) && (config != AJS_IO_PIN_OUTPUT)) {
        gpio->in = new DigitalIn((PinName)pinInfo[pin].pinId);
        switch (config) {
        case (AJS_IO_PIN_OPEN_DRAIN):
            //gpio->in->mode(OpenDrain); //TODO: This enum does not exist
            break;

        case (AJS_IO_PIN_PULL_UP):
            gpio->in->mode(PullUp);
            break;

        case (AJS_IO_PIN_PULL_DOWN):
            gpio->in->mode(PullDown);
            break;

        default:
            break;
        }
    } else {
        gpio->inOut = new DigitalInOut((PinName)pinInfo[pin].pinId);
    }
    gpio->trigId = -1;
    gpio->pinIdx = pinIndex;

    *pinCtx = gpio;

    return AJ_OK;
}

#define MAX_PWM_PINS 9

static GPIO* pwmPins[MAX_PWM_PINS];
static uint8_t pwmCount = 0;

AJ_Status AllocPWM(GPIO* pin)
{
    size_t i;
    size_t idx;
    uint16_t physicalPin = AJS_TargetIO_GetInfo(pin->pinIdx)->physicalPin;
    for (idx = 0; idx < ArraySize(pinInfo); ++idx) {
        if (pinInfo[idx].pinNum == physicalPin) {
            break;
        }
    }
    AJ_ASSERT(!pin->pwm.dutyCycle);
    for (i = 0; i < MAX_PWM_PINS; ++i) {
        if (!pwmPins[i]) {
            pwmPins[i] = pin;
            pin->pwm.count = 0;
            ++pwmCount;
            //pin->pwm.object = new PwmOut((PinName)pinInfo[idx].pinId);
            return AJ_OK;
        }
    }
    return AJ_ERR_RESOURCES;
}

void ReleasePWM(GPIO* pin)
{
    size_t i;
    for (i = 0; i < MAX_PWM_PINS; ++i) {
        if (pwmPins[i] == pin) {
            pwmPins[i] = NULL;
            pin->pwm.dutyCycle = 0;
            --pwmCount;
            //delete pin->pwm.object;
            break;
        }
    }
}

static void PWM_Interrupt(void)
{
    int i;
    for (i = 0; i < MAX_PWM_PINS; i++) {
        GPIO* gpio = pwmPins[i];
        if (gpio) {
            uint8_t count = ++gpio->pwm.count;
            if (count == gpio->pwm.dutyCycle) {
                if (gpio->out) {
                    gpio->out->write(0);
                }
            } else {
                if (gpio->out) {
                    gpio->out->write(1);
                }
            }
        }
    }
}

static Ticker* ticker;
static uint8_t initialized = false;

static void AJS_SetHWTimer(uint32_t freq)
{
    if (!initialized) {
        ticker = new Ticker();
        ticker->attach_us(&PWM_Interrupt, freq * 1000);
        initialized = true;
    }
}

/*
 * TODO: Find a solution to PWM with MBED. There is presumably a timer issue
 * with PWM/MBED RTOS. Currently PWM does not work correctly.
 */
extern "C" AJ_Status AJS_TargetIO_PinPWM(void* pinCtx, double dutyCycle, uint32_t freq)
{
    //TODO: Incorporate frequency
    AJ_Status status = AJ_OK;;
    GPIO* gpio = (GPIO*)pinCtx;
    uint8_t dc = (uint8_t)(dutyCycle * 255);

    if ((dc == 0) || (dc == 255)) {
        AJS_TargetIO_PinSet(pinCtx, dutyCycle);
        return AJ_OK;
    }
    if (!gpio->pwm.dutyCycle) {
        status = AllocPWM(gpio);
    }
    if (status == AJ_OK) {
        //AJ_Printf("Duty float = %f\n", dcfloat);
        gpio->pwm.dutyCycle = dc;
        AJS_SetHWTimer(freq);
        //gpio->pwm.object->write(dutyCycle);
    }
    return AJ_OK;
}

