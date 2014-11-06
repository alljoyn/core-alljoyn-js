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

extern void ReleasePWM(GPIO* pin);
extern AJ_Status AllocPWM(GPIO* pin);

#define MAX_TRIGGERS 15

/*
 * Bit mask of allocated triggers (max 15)
 */
static uint32_t trigUse;
static uint32_t trigSet;

#define BIT_IS_SET(i, b)  ((i) & (1 << (b)))
#define BIT_SET(i, b)     ((i) |= (1 << (b)))
#define BIT_CLR(i, b)    ((i) &= ~(1 << (b)))

static int8_t AllocTrigId(uint8_t pinNum)
{
    int8_t id = 0;
    while (BIT_IS_SET(trigUse, id)) {
        ++id;
    }
    if (id == MAX_TRIGGERS) {
        return AJS_IO_PIN_NO_TRIGGER;
    } else {
        BIT_SET(trigUse, id);
        pinIdToSource[id] = pinNum;
        return id;
    }
}
AJ_Status AJS_TargetIO_PinEnableTrigger(void* pinCtx, AJS_IO_PinTriggerMode trigger, int32_t* trigId, uint8_t debounce)
{
    GPIO* gpio = (GPIO*)pinCtx;
    uint8_t trig;
    uint8_t exti_gpio;
    uint8_t exti_pin;
    uint8_t exti_irqn;
    uint8_t enable = ENABLE;
    EXTI_InitTypeDef interrupt;
    NVIC_InitTypeDef nvic;
    GPIO_InitTypeDef gpio_int;

    if ((trigger != AJS_IO_PIN_TRIGGER_ON_RISE) && (trigger != AJS_IO_PIN_TRIGGER_ON_FALL)) {
        /*
         * Disable interrupts on this pin
         */
        enable = DISABLE;
        return AJ_OK;
    }

    switch (gpio->gpioPin) {
    case (GPIO_Pin_0):
        exti_pin = EXTI_PinSource0;
        exti_irqn = EXTI0_IRQn;
        break;

    case (GPIO_Pin_1):
        /*
         * EXTI1 is already used for WSL so return AJ_ERR_RESOURCES
         */
        //exti_pin = EXTI_PinSource1;
        //exti_irqn = EXTI1_IRQn;
        return AJ_ERR_RESOURCES;
        break;

    case (GPIO_Pin_2):
        exti_pin = EXTI_PinSource2;
        exti_irqn = EXTI2_IRQn;
        break;

    case (GPIO_Pin_3):
        exti_pin = EXTI_PinSource3;
        exti_irqn = EXTI3_IRQn;
        break;

    case (GPIO_Pin_4):
        exti_pin = EXTI_PinSource4;
        exti_irqn = EXTI4_IRQn;
        break;

    case (GPIO_Pin_5):
        exti_pin = EXTI_PinSource5;
        exti_irqn = EXTI9_5_IRQn;
        break;

    case (GPIO_Pin_6):
        exti_pin = EXTI_PinSource6;
        exti_irqn = EXTI9_5_IRQn;
        break;

    case (GPIO_Pin_7):
        exti_pin = EXTI_PinSource7;
        exti_irqn = EXTI9_5_IRQn;
        break;

    case (GPIO_Pin_8):
        exti_pin = EXTI_PinSource8;
        exti_irqn = EXTI9_5_IRQn;
        break;

    case (GPIO_Pin_9):
        exti_pin = EXTI_PinSource9;
        exti_irqn = EXTI9_5_IRQn;
        break;

    case (GPIO_Pin_10):
        exti_pin = EXTI_PinSource10;
        exti_irqn = EXTI15_10_IRQn;
        break;

    case (GPIO_Pin_11):
        exti_pin = EXTI_PinSource11;
        exti_irqn = EXTI15_10_IRQn;
        break;

    case (GPIO_Pin_12):
        exti_pin = EXTI_PinSource12;
        exti_irqn = EXTI15_10_IRQn;
        break;

    case (GPIO_Pin_13):
        exti_pin = EXTI_PinSource13;
        exti_irqn = EXTI15_10_IRQn;
        break;

    case (GPIO_Pin_14):
        exti_pin = EXTI_PinSource14;
        exti_irqn = EXTI15_10_IRQn;
        break;

    case (GPIO_Pin_15):
        exti_pin = EXTI_PinSource15;
        exti_irqn = EXTI15_10_IRQn;
        break;
    }
    switch ((uint32_t)gpio->GPIOx) {
    case ((uint32_t)GPIOA):
        exti_gpio = EXTI_PortSourceGPIOA;
        break;

    case ((uint32_t)GPIOB):
        exti_gpio = EXTI_PortSourceGPIOB;
        break;

    case ((uint32_t)GPIOC):
        exti_gpio = EXTI_PortSourceGPIOC;
        break;

    case ((uint32_t)GPIOD):
        exti_gpio = EXTI_PortSourceGPIOD;
        break;

    case ((uint32_t)GPIOE):
        exti_gpio = EXTI_PortSourceGPIOE;
        break;

    case ((uint32_t)GPIOF):
        exti_gpio = EXTI_PortSourceGPIOF;
        break;

    case ((uint32_t)GPIOG):
        exti_gpio = EXTI_PortSourceGPIOG;
        break;

    case ((uint32_t)GPIOH):
        exti_gpio = EXTI_PortSourceGPIOH;
        break;

    case ((uint32_t)GPIOI):
        exti_gpio = EXTI_PortSourceGPIOI;
        break;
    }

    gpio->trigId = AllocTrigId(exti_pin);
    if (gpio->trigId == AJS_IO_PIN_NO_TRIGGER) {
        return AJ_ERR_RESOURCES;
    }
    gpio_int.GPIO_Pin = gpio->gpioPin;
    gpio_int.GPIO_OType = GPIO_OType_PP;
    gpio_int.GPIO_Mode = GPIO_Mode_IN;
    gpio_int.GPIO_PuPd = GPIO_PuPd_NOPULL;
    gpio_int.GPIO_Speed = GPIO_Speed_2MHz;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    SYSCFG_EXTILineConfig(exti_gpio, exti_pin);

    if (enable == ENABLE) {
        GPIO_Init(gpio->GPIOx, &gpio_int);
    } else {
        GPIO_DeInit(gpio->GPIOx);
    }

    trig = (trigger == AJS_IO_PIN_TRIGGER_ON_RISE) ? EXTI_Trigger_Rising : EXTI_Trigger_Falling;
    interrupt.EXTI_Trigger = trig;
    interrupt.EXTI_Mode = EXTI_Mode_Interrupt;
    interrupt.EXTI_LineCmd = enable;
    if (gpio->gpioPin <= GPIO_Pin_15) {
        /*
         * The gpio pin defines match the EXTI line defines so there is not need for a
         * large switch statement finding the correct match
         */
        interrupt.EXTI_Line = gpio->gpioPin;
    }
    EXTI_Init(&interrupt);

    nvic.NVIC_IRQChannel = exti_irqn;
    nvic.NVIC_IRQChannelCmd = enable;
    nvic.NVIC_IRQChannelPreemptionPriority = 7;
    nvic.NVIC_IRQChannelSubPriority = 7;

    NVIC_Init(&nvic);

    *trigId = gpio->trigId;
    return AJ_OK;
}
int32_t AJS_TargetIO_PinTrigId(uint32_t* level)
{
    if (trigSet == 0) {
        return AJS_IO_PIN_NO_TRIGGER;
    } else {
        static uint32_t id = 0;
        while (!BIT_IS_SET(trigSet, id % MAX_TRIGGERS)) {
            ++id;
        }
        BIT_CLR(trigSet, id % MAX_TRIGGERS);
        return (id % MAX_TRIGGERS);
    }
}
uint32_t AJS_TargetIO_PinGet(void* pinCtx)
{
    GPIO* gpio = (GPIO*)pinCtx;
    return GPIO_ReadInputDataBit(gpio->GPIOx, gpio->gpioPin);
}
void AJS_TargetIO_PinSet(void* pinCtx, uint32_t val)
{
    GPIO* gpio = (GPIO*)pinCtx;
    if (gpio->pwm.dutyCycle) {
        ReleasePWM(gpio);
    }
    if (val == 0) {
        GPIO_ResetBits(gpio->GPIOx, gpio->gpioPin);
    } else {
        GPIO_SetBits(gpio->GPIOx, gpio->gpioPin);
    }
    return;
}
void AJS_TargetIO_PinToggle(void* pinCtx)
{
    GPIO* gpio = (GPIO*)pinCtx;
    if (gpio->pwm.dutyCycle) {
        ReleasePWM(gpio);
    }
    GPIO_ToggleBits(gpio->GPIOx, gpio->gpioPin);
    return;
}
/*
 * No implementation for this on the SP140
 */
