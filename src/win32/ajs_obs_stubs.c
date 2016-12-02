/**
 * @file
 */
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

#include <ajtcl/alljoyn.h>
#include <ajtcl/services/OnboardingService.h>
#include <ajtcl/services/OnboardingManager.h>

int8_t AJOBS_GetState()
{
    return AJOBS_STATE_CONFIGURED_VALIDATED;
}

AJ_Status AJOBS_Start(const AJOBS_Settings* settings)
{
    return AJ_OK;
}

AJ_Status AJOBS_EstablishWiFi()
{
    return AJ_OK;
}

AJ_Status AJOBS_DisconnectWiFi()
{
    return AJ_OK;
}

AJ_Status AJOBS_SwitchToRetry()
{
    return AJ_OK;
}

AJ_Status AJOBS_ConnectedHandler(AJ_BusAttachment* busAttachment)
{
    return AJ_OK;
}

AJ_Status AJOBS_DisconnectHandler(AJ_BusAttachment* busAttachment)
{
    return AJ_OK;
}

AJSVC_ServiceStatus AJOBS_MessageProcessor(AJ_BusAttachment* busAttachment, AJ_Message* msg, AJ_Status* msgStatus)
{
    return AJSVC_SERVICE_STATUS_NOT_HANDLED;
}