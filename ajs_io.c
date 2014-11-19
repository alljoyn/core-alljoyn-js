/**
 * @file
 */
/******************************************************************************
 * Copyright (c) 2013, 2014, AllSeen Alliance. All rights reserved.
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
#include "ajs_util.h"
#include "ajs_io.h"

/*
 * Get pin context pointer from the "this" object
 */
void* PinCtxPtr(duk_context* ctx)
{
    void* pinCtx;
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, AJS_HIDDEN_PROP("ctx"));
    pinCtx = duk_require_pointer(ctx, -1);
    duk_pop_2(ctx);
    return pinCtx;
}

/*
 * Create a new IO object
 */
static int NewIOObject(duk_context* ctx, void* pinCtx, duk_c_function finalizer)
{
    int idx = duk_push_object(ctx);

    duk_dup(ctx, 0);
    duk_put_prop_string(ctx, idx, "pin");
    duk_push_pointer(ctx, pinCtx);
    duk_put_prop_string(ctx, idx, AJS_HIDDEN_PROP("ctx"));
    /*
     * Callback from when the pin object is garbage collected.
     */
    AJS_RegisterFinalizer(ctx, idx, finalizer);
    return idx;
}

static int NativeSetTrigger(duk_context* ctx)
{
    AJ_Status status;
    int32_t trigId;
    uint32_t debounce = 0;
    AJS_IO_PinTriggerMode mode = (AJS_IO_PinTriggerMode)duk_require_int(ctx, 0);

    if (mode != AJS_IO_PIN_TRIGGER_DISABLE) {
        if (!duk_is_function(ctx, 1)) {
            duk_error(ctx, DUK_ERR_TYPE_ERROR, "Trigger function required");
        }
        if (duk_is_number(ctx, 2)) {
            debounce = duk_get_int(ctx, 2);
            debounce = min(255, debounce);
        }
    }
    /*
     * Enable or disable the trigger
     */
    status = AJS_TargetIO_PinEnableTrigger(PinCtxPtr(ctx), mode, &trigId, debounce);
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "Error %s", AJ_StatusText(status));
    }

    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, "IO");
    duk_get_prop_string(ctx, -1, AJS_HIDDEN_PROP("trigs"));

    if ((mode & AJS_IO_PIN_TRIGGER_ON_RISE) || (mode & AJS_IO_PIN_TRIGGER_ON_FALL)) {
        /*
         * Set the callback function on the pin object
         */
        duk_push_this(ctx);
        duk_dup(ctx, 1);
        duk_put_prop_string(ctx, -2, "trigger");
        /*
         * Add the pin object to the triggers array.
         */
        duk_put_prop_index(ctx, -2, trigId);
    } else {
        /*
         * Delete the pin object if it was registered
         */
        if (trigId != AJS_IO_PIN_NO_TRIGGER) {
            duk_del_prop_index(ctx, -1, trigId);
        }
    }
    duk_pop_3(ctx);
    /*
     * Leave pin object on the stack
     */
    return 1;
}

static int NativeLevelGetter(duk_context* ctx)
{
    duk_push_number(ctx, AJS_TargetIO_PinGet(PinCtxPtr(ctx)));
    return 1;
}

static int NativeLevelSetter(duk_context* ctx)
{
    AJS_TargetIO_PinSet(PinCtxPtr(ctx), duk_require_int(ctx, 0));
    return 0;
}

static int NativeTogglePin(duk_context* ctx)
{
    AJS_TargetIO_PinToggle(PinCtxPtr(ctx));
    return 0;
}

