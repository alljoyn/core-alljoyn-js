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

#include "ajs_console.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#ifdef __MACH__
#include <sys/time.h>
#include <sys/errno.h>
#endif

#define METHODCALL_TIMEOUT 30000
#define NANO_TO_SEC_CONVERSION 1e9

using namespace std;
using namespace qcc;
using namespace ajn;

static const char* interfaces[] = { "org.allseen.scriptConsole", "org.allseen.scriptDebugger" };
/*
 * This is the wellknown port number for the console service. This value must match the console port number defined in AllJoyn.js
 */
#define SCRIPT_CONSOLE_PORT 7714

static const char notificationXML[] =
    " <interface name=\"org.alljoyn.Notification\"> "
    "   <signal name=\"notify\"> "
    "     <arg name=\"version\" type=\"q\"/> "
    "     <arg name=\"messageId\" type=\"i\"/> "
    "     <arg name=\"messageType\" type=\"q\"/> "
    "     <arg name=\"deviceId\" type=\"s\"/> "
    "     <arg name=\"deviceName\" type=\"s\"/> "
    "     <arg name=\"appId\" type=\"ay\"/> "
    "     <arg name=\"appName\" type=\"s\"/> "
    "     <arg name=\"attributes\" type=\"a{iv}\"/> "
    "     <arg name=\"customAttributes\" type=\"a{ss}\"/> "
    "     <arg name=\"notifications\" type=\"a(ss)\"/> "
    "   </signal> "
    "   <property name=\"Version\" type=\"q\" access=\"read\"/> "
    " </interface> ";

static const char consoleXML[] =
    " <node name=\"/ScriptConsole\"> "
    " <interface name=\"org.allseen.scriptConsole\"> "
    "   <property name=\"engine\" type=\"s\" access=\"read\"/> "
    "   <property name=\"maxEvalLen\" type=\"u\" access=\"read\"/> "
    "   <property name=\"maxScriptLen\" type=\"u\" access=\"read\"/> "
    "   <method name=\"eval\"> "
    "     <arg name=\"script\" type=\"ay\" direction=\"in\"/> "
    "     <arg name=\"status\" type=\"y\" direction=\"out\"/> "
    "     <arg name=\"output\" type=\"s\" direction=\"out\"/> "
    "   </method> "
    "   <method name=\"install\"> "
    "     <arg name=\"name\" type=\"s\" direction=\"in\"/> "
    "     <arg name=\"script\" type=\"ay\" direction=\"in\"/> "
    "     <arg name=\"status\" type=\"y\" direction=\"out\"/> "
    "     <arg name=\"output\" type=\"s\" direction=\"out\"/> "
    "   </method> "
    "   <method name=\"reset\"> "
    "   </method> "
    "   <method name=\"reboot\"> "
    "   </method> "
    "   <signal name=\"print\"> "
    "     <arg name=\"txt\" type=\"s\"/> "
    "   </signal> "
    "   <signal name=\"alert\"> "
    "     <arg name=\"txt\" type=\"s\"/> "
    "   </signal> "
    " </interface> "
    " <interface name=\"org.allseen.scriptDebugger\"> "
    "   <method name=\"begin\"> "
    "     <arg name=\"quiet\" type=\"y\" direction=\"in\"/> "
    "     <arg name=\"output\" type=\"y\" direction=\"out\"/> "
    "   </method> "
    "   <method name=\"end\"> "
    "     <arg name=\"output\" type=\"y\" direction=\"out\"/> "
    "   </method> "
    /* Request for notification update, reply will come as the 'notification' signal (above) */
    "   <signal name=\"notification\"> "
    "     <arg name=\"id\" type=\"y\"/> "
    "     <arg name=\"data\" type=\"yssyy\"/> "
    "   </signal> "
    /* Request for basic debug information (version, target info etc) */
    "   <method name=\"basicInfo\"> "
    "     <arg name=\"reply\" type=\"yssy\" direction=\"out\"/> "
    "   </method> "
    /* Request for trigger status */
    "   <method name=\"triggerStatus\"> "
    "     <arg name=\"reply\" type=\"y\"/> "
    "   </method> "
    /* Request to pause debugging */
    "   <method name=\"pause\"> "
    "     <arg name=\"reply\" type=\"y\" direction=\"out\"/> "
    "   </method> "
    /* Request to resume debugging */
    "   <method name=\"resume\"> "
    "     <arg name=\"reply\" type=\"y\" direction=\"out\"/> "
    "   </method> "
    /* Step Into request */
    "   <method name=\"stepInto\"> "
    "     <arg name=\"reply\" type=\"y\" direction=\"out\"/> "
    "   </method> "
    /* Step Over request */
    "   <method name=\"stepOver\"> "
    "     <arg name=\"reply\" type=\"y\" direction=\"out\"/> "
    "   </method> "
    /* Step Out request */
    "   <method name=\"stepOut\"> "
    "     <arg name=\"reply\" type=\"y\" direction=\"out\"/> "
    "   </method> "
    /* List breakpoints */
    "   <method name=\"listBreak\"> "
    "     <arg name=\"reply\" type=\"a(sy)\" direction=\"out\"/> "
    "   </method> "
    /* Add breakpoint request */
    "   <method name=\"addBreak\"> "
    "     <arg name=\"request\" type=\"sy\" direction=\"in\"/> "
    "     <arg name=\"reply\" type=\"y\" direction=\"out\"/> "
    "   </method> "
    /* Delete breakpoint request */
    "   <method name=\"delBreak\"> "
    "     <arg name=\"request\" type=\"y\" direction=\"in\"/> "
    "     <arg name=\"reply\" type=\"y\" direction=\"out\"/> "
    "   </method> "
    /* GetVar request */
    "   <method name=\"getVar\"> "
    "     <arg name=\"request\" type=\"s\" direction=\"in\"/> "
    "     <arg name=\"reply\" type=\"yyv\" direction=\"out\"/> "
    "   </method> "
    /* PutVar request */
    "   <method name=\"putVar\"> "
    "     <arg name=\"name\" type=\"s\" direction=\"in\"/> "
    "     <arg name=\"type\" type=\"y\" direction=\"in\"/> "
    "     <arg name=\"value\" type=\"ay\" direction=\"in\"/> "
    "     <arg name=\"reply\" type=\"y\" direction=\"out\"/> "
    "   </method> "
    /* Get call stack request */
    "   <method name=\"getCallStack\"> "
    "     <arg name=\"reply\" type=\"a(ssyy)\" direction=\"out\"/> "
    "   </method> "
    /* Get locals request */
    "   <method name=\"getLocals\"> "
    "     <arg name=\"reply\" type=\"a(ysv)\" direction=\"out\"/> "
    "   </method> "
    /* Eval request */
    "   <method name=\"eval\"> "
    "     <arg name=\"string\" type=\"s\" direction=\"in\" /> "
    "     <arg name=\"return\" type=\"yyv\" direction=\"out\" /> "
    "   </method> "
    /* Dump heap request */
    "   <method name=\"dumpHeap\"> "
    "     <arg name=\"reply\" type=\"av\" direction=\"out\"/> "
    "   </method> "
    /* Get the script installed on the target */
    "   <method name=\"getScript\"> "
    "     <arg name=\"script\" type=\"ay\" direction=\"out\"/> "
    "   </method> "
    "   <method name=\"detach\"> "
    "     <arg name=\"reply\" type=\"y\" direction=\"out\"/> "
    "   </method> "
    "   <signal name=\"version\"> "
    "     <arg name=\"versionString\" type=\"s\"/> "
    "   </signal> "
    " </interface> "
    " </node> ";

