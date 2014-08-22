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

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/String.h>

#include <errno.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/version.h>
#include <alljoyn/about/AnnounceHandler.h>
#include <alljoyn/about/AboutClient.h>
#include <alljoyn/about/AnnouncementRegistrar.h>

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN"

#define METHODCALL_TIMEOUT 30000

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

static bool verbose = false;
static volatile sig_atomic_t g_interrupt = false;

static void SigIntHandler(int sig)
{
    g_interrupt = true;
}

#ifdef _WIN32
class Event {

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
class Event {

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

        clock_gettime(CLOCK_REALTIME, &ts);
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

class AJS_Console : public BusListener, public SessionListener, public ajn::services::AnnounceHandler {
  public:

    AJS_Console() : BusListener(), sessionId(0), proxy(NULL), aj(NULL) { }

    ~AJS_Console() {
        delete proxy;
        delete aj;
    }

    QStatus Connect(const char* deviceName);

    QStatus Eval(const String script);

    QStatus Reboot();

    QStatus Install(qcc::String name, const uint8_t* script, size_t len);

    void SessionLost(SessionId sessionId, SessionLostReason reason) {
        QCC_SyncPrintf("SessionLost(%08x) was called. Reason=%u.\n", sessionId, reason);
        _exit(1);
    }

    void Notification(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg);

    void Print(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg);

    void Alert(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg);

    void Announce(uint16_t version, uint16_t port, const char* busName, const ObjectDescriptions& objectDescs, const AboutData& aboutData);

  private:

    uint32_t sessionId;
    ProxyBusObject* proxy;
    BusAttachment* aj;
    Event ev;
    String deviceName;

};

void AJS_Console::Announce(uint16_t version, uint16_t port, const char* busName, const ObjectDescriptions& objectDescs, const AboutData& aboutData)
{
    SessionOpts opts;
    QStatus status;
    qcc::String matchRule = "type='signal',sessionless='t',interface='org.alljoyn.Notification',member='notify'";

    if (!sessionId) {
        /*
         * If a device name was given it must match
         */
        if (deviceName != "") {
            AboutData::const_iterator iter = aboutData.find("DeviceName");
            /*
             *
             */
            if (iter == std::end(aboutData)) {
                QCC_LogError(ER_NO_SUCH_DEVICE, ("Missing device name in about data"));
                return;
            }
            const MsgArg &arg = iter->second;
            const char* name;
            if ((arg.Get("s", &name) != ER_OK) || (deviceName != name)) {
                QCC_SyncPrintf("Found device \"%s\" this is not the device you are looking for\n", name);
                return;
            }
        }
        /*
         * Prevent concurrent JoinSession calls
         */
        sessionId = -1;
        /*
         * Enable concurrent callbacks because we make blocking calls below
         */
        aj->EnableConcurrentCallbacks();

        QCC_SyncPrintf("Found script console service: %s\n", busName);
        /*
         * Join the session
         */
        status = aj->JoinSession(busName, SCRIPT_CONSOLE_PORT, this, sessionId, opts);
        if (status != ER_OK) {
            sessionId = 0;
            QCC_LogError(status, ("JoinSession(%s) failed", busName));
            if (status == ER_ALLJOYN_JOINSESSION_REPLY_REJECTED) {
                /*
                 * TODO - maybe a blacklist this responder so we don't keep getting rejected
                 */
                ev.Set(status);
            }
            return;
        }
        QCC_SyncPrintf("Joined session: %u\n", sessionId);
        /*
         * Add a match rule to receive notifications from the script service
         */
        matchRule += "sender='" + qcc::String(busName) + "'";
        aj->AddMatch(matchRule.c_str());
        /*
         * Create the proxy object from the XML
         */
        proxy = new ProxyBusObject(*aj, busName, "/ScriptConsole", sessionId, false);
        status = proxy->ParseXml(consoleXML);
        assert(status == ER_OK);
        ev.Set(ER_OK);
    }
}

QStatus AJS_Console::Connect(const char* deviceName)
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
        ajn::services::AnnouncementRegistrar::RegisterAnnounceHandler(*aj, *this, interfaces, 1);
    }
    do {
        status = ev.Wait(1000);
        if (g_interrupt) {
            status = ER_OS_ERROR;
        }
    } while (status == ER_TIMEOUT);