AJ_Status AJS_TargetIO_System(const char* cmd, char* output, uint16_t length)
{
    return AJ_ERR_FAILURE;
}

AJ_Status AJS_TargetIO_PinClose(void* pinCtx)
{
    GPIO* gpio = (GPIO*)pinCtx;
    if (gpio->pwm.dutyCycle) {
        ReleasePWM(gpio);
    }
    GPIO_DeInit(gpio->GPIOx);
    return AJ_OK;
}
AJ_Status AJS_TargetIO_PinOpen(uint16_t pinIndex, AJS_IO_PinConfig config, void** pinCtx)
{
    GPIO* gpio;
    GPIO_InitTypeDef GPIO_Pin;
    size_t pin;
    uint16_t physicalPin = AJS_TargetIO_GetInfo(pinIndex)->physicalPin;
    for (pin = 0; pin < ArraySize(pinInfo); ++pin) {
        if (pinInfo[pin].pinNum == physicalPin) {
            break;
        }
    }
    if (pin >= ArraySize(pinInfo)) {
        return AJ_ERR_INVALID;
    }
    gpio = AJS_Alloc(NULL, sizeof(GPIO));
    memset(gpio, 0, sizeof(GPIO));
    gpio->trigId = -1;
    gpio->gpioAF = 0;
    gpio->GPIOx = pinInfo[pin].GPIOx;
    gpio->gpioPin = pinInfo[pin].physicalPin;

    GPIO_Pin.GPIO_Speed = GPIO_Speed_50MHz;
    if (config & AJS_IO_PIN_OUTPUT) {
        GPIO_Pin.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_Pin.GPIO_OType = GPIO_OType_PP;
        GPIO_Pin.GPIO_Pin = pinInfo[pin].physicalPin;
        GPIO_Pin.GPIO_PuPd = GPIO_PuPd_UP;
    } else if (config & AJS_IO_PIN_INPUT) {
        GPIO_Pin.GPIO_Mode = GPIO_Mode_IN;
        GPIO_Pin.GPIO_OType = GPIO_OType_PP;
        GPIO_Pin.GPIO_Pin = pinInfo[pin].physicalPin;
        GPIO_Pin.GPIO_PuPd = GPIO_PuPd_UP;
    } else {
        if (config & AJS_IO_PIN_OPEN_DRAIN) {
            GPIO_Pin.GPIO_Mode = GPIO_Mode_OUT;
            GPIO_Pin.GPIO_OType = GPIO_OType_OD;
            GPIO_Pin.GPIO_Pin = pinInfo[pin].physicalPin;
            GPIO_Pin.GPIO_PuPd = GPIO_PuPd_UP;
        } else if (config & AJS_IO_PIN_PULL_UP) {
            GPIO_Pin.GPIO_Mode = GPIO_Mode_OUT;
            GPIO_Pin.GPIO_OType = GPIO_OType_PP;
            GPIO_Pin.GPIO_Pin = pinInfo[pin].physicalPin;
            GPIO_Pin.GPIO_PuPd = GPIO_PuPd_UP;
        } else {
            GPIO_Pin.GPIO_Mode = GPIO_Mode_OUT;
            GPIO_Pin.GPIO_OType = GPIO_OType_PP;
            GPIO_Pin.GPIO_Pin = pinInfo[pin].physicalPin;
            GPIO_Pin.GPIO_PuPd = GPIO_PuPd_DOWN;
        }
    }
    GPIO_Init(gpio->GPIOx, &GPIO_Pin);

    *pinCtx = gpio;

    return AJ_OK;
}
int8_t findTrigId(uint8_t pinSource)
{
    int8_t i;
    for (i = 0; i < MAX_TRIGGERS; i++) {
        if (pinIdToSource[i] == pinSource) {
            return i;
        }
    }
    return -1;
}

