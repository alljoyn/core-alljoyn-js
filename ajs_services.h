#ifndef _AJS__SERVICES_H
#define _AJS__SERVICES_H
/**
 * @file
 */
/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
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