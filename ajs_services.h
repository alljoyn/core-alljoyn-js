#ifndef _AJS__SERVICES_H
#define _AJS__SERVICES_H
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

#include <aj_target.h>
#include <alljoyn.h>

#define NUM_CONTROLPANEL_OBJECTS 0

#include <alljoyn/services_common/ServicesHandlers.h>
#include <alljoyn/services_common/ServicesCommon.h>
#include <alljoyn/onboarding/OnboardingManager.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the Services. called at the start of the application
 *
 * @param aj                      The bus attachment
 */
AJ_Status AJS_ServicesInit(AJ_BusAttachment* aj);

/**
 * Pass a message for possible handling by one of the basic services.
 *
 * @param msg  The message to be handled
 *
 * @return - AJ_OK if the message was handled by a service
 *         - AJ_ERR_NO_MATCH if the message was not handled
 *         - Other errors indicating an unmarshalling failure
 */
AJ_Status AJS_ServicesMsgHandler(AJ_Message* msg);

#ifdef __cplusplus
}
#endif

#endif