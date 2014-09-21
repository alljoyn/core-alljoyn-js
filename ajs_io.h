#ifndef _AJS_IO_H
#define _AJS_IO_H
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

/**
 * I/O functions that might be configurable on various pins
 */
#define AJS_IO_FUNCTION_DIGITAL_IN  0x00000001
#define AJS_IO_FUNCTION_DIGITAL_OUT 0x00000002
#define AJS_IO_FUNCTION_PWM         0x00000004
#define AJS_IO_FUNCTION_DIGITAL_IO  0x00000003 /* Input or Output */

#define AJS_IO_FUNCTION_ANALOG_IN   0x00000010
#define AJS_IO_FUNCTION_ANALOG_OUT  0x00000020

#define AJS_IO_FUNCTION_UART_TX     0x00000100
#define AJS_IO_FUNCTION_UART_RX     0x00000200

#define AJS_IO_FUNCTION_I2C_SDA     0x00001000
#define AJS_IO_FUNCTION_I2C_SDC     0x00002000

#define AJS_IO_FUNCTION_SPI_SCK     0x00010000
#define AJS_IO_FUNCTION_SPI_SS      0x00020000
#define AJS_IO_FUNCTION_SPI_MOSI    0x00040000
#define AJS_IO_FUNCTION_SPI_MISO    0x00080000

#define AJS_IO_FUNCTION_I2S_CLK     0x00100000
#define AJS_IO_FUNCTION_I2S_WS      0x00200000
#define AJS_IO_FUNCTION_I2S_SDO     0x00400000
#define AJS_IO_FUNCTION_I2S_SDI     0x00800000

/**
 * Info of a IO pin.
 */
typedef struct {
    uint32_t functions;       /* OR of the various IO function bits */
    uint16_t physicalPin;     /* Physical pin on the package */
    const char*  schematicId; /* The ID used on the board schematic */
    const char*  datasheetId; /* The ID used in the MCU or MCP data sheet */
    const char*  description; /* Description of any dedicated use for the PIN */
} AJS_IO_Info;

/**
 * GPIO pin cofiguration options
 */
typedef enum {
    AJS_IO_PIN_INPUT      = 0x0,
    AJS_IO_PIN_OUTPUT     = 0x1,
    AJS_IO_PIN_OPEN_DRAIN = 0x2,
    AJS_IO_PIN_PULL_UP    = 0x4,
    AJS_IO_PIN_PULL_DOWN  = 0x8
} AJS_IO_PinConfig;

/**
 * Trigger modes for digital input pins
 */
typedef enum {
    AJS_IO_PIN_TRIGGER_DISABLE = 0,
    AJS_IO_PIN_TRIGGER_ON_RISE = 1,
    AJS_IO_PIN_TRIGGER_ON_FALL = 2
} AJS_IO_PinTriggerMode;

/**
 * Indicates no trigger set on a digital input pin
 */
#define AJS_IO_PIN_NO_TRIGGER (-1)

/**
 * Get name for one of the AJS_IO_* functions
 *
 * @param  The function
 * @return A text string name for the function
 */
const char* AJS_IO_FunctionName(uint32_t function);

/**
 * Get number of I/O pins
 */
uint16_t AJS_TargetIO_GetNumPins();

/**
 * Get information about a GPIO pin.
 *
 * @param pin    GPIO pin index
 *
 * @return   Returns the requested information or NULL if the pin index is invalid.
 */
const AJS_IO_Info* AJS_TargetIO_GetInfo(uint16_t pinIndex);

/**
 * Initialize a GPIO pin for input or output
 *
 * @param pinIndex  GPIO pin index in the AJS_IO_INFO array
 * @param config    Pin configuration
 * @param pinCtx    Returns an opaque pointer to a target-specific structure for the GPIO pin
 *
 * @return - AJ_OK if the pin was succesfully initialized
 *         - AJ_ERR_INVALID if pin doesn't identify a valid pin for this target
 */
AJ_Status AJS_TargetIO_PinOpen(uint16_t pinIndex, AJS_IO_PinConfig config, void** pinCtx);

/**
 * Close a previously opened GPIO pin freeing any resources
 *
 * @param pinCtx    Target-specific context for the GPIO pin
 */
AJ_Status AJS_TargetIO_PinClose(void* pinCtx);

/**
 * Set a GPIO pin
 *
 * @param pinCtx  The target-specific identifier for the GPIO pin
 * @param val     The value (0 or 1) to set.
 */
void AJS_TargetIO_PinSet(void* pinCtx, uint32_t val);

/**
 * Toggle a GPIO pin
 *
 * @param pinCtx  The target-specific identifier for the GPIO pin
 */
