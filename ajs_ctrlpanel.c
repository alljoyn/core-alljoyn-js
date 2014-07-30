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
#include "ajs_util.h"
#include "ajs_translations.h"
#include "ajs_ctrlpanel.h"

static int NativeContainerWidget(duk_context* ctx);

/*
 * Leaves the widget (this) object on the stack
 */
static AJS_Widget* GetWidgetFromThis(duk_context* ctx)
{
    AJS_Widget* widget;

    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, AJS_HIDDEN_PROP("wbuf"));
#ifndef NDEBUG
    if (!duk_is_buffer(ctx, -1)) {
        AJ_ErrPrintf(("GetWidgetFromThis \"this\" is not a widget\n"));
    }
#endif
    widget = duk_require_buffer(ctx, -1, NULL);
    duk_pop(ctx);
    return widget;
}

static AJS_Widget* CreateWidget(duk_context* ctx, uint8_t type)
{
    AJS_Widget* widget;

    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, AJS_AllJoynObject);
    duk_get_prop_string(ctx, -1, "controlPanel");
    duk_get_prop_string(ctx, -1, "baseWidget");
    /*
     * Create a new object from this protoype
     */
    AJS_CreateObjectFromPrototype(ctx, -1);
    /*
     * Clean up stack leaving new widget on the top
     */
    duk_swap_top(ctx, -5);
    duk_pop_n(ctx, 4);
    /*
     * Allocate a buffer for the CPS widget and set it
     */
    widget = (AJS_Widget*)duk_push_fixed_buffer(ctx, sizeof(AJS_Widget));
    widget->type = type;
    widget->dukCtx = ctx;
    duk_put_prop_string(ctx, -2, AJS_HIDDEN_PROP("wbuf"));
    return widget;
}

static const char* WidgetTypeTxt(uint8_t type)
{
    switch (type) {
    case WIDGET_TYPE_LABEL:
        return "LABEL";

    case WIDGET_TYPE_PROPERTY:
        return "PROPERTY";

    case WIDGET_TYPE_DIALOG:
        return "DIALOG";

    case WIDGET_TYPE_ACTION:
        return "ACTION";

    case WIDGET_TYPE_CONTAINER:
        return "CONTAINER";

    default:
        return "";
    }
}

/*
 * TODO - this is really cumbersome and expensive - perhaps better to just set the widget list as a
 * property on the AJ object.
 */
static void GetWidgetObject(duk_context* ctx, int index)
{
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "widgets");
    duk_get_prop_index(ctx, -1, index);
    duk_swap_top(ctx, -3);
    duk_pop_2(ctx);
    AJ_ASSERT(duk_is_object(ctx, -1));
}

static const char* GetLabel(BaseWidget* widget, uint16_t language)
{
    AJS_Widget* ajsWidget = (AJS_Widget*)widget;
    duk_context* ctx = ajsWidget->dukCtx;
    const char* label;

    AJ_InfoPrintf(("GetLabel from %s\n", ajsWidget->path));
    GetWidgetObject(ctx, ajsWidget->index);
    duk_get_prop_string(ctx, -1, "label");
    label = AJS_GetTranslatedString(ctx, -1, language);
    duk_pop_2(ctx);
    /*
     * String pointers are stable so we can simply return it
     */
    return label;
}

static const char* GetLabelWidgetLabel(LabelWidget* widget, uint16_t language)
{
    return GetLabel((BaseWidget*)widget, language);
}

static void* GetValue(PropertyWidget* widget)
{
    AJS_Widget* ajsWidget = (AJS_Widget*)widget;
    AJ_InfoPrintf(("GetValue from %s\n", ajsWidget->path));
    return &ajsWidget->property.val;
}

static AJS_Widget* AddWidget(duk_context* ctx, uint8_t type)
{
    size_t num;
    const char* parent;
    AJS_Widget* widget = CreateWidget(ctx, type);
    duk_idx_t wIdx = duk_get_top_index(ctx);
    /*
     * Get widgets array index for the new widget
     */
    duk_get_prop_string(ctx, wIdx, "widgets");
    num = duk_get_length(ctx, -1);
    /*
     * We need the parent of this widget so we can generate the object path
     */
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "path");
    parent = duk_require_string(ctx, -1);
    duk_pop_2(ctx);
    /*
     * Set the object path on the widget. Dialogs are not child objects of the control panel.
     */
    if ((type == WIDGET_TYPE_DIALOG) && (strcmp(parent, "/ControlPanel") == 0)) {
        duk_push_sprintf(ctx, "/%d", WidgetTypeTxt(type), num);
    } else {
        duk_push_sprintf(ctx, "%s/%s%d", parent, WidgetTypeTxt(type), num);
    }
#ifndef NDEBUG
    widget->path = duk_get_string(ctx, -1);
    AJ_InfoPrintf(("Adding widget %s\n", widget->path));
