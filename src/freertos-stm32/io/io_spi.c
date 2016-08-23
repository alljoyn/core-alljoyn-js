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
#include "FreeRTOSConfig.h"

AJ_Status AJS_TargetIO_SpiRead(void* ctx, uint32_t length, uint8_t* buffer)
{
    SPI_Pin* spi = (SPI_Pin*)ctx;
    int i = 0;
    AJ_EnterCriticalRegion();
    GPIO_ResetBits(spi->SS_GPIO, spi->SS_Pin);
    while (i < length) {
        while (!SPI_I2S_GetFlagStatus(spi->SPIx, SPI_I2S_FLAG_TXE));
        SPI_I2S_SendData(spi->SPIx, 0x00);
        while (SPI_I2S_GetFlagStatus(spi->SPIx, SPI_I2S_FLAG_RXNE) == RESET);
        *(buffer + i) = SPI_I2S_ReceiveData(spi->SPIx);
        ++i;
    }
    GPIO_SetBits(spi->SS_GPIO, spi->SS_Pin);
    AJ_LeaveCriticalRegion();
    return AJ_OK;
}
void AJS_TargetIO_SpiWrite(void* ctx, uint8_t* data, uint32_t length)
{
    SPI_Pin* spi = (SPI_Pin*)ctx;
    int i = 0;
    AJ_EnterCriticalRegion();
    GPIO_ResetBits(spi->SS_GPIO, spi->SS_Pin);
    while (i < length) {
        while (!SPI_I2S_GetFlagStatus(spi->SPIx, SPI_I2S_FLAG_TXE));
        SPI_I2S_SendData(spi->SPIx, *(data + i));
        while (!SPI_I2S_GetFlagStatus(spi->SPIx, SPI_I2S_FLAG_RXNE));
        SPI_I2S_ReceiveData(spi->SPIx);
        ++i;
    }
    GPIO_SetBits(spi->SS_GPIO, spi->SS_Pin);
    AJ_LeaveCriticalRegion();
}
AJ_Status AJS_TargetIO_SpiOpen(uint8_t mosi, uint8_t miso, uint8_t cs, uint8_t clk, uint32_t clock,
                               uint8_t master, uint8_t cpol, uint8_t cpha, uint8_t data, void** spiCtx)
{
    SPI_InitTypeDef spi_config;
    GPIO_InitTypeDef gpio_config;

    uint16_t mosiPin = AJS_TargetIO_GetInfo(mosi)->physicalPin;
    uint16_t misoPin = AJS_TargetIO_GetInfo(miso)->physicalPin;
    uint16_t csPin = AJS_TargetIO_GetInfo(cs)->physicalPin;
    uint16_t clkPin = AJS_TargetIO_GetInfo(clk)->physicalPin;

    uint8_t indexMosi, indexMiso, indexCs, indexClk;

    SPI_Pin* ret = AJS_Alloc(NULL, sizeof(SPI_Pin));

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
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

    gpio_config.GPIO_Mode = GPIO_Mode_OUT;
    gpio_config.GPIO_PuPd = GPIO_PuPd_NOPULL;
    gpio_config.GPIO_Pin = pinInfo[indexCs].physicalPin;
    GPIO_Init(spiInfo[indexCs].GPIOx, &gpio_config);

    ret->SPIx = spiInfo[indexCs].SPIx;
    ret->SS_GPIO = spiInfo[indexCs].GPIOx;
    ret->SS_Pin = spiInfo[indexCs].physicalPin;

    AJ_Printf("SPI1 = 0x%x\n", SPI1);
    // SPI1 is already configured so only configure if it is SPI2
    if ((uint32_t)spiInfo[indexMosi].SPIx != (uint32_t)SPI1 &&
        (uint32_t)spiInfo[indexMiso].SPIx != (uint32_t)SPI1 &&
        (uint32_t)spiInfo[indexCs].SPIx != (uint32_t)SPI1 &&
        (uint32_t)spiInfo[indexClk].SPIx != (uint32_t)SPI1) {
        AJ_Printf("Setting up new spi controller\n");
        gpio_config.GPIO_Mode = GPIO_Mode_AF;
        gpio_config.GPIO_Speed = GPIO_Speed_50MHz;
        gpio_config.GPIO_OType = GPIO_OType_PP;
        gpio_config.GPIO_PuPd = GPIO_PuPd_UP;
        gpio_config.GPIO_Pin = pinInfo[indexMosi].physicalPin;
        GPIO_Init(spiInfo[indexMosi].GPIOx, &gpio_config);

        gpio_config.GPIO_Pin = pinInfo[indexMiso].physicalPin;
        GPIO_Init(spiInfo[indexMiso].GPIOx, &gpio_config);

        gpio_config.GPIO_Pin = pinInfo[indexClk].physicalPin;
        GPIO_Init(spiInfo[indexClk].GPIOx, &gpio_config);

        if (master == TRUE) {
            spi_config.SPI_Mode = SPI_Mode_Master;
        } else {
            spi_config.SPI_Mode = SPI_Mode_Slave;
        }
        if (cpol == 0) {
            spi_config.SPI_CPOL = SPI_CPOL_Low;
        } else {
            spi_config.SPI_CPOL = SPI_CPOL_High;
        }
        if (cpha == 1) {
            spi_config.SPI_CPHA = SPI_CPHA_1Edge;
        } else {
            spi_config.SPI_CPHA = SPI_CPHA_2Edge;
        }
        if (data == 8) {
            spi_config.SPI_DataSize = SPI_DataSize_8b;
        } else if (data == 16) {
            spi_config.SPI_DataSize = SPI_DataSize_16b;
        }
        if (clock >= (configCPU_CLOCK_HZ / 2)) {
            spi_config.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
        } else if (clock >= (configCPU_CLOCK_HZ / 4)) {
            spi_config.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
        } else if (clock >= (configCPU_CLOCK_HZ / 8)) {
            spi_config.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
        } else if (clock >= (configCPU_CLOCK_HZ / 16)) {
            spi_config.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
        } else if (clock >= (configCPU_CLOCK_HZ / 32)) {
            spi_config.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;
        } else if (clock >= (configCPU_CLOCK_HZ / 64)) {
            spi_config.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
        } else if (clock >= (configCPU_CLOCK_HZ / 128)) {
            spi_config.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;
        } else {
            spi_config.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
        }
        spi_config.SPI_CRCPolynomial = 7;
        spi_config.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
        spi_config.SPI_FirstBit = SPI_FirstBit_MSB;
        spi_config.SPI_NSS = SPI_NSS_Soft;

        SPI_Init(spiInfo[indexMosi].SPIx, ENABLE);
    }
    *spiCtx = (void*)ret;

    return AJ_OK;

}
AJ_Status AJS_TargetIO_SpiClose(void* spiCtx)
{
    return AJ_OK;
}
