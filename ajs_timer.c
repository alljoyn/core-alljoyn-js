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

#define IsIntervalTimer(t)  ((t).interval > 0)

typedef struct _AJS_TIMER {
    uint32_t id;        /* Id for the timer composed from index + salt value */
    int32_t interval;   /* Scheduling interval in milliseconds - negative value indicates a one-shot timer */
    uint32_t countDown; /* Count down time */
} AJS_TIMER;

#define NUM_TIMERS 2   /* number of timers to allocate initially */
#define ADD_TIMERS 2   /* numer of timers to add if we need to expand the table */
#define MAX_TIMERS 256 /* maximum number of timers */


#define GET_TIMER_INDEX(id)   ((id) >> 24)
#define SALT_TIMER_ID(index)  ((index << 24) || (++timerSalt & 0xFFFFFF))

/*
 * Salt for uniquefying timer identifiers
 */
static uint32_t timerSalt;

/*
 * Current deadline - zero means we don't know the deadline
 */
static uint32_t deadline = 0;

/*
 * The same function is used to register interval and one-shot timers
 */
static int RegisterTimer(duk_context* ctx, int32_t ms)
{
    AJS_TIMER* timers;
    size_t numTimers;
    size_t timerEntry;

    if (!duk_is_callable(ctx, 0)) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "First argument must be a function");
    }
    /*
     * Get the global state management objects
     */
    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, "$timerFuncs");
    duk_get_prop_string(ctx, -2, "$timerState");

    timers = duk_get_buffer(ctx, -1, &numTimers);
    numTimers = numTimers / sizeof(AJS_TIMER);
    /*
     * Find first free slot in the timer table expanding the table if necessary
     */
    for (timerEntry = 0; timerEntry <= numTimers; ++timerEntry) {
        if (timerEntry == numTimers) {
            numTimers += ADD_TIMERS;
            if (numTimers > MAX_TIMERS) {
                duk_error(ctx, DUK_ERR_ALLOC_ERROR, "Too many timers");
            }
            timers = duk_resize_buffer(ctx, -1, numTimers * sizeof(AJS_TIMER));
            if (!timers) {
                duk_error(ctx, DUK_ERR_ALLOC_ERROR, "Could not allocate timer");
            }
        }
        /*
         * Zero indicates an unused timer entry
         */
        if (timers[timerEntry].interval == 0) {
            break;
        }
    }
    /*
     * Pop the buffer off the stack
     */
    duk_pop(ctx);
    /*
     * Push the callable function onto the stack and set in array
     */
    duk_dup(ctx, 0);
    duk_put_prop_index(ctx, -2, timerEntry);
    /*
     * Pop the callable, the timerFuncsArray and the global context
     */
    duk_pop_3(ctx);
    /*
     * Push the salted timer index, this is what we will return from this call
     */
    timers[timerEntry].id = SALT_TIMER_ID(timerEntry);
    duk_push_int(ctx, timers[timerEntry].id);
    /*
     * Set the interval and initialize the countDown timer
     */
    timers[timerEntry].interval = ms;
    timers[timerEntry].countDown = abs(ms);
    /*
     * We need to recompute the deadline
     */
    deadline = 0;

    return 1;
}

static AJS_TIMER* GetTimer(duk_context* ctx, uint8_t isInterval)
{
    AJS_TIMER* timers;
    size_t numTimers;
    uint32_t timerId = (uint32_t)duk_require_int(ctx, 0);
    uint32_t timerEntry = GET_TIMER_INDEX(timerId);

    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, "$timerState");
    timers = duk_get_buffer(ctx, -1, &numTimers);
    numTimers = numTimers / sizeof(AJS_TIMER);
    /*
     * Check timer exists
     */
    if ((timerEntry > numTimers) || (timerId != timers[timerEntry].id)) {
        duk_error(ctx, DUK_ERR_RANGE_ERROR, "No such timer");
    }
    /*
     * Check timer has the expected type
     */
    if (isInterval != IsIntervalTimer(timers[timerEntry])) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Timer has wrong type for this operation");
    }
    duk_pop_3(ctx);
    return &timers[timerEntry];
}

static int ClearTimer(duk_context* ctx, uint8_t isInterval)
{
    AJS_TIMER* timer = GetTimer(ctx, isInterval);
    uint32_t timerEntry = GET_TIMER_INDEX(timer->id);

    /*
     * Clear timer
     */
    memset(timer, 0, sizeof(AJS_TIMER));
    /*
     * Remove reference to timer callback function
     */
    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, "$timerFuncs");
    duk_push_undefined(ctx);
    duk_put_prop_index(ctx, -2, timerEntry);
    duk_pop(ctx);
    /*
     * We need to recompute the deadline
     */
    deadline = 0;
    return 0;
}

static int NativeSetInterval(duk_context* ctx)
{
    int32_t ms = duk_require_int(ctx, 1);

    if (ms <= 0) {
        duk_error(ctx, DUK_ERR_RANGE_ERROR, "Interval must be >= 0");
    }
    AJ_InfoPrintf(("setInterval(%d)", ms));
    return RegisterTimer(ctx, ms);
}

static int NativeSetTimeout(duk_context* ctx)
{
    int32_t ms = duk_require_int(ctx, 1);

    if (ms <= 0) {
        duk_error(ctx, DUK_ERR_RANGE_ERROR, "Timeout must be >= 0");
    }
    AJ_InfoPrintf(("setTimeout(%d)", ms));
    return RegisterTimer(ctx, -ms);
}

