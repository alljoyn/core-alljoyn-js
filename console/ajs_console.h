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
#ifndef _AJS_CONSOLE_H

#include <signal.h>
#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/String.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/version.h>
#include <alljoyn/AboutListener.h>

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN"

class AJS_Console : public ajn::BusListener, public ajn::SessionListener, public ajn::AboutListener, public ajn::MessageReceiver, public ajn::BusAttachment::JoinSessionAsyncCB {
  public:

    AJS_Console();

    virtual ~AJS_Console();

    QStatus Connect(const char* deviceName, volatile sig_atomic_t* interrupt);

    QStatus Eval(const qcc::String script);

    QStatus Reboot();

    QStatus Install(qcc::String name, const uint8_t* script, size_t len);

    void SessionLost(ajn::SessionId sessionId, SessionLostReason reason);

    virtual void Notification(const ajn::InterfaceDescription::Member* member, const char* sourcePath, ajn::Message& msg);

    virtual void Print(const char* fmt, ...) = 0;

    virtual void PrintMsg(const ajn::InterfaceDescription::Member* member, const char* sourcePath, ajn::Message& msg);

    virtual void AlertMsg(const ajn::InterfaceDescription::Member* member, const char* sourcePath, ajn::Message& msg);

    void Announced(const char* busName, uint16_t version, ajn::SessionPort port, const ajn::MsgArg& objectDescriptionArg, const ajn::MsgArg& aboutDataArg);

    void SetVerbose(bool newValue) {
        verbose = newValue;
    }

    bool GetVerbose() {
        return verbose;
    }

    class Event;

  private:

    virtual void RegisterHandlers(ajn::BusAttachment* ajb);
    virtual void JoinSessionCB(QStatus status, ajn::SessionId sessionId, const ajn::SessionOpts& opts, void* context);

    ajn::SessionId sessionId;
    ajn::ProxyBusObject* proxy;
    char* connectedBusName;
    ajn::BusAttachment* aj;
    Event* ev;
    bool verbose;
    qcc::String deviceName;
};

#endif
