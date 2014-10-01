/**
   [22] * @file
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
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_UART_RX,                          1,  "UART_RX_GPIO",         "PTC16",        "UART RX line" },       //pin[0]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_UART_TX,                          2,  "UART_TX_GPIO",         "PTC17",        "UART TX line" },       //pin[1]
    { AJS_IO_FUNCTION_DIGITAL_IO,                       3,  "GPIO_PTB9",            "PTB9",         "General purpose IO" }, //pin[2]
    { AJS_IO_FUNCTION_DIGITAL_IO,                       5,  "GPIO_PTB23",           "PTB23",        "General purpose IO" }, //pin[3]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_PWM,                              6,  "PWM_GPIO_PTA2",        "PTA2",         "GPIO and PWM" },       //pin[4]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_PWM,                              7,  "PWM_GPIO_PTC2",        "PTC2",         "GPIO and PWM" },       //pin[5]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_PWM,                              9,  "PWM_GPIO_PTA0",        "PTA0",         "GPIO and PWM" },       //pin[6]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_PWM,                              10, "PWM_GPIO_PTC4",        "PTC4",         "GPIO and PWM" },       //pin[7]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_PWM |
      AJS_IO_FUNCTION_SPI_MOSI,                         12, "MOSI_PWM_GPIO_PTD2",   "PTD2",         "GPIO, PWM and MOSI" }, //pin[8]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_PWM |
      AJS_IO_FUNCTION_SPI_MISO,                         13, "MISO_PWM_GPIO_PTD3",   "PTD3",         "GPIO, PWM and MISO" }, //pin[9]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_PWM |
      AJS_IO_FUNCTION_SPI_SCK,                          14, "SCK_PWM_GPIO_PTD1",    "PTD1",         "GPIO, PWM, and SCK" }, //pin[10]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_I2C_SDA,                          17, "SDA_GPIO_PTE25",       "PTE25",        "GPIO and SDA" },       //pin[11]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_I2C_SCL,                          18, "SDC_GPIO_PTE24",       "PTE24",        "GPIO and SDC" },       //pin[12]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_ANALOG_IN,                        27, "ADC_GPIO_PTB2",        "PTB2",         "GPIO and ADC" },       //pin[13]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_ANALOG_IN,                        28, "ADC_GPIO_PTB3",        "PTB3",         "GPIO and ADC" },       //pin[14]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_ANALOG_IN,                        29, "ADC_GPIO_PTB10",       "PTB10",        "GPIO and ADC" },       //pin[15]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_ANALOG_IN,                        30, "ADC_GPIO_PTB11",       "PTB11",        "GPIO and ADC" },       //pin[16]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_PWM |
      AJS_IO_FUNCTION_ANALOG_IN,                        31, "ADC_PWM_GPIO_PTC11",   "PTC11",        "GPIO, PWM and ADC" },  //pin[17]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_PWM |
      AJS_IO_FUNCTION_ANALOG_IN,                        32, "ADC_PWM_GPIO_PTC10",   "PTC10",        "GPIO, PWM, and ADC" }, //pin[18]
    { AJS_IO_FUNCTION_DIGITAL_OUT,                      33, "LED_RED_PTB22",        "PTB22",        "Red LED" },            //pin[19]
    { AJS_IO_FUNCTION_DIGITAL_OUT,                      34, "LED_GREEN_PTE26",      "PTE26",        "Green LED" },          //pin[20]
    { AJS_IO_FUNCTION_DIGITAL_OUT,                      35, "LED_BLUE_PTB21",       "PTB21",        "Blue LED" },           //pin[21]
    { AJS_IO_FUNCTION_DIGITAL_IN,                       36, "PB_SW2",               "PTC6",         "Push Button 2" },      //pin[22]
    { AJS_IO_FUNCTION_DIGITAL_IN,                       37, "PB_SW3",               "PTA4",         "Push Button 3" },      //pin[23]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_UART_RX,                          38, "UART_RX",              "PTC15",        "Uart BT module RX" },   //pin[24]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_UART_TX,                          39, "UART_TX",              "PTC14",        "Uart BT module TX" },   //pin[25]
    { AJS_IO_FUNCTION_DIGITAL_IO,                       40, "GPIO_WIFI_SPI_CE",     "PTC12",        "WiFi header CE pin" },  //pin[26]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_SPI_SS,                           41, "GPIO_WIFI_SPI_CS",     "PTD4",         "WiFi header CS pin" },  //pin[27]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_SPI_SCK,                          42, "GPIO_WIFI_SPI_SCK",    "PTD5",         "WiFi header SCK pin" }, //pin[28]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_SPI_MOSI,                         43, "GPIO_WIFI_SPI_MOSI",   "PTD6",         "WiFi header MOSI pin" }, //pin[29]
    { AJS_IO_FUNCTION_DIGITAL_IO |
      AJS_IO_FUNCTION_SPI_MISO,                         44, "GPIO_WIFI_SPI_MISO",   "PTD7",         "WiFi header MISO pin" }, //pin[30]
    { AJS_IO_FUNCTION_DIGITAL_IN,                       45, "GPIO_WIFI_SPI_IRQ",    "PTC18",        "WiFi header IRQ" }      //pin[31]
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

