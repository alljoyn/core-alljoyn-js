#ifndef _AJS_SECURITY_H
#define _AJS_SECURITY_H
/**
 * @file
 */
/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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
