/**
 * @file
 */
/******************************************************************************
 *  * Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include "ajs.h"
#include "ajs_util.h"
#include "ajs_security.h"
#include "ajs_target.h"
#include "ajs_ctrlpanel.h"
#include "ajs_heap.h"
#include "ajs_storage.h"
#include <ajtcl/aj_security.h>
#include <ajtcl/aj_creds.h>
#include <ajtcl/aj_util.h>
#include <ajtcl/aj_introspect.h>
#include <ajtcl/services/ControlPanelService.h>

/**
 * Turn on per-module debug printing by setting this variable to non-zero value
 * (usually in debugger).
 */
#ifndef NDEBUG
uint8_t dbgAJS = 1;
#endif

static AJ_BusAttachment ajBus;
static uint8_t lockdown = AJS_CONSOLE_LOCK_ERR;

const char* AJS_AJObjectName = "\377AJ";
const char* AJS_IOObjectName = "\377IO";

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
#ifdef CONTROLPANEL_SERVICE
    AJS_RegisterControlPanelHandlers(aj, ctx, ajIdx);
#endif
    /*
     * Register translations table
     */
    AJS_RegisterTranslations(ctx, ajIdx);
    /*
     * Compact the AllJoyn object
     */
    duk_compact(ctx, ajIdx);
}

static uint8_t ajRunning = FALSE;
static uint8_t busAttached = FALSE;

uint8_t AJS_IsRunning()
{
    return ajRunning;
}

/*
 * Gets the current lockdown configuration.
 */
AJ_Status AJS_GetLockdownState(uint8_t* state)
{
    AJ_NV_DATASET* ds = NULL;
    uint8_t bit = AJS_CONSOLE_UNLOCKED;
    /*
     * If this is the first get then we need to update the global from NVRAM
     */
    if (lockdown == AJS_CONSOLE_LOCK_ERR) {
        /*
         * If there is no NVRAM entry create one and set to unlocked
         */
        if (!AJ_NVRAM_Exist(AJS_LOCKDOWN_NVRAM_ID)) {
            ds = AJ_NVRAM_Open(AJS_LOCKDOWN_NVRAM_ID, "w", 1);
            if (ds) {
                if (AJ_NVRAM_Write(&bit, 1, ds) != 1) {
                    AJ_ErrPrintf(("SetLockdownBit(): Error writing NVRAM entry\n"));
                    lockdown = bit;
                }
                AJ_NVRAM_Close(ds);
            }
            *state = bit;
            return AJ_OK;
        }
        ds = AJ_NVRAM_Open(AJS_LOCKDOWN_NVRAM_ID, "r", 1);
        if (ds) {
            if (AJ_NVRAM_Read(&bit, 1, ds) != 1) {
                AJ_ErrPrintf(("GetLockdownBit(): Error reading NVRAM entry\n"));
                return AJ_ERR_NVRAM_READ;
            }
            /* Update the global lockdown */
            if (lockdown != bit) {
                lockdown = bit;
            }
            AJ_NVRAM_Close(ds);
        }
    }
    *state = lockdown;
    return AJ_OK;
}

AJ_Status AJS_SetLockdownState(uint8_t state)
{
    AJ_NV_DATASET* ds = NULL;
    if (state == AJS_CONSOLE_UNLOCKED || state == AJS_CONSOLE_LOCKED) {
        ds = AJ_NVRAM_Open(AJS_LOCKDOWN_NVRAM_ID, "w", 1);
        if (ds) {
            if (AJ_NVRAM_Write(&state, 1, ds) != 1) {
                AJ_ErrPrintf(("SetLockdownBit(): Error writing NVRAM entry\n"));
            }
            lockdown = state;
            AJ_NVRAM_Close(ds);
        } else {
            AJ_ErrPrintf(("AJS_SetLockdownBit(): Error opening lockdown dataset\n"));
            return AJ_ERR_FAILURE;
        }
    } else {
        AJ_ErrPrintf(("AJS_SetLockdownBit(): Error, bit must be 0 (zero) or 1 (one). You supplied %d\n", state));
        return AJ_ERR_INVALID;
    }
    return AJ_OK;
}

