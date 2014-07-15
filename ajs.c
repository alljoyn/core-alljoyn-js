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
#include "ajs_target.h"
#include "ajs_ctrlpanel.h"
#include "aj_util.h"
#include "ajs_heap.h"

/**
 * Turn on per-module debug printing by setting this variable to non-zero value
 * (usually in debugger).
 */
#ifndef NDEBUG
uint8_t dbgAJS = 1;
#endif

static AJ_BusAttachment ajBus;

const char* AJS_AllJoynObject = "AJ";

/*
 * Initialization of the AllJoyn root object required for any application
 */
static const char aj_init[] =
    "var AJ={"
    "interfaceDefinition:{},"
    "objectDefinition:{},"
    "config:{linkTimeout:10000,callTimeout:10000},"
    "onSignal:function(msg){return undefined;},"
    "onMethodCall:function(msg){return undefined;},"
    "onAttach:function(){return undefined;},"
    "onDetach:function(){return undefined;},"
    "METHOD:0,"
    "SIGNAL:1,"
    "PROPERTY:2,"
    "defaultLanguage:'en'"
    "};"
    "print('AJ initialized\\n');";

#if defined(BIG_HEAP)

static const AJS_HeapConfig heapConfig[] = {
    { 16,     200 },
    { 24,     800 },
    { 32,     800 },
    { 48,     800 },
    { 64,     800 },
    { 96,     800 },
    { 128,     64 },
    { 256,     64 },
    { 512,     16 },
    { 1024,    16 },
    { 2048,     8 },
    { 3000,     8 },
#ifdef DUK_FIXED_SIZE_ST
    { DUK_FIXED_SIZE_ST* sizeof(void*), 1 },
#else
    { 4096,     2 },
#endif
    { 16384,    2 }
};

static uint32_t heap[500000 / 4];

#else

static const AJS_HeapConfig heapConfig[] = {
    { 12,      20, AJS_POOL_BORROW },
    { 20,      60, AJS_POOL_BORROW },
    { 24,     300, AJS_POOL_BORROW },
    { 32,     500, AJS_POOL_BORROW },
    { 40,     400 },
    { 128,    80 },
    { 256,    30 },
    { 512,    16 },
    { 1024,   6 },
    { 2048,   4 },
#ifdef DUK_FIXED_SIZE_ST
    { DUK_FIXED_SIZE_ST* sizeof(void*), 1 }
#else
    { 4096,     2 }
#endif
};

static uint32_t heap[92928 / 4];
#endif

static void ErrorHandler(duk_context* ctx, duk_errcode_t code, const char* msg)
{
    AJ_ErrPrintf(("ErrorHandler error code:%d %s\n", (int)code, msg));
#ifndef NDEBUG
    AJS_HeapDump();
#endif
    exit(0);
}

AJ_BusAttachment* AJS_GetBusAttachment()
{
    return &ajBus;
}

/*
 * Register builtin methods on the AJ object
 */
static void AJRegistrations(AJ_BusAttachment* aj, duk_context* ctx, duk_idx_t ajIdx)
{
    /*
     * Register message functions
     */
    AJS_RegisterMsgFunctions(aj, ctx, ajIdx);
    /*
     * Register native handler functions
     */
    AJS_RegisterHandlers(aj, ctx, ajIdx);
    /*
     * Register and initialize notification support
     */
    AJS_RegisterNotifHandlers(aj, ctx, ajIdx);
    /*
     * Register control panel support
     */
    AJS_RegisterControlPanelHandlers(aj, ctx, ajIdx);
    /*
     * Register target-specific I/O functions
     */
    AJS_RegisterIO(ctx);
    /*
     * Register setTimeout, setInterval timer functions.
     */
    AJS_RegisterTimerFuncs(ctx);
    /*
     * Register translations table
     */
    AJS_RegisterTranslations(ctx, ajIdx);
}

