#ifndef _AJS_SECURITY_H
#define _AJS_SECURITY_H
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
#include <ajtcl/aj_authorisation.h>
#include <duktape.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t AJS_IsSecurityEnabled(void);

/**
 * Return permission members
 */
AJ_PermissionMember* AJS_GetPermissionMembers();

/**
 * Set interface permission rules
 *
 * @param ctx An opaque pointer to a duktape context structure
 * @param rules Permission rule set
 * @param numRules Permission rule set size
 */
void AJS_SetSecurityRules(duk_context* ctx, const AJ_PermissionRule const* rules, const unsigned int numRules);

/**
 * Build security properties from securityDefinition JavaScript object
 *
 * @param ctx An opaque pointer to a duktape context structure
 * @param enumIdx Index of securityDefinition object within duktape context
 */
AJ_Status AJS_GetAllJoynSecurityProps(duk_context* ctx, duk_idx_t enumIdx);

/**
 * Enable security
 *
 * @param ctx An opaque pointer to a duktape context structure
 */
AJ_Status AJS_EnableSecurity(duk_context* ctx);

#ifdef __cplusplus
}
#endif
#endif