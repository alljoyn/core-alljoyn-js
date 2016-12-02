/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/


#include "ajs.h"
#include <aj_debug.h>
#include <aj_config.h>
#include <aj_creds.h>
#include <aj_link_timeout.h>

#include "ajs_ctrlpanel.h"
#include "ajs_services.h"
#include "ajs_propstore.h"

#include <alljoyn/config/ConfigService.h>
#include <alljoyn/notification/NotificationProducer.h>
#include <alljoyn/services_common/PropertyStore.h>

static AJ_Status SetPasscode(const char* routerRealm, const uint8_t* newPasscode, uint8_t newPasscodeLen)
{
    AJ_Status status = AJ_OK;
    char newStringPasscode[PASSWORD_VALUE_LENGTH + 1];

    status = AJ_RawToHex(newPasscode, newPasscodeLen, newStringPasscode, sizeof(newStringPasscode), FALSE);
    if (status != AJ_OK) {
        return status;
    }
    if (AJSVC_PropertyStore_SetValue(AJSVC_PROPERTY_STORE_REALM_NAME, routerRealm) && AJSVC_PropertyStore_SetValue(AJSVC_PROPERTY_STORE_PASSCODE, newStringPasscode)) {
        status = AJSVC_PropertyStore_SaveAll();
        if (status != AJ_OK) {
            return status;
        }
        AJ_ClearCredentials();
        status = AJ_ERR_READ;     //Force disconnect of AJ and services to refresh current sessions
    } else {
        status = AJSVC_PropertyStore_LoadAll();
        if (status != AJ_OK) {
            return status;
        }
    }
    return status;
}

AJ_Status AJS_FactoryReset(void)
{
    AJ_Status status = AJ_OK;

    AJ_WarnPrintf(("FactoryReset\n"));
    status = AJSVC_PropertyStore_ResetAll();
    if (status != AJ_OK) {
        return status;
    }
    status = AJOBS_ClearInfo();
    if (status != AJ_OK) {
        return status;
    }
    AJ_ClearCredentials();
    return AJ_ERR_RESTART;
}

static uint8_t IsValueValid(const char* key, const char* value)
{
    return TRUE;
}

static AJ_Status Restart(void)
{
    AJ_WarnPrintf(("Restart\n"));
    AJ_AboutSetShouldAnnounce();
    return AJ_ERR_RESTART;
}

AJ_Status AJS_ServicesInit(AJ_BusAttachment* aj)
{
    AJ_Status status = AJ_OK;

    AJ_BusSetPasswordCallback(aj, AJS_PasswordCallback);
    status = AJCFG_Start(AJS_FactoryReset, Restart, SetPasscode, IsValueValid);
    if (status != AJ_OK) {
        goto Exit;
    }
    status = AJNS_Producer_Start();
    if (status != AJ_OK) {
        goto Exit;
    }

Exit:
    return status;
}

AJ_Status AJS_ServicesMsgHandler(AJ_Message* msg)
{
    AJ_Status status = AJ_OK;
    AJSVC_ServiceStatus svcStatus;

    svcStatus = AJSVC_MessageProcessorAndDispatcher(msg->bus, msg, &status);
    if (svcStatus == AJSVC_SERVICE_STATUS_NOT_HANDLED) {
        return AJ_ERR_NO_MATCH;
    } else {
        return status;
    }
}