void AJS_TargetIO_PinToggle(void* pinCtx);

/**
 * Set the pwm duty cycle on a GPI pin
 *
 * @param pinCtx    The target-specific identifier for the GPIO pin
 * @param dutyCycle Value between 0.0 and 1.0 for the duty cycle
 * @param freq      Repetition frequency to rotate the bit pattern
 */
AJ_Status AJS_TargetIO_PinPWM(void* pinCtx, double dutyCycle, uint32_t freq);

/**
 * Get a GPIO pin
 *
 * @param pinCtx  The target-specific identifier for the GPIO pin
 * @param return  The value (0 or 1)
 */
uint32_t AJS_TargetIO_PinGet(void* pinCtx);

/**
 * Returns the trigger id for the GPIO pin that was triggered. If called repeatedly it will return each
 * of the trigger indices in order.
 *
 * @return  A trigger index or -1 if no GPIO pins are currently triggered.
 */
int32_t AJS_TargetIO_PinTrigId();

/**
 * Enable (or disable) trigger mode for a GPIO pin
 *
 * @param pinCtx   The target-specific identifier for the GPIO pin
 * @param trigger  The trigger mode that is being enabled
 * @param trigId   Returns an integer index for the trigger
 * @param debounce Debounce time in milliseconds
 */
AJ_Status AJS_TargetIO_PinEnableTrigger(void* pinCtx, AJS_IO_PinTriggerMode trigger, int32_t* trigId, uint8_t debounce);

/**
 * Open and configure an ADC pin
 *
 * @param pinIndex   ADC pin index in the AJS_IO_INFO array
 * @param adcCtx     Returns a pointer to an opaque target specific data structure for the ADC
 *
 * @return AJ_OK if the ADC pin was succesfully opened
 */
AJ_Status AJS_TargetIO_AdcOpen(uint16_t pinIndex, void** adcCtx);

/**
 * Close and an ADC pin and release any resources
 *
 * @param adcCtx Pointer to an opaque target specific data structure for the ADC pin
 *
 * @return AJ_OK if the pin was succesfully closed
 */
AJ_Status AJS_TargetIO_AdcClose(void* adcCtx);

/**
 * Read a single value from an ADC channel
 *
 * @param adcCtx Pointer to an opaque target specific data structure for the ADC channel
 *
 * @param return  The value read
 */
uint32_t AJS_TargetIO_AdcRead(void* adcCtx);

/**
 * Open and configure an DAC pin
 *
 * @param pin     DAC pin number
 * @param dacCtx  Returns a pointer to an opaque target specific data structure for the DAC
 *
 * @return AJ_OK if the DAC pin was succesfully opened
 */
AJ_Status AJS_TargetIO_DacOpen(uint16_t pin, void** dacCtx);

/**
 * Close and an DAC pin and release any resources
 *
 * @param dacCtx Pointer to an opaque target specific data structure for the DAC pin
 *
 * @return AJ_OK if the pin was succesfully closed
 */
AJ_Status AJS_TargetIO_DacClose(void* dacCtx);

/**
 * Write a single value to a DAC pin
 *
 * @param dacCtx  Pointer to an opaque target specific data structure for the DAC channel
 * @param val     The value to write
 *
 * @param return  The value read
 */
void AJS_TargetIO_DacWrite(void* dacCtx, uint32_t val);

/**
 * Emit an arbitrary command to the underlying system if supported
 *
 * @param cmd  The commmand to emit
 */
AJ_Status AJS_TargetIO_System(const char* cmd);

/**
 * Read from the SPI peripheral
 *
 * @param ctx       Pointer to an opaque target specific data structure for the SPI channel
 * @param length    Length in bytes that you want to read from the SPI peripheral
 *
 * @return          Pointer to the data that was read from the SPI peripheral
 */
uint8_t* AJS_TargetIO_SpiRead(void* ctx, uint32_t length);

/**
 * Write to the SPI peripheral
 *
 * @param ctx       Pointer to an opaque target specific data structure for the SPI channel
 * @param data      Pointer to the data you want to write
 * @param length    Length of the data you want to write
 */
void AJS_TargetIO_SpiWrite(void* ctx, uint8_t* data, uint32_t length);

