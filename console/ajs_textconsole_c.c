/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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

#include "ajs_debug_c.h"
#include "ajs_console_c.h"
#include "ajs_console_common.h"
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

static volatile sig_atomic_t g_interrupt = 0;

static void FatalError(void)
{
    printf("There was a fatal error, exiting\n");
    exit(1);
}


static void SigIntHandler(int sig)
{
    g_interrupt = 1;
}

static char* ReadLine()
{
    char inbuf[1024];
    char* inp = NULL;
    while (!g_interrupt && !inp) {
        inp = fgets(inbuf, sizeof(inbuf), stdin);
    }
    if (inp) {
        size_t len = strlen(inp);
        inp[len - 1] = 0;
    }
    return inp;
}

static int ReadScriptFile(const char* fname, uint8_t** data, size_t* len)
{
    FILE* scriptf;
#ifdef WIN32
    errno_t ret = fopen_s(&scriptf, fname, "rb");
    if (ret != 0) {
        return 0;
    }
#else
    scriptf = fopen(fname, "rb");
    if (!scriptf) {
        return 0;
    }
#endif
    if (fseek(scriptf, 0, SEEK_END) == 0) {
        *len = ftell(scriptf);
        *data = (uint8_t*)malloc(*len);
        if (!*data) {
            FatalError();
        }
        fseek(scriptf, 0, SEEK_SET);
        fread(*data, *len, 1, scriptf);
        fclose(scriptf);
        return 1;
    } else {
        fclose(scriptf);
        return 0;
    }
}

void Notification(const char* appName, const char* deviceName, AJS_NotifText* strings, uint32_t numStrings)
{
    int i;
    printf("Notification: App: %s, Device: %s\n", appName, deviceName);
    for (i = 0; i < numStrings; i++) {
        printf("String %i: [%s] %s\n", i, strings->lang, strings->txt);
    }
}

void Print(const char* message)
{
    printf("PRINT: %s\n", message);
}

void Alert(const char* message)
{
    printf("ALERT: %s\n", message);
}

void DebugVersion(const char* version)
{
    printf("DEBUG VERSION: %s\n", version);
}

void DebugNotification(uint8_t id, uint8_t state, const char* file, const char* function, uint8_t line, uint8_t pc)
{
    printf("DEBUG NOTIFICATION: ID: %i, State: %i, File: %s, Function: %s, Line: %i, PC: %i\n", id, state, file, function, line, pc);
}

void EvalResult(uint8_t code, const char* result)
{
    printf("Eval result=%d: %s\n", code, result);
}