#ifdef _WIN32
class AJS_Console::Event {

  public:

    Event() : status(ER_OK)
    {
        cond = CreateEvent(NULL, TRUE, FALSE, NULL);
    }

    ~Event()
    {
        CloseHandle(cond);
    }

    QStatus Wait(uint32_t ms)
    {
        DWORD ret = WaitForSingleObject(cond, ms);
        if (ret == WAIT_TIMEOUT) {
            return ER_TIMEOUT;
        } else {
            return ret == WAIT_OBJECT_0 ? status : ER_OS_ERROR;
        }
    }

    void Set(QStatus status)
    {
        this->status = status;
        SetEvent(cond);
    }

  private:

    QStatus status;
    HANDLE cond;
};
#else
class AJS_Console::Event {

  public:

    Event() : status(ER_OK)
    {
        pthread_cond_init(&cond, NULL);
        pthread_mutex_init(&mutex, NULL);
    }

    ~Event()
    {
        pthread_cond_destroy(&cond);
        pthread_mutex_destroy(&mutex);
    }

    QStatus Wait(uint32_t ms)
    {
        struct timespec ts;
        int ret;
    #ifdef __MACH__
        struct timeval tv;
        gettimeofday(&tv, NULL);
        ts.tv_sec = tv.tv_sec + 0;
        ts.tv_nsec = 0;
    #else
        clock_gettime(CLOCK_REALTIME, &ts);
    #endif
        ts.tv_sec += ms / 1000;
        ts.tv_nsec += (ms % 1000) * 1000000;
        pthread_mutex_lock(&mutex);
        ret = pthread_cond_timedwait(&cond, &mutex, &ts);
        pthread_mutex_unlock(&mutex);
        if (ret) {
            return ret == ETIMEDOUT ? ER_TIMEOUT : ER_OS_ERROR;
        } else {
            return status;
        }
    }

    void Set(QStatus status)
    {
        this->status = status;
        pthread_mutex_lock(&mutex);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }

  private:

    QStatus status;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
};
#endif

#ifdef SHOW_DBG_MSG
static void printDebugMsg(void* buffer, uint32_t length)
{
    int i = 0;
    while (i < length) {
        QCC_SyncPrintf("0x%02x, ", *((uint8_t*)buffer + i));
        i++;
    }
    QCC_SyncPrintf(" Size = %u\n", length);
}
#endif

/*
 * Parse a variant knowing that it contains some kind of duktape dvalue/tvalue
 */
