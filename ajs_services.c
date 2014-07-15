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


#include "ajs.h"
#include <aj_debug.h>
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

static uint32_t PasswordCallback(uint8_t* buffer, uint32_t bufLen)
{
    AJ_Status status = AJ_OK;
    const char* hexPassword;
    size_t hexPasswordLen;
    uint32_t len = 0;

    hexPassword = AJSVC_PropertyStore_GetValue(AJSVC_PROPERTY_STORE_PASSCODE);
    if (hexPassword == NULL) {
        AJ_AlwaysPrintf(("Password is NULL!\n"));
        return len;
    }
    AJ_AlwaysPrintf(("Retrieved password=%s\n", hexPassword));
    hexPasswordLen = strlen(hexPassword);
    len = hexPasswordLen / 2;
    status = AJ_HexToRaw(hexPassword, hexPasswordLen, buffer, bufLen);
    if (status == AJ_ERR_RESOURCES) {
        len = 0;
    }
    return len;
}

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

static AJ_Status FactoryReset(void)
{
    AJ_Status status = AJ_OK;

    AJ_WarnPrintf(("GOT FACTORY RESET\n"));
    status = AJSVC_PropertyStore_ResetAll();
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
    AJ_WarnPrintf(("GOT RESTART REQUEST\n"));
    AJ_AboutSetShouldAnnounce();
    return AJ_ERR_RESTART;
}

AJ_Status AJS_ServicesInit(AJ_BusAttachment* aj)
{
    AJ_Status status = AJ_OK;

    AJ_BusSetPasswordCallback(aj, PasswordCallback);
    status = AJCFG_Start(FactoryReset, Restart, SetPasscode, IsValueValid);
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
