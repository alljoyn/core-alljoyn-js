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

static const char* interfaces[] = { "org.allseen.scriptConsole" };
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

AJS_Console::AJS_Console() : BusListener(), sessionId(0), proxy(NULL), connectedBusName(NULL), aj(NULL), ev(new Event()), verbose(false), deviceName() { }

AJS_Console::~AJS_Console() {
     delete proxy;
     delete aj;
     delete ev;
     free(connectedBusName);
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
        aj->WhoImplements(interfaces, 1);
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

    ifc = ajb->GetInterface("org.allseen.scriptConsole");
    ajb->RegisterSignalHandler(this,
                               static_cast<MessageReceiver::SignalHandler>(&AJS_Console::PrintMsg),
                               ifc->GetMember("print"),
                               "/ScriptConsole");
    ajb->RegisterSignalHandler(this,
                               static_cast<MessageReceiver::SignalHandler>(&AJS_Console::AlertMsg),
                               ifc->GetMember("alert"),
                               "/ScriptConsole");

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
    Print("%s\n", msg->GetArg()->v_string.str);
}

void AJS_Console::AlertMsg(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg)
{
    Print("%s\n", msg->GetArg()->v_string.str);
}