static void ParseDvalData(MsgArg* variant, uint8_t identifier, uint8_t** value, uint32_t* size, uint8_t* type)
{
    qcc::String vsig;
    vsig = variant->Signature();
    switch (identifier) {
    case DBG_TYPE_STRING4:
    case DBG_TYPE_STRING2:
        variant->Get("s", value);
        *size = strlen((char*)(*value)) + 1;
        *type = identifier;
        break;

    case DBG_TYPE_INTEGER4:

    case DBG_TYPE_UNUSED:
    case DBG_TYPE_UNDEFINED:
    case DBG_TYPE_NULL:
    case DBG_TYPE_TRUE:
    case DBG_TYPE_FALSE:
        *size = 1;
        (*value) = (uint8_t*)malloc(sizeof(uint8_t));
        variant->Get("y", (*value));
        *type = *(*value); /* Value is also the ID */
        break;

    case DBG_TYPE_NUMBER:
        /* Number type */
        (*value) = (uint8_t*)malloc(sizeof(uint8_t) * 8);
        *size = 8;
        variant->Get("t", (*value));
        *type = 0x1a;
        break;

    case DBG_TYPE_HEAPPTR:
    case DBG_TYPE_OBJECT:
    case DBG_TYPE_POINTER:
        {
            /* Object, pointer, or heap pointer type */
            uint8_t id;
            uint8_t tmp; /* Class for object type, not used for pointer/heap pointer */
            size_t sz;
            uint8_t* data;
            variant->Get("(yyay)", &id, &tmp, &sz, &data);
            *type = id;
            /* Object */
            if (id == DBG_TYPE_OBJECT) {
                /* Object contains the class number as well as a pointer so it needs an extra byte */
                (*value) = (uint8_t*)malloc(sizeof(uint8_t) * sz + 1);
                memcpy((*value), &tmp, 1);
                memcpy(((*value) + 1), data, sz);
                *size = sz + 1;
                /* Pointer or heap pointer */
            } else if ((id == DBG_TYPE_POINTER) || (id == DBG_TYPE_HEAPPTR)) {
                (*value) = (uint8_t*)malloc(sizeof(uint8_t) * sz);
                memcpy((*value), data, sz);
                *size = sz;
            } else {
                QCC_LogError(ER_FAIL, ("ParseDvalData(): Invalid identifier value: 0x%02x\n", id));
            }
        }
        break;

    case DBG_TYPE_BUFFER2:
    case DBG_TYPE_BUFFER4:
    case DBG_TYPE_LIGHTFUNC:
        {
            /* Light function or 2 byte buffer */
            uint16_t flags;
            size_t sz;
            uint8_t* data;
            variant->Get("(qay)", &flags, &sz, &data);
            *type = identifier;
            if ((identifier == DBG_TYPE_BUFFER2) || (identifier == DBG_TYPE_BUFFER4)) {
                (*value) = (uint8_t*)malloc(sizeof(uint8_t) * sz);
                *size = sz;
                memcpy(((*value)), data, *size);
            } else {
                (*value) = (uint8_t*)malloc(sizeof(uint8_t) * sz + 2);
                memcpy((*value), &flags, 2);
                memcpy(((*value) + 2), data, sz);
                *size = sz + 2;
            }
        }
        break;

    default:
        /* Ranged values, like strings, small ints and large ints must be handled with if/else */
        if ((identifier >= DBG_TYPE_STRLOW) && (identifier <= DBG_TYPE_STRHIGH)) {
            /* String type */
            char* str;
            variant->Get("s", &str);
            *size = strlen(str) + 1;
            (*value) = (uint8_t*)malloc(sizeof(char) * (*size));
            memcpy((*value), str, (*size) - 1);
            (*value)[(*size) - 1] = (uint8_t)'\0';
            *type = DBG_TYPE_STRLOW;
        } else if ((identifier >= DBG_TYPE_INTSMLOW) && (identifier <= DBG_TYPE_INTSMHIGH)) {
            (*value) = (uint8_t*)malloc(sizeof(uint8_t));
            variant->Get("y", (*value));
            *size = 1;
            *type = DBG_TYPE_INTLGLOW;
        } else if ((identifier >= DBG_TYPE_INTLGLOW) && (identifier <= DBG_TYPE_INTLGHIGH)) {
            uint16_t bytes;
            (*value) = (uint8_t*)malloc(sizeof(uint8_t) * 2);
            bytes = variant->Get("q", &bytes);
            (*value)[0] = ((((bytes & 0xff00) >> 8) - 0xc0) << 8);
            (*value)[1] = (bytes & 0x00ff);
            *size = 2;
            *type = DBG_TYPE_INTLGLOW;
        } else {
            QCC_LogError(ER_FAIL, ("GetVar(): Invalid identifier value: 0x%02x\n", identifier));
        }
        break;
    }
}

AJS_Console::AJS_Console() : BusListener(), debugState(DEBUG_DETACHED), activeDebug(false), quiet(false), sessionId(0), proxy(NULL), connectedBusName(NULL), aj(NULL), ev(new Event()), verbose(false), deviceName() {
}

AJS_Console::~AJS_Console() {
    delete proxy;
    delete aj;
    delete ev;
    free(connectedBusName);
}

void AJS_Console::BasicInfo(uint16_t* version, char** describe, char** targInfo, uint8_t* endianness)
{
    QStatus status;
    Message reply(*aj);
    if (debugState == DEBUG_ATTACHED_PAUSED) {
        status = proxy->MethodCall("org.allseen.scriptDebugger", "basicInfo", NULL, 0, reply);
        if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
            QCC_SyncPrintf("BasicInfo failed because debugger is in running state\n");
            return;
        } else if (status != ER_OK) {
            QCC_SyncPrintf("MethodCall(\"basicInfo\") failed, status = %u\n", status);
            return;
        }
        reply->GetArgs("yssy", version, describe, targInfo, endianness);
    }
}

void AJS_Console::Trigger(void)
{
    QStatus status;
    Message reply(*aj);
    uint8_t ret;
    if (debugState == DEBUG_ATTACHED_PAUSED) {
        status = proxy->MethodCall("org.allseen.scriptDebugger", "triggerStatus", NULL, 0, reply);
        if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
            QCC_SyncPrintf("triggerStatus failed because debugger is in running state\n");
            return;
        } else if (status != ER_OK) {
            QCC_SyncPrintf("MethodCall(\"triggerStatus\") failed, status = %u\n", status);
            return;
        }
        reply->GetArgs("y", &ret);
    }
}

