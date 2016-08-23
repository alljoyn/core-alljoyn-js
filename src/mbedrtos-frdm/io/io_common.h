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