#endif
    duk_put_prop_string(ctx, wIdx, "path");
    /*
     * Add the widget to the widgets array (still at top of stack)
     */
    duk_dup(ctx, wIdx);
    AJ_ASSERT(duk_is_array(ctx, -2));
    duk_put_prop_index(ctx, -2, (duk_uarridx_t)num);
    /*
     * Pop widgets array leaving newly created widget on the top of the duk stack
     */
    duk_pop(ctx);
    /*
     * Per widget type initiliazation
     */
    switch (type) {
    case WIDGET_TYPE_LABEL:
        initializeLabelWidget(&widget->label);
        widget->label.getLabel = GetLabelWidgetLabel;
        break;

    case WIDGET_TYPE_PROPERTY:
        initializePropertyWidget(&widget->property.wdt);
        setBaseWritable(&widget->base, TRUE);
        break;

    case WIDGET_TYPE_DIALOG:
        initializeDialogWidget(&widget->dialog);
        break;

    case WIDGET_TYPE_ACTION:
        initializeActionWidget(&widget->action);
        break;

    case WIDGET_TYPE_CONTAINER:
        initializeContainerWidget(&widget->container);
        break;
    }
    /*
     * Common initialization
     */
    widget->base.optParams.hints = widget->hints;
    widget->base.optParams.numHints = 0;
    widget->base.numLanguages = 1;
    widget->index = (uint16_t)num;
    widget->base.optParams.bgColor = -1;
    /*
     * All widgets start out enabled
     */
    setBaseEnabled(&widget->base, TRUE);
    return widget;
}

static int NativeLabelWidget(duk_context* ctx)
{
    AddWidget(ctx, WIDGET_TYPE_LABEL);
    /*
     * Set the initial label on the object. This will call the widget label setter.
     */
    duk_dup(ctx, 0);
    duk_put_prop_string(ctx, -2, "label");
    /*
     * The new object is the return value - leave it on the stack
     */
    return 1;
}

static void SetIntProp(duk_context* ctx, int idx, const char* prop, int val)
{
    idx = duk_normalize_index(ctx, idx);
    duk_push_int(ctx, val);
    duk_put_prop_string(ctx, idx, prop);
}

static int ReqIntProp(duk_context* ctx, int idx, const char* prop)
{
    int val;
    duk_get_prop_string(ctx, idx, prop);
    val = duk_require_int(ctx, -1);
    duk_pop(ctx);
    return val;
}

/*
 * Determine if a number at the given index is an integer value
 */
static uint8_t IsInt(duk_context* ctx, int idx)
{
    double n = duk_require_number(ctx, idx);
    return n == (double)(int)(n);
}

#define FIT_VAL(x, min, max)  ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

static void FitValueInRange(AJS_Widget* widget)
{
    if (widget->property.wdt.signature[0] == 'i') {
        int32_t min = *((int32_t*)widget->property.wdt.optParams.constraintRange.minValue);
        int32_t max = *((int32_t*)widget->property.wdt.optParams.constraintRange.maxValue);
        int32_t inc = *((int32_t*)widget->property.wdt.optParams.constraintRange.increment);
        int32_t i = inc * ((widget->property.val.i + inc / 2) / inc);
        widget->property.val.i = FIT_VAL(i, min, max);
        AJ_InfoPrintf(("Range adjusted value = %d\n", i));
    } else {
        double min = *((double*)widget->property.wdt.optParams.constraintRange.minValue);
        double max = *((double*)widget->property.wdt.optParams.constraintRange.maxValue);
        double inc = *((double*)widget->property.wdt.optParams.constraintRange.increment);
        double d = inc * ((widget->property.val.d + inc / 2) / inc);
        widget->property.val.d = FIT_VAL(d, min, max);
        AJ_InfoPrintf(("Range adjusted value = %f\n", d));
    }
}