void AJS_Console::Attach(void)
{
    QStatus status;
    Message reply(*aj);
    uint8_t ret;
    status = proxy->MethodCall("org.allseen.scriptDebugger", "attach", NULL, 0, reply);
    if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
        QCC_SyncPrintf("attach failed because debugger is in running state\n");
        return;
    } else if (status != ER_OK) {
        QCC_SyncPrintf("MethodCall(\"attach\") failed, status = %u\n", status);
        return;
    }
    debugState = DEBUG_ATTACHED_PAUSED;
    reply->GetArgs("y", &ret);
}

void AJS_Console::Detach(void)
{
    QStatus status;
    Message reply(*aj);
    uint8_t ret;
    status = proxy->MethodCall("org.allseen.scriptDebugger", "detach", NULL, 0, reply);
    if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
        QCC_SyncPrintf("detach failed because debugger is in running state\n");
        return;
    } else if (status != ER_OK) {
        QCC_SyncPrintf("MethodCall(\"detach\") failed, status = %u\n", status);
        return;
    }
    debugState = DEBUG_DETACHED;
    reply->GetArgs("y", &ret);
}

void AJS_Console::StepIn(void)
{
    QStatus status;
    Message reply(*aj);
    uint8_t ret;
    if (debugState == DEBUG_ATTACHED_PAUSED) {
        status = proxy->MethodCall("org.allseen.scriptDebugger", "stepInto", NULL, 0, reply);
        if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
            QCC_SyncPrintf("stepInfo failed because debugger is in running state\n");
            return;
        } else if (status != ER_OK) {
            QCC_SyncPrintf("MethodCall(\"stepInto\") failed, status = %u\n", status);
            return;
        }
        reply->GetArgs("y", &ret);
    }
}

void AJS_Console::StepOut(void)
{
    QStatus status;
    Message reply(*aj);
    uint8_t ret;
    if (debugState == DEBUG_ATTACHED_PAUSED) {
        status = proxy->MethodCall("org.allseen.scriptDebugger", "stepOut", NULL, 0, reply);
        if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
            QCC_SyncPrintf("stepOut failed because debugger is in running state\n");
            return;
        } else if (status != ER_OK) {
            QCC_SyncPrintf("MethodCall(\"stepOut\") failed, status = %u\n", status);
            return;
        }
        reply->GetArgs("y", &ret);
    }
}

void AJS_Console::StepOver(void)
{
    QStatus status;
    Message reply(*aj);
    uint8_t ret;
    if (debugState == DEBUG_ATTACHED_PAUSED) {
        status = proxy->MethodCall("org.allseen.scriptDebugger", "stepOver", NULL, 0, reply);
        if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
            QCC_SyncPrintf("stepOver failed because debugger is in running state\n");
            return;
        } else if (status != ER_OK) {
            QCC_SyncPrintf("MethodCall(\"stepOver\") failed, status = %u\n", status);
            return;
        }
        reply->GetArgs("y", &ret);
    }
}

void AJS_Console::Resume(void)
{
    QStatus status;
    Message reply(*aj);
    uint8_t ret;
    if (debugState == DEBUG_ATTACHED_PAUSED) {
        status = proxy->MethodCall("org.allseen.scriptDebugger", "resume", NULL, 0, reply);
        if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
            QCC_SyncPrintf("resume failed because debugger is in running state\n");
            return;
        } else if (status != ER_OK) {
            QCC_SyncPrintf("MethodCall(\"resume\") failed, status = %u\n", status);
            return;
        }
        reply->GetArgs("y", &ret);
        debugState = DEBUG_ATTACHED_RUNNING;
    }
}

void AJS_Console::Pause(void)
{
    QStatus status;
    Message reply(*aj);
    uint8_t ret;
    if ((debugState == DEBUG_ATTACHED_PAUSED) || (debugState == DEBUG_ATTACHED_RUNNING)) {
        status = proxy->MethodCall("org.allseen.scriptDebugger", "pause", NULL, 0, reply);
        if (status != ER_OK) {
            QCC_SyncPrintf("MethodCall(\"pause\") failed, status = %u\n", status);
            return;
        }
        reply->GetArgs("y", &ret);
        debugState = DEBUG_ATTACHED_PAUSED;
    }
}

void AJS_Console::AddBreak(char* file, uint8_t line)
{
    QStatus status;
    Message reply(*aj);
    MsgArg args[2];
    uint8_t ret;
    if ((debugState == DEBUG_ATTACHED_PAUSED) || (debugState == DEBUG_ATTACHED_RUNNING)) {
        args[0].Set("s", file);
        args[1].Set("y", line);
        status = proxy->MethodCall("org.allseen.scriptDebugger", "addBreak", args, 2, reply);
        if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
            QCC_SyncPrintf("addBreak failed because debugger is in running state\n");
            return;
        } else if (status == ER_OK) {
            reply->GetArgs("y", &ret);
        } else {
            QCC_LogError(status, ("MethodCall(\"addBreak\") failed\n"));
        }
    }
}

void AJS_Console::DelBreak(uint8_t index)
{
    QStatus status;
    Message reply(*aj);
    MsgArg args;
    uint8_t ret;
    if (debugState == DEBUG_ATTACHED_PAUSED) {
        args.Set("y", index);
        status = proxy->MethodCall("org.allseen.scriptDebugger", "delBreak", &args, 1, reply);
        if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
            QCC_SyncPrintf("delBreak failed because debugger is in running state\n");
            return;
        } else if (status == ER_OK) {
            reply->GetArgs("y", &ret);
        } else {
            QCC_LogError(status, ("MethodCall(\"delBreak\") failed\n"));
        }
    }
}

