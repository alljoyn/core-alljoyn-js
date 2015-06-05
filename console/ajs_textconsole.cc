/**
 * @file
 */
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

#include <alljoyn/Init.h>
#include "ajs_console.h"
#include <stdio.h>

static volatile sig_atomic_t g_interrupt = false;

using namespace qcc;

static char inbuf[1024] = { 0 };

class AJS_TextConsole : public AJS_Console {
  public:
    virtual void Print(const char* fmt, ...);

  private:
    static const size_t printBufLen = 1024;
    char printBuf[printBufLen];
};

void AJS_TextConsole::Print(const char* fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);

    ret = vsnprintf(printBuf, printBufLen, fmt, ap);
    if (ret > 0) {
        QCC_SyncPrintf("%s", printBuf);
    }

    va_end(ap);
}

static void FatalError(void)
{
    printf("There was a fatal error, exiting\n");
    exit(1);
}

static void SigIntHandler(int sig)
{
    g_interrupt = true;
}

static String ReadLine()
{
    char* inp = NULL;
    while (!g_interrupt && !inp) {
        inp = fgets(inbuf, sizeof(inbuf), stdin);
    }
    if (inp) {
        size_t len = strlen(inbuf);
        inp[len - 1] = 0;
    }
    return inbuf;
}

static QStatus ReadScriptFile(const char* fname, uint8_t** data, size_t* len)
{
    QStatus status = ER_OK;
    FILE* scriptf;
#ifdef WIN32
    errno_t ret = fopen_s(&scriptf, fname, "rb");
    if (ret != 0) {
        return ER_OPEN_FAILED;
    }
#else
    scriptf = fopen(fname, "rb");
    if (!scriptf) {
        return ER_OPEN_FAILED;
    }
#endif
    if (fseek(scriptf, 0, SEEK_END) == 0) {
        *len = ftell(scriptf);
        *data = (uint8_t*)malloc(*len);
        if (!*data) {
            QCC_SyncPrintf("ReadScriptFile(): Malloc failed to allocate %d bytes\n", *len);
            FatalError();
        }
        fseek(scriptf, 0, SEEK_SET);
        fread(*data, *len, 1, scriptf);
    } else {
        status = ER_READ_ERROR;
    }
    fclose(scriptf);
    return status;
}

