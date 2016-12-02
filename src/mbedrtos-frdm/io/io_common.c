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

#include "../../ajs.h"
#include "io_common.h"

uint8_t AJS_GetPinNum(void* infoStruct, uint8_t physicalPin)
{
    GenericPinInfo* pinInfo = (GenericPinInfo*)infoStruct;
    uint8_t pin;
    for (pin = 0; pin < ArraySize(pinInfo); ++pin) {
        if (pinInfo[pin].pinNum == physicalPin) {
            break;
        }
    }
}