static int NativePWM(duk_context* ctx)
{
    AJ_Status status;
    double dutyCycle = duk_require_number(ctx, 0);
    uint32_t freq = 0;

    if (dutyCycle > 1.0 || dutyCycle < 0.0) {
        duk_error(ctx, DUK_ERR_RANGE_ERROR, "Duty cycle must be in the range 0.0 to 1.0");
    }
    /*
     * Frequency is optional. If not specified zero is passed into the target code and the default
     * target PWM frequency is used.
     */
    if (!duk_is_undefined(ctx, 1)) {
        freq = duk_require_int(ctx, 1);
        if (freq == 0) {
            duk_error(ctx, DUK_ERR_RANGE_ERROR, "Frequency cannot be zero Hz");
        }
    }
    status = AJS_TargetIO_PinPWM(PinCtxPtr(ctx), dutyCycle, freq);
    if (status != AJ_OK) {
        if (status == AJ_ERR_RESOURCES) {
            duk_error(ctx, DUK_ERR_RANGE_ERROR, "Too many PWM pins");
        } else {
            duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "Error setting PWM %s", AJ_StatusText(status));
        }
    }
    return 0;
}

static int NativePinFinalizer(duk_context* ctx)
{
    AJ_InfoPrintf(("Closing Pin\n"));
    duk_get_prop_string(ctx, 0, AJS_HIDDEN_PROP("ctx"));
    AJS_TargetIO_PinClose(duk_require_pointer(ctx, -1));
    duk_pop(ctx);
    return 0;
}

/*
 * Returns the pid id for a pin object. Also checks that the pin supports the requested function.
 */
static uint32_t GetPinId(duk_context* ctx, int idx, uint32_t function)
{
    uint32_t id;
    if (!duk_is_object(ctx, idx)) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Requires a pin object");
    }
    /*
     * Get pin id
     */
    duk_get_prop_string(ctx, idx, "id");
    id = duk_require_int(ctx, -1);
    duk_pop(ctx);
    /*
     * Check that the required I/O function is supported
     */
    if (function) {
        const AJS_IO_Info* info = AJS_TargetIO_GetInfo(id);
        if (!info) {
            duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "Undefined I/O pin%d", id);
        }
        if (!(info->functions & function)) {
            duk_error(ctx, DUK_ERR_TYPE_ERROR, "I/O function %s not supported on pin%d", AJS_IO_FunctionName(function), id);
        }
    }
    return id;
}

/*
 * Configures a pin as a digital output pin
 */
static int NativeIoDigitalOut(duk_context* ctx)
{
    AJ_Status status;
    int idx;
    void* pinCtx;
    uint32_t pin = GetPinId(ctx, 0, AJS_IO_FUNCTION_DIGITAL_OUT);

    /*
     * Target specific I/O pin initialization
     */
    status = AJS_TargetIO_PinOpen(pin, AJS_IO_PIN_OUTPUT, &pinCtx);
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "Failed to configure digital output pin: %s", AJ_StatusText(status));
    }
    idx = NewIOObject(ctx, pinCtx, NativePinFinalizer);
    /*
     * Functions to set/get the pin level
     */
    AJS_SetPropertyAccessors(ctx, idx, "level", NativeLevelSetter, NativeLevelGetter);
    /*
     * Check for and set initial value
     */
    if (duk_is_number(ctx, 1)) {
        duk_dup(ctx, 1);
        duk_put_prop_string(ctx, idx, "level");
    }
    /*
     * Toggle function
     */
    duk_push_c_function(ctx, NativeTogglePin, 0);
    duk_put_prop_string(ctx, idx, "toggle");
    /*
     * Only allow if the PWM functions is supported
     */
    if (AJS_TargetIO_GetInfo(pin)->functions & AJS_IO_FUNCTION_PWM) {
        duk_push_c_function(ctx, NativePWM, 2);
        duk_put_prop_string(ctx, idx, "pwm");
    }
    /*
     * Return the digital output pin object
     */
    return 1;
}

/*
 * Configures a pin as a digital input pin
 */