static int NativeValueSetter(duk_context* ctx)
{
    AJS_Widget* widget = GetWidgetFromThis(ctx);

    AJ_InfoPrintf(("Native value setter called for %s\n", widget->path));

    switch (widget->hints[0]) {
    case PROPERTY_WIDGET_HINT_SWITCH:
        widget->property.val.b = duk_require_boolean(ctx, 0);
        AJ_InfoPrintf(("Value = %d\n", widget->property.val.b));
        break;

    case PROPERTY_WIDGET_HINT_CHECKBOX:
    case PROPERTY_WIDGET_HINT_SPINNER:
    case PROPERTY_WIDGET_HINT_RADIOBUTTON:
    case PROPERTY_WIDGET_HINT_KEYPAD:
        widget->property.val.q = duk_require_int(ctx, 0);
        AJ_InfoPrintf(("Value = %d\n", widget->property.val.q));
        break;

    case PROPERTY_WIDGET_HINT_SLIDER:
    case PROPERTY_WIDGET_HINT_ROTARYKNOB:
    case PROPERTY_WIDGET_HINT_NUMERICVIEW:
    case PROPERTY_WIDGET_HINT_NUMBERPICKER:
        if (widget->property.wdt.signature[0] == 'i') {
            widget->property.val.i = duk_get_int(ctx, 0);
            AJ_InfoPrintf(("Value = %d\n", widget->property.val.i));
        } else {
            widget->property.val.d = duk_require_number(ctx, 0);
            AJ_InfoPrintf(("Value = %f\n", widget->property.val.d));
        }
        if (widget->property.wdt.optParams.constraintRangeDefined) {
            FitValueInRange(widget);
        }
        break;

    case PROPERTY_WIDGET_HINT_TIMEPICKER:
        widget->property.val.time.hour = ReqIntProp(ctx, 0, "hour");
        widget->property.val.time.minute = ReqIntProp(ctx, 0, "minute");
        widget->property.val.time.second = ReqIntProp(ctx, 0, "second");
        AJ_InfoPrintf(("Value = %d:%d.%d", widget->property.val.time.hour, widget->property.val.time.minute, widget->property.val.time.second));
        break;

    case PROPERTY_WIDGET_HINT_DATEPICKER:
        widget->property.val.date.mDay = ReqIntProp(ctx, 0, "day");
        widget->property.val.date.month = ReqIntProp(ctx, 0, "month");
        widget->property.val.date.fullYear = ReqIntProp(ctx, 0, "year");
        if (widget->property.val.date.fullYear < 100) {
            widget->property.val.date.fullYear += 2000;
        }
        AJ_InfoPrintf(("Value = %d/%d/%d", widget->property.val.date.month, widget->property.val.date.mDay, widget->property.val.date.fullYear));
        break;

    case PROPERTY_WIDGET_HINT_TEXTVIEW:
    case PROPERTY_WIDGET_HINT_EDITTEXT:
        /*
         * String cases
         */
        widget->property.val.s = duk_require_string(ctx, 0);
        AJ_InfoPrintf(("Value = \"%s\"\n", widget->property.val.s));
        /*
         * String needs to be stabilized so we need to set a property
         */
        duk_dup(ctx, 0);
        duk_put_prop_string(ctx, -2, AJS_HIDDEN_PROP("value"));
        break;
    }
    duk_pop(ctx);
    AJS_CPS_SignalValueChanged(AJS_GetBusAttachment(), widget);
    return 0;
}

static int ValueGetter(duk_context* ctx, AJS_Widget* widget)
{
    switch (widget->property.wdt.signature[0]) {
    case 'i':
        duk_push_int(ctx, widget->property.val.i);
        break;

    case 'q':
        duk_push_int(ctx, widget->property.val.q);
        break;

    case 'b':
        duk_push_boolean(ctx, widget->property.val.b);
        break;

    case 'd':
        duk_push_number(ctx, widget->property.val.d);
        break;

    case 's':
        duk_push_string(ctx, widget->property.val.s);
        break;

    default:
        duk_push_object(ctx);
        if (widget->property.wdt.propertyType == TIME_VALUE_PROPERTY) {
            SetIntProp(ctx, -1, "hour", widget->property.val.time.hour);
            SetIntProp(ctx, -1, "minute", widget->property.val.time.minute);
            SetIntProp(ctx, -1, "second", widget->property.val.time.second);
        } else {
            SetIntProp(ctx, -1, "day", widget->property.val.date.mDay);
            SetIntProp(ctx, -1, "month", widget->property.val.date.month);
            SetIntProp(ctx, -1, "year", widget->property.val.date.fullYear);
        }
        break;
    }
    return 1;
}

static int NativeValueGetter(duk_context* ctx)
{
    AJS_Widget* ajsWidget = GetWidgetFromThis(ctx);

    AJ_InfoPrintf(("Native value getter called for %s\n", ajsWidget->path));
    /*
     * Don't need the JavaScript widget object
     */
    duk_pop(ctx);
    return ValueGetter(ctx, ajsWidget);
}

/*
 * Called when a widget value has been changed
 */
AJ_Status AJS_CPS_OnValueChanged(AJS_Widget* ajsWidget)
{
    AJ_Status status = AJ_OK;
    duk_context* ctx = ajsWidget->dukCtx;

    AJ_InfoPrintf(("AJS_CPS_OnValueChanged\n"));

    GetWidgetObject(ctx, ajsWidget->index);
    /*
     * Strings need to be stabilized - other property values are held in the AJS_Widget itself.
     */
    if (ajsWidget->property.wdt.signature[0] == 's') {
        duk_push_string(ctx, ajsWidget->property.val.s);
        ajsWidget->property.val.s = duk_get_string(ctx, -1);
        duk_put_prop_string(ctx, -2, AJS_HIDDEN_PROP("value"));
    }
    duk_get_prop_string(ctx, -1, "onValueChanged");
    if (duk_is_callable(ctx, -1)) {
        int ret;
        /*
         * The widget object is the "this" binding for this call. The argument to the call is the
         * new value.
         */
        duk_dup(ctx, -2);
        ValueGetter(ctx, ajsWidget);
        ret = duk_pcall_method(ctx, 1);
        if (ret == DUK_EXEC_SUCCESS) {
            status = AJ_OK;
        } else {
            AJS_ConsoleSignalError(ctx);
            status = AJ_ERR_FAILURE;
        }
    }
    /*
     * Pops the widget and if onValueChanges exists pops the result otherwise pops an "undefined"
     */
    duk_pop_2(ctx);
    return status;
}

static const char* GetUnits(PropertyWidget* widget, uint16_t language)
{
    const char* units;
    AJS_Widget* ajsWidget = (AJS_Widget*)widget;
    duk_context* ctx = ajsWidget->dukCtx;

    GetWidgetObject(ctx, ajsWidget->index);
    duk_get_prop_string(ctx, -1, "units");
    units = AJS_GetTranslatedString(ctx, -1, language);
    duk_pop_2(ctx);
    return units;
}

