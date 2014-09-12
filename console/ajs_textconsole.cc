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

#include "ajs_console.h"

static volatile sig_atomic_t g_interrupt = false;

class AJS_TextConsole : public AJS_Console
{
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

static void SigIntHandler(int sig)
{
    g_interrupt = true;
}

static String ReadLine()
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

static QStatus ReadScriptFile(const char* fname, uint8_t** data, size_t* len)
{
    FILE*scriptf;
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
    if (fseek((FILE*)scriptf, 0, SEEK_END) == 0) {
        *len = ftell((FILE*)scriptf);
        *data = (uint8_t*)malloc(*len);
        fseek((FILE*)scriptf, 0, SEEK_SET);
        fread(*data, *len, 1, (FILE*)scriptf);
        return ER_OK;
    } else {
        return ER_READ_ERROR;
    }
}

int main(int argc, char** argv)
{
    QStatus status;
    AJS_TextConsole ajsConsole;
    const char* scriptName = NULL;
    const char* deviceName = NULL;
    uint8_t* script = NULL;
    size_t scriptLen = 0;

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    for (int i = 1; i < argc; ++i) {
        if (*argv[i] == '-') {
            if (strcmp(argv[i], "--verbose") == 0) {
                ajsConsole.SetVerbose(true);
            } else if (strcmp(argv[i], "--name") == 0) {
                if (++i == argc) {
                    goto Usage;
                }
                deviceName = argv[i];
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

    status = ajsConsole.Connect(deviceName, &g_interrupt);
    if (status == ER_OK) {
        if (scriptLen) {
            status = ajsConsole.Install(scriptName, script, scriptLen);
            free(script);
        }
        while (!g_interrupt && (status == ER_OK)) {
            String input = ReadLine();
            if (input.size() > 0) {
                if (input == "quit") {
                    break;
                }
                if (input == "reboot") {
                    break;
                }
                if (input[input.size() - 1] != ';') {
                    input += ';';
                }
                status = ajsConsole.Eval(input);
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
    return -((int)status);

Usage:

    QCC_SyncPrintf("usage: %s [--verbose] [--name <device-name>] [javascript-file]\n", argv[0]);
    return -1;
}
