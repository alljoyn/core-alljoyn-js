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
/******************************************************************************
 * This code statically links to code available from
 * http://www.st.com/web/en/catalog/tools/ and that code is subject to a license
 * agreement with terms and conditions that you will be responsible for from
 * STMicroelectronics if you employ that code. Use of such code is your responsibility.
 * Neither AllSeen Alliance nor any contributor to this AllSeen code base has any
 * obligations with respect to the STMicroelectronics code that to which you will be
 * statically linking this code. One requirement in the license is that the
 * STMicroelectronics code may only be used with STMicroelectronics processors as set
 * forth in their agreement."
 *******************************************************************************/
#include "io_common.h"

uint8_t pinToSource(uint16_t pin) {
    uint8_t pinSource;
    switch (pin) {
    case (GPIO_Pin_0):
        pinSource = GPIO_PinSource0;
        break;

    case (GPIO_Pin_1):
        pinSource = GPIO_PinSource1;
        break;

    case (GPIO_Pin_2):
        pinSource = GPIO_PinSource2;
        break;

    case (GPIO_Pin_3):
        pinSource = GPIO_PinSource3;
        break;

    case (GPIO_Pin_4):
        pinSource = GPIO_PinSource4;
        break;

    case (GPIO_Pin_5):
        pinSource = GPIO_PinSource5;
        break;

    case (GPIO_Pin_6):
        pinSource = GPIO_PinSource6;
        break;

    case (GPIO_Pin_7):
        pinSource = GPIO_PinSource7;
        break;

    case (GPIO_Pin_8):
        pinSource = GPIO_PinSource8;
        break;

    case (GPIO_Pin_9):
        pinSource = GPIO_PinSource9;
        break;

    case (GPIO_Pin_10):
        pinSource = GPIO_PinSource10;
        break;

    case (GPIO_Pin_11):
        pinSource = GPIO_PinSource11;
        break;

    case (GPIO_Pin_12):
        pinSource = GPIO_PinSource12;
        break;

    case (GPIO_Pin_13):
        pinSource = GPIO_PinSource13;
        break;

    case (GPIO_Pin_14):
        pinSource = GPIO_PinSource14;
        break;

    case (GPIO_Pin_15):
        pinSource = GPIO_PinSource15;
        break;
    }
    return pinSource;
}
