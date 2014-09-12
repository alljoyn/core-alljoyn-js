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

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/String.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/version.h>
#include <alljoyn/about/AnnounceHandler.h>
#include <alljoyn/about/AboutClient.h>
#include <alljoyn/about/AnnouncementRegistrar.h>

#include <alljoyn/Status.h>

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;
using namespace ajn;

class Event;

class AJS_Console : public BusListener, public SessionListener, public ajn::services::AnnounceHandler, public BusAttachment::JoinSessionAsyncCB {
public:

     AJS_Console();

     virtual ~AJS_Console();

     QStatus Connect(const char* deviceName, volatile sig_atomic_t* interrupt);

     QStatus Eval(const String script);

     QStatus Reboot();

     QStatus Install(qcc::String name, const uint8_t* script, size_t len);

     void SessionLost(SessionId sessionId, SessionLostReason reason);

     virtual void Notification(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg);

     virtual void Print(const char* fmt, ...) = 0;

     virtual void PrintMsg(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg);

     virtual void AlertMsg(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg);

     void Announce(uint16_t version, uint16_t port, const char* busName, const ObjectDescriptions& objectDescs, const AboutData& aboutData);

     void SetVerbose(bool newValue) {
         verbose = newValue;
     }

     bool GetVerbose() {
         return verbose;
     }

private:

     virtual void RegisterHandlers(BusAttachment* ajb);
     virtual void JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& opts, void* context);

     uint32_t sessionId;
     ProxyBusObject* proxy;
     char* connectedBusName;
     BusAttachment* aj;
     Event* ev;
     bool verbose;
     String deviceName;
};

#endif