static int ResetTimer(duk_context* ctx, uint8_t isInterval)
{
    int32_t ms = duk_require_int(ctx, 1);
    AJS_TIMER* timer = GetTimer(ctx, isInterval);

    if (ms <= 0) {
        duk_error(ctx, DUK_ERR_RANGE_ERROR, "%s must be >= 0", isInterval ? "Interval" : "Timeout");
    }
    /*
     * Reset the timer properties
     */
    timer->interval = isInterval ? ms : -1;
    timer->countDown = (uint32_t)(ms);
    /*
     * We need to recompute the deadline
     */
    deadline = 0;
    /*
     * Push the timer id, this is what we will return from this call
     */
    duk_push_int(ctx, timer->id);
    return 1;
}

static int NativeResetInterval(duk_context* ctx)
{
    return ResetTimer(ctx, TRUE);
}

static int NativeResetTimeout(duk_context* ctx)
{
    return ResetTimer(ctx, FALSE);
}

static int NativeClearInterval(duk_context* ctx)
{
    return ClearTimer(ctx, TRUE);
}

static int NativeClearTimeout(duk_context* ctx)
{
    return ClearTimer(ctx, FALSE);
}

AJ_Status AJS_RegisterTimerFuncs(duk_context* ctx)
{
    duk_push_global_object(ctx);

    /*
     * Timer state is managed via two global properties, "$timerFuncs" is an array that hold
     * references to the timer callback functions, "$timerState" is a memory blob that holds
     * an array of C structs that provide information about the active timers.
     */
    duk_push_array(ctx);
    duk_put_prop_string(ctx, -2, "$timerFuncs");
    duk_push_dynamic_buffer(ctx, NUM_TIMERS * sizeof(AJS_TIMER));
    duk_put_prop_string(ctx, -2, "$timerState");
    /*
     * Register interval and timeout functions
     */
    duk_push_c_function(ctx, NativeSetInterval, 2);
    duk_put_prop_string(ctx, -2, "setInterval");
    duk_push_c_function(ctx, NativeClearInterval, 1);
    duk_put_prop_string(ctx, -2, "clearInterval");
    duk_push_c_function(ctx, NativeResetInterval, 2);
    duk_put_prop_string(ctx, -2, "resetInterval");
    duk_push_c_function(ctx, NativeSetTimeout, 2);
    duk_put_prop_string(ctx, -2, "setTimeout");
    duk_push_c_function(ctx, NativeClearTimeout, 1);
    duk_put_prop_string(ctx, -2, "clearTimeout");
    duk_push_c_function(ctx, NativeResetTimeout, 2);
    duk_put_prop_string(ctx, -2, "resetTimeout");

    duk_pop(ctx);

    deadline = 0;

    return AJ_OK;
}

AJ_Status AJS_RunTimers(duk_context* ctx, AJ_Time* clock, uint32_t* currentTO)
{
    AJS_TIMER* timers;
    size_t numTimers;
    size_t timerEntry;
    uint32_t elapsed;
    uint32_t prevDeadline = deadline;

    /*
     * Time elapsed since this function was last called
     */
    elapsed = AJ_GetElapsedTime(clock, FALSE);
    /*
     * Not much to do if there is a current timeout and we are working towards a deadline
     */
    if ((elapsed < *currentTO) && deadline) {
        *currentTO -= elapsed;
        //AJ_InfoPrintf(("AJS_RunTimers elapsed=%d newTO=%d\n", elapsed, *currentTO));
        return AJ_OK;
    }
    /*
     * Get the global state management objects
     */
    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, "$timerState");
    timers = duk_get_buffer(ctx, -1, &numTimers);
    numTimers = numTimers / sizeof(AJS_TIMER);
    duk_get_prop_string(ctx, -2, "$timerFuncs");
    /*
     * Iterate over the timers and call functions at or past the deadline
     */
    for (timerEntry = 0; timerEntry < numTimers; ++timerEntry) {
        if (timers[timerEntry].interval != 0) {
            AJ_InfoPrintf(("timer[%d].countDown = %d\n", (int)timerEntry, timers[timerEntry].countDown));
            if (timers[timerEntry].countDown <= prevDeadline) {
                /*
                 * Get timer function on the stack
                 */
                duk_get_prop_index(ctx, -1, timerEntry);
                /*
                 * Update an interval timer or delete a one-shot timeout timer.
                 */
                if (IsIntervalTimer(timers[timerEntry])) {
                    timers[timerEntry].countDown = timers[timerEntry].interval;
                } else {
                    memset(&timers[timerEntry], 0, sizeof(AJS_TIMER));
                    duk_push_undefined(ctx);
                    duk_put_prop_index(ctx, -2, timerEntry);
                }
                /*
                 * Call the timer function
                 */
                if (duk_pcall(ctx, 0) != DUK_EXEC_SUCCESS) {
                    AJS_ConsoleSignalError(ctx);
                }
                duk_pop(ctx); // return value
            } else {
                timers[timerEntry].countDown -= prevDeadline;
            }
        }
    }
    /*
     * Pop timer funcs array
     */
    duk_pop(ctx);
    /*
     * Reload the timer state and calculate the new deadline.
     */
    timers = duk_get_buffer(ctx, -1, &numTimers);
    numTimers = numTimers / sizeof(AJS_TIMER);
    deadline = 0x7FFFFFFF;
    for (timerEntry = 0; timerEntry < numTimers; ++timerEntry) {
        if (timers[timerEntry].interval != 0) {
            deadline = min(timers[timerEntry].countDown, deadline);
        }
    }

    duk_pop_2(ctx);
    *currentTO = deadline;

    AJ_InfoPrintf(("AJS_RunTimers deadline=%d\n", deadline));

    return AJ_OK;
}
