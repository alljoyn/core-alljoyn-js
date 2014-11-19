/**
 * @file
 */
/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

typedef struct {
    I2C_InitTypeDef* I2Cx;
    GPIO_InitTypeDef* GPIOx;
    uint16_t physicalPin;
    uint16_t pinNum;
}I2C_Info;

typedef struct {
    I2C_InitTypeDef* I2Cx;
}I2C_Pin;

static const I2C_Info i2cInfo[] = {
    { I2C1, GPIOB, GPIO_Pin_6, 92 },            //PB6 SCL
    { I2C1, GPIOB, GPIO_Pin_9, 96 },            //PB9 SDA
    { I2C1, GPIOB, GPIO_Pin_8, 95 },            //PB8 SCL
    { I2C1, GPIOB, GPIO_Pin_7, 93 }             //PB7 SDA
};
void AJS_TargetIO_I2cStart(void* ctx, uint8_t addr)
{
    I2C_Pin* i2c = (I2C_Pin*)ctx;
    while (I2C_GetFlagStatus(i2c->I2Cx, I2C_FLAG_BUSY)) ;
    I2C_GenerateSTART(i2c->I2Cx, ENABLE);
    while (!I2C_CheckEvent(i2c->I2Cx, I2C_EVENT_MASTER_MODE_SELECT)) ;
    I2C_Send7bitAddress(i2c->I2Cx, addr, I2C_Direction_Transmitter);
}
void AJS_TargetIO_I2cStop(void* ctx)
{
    I2C_Pin* i2c = (I2C_Pin*)ctx;
    I2C_GenerateSTOP(i2c->I2Cx, ENABLE);
    while (!I2C_CheckEvent(i2c->I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) ;
}

uint8_t AJS_TargetIO_I2cRead(void* ctx)
{
    I2C_Pin* i2c = (I2C_Pin*)ctx;
    return I2C_ReceiveData(i2c->I2Cx);
}
void AJS_TargetIO_I2cWrite(void* ctx, uint8_t data)
{
    I2C_Pin* i2c = (I2C_Pin*)ctx;
    I2C_SendData(i2c->I2Cx, data);
}
AJ_Status AJS_TargetIO_I2cOpen(uint8_t sda, uint8_t scl, uint32_t clock, uint8_t mode, uint8_t ownAddress, void** ctx)
{
    GPIO_InitTypeDef i2cGPIO;
    I2C_InitTypeDef i2cInit;
    uint16_t sdaPin = AJS_TargetIO_GetInfo(sda)->physicalPin;
    uint16_t sclPin = AJS_TargetIO_GetInfo(scl)->physicalPin;
    uint8_t indexSda, indexScl;
    I2C_Pin* i2cPin;
    uint8_t pinSource;

    i2cPin = AJS_Alloc(NULL, sizeof(I2C_Pin));

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    for (indexSda = 0; indexSda < ArraySize(i2cInfo); indexSda++) {
        if (i2cInfo[indexSda].pinNum == sda) {
            break;
        }
    }
    for (indexScl = 0; indexScl < ArraySize(i2cInfo); indexScl++) {
        if (i2cInfo[indexScl].pinNum == scl) {
            break;
        }
    }

    i2cGPIO.GPIO_Pin = i2cInfo[indexSda].physicalPin | i2cInfo[indexScl].physicalPin;
    i2cGPIO.GPIO_Mode = GPIO_Mode_AF;
    i2cGPIO.GPIO_OType = GPIO_OType_OD;
    i2cGPIO.GPIO_PuPd = GPIO_PuPd_UP;
    i2cGPIO.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(i2cInfo[indexSda].GPIOx, &i2cGPIO);
    pinSource = pinToSource(i2cInfo[indexSda].physicalPin);
    GPIO_PinAFConfig(i2cInfo[indexSda].GPIOx, pinSource, GPIO_AF_I2C1);
    pinSource = pinToSource(i2cInfo[indexScl].physicalPin);
    GPIO_PinAFConfig(i2cInfo[indexScl].GPIOx, pinSource, GPIO_AF_I2C1);

    i2cInit.I2C_Ack = I2C_Ack_Disable;
    i2cInit.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    i2cInit.I2C_ClockSpeed = clock;
    i2cInit.I2C_DutyCycle = I2C_DutyCycle_2;
    i2cInit.I2C_Mode = I2C_Mode_I2C;
    i2cInit.I2C_OwnAddress1 = 0x00;
    I2C_Init(I2C1, &i2cInit);

    I2C_Cmd(I2C1, ENABLE);

    *ctx = i2cCtx;

    return AJ_OK;
}
AJ_Status AJS_TargetIO_I2cClose(void* ctx)
{
    return AJ_OK;
}
