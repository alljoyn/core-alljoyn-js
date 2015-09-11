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
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include "ajs.h"
#include "ajs_io.h"


static const AJS_IO_Info info[] = {
    {  AJS_IO_FUNCTION_DIGITAL_OUT, 0,  "LED_1",   "",  "Red LED" },
    {  AJS_IO_FUNCTION_DIGITAL_OUT, 1,  "LED_2",   "",  "Green LED" },
    {  AJS_IO_FUNCTION_DIGITAL_OUT, 3,  "LED_3",   "",  "Yellow LED" },
    {  AJS_IO_FUNCTION_DIGITAL_OUT |
       AJS_IO_FUNCTION_PWM,         3,  "LED_4",   "",  "Blue LED" },
    {  AJS_IO_FUNCTION_DIGITAL_IN,  4,  "GPIO_1",  "",  "Digital input 1" },
    {  AJS_IO_FUNCTION_DIGITAL_IN,  5,  "GPIO_2",  "",  "Digital input 2" },
    {  AJS_IO_FUNCTION_DIGITAL_IN,  6,  "GPIO_3",  "",  "Digital input 3" },
    {  AJS_IO_FUNCTION_DIGITAL_IN,  7,  "GPIO_4",  "",  "Digital input 4" },
    {  AJS_IO_FUNCTION_DIGITAL_IN,  8,  "PB_A",    "",  "Push button A" },
    {  AJS_IO_FUNCTION_DIGITAL_IN,  9,  "PB_B",    "",  "Push button B" },
    {  AJS_IO_FUNCTION_DIGITAL_IN, 19,  "PB_C",    "",  "Push button C" },
    {  AJS_IO_FUNCTION_DIGITAL_IN, 11,  "PB_D",    "",  "Push button D" },
    {  AJS_IO_FUNCTION_UART_TX,    12,  "UART_TX", "",  "UART transmit" },
    {  AJS_IO_FUNCTION_UART_RX,    13,  "UART_RX", "",  "UART receive" }
};

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