static int NativeIoDigitalIn(duk_context* ctx)
{
    AJ_Status status;
    int idx;
    void* pinCtx;
    int config = -1;
    uint32_t pin = GetPinId(ctx, 0, AJS_IO_FUNCTION_DIGITAL_IN);

    if (duk_is_undefined(ctx, 1)) {
        config = AJS_IO_PIN_PULL_UP;
    } else if (duk_is_number(ctx, 1)) {
        config = (AJS_IO_PinConfig)duk_get_int(ctx, 1);
    }
    if ((config != AJS_IO_PIN_OPEN_DRAIN) && (config != AJS_IO_PIN_PULL_UP) && (config != AJS_IO_PIN_PULL_DOWN)) {
        duk_error(ctx, DUK_ERR_RANGE_ERROR, "Configuration must be pullUp, pullDown, or openDrain");
    }
    /*
     * Target specific I/O pin initialization
     */
    status = AJS_TargetIO_PinOpen(pin, (AJS_IO_PinConfig)config, &pinCtx);
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "Failed to open digital input pin: %s", AJ_StatusText(status));
    }
    idx = NewIOObject(ctx, pinCtx, NativePinFinalizer);
    /*
     * Function to get the pin level
     */
    AJS_SetPropertyAccessors(ctx, idx, "level", NULL, NativeLevelGetter);
    /*
     * Function to set a trigger
     */
    duk_push_c_function(ctx, NativeSetTrigger, 3);
    duk_put_prop_string(ctx, idx, "setTrigger");
    /*
     * Return the digital input pin object
     */
    return 1;
}

static int NativeIoAdcGetter(duk_context* ctx)
{
    duk_push_number(ctx, AJS_TargetIO_AdcRead(PinCtxPtr(ctx)));
    return 1;
}

static int NativeAdcFinalizer(duk_context* ctx)
{
    AJ_InfoPrintf(("Closing ADC\n"));
    duk_get_prop_string(ctx, 0, AJS_HIDDEN_PROP("ctx"));
    AJS_TargetIO_AdcClose(duk_require_pointer(ctx, -1));
    duk_pop(ctx);
    return 0;
}

static int NativeIoAnalogIn(duk_context* ctx)
{
    AJ_Status status;
    int idx;
    void* adcCtx;
    uint32_t pin = GetPinId(ctx, 0, AJS_IO_FUNCTION_ANALOG_IN);

    status = AJS_TargetIO_AdcOpen(pin, &adcCtx);
    if (status == AJ_ERR_INVALID) {
        duk_error(ctx, DUK_ERR_UNSUPPORTED_ERROR, "Analog input function not supported on pin%d", pin);
    }
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "Failed to configure analog digital input pin: %s", AJ_StatusText(status));
    }
    idx = NewIOObject(ctx, adcCtx, NativeAdcFinalizer);
    /*
     * Function to read the ADC
     */
    AJS_SetPropertyAccessors(ctx, idx, "value", NULL, NativeIoAdcGetter);
    /*
     * Return the ADC object
     */
    return 1;
}

static int NativeIoDacSetter(duk_context* ctx)
{
    AJS_TargetIO_DacWrite(PinCtxPtr(ctx), duk_require_int(ctx, 0));
    return 0;
}

static int NativeDacFinalizer(duk_context* ctx)
{
    AJ_InfoPrintf(("Closing DAC\n"));
    duk_get_prop_string(ctx, 0, AJS_HIDDEN_PROP("ctx"));
    AJS_TargetIO_AdcClose(duk_require_pointer(ctx, -1));
    duk_pop(ctx);
    return 0;
}

static int NativeIoAnalogOut(duk_context* ctx)
{
    AJ_Status status;
    int idx;
    void* dacCtx;
    uint32_t pin = GetPinId(ctx, 0, AJS_IO_FUNCTION_ANALOG_OUT);

    status = AJS_TargetIO_DacOpen(pin, &dacCtx);
    if (status == AJ_ERR_INVALID) {
        duk_error(ctx, DUK_ERR_UNSUPPORTED_ERROR, "Analog input function not supported on pin%d", pin);
    }
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "Failed to configure analog digital input pin: %s", AJ_StatusText(status));
    }
    idx = NewIOObject(ctx, dacCtx, NativeDacFinalizer);
    /*
     * Function to set the DAC
     */
    AJS_SetPropertyAccessors(ctx, idx, "value", NativeIoDacSetter, NULL);
    /*
     * Return the DAC object
     */
    return 1;
}

