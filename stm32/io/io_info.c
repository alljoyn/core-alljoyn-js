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

#include "ajs.h"
#include "ajs_io.h"

static const AJS_IO_Info info[] = {
    { AJS_IO_FUNCTION_SPI_MOSI,                          18, "SPI2_MOSI_1",         "PC3",      "SPI2 MOSI line" },     //pin1
    { AJS_IO_FUNCTION_SPI_MISO,                          19, "SPI2_MISO_1",         "PC2",      "SPI2 MISO line" },     //pin2
    { AJS_IO_FUNCTION_UART_RX,                           24, "UART2_RX",            "PA3",      "UART RX" },            //pin3
    { AJS_IO_FUNCTION_UART_TX,                           25, "UART2_TX",            "PA2",      "UART RX" },            //pin4
    { AJS_IO_FUNCTION_DIGITAL_IN,                        23, "GPIO_PA0",            "PA0",      "Push button" },         //pin5
    { AJS_IO_FUNCTION_DIGITAL_IO,                        77, "GPIO_PD7",            "PD7",      "General purpose" },    //pin6
    { AJS_IO_FUNCTION_DIGITAL_IO,                        86, "GPIO_PD5",            "PD5",      "General purpose" },    //pin7
    { AJS_IO_FUNCTION_DIGITAL_IO,                        87, "GPIO_PD6",            "PD6",      "General purpose" },    //pin8
    { AJS_IO_FUNCTION_DIGITAL_IO,                        84, "GPIO_PD3",            "PD3",      "General purpose" },    //pin9
    { AJS_IO_FUNCTION_DIGITAL_IO,                        85, "GPIO_PD4",            "PD4",      "General purpose" },    //pin10
    { AJS_IO_FUNCTION_DIGITAL_IO,                        82, "GPIO_PD1",            "PD1",      "General purpose" },    //pin11
    { AJS_IO_FUNCTION_DIGITAL_IO,                        83, "GPIO_PD2",            "PD2",      "General purpose" },    //pin12
    { AJS_IO_FUNCTION_DIGITAL_OUT,                       59, "GPIO_PD12",           "PD12",     "Green LED" },          //pin13
    { AJS_IO_FUNCTION_DIGITAL_OUT,                       60, "GPIO_PD13",           "PD13",     "Orange LED" },         //pin14
    { AJS_IO_FUNCTION_DIGITAL_OUT,                       61, "GPIO_PD14",           "PD14",     "Red LED" },            //pin15
    { AJS_IO_FUNCTION_DIGITAL_OUT,                       62, "GPIO_PD15",           "PD15",     "Blue LED" },           //pin16
    { AJS_IO_FUNCTION_PWM | AJS_IO_FUNCTION_DIGITAL_OUT, 63, "GPIO_PC6",            "PC6",      "PWM 1" },               //pin17
    { AJS_IO_FUNCTION_PWM | AJS_IO_FUNCTION_DIGITAL_OUT, 64, "GPIO_PC7",            "PC7",      "PWM 2" },               //pin18
    { AJS_IO_FUNCTION_PWM | AJS_IO_FUNCTION_DIGITAL_OUT, 35, "GPIO_PB0",            "PB0",      "PWM 3" },               //pin19
    { AJS_IO_FUNCTION_PWM | AJS_IO_FUNCTION_DIGITAL_OUT, 36, "GPIO_PB1",            "PB1",      "PWM 4" },               //pin20
    { AJS_IO_FUNCTION_ANALOG_IN,                         16, "ADC_IN_11",           "PC1",      "Analog In" },          //pin21
    { AJS_IO_FUNCTION_ANALOG_IN,                         17, "ADC_IN_12",           "PC2",      "Analog In" },          //pin22
    { AJS_IO_FUNCTION_ANALOG_IN,                         34, "ADC_IN_15",           "PC5",      "Analog In" },          //pin23
    { AJS_IO_FUNCTION_ANALOG_IN,                         33, "ADC_IN_14",           "PC4",      "Temperature Sensor" }, //pin24
    { AJS_IO_FUNCTION_ANALOG_IN,                         97, "MEMS_X_AXIS",         "MEMX",     "Accelerometer X" },    //pin25
    { AJS_IO_FUNCTION_ANALOG_IN,                         98, "MEMS_Y_AXIS",         "MEMY",     "Accelerometer Y" },    //pin26
    { AJS_IO_FUNCTION_ANALOG_IN,                         99, "MEMS_Z_AXIS",         "MEMZ",     "Accelerometer Z" },    //pin27
    { AJS_IO_FUNCTION_SPI_SCK | AJS_IO_FUNCTION_UART_TX, 47, "SPI2_SCK_1",          "PB10",     "SPI2 SCK / USART3 TX" }, //pin28
    { AJS_IO_FUNCTION_SPI_SS,                            51, "SPI2_SS_1",           "PB12",     "SPI2 SS line" },       //pin29
    { AJS_IO_FUNCTION_SPI_SCK,                           52, "SPI2_SCK_2",          "PB13",     "SPI2 SCK line" },      //pin30
    { AJS_IO_FUNCTION_SPI_MISO,                          53, "SPI2_MISO_2",         "PB14",     "SPI2 MISO line" },     //pin31
    { AJS_IO_FUNCTION_SPI_MOSI,                          54, "SPI2_MOSI_2",         "PB15",     "SPI2 MOSI line" },     //pin32
    { AJS_IO_FUNCTION_SPI_SS,                            96, "SPI2_SS_2",           "PB9",      "SPI2 SS line" },       //pin33
    { AJS_IO_FUNCTION_SPI_SCK,                           30, "SPI1_SCK_1",          "PA5",      "SPI1 SCK line" },       //pin34
    { AJS_IO_FUNCTION_SPI_SS,                             2, "SPI1_SS_2",           "PE3",      "SPI1 SS line (accel)" }, //pin35
    { AJS_IO_FUNCTION_SPI_MOSI,                          32, "SPI1_MOSI_1",         "PA7",      "SPI1 MOSI line" },     //pin36
    { AJS_IO_FUNCTION_SPI_MISO,                          31, "SPI1_MISO_1",         "PA6",      "SPI1 MISO line" },     //pin37
    { AJS_IO_FUNCTION_UART_TX,                           92, "USART1_TX_1",         "PB6",      "USART1 TX line" },     //pin38
    { AJS_IO_FUNCTION_UART_TX,                           68, "USART1_TX_2",         "PA9",      "USART1 TX line" },     //pin39
    { AJS_IO_FUNCTION_UART_TX,                           47, "USART3_TX_1",         "PB10",     "USART3 TX line" },     //pin40
    { AJS_IO_FUNCTION_UART_TX,                           55, "USART3_TX_2",         "PD8",      "USART3 TX line" },     //pin41
    { AJS_IO_FUNCTION_UART_TX,                           78, "USART3_TX_3",         "PC10",     "USART3 TX line" },     //pin42
    { AJS_IO_FUNCTION_UART_RX,                           93, "USART1_RX_1",         "PB7",      "USART1 RX line" },     //pin43
    { AJS_IO_FUNCTION_UART_RX,                           69, "USART1_RX_2",         "PA10",     "USART1 RX line" },     //pin44
    { AJS_IO_FUNCTION_UART_RX,                           48, "USART3_RX_1",         "PB11",     "USART3 RX line" },     //pin45
    { AJS_IO_FUNCTION_UART_RX,                           56, "USART3_RX_2",         "PD9",      "USART3 RX line" },     //pin46
    { AJS_IO_FUNCTION_UART_RX,                           79, "USART3_RX_3",         "PC11",     "USART3 RX line" }      //pin47
};

uint8_t AJS_ValidateSpiPins(uint16_t mosi, uint16_t miso, uint16_t clk)
{
    /*
     * Check that the SPI pins will work with each other
     * (Chip select can be any GPIO pin so we don't check for that)
     */
    if (info[mosi].functions == AJS_IO_FUNCTION_SPI_MOSI &&
        info[miso].functions == AJS_IO_FUNCTION_SPI_MISO &&
        info[clk].functions == AJS_IO_FUNCTION_SPI_SCK) {
        return TRUE;
    }
}

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

