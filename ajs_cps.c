/******************************************************************************
 * Copyright (c) 2016 Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright 2016 Open Connectivity Foundation and Contributors to
 *    AllSeen Alliance. All rights reserved.
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

/**
 * Per-module definition of the current module for debug logging.  Must be defined
 * prior to first inclusion of aj_debug.h.
 * The corresponding flag dbgAJSVCAPP is defined in the containing sample app.
 */
#include "ajs.h"
#include "ajs_util.h"
#include "ajs_ctrlpanel.h"
#include "aj_debug.h"

#include <alljoyn/services_common/ServicesCommon.h>
#include <alljoyn/controlpanel/Common/ConstraintList.h>
#include <alljoyn/controlpanel/ControlPanelService.h>

#define LST_INDEX(id)  (((id) >> 24) & 0x7F)  /* Extract the object list index from a message or property id */
#define OBJ_INDEX(id)  (((id) >> 16) & 0xFF)  /* Extract the object index from a message or property id */
#define MBR_INDEX(id)  ((id) & 0xFF)          /* Extract the member index from a message or property id */

/*
 * Global so the CPS callbacks can access it.
 */
static AJ_Object* objectList;

static uint8_t InvolvesRootWidget(uint32_t identifier)
{
    return OBJ_INDEX(identifier) == 0;
}

static AJ_Status SetWidgetProp(AJ_Message* msg)
{
    AJS_Widget* widget = NULL;
    AJ_Message reply;
    AJ_Status status;
    uint32_t propId;
    const char* vsig;

    AJ_InfoPrintf(("SetWidgetProp %s\n", msg->objPath));

    status = AJ_UnmarshalPropertyArgs(msg, &propId, &vsig);
    /*
     * Two levels of variant because Set always uses a variant and the property type for the widget
     * is not known until runtime so is specified as a variant type in the property widget interface.
     */
    if (status == AJ_OK) {
        status = AJ_UnmarshalVariant(msg, &vsig);
    }
    if (status == AJ_OK) {
        status = AJ_UnmarshalVariant(msg, &vsig);
    }
    if (status == AJ_OK) {
        widget = (AJS_Widget*)objectList[OBJ_INDEX(propId)].context;
        /*
         * Value is the only property that is writeable. Figure out how to unmarshal it.
         */
        switch (widget->property.wdt.signature[0]) {
        case 'i':
            status = AJ_UnmarshalArgs(msg, "i", &widget->property.val.i);
            break;

        case 'q':
            status = AJ_UnmarshalArgs(msg, "q", &widget->property.val.q);
            break;

        case 'b':
            status = AJ_UnmarshalArgs(msg, "b", &widget->property.val.b);
            break;

        case 'd':
            status = AJ_UnmarshalArgs(msg, "d", &widget->property.val.d);
            break;

        case 's':
            status = AJ_UnmarshalArgs(msg, "s", &widget->property.val.s);
            break;

        default:
            {
                AJ_Arg st;
                uint16_t propertyType;
                status = AJ_UnmarshalContainer(msg, &st, AJ_ARG_STRUCT);
                if (status != AJ_OK) {
                    break;
                }
                status = AJ_UnmarshalArgs(msg, "q", &propertyType);
                if (status != AJ_OK) {
                    break;
                }
                /*
                 * For some reason the implementors of the control panel service used 0/1 to
                 * distingsuish between date/time rather than 1/2. Incrementing the property type
                 * fixes this oversight.
                 */
                if (++propertyType != widget->property.wdt.propertyType) {
                    status = AJ_ERR_INVALID;
                    break;
                }
                if (propertyType == TIME_VALUE_PROPERTY) {
                    status = AJ_UnmarshalArgs(msg, "(qqq)", &widget->property.val.time.hour, &widget->property.val.time.minute, &widget->property.val.time.second);
                } else {
                    status = AJ_UnmarshalArgs(msg, "(qqq)", &widget->property.val.date.mDay, &widget->property.val.date.month, &widget->property.val.date.fullYear);
                }
                /*
                 * Signal that the value has been changed
                 */
                AJS_CP_SignalValueChanged(AJS_GetBusAttachment(), widget);
                if (status == AJ_OK) {
                    status = AJ_UnmarshalCloseContainer(msg, &st);
                }
            }
            break;
        }
    } else {
        return AJ_ERR_RESOURCES;
    }
    /*
     * Need to make a clone of the message and close the original
     */
    msg = AJS_CloneAndCloseMessage(widget->dukCtx, msg);
    if (!msg) {
        return AJ_ERR_RESOURCES;
    }
    if (status == AJ_OK) {
        /*
         * Call JavaScript to report the value change
         */
        status =  AJS_CP_OnValueChanged(widget, msg->sender);
    } else {
        AJ_ErrPrintf(("SetWidgetProp %s\n", AJ_StatusText(status)));
    }
    if (status == AJ_OK) {
        AJ_MarshalReplyMsg(msg, &reply);
    } else {
        AJ_MarshalStatusMsg(msg, &reply, status);
    }
    AJ_DeliverMsg(&reply);

    return status;
}

