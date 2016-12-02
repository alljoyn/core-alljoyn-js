/**
 * @file
 */
/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#include "ajs.h"
#include "ajs_io.h"
#include "mbed.h"

typedef struct {
    AnalogIn* adcObj;
}ADC;

typedef struct {
    uint8_t pinNum;
    uint16_t pinId;
}ADC_Info;

static ADC_Info pinInfo[] = {
    { 27, PTB2 },
    { 28, PTB3 },
    { 29, PTB10 },
    { 30, PTB11 },
    { 31, PTC11 },
    { 32, PTC10 }
};

AJ_Status AJS_TargetIO_AdcOpen(uint16_t pinIndex, void** adcCtx)
{
    ADC* adc;
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
    adc = (ADC*)AJS_Alloc(NULL, sizeof(ADC));
    memset(adc, 0, sizeof(ADC));

    adc->adcObj = new AnalogIn((PinName)pinInfo[pin].pinId);

    *adcCtx = adc;

    return AJ_OK;
}

AJ_Status AJS_TargetIO_AdcClose(void* adcCtx)
{
    ADC* adc = (ADC*)adcCtx;
    if (adc) {
        if (adc->adcObj) {
            delete adc->adcObj;
        }
        AJS_Free(NULL, adc);
    }
    return AJ_OK;
}

uint32_t AJS_TargetIO_AdcRead(void* adcCtx)
{
    ADC* adc = (ADC*)adcCtx;
    return adc->adcObj->read_u16();
}