extern void AJ_Net_Interrupt(void);

void EXTI0_IRQHandler(void)
{
    int8_t trigId;
    if (EXTI_GetITStatus(EXTI_Line0) != RESET) {
        //Find the trigger ID and set the correct bit
        trigId = findTrigId(EXTI_PinSource0);
        BIT_SET(trigSet, trigId);
        AJ_Net_Interrupt();
        EXTI_ClearITPendingBit(EXTI_Line0);
        EXTI_ClearFlag(EXTI_Line0);
    }
}
void EXTI2_IRQHandler(void)
{
    AJ_Printf("EXTI2_IRQHandler\n");
    int8_t trigId;
    if (EXTI_GetITStatus(EXTI_Line2) != RESET) {
        //Find the trigger ID and set the correct bit
        trigId = findTrigId(EXTI_PinSource2);
        BIT_SET(trigSet, trigId);
        EXTI_ClearITPendingBit(EXTI_Line2);
        EXTI_ClearFlag(EXTI_Line2);
    }
}
void EXTI3_IRQHandler(void)
{
    AJ_Printf("EXTI3_IRQHandler\n");
    if (EXTI_GetITStatus(EXTI_Line3) != RESET) {
        //Handle the interrupt
        EXTI_ClearITPendingBit(EXTI_Line3);
        EXTI_ClearFlag(EXTI_Line3);
    }
}
void EXTI4_IRQHandler(void)
{
    AJ_Printf("EXTI4_IRQHandler\n");
    if (EXTI_GetITStatus(EXTI_Line4) != RESET) {
        //Handle the interrupt
        EXTI_ClearITPendingBit(EXTI_Line4);
        EXTI_ClearFlag(EXTI_Line4);
    }
}
void EXTI9_5_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line5) != RESET) {
        //Handle the interrupt
        AJ_Printf("EXTI5_IRQHandler\n");
        EXTI_ClearITPendingBit(EXTI_Line5);
        EXTI_ClearFlag(EXTI_Line5);
    } else if (EXTI_GetITStatus(EXTI_Line6) != RESET) {
        AJ_Printf("EXTI6_IRQHandler\n");
        EXTI_ClearITPendingBit(EXTI_Line6);
        EXTI_ClearFlag(EXTI_Line6);
    } else if (EXTI_GetITStatus(EXTI_Line7) != RESET) {
        AJ_Printf("EXTI7_IRQHandler\n");
        EXTI_ClearITPendingBit(EXTI_Line7);
        EXTI_ClearFlag(EXTI_Line7);
    } else if (EXTI_GetITStatus(EXTI_Line8) != RESET) {
        AJ_Printf("EXTI8_IRQHandler\n");
        EXTI_ClearITPendingBit(EXTI_Line8);
        EXTI_ClearFlag(EXTI_Line8);
    } else if (EXTI_GetITStatus(EXTI_Line9) != RESET) {
        AJ_Printf("EXTI9_IRQHandler\n");
        EXTI_ClearITPendingBit(EXTI_Line9);
        EXTI_ClearFlag(EXTI_Line9);
    } else {
        AJ_ErrPrintf(("EXTI9_5_IRQHandler(): IRQ Error\n"));
    }
}
void EXTI15_10_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line10) != RESET) {
        //Handle the interrupt
        AJ_Printf("EXTI10_IRQHandler\n");
        EXTI_ClearITPendingBit(EXTI_Line10);
        EXTI_ClearFlag(EXTI_Line10);
    } else if (EXTI_GetITStatus(EXTI_Line11) != RESET) {
        AJ_Printf("EXTI11_IRQHandler\n");
        EXTI_ClearITPendingBit(EXTI_Line11);
        EXTI_ClearFlag(EXTI_Line11);
    } else if (EXTI_GetITStatus(EXTI_Line12) != RESET) {
        AJ_Printf("EXTI12_IRQHandler\n");
        EXTI_ClearITPendingBit(EXTI_Line12);
        EXTI_ClearFlag(EXTI_Line12);
    } else if (EXTI_GetITStatus(EXTI_Line13) != RESET) {
        AJ_Printf("EXTI13_IRQHandler\n");
        EXTI_ClearITPendingBit(EXTI_Line13);
        EXTI_ClearFlag(EXTI_Line13);
    } else if (EXTI_GetITStatus(EXTI_Line14) != RESET) {
        AJ_Printf("EXTI14_IRQHandler\n");
        EXTI_ClearITPendingBit(EXTI_Line14);
        EXTI_ClearFlag(EXTI_Line14);
    } else if (EXTI_GetITStatus(EXTI_Line15) != RESET) {
        AJ_Printf("EXTI15_IRQHandler\n");
        EXTI_ClearITPendingBit(EXTI_Line15);
        EXTI_ClearFlag(EXTI_Line15);
    } else {
        AJ_ErrPrintf(("EXTI15_10_IRQHandler(): IRQ Error\n"));
    }
}