static AJ_Status ExecuteAction(AJ_Message* msg, uint8_t action, void* context)
{
    AJS_Widget* widget = (AJS_Widget*)objectList[OBJ_INDEX(msg->msgId)].context;
    AJ_Status status;
    AJ_Message reply;

    /*
     * Need to make a clone of the message and close the original
     */
    msg = AJS_CloneAndCloseMessage(widget->dukCtx, msg);
    if (!msg) {
        return AJ_ERR_RESOURCES;
    }
    /*
     * Call into JavaScript object to perform action
     */
    status = AJS_CP_OnExecuteAction(widget, action, msg->sender);
    if (status == AJ_OK) {
        AJ_MarshalReplyMsg(msg, &reply);
    } else {
        AJ_MarshalStatusMsg(msg, &reply, status);
    }
    return AJ_DeliverMsg(&reply);
}

#define WIDGET_IFACE_INDEX      1
#define METADATA_CHANGED_INDEX  3
#define VALUE_CHANGED_INDEX     5

void AJS_CP_SignalValueChanged(AJ_BusAttachment* aj, AJS_Widget* ajsWidget)
{
    uint32_t session = AJCPS_GetCurrentSessionId();
    /*
     * Nothing to do unless the control panel is loaded and in a session
     */
    if (objectList && session) {
        /*
         * This is a little fragile because the indices might change if the interface definitions change
         * but there is no easy way to figure out what message id to use.
         */
        uint32_t sigId = AJ_ENCODE_MESSAGE_ID(AJCPS_OBJECT_LIST_INDEX, ajsWidget->index, WIDGET_IFACE_INDEX, VALUE_CHANGED_INDEX);
        /*
         * Need to find the message id for the value changed signal for this widget
         */
        AJ_InfoPrintf(("SignalValueChanged %s\n", ajsWidget->path));
        (void)AJCPS_SendPropertyChangedSignal(aj, sigId, session);
    }
}

void AJS_CP_SignalMetadataChanged(AJ_BusAttachment* aj, AJS_Widget* ajsWidget)
{
    uint32_t session = AJCPS_GetCurrentSessionId();
    /*
     * Nothing to do unless the control panel is loaded and in a session
     */
    if (objectList && session) {
        /*
         * This is a little fragile because the indices might change if the interface definitions change
         * but there is no easy way to figure out what message id to use.
         */
        uint32_t sigId = AJ_ENCODE_MESSAGE_ID(AJCPS_OBJECT_LIST_INDEX, ajsWidget->index, WIDGET_IFACE_INDEX, METADATA_CHANGED_INDEX);
        /*
         * Need to find the message id for the value changed signal for this widget
         */
        AJ_InfoPrintf(("SignalMetadataChanged %s\n", ajsWidget->path));
        (void)AJCPS_SendPropertyChangedSignal(aj, sigId, session);
    }
}

static AJSVC_ServiceStatus CPSMessageHandler(AJ_BusAttachment* bus, AJ_Message* msg, AJ_Status* msgStatus)
{
    AJ_Status status = AJ_OK;
    AJSVC_ServiceStatus handled = AJSVC_SERVICE_STATUS_HANDLED;
    AJ_MemberType mt;
    uint8_t isRoot;
    const char* member;

    if (!objectList || (LST_INDEX(msg->msgId) != AJCPS_OBJECT_LIST_INDEX)) {
        return AJSVC_SERVICE_STATUS_NOT_HANDLED;
    }
    AJ_InfoPrintf(("CPSMessageHandler %s::%s.%s\n", msg->objPath, msg->iface, msg->member));
    mt = AJ_GetMemberType(msg->msgId, &member, NULL);
    if (mt != AJ_METHOD_MEMBER) {
        return AJSVC_SERVICE_STATUS_NOT_HANDLED;
    }
    isRoot = InvolvesRootWidget(msg->msgId);
    if (strcmp(msg->member, "Get") == 0) {
        status = AJ_BusPropGet(msg, isRoot ? AJCPS_GetRootProperty : AJCPS_GetWidgetProperty, NULL);
    } else if (strcmp(msg->member, "Set") == 0) {
        status = SetWidgetProp(msg);
    } else if (strcmp(msg->member, "GetAll") == 0) {
        if (isRoot) {
            status = AJCPS_GetAllRootProperties(msg, msg->msgId);
        } else {
            status = AJCPS_GetAllWidgetProperties(msg, msg->msgId);
        }
    } else if (strcmp(msg->member, "Exec") == 0) {
        status = ExecuteAction(msg, 0, NULL);
    } else if ((strncmp(msg->member, "Action", 6) == 0) && !msg->member[7]) {
        status = ExecuteAction(msg, msg->member[6] - '0', NULL);
    } else if (strcmp(msg->member, "GetURL") == 0) {
        status = AJCPS_SendRootUrl(msg, msg->msgId);
    } else {
        handled = AJSVC_SERVICE_STATUS_NOT_HANDLED;
    }
    *msgStatus = status;
    return handled;
}

/*
 * Property names for widges are all unique in the first N byes
 */
#define UNIQUE_PREFIX_LEN  2