static int NativeIoSystem(duk_context* ctx)
{
    AJ_Status status;
    const char* cmd = duk_require_string(ctx, 0);

    status = AJS_TargetIO_System(cmd);
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_UNSUPPORTED_ERROR, "System '%s'", cmd);
    }
    return 1;
}

/*
 * Serializes various data types into a buffer. Leaves the buffer on the top of the stack.
 */
static uint8_t* SerializeToBuffer(duk_context* ctx, duk_idx_t idx, duk_size_t*sz)
{
    uint8_t* ptr;

    switch (duk_get_type(ctx, idx)) {
    case DUK_TYPE_BUFFER:
        duk_dup(ctx, idx);
        ptr = duk_get_buffer(ctx, -1, sz);
        break;

    case DUK_TYPE_BOOLEAN:
        ptr = duk_push_fixed_buffer(ctx, 1);
        ptr[0] = duk_get_boolean(ctx, idx);
        *sz = 1;
        break;

    case DUK_TYPE_NUMBER:
        ptr = duk_push_fixed_buffer(ctx, 1);
        ptr[0] = duk_get_int(ctx, idx);
        *sz = 1;
        break;

    case DUK_TYPE_STRING:
        duk_dup(ctx, idx);
        ptr = duk_to_fixed_buffer(ctx, -1, sz);
        break;

    case DUK_TYPE_OBJECT:
        if (duk_is_array(ctx, idx)) {
            duk_idx_t i;
            duk_idx_t len = duk_get_length(ctx, idx);
            ptr = duk_push_fixed_buffer(ctx, len);
            for (i = 0; i < len; ++i) {
                duk_get_prop_index(ctx, idx, i);
                ptr[i] = duk_require_uint(ctx, -1);
                duk_pop(ctx);
            }
            *sz = len;
        } else {
            duk_error(ctx, DUK_ERR_TYPE_ERROR, "Can only serialize arrays of numbers");
        }
        break;

    default:
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Cannot serialize");
        break;
    }
    return ptr;
}

static int NativeSpiFinalizer(duk_context* ctx)
{
    AJ_InfoPrintf(("Closing SPI\n"));
    duk_get_prop_string(ctx, 0, AJS_HIDDEN_PROP("ctx"));
    AJS_TargetIO_SpiClose(duk_require_pointer(ctx, -1));
    duk_pop(ctx);
    return 0;
}

static int NativeSpiRead(duk_context* ctx)
{
    int size = duk_require_int(ctx, -1);
    void* ptrIn = AJS_TargetIO_SpiRead(PinCtxPtr(ctx), size);
    void* ptrOut = duk_push_fixed_buffer(ctx, size);
    memcpy(ptrOut, ptrIn, size);
    return 1;
}

static int NativeSpiWrite(duk_context* ctx)
{
    duk_size_t len;
    uint8_t* ptr = SerializeToBuffer(ctx, 0, &len);
    AJS_TargetIO_SpiWrite(PinCtxPtr(ctx), ptr, len);
    duk_pop(ctx);
    return 0;
}