void AJS_Console::ListBreak(BreakPoint** breakpoints, uint8_t* count)
{
    QStatus status;
    Message reply(*aj);
    const MsgArg* entries;
    MsgArg* newEntries;
    size_t num;
    if (debugState == DEBUG_ATTACHED_PAUSED) {
        status = proxy->MethodCall("org.allseen.scriptDebugger", "listBreak", NULL, 0, reply);
        if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
            QCC_SyncPrintf("listBreak failed because debugger is in running state\n");
            return;
        } else if (status != ER_OK) {
            QCC_SyncPrintf("MethodCall(\"listBreak\") failed, status = %u\n", status);
            return;
        }
        entries = reply->GetArg(0);
        entries->Get("a(sy)", &num, &newEntries);
        *(breakpoints) = (BreakPoint*)malloc(sizeof(BreakPoint) * num);
        *count = num;
        for (size_t i = 0; i < num; ++i) {
            char* file;
            uint8_t line;
            uint8_t len;
            newEntries[i].Get("(sy)", &file, &line);
            if (strcmp(file, "") != 0) {
                len = strlen(file);
                (*breakpoints)[i].fname = (char*)malloc(sizeof(char) * len + 1);
                memcpy((void*)(*breakpoints)[i].fname, file, len);
                (*breakpoints)[i].fname[len] = '\0';
                (*breakpoints)[i].line = line;
            }
        }
    }
}

void AJS_Console::FreeBreakpoints(BreakPoint* breakpoints, uint8_t num)
{
    int i;
    if (breakpoints) {
        for (i = 0; i < num; i++) {
            if (breakpoints[i].fname) {
                free(breakpoints[i].fname);
            }
        }
        free(breakpoints);
    }
}

void AJS_Console::GetCallStack(CallStack** stack, uint8_t* size)
{
    QStatus status;
    Message reply(*aj);
    const MsgArg* entries;
    MsgArg* newEntries;
    size_t num;
    if (debugState == DEBUG_ATTACHED_PAUSED) {
        status = proxy->MethodCall("org.allseen.scriptDebugger", "getCallStack", NULL, 0, reply);
        if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
            QCC_SyncPrintf("getCallStack failed because debugger is in running state\n");
            return;
        } else if (status != ER_OK) {
            QCC_SyncPrintf("MethodCall(\"getCallStack\") failed, status = %u\n", status);
            return;
        }

        entries = reply->GetArg(0);
        entries->Get("a(ssyy)", &num, &newEntries);
        (*stack) = (CallStack*)malloc(sizeof(CallStack) * num);
        *size = num;
        for (size_t i = 0; i < num; ++i) {
            char* file, *func;
            uint8_t line, pc;
            uint8_t fileLen, funcLen;
            newEntries[i].Get("(ssyy)", &file, &func, &line, &pc);
            fileLen = strlen(file);
            funcLen = strlen(func);
            (*stack)[i].filename = (char*)malloc(sizeof(char) * fileLen + 1);
            (*stack)[i].function = (char*)malloc(sizeof(char) * funcLen + 1);
            memcpy((*stack)[i].filename, file, fileLen);
            (*stack)[i].filename[fileLen] = '\0';
            memcpy((*stack)[i].function, func, funcLen);
            (*stack)[i].function[funcLen] = '\0';
            (*stack)[i].line = line;
            (*stack)[i].pc = pc;
        }
    }
}

void AJS_Console::FreeCallStack(CallStack* stack, uint8_t size)
{
    int i;
    if (stack) {
        for (i = 0; i < size; i++) {
            if (stack[i].filename) {
                free(stack[i].filename);
            }
            if (stack[i].function) {
                free(stack[i].function);
            }
        }
        free(stack);
    }
}

void AJS_Console::GetLocals(Locals** list, uint16_t* size)
{
    QStatus status;
    Message reply(*aj);
    const MsgArg* entries;
    MsgArg* newEntries;
    size_t num;
    (*list) = NULL;
    if (debugState == DEBUG_ATTACHED_PAUSED) {
        status = proxy->MethodCall("org.allseen.scriptDebugger", "getLocals", NULL, 0, reply);
        if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
            QCC_SyncPrintf("getLocals failed because debugger is in running state\n");
            return;
        } else if (status != ER_OK) {
            QCC_SyncPrintf("MethodCall(\"getLocals\") failed, status = %u\n", status);
            return;
        }
        entries = reply->GetArg(0);
        entries->Get("a(ysv)", &num, &newEntries);
        *size = num;
        if (num) {
            (*list) = (Locals*)malloc(sizeof(Locals) * num);
        }
        for (size_t i = 0; i < num; ++i) {
            char* name;
            uint32_t sz;
            uint8_t type;
            uint8_t* val;
            qcc::String vsig;
            MsgArg* variant;
            uint8_t identifier;
            newEntries[i].Get("(ysv)", &identifier, &name, &variant);
            vsig = variant->Signature();
            (*list)[i].name = NULL;
            (*list)[i].data = NULL;
            if (identifier) {
                (*list)[i].name = (char*)malloc(sizeof(char) * strlen(name) + 1);
                memcpy((*list)[i].name, name, strlen(name));
                (*list)[i].name[strlen(name)] = '\0';
                /*
                 * Unmarshal the variants signature
                 */
                ParseDvalData(variant, identifier, &val, &sz, &type);
                (*list)[i].data = (uint8_t*)malloc(sizeof(uint8_t) * sz);
                memcpy((*list)[i].data, val, sz);
                (*list)[i].size = sz;
                (*list)[i].type = type;
            } else {
                (*list) = NULL;
            }
        }
    }
}

