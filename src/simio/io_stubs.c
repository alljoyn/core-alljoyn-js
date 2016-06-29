/**
 * @file
 */
/******************************************************************************
 * Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
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

#include "../ajs.h"
#include "../ajs_io.h"

AJ_Status AJS_TargetIO_AdcOpen(uint16_t channel, void** adcCtx)
{
    return AJ_ERR_UNEXPECTED;
}

AJ_Status AJS_TargetIO_AdcClose(void* adcCtx)
{
    return AJ_ERR_UNEXPECTED;
}

uint32_t AJS_TargetIO_AdcRead(void* adcCtx)
{
    return 0;
}

AJ_Status AJS_TargetIO_DacOpen(uint16_t pin, void** dacCtx)
{
    return AJ_ERR_UNEXPECTED;
}

AJ_Status AJS_TargetIO_DacClose(void* dacCtx)
{
    return AJ_ERR_UNEXPECTED;
}

void AJS_TargetIO_DacWrite(void* dacCtx, uint32_t val)
{
}

AJ_Status AJS_TargetIO_SpiRead(void* ctx, uint32_t length, uint8_t* buffer)
{
    AJ_InfoPrintf(("AJS_TargetIO_SpiRead()"));
    return AJ_ERR_UNEXPECTED;
}

void AJS_TargetIO_SpiWrite(void* ctx, uint8_t* data, uint32_t length)
{
    AJ_InfoPrintf(("AJS_TargetIO_SpiWrite(): Wrote %i\n", data));
    return;
}

AJ_Status AJS_TargetIO_SpiOpen(uint8_t mosi, uint8_t miso, uint8_t cs, uint8_t clk, uint32_t clock,
                               uint8_t master, uint8_t cpol, uint8_t cpha, uint8_t data, void** spiCtx)
{
    return AJ_ERR_UNEXPECTED;
}

AJ_Status AJS_TargetIO_SpiClose(void* spiCtx)
{
    return AJ_ERR_UNEXPECTED;
}

AJ_Status AJS_TargetIO_I2cOpen(uint8_t sda, uint8_t scl, uint32_t clock, uint8_t mode, uint8_t ownAddress, void** ctx)
{
    return AJ_ERR_UNEXPECTED;
}

AJ_Status AJS_TargetIO_I2cClose(void* ctx)
{
    return AJ_ERR_UNEXPECTED;
}

AJ_Status AJS_TargetIO_I2cTransfer(void* ctx, uint8_t addr, uint8_t* txBuf, uint8_t txLen, uint8_t* rxBuf, uint8_t rxLen, uint8_t* rxBytes)
{
    return AJ_ERR_UNEXPECTED;
}