static int NativeIoSpi(duk_context* ctx)
{
    uint32_t mosi, miso, cs, clk, prescaler;
    uint8_t master;
    uint8_t cpol, cpha, data;
    void* spiCtx;
    AJ_Status status;
    int idx;

    mosi = GetPinId(ctx, 0, AJS_IO_FUNCTION_SPI_MOSI);
    miso = GetPinId(ctx, 1, AJS_IO_FUNCTION_SPI_MISO);
    cs = GetPinId(ctx, 2, AJS_IO_FUNCTION_SPI_SS);
    clk = GetPinId(ctx, 3, AJS_IO_FUNCTION_SPI_SCK);

    duk_get_prop_string(ctx, 4, "prescaler");
    prescaler = duk_require_int(ctx, -1);
    duk_get_prop_string(ctx, 4, "master");
    if (duk_get_boolean(ctx, -1)) {
        master = TRUE;
    } else {
        master = FALSE;
    }
    duk_get_prop_string(ctx, 4, "cpol");
    cpol = duk_require_int(ctx, -1);
    duk_get_prop_string(ctx, 4, "cpha");
    cpha = duk_require_int(ctx, -1);
    duk_get_prop_string(ctx, 4, "data");
    data = duk_require_int(ctx, -1);
    AJ_Printf("Clock = %u\n", prescaler);
    AJ_Printf("Master? = %u\n", master);
    AJ_Printf("cpol = %u\n", cpol);
    AJ_Printf("cpha = %u\n", cpha);
    AJ_Printf("data = %u\n", data);
    duk_pop(ctx);

    // Clock polarity must be high (1) or low (0)
    if (cpol != 0 && cpol != 1) {
        duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "cpol must be 0 or 1; you gave %u", cpol);
    }
    // Clock edge phase must be one edge (1) or two edge (2)
    if (cpha != 1 && cpha != 2) {
        duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "cpha must be 1 or 2; you gave %u", cpha);
    }
    // Data bits must be 8, 16, or 32
    if (data != 8 && data != 16) {
        duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "data must be 8 or 16; you gave %u", data);
    }
    if (prescaler != 2 && prescaler != 4 && prescaler != 8 &&
        prescaler != 16 && prescaler != 32 && prescaler != 64 &&
        prescaler != 128 && prescaler != 256) {
        duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "prescaler must be 2, 4, 8, 16, 32, 64, 128 or 256; you gave %u", prescaler);
    }

    status = AJS_TargetIO_SpiOpen(mosi, miso, cs, clk, prescaler, master, cpol, cpha, data, &spiCtx);
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "Failed to open SPI device");
    }
    idx = NewIOObject(ctx, spiCtx, NativeSpiFinalizer);

    duk_push_c_function(ctx, NativeSpiRead, 1);
    duk_put_prop_string(ctx, idx, "read");

    duk_push_c_function(ctx, NativeSpiWrite, 1);
    duk_put_prop_string(ctx, idx, "write");

    return 1;
}

static int NativeUartFinalizer(duk_context* ctx)
{
    AJ_InfoPrintf(("Closing UART\n"));
    duk_get_prop_string(ctx, 0, AJS_HIDDEN_PROP("ctx"));
    AJS_TargetIO_UartClose(duk_require_pointer(ctx, -1));
    duk_pop(ctx);
    return 0;
}

static int NativeUartRead(duk_context* ctx)
{
    int size = duk_require_int(ctx, -1);
    void* ptrIn = AJS_TargetIO_UartRead(PinCtxPtr(ctx), size);
    void* ptrOut = duk_push_fixed_buffer(ctx, size);
    memcpy(ptrOut, ptrIn, size);
    return 1;
}

static int NativeUartWrite(duk_context* ctx)
{
    duk_size_t len;
    uint8_t* ptr = SerializeToBuffer(ctx, 0, &len);
    AJS_TargetIO_UartWrite(PinCtxPtr(ctx), ptr, len);
    duk_pop(ctx);
    return 0;
}

static int NativeIoUart(duk_context* ctx)
{
    AJ_Status status;
    uint8_t tx, rx;
    uint32_t baud;
    void* uartCtx;
    int idx;

    tx = GetPinId(ctx, 0, AJS_IO_FUNCTION_UART_TX);
    rx = GetPinId(ctx, 1, AJS_IO_FUNCTION_UART_RX);
    baud = duk_require_int(ctx, 2);
    duk_pop(ctx);
    status = AJS_TargetIO_UartOpen(tx, rx, baud, &uartCtx);
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "Failed to open USART device\n");
    }
    idx = NewIOObject(ctx, uartCtx, NativeUartFinalizer);

    duk_push_c_function(ctx, NativeUartRead, 1);
    duk_put_prop_string(ctx, idx, "read");

    duk_push_c_function(ctx, NativeUartWrite, 1);
    duk_put_prop_string(ctx, idx, "write");

    return 1;
}

