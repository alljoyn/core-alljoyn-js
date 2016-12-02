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
#ifndef IO_COMMON_H_
#define IO_COMMON_H_

/*
 * This struct is used as a hack way to get the actual pin
 * number mapped from the io_info struct to the various
 * peripherals supported. Each peripheral (GPIO, I2C, ADC etc.)
 * has its own struct with mappings to the real pin number that
 * is always needed to initialize each peripheral. If this pin
 * number is always a uint8_t and always the first element it
 * can be cast to the GenericPinInfo struct to get just the pin.
 */
typedef struct _GenericPinInfo {
    uint8_t pinNum;
} GenericPinInfo;

/*
 * Get the pin index from the physicalPin
 */
#ifdef __cplusplus
extern "C" {
#endif
uint8_t AJS_GetPinNum(void* infoStruct, uint8_t physicalPin);
#ifdef __cplusplus
}
#endif
#endif /* IO_COMMON_H_ */