static AJ_Status Run(AJ_BusAttachment* aj, duk_context* ctx)
{
    AJ_Status status = AJ_OK;
    duk_idx_t ajIdx = -1;

    if (duk_get_global_string(ctx, AJS_AJObjectName)) {
        ajIdx = duk_get_top_index(ctx);
        /*
         * Add JavaScript objects and interfaces registered by the script
         */
        status = AJS_InitTables(ctx, ajIdx);
    }
    while (status == AJ_OK) {
        /*
         * Attach our BusAttachment to an AllJoyn routing node
         */
        if (!busAttached) {
            AJS_HeapDump();
            status = AJS_AttachAllJoyn(aj);
            if (status == AJ_OK) {
#if !defined(AJS_CONSOLE_LOCKDOWN)
                if (lockdown == AJS_CONSOLE_UNLOCKED) {
                    status = AJS_ConsoleInit(aj);
                }
#endif
            }
#ifdef CONTROLPANEL_SERVICE
            /*
             * Prepare the ControlPanel connection.
             */
            if (status == AJ_OK) {
                status = AJCPS_ConnectedHandler(aj);
            }
#endif
            if (status == AJ_OK) {
                /*
                 * Bind the application session port
                 */
                status = AJ_BusBindSessionPort(aj, AJS_APP_PORT, NULL, AJ_FLAG_NO_REPLY_EXPECTED);
            }
            /*
             * If securityDefinition was populated, enable security
             * after we attach to alljoyn. AJS_EnableSecurity will check
             * if rules were created while parsing securityDefinition
             */
            if (status == AJ_OK) {
                status = AJS_EnableSecurity(ctx);
            }
            if (status != AJ_OK) {
                break;
            }
            busAttached = TRUE;
        }
        /*
         * Let JavaScript know we are attached to the bus
         */
        if (ajIdx >= 0) {
            ajRunning = TRUE;
            duk_get_prop_string(ctx, ajIdx, "onAttach");
            if (duk_is_callable(ctx, -1)) {
                duk_dup(ctx, ajIdx);
                if (duk_pcall_method(ctx, 0) != DUK_EXEC_SUCCESS) {
                    AJS_ConsoleSignalError(ctx);
                }
            }
            duk_pop(ctx);
        }
        /*
         * Start running
         */
        status = AJS_MessageLoop(ctx, aj, ajIdx);
    }
    AJ_WarnPrintf(("Exiting AJS main loop with status=%s\n", AJ_StatusText(status)));
    /*
     * JavaScript table entries are no longer valid
     */
    AJS_ResetTables(ctx);
    /*
     * Make sure the control panel service is terminated
     */
#ifdef CONTROLPANEL_SERVICE
    AJS_CP_Terminate();
#endif
    /*
     * If we told JavaScript we are attached now indicate we are detached
     */
    if (ajRunning) {
        duk_get_prop_string(ctx, ajIdx, "onDetach");
        if (duk_is_callable(ctx, -1)) {
            duk_dup(ctx, ajIdx);
            if (duk_pcall_method(ctx, 0) != DUK_EXEC_SUCCESS) {
                AJS_ConsoleSignalError(ctx);
            }
        }
        duk_pop(ctx);
        ajRunning = FALSE;
    }
    /*
     * A RESTART_APP status indicates that the script engine must be restarted but we can keep our
     * existing BusAttachment.
     */
    if (status != AJ_ERR_RESTART_APP) {
#if !defined(AJS_CONSOLE_LOCKDOWN)
        if (lockdown == AJS_CONSOLE_UNLOCKED) {
            AJS_ConsoleTerminate();
        }
#endif
        AJS_DetachAllJoyn(aj, status);
        busAttached = FALSE;
    }

    duk_pop(ctx);
    return status;
}

