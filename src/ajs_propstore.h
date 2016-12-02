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

#ifndef _AJS_PROPSTORE_H_
#define _AJS_PROPSTORE_H_

#include <ajtcl/alljoyn.h>
#include <ajtcl/services/PropertyStore.h>

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
 * Set the PropertyStore passcode
 *
 * @param passcode Passcode to use
 *
 * @return Returns TRUE if successful
 */
uint8_t AJS_PropertyStore_SetPasscode(const char* passcode);

#ifdef __cplusplus
}
#endif

#endif //_AJS_PROPSTORE_H