int main(int argc, char** argv)
{
    QStatus status;
    AJS_TextConsole* ajsConsole = new AJS_TextConsole();
    const char* scriptName = NULL;
    const char* deviceName = NULL;
    uint8_t* script = NULL;
    size_t scriptLen = 0;

    AllJoynInit();
    AllJoynRouterInit();
    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    for (int i = 1; i < argc; ++i) {
        if (*argv[i] == '-') {
            if (strcmp(argv[i], "--verbose") == 0) {
                ajsConsole->SetVerbose(true);
            } else if (strcmp(argv[i], "--name") == 0) {
                if (++i == argc) {
                    goto Usage;
                }
                deviceName = argv[i];
            } else if (strcmp(argv[i], "--debug") == 0) {
                ajsConsole->activeDebug = true;
            } else if (strcmp(argv[i], "--quiet") == 0) {
                ajsConsole->quiet = true;
            } else {
                goto Usage;
            }
        } else {
            scriptName = argv[i];
            status = ReadScriptFile(scriptName, &script, &scriptLen);
            if (status != ER_OK) {
                QCC_LogError(status, ("Failed to load script file %s\n", argv[1]));
                return -1;
            }
        }
    }
    status = ajsConsole->Connect(deviceName, &g_interrupt);

    if (status == ER_OK) {
        if (scriptLen) {
            status = ajsConsole->Install(scriptName, script, scriptLen);
            free(script);
            script = NULL;
        }
        if (ajsConsole->activeDebug) {
            ajsConsole->StartDebugger();
            ajsConsole->SetDebugState(AJS_DEBUG_ATTACHED_PAUSED);
        }
        while (!g_interrupt && (status == ER_OK)) {
            int8_t ret;
            String input = ReadLine();
            if (input.size() > 0) {
                if (input == "quit") {
                    break;
                }
                if (input == "reboot") {
                    break;
                }
                if (input == "detach") {
                    ajsConsole->StopDebugger();
                    continue;
                }
                if (strncmp(input.c_str(), "$install", 8) == 0) {
                    const char* fname;
                    uint8_t* newscript;
                    size_t newlen;

                    if (strlen(input.c_str()) <= 9) {
                        QCC_SyncPrintf("$install requires a script as a parameter\n");
                        continue;
                    }
                    fname = input.c_str() + 9;
                    newscript = NULL;
                    newlen = 0;
                    status = ReadScriptFile(fname, &newscript, &newlen);
                    if (status != ER_OK || newlen == 0) {
                        QCC_LogError(status, ("Failed to load script file %s\n", fname));
                    } else {
                        ajsConsole->Detach();
                        status = ajsConsole->Install(fname, newscript, newlen);
                        free(newscript);
                        if (status != ER_OK) {
                            QCC_LogError(status, ("Failed to install script %s\n", fname));
                        }
                        ajsConsole->StartDebugger();
                    }
                    continue;
                }
                /* Command line debug commands (only if debugging was enabled at start, and connected)*/
                if (ajsConsole->activeDebug) {
                    if (input == "$attach") {
                        ajsConsole->StartDebugger();
                        ajsConsole->SetDebugState(AJS_DEBUG_ATTACHED_PAUSED);
                        /* Add breakpoint is before the 'debugConnected' check because they can be added to a running target */
                    } else if (input == "$pause") {

                        ajsConsole->Pause();
                        ajsConsole->SetDebugState(AJS_DEBUG_ATTACHED_PAUSED);

                    } else if (input == "$getscript") {
                        bool valid;
                        char* targ_script = NULL;
                        uint32_t length;
                        valid = ajsConsole->GetScript((uint8_t**)&targ_script, &length);
                        if (valid) {
                            QCC_SyncPrintf("Script:\n%s", targ_script);
                        } else {
                            QCC_SyncPrintf("No script on target\n");
                        }
                        if (targ_script) {
                            free(targ_script);
                        }
                    } else if (strncmp(input.c_str(), "$addbreak", 9) == 0) {
                        char* i = (char*)input.c_str() + 10;
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
                        memcpy(file, input.c_str() + 10, j);
                        file[j] = '\0';
                        line = atoi(input.c_str() + 11 + j);
                        QCC_SyncPrintf("Add Break; File: %s, Line: %u\n", file, line);
                        ajsConsole->AddBreak(file, line);
                        free(file);
                        /* Adding a breakpoint will cause the debugger to pause */
                        ajsConsole->SetDebugState(AJS_DEBUG_ATTACHED_PAUSED);
                    } else if (ajsConsole->GetDebugState() == AJS_DEBUG_ATTACHED_PAUSED) {
                        if (input == "$info") {
                            uint16_t duk_version;
                            uint8_t endianness;
                            char* duk_describe, *target_info;
                            ajsConsole->BasicInfo(&duk_version, &duk_describe, &target_info, &endianness);
                            QCC_SyncPrintf("Basic Info Request:\n");
                            QCC_SyncPrintf("\tVersion: %u, Description: %s, Target: %s, Endianness:%u \n", duk_version, duk_describe, target_info, endianness);
                        } else if (input == "$trigger") {
                            ajsConsole->Trigger();
                        } else if (input == "$resume") {
                            ajsConsole->Resume();
                            ajsConsole->SetDebugState(AJS_DEBUG_ATTACHED_RUNNING);
                        } else if (input == "$in") {
                            ajsConsole->StepIn();
                        } else if (input == "$over") {
                            ajsConsole->StepOver();
                        } else if (input == "$out") {
                            ajsConsole->StepOut();
                        } else if (input == "$detach") {
                            ajsConsole->Detach();
                            ajsConsole->SetDebugState(AJS_DEBUG_DETACHED);
                        } else if (input == "$dump") {
                            ajsConsole->DumpHeap();
                        } else if (input == "$lb") {
                            AJS_BreakPoint* breakpoint = NULL;
                            uint8_t num;
                            int i;
                            ajsConsole->ListBreak(&breakpoint, &num);
                            if (breakpoint) {
                                QCC_SyncPrintf("Breakpoints[%d]: \n", num);
                                for (i = 0; i < num; i++) {
                                    QCC_SyncPrintf("File: %s, Line: %u\n", breakpoint[i].fname, breakpoint[i].line);
                                }
                                ajsConsole->FreeBreakpoints(breakpoint, num);
                            }
                        } else if (input == "$bt") {
                            AJS_CallStack* stack = NULL;
                            uint8_t size;
                            int i;
                            ajsConsole->GetCallStack(&stack, &size);
                            if (stack) {
                                for (i = 0; i < size; i++) {
                                    QCC_SyncPrintf("File: %s, Function: %s, Line: %u, PC: %u\n", stack[i].filename, stack[i].function, stack[i].line, stack[i].pc);
                                }
                                ajsConsole->FreeCallStack(stack, size);
                            }
                        } else if (input == "$locals") {
                            AJS_Locals* vars = NULL;
                            uint16_t size;
                            int i;
                            QCC_SyncPrintf("Local Variables:\n");
                            ajsConsole->GetLocals(&vars, &size);
                            if (vars) {
                                if (strcmp(vars[0].name, "N/A") != 0) {
                                    for (i = 0; i < size; i++) {
                                        uint32_t j;
                                        QCC_SyncPrintf("Name: %s, Data: ", vars[i].name);
                                        for (j = 0; j < vars[i].size; j++) {
                                            QCC_SyncPrintf("0x%02x, ", vars[i].data[j]);
                                        }
                                        QCC_SyncPrintf("\n");
                                    }

                                } else {
                                    QCC_SyncPrintf("No locals\n");
                                }
                                ajsConsole->FreeLocals(vars, size);
                            }
                        } else if (input == "$getscript") {
                            bool valid;
                            char* script;
                            uint32_t length;
                            valid = ajsConsole->GetScript((uint8_t**)&script, &length);
                            if (valid) {
                                QCC_SyncPrintf("Script:\n%s", script);
                                free(script);
                            } else {
                                QCC_SyncPrintf("No script on target\n");
                            }
                        } else if (input == "$AJS_LOCKDOWN") {
                            goto DoLockdown;
                        } else if (input.c_str()[0] == '$') {
                            if (strncmp(input.c_str(), "$delbreak", 9) == 0) {
                                char* i = (char*)input.c_str() + 9;
                                int j = 0;
                                uint8_t index;
                                while (*i == ' ') {
                                    i++;
                                    j++;
                                }
                                index = atoi(input.c_str() + 9 + j);
                                QCC_SyncPrintf("Delete break; Index = %u\n", index);
                                ajsConsole->DelBreak(index);
                            } else if (strncmp(input.c_str(), "$getvar", 7) == 0) {
                                uint8_t* value = NULL;
                                uint32_t size;
                                uint32_t k = 0;
                                uint8_t type;
                                char* i = (char*)input.c_str() + 8;
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
                                memcpy(var, input.c_str() + 8, j);
                                var[j] = '\0';
                                QCC_SyncPrintf("Get Var: %s, ", var);
                                ajsConsole->GetVar(var, &value, &size, &type);
                                if (value) {
                                    QCC_SyncPrintf("Type: 0x%02x\nVar Bytes: ", type);
                                    while (k < size) {
                                        QCC_SyncPrintf("0x%02x, ", (uint8_t)value[k]);
                                        k++;
                                    }
                                    QCC_SyncPrintf("\n");
                                    free(value);
                                }
                                free(var);
                            } else if (strncmp(input.c_str(), "$putvar", 7) == 0) {
                                uint8_t type;
                                char* i = (char*)input.c_str() + 8;
                                char* name;
                                char* value;
                                int j = 0;
                                uint32_t k = 0;
                                while (*i != ' ') {
                                    i++;
                                    j++;
                                }
                                /* Get the variables name */
                                name = (char*)malloc(sizeof(char) * j + 1);
                                if (!name) {
                                    FatalError();
                                }
                                memcpy(name, input.c_str() + 8, j);
                                name[j] = '\0';

                                /* Get the variables string value */
                                value = (char*)malloc(sizeof(char) * (strlen(input.c_str()) - (8 + j)) + 1);
                                if (!value) {
                                    FatalError();
                                }
                                memcpy(value, input.c_str() + 9 + j, (strlen(input.c_str()) - (8 + j)));
                                value[(strlen(input.c_str()) - (8 + j))] = '\0';
                                /* Must get the variables type to ensure the input is valid */
                                ajsConsole->GetVar(name, NULL, NULL, &type);
                                /* Currently basic types are supported (numbers, strings, true/false/null/undef/unused) */
                                if ((type >= 0x60) && (type <= 0x7f)) {
                                    /* Any characters can be treated as a string */
                                    ajsConsole->PutVar(name, (uint8_t*)value, strlen(value), type);
                                } else if (type == 0x1a) {
                                    double number;
                                    /* Check the input to ensure its a number */
                                    for (k = 0; k < strlen(value); k++) {
                                        /* ASCII '0' and ASCII '9' */
                                        if (((uint8_t)value[k] <= 48) && ((uint8_t)value[k] >= 57)) {
                                            QCC_SyncPrintf("Invalid input for variable %s\n", name);
                                            break;
                                        }
                                    }
                                    number = atof(value);
                                    ajsConsole->PutVar(name, (uint8_t*)&number, sizeof(double), type);
                                }
                                free(name);
                                free(value);
                            } else if (strncmp(input.c_str(), "$eval", 5) == 0) {
                                char* evalString;
                                uint8_t* value = NULL;
                                uint32_t size;
                                uint8_t type;
                                uint32_t k = 0;
                                if (input[input.size() - 1] != ';') {
                                    input += ';';
                                }
                                evalString = (char*)input.c_str() + 6;
                                ajsConsole->DebugEval(evalString, &value, &size, &type);
                                if (value) {
                                    QCC_SyncPrintf("Type: 0x%02x\nVar Bytes: ", type);
                                    while (k < size) {
                                        QCC_SyncPrintf("0x%02x, ", (uint8_t)value[k]);
                                        k++;
                                    }
                                    QCC_SyncPrintf("\n");
                                    free(value);
                                }
                            }
                        } else {
                            goto DoEval;
                        }
                    }
                    continue;
                } else if (input == "$AJS_LOCKDOWN") {
                    goto DoLockdown;
                }
            DoEval:
                if (input[input.size() - 1] != ';') {
                    input += ';';
                }
                ret = ajsConsole->Eval(input);
                switch (ret) {
                case -1:
                    /* Eval method failed */
                    break;

                case 0:
                    QCC_SyncPrintf("Eval compile success\n");
                    break;

                case 1:
                    QCC_SyncPrintf("Eval syntax error\n");
                    break;

                case 2:
                    QCC_SyncPrintf("Type or Range error\n");
                    break;

                case 3:
                    QCC_SyncPrintf("Resource Error");
                    break;

                case 5:
                    QCC_SyncPrintf("Internal Duktape Error");
                    break;
                }
                continue;

            DoLockdown:
                ret = ajsConsole->LockdownConsole();
                if (ret != -1) {
                    QCC_SyncPrintf("Console was successfully locked down. Session will be terminated\n");
                }
                continue;
            }
        }
    } else {
        if (status == ER_ALLJOYN_JOINSESSION_REPLY_REJECTED) {
            QCC_SyncPrintf("Connection was rejected\n");
        } else {
            QCC_SyncPrintf("Failed to connect to script console\n");
        }
    }
    if (g_interrupt) {
        QCC_SyncPrintf(("Interrupted by Ctrl-C\n"));
    }
    delete ajsConsole;
    AllJoynShutdown();
    AllJoynRouterShutdown();
    return -((int)status);

Usage:

    QCC_SyncPrintf("usage: %s [--verbose] [--debug] [--quiet] [--name <device-name>] [javascript-file]\n", argv[0]);
    return -1;
}