/**
 * Open and configure a SPI peripheral
 *
 * @param mosi      Pin number of the MOSI (Master Out Slave In) SPI pin
 * @param miso      Pin number of the MISO (Master In Slave Out) SPI pin
 * @param cs        Pin number of the Chip Select SPI pin
 * @param clk       Pin number of the Clock SPI pin
 * @param prescaler A divisor value for the SPI clock speed (SPI Clock = CPU Clock / prescaler)
 * @param master    Flag configuring the SPI device as master or slave
 * @param cpol      Clock polarity for the SPI device (High or Low)
 * @param cpha      Clock phase for the SPI device (1 Edge or 2 Edge)
 * @param data      Number of data bits per transmission (8, 16, 32 etc)
 * @param cpiCtx    Returns a pointer to an opaque target specific data structure the SPI device
 *
 * @return          AJ_OK if the device was configured successfully
 */
AJ_Status AJS_TargetIO_SpiOpen(uint8_t mosi, uint8_t miso, uint8_t cs, uint8_t clk, uint32_t prescaler,
                               uint8_t master, uint8_t cpol, uint8_t cpha, uint8_t data, void** spiCtx);

/**
 * Close a SPI device
 *
 * @param spiCtx    Pointer to an opaque target specific data structure for the SPI channel
 *
 * @return          AJ_OK if the SPI device was closed successfully
 */
AJ_Status AJS_TargetIO_SpiClose(void* spiCtx);

/**
 * Read from the UART peripheral
 *
 * @param uartCtx   Pointer to an opaque target specific data structure for the UART channel
 * @param length    Number of bytes requesting to be read
 *
 * @return          Pointer to the data that was read
 */
uint8_t* AJS_TargetIO_UartRead(void* uartCtx, uint32_t length);

/**
 * Write data to the UART peripheral
 *
 * @param uartCtx   Pointer to an opaque target specific data structure for the UART channel
 * @param data      Pointer to the data buffer being written
 * @param length    Length of the data buffer
 *
 * @return          AJ_OK if the data was written successfully
 */
AJ_Status AJS_TargetIO_UartWrite(void* uartCtx, uint8_t* data, uint32_t length);

/**
 * Open and configure the a UART peripheral
 *
 * @param txPin     Pin number for the pin designated as transmit
 * @param rxPin     Pin number for the pin designated as receive
 * @param baud      Desired baud rate for the UART peripheral
 * @param uartCtx   Returns a pointer to an opaque target specific data structure the UART device
 */
AJ_Status AJS_TargetIO_UartOpen(uint8_t txPin, uint8_t rxPin, uint32_t baud, void** uartCtx);

/**
 * Close a UART device
 *
 * @param uartCtx   Pointer to an opaque target specific data structure for the UART channel
 *
 * @return          AJ_OK if the UART device was closed successfully
 */
AJ_Status AJS_TargetIO_UartClose(void* uartCtx);

/**
 * Start an I2C data transfer. This must be called first followed by AJS_TargetIO_I2cRead()/Write()
 * and ended by AJS_TargetIO_I2cStop()
 *
 * @param ctx       Pointer to an opaque target specific data structure for the I2C channel
 * @param addr      Address to the device on the I2C bus to talk to
 *
 */
void AJS_TargetIO_I2cStart(void* ctx, uint8_t addr);

/**
 * Stop an I2C transfer.
 *
 * @param ctx       Pointer to an opaque target specific data structure for the UART channel
 */
void AJS_TargetIO_I2cStop(void* ctx);

/**
 * Read from an I2C device.
 *
 * @param ctx       Pointer to an opaque target specific data structure for the UART channel
 *
 * @return          The byte that was read for the I2C device
 */
uint8_t AJS_TargetIO_I2cRead(void* ctx);

/**
 * Write to an I2C device.
 *
 * @param ctx       Pointer to an opaque target specific data structure for the UART channel
 * @param data      Byte of data to write to the I2C device
 */
void AJS_TargetIO_I2cWrite(void* ctx, uint8_t data);

/**
 * Open and configure an I2C peripheral
 *
 * @param sda       Pin number for the SDA (data) pin
 * @param dcl       Pin number for the SCL (clock) pin
 * @param clock     Clock rate for the I2C peripheral
 * @param mode      Specifies if the peripheral should be configured as master or slave
 * @param ownAddress The peripherals own I2C address. Only relevant if configuring for slave mode.
 * @param i2cCtx    Returned context pointer for the I2C peripheral
 *
 * @return          AJ_OK if the peripheral was configured correctly
 */
AJ_Status AJS_TargetIO_I2cOpen(uint8_t sda, uint8_t scl, uint32_t clock, uint8_t mode, uint8_t ownAddress, void** i2cCtx);

/**
 * Close a previously opened I2C peripheral
 *
 * @param ctx       Pointer to an opaque target specific data structure for the UART channel
 *
 * @return          AJ_OK if the I2C peripheral was closed correctly
 */
AJ_Status AJS_TargetIO_I2cClose(void* ctx);

#endif