static uint8_t ajRunning = FALSE;
static uint8_t busAttached = FALSE;

uint8_t AJS_IsRunning()
{
    return ajRunning;
}

static AJ_Status Run(AJ_BusAttachment* aj, duk_context* ctx, int ajIdx)
{
    AJ_Status status = AJ_OK;

    /*
     * Add JavaScript objects and interfaces
     */
    status = AJS_InitTables(ctx);

    while (status == AJ_OK) {
        /*
         * Attach our BusAttachment to an AllJoyn routing node
         */
        if (!busAttached) {
            AJS_HeapDump();
            status = AJS_AttachAllJoyn(aj);
            if (status == AJ_OK) {
                status = AJS_ConsoleInit(aj);
            }
            if (status == AJ_OK) {
                /*
                 * Bind the application session port
                 */
                status = AJ_BusBindSessionPort(aj, AJS_APP_PORT, NULL, AJ_FLAG_NO_REPLY_EXPECTED);
            }
            if (status != AJ_OK) {
                break;
            }
            busAttached = TRUE;
        }
        /*
         * Let JavaScript know we are attached to the bus
         */
        ajRunning = TRUE;
        duk_push_string(ctx, "onAttach");
        if (duk_pcall_prop(ctx, ajIdx, 0) != DUK_EXEC_SUCCESS) {
            AJS_ConsoleSignalError(ctx);
        }
        duk_pop(ctx);
        /*
         * Start running
         */
        status = AJS_MessageLoop(ctx, aj);
    }
    AJ_WarnPrintf(("Exiting AJS main loop with status=%s\n", AJ_StatusText(status)));
    /*
     * JavaScript table entries are no longer valid
     */
    AJS_ResetTables(ctx);
    /*
     * Make sure the control panel service is terminated
     */
    AJS_CP_Terminate();
    /*
     * A RESTART_APP status indicates that the script engine must be restarted but we can keep our
     * existing BusAttachment.
     */
    if (status != AJ_ERR_RESTART_APP) {
        AJS_ConsoleTerminate();
        AJS_DetachAllJoyn(aj, status);
        busAttached = FALSE;
    }
    /*
     * If we told JavaScript we are attached now indicate we are detached
     */
    if (ajRunning) {
        duk_push_string(ctx, "onDetach");
        duk_pcall_prop(ctx, ajIdx, 0);
        duk_pop(ctx);
        ajRunning = FALSE;
    }
    return status;
}

static const uint8_t icon[] = {
#include "icon.inc"
};

static int NativeOverrideAlert(duk_context* ctx)
{
    AJS_AlertHandler(ctx, 1 /* alert */);
    return 0;
}

static int NativeOverridePrint(duk_context* ctx)
{
    AJS_AlertHandler(ctx, 0 /* print */);
    return 0;
}