static int NativeI2cTransfer(duk_context* ctx)
{
    AJ_Status status;
    uint8_t addr = duk_require_int(ctx, 0);
    uint8_t* txBuf = NULL;
    uint8_t* rxBuf = NULL;
    duk_size_t txLen = 0;
    duk_size_t rxLen = 0;
    uint8_t rxBytes = 0;

    if (duk_is_undefined(ctx, 2)) {
        duk_push_undefined(ctx);
    } else {
        rxLen = duk_require_uint(ctx, 2);
        rxBuf = duk_push_dynamic_buffer(ctx, rxLen);
    }
    if (duk_is_null(ctx, 1)) {
        duk_push_undefined(ctx);
    } else {
        txBuf = SerializeToBuffer(ctx, 1, &txLen);
    }
    status = AJS_TargetIO_I2cTransfer(PinCtxPtr(ctx), addr, txBuf, txLen, rxBuf, rxLen, &rxBytes);
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "I2C transfer failed %s\n", AJ_StatusText(status));
    }
    duk_pop(ctx);
    if (rxLen) {
        duk_resize_buffer(ctx, -1, rxBytes);
    }
    return 1;
}

static int NativeI2cFinalizer(duk_context* ctx)
{
    AJ_InfoPrintf(("Closing i2c\n"));
    duk_get_prop_string(ctx, 0, AJS_HIDDEN_PROP("ctx"));
    AJS_TargetIO_I2cClose(duk_require_pointer(ctx, -1));
    duk_pop(ctx);
    return 0;
}

/*
 * Uses "magic" property to determine if i2c is being configured as master or slave.
 */
static int NativeIoI2c(duk_context* ctx)
{
    AJ_Status status;
    uint8_t address = 0;
    uint32_t clock = 0;
    void* i2cCtx;
    uint8_t mode = duk_get_current_magic(ctx);
    uint8_t sda = GetPinId(ctx, 0, AJS_IO_FUNCTION_I2C_SDA);
    uint8_t scl = GetPinId(ctx, 1, AJS_IO_FUNCTION_I2C_SCL);

    if (mode == AJS_I2C_MODE_MASTER) {
        if (!duk_is_undefined(ctx, 2)) {
            clock = duk_require_int(ctx, 2);
        }
    } else {
        address = duk_require_int(ctx, 2);
    }
    status = AJS_TargetIO_I2cOpen(sda, scl, clock, mode, address, &i2cCtx);
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_INTERNAL_ERROR, "Failed to open I2C device\n");
    }
    NewIOObject(ctx, i2cCtx, NativeI2cFinalizer);
    duk_push_c_function(ctx, NativeI2cTransfer, 3);
    duk_put_prop_string(ctx, -2, "transfer");

    return 1;
}

/*
 * Returns the IO functions supported by this pin
 */
static int NativeFunctionsGetter(duk_context* ctx)
{
    uint8_t bit;
    uint8_t numFuncs = 0;
    const AJS_IO_Info* info;
    uint32_t pin;

    duk_push_this(ctx);
    pin = GetPinId(ctx, -1, 0);
    duk_pop(ctx);

    info = AJS_TargetIO_GetInfo(pin);
    duk_push_string(ctx, ", ");
    /*
     * Test each function bit
     */
    for (bit = 0; bit < 32; ++bit) {
        if (info->functions & (1 << bit)) {
            const char* name = AJS_IO_FunctionName(1 << bit);
            duk_push_string(ctx, name);
            ++numFuncs;
        }
    }
    duk_join(ctx, numFuncs);
    return 1;
}

