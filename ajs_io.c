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
    duk_get_prop_string(ctx, -1, "$ctx$");
    pinCtx = duk_require_pointer(ctx, -1);
    duk_pop(ctx);
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
    duk_put_prop_string(ctx, idx, "$ctx$");
    /*
     * Callback from when the pin object is garbage collected.
     */
    AJS_RegisterFinalizer(ctx, idx, finalizer);
    return idx;
}

static int NativeSetTrigger(duk_context* ctx)
{
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
    AJS_TargetIO_PinEnableTrigger(PinCtxPtr(ctx), mode, &trigId, debounce);

    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, "IO");
    duk_get_prop_string(ctx, -1, "$triggers$");

    if ((mode == AJS_IO_PIN_TRIGGER_ON_RISE) || (mode == AJS_IO_PIN_TRIGGER_ON_FALL)) {
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
    uint32_t freq = duk_require_int(ctx, 1);

    if (dutyCycle > 1.0 || dutyCycle < 0.0) {
        duk_error(ctx, DUK_ERR_RANGE_ERROR, "Duty cycle must be in the range 0.0 to 1.0");
    }
    if (freq == 0) {
        duk_error(ctx, DUK_ERR_RANGE_ERROR, "Frequency cannot be zero Hz");
    }
    status = AJS_TargetIO_PinPWM(PinCtxPtr(ctx), dutyCycle, freq);
    if (status != AJ_OK) {
        duk_error(ctx, DUK_ERR_RANGE_ERROR, "Too many PWM pins");
    }
    return 0;
}

static int NativePinFinalizer(duk_context* ctx)
{
    AJ_InfoPrintf(("Closing Pin\n"));
    AJS_TargetIO_PinClose(PinCtxPtr(ctx));
    return 0;
}

/*
 * Returns the pid id for a pin object. Also checks that the pin supports the requested function.
 * Note that we number pins from 1 in JavaScript (where it is just a name) but the pin identifier in
 * the C code is zero based because it is used as an array index. This may seem to add an
 * unnecessary complexity but in hardware pins are always numbered starting at 1.
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
    id = duk_require_int(ctx, -1) - 1;
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
    AJS_IO_PinConfig config;
    uint32_t pin = GetPinId(ctx, 0, AJS_IO_FUNCTION_DIGITAL_IN);

    if (!duk_is_number(ctx, 1)) {
        duk_error(ctx, DUK_ERR_RANGE_ERROR, "Configuration must be pullUp, pullDown, or openDrain");
    }
    config = (AJS_IO_PinConfig)duk_get_int(ctx, 1);
    /*
     * Target specific I/O pin initialization
     */
    status = AJS_TargetIO_PinOpen(pin, config, &pinCtx);
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
    AJS_TargetIO_AdcClose(PinCtxPtr(ctx));
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
    AJS_TargetIO_AdcClose(PinCtxPtr(ctx));
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
    while (numPins) {
        duk_push_sprintf(ctx, "pin%d", numPins);
        AJS_CreateObjectFromPrototype(ctx, pinIdx);
        duk_push_int(ctx, numPins);
        duk_put_prop_string(ctx, -2, "id");
        duk_put_prop(ctx, ioIdx);
        --numPins;
    }
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
    duk_put_prop_string(ctx, ioIdx, "$triggers$");

    duk_put_prop_string(ctx, -2, "IO");
    duk_pop(ctx);

    return AJ_OK;
}

AJ_Status AJS_ServiceIO(duk_context* ctx)
{
    int32_t trigId;

    trigId = AJS_TargetIO_PinTrigId();
    if (trigId != AJS_IO_PIN_NO_TRIGGER) {
        AJ_InfoPrintf(("triggered on id %d\n", trigId));
        /*
         * Lookup the pin object in the triggers array
         */
        duk_push_global_object(ctx);
        duk_get_prop_string(ctx, -1, "IO");
        duk_get_prop_string(ctx, -1, "$triggers$");
        do {
            duk_get_prop_index(ctx, -1, trigId);
            if (duk_is_object(ctx, -1)) {
                /*
                 * Call the trigger function passing the pin object as an argument
                 */
                duk_get_prop_string(ctx, -1, "trigger");
                duk_dup(ctx, -2);
                if (duk_pcall(ctx, 1) != DUK_EXEC_SUCCESS) {
                    AJS_ConsoleSignalError(ctx);
                }
                /*
                 * Pop the call return value
                 */
                duk_pop(ctx);
            } else {
                AJ_ErrPrintf(("Expected a pin object\n"));
            }
            duk_pop(ctx);
            trigId = AJS_TargetIO_PinTrigId();
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

    case AJS_IO_FUNCTION_I2C_SDC:
        return "i2cSDC";

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