    ajn::services::AnnouncementRegistrar::UnRegisterAllAnnounceHandlers(*aj);

    if (status == ER_OK) {
        const InterfaceDescription* ifc;

        ifc = aj->GetInterface("org.allseen.scriptConsole");
        aj->RegisterSignalHandler(this,
                                  static_cast<MessageReceiver::SignalHandler>(&AJS_Console::Print),
                                  ifc->GetMember("print"),
                                  "/ScriptConsole");
        aj->RegisterSignalHandler(this,
                                  static_cast<MessageReceiver::SignalHandler>(&AJS_Console::Alert),
                                  ifc->GetMember("alert"),
                                  "/ScriptConsole");

        ifc = aj->GetInterface("org.alljoyn.Notification");
        aj->RegisterSignalHandler(this,
                                  static_cast<MessageReceiver::SignalHandler>(&AJS_Console::Notification),
                                  ifc->GetMember("notify"),
                                  NULL);


    } else {
        delete aj;
        aj = NULL;
    }
    return status;
}

QStatus AJS_Console::Eval(const String script)
{
    QStatus status;
    Message reply(*aj);
    MsgArg arg("ay", script.size() + 1, script.c_str());

    QCC_SyncPrintf("Eval: %s\n", script.c_str());
    status = proxy->MethodCall("org.allseen.scriptConsole", "eval", &arg, 1, reply);
    if (status == ER_OK) {
        uint8_t result;
        const char* output;

        reply->GetArgs("ys", &result, &output);
        QCC_SyncPrintf("Eval result=%d: %s\n", result, output);
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
    QCC_SyncPrintf("Installing script %s\n", name.c_str());

    args[0].Set("s", name.c_str());
    args[1].Set("ay", scriptLen, script);

    status = proxy->MethodCall("org.allseen.scriptConsole", "reset", NULL, 0, reply);
    if (status != ER_OK) {
        QCC_LogError(status, ("MethodCall(\"reset\") failed\n"));
        return status;
    }

    QCC_SyncPrintf("Installing script of length %d\n", scriptLen);

    status = proxy->MethodCall("org.allseen.scriptConsole", "install", args, 2, reply);
    if (status == ER_OK) {
        uint8_t result;
        const char* output;

        reply->GetArgs("ys", &result, &output);
        QCC_SyncPrintf("Eval result=%d: %s\n", result, output);
    } else {
        QCC_LogError(status, ("MethodCall(\"install\") failed\n"));
    }
    return status;
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

    if (verbose) {
        QCC_SyncPrintf("NOTIFICATION:\n%s\n", msg->ToString().c_str());
    }

    QStatus status = msg->GetArgs("qiqssays***", &version, &notifId, &notifType, &deviceId, &deviceName, &appIdLen, &appId, &appName, &attrs, &cust, &strings);
    if (status != ER_OK) {
        QCC_LogError(status, ("Notification GetArgs failed\n"));
        return;
    }
    if (notifType > 2) {
        notifType = 3;
    }
    QCC_SyncPrintf("%s Notification from app:%s on device:%s\n", TYPES[notifType], appName, deviceName);
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
        QCC_SyncPrintf("%s: %s\n", lang, txt);
    }
}

void AJS_Console::Print(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg)
{
    QCC_SyncPrintf("%s\n", msg->GetArg()->v_string.str);
}

void AJS_Console::Alert(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg)
{
    QCC_SyncPrintf("%s\n", msg->GetArg()->v_string.str);
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
    AJS_Console ajsConsole;
    const char* scriptName;
    const char* deviceName;
    uint8_t* script;
    size_t scriptLen = 0;

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    for (int i = 1; i < argc; ++i) {
        if (*argv[i] == '-') {
            if (strcmp(argv[i], "--verbose") == 0) {
                verbose = true;
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

    status = ajsConsole.Connect(deviceName);
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
