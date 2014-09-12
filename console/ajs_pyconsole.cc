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

#include <Python.h>
#include "ajs_console.h"
#include "alljoyn/Status.h"

class AJS_PyConsole : public AJS_Console
{
public:
    virtual void Notification(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg);

    virtual void Print(const char* fmt, ...);

    virtual void PrintMsg(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg);

    virtual void AlertMsg(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg);

    void Msg(const char* msgType, const char* msgText);

    PyObject* pycallback = NULL; /* Only modify when Python GIL is held */

private:
    virtual void RegisterHandlers(BusAttachment* ajb);

};

static AJS_PyConsole* console = NULL;

void AJS_PyConsole::Print(const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);

    (void) vprintf(fmt, ap);

    va_end(ap);
}

void AJS_PyConsole::RegisterHandlers(BusAttachment* ajb)
{
    const InterfaceDescription* ifc;

    ifc = ajb->GetInterface("org.allseen.scriptConsole");
    ajb->RegisterSignalHandler(this,
                              static_cast<MessageReceiver::SignalHandler>(&AJS_PyConsole::PrintMsg),
                              ifc->GetMember("print"),
                              "/ScriptConsole");
    ajb->RegisterSignalHandler(this,
                              static_cast<MessageReceiver::SignalHandler>(&AJS_PyConsole::AlertMsg),
                              ifc->GetMember("alert"),
                              "/ScriptConsole");

    ifc = ajb->GetInterface("org.alljoyn.Notification");
    ajb->RegisterSignalHandler(this,
                              static_cast<MessageReceiver::SignalHandler>(&AJS_PyConsole::Notification),
                              ifc->GetMember("notify"),
                              NULL);
}

void AJS_PyConsole::Notification(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg)
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

    if (pycallback == NULL) {
        Print("No callback, skipping notification\n");
        return;
    }

    QStatus status = msg->GetArgs("qiqssays***", &version, &notifId, &notifType, &deviceId, &deviceName, &appIdLen, &appId, &appName, &attrs, &cust, &strings);
    if (status != ER_OK) {
        QCC_LogError(status, ("Notification GetArgs failed\n"));
        return;
    }
    if (notifType > 2) {
        notifType = 3;
    }

    //Print("%s Notification from app:%s on device:%s\n", TYPES[notifType], appName, deviceName);

    /* Start Python work... Get the GIL */
    PyGILState_STATE gilstate = PyGILState_Ensure();

    /*
     * Unpack the notification strings
     */
    PyObject* tup;
    PyObject* list = PyList_New(0);
    if (list == NULL) {
        return;
    }

    MsgArg* entries;
    size_t num;
    strings->Get("a(ss)", &num, &entries);
    for (size_t i = 0; i < num; ++i) {
        char* lang;
        char* txt;
        entries[i].Get("(ss)", &lang, &txt);
        tup = Py_BuildValue("ss", lang, txt);
        if (tup != NULL) {
            PyList_Append(list, tup);
            Py_DECREF(tup);
        }
    }

    tup = PyList_AsTuple(list);
    Py_DECREF(list);
    PyObject* cbdata = Py_BuildValue("s(sssO)", "Notification", TYPES[notifType], appName, deviceName, tup);
    Py_DECREF(tup);

    if (cbdata != NULL) {
        PyObject* result = PyObject_CallObject(console->pycallback, cbdata);
        Py_DECREF(cbdata);

        if (result == NULL) {
            Print("Callback error\n");
        } else {
            Py_DECREF(result);
        }
    }

    /* Release interpreter lock */
    PyGILState_Release(gilstate);
}

void AJS_PyConsole::Msg(const char* msgType, const char* msgText)
{
    /* Start Python work... Get the GIL */
    PyGILState_STATE gilstate = PyGILState_Ensure();

    PyObject* cbdata = Py_BuildValue("ss", msgType, msgText);

    if (cbdata != NULL) {
        PyObject* result = PyObject_CallObject(console->pycallback, cbdata);
        Py_DECREF(cbdata);

        if (result == NULL) {
            Print("Callback error\n");
        } else {
            Py_DECREF(result);
        }
    }

    /* Release interpreter lock */
    PyGILState_Release(gilstate);
}

void AJS_PyConsole::PrintMsg(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg)
{
    Msg("Print", msg->GetArg()->v_string.str);
}

void AJS_PyConsole::AlertMsg(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg)
{
    Msg("Alert", msg->GetArg()->v_string.str);
}

static PyObject* statusobject(QStatus status)
{
    return Py_BuildValue("s", QCC_StatusText(status));
}

static PyObject* py_connect(PyObject* self, PyObject* args)
{
    QStatus status;
    // TODO: Make nonblocking. Start connection on another thread, use g_interrupt to stop it.
    Py_BEGIN_ALLOW_THREADS
    status = console->Connect(NULL, NULL);
    Py_END_ALLOW_THREADS

    return statusobject(status);
}

static PyObject* py_eval(PyObject* self, PyObject* args)
{
    QStatus status;
    const char* text;

    if (!PyArg_ParseTuple(args, "s", &text)) {
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    status = console->Eval(String(text));
    Py_END_ALLOW_THREADS

    return statusobject(status);
}

static PyObject* py_install(PyObject* self, PyObject* args)
{
    QStatus status;
    const char* name;
    const uint8_t* script;
    int scriptlen;

    if (!PyArg_ParseTuple(args, "ss#", &name, &script, &scriptlen)) {
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    status = console->Install(String(name), script, scriptlen);
    Py_END_ALLOW_THREADS

    return statusobject(status);
}

static PyObject* py_reboot(PyObject* self, PyObject* args)
{
    QStatus status;

    Py_BEGIN_ALLOW_THREADS
    status = console->Reboot();
    Py_END_ALLOW_THREADS

    return statusobject(status);
}

static PyObject* py_setcallback(PyObject* self, PyObject* args)
{
    PyObject* tempCB;

    if (!PyArg_ParseTuple(args, "O", &tempCB)) {
        return NULL;
    }

    if (!PyCallable_Check(tempCB)) {
        PyErr_SetString(PyExc_TypeError, "Not a callable");
        return NULL;
    }

    Py_XINCREF(tempCB);
    Py_XDECREF(console->pycallback);
    console->pycallback = tempCB;

    return Py_BuildValue("");
}

static PyMethodDef AJSConsoleMethods[] = {
    {"Connect", py_connect, METH_VARARGS, "Make a connection"},
    {"Eval", py_eval, METH_VARARGS, "Evaluate a statement"},
    {"Install", py_install, METH_VARARGS, "Install a script"},
    {"Reboot", py_reboot, METH_VARARGS, "Reboot target"},
    {"SetCallback", py_setcallback, METH_VARARGS, "Set callback function"},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initAJSConsole(void)
{
    (void) Py_InitModule("AJSConsole", AJSConsoleMethods);

    PyEval_InitThreads();
    console = new AJS_PyConsole();
}