/*
 * Returns information about this pin
 */
static int NativeInfoGetter(duk_context* ctx)
{
    int idx;
    uint32_t pin;
    const AJS_IO_Info* info;

    duk_push_this(ctx);
    pin = GetPinId(ctx, -1, 0);
    duk_pop(ctx);

    info = AJS_TargetIO_GetInfo(pin);

    idx = duk_push_object(ctx);
    duk_push_int(ctx, info->physicalPin);
    duk_put_prop_string(ctx, idx, "physicalPin");
    duk_push_string(ctx, info->schematicId);
    duk_put_prop_string(ctx, idx, "schematicId");
    duk_push_string(ctx, info->datasheetId);
    duk_put_prop_string(ctx, idx, "datasheetId");
    duk_push_string(ctx, info->description);
    duk_put_prop_string(ctx, idx, "description");
    /*
     * Return the object we just created
     */
    return 1;
}

AJ_Status AJS_RegisterIO(duk_context* ctx)
{
    duk_idx_t ioIdx;
    duk_idx_t pinIdx;
    duk_idx_t i;
    uint16_t numPins = AJS_TargetIO_GetNumPins();

    duk_push_global_object(ctx);
    ioIdx = duk_push_object(ctx);

    /*
     * Create base pin protoype
     */
    pinIdx = duk_push_object(ctx);
    AJS_SetPropertyAccessors(ctx, pinIdx, "functions", NULL, NativeFunctionsGetter);
    AJS_SetPropertyAccessors(ctx, pinIdx, "info", NULL, NativeInfoGetter);
    /*
     * Create the pin objects
     */
    duk_push_array(ctx);
    for (i = 0; i < numPins; ++i) {
        AJS_CreateObjectFromPrototype(ctx, pinIdx);
        duk_push_int(ctx, i);
        duk_put_prop_string(ctx, -2, "id");
        duk_put_prop_index(ctx, -2, i);
    }
    duk_put_prop_string(ctx, ioIdx, "pin");
    duk_pop(ctx);

    duk_push_c_function(ctx, NativeIoDigitalIn, 2);
    duk_put_prop_string(ctx, ioIdx, "digitalIn");

    duk_push_c_function(ctx, NativeIoDigitalOut, 2);
    duk_put_prop_string(ctx, ioIdx, "digitalOut");

    duk_push_c_function(ctx, NativeIoAnalogIn, 2);
    duk_put_prop_string(ctx, ioIdx, "analogIn");

    duk_push_c_function(ctx, NativeIoAnalogOut, 2);
    duk_put_prop_string(ctx, ioIdx, "analogOut");

    duk_push_c_function(ctx, NativeIoSystem, 1);
    duk_put_prop_string(ctx, ioIdx, "system");

    duk_push_c_function(ctx, NativeIoSpi, 5);
    duk_put_prop_string(ctx, ioIdx, "spi");

    duk_push_c_function(ctx, NativeIoUart, 3);
    duk_put_prop_string(ctx, ioIdx, "uart");

    duk_push_c_function(ctx, NativeIoI2c, 3);
    duk_set_magic(ctx, -1, AJS_I2C_MODE_MASTER);
    duk_put_prop_string(ctx, ioIdx, "i2cMaster");

    duk_push_c_function(ctx, NativeIoI2c, 3);
    duk_set_magic(ctx, -1, AJS_I2C_MODE_SLAVE);
    duk_put_prop_string(ctx, ioIdx, "i2cSlave");
    /*
     * GPIO attribute constants
     */
    duk_push_int(ctx, AJS_IO_PIN_OPEN_DRAIN);
    duk_put_prop_string(ctx, ioIdx, "openDrain");
    duk_push_int(ctx, AJS_IO_PIN_PULL_UP);
    duk_put_prop_string(ctx, ioIdx, "pullUp");
    duk_push_int(ctx, AJS_IO_PIN_PULL_DOWN);
    duk_put_prop_string(ctx, ioIdx, "pullDown");
    duk_push_int(ctx, AJS_IO_PIN_TRIGGER_ON_RISE);
    duk_put_prop_string(ctx, ioIdx, "risingEdge");
    duk_push_int(ctx, AJS_IO_PIN_TRIGGER_ON_FALL);
    duk_put_prop_string(ctx, ioIdx, "fallingEdge");
    /*
     * Property for keeping track of triggers
     */
    duk_push_array(ctx);
    duk_put_prop_string(ctx, ioIdx, AJS_HIDDEN_PROP("trigs"));

    duk_put_prop_string(ctx, -2, "IO");
    duk_pop(ctx);

    return AJ_OK;
}