int main(int argc, char** argv)
{
    int status = 1;
    const char* scriptName = NULL;
    const char* deviceName = NULL;
    uint8_t* script = NULL;
    size_t scriptLen = 0;
    SignalRegistration handlers;
    int i;
    AJS_ConsoleCtx* ctx = AJS_ConsoleInit();
    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);
    handlers.notification = &Notification;
    handlers.print = &Print;
    handlers.alert = &Alert;
    handlers.dbgVersion = &DebugVersion;
    handlers.dbgNotification = &DebugNotification;
    handlers.evalResult = &EvalResult;
    /* Register signal handlers */
    AJS_ConsoleRegisterSignalHandlers(ctx, &handlers);
    for (i = 1; i < argc; ++i) {
        if (*argv[i] == '-') {
            if (strcmp(argv[i], "--verbose") == 0) {
                AJS_ConsoleSetVerbose(ctx, 1);
            } else if (strcmp(argv[i], "--name") == 0) {
                if (++i == argc) {
                    AJS_ConsoleDeinit(ctx);
                    goto Usage;
                }
                deviceName = argv[i];
            } else if (strcmp(argv[i], "--debug") == 0) {
                AJS_Debug_SetActiveDebug(ctx, 1);
            } else if (strcmp(argv[i], "--quiet") == 0) {
                AJS_Debug_SetQuiet(ctx, 1);
            } else {
                AJS_ConsoleDeinit(ctx);
                goto Usage;
            }
        } else {
            scriptName = argv[i];
            status = ReadScriptFile(scriptName, &script, &scriptLen);
            if (status != 1) {
                printf("Failed to load script file %s\n", argv[1]);
                AJS_ConsoleDeinit(ctx);
                return -1;
            }
        }
    }
    status = AJS_ConsoleConnect(ctx, deviceName, &g_interrupt);
    if (status == 1) {
        if (script) {
            status = AJS_ConsoleInstall(ctx, scriptName, script, scriptLen);
            free(script);
        }
        if (AJS_Debug_GetActiveDebug(ctx)) {
            AJS_Debug_StartDebugger(ctx);
            AJS_Debug_SetDebugState(ctx, AJS_DEBUG_ATTACHED_PAUSED);
        }
        while (!g_interrupt && (status == 1)) {
            int8_t ret;
            char* input = ReadLine();
            if (input && (strlen(input) > 0)) {
                if (strcmp(input, "quit") == 0) {
                    break;
                }
                if (strcmp(input, "reboot") == 0) {
                    break;
                }
                if (strcmp(input, "detach") == 0) {
                    AJS_Debug_StopDebugger(ctx);
                    continue;
                }
                /* Command line debug commands (only if debugging was enabled at start, and connected)*/
                if (AJS_Debug_GetActiveDebug(ctx)) {
                    if (strcmp(input, "$attach") == 0) {
                        AJS_Debug_StartDebugger(ctx);
                        AJS_Debug_SetDebugState(ctx, AJS_DEBUG_ATTACHED_PAUSED);
                        /* Add breakpoint is before the 'debugConnected' check because they can be added to a running target */
                    } else if (strcmp(input, "$pause") == 0) {
                        AJS_Debug_Pause(ctx);
                        AJS_Debug_SetDebugState(ctx, AJS_DEBUG_ATTACHED_PAUSED);
                    } else if (strcmp(input, "$getscript") == 0) {
                        uint8_t valid;
                        char* targ_script = NULL;
                        uint32_t length;
                        valid = AJS_Debug_GetScript(ctx, (uint8_t**)&targ_script, &length);
                        if (valid) {
                            printf("Script:\n%s", targ_script);
                        } else {
                            printf("No script on target\n");
                        }
                        if (targ_script) {
                            free(targ_script);
                        }
                    } else if (strncmp(input, "$addbreak", 9) == 0) {
                        char* i = (char*)input + 10;
                        int j = 0;
                        char* file;
                        uint8_t line;
                        while (*i != ' ') {
                            i++;
                            j++;
                        }
                        file = (char*)malloc(sizeof(char) * j + 1);
                        if (!file) {
                            FatalError();
                        }
                        memcpy(file, input + 10, j);
                        file[j] = '\0';
                        line = atoi(input + 11 + j);
                        printf("Add Break; File: %s, Line: %u\n", file, line);

                        AJS_Debug_AddBreakpoint(ctx, file, line);
                        free(file);
                        /* Adding a breakpoint will cause the debugger to pause */
                        AJS_Debug_SetDebugState(ctx, AJS_DEBUG_ATTACHED_PAUSED);
                    } else if (AJS_Debug_GetDebugState(ctx) == AJS_DEBUG_ATTACHED_PAUSED) {
                        if (strcmp(input, "$info") == 0) {
                            uint16_t duk_version;
                            uint8_t endianness;
                            char* duk_describe, *target_info;
                            AJS_Debug_BasicInfo(ctx, &duk_version, &duk_describe, &target_info, &endianness);
                            printf("Basic Info Request:\n");
                            printf("\tVersion: %u, Description: %s, Target: %s, Endianness:%u \n", duk_version, duk_describe, target_info, endianness);
                        } else if (strcmp(input, "$trigger") == 0) {
                            AJS_Debug_Trigger(ctx);
                        } else if (strcmp(input, "$resume") == 0) {
                            AJS_Debug_Resume(ctx);
                            AJS_Debug_SetDebugState(ctx, AJS_DEBUG_ATTACHED_RUNNING);
                        } else if (strcmp(input, "$in") == 0) {
                            AJS_Debug_StepIn(ctx);
                        } else if (strcmp(input, "$over") == 0) {
                            AJS_Debug_StepOver(ctx);
                        } else if (strcmp(input, "$out") == 0) {
                            AJS_Debug_StepOut(ctx);
                        } else if (strcmp(input, "$detach") == 0) {
                            AJS_Debug_Detach(ctx);
                            AJS_Debug_SetDebugState(ctx, AJS_DEBUG_DETACHED);
                        } else if (strcmp(input, "$lb") == 0) {
                            AJS_BreakPoint* breakpoint = NULL;
                            uint8_t num;
                            int i;
                            AJS_Debug_ListBreakpoints(ctx, &breakpoint, &num);
                            if (breakpoint) {
                                printf("Breakpoints[%d]: \n", num);
                                for (i = 0; i < num; i++) {
                                    printf("File: %s, Line: %u\n", breakpoint[i].fname, breakpoint[i].line);
                                }
                                AJS_Debug_FreeBreakpoints(ctx, breakpoint, num);
                            }
                        } else if (strcmp(input, "$bt") == 0) {
                            AJS_CallStack* stack = NULL;
                            uint8_t size;
                            int i;
                            AJS_Debug_GetCallStack(ctx, &stack, &size);
                            if (stack) {
                                for (i = 0; i < size; i++) {
                                    printf("File: %s, Function: %s, Line: %u, PC: %u\n", stack[i].filename, stack[i].function, stack[i].line, stack[i].pc);
                                }
                                AJS_Debug_FreeCallStack(ctx, stack, size);
                            }
                        } else if (strcmp(input, "$locals") == 0) {
                            AJS_Locals* vars = NULL;
                            uint16_t size;
                            int i;
                            printf("Local Variables:\n");
                            AJS_Debug_GetLocals(ctx, &vars, &size);
                            if (vars) {
                                if (strcmp(vars[0].name, "N/A") != 0) {
                                    for (i = 0; i < size; i++) {
                                        int j;
                                        printf("Name: %s, Data: ", vars[i].name);
                                        for (j = 0; j < vars[i].size; j++) {
                                            printf("0x%02x, ", vars[i].data[j]);
                                        }
                                        printf("\n");
                                    }

                                } else {
                                    printf("No locals\n");
                                }
                                AJS_Debug_FreeLocals(ctx, vars, size);
                            }
                        } else if (strcmp(input, "$getscript") == 0) {
                            uint8_t valid;
                            char* script;
                            uint32_t length;
                            valid = AJS_Debug_GetScript(ctx, (uint8_t**)&script, &length);
                            if (valid) {
                                printf("Script:\n%s", script);
                            } else {
                                printf("No script on target\n");
                            }
                        } else if (strcmp(input, "$AJS_LOCKDOWN") == 0) {
                            goto DoLockdown;
                        } else if (input[0] == '$') {
                            if (strncmp(input, "$delbreak", 9) == 0) {
                                char* i = (char*)input + 9;
                                int j = 0;
                                uint8_t index;
                                while (*i == ' ') {
                                    i++;
                                    j++;
                                }
                                index = atoi(input + 9 + j);
                                QCC_SyncPrintf("Delete break; Index = %u\n", index);
                                AJS_Debug_DelBreakpoint(ctx, index);
                            } else if (strncmp(input, "$getvar", 7) == 0) {
                                uint8_t* value = NULL;
                                uint32_t size;
                                uint32_t k = 0;
                                uint8_t type;
                                char* i = (char*)input + 8;
                                char* var;
                                int j = 0;
                                while (*i != ' ') {
                                    i++;
                                    j++;
                                }
                                var = (char*)malloc(sizeof(char) * j + 1);
                                if (!var) {
                                    FatalError();
                                }
                                memcpy(var, input + 8, j);
                                var[j] = '\0';
                                printf("Get Var: %s, ", var);
                                AJS_Debug_GetVar(ctx, var, &value, &size, &type);
                                if (value) {
                                    printf("Type: 0x%02x\nVar Bytes: ", type);
                                    while (k < size) {
                                        printf("0x%02x, ", (uint8_t)value[k]);
                                        k++;
                                    }
                                    printf("\n");
                                    free(value);
                                }
                                free(var);
                            } else if (strncmp(input, "$putvar", 7) == 0) {
                                uint8_t type;
                                char* i = (char*)input + 8;
                                char* name;
                                char* value;
                                int j = 0;
                                int k = 0;
                                while (*i != ' ') {
                                    i++;
                                    j++;
                                }
                                /* Get the variables name */
                                name = (char*)malloc(sizeof(char) * j + 1);
                                if (!name) {
                                    FatalError();
                                }
                                memcpy(name, input + 8, j);
                                name[j] = '\0';

                                /* Get the variables string value */
                                value = (char*)malloc(sizeof(char) * (strlen(input) - (8 + j)) + 1);
                                if (!value) {
                                    FatalError();
                                }
                                memcpy(value, input + 9 + j, (strlen(input) - (8 + j)));
                                value[(strlen(input) - (8 + j))] = '\0';
                                /* Must get the variables type to ensure the input is valid */
                                AJS_Debug_GetVar(ctx, name, NULL, NULL, &type);
                                /* Currently basic types are supported (numbers, strings, true/false/null/undef/unused) */
                                if ((type >= 0x60) && (type <= 0x7f)) {
                                    /* Any characters can be treated as a string */
                                    AJS_Debug_PutVar(ctx, name, (uint8_t*)value, strlen(value), type);
                                } else if (type == 0x1a) {
                                    double number;
                                    /* Check the input to ensure its a number */
                                    for (k = 0; k < strlen(value); k++) {
                                        /* ASCII '0' and ASCII '9' */
                                        if (((uint8_t)value[k] <= 48) && ((uint8_t)value[k] >= 57)) {
                                            printf("Invalid input for variable %s\n", name);
                                            break;
                                        }
                                    }
                                    number = atof(value);
                                    AJS_Debug_PutVar(ctx, name, (uint8_t*)&number, sizeof(double), type);
                                }
                                free(name);
                                free(value);
                            } else if (strncmp(input, "$eval", 5) == 0) {
                                char* evalString;
                                uint8_t* value = NULL;
                                uint32_t size;
                                uint8_t type;
                                uint32_t k = 0;
                                if (input[strlen(input) - 1] != ';') {
                                    input += ';';
                                }
                                evalString = (char*)input + 6;
                                AJS_Debug_Eval(ctx, evalString, &value, &size, &type);
                                if (value) {
                                    printf("Type: 0x%02x\nVar Bytes: ", type);
                                    while (k < size) {
                                        printf("0x%02x, ", (uint8_t)value[k]);
                                        k++;
                                    }
                                    printf("\n");
                                    free(value);
                                }
                            }
                        } else {
                            goto DoEval;
                        }
                    }
                    continue;
                } else if (strcmp(input, "$AJS_LOCKDOWN") == 0) {
                    goto DoLockdown;
                }
            DoEval:
                if (input[strlen(input) - 1] != ';') {
                    input += ';';
                }
                ret = AJS_ConsoleEval(ctx, input);
                switch (ret) {
                case -1:
                    /* Eval method failed */
                    break;

                case 0:
                    printf("Eval compile success\n");
                    break;

                case 1:
                    printf("Eval syntax error\n");
                    break;

                case 2:
                    printf("Type or Range error\n");
                    break;

                case 3:
                    printf("Resource Error");
                    break;

                case 5:
                    printf("Internal Duktape Error");
                    break;
                }
                continue;

            DoLockdown:
                ret = AJS_ConsoleLockdown(ctx);
                if (ret != -1) {
                    QCC_SyncPrintf("Console was successfully locked down. Session will be terminated\n");
                }
                continue;
            }
        }
    } else {
        if (status == 0) {
            printf("Connection was rejected\n");
        } else {
            printf("Failed to connect to script console\n");
        }
    }
    if (g_interrupt) {
        printf(("Interrupted by Ctrl-C\n"));
    }
    AJS_ConsoleDeinit(ctx);
    return -((int)status);
Usage:

    printf("usage: %s [--verbose] [--debug] [--quiet] [--name <device-name>] [javascript-file]\n", argv[0]);
    AJS_ConsoleDeinit(ctx);
    return -1;
}