/*
 * Called with an identifier for a method call or property access message. Finds the widget that is
 * involved and determine what property value type if any is affected.
 */
static void* CallInvolvesWidget(uint32_t identifier, uint16_t* widgetType, uint16_t* propType, uint16_t* language)
{
    AJS_Widget* widget = (AJS_Widget*)objectList[OBJ_INDEX(identifier)].context;
    const char* member;
    AJ_MemberType mt = AJ_GetMemberType(identifier, &member, NULL);

    if (mt == AJ_INVALID_MEMBER) {
        return NULL;
    }
    *widgetType = widget->type;
    if (mt == AJ_PROPERTY_MEMBER) {
        /*
         * We have to do string comparisons here because the properties appears at different offsets
         * in the different widget interfaces, but we only need to check the first few characters.
         */
        if (strncmp(member, PROPERTY_TYPE_VERSION_NAME, UNIQUE_PREFIX_LEN) == 0) {
            *propType = PROPERTY_TYPE_VERSION;
        } else if (strncmp(member, PROPERTY_TYPE_STATES_NAME, UNIQUE_PREFIX_LEN) == 0) {
            *propType = PROPERTY_TYPE_STATES;
        } else if (strncmp(member, PROPERTY_TYPE_OPTPARAMS_NAME, UNIQUE_PREFIX_LEN) == 0) {
            *propType = PROPERTY_TYPE_OPTPARAMS;
        } else if (strncmp(member, PROPERTY_TYPE_VALUE_NAME, UNIQUE_PREFIX_LEN) == 0) {
            *propType = PROPERTY_TYPE_VALUE;
        } else if (strncmp(member, PROPERTY_TYPE_MESSAGE_NAME, UNIQUE_PREFIX_LEN) == 0) {
            *propType = PROPERTY_TYPE_MESSAGE;
        } else if (strncmp(member, PROPERTY_TYPE_NUM_ACTIONS_NAME, UNIQUE_PREFIX_LEN) == 0) {
            *propType = PROPERTY_TYPE_NUM_ACTIONS;
        } else if (strncmp(member, PROPERTY_TYPE_LABEL_NAME, UNIQUE_PREFIX_LEN) == 0) {
            *propType = PROPERTY_TYPE_LABEL;
        }
    } else {
        *propType = 0;
    }
    *language = 0;
    return (void*)widget;
}

const AJ_InterfaceDescription* AJS_WidgetInterfaces(uint8_t type)
{
    switch (type) {
    case WIDGET_TYPE_DIALOG:
        return DialogInterfaces;

    case WIDGET_TYPE_LABEL:
        return LabelPropertyInterfaces;

    case WIDGET_TYPE_ACTION:
        return ActionInterfaces;

    case WIDGET_TYPE_PROPERTY:
        return PropertyInterfaces;

    case WIDGET_TYPE_CONTAINER:
        return ContainerInterfaces;
    }
    return NULL;
}

/*
 * Called with a signal message id that is reporting a property value or metadata change. Finds out
 * which widget is involved and if the signal is reporting a value change or a metadata change.
 */
static void* SignalInvolvesWidget(uint32_t identifier, uint8_t* isProperty)
{
    const char* member;
    AJ_MemberType mt = AJ_GetMemberType(identifier, &member, NULL);

    if (mt == AJ_SIGNAL_MEMBER) {
        *isProperty = (member[0] == 'V');
        return objectList[OBJ_INDEX(identifier)].context;
    } else {
        return NULL;
    }
}

AJSVC_ServiceStatus AJS_CP_MessageHandler(AJ_BusAttachment* busAttachment, AJ_Message* msg, AJ_Status* status)
{
    if (objectList && (LST_INDEX(msg->msgId) == AJCPS_OBJECT_LIST_INDEX)) {
        return AJCPS_MessageProcessor(busAttachment, msg, status);
    } else {
        return AJSVC_SERVICE_STATUS_NOT_HANDLED;
    }
}

AJ_Status AJS_CP_Terminate()
{
    if (objectList) {
        objectList = NULL;
        return AJCPS_Stop();
    } else {
        AJ_ErrPrintf(("Object list is NULL\n"));
        return AJ_ERR_INVALID;
    }
}

AJ_Status AJS_CP_Init(AJ_Object* cpObjects)
{
    AJ_Status status;
    AJ_Object* w = cpObjects;

    while (w->path) {
        ++w;
    }
    w->path = "/NotificationActions";
    w->interfaces = NotificationActionInterfaces;
    /*
     * The control panel object is the only one that gets announced
     */
    cpObjects[0].interfaces = ControlPanelInterfaces;
    cpObjects[0].flags = AJ_OBJ_FLAG_ANNOUNCED;

    status = AJCPS_Start(cpObjects, CPSMessageHandler, CallInvolvesWidget, SignalInvolvesWidget, InvolvesRootWidget);
    if (status == AJ_OK) {
        AJ_WarnPrintf(("Control panel service succesfully initialized\n"));
        objectList = cpObjects;
        AJ_AboutSetShouldAnnounce();
#ifndef NDEBUG
        if (dbgAJS) {
            AJ_PrintXML(cpObjects);
        }
#endif

    }
    return status;
}