static const char* GetChoice(PropertyWidget* widget, uint16_t index, const void** val, uint16_t language)
{
    static uint32_t choiceVal;
    const char* choice;
    AJS_Widget* ajsWidget = (AJS_Widget*)widget;
    duk_context* ctx = ajsWidget->dukCtx;

    GetWidgetObject(ctx, ajsWidget->index);
    duk_get_prop_string(ctx, -1, AJS_HIDDEN_PROP("choices"));
    duk_get_prop_index(ctx, -1, index);
    choice = AJS_GetTranslatedString(ctx, -1, language);
    duk_pop_3(ctx);
    choiceVal = index;
    *val = &choiceVal;
    AJ_InfoPrintf(("GetChoice lang=%d index=%d txt=%s\n", language, index, choice));
    return choice;
}

static int NativeChoicesGetter(duk_context* ctx)
{
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, AJS_HIDDEN_PROP("choices"));
    duk_remove(ctx, -2);
    return 1;
}

/*
 * Enumeration is an array of strings or array of arrays of strings for multi-language support
 */
static int NativeChoicesSetter(duk_context* ctx)
{
    AJS_Widget* widget = GetWidgetFromThis(ctx);
    duk_uarridx_t i;
    size_t numChoices;

    AJ_InfoPrintf(("Native choices setter called\n"));

    if (!duk_is_array(ctx, 0)) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Choices must be an array of strings or string arrays");
    }
    numChoices = duk_get_length(ctx, 0);
    if (numChoices < 2) {
        duk_error(ctx, DUK_ERR_RANGE_ERROR, "Must be at least two choices");
    }
    for (i = 0; i < numChoices; ++i) {
        duk_get_prop_index(ctx, 0, i);
        duk_require_string(ctx, -1);
        duk_pop(ctx);
    }
    /*
     * Set enumeration as a property on the widget object
     */
    duk_dup(ctx, 0);
    duk_put_prop_string(ctx, -2, AJS_HIDDEN_PROP("choices"));

    widget->property.wdt.optParams.numConstraints = (uint16_t)numChoices;
    widget->property.wdt.optParams.getConstraint = GetChoice;

    duk_pop(ctx);
    return 0;
}

static int NativeRangeSetter(duk_context* ctx)
{
    AJS_Widget* widget = GetWidgetFromThis(ctx);
    AJS_WidgetVal* range;

    if (!duk_is_object(ctx, 0)) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Requires a range object");
    }
    range = duk_push_fixed_buffer(ctx, 3 * sizeof(AJS_WidgetVal));
    duk_get_prop_string(ctx, 0, "min");
    duk_get_prop_string(ctx, 0, "max");
    if (duk_has_prop_string(ctx, 0, "increment")) {
        duk_get_prop_string(ctx, 0, "increment");
    } else {
        duk_push_int(ctx, 0);
    }
    /*
     * Initialize the range and set the type
     */
    if (IsInt(ctx, -1) && IsInt(ctx, -2) && IsInt(ctx, -3)) {
        range[0].i = duk_get_int(ctx, -3);
        range[1].i = duk_get_int(ctx, -2);
        range[2].i = duk_get_int(ctx, -1);
        widget->property.wdt.signature = "i";
    } else {
        range[0].d = duk_get_number(ctx, -3);
        range[1].d = duk_get_number(ctx, -2);
        range[2].d = duk_get_number(ctx, -1);
        widget->property.wdt.signature = "d";
    }
    duk_pop_3(ctx);
    widget->property.wdt.optParams.constraintRange.signature = &widget->property.wdt.signature;
    widget->property.wdt.optParams.constraintRange.minValue = &range[0];
    widget->property.wdt.optParams.constraintRange.maxValue = &range[1];
    widget->property.wdt.optParams.constraintRange.increment = &range[2];
    widget->property.wdt.optParams.constraintRangeDefined = TRUE;
    duk_put_prop_string(ctx, -2, AJS_HIDDEN_PROP("range"));

    if (duk_has_prop_string(ctx, 0, "units")) {
        duk_get_prop_string(ctx, 0, "units");
        duk_require_string(ctx, -1);
        duk_put_prop_string(ctx, -2, "units");
        widget->property.wdt.optParams.getUnitOfMeasure = GetUnits;
    }
    duk_pop(ctx);
    /*
     * Make sure the initial value is in required range
     */
    FitValueInRange(widget);
    return 0;
}

/*
 * Called with 1 or 2 parameters - layout and initial value
 *
 * Layout is one of the following
 *
 * SWITCH        1
 * CHECKBOX      2
 * SPINNER       3
 * RADIO_BUTTON  4
 * SLIDER        5
 * TIME_PICKER   6
 * DATE_PICKER   7
 * NUMBER_PICKER 8
 * KEYPAD        9
 * ROTARY_KNOB  10
 * TEXT_VIEW    11
 * NUMERIC_VIEW 12
 * EDIT_TEXT    13
 *
 * We support 6 property types determined at runtime from the initial value.
 *
 * INTEGER, BOOLEAN, NUMBER, STRING, DATE_TIME, and TIME
 */