AJ_Status AJS_ServiceIO(duk_context* ctx)
{
    int32_t trigId;
    uint32_t level;

    trigId = AJS_TargetIO_PinTrigId(&level);
    if (trigId != AJS_IO_PIN_NO_TRIGGER) {
        AJ_InfoPrintf(("triggered on id %d\n", trigId));
        /*
         * Lookup the pin object in the triggers array
         */
        duk_push_global_object(ctx);
        duk_get_prop_string(ctx, -1, "IO");
        duk_get_prop_string(ctx, -1, AJS_HIDDEN_PROP("trigs"));
        do {
            duk_get_prop_index(ctx, -1, trigId);
            if (duk_is_object(ctx, -1)) {
                /*
                 * Call the trigger function passing the pin object and level as arguments
                 */
                duk_get_prop_string(ctx, -1, "trigger");
                duk_dup(ctx, -2);
                duk_push_int(ctx, level);
                if (duk_pcall(ctx, 2) != DUK_EXEC_SUCCESS) {
                    AJS_ConsoleSignalError(ctx);
                }
                /*
                 * Pop the call return value
                 */
                duk_pop(ctx);
            } else {
                AJ_ErrPrintf(("Expected a pin object trigId = %d\n", trigId));
            }
            duk_pop(ctx);
            trigId = AJS_TargetIO_PinTrigId(&level);
        } while (trigId != AJS_IO_PIN_NO_TRIGGER);
        duk_pop_3(ctx);
    }
    return AJ_OK;
}

const char* AJS_IO_FunctionName(uint32_t function)
{
    switch (function) {
    case AJS_IO_FUNCTION_DIGITAL_IN:
        return "digitalIn";

    case AJS_IO_FUNCTION_DIGITAL_OUT:
        return "digitalOut";

    case AJS_IO_FUNCTION_PWM:
        return "pwm";

    case AJS_IO_FUNCTION_ANALOG_IN:
        return "analogIn";

    case AJS_IO_FUNCTION_ANALOG_OUT:
        return "analogOut";

    case AJS_IO_FUNCTION_UART_TX:
        return "uartTx";

    case AJS_IO_FUNCTION_UART_RX:
        return "uartRx";

    case AJS_IO_FUNCTION_I2C_SDA:
        return "i2cSDA";

    case AJS_IO_FUNCTION_I2C_SCL:
        return "i2cSCL";

    case AJS_IO_FUNCTION_SPI_SCK:
        return "spiSCK";

    case AJS_IO_FUNCTION_SPI_SS:
        return "spiSS";

    case AJS_IO_FUNCTION_SPI_MOSI:
        return "spiMOSI";

    case AJS_IO_FUNCTION_SPI_MISO:
        return "spiMISO";

    case AJS_IO_FUNCTION_I2S_CLK:
        return "i2sCLK";

    case AJS_IO_FUNCTION_I2S_WS:
        return "i2sWS";

    case AJS_IO_FUNCTION_I2S_SDO:
        return "i2sSDO";

    case AJS_IO_FUNCTION_I2S_SDI:
        return "i2sSDI";

    default:
        return NULL;
    }
}
