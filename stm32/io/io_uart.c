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
    GPIO_TypeDef* GPIOx;
    USART_TypeDef* USARTx;
    uint16_t physicalPin;
    uint16_t pinNum;
}UART_Info;

typedef struct {
    USART_TypeDef* USARTx;
} UART;

UART_Info uartInfo[] = {
    { GPIOB, USART1, GPIO_Pin_6,  92, }, //USART1 TX PB6
    { GPIOA, USART1, GPIO_Pin_9,  68, }, //USART1 TX PA9
    { GPIOB, USART3, GPIO_Pin_10, 47, }, //USART3 TX PB10
    { GPIOD, USART3, GPIO_Pin_8,  55, }, //USART3 TX PD8
    { GPIOC, USART3, GPIO_Pin_10, 78, }, //USART3 TX PC10
    { GPIOB, USART1, GPIO_Pin_7,  93, }, //USART1 RX PB7
    { GPIOA, USART1, GPIO_Pin_10, 69, }, //USART1 RX PA10
    { GPIOB, USART3, GPIO_Pin_11, 48, }, //USART3 RX PB11
    { GPIOD, USART3, GPIO_Pin_9,  56, }, //USART3 RX PD9
    { GPIOC, USART3, GPIO_Pin_11, 79, }  //USART3 RX PC11
};

static uint8_t validatePins(uint8_t txPin, uint8_t rxPin)
{
    if ((txPin + 1) == rxPin) {
        return TRUE;
    }
    if (((txPin == 92) || (txPin == 68)) &&
        ((rxPin != 93) || (rxPin != 69))) {
        return FALSE;
    } else if (((txPin == 47) || (txPin == 55) || (txPin == 78)) &&
               ((rxPin != 48) || (rxPin != 56) || (rxPin != 79))) {
        return FALSE;
    }
    return TRUE;
}
uint8_t* AJS_TargetIO_UartRead(void* uartCtx, uint32_t length)
{
    UART* uart = (UART*)uartCtx;
    uint8_t i = 0;
    uint8_t* buffer = AJS_Alloc(NULL, length);
    while (i < length) {
        while (USART_GetFlagStatus(uart->USARTx, USART_FLAG_TXE) == RESET) ;

        *(buffer + i) = USART_ReceiveData(uart->USARTx);
        ++i;
    }
    return buffer;

}
AJ_Status AJS_TargetIO_UartWrite(void* uartCtx, uint8_t* buffer, uint32_t length)
{
    UART* uart = (UART*)uartCtx;
    uint8_t i = 0;
    while (i < length) {
        while (USART_GetFlagStatus(uart->USARTx, USART_FLAG_TXE) == RESET) ;

        USART_SendData(uart->USARTx, *(buffer + i));
        ++i;
    }
    return AJ_OK;
}

AJ_Status AJS_TargetIO_UartOpen(uint8_t txPin, uint8_t rxPin, uint32_t baud, void** uartCtx)
{
    GPIO_InitTypeDef USART_GPIO;
    USART_ClockInitTypeDef USART_ClkInit;
    USART_InitTypeDef uartHandle;
    UART* uart;
    uint16_t pinTx, pinRx;
    uint16_t physicalTxPin = AJS_TargetIO_GetInfo(txPin)->physicalPin;
    uint16_t physicalRxPin = AJS_TargetIO_GetInfo(rxPin)->physicalPin;
    uint8_t pinSource;
    uint8_t AFConfig;

    if (!validatePins(txPin, rxPin)) {
        return AJ_ERR_INVALID;
    }
    for (pinTx = 0; pinTx < ArraySize(uartInfo); ++pinTx) {
        if (uartInfo[pinTx].pinNum == physicalTxPin) {
            break;
        }
    }
    if (pinTx >= ArraySize(uartInfo)) {
        return AJ_ERR_INVALID;
    }
    for (pinRx = 0; pinRx < ArraySize(uartInfo); ++pinRx) {
        if (uartInfo[pinRx].pinNum == physicalRxPin) {
            break;
        }
    }
    if (pinRx >= ArraySize(uartInfo)) {
        return AJ_ERR_INVALID;
    }
    if ((uint32_t)uartInfo[pinTx].USARTx == (uint32_t)USART1) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
        AFConfig = GPIO_AF_USART1;
    } else if ((uint32_t)uartInfo[pinTx].USARTx == (uint32_t)USART3) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
        AFConfig = GPIO_AF_USART3;
    } else {
        AJ_ErrPrintf(("Configuring an un-configurable USART\n"));
        return AJ_ERR_INVALID;
    }
    pinSource = pinToSource(uartInfo[pinTx].physicalPin);

    GPIO_PinAFConfig(uartInfo[pinTx].GPIOx, pinSource, AFConfig);
    USART_Cmd(uartInfo[pinTx].USARTx, ENABLE);

    USART_GPIO.GPIO_Mode = GPIO_Mode_AF;
    USART_GPIO.GPIO_OType = GPIO_OType_PP;
    USART_GPIO.GPIO_Pin = (uartInfo[pinTx].physicalPin | uartInfo[pinRx].physicalPin);
    USART_GPIO.GPIO_PuPd = GPIO_PuPd_UP;
    USART_GPIO.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(uartInfo[pinTx].GPIOx, &USART_GPIO);
    USART_ClockInit(uartInfo[pinTx].USARTx, &USART_ClkInit);

    uartHandle.USART_BaudRate = baud * 3;
    uartHandle.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    uartHandle.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    uartHandle.USART_Parity = USART_Parity_No;
    uartHandle.USART_StopBits = USART_StopBits_1;
    uartHandle.USART_WordLength = USART_WordLength_8b;

    USART_Init(uartInfo[pinTx].USARTx, &uartHandle);

    uart = AJS_Alloc(NULL, sizeof(UART));
    uart->USARTx = uartInfo[pinTx].USARTx;

    *uartCtx = (void*)uart;

    return AJ_OK;
}
AJ_Status AJS_TargetIO_UartClose(void* uartCtx)
{
    return AJ_OK;
}