static int NativePropertyWidget(duk_context* ctx)
{
    duk_idx_t numArgs = duk_get_top(ctx);
    AJS_Widget* widget;
    int layout;

    if (numArgs < 1) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Property widget needs a type");
    }
    layout = duk_require_int(ctx, 0);
    /*
     * Create the property widget - leave widget object on the top of the stack
     */
    widget = AddWidget(ctx, WIDGET_TYPE_PROPERTY);
    widget->property.wdt.getValue = GetValue;
    widget->hints[0] = layout;
    widget->property.wdt.propertyType = SINGLE_VALUE_PROPERTY;
    widget->base.optParams.numHints = 1;
    AJS_SetPropertyAccessors(ctx, -1, "value", NativeValueSetter, NativeValueGetter);
    AJS_SetPropertyAccessors(ctx, -1, "choices", NativeChoicesSetter, NativeChoicesGetter);
    AJS_SetPropertyAccessors(ctx, -1, "range", NativeRangeSetter, NULL);

    switch (layout) {
    case PROPERTY_WIDGET_HINT_SWITCH:
        widget->property.wdt.signature = "b";
        break;

    case PROPERTY_WIDGET_HINT_TEXTVIEW:
    case PROPERTY_WIDGET_HINT_EDITTEXT:
        widget->property.wdt.signature = "s";
        widget->property.val.s = "";
        break;

    case PROPERTY_WIDGET_HINT_CHECKBOX:
    case PROPERTY_WIDGET_HINT_SPINNER:
    case PROPERTY_WIDGET_HINT_RADIOBUTTON:
    case PROPERTY_WIDGET_HINT_KEYPAD:
        widget->property.wdt.signature = "q";
        break;

    case PROPERTY_WIDGET_HINT_SLIDER:
    case PROPERTY_WIDGET_HINT_ROTARYKNOB:
    case PROPERTY_WIDGET_HINT_NUMERICVIEW:
    case PROPERTY_WIDGET_HINT_NUMBERPICKER:
        /*
         * Assume an integer but may updated if a range is set
         */
        widget->property.wdt.signature = "i";
        break;

    case PROPERTY_WIDGET_HINT_TIMEPICKER:
        widget->property.wdt.propertyType = TIME_VALUE_PROPERTY;
        widget->property.wdt.signature = TIME_PROPERTY_SIG;
        break;

    case PROPERTY_WIDGET_HINT_DATEPICKER:
        widget->property.wdt.propertyType = DATE_VALUE_PROPERTY;
        widget->property.wdt.signature = DATE_PROPERTY_SIG;
        break;

    default:
        duk_error(ctx, DUK_ERR_RANGE_ERROR, "Not a valid property type");
        break;
    }
    if (numArgs >= 2) {
        /*
         * Set the initial value - this will call setter function we just registered
         */
        duk_dup(ctx, 1);
        duk_put_prop_string(ctx, -2, "value");
    }
    if (numArgs >= 3) {
        /*
         * Set the label
         */
        duk_dup(ctx, 2);
        duk_put_prop_string(ctx, -2, "label");
    }
    /*
     * The new object is the return value - leave it on the stack
     */
    return 1;
}

static const char* DialogLabel(DialogWidget* widget, uint8_t action, uint16_t language)
{
    AJS_Widget* ajsWidget = (AJS_Widget*)widget;
    duk_context* ctx = ajsWidget->dukCtx;
    const char* label = "";
    int top = duk_get_top_index(ctx);

    AJ_InfoPrintf(("DialogLabel from %s\n", ajsWidget->path));

    GetWidgetObject(ctx, ajsWidget->index);
    duk_get_prop_string(ctx, -1, AJS_HIDDEN_PROP("buttons"));
    if (duk_is_array(ctx, -1)) {
        duk_get_prop_index(ctx, -1, action);
        if (duk_is_object(ctx, -1)) {
            duk_get_prop_string(ctx, -1, "label");
            label = AJS_GetTranslatedString(ctx, -1, language);
            duk_pop(ctx);
        } else {
            AJ_ErrPrintf(("DialogLabel no action%d\n", action));
        }
        duk_pop(ctx);
    }
    duk_pop_2(ctx);
    AJ_ASSERT(top == duk_get_top_index(ctx));
    AJ_InfoPrintf(("DialogLabel returning %s\n", label));
    return label;
}

static const char* DialogLabel1(DialogWidget* widget, uint16_t language) {
    return DialogLabel(widget, 0, language);
}
static const char* DialogLabel2(DialogWidget* widget, uint16_t language) {
    return DialogLabel(widget, 1, language);
}
static const char* DialogLabel3(DialogWidget* widget, uint16_t language) {
    return DialogLabel(widget, 2, language);
}

/*
 * Actions property must be an array of objects where each objects has a "label" and "onClick"
 * property.
 */