static const uint8_t icon[] = {
#include "icon.inc"
};

static duk_ret_t NativeOverrideEval(duk_context* ctx)
{
    AJ_ErrPrintf(("Eval is not allowed\n"));
    return 0;
}

#if !defined(AJS_CONSOLE_LOCKDOWN)
static duk_ret_t NativeOverrideAlert(duk_context* ctx)
{
    AJS_AlertHandler(ctx, 1 /* alert */);
    return 0;
}

static duk_ret_t NativeOverridePrint(duk_context* ctx)
{
    AJS_AlertHandler(ctx, 0 /* print */);
    return 0;
}
#endif

static const duk_number_list_entry AJ_constants[] = {
    { "METHOD",      0 },
    { "SIGNAL",      1 },
    { "PROPERTY",    2 },

    { "READONLY",  1},
    { "ANNOUNCE",  2},
    { "LOCALIZED", 4},
    { "PRIVATE",   8},

    { "SECURE",    AJ_OBJ_FLAG_SECURE    },
    { "HIDDEN",    AJ_OBJ_FLAG_HIDDEN    },
    { "DISABLED",  AJ_OBJ_FLAG_DISABLED  },
    { "ANNOUNCED", AJ_OBJ_FLAG_ANNOUNCED },
    { "PROXY",     AJ_OBJ_FLAG_IS_PROXY  },

    { "CRED_ALL", 0 },
    { "CRED_GENERIC",   AJ_CRED_TYPE_GENERIC     },
    { "CRED_AES",       AJ_CRED_TYPE_AES         },
    { "CRED_PRIVATE",   AJ_CRED_TYPE_PRIVATE     },
    { "CRED_PEM",       AJ_CRED_TYPE_PEM         },
    { "CRED_PUBLIC",    AJ_CRED_TYPE_PUBLIC      },
    { "CRED_CERT",      AJ_CRED_TYPE_CERTIFICATE },
    { "CRED_MANIFESTS", AJ_CRED_TYPE_MANIFESTS   },
    { "CRED_POLICY",    AJ_CRED_TYPE_POLICY      },
    { "CRED_CONFIG",    AJ_CRED_TYPE_CONFIG      },

    { "CLAIM_NULL",  CLAIM_CAPABILITY_ECDHE_NULL  },
    { "CLAIM_ECDSA", CLAIM_CAPABILITY_ECDHE_ECDSA },
    { "CLAIM_SPEKE", CLAIM_CAPABILITY_ECDHE_SPEKE },

    {"TRANPSORT_NONE",  AJ_TRANSPORT_NONE},
    {"TRANSPORT_LOCAL", AJ_TRANSPORT_LOCAL},
    {"TRANSPORT_TCP",   AJ_TRANSPORT_TCP},
    {"TRANSPORT_UDP",   AJ_TRANSPORT_UDP},
    {"TRANSPORT_IP",    AJ_TRANSPORT_IP},
    {"TRANSPORT_ANY",   AJ_TRANSPORT_ANY},

    { NULL }
};

static const duk_number_list_entry AJ_config_constants[] = {
    { "linkTimeout",    10000 },
    { "callTimeout",    10000 },
    { "minProtoVersion",   12 },
    { NULL }
};

static void InitAllJoynObject(duk_context* ctx, duk_idx_t ajIdx)
{
    duk_put_number_list(ctx, ajIdx, AJ_constants);
    duk_push_object(ctx);
    duk_put_number_list(ctx, -1, AJ_config_constants);
    duk_put_prop_string(ctx, ajIdx, "config");
    duk_push_object(ctx);
    duk_put_prop_string(ctx, ajIdx, "aboutDefinition");
    duk_push_object(ctx);
    duk_put_prop_string(ctx, ajIdx, "interfaceDefinition");
    duk_push_object(ctx);
    duk_put_prop_string(ctx, ajIdx, "objectDefinition");
    duk_push_object(ctx);
    duk_put_prop_string(ctx, ajIdx, "securityDefinition");
    /*
     * Register exported properties on the AllJoyn object
     */
    AJRegistrations(&ajBus, ctx, ajIdx);
    /*
     * We need an internal name so other code can lookup the AllJoyn object.
     * We use a hidden name for this.
     */
    duk_dup(ctx, ajIdx);
    duk_put_global_string(ctx, AJS_AJObjectName);
}

