/******************************************************************************
 *  *    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
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

#include "mbed.h"
#include "../../ajs.h"
#include "../../ajs_io.h"
#include "../../ajs_heap.h"

typedef struct {
    SPI* object;
    DigitalOut* cs;
} SPI_Pin;

typedef struct {
    uint8_t pinNum;     //Pin number from io_info.c
    uint16_t pinId;     //mbed pin ID (PTxxx)
    uint32_t function;  //MOSI, MISO, SCK
} SPI_Info;

static SPI_Info spiInfo[] = {
    { 41, PTD4, AJS_IO_FUNCTION_SPI_SS   },
    { 42, PTD5, AJS_IO_FUNCTION_SPI_SCK  },
    { 43, PTD6, AJS_IO_FUNCTION_SPI_MOSI },
    { 44, PTD7, AJS_IO_FUNCTION_SPI_MISO }
};

AJ_Status AJS_TargetIO_SpiRead(void* ctx, uint32_t length, uint8_t* buffer)
{
    SPI_Pin* spi = (SPI_Pin*)ctx;
    uint32_t i = 0;
    spi->cs->write(0);
    while (i < length) {
        *(buffer + i) = spi->object->write(0x00);
        i++;
    }
    spi->cs->write(1);
    return AJ_OK;
}

void AJS_TargetIO_SpiWrite(void* ctx, uint8_t* data, uint32_t length)
{
    SPI_Pin* spi = (SPI_Pin*)ctx;
    uint32_t i = 0;
    spi->cs->write(0);
    while (i < length) {
        spi->object->write(*(data + i));
    }
    spi->cs->write(1);
    return;
}

extern uint32_t SystemCoreClock;

AJ_Status AJS_TargetIO_SpiOpen(uint8_t mosi, uint8_t miso, uint8_t cs, uint8_t clk, uint32_t clock,
                               uint8_t master, uint8_t cpol, uint8_t cpha, uint8_t data, void** spiCtx)
{
    SPI_Pin* spi;
    uint8_t mode = 0;
    uint16_t mosiPin = AJS_TargetIO_GetInfo(mosi)->physicalPin;
    uint16_t misoPin = AJS_TargetIO_GetInfo(miso)->physicalPin;
    uint16_t csPin = AJS_TargetIO_GetInfo(cs)->physicalPin;
    uint16_t clkPin = AJS_TargetIO_GetInfo(clk)->physicalPin;

    uint8_t indexMosi, indexMiso, indexCs, indexClk;

    /*
     * Get the pin information for all the SPI pins
     */
    for (indexMosi = 0; indexMosi < ArraySize(spiInfo); ++indexMosi) {
        if (spiInfo[indexMosi].pinNum == mosiPin) {
            break;
        }
    }
    for (indexMiso = 0; indexMiso < ArraySize(spiInfo); ++indexMiso) {
        if (spiInfo[indexMiso].pinNum == misoPin) {
            break;
        }
    }
    for (indexCs = 0; indexCs < ArraySize(spiInfo); ++indexCs) {
        if (spiInfo[indexCs].pinNum == csPin) {
            break;
        }
    }
    for (indexClk = 0; indexClk < ArraySize(spiInfo); ++indexClk) {
        if (spiInfo[indexClk].pinNum == clkPin) {
            break;
        }
    }
    spi = (SPI_Pin*)AJ_Malloc(sizeof(SPI_Pin));
    spi->object = new SPI((PinName)spiInfo[indexMosi].pinId, (PinName)spiInfo[indexMiso].pinId, (PinName)spiInfo[indexClk].pinId);
    spi->cs = new DigitalOut((PinName)spiInfo[indexCs].pinId);
    spi->object->frequency(clock);
    /*
     * Mode     cpol    cpha
     * 0        0       0
     * 1        0       1
     * 2        1       0
     * 3        1       1
     */
    mode = (cpol << 1 | cpha << 0);
    if (mode > 3) {
        AJ_ErrPrintf(("AJS_TargetIO_SpiOpen(): cpol/cpha must be either 0 or 1\n"));
        return AJ_ERR_UNEXPECTED;
    }
    spi->object->format(data, mode);

    *spiCtx = spi;

    return AJ_OK;
}

AJ_Status AJS_TargetIO_SpiClose(void* spiCtx)
{
    SPI_Pin* spi = (SPI_Pin*)spiCtx;
    if (spi) {
        if (spi->cs) {
            delete spi->cs;
        }
        if (spi->object) {
            delete spi->object;
        }
        AJ_Free(spi);
    }
    return AJ_OK;
}