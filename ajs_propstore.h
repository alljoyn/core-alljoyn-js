/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
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
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
******************************************************************************/

#ifndef _AJS_PROPSTORE_H_
#define _AJS_PROPSTORE_H_

#include <alljoyn.h>
#include <alljoyn/services_common/PropertyStore.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PASSWORD_VALUE_LENGTH     (AJ_ADHOC_LEN * 2)

/**
 * Initialize the property store
 */
AJ_Status AJS_PropertyStore_Init();

/**
 * Returns TRUE is the property is read only
 */
uint8_t AJS_PropertyStore_IsReadOnly(int8_t index);

/**
 * Function to retrieve a password from the property store.
 *
 * @param buffer Buffer to return the password
 * @param bufLen The size of the buffer
 *
 * @return  Returns the actual length of the password
 */
uint32_t AJS_PasswordCallback(uint8_t* buffer, uint32_t bufLen);

#ifdef __cplusplus
}
#endif

#endif //_AJS_PROPSTORE_H