static int NativeButtonsSetter(duk_context* ctx)
{
    AJS_Widget* widget = GetWidgetFromThis(ctx);
    duk_uarridx_t i;

    if (duk_is_array(ctx, 0)) {
        widget->dialog.numActions = (uint16_t)duk_get_length(ctx, 0);
    }
    if ((widget->dialog.numActions < 1) || (widget->dialog.numActions > 3)) {
        goto TypeError;
    }
    /*
     * Validate the array
     */
    for (i = 0; i < widget->dialog.numActions; ++i) {
        duk_get_prop_index(ctx, 0, i);
        duk_get_prop_string(ctx, -1, "label");
        if (!duk_is_string(ctx, -1)) {
            duk_pop_2(ctx);
            goto TypeError;
        }
        duk_pop(ctx);
        duk_get_prop_string(ctx, -1, "onClick");
        if (!duk_is_callable(ctx, -1)) {
            duk_pop_2(ctx);
            goto TypeError;
        }
        duk_pop_2(ctx);
    }
    duk_dup(ctx, 0);
    duk_put_prop_string(ctx, -2, AJS_HIDDEN_PROP("buttons"));
    duk_pop(ctx);
    return 0;

TypeError:
    duk_pop(ctx);
    duk_error(ctx, DUK_ERR_TYPE_ERROR, "Requires array of 1..3 objects with a label and onClick function");
    return 0;

}

AJ_Status AJS_CP_ExecuteAction(AJS_Widget* ajsWidget, uint8_t action)
{
    AJ_Status status = AJ_ERR_NO_MATCH;
    duk_context* ctx = ajsWidget->dukCtx;

    GetWidgetObject(ctx, ajsWidget->index);
    AJ_InfoPrintf(("Execute action%d on %s\n", action, ajsWidget->path));
    duk_get_prop_string(ctx, -1, AJS_HIDDEN_PROP("buttons"));
    if (duk_is_array(ctx, -1)) {
        duk_get_prop_index(ctx, -1, action);
        if (duk_is_object(ctx, -1)) {
            int ret;
            duk_get_prop_string(ctx, -1, "onClick");
            ret = duk_pcall(ctx, 0);
            duk_pop(ctx);
            if (ret == DUK_EXEC_SUCCESS) {
                status = AJ_OK;
            } else {
                AJS_ConsoleSignalError(ctx);
                status = AJ_ERR_FAILURE;
            }
        }
        duk_pop(ctx);
    }
    duk_pop_2(ctx);
    return status;
}

static const char* GetDialogMessage(DialogWidget* widget, uint16_t language)
{
    AJS_Widget* ajsWidget = (AJS_Widget*)widget;
    duk_context* ctx = ajsWidget->dukCtx;
    const char* message = "";

    GetWidgetObject(ctx, ajsWidget->index);
    /*
     * String pointers are stable so we can simply return it
     */
    duk_get_prop_string(ctx, -1, "message");
    message = AJS_GetTranslatedString(ctx, -1, language);
    duk_pop_2(ctx);
    return message;
}

static int NativeDialogWidget(duk_context* ctx)
{
    duk_idx_t numArgs = duk_get_top(ctx);
    AJS_Widget* widget = AddWidget(ctx, WIDGET_TYPE_DIALOG);

    if (numArgs > 0) {
        duk_require_string(ctx, 0);
        duk_dup(ctx, 0);
    } else {
        duk_push_string(ctx, "");
    }
    duk_put_prop_string(ctx, -2, "message");
    /*
     * Set dialog callbacks
     */
    widget->dialog.getMessage = GetDialogMessage;
    widget->dialog.optParams.getLabelAction1 = DialogLabel1;
    widget->dialog.optParams.getLabelAction2 = DialogLabel2;
    widget->dialog.optParams.getLabelAction3 = DialogLabel3;
    /*
     * Function to set actions on this dialog
     */
    AJS_SetPropertyAccessors(ctx, -1, "buttons", NativeButtonsSetter, NULL);
    /*
     * The new object is the return value - leave it on the stack
     */
    return 1;
}

static int NativeActionWidget(duk_context* ctx)
{
    duk_require_string(ctx, 0);
    AddWidget(ctx, WIDGET_TYPE_ACTION);
    duk_dup(ctx, 0);
    duk_put_prop_string(ctx, -2, "label");
    /*
     * An action widget can contain a dialog widget
     */
    duk_push_c_function(ctx, NativeDialogWidget, 1);
    duk_put_prop_string(ctx, -2, "dialogWidget");
    /*
     * The new object is the return value - leave it on the stack
     */
    return 1;
}

/*
 * Add the functions for creating widgets in this container
 */
static void AddContainerFunctions(duk_context* ctx, AJS_Widget* widget)
{
    duk_push_c_function(ctx, NativeLabelWidget, 1);
    duk_put_prop_string(ctx, -2, "labelWidget");
    duk_push_c_function(ctx, NativePropertyWidget, DUK_VARARGS);
    duk_put_prop_string(ctx, -2, "propertyWidget");
    duk_push_c_function(ctx, NativeActionWidget, 1);
    duk_put_prop_string(ctx, -2, "actionWidget");
    duk_push_c_function(ctx, NativeContainerWidget, DUK_VARARGS);
    duk_put_prop_string(ctx, -2, "containerWidget");
}

static int NativeContainerWidget(duk_context* ctx)
{
    duk_idx_t numArgs = duk_get_top(ctx);

    AJS_Widget* widget = AddWidget(ctx, WIDGET_TYPE_CONTAINER);
    AJ_InfoPrintf(("Creating container widget\n"));
    AddContainerFunctions(ctx, widget);

    widget->base.optParams.numHints = 1;
    if (numArgs == 0) {
        widget->hints[0] = LAYOUT_HINT_HORIZONTAL_LINEAR;
    } else {
        if (numArgs > 0) {
            widget->hints[0] = duk_require_int(ctx, 0);
        }
        if (numArgs > 1) {
            widget->base.optParams.numHints = 2;
            widget->hints[1] = duk_require_int(ctx, 1);
        }
    }
    return 1;
}

static int NativeLoadControlPanel(duk_context* ctx)
{
    size_t num;
    duk_uarridx_t i;
    AJ_Object* objList;

    AJ_InfoPrintf(("Loading control panel\n"));

    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "widgets");
    num = duk_get_length(ctx, -1);
    /*
     * Create the AllJoyn object list and put it in the global stash
     */
    objList = duk_push_fixed_buffer(ctx, (num + 1) * sizeof(AJ_Object));
    duk_put_prop_string(ctx, -3, "cpsObjects");
    /*
     * Initialize the object list
     */
    for (i = 0; i < num; ++i) {
        AJS_Widget* widget;
        duk_get_prop_index(ctx, -1, i);
        duk_get_prop_string(ctx, -1, "path");
        objList[i].path = duk_require_string(ctx, -1);
        duk_pop(ctx);
        duk_get_prop_string(ctx, -1, AJS_HIDDEN_PROP("wbuf"));
        widget = duk_require_buffer(ctx, -1, NULL);
        AJ_ASSERT(widget->index == i);
        duk_pop_2(ctx);
        objList[i].interfaces = AJS_WidgetInterfaces(widget->type);
        objList[i].context = widget;
    }
    /*
     * Initialize the control panel service
     */
    AJS_CP_Init(objList);
    duk_pop_2(ctx);
    return 0;
}

static int NativeControlPanelFinalizer(duk_context* ctx)
{
    /*
     * Delete the widgets array and objects list from the global stash object
     */
    duk_push_global_stash(ctx);
    duk_del_prop_string(ctx, -1, "widgets");
    duk_del_prop_string(ctx, -1, "cpsObjects");
    duk_pop(ctx);
    AJS_CP_Terminate();
    return 0;
}

static int NativeOptColorSetter(duk_context* ctx)
{
    int r, g, b;
    AJS_Widget* widget = GetWidgetFromThis(ctx);
    AJ_InfoPrintf(("Native color setter called\n"));
    if (duk_is_number(ctx, 0)) {
        int color = duk_get_int(ctx, 0);
        r = (color >> 16);
        g = (color >> 8) & 0xFF;
        b = (color) & 0xFF;
    } else {
        r = ReqIntProp(ctx, 0, "red");
        g = ReqIntProp(ctx, 0, "green");
        b = ReqIntProp(ctx, 0, "blue");
    }
    r = min(r, 255);
    g = min(g, 255);
    b = min(b, 255);
    widget->base.optParams.bgColor = (r << 16) | (g << 8) | b;
    duk_pop(ctx);
    AJS_CPS_SignalMetadataChanged(AJS_GetBusAttachment(), widget);
    return 0;
}

static int NativeOptColorGetter(duk_context* ctx)
{
    AJS_Widget* widget = GetWidgetFromThis(ctx);
    AJ_InfoPrintf(("Native color getter called\n"));

    duk_push_int(ctx, widget->base.optParams.bgColor);
    return 1;
}

static int NativeWriteableSetter(duk_context* ctx)
{
    AJS_Widget* widget = GetWidgetFromThis(ctx);
    setBaseWritable(&widget->base, duk_require_boolean(ctx, 0));
    duk_pop(ctx);
    AJS_CPS_SignalMetadataChanged(AJS_GetBusAttachment(), widget);
    return 0;
}

static int NativeEnabledSetter(duk_context* ctx)
{
    AJS_Widget* widget = GetWidgetFromThis(ctx);
    setBaseEnabled(&widget->base, duk_require_boolean(ctx, 0));
    duk_pop(ctx);
    AJS_CPS_SignalMetadataChanged(AJS_GetBusAttachment(), widget);
    return 0;
}

static int NativeOptLabelSetter(duk_context* ctx)
{
    AJS_Widget* widget = GetWidgetFromThis(ctx);
    AJ_InfoPrintf(("Native label setter called\n"));
    widget->base.optParams.getLabel = GetLabel;
    duk_dup(ctx, 0);
    duk_put_prop_string(ctx, -2, AJS_HIDDEN_PROP("label"));
    duk_pop(ctx);
    AJS_CPS_SignalMetadataChanged(AJS_GetBusAttachment(), widget);
    return 0;
}

static int NativeOptLabelGetter(duk_context* ctx)
{
    GetWidgetFromThis(ctx);
    AJ_InfoPrintf(("Native label getter called\n"));
    duk_get_prop_string(ctx, -1, AJS_HIDDEN_PROP("label"));
    duk_remove(ctx, -2);
    return 1;
}

static int NativeControlPanel(duk_context* ctx)
{
    AJS_Widget* widget = CreateWidget(ctx, WIDGET_TYPE_CONTAINER);

    AddContainerFunctions(ctx, widget);
    widget->base.optParams.numHints = 2;
    widget->hints[0] = LAYOUT_HINT_VERTICAL_LINEAR;
    widget->hints[1] = LAYOUT_HINT_HORIZONTAL_LINEAR;
    /*
     * Set the path on the control panel
     */
    duk_push_string(ctx, "/ControlPanel");
    duk_put_prop_string(ctx, -2, "path");
    /*
     * Set control panel as first element in the widgets array
     */
    duk_get_prop_string(ctx, -1, "widgets");
    AJ_ASSERT(duk_is_array(ctx, -1));
    duk_dup(ctx, -2);
    duk_put_prop_index(ctx, -2, 0);
    duk_pop(ctx);
    /*
     * Dialog widgets reside at the top level
     */
    duk_push_c_function(ctx, NativeDialogWidget, 1);
    duk_put_prop_string(ctx, -2, "dialogWidget");
    /*
     * Add the load function that actually registers the AllJoyn objects for the widget tree
     */
    duk_push_c_function(ctx, NativeLoadControlPanel, 0);
    duk_put_prop_string(ctx, -2, "load");
    /*
     * Register a finalizer so we can unload the control panel
     */
    AJS_RegisterFinalizer(ctx, -1, NativeControlPanelFinalizer);
    /*
     * Control panel constants
     */
    SetIntProp(ctx, -1, "VERTICAL", LAYOUT_HINT_VERTICAL_LINEAR);
    SetIntProp(ctx, -1, "HORIZONTAL", LAYOUT_HINT_HORIZONTAL_LINEAR);

    SetIntProp(ctx, -1, "SWITCH", PROPERTY_WIDGET_HINT_SWITCH);
    SetIntProp(ctx, -1, "CHECK_BOX", PROPERTY_WIDGET_HINT_CHECKBOX);
    SetIntProp(ctx, -1, "SPINNER", PROPERTY_WIDGET_HINT_SPINNER);
    SetIntProp(ctx, -1, "RADIO_BUTTON", PROPERTY_WIDGET_HINT_RADIOBUTTON);
    SetIntProp(ctx, -1, "SLIDER", PROPERTY_WIDGET_HINT_SLIDER);
    SetIntProp(ctx, -1, "TIME_PICKER", PROPERTY_WIDGET_HINT_TIMEPICKER);
    SetIntProp(ctx, -1, "DATE_PICKER", PROPERTY_WIDGET_HINT_DATEPICKER);
    SetIntProp(ctx, -1, "NUMBER_PICKER", PROPERTY_WIDGET_HINT_NUMBERPICKER);
    SetIntProp(ctx, -1, "KEYPAD", PROPERTY_WIDGET_HINT_KEYPAD);
    SetIntProp(ctx, -1, "ROTARY_KNOB", PROPERTY_WIDGET_HINT_ROTARYKNOB);
    SetIntProp(ctx, -1, "TEXT_VIEW", PROPERTY_WIDGET_HINT_TEXTVIEW);
    SetIntProp(ctx, -1, "NUMERIC_VIEW", PROPERTY_WIDGET_HINT_NUMERICVIEW);
    SetIntProp(ctx, -1, "EDIT_TEXT", PROPERTY_WIDGET_HINT_EDITTEXT);
    /*
     * The new object is the return value - leave it on the stack
     */
    return 1;
}

AJ_Status AJS_RegisterControlPanelHandlers(AJ_BusAttachment* bus, duk_context* ctx, duk_idx_t ajIdx)
{
    duk_idx_t baseIdx;
    duk_idx_t cpIdx;

    cpIdx = duk_push_c_function(ctx, NativeControlPanel, 0);
    duk_dup_top(ctx);
    duk_put_prop_string(ctx, ajIdx, "controlPanel");
    /*
     * Create the base widget;the prototype for all widgets including the control panel itself.
     */
    baseIdx = duk_push_object(ctx);
    AJS_SetPropertyAccessors(ctx, baseIdx, "color", NativeOptColorSetter, NativeOptColorGetter);
    AJS_SetPropertyAccessors(ctx, baseIdx, "label", NativeOptLabelSetter, NativeOptLabelGetter);
    AJS_SetPropertyAccessors(ctx, baseIdx, "enabled", NativeEnabledSetter, NULL);
    AJS_SetPropertyAccessors(ctx, baseIdx, "writeable", NativeWriteableSetter, NULL);
    /*
     * Create an empty widget array
     */
    duk_push_array(ctx);
    /*
     * Put the widget in the global stash for easy top level access.
     */
    duk_push_global_stash(ctx);
    duk_dup(ctx, -2);
    duk_put_prop_string(ctx, -2, "widgets");
    duk_pop(ctx);
    /*
     * Add widget array to the base widget - this will be inherited by all widgets
     */
    duk_put_prop_string(ctx, baseIdx, "widgets");
    duk_put_prop_string(ctx, cpIdx, "baseWidget");
    /*
     * Pop the global stash and controlPanel objects
     */
    duk_pop(ctx);
    return AJ_OK;
}