AJ_Status AJS_Main()
{
    AJ_Status status = AJ_OK;
    duk_context* ctx;
    duk_int_t ret;
    size_t heapSz;
    duk_idx_t ajIdx;

    AJ_AboutSetIcon(icon, sizeof(icon), "image/jpeg", NULL);

    /*
     * Zero out the bus attachment
     */
    memset(&ajBus, 0, sizeof(ajBus));

    while (status == AJ_OK) {
        /*
         * Allocate the heap pools
         */
        heapSz = AJS_HeapRequired(heapConfig, ArraySize(heapConfig));
        if (heapSz > sizeof(heap)) {
            AJ_ErrPrintf(("Heap space is too small %d required %d\n", (int)sizeof(heap), (int)heapSz));
            status = AJ_ERR_RESOURCES;
            break;
        }
        AJ_Printf("Allocated heap %d bytes\n", (int)heapSz);
        AJS_HeapInit(heap, heapSz, heapConfig, ArraySize(heapConfig));

        ctx = duk_create_heap(AJS_Alloc, AJS_Realloc, AJS_Free, &ctx, ErrorHandler);
        if (!ctx) {
            AJ_ErrPrintf(("Failed to create duktape heap\n"));
            status = AJ_ERR_RESOURCES;
            break;
        }
        AJS_HeapDump();
        duk_push_string(ctx, "AJInit.js");
        ret = duk_pcompile_lstring_filename(ctx, 0, aj_init, sizeof(aj_init) - 1);
        if (ret == DUK_EXEC_SUCCESS) {
            ret = duk_pcall(ctx, 0);
        }
        if (ret != DUK_EXEC_SUCCESS) {
            AJ_ErrPrintf(("AJ init script failed %s\n", duk_safe_to_string(ctx, -1)));
            status = AJ_ERR_RESOURCES;
            break;
        }
        duk_pcall(ctx, 0);
        duk_pop(ctx);

        AJS_HeapDump();
        duk_gc(ctx, 0);
        AJS_HeapDump();

        status = AJS_PropertyStoreInit(ctx);
        if (status != AJ_OK) {
            AJ_ErrPrintf(("Failed to initialize property store\n"));
        }

        duk_push_global_object(ctx);
        /*
         * Override the builtin alert and print functions so we can redirect output to debug output
         * or the console if one is attached.
         */
        duk_push_c_function(ctx, NativeOverridePrint, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "print");
        duk_push_c_function(ctx, NativeOverrideAlert, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "alert");
        /*
         * Get the AJ object on the top of the duk stack
         */
        duk_get_prop_string(ctx, -1, AJS_AllJoynObject);
        /*
         * Don't need the global object
         */
        duk_remove(ctx, -2);
        ajIdx = duk_get_top_index(ctx);
        /*
         * Register various functions and object on the AJ object
         */
        AJRegistrations(&ajBus, ctx, ajIdx);
        /*
         * Evaluate the installed script
         */
        if (AJS_IsScriptInstalled()) {
            void* sf = AJS_OpenScript(AJS_SCRIPT_READ);
            if (sf) {
                const uint8_t* js;
                size_t len;
                AJS_ReadScript(sf, &js, &len);
                /*
                 * TODO - get a script name
                 */
                duk_push_string(ctx, "InstalledScript.js");
                ret = duk_pcompile_lstring_filename(ctx, 0, (const char*)js, len);
                if (ret == DUK_EXEC_SUCCESS) {
                    ret = duk_pcall(ctx, 0);
                }
                if (ret != DUK_EXEC_SUCCESS) {
                    AJS_ConsoleSignalError(ctx);
                    status = AJ_ERR_FAILURE;
                }
                duk_pop(ctx);
                if (duk_get_top_index(ctx) != ajIdx) {
                    AJ_ErrPrintf(("ajIdx == %d, top_index == %d\n", (int)ajIdx, (int)duk_get_top_index(ctx)));
                    AJ_ASSERT(duk_get_top_index(ctx) == ajIdx);
                }
                AJS_CloseScript(sf);
                AJS_HeapDump();
                if (status == AJ_OK) {
                    AJ_InfoPrintf(("Installed script from NVRAM\n"));
                } else {
                    AJ_ErrPrintf(("Failed to install script from NVRAM\n"));
                    /*
                     * We don't want to keep trying to install a broken script
                     */
                    AJS_DeleteScript();
                    status = AJ_ERR_RESTART_APP;
                }
                duk_gc(ctx, 0);
                AJS_HeapDump();
            } else {
                status = AJ_ERR_READ;
            }
        }
        if (status == AJ_OK) {
            status = Run(&ajBus, ctx, ajIdx);
        }
        if (status == AJ_ERR_RESTART_APP) {
            status = AJ_OK;
        }
        /*
         * The languages list was on the duktape heap so needs to be deregistered.
         */
        AJ_RegisterDescriptionLanguages(NULL);

        duk_destroy_heap(ctx);

    }
    return status;
}