void AJS_Console::FreeLocals(Locals* list, uint16_t size)
{
    int i;
    if (list) {
        for (i = 0; i < size; i++) {
            if (list[i].name) {
                free(list[i].name);
            }
            if (list[i].data) {
                free(list[i].data);
            }
        }
        free(list);
    }
}

bool AJS_Console::GetVar(char* var, uint8_t** value, uint32_t* size, uint8_t* type)
{
    QStatus status;
    Message reply(*aj);
    MsgArg args, *variant;
    uint8_t valid, ret, identifier;
    qcc::String vsig;
    if (debugState == DEBUG_ATTACHED_PAUSED) {
        args.Set("s", var);
        status = proxy->MethodCall("org.allseen.scriptDebugger", "getVar", &args, 1, reply);
        if (status == ER_OK) {
            reply->GetArgs("y", &ret);
        } else {
            QCC_LogError(status, ("MethodCall(\"getVar\") failed\n"));
        }
        reply->GetArgs("yyv", &valid, &identifier, &variant);
        if (value && size && valid) {
            ParseDvalData(variant, identifier, value, size, type);
        } else {
            /* Value and size were NULL so just return the type */
            *type = identifier;
        }
        return true;
    }
    return false;
}

bool AJS_Console::PutVar(char* var, uint8_t* value, uint32_t size)
{
    QStatus status;
    Message reply(*aj);
    MsgArg args[3];
    uint8_t type;
    uint8_t ret;
    /*
     * Need to get the variable to establish its type
     */
    this->GetVar(var, NULL, NULL, &type);

    args[0].Set("s", var);
    if ((type >= 0x60) && (type <= 0x7f)) {
        /* Adjust type/size */
        type = (size + 0x60);
    } else if ((type == DBG_TYPE_TRUE) || (type == DBG_TYPE_FALSE)) {
        if (*value == DBG_TYPE_FALSE) {
            type = DBG_TYPE_FALSE;
        } else if (*value == DBG_TYPE_TRUE) {
            type = DBG_TYPE_TRUE;
        }
    }
    args[1].Set("y", type);
    args[2].Set("ay", size, value);
    status = proxy->MethodCall("org.allseen.scriptDebugger", "putVar", args, 3, reply);
    if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
        QCC_SyncPrintf("putVar failed because debugger is in running state\n");
        return false;
    } else if (status == ER_OK) {
        reply->GetArgs("y", &ret);
    } else {
        QCC_LogError(status, ("MethodCall(\"putVar\") failed\n"));
    }
    return true;
}

void AJS_Console::DebugEval(char* str, uint8_t** value, uint32_t* size, uint8_t* type)
{
    QStatus status;
    Message reply(*aj);
    MsgArg args, *variant;
    uint8_t valid, ret, identifier;
    qcc::String vsig;
    if (debugState == DEBUG_ATTACHED_PAUSED) {
        args.Set("s", str);
        status = proxy->MethodCall("org.allseen.scriptDebugger", "eval", &args, 1, reply);
        if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
            QCC_SyncPrintf("eval failed because debugger is in running state\n");
            return;
        } else if (status == ER_OK) {
            reply->GetArgs("y", &ret);
        } else {
            QCC_LogError(status, ("MethodCall(\"eval\") failed\n"));
            return;
        }
        reply->GetArgs("yyv", &valid, &identifier, &variant);
        if (valid == 0) {
            ParseDvalData(variant, identifier, value, size, type);
        }
    }
}

void AJS_Console::DumpHeap(void)
{
    QStatus status;
    Message reply(*aj);
    uint8_t ret;
    if (debugState == DEBUG_ATTACHED_PAUSED) {
        status = proxy->MethodCall("org.allseen.scriptDebugger", "stepOver", NULL, 0, reply);
        if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
            QCC_SyncPrintf("stepOver failed because debugger is in running state\n");
            return;
        } else if (status != ER_OK) {
            QCC_SyncPrintf("MethodCall(\"stepOver\") failed, status = %u\n", status);
            return;
        }
        reply->GetArgs("y", &ret);
    }
}

bool AJS_Console::GetScript(uint8_t** script, uint32_t* length)
{
    QStatus status;
    Message reply(*aj);
    const MsgArg* entries;
    uint8_t* s;
    status = proxy->MethodCall("org.allseen.scriptDebugger", "getScript", NULL, 0, reply);
    if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
        return false;
    } else if (status != ER_OK) {
        QCC_SyncPrintf("MethodCall(\"getScript\") failed, status = %u\n", status);
        return false;
    }
    entries = reply->GetArg(0);
    entries->Get("ay", length, &s);
    (*script) = (uint8_t*)malloc(sizeof(char) * (*length) + 1);
    memcpy((*script), (s + 4), (*length) + 4);
    (*script)[(*length) + 4] = (uint8_t)'\0';
    return true;
}

void AJS_Console::StopDebugger()
{
    QStatus status;
    Message reply(*aj);
    int ret;
    QCC_SyncPrintf("Stopping Debugger\n");
    status = proxy->MethodCall("org.allseen.scriptDebugger", "end", NULL, 0, reply);
    if (status != ER_OK) {
        QCC_SyncPrintf("MethodCall(\"begin\") failed\n");
        return;
    }
    debugState = DEBUG_DETACHED;
    /* Get the reply data */
    reply->GetArgs("y", &ret);
}