static duk_ret_t NativeModuleLoader(duk_context* ctx)
{
    const char* id = duk_require_string(ctx, 0);

    if (strcmp(id, "AllJoyn") == 0) {
        InitAllJoynObject(ctx, 2);
    } else if (strcmp(id, "IO") == 0) {
        /*
         * Register target-specific I/O functions
         */
        AJS_RegisterIO(ctx, 2);
    } else {
        /*
         * Load any target specific modules
         */
        if (AJS_TargetModuleLoad(ctx, 2, id) != AJ_OK) {
            duk_push_sprintf(ctx, "Unknown module: \"%s\"\n", id);
            duk_throw(ctx);
        }
    }
    return 0;
}

AJ_Status AJS_Main(const char* deviceName)
{
    AJ_Status status = AJ_OK;
    duk_context* ctx;
    duk_int_t ret;

#if defined(AJS_CONSOLE_LOCKDOWN)
    /* If lockdown is compiled in then set the bit */
    status = AJS_SetLockdownState(AJS_CONSOLE_LOCKED);
    lockdown = AJS_CONSOLE_LOCKED;
    if (status != AJ_OK) {
        AJ_ErrPrintf(("AJS_Main(): Error setting lockdown state, status = %s\n", AJ_StatusText(status)));
        return status;
    }
#else
    /* Get the current lockdown bit */
    status = AJS_GetLockdownState(&lockdown);
    if (status != AJ_OK) {
        AJ_ErrPrintf(("AJS_Main(): Error getting lockdown state, status = %s\n", AJ_StatusText(status)));
        return status;
    }
#endif


    AJ_AboutSetIcon(icon, sizeof(icon), "image/jpeg", NULL);

    /*
     * Zero out the bus attachment
     */
    memset(&ajBus, 0, sizeof(ajBus));

    while (status == AJ_OK) {

        status = AJS_HeapCreate();
        if (status != AJ_OK) {
            break;
        }
        ctx = duk_create_heap(AJS_Alloc, AJS_Realloc, AJS_Free, &ctx, ErrorHandler);
        if (!ctx) {
            AJ_ErrPrintf(("Failed to create duktape heap\n"));
            status = AJ_ERR_RESOURCES;
            AJS_HeapDestroy();
            break;
        }
        AJS_HeapDump();
        /*
         * Register module loader (search) function with the Duktape object
         */
        duk_get_global_string(ctx, "Duktape");
        duk_push_c_function(ctx, NativeModuleLoader, 4);
        duk_put_prop_string(ctx, -2, "modSearch");
        /*
         * The Duktape.act() and Duktape.info() functions may unsafe so we disable them.
         */
        duk_push_undefined(ctx);
        duk_put_prop_string(ctx, -2, "act");
        duk_push_undefined(ctx);
        duk_put_prop_string(ctx, -2, "info");
        /*
         * Disable the "eval" function
         */
        duk_push_c_function(ctx, NativeOverrideEval, DUK_VARARGS);
        duk_put_global_string(ctx, "eval");
#if !defined(AJS_CONSOLE_LOCKDOWN)
        if (lockdown == AJS_CONSOLE_UNLOCKED) {
            /*
             * Override the builtin alert and print functions so we can redirect output to debug output
             * or the console if one is attached.
             */
            duk_push_c_function(ctx, NativeOverridePrint, DUK_VARARGS);
            duk_put_global_string(ctx, "print");
            duk_push_c_function(ctx, NativeOverrideAlert, DUK_VARARGS);
            duk_put_global_string(ctx, "alert");
        }
#endif
        if (status != AJ_OK) {
            AJ_ErrPrintf(("Failed to initialize property store\n"));
        }
        /*
         * Register setTimeout, setInterval timer functions.
         */
        AJS_RegisterTimerFuncs(ctx);
        /*
         * Evaluate the installed script
         */
        if (AJ_NVRAM_Exist(AJS_SCRIPT_NAME_NVRAM_ID)) {
            AJ_NV_DATASET* ds;
            void* sctx = NULL;
            /*
             * Read the script name if there is one
             */
            ds = AJ_NVRAM_Open(AJS_SCRIPT_NAME_NVRAM_ID, "r", 0);
            if (ds) {
                const char* scriptName = AJ_NVRAM_Peek(ds);
                if (scriptName) {
                    duk_push_string(ctx, scriptName);
                } else {
                    duk_push_string(ctx, "InstalledScript.js");
                }
                AJ_NVRAM_Close(ds);
            }
            /*
             * Now load the script
             */
            status = AJS_OpenScript(0, &sctx);
            if (status != AJ_OK) {
                AJ_WarnPrintf(("AJS_Main(): Script does not exist\n"));
                /*
                 * We want to go into Run() which requires the status to be AJ_OK
                 */
                status = AJ_OK;
                goto Run;
            } else {
                uint32_t len;
                const char* js;
                status = AJS_ReadScript((uint8_t**)&js, &len, sctx);
                if (status != AJ_OK) {
                    AJ_ErrPrintf(("AJS_Main(): Error reading script, status = %s\n", AJ_StatusText(status)));
                    AJS_CloseScript(sctx);
                    /*
                     * At this point we should restart
                     */
                    status = AJ_ERR_RESTART;
                    goto Run;
                }
                AJS_CloseScript(sctx);
                ret = duk_pcompile_lstring_filename(ctx, 0, (const char*)js, len);
                if (ret == DUK_EXEC_SUCCESS) {
                    AJS_SetWatchdogTimer(AJS_DEFAULT_WATCHDOG_TIMEOUT);
                    ret = duk_pcall(ctx, 0);
                    AJS_ClearWatchdogTimer();
                }
                if (ret != DUK_EXEC_SUCCESS) {
                    AJS_ConsoleSignalError(ctx);
                    status = AJ_ERR_FAILURE;
                }
                duk_pop(ctx);
                if (status == AJ_OK) {
                    AJ_InfoPrintf(("Installed script from NVRAM\n"));
                } else {
                    AJ_ErrPrintf(("Deleting bad script from NVRAM\n"));
                    /*
                     * We don't want to repeatedly attempt to install a broken script
                     */
                    AJS_DeleteScript();
                    /*
                     * Delete the name entry so after restart we will go into Run()
                     */
                    AJ_NVRAM_Delete(AJS_SCRIPT_NAME_NVRAM_ID);
                    status = AJ_ERR_RESTART_APP;
                }
                duk_gc(ctx, 0);
                AJS_HeapDump();
            }
        }
    Run:
        status = AJS_PropertyStoreInit(ctx, deviceName);
        if (status == AJ_OK) {
            status = Run(&ajBus, ctx);
        }
        if (status == AJ_ERR_RESTART_APP) {
            status = AJ_OK;
        }
        /*
         * The languages list was on the duktape heap so needs to be deregistered.
         */
        AJ_RegisterDescriptionLanguages(NULL);

        duk_destroy_heap(ctx);
        AJS_HeapDestroy();
    }
    /*
     * Returning here will cause AllJoyn to fully restart so a fatal error
     * can be ignored.
     */
    if ((status == AJ_ERR_READ) || (status == AJ_ERR_WRITE)) {
        status = AJ_ERR_RESTART;
    }
    return status;
}