void AJS_Console::StartDebugger()
{
    QStatus status;
    MsgArg args;
    Message reply(*aj);
    uint8_t start = 8;
    int ret;
    args.Set("y", start);
    if (quiet) {
        args.Set("y", 1);
    } else {
        args.Set("y", 0);
    }
    QCC_SyncPrintf("Starting Debugger\n");
    status = proxy->MethodCall("org.allseen.scriptDebugger", "begin", &args, 1, reply);
    if (status != ER_OK) {
        QCC_SyncPrintf("MethodCall(\"begin\") failed\n");
        return;
    }
    debugState = DEBUG_ATTACHED_PAUSED;
    /* Get the reply data */
    reply->GetArgs("y", &ret);
}

void AJS_Console::Announced(const char* busName, uint16_t version, SessionPort port, const MsgArg& objectDescriptionArg, const MsgArg& aboutDataArg)
{
    AboutData aboutData(aboutDataArg);
    SessionOpts opts;
    QStatus status;
    char* context;

    if (!sessionId) {
        /*
         * If a device name was given it must match
         */
        if (deviceName != "") {
            char* name;
            status = aboutData.GetDeviceName(&name);
            if (status != ER_OK) {
                QCC_LogError(status, ("AboutData::GetDeviceName failed"));
                return;
            }
            if (deviceName != name) {
                Print("Found device \"%s\" this is not the device you are looking for\n", name);
                return;
            }
        }
        /*
         * Prevent concurrent JoinSession calls
         */
        sessionId = -1;

        Print("Found script console service: %s\n", busName);
        /*
         * Join the session
         */
        context = strdup(busName);
        status = aj->JoinSessionAsync(busName, SCRIPT_CONSOLE_PORT, this, opts, this, context);

        if (status != ER_OK) {
            sessionId = 0;
            free(context);
            QCC_LogError(status, ("JoinSession failed"));
            ev->Set(status);
        }
    }
}

void AJS_Console::JoinSessionCB(QStatus status, SessionId sid, const SessionOpts& opts, void* context)
{
    char* busName = (char*)context;

    if (status != ER_OK) {
        sessionId = 0;
        QCC_LogError(status, ("JoinSession failed"));
        if (status == ER_ALLJOYN_JOINSESSION_REPLY_REJECTED) {
            /*
             * TODO - maybe a blacklist this responder so we don't keep getting rejected
             */
            ev->Set(status);
        }
        free(busName);
    } else {
        sessionId = sid;
        Print("Joined session: %u\n", sessionId);
        free(connectedBusName);
        connectedBusName = busName;
        ev->Set(status);
    }

}

QStatus AJS_Console::Connect(const char* deviceName, volatile sig_atomic_t* interrupt)
{
    QStatus status;

    /*
     * The device we wil be looking for
     */
    this->deviceName = deviceName;

    aj = new BusAttachment("console", true);
    aj->Start();
    aj->RegisterBusListener(*this);
    aj->CreateInterfacesFromXml(notificationXML);

    status = aj->Connect();
    if (status == ER_OK) {
        /*
         * Register for announcements from script engines
         */
        aj->RegisterAboutListener(*this);
        aj->WhoImplements(interfaces, 2);
    }

    do {
        status = ev->Wait(1000);
        if (interrupt != NULL && *interrupt) {
            status = ER_OS_ERROR;
        }
    } while (status == ER_TIMEOUT);

    aj->UnregisterAboutListener(*this);

    if (status == ER_OK) {
        qcc::String matchRule = "type='signal',sessionless='t',interface='org.alljoyn.Notification',member='notify'";
        /*
         * Add a match rule to receive notifications from the script service
         */
        matchRule += "sender='" + qcc::String(connectedBusName) + "'";
        aj->AddMatch(matchRule.c_str());

        /*
         * Create the proxy object from the XML
         */
        proxy = new ProxyBusObject(*aj, connectedBusName, "/ScriptConsole", sessionId, false);
        status = proxy->ParseXml(consoleXML);
        assert(status == ER_OK);
        RegisterHandlers(aj);
    } else {
        delete aj;
        aj = NULL;
    }
    return status;
}

void AJS_Console::RegisterHandlers(BusAttachment* ajb)
{
    const InterfaceDescription* ifc;
    const InterfaceDescription* dbg_ifc;

    ifc = ajb->GetInterface("org.allseen.scriptConsole");
    dbg_ifc = ajb->GetInterface("org.allseen.scriptDebugger");

    assert(ifc != NULL);
    assert(dbg_ifc != NULL);

    ajb->RegisterSignalHandler(this,
                               static_cast<MessageReceiver::SignalHandler>(&AJS_Console::PrintMsg),
                               ifc->GetMember("print"),
                               "/ScriptConsole");
    ajb->RegisterSignalHandler(this,
                               static_cast<MessageReceiver::SignalHandler>(&AJS_Console::AlertMsg),
                               ifc->GetMember("alert"),
                               "/ScriptConsole");

    if (this->activeDebug) {
        ajb->RegisterSignalHandler(this,
                                   static_cast<MessageReceiver::SignalHandler>(&AJS_Console::DebugNotification),
                                   dbg_ifc->GetMember("notification"),
                                   "/ScriptConsole");
        ajb->RegisterSignalHandler(this,
                                   static_cast<MessageReceiver::SignalHandler>(&AJS_Console::DebugVersion),
                                   dbg_ifc->GetMember("version"),
                                   "/ScriptConsole");
    }
    ifc = ajb->GetInterface("org.alljoyn.Notification");
    ajb->RegisterSignalHandler(this,
                               static_cast<MessageReceiver::SignalHandler>(&AJS_Console::Notification),
                               ifc->GetMember("notify"),
                               NULL);
}

QStatus AJS_Console::Eval(const String script)
{
    QStatus status;
    Message reply(*aj);
    MsgArg arg("ay", script.size() + 1, script.c_str());

    Print("Eval: %s\n", script.c_str());
    status = proxy->MethodCall("org.allseen.scriptConsole", "eval", &arg, 1, reply);
    if (status == ER_OK) {
        uint8_t result;
        const char* output;

        reply->GetArgs("ys", &result, &output);
        Print("Eval result=%d: %s\n", result, output);
    } else {
        QCC_LogError(status, ("MethodCall(\"eval\") failed\n"));
    }
    return status;
}

QStatus AJS_Console::Reboot()
{
    Message reply(*aj);
    QStatus status = proxy->MethodCall("org.allseen.scriptConsole", "reboot", NULL, 0, reply);
    if (status != ER_OK) {
        QCC_LogError(status, ("MethodCall(\"reboot\") failed\n"));
    }
    return status;
}

QStatus AJS_Console::Install(qcc::String name, const uint8_t* script, size_t scriptLen)
{
    QStatus status;
    Message reply(*aj);
    MsgArg args[2];

    /*
     * Strip file path from the name
     */
    size_t pos = name.find_last_of("/\\");
    if (pos != qcc::String::npos) {
        name = name.substr(pos + 1);
    }
    Print("Installing script %s\n", name.c_str());

    args[0].Set("s", name.c_str());
    args[1].Set("ay", scriptLen, script);

    status = proxy->MethodCall("org.allseen.scriptConsole", "reset", NULL, 0, reply);
    if (status != ER_OK) {
        QCC_LogError(status, ("MethodCall(\"reset\") failed\n"));
        return status;
    }

    Print("Installing script of length %d\n", scriptLen);

    status = proxy->MethodCall("org.allseen.scriptConsole", "install", args, 2, reply);
    if (status == ER_OK) {
        uint8_t result;
        const char* output;

        reply->GetArgs("ys", &result, &output);
        Print("Eval result=%d: %s\n", result, output);
    } else {
        QCC_LogError(status, ("MethodCall(\"install\") failed\n"));
    }
    return status;
}

void AJS_Console::SessionLost(SessionId sessionId, SessionLostReason reason)
{
    Print("SessionLost(%08x) was called. Reason=%u.\n", sessionId, reason);
    _exit(1);
}

void AJS_Console::Notification(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg)
{
    const char* TYPES[] = { "Emergency", "Warning", "Informational", "Unknown" };
    uint16_t version;
    uint32_t notifId;
    uint16_t notifType;
    const char* deviceId;
    const char* deviceName;
    const uint8_t* appId;
    size_t appIdLen;
    const char* appName;
    const MsgArg* attrs;
    const MsgArg* cust;
    const MsgArg* strings;

    if (GetVerbose()) {
        Print("NOTIFICATION:\n%s\n", msg->ToString().c_str());
    }

    QStatus status = msg->GetArgs("qiqssays***", &version, &notifId, &notifType, &deviceId, &deviceName, &appIdLen, &appId, &appName, &attrs, &cust, &strings);
    if (status != ER_OK) {
        QCC_LogError(status, ("Notification GetArgs failed\n"));
        return;
    }
    if (notifType > 2) {
        notifType = 3;
    }
    Print("%s Notification from app:%s on device:%s\n", TYPES[notifType], appName, deviceName);
    /*
     * Unpack the notification strings
     */
    MsgArg*entries;
    size_t num;
    strings->Get("a(ss)", &num, &entries);
    for (size_t i = 0; i < num; ++i) {
        char*lang;
        char*txt;
        entries[i].Get("(ss)", &lang, &txt);
        Print("%s: %s\n", lang, txt);
    }
}

void AJS_Console::PrintMsg(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg)
{
    if (!quiet) {
        Print("%s\n", msg->GetArg()->v_string.str);
    }
}

void AJS_Console::AlertMsg(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg)
{
    if (!quiet) {
        Print("%s\n", msg->GetArg()->v_string.str);
    }
}

void AJS_Console::DebugVersion(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg)
{
    QCC_SyncPrintf("Version Information:\n");
    QCC_SyncPrintf("%s\n", msg->GetArg()->v_string.str);
}

void AJS_Console::DebugNotification(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg)
{
    uint8_t id = msg->GetArg()->v_byte;
    switch (id) {
    case STATUS_NOTIFICATION:
        {
            uint8_t state;
            const char* fileName;
            const char* funcName;
            uint8_t lineNumber, pc;
            msg->GetArgs("yyssyy", &id, &state, &fileName, &funcName, &lineNumber, &pc);
            QCC_SyncPrintf("Got status notification\nState=%u, File Name=%s, Function=%s, Line=%u, PC=%u\n", state, fileName, funcName, lineNumber, pc);
            if (state == 1) {
                debugState = DEBUG_ATTACHED_PAUSED;
            }
        }
        break;

    case PRINT_NOTIFICATION:
        QCC_SyncPrintf("PRINT NOTIFICATION: %s\n", msg->GetArg()->v_string.str);
        break;

    case ALERT_NOTIFICATION:
        QCC_SyncPrintf("ALERT NOTIFICATION: %s\n", msg->GetArg()->v_string.str);
        break;

    case LOG_NOTIFICATION:
        QCC_SyncPrintf("LOG NOTIFICATION: %u %s\n", msg->GetArg()->v_byte, msg->GetArg()->v_string.str);
        break;
    }
}
