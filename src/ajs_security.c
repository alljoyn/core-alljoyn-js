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

#include "ajs.h"
#include "ajs_util.h"
#include "ajs_debugger.h"
#include "ajs_security.h"
#include <ajtcl/aj_msg_priv.h>

#include <ajtcl/aj_cert.h>
#include <ajtcl/aj_peer.h>
#include <ajtcl/aj_creds.h>
#include <ajtcl/aj_auth_listener.h>
#include <ajtcl/aj_authentication.h>
#include <ajtcl/aj_security.h>

typedef struct {
    char* peer;
    duk_context* ctx;
} PeerInfo;

/*
 * suites has a size of 3 for the only
 * only 3 valid suites in thin core
 */
static uint32_t suites[3] = {0, 0, 0} ;
static uint32_t keyExpiration = 0xFFFFFFFF;
static const char* pem_prv = NULL;

static const char* pem_x509 = NULL;

static const char* ecspeke_password = NULL;
static X509CertificateChain* chain = NULL;

static uint8_t isSecurityEnabled = FALSE;
static AJ_PermissionRule* securityRules = NULL;
static AJ_PermissionMember members[] = { { "*", AJ_MEMBER_TYPE_ANY, AJ_ACTION_PROVIDE | AJ_ACTION_OBSERVE, NULL } };

static AJ_Status AuthListenerCallback(uint32_t authmechanism, uint32_t command, AJ_Credential*cred)
{
    AJ_Status status = AJ_ERR_INVALID;
    X509CertificateChain* node;

    AJ_InfoPrintf(("AuthListenerCallback authmechanism %08X command %d\n", authmechanism, command));

    switch (authmechanism) {
    case AUTH_SUITE_ECDHE_NULL:
        cred->expiration = keyExpiration;
        status = AJ_OK;
        break;

    case AUTH_SUITE_ECDHE_SPEKE:
        switch (command) {
        case AJ_CRED_PASSWORD:
            if (ecspeke_password) {
                cred->data = (uint8_t*)ecspeke_password;
                cred->len = strlen(ecspeke_password);
                cred->expiration = keyExpiration;
                status = AJ_OK;
            }
            break;
        }
        break;

    case AUTH_SUITE_ECDHE_ECDSA:
        switch (command) {
        case AJ_CRED_PRV_KEY:
            AJ_ASSERT(sizeof (AJ_ECCPrivateKey) == cred->len);
            status = AJ_DecodePrivateKeyPEM((AJ_ECCPrivateKey*) cred->data, pem_prv);
            cred->expiration = keyExpiration;
            break;

        case AJ_CRED_CERT_CHAIN:
            switch (cred->direction) {
            case AJ_CRED_REQUEST:
                // Free previous certificate chain
                if (pem_x509) {
                    AJ_X509FreeDecodedCertificateChain(chain);
                    chain = AJ_X509DecodeCertificateChainPEM(pem_x509);
                    if (NULL == chain) {
                        return AJ_ERR_INVALID;
                    }
                    cred->data = (uint8_t*) chain;
                    cred->expiration = keyExpiration;
                    status = AJ_OK;
                }
                break;

            case AJ_CRED_RESPONSE:
                node = (X509CertificateChain*) cred->data;
                status = AJ_X509VerifyChain(node, NULL, AJ_CERTIFICATE_IDN_X509);
                while (node) {
                    AJ_DumpBytes("CERTIFICATE", node->certificate.der.data, node->certificate.der.size);
                    node = node->next;
                }
                break;
            }
            break;
        }
        break;

    default:
        break;
    }
    return status;
}

AJ_PermissionMember* AJS_GetPermissionMembers()
{
    return members;
}

uint8_t AJS_IsSecurityEnabled(void)
{
    return isSecurityEnabled;
}

void AJS_SetSecurityRules(duk_context* ctx, const AJ_PermissionRule* rules, const unsigned int numRules)
{
    if (securityRules) {
        duk_free(ctx, securityRules);
        securityRules = NULL;
    }
    securityRules = duk_alloc(ctx, numRules*sizeof(AJ_PermissionRule));
    memcpy(securityRules, rules, numRules*sizeof(AJ_PermissionRule));
}

uint32_t AJS_GetAllJoynSecurityProps(duk_context* ctx, duk_idx_t enumIdx)
{
   size_t suitesIdx = 0;
   duk_enum(ctx, enumIdx, DUK_ENUM_OWN_PROPERTIES_ONLY);
   while (duk_next(ctx, -1, 1)) {
       const char* property = duk_require_string(ctx, -2);
       if (strcmp(property, "expiration") == 0) {
           keyExpiration = duk_get_int(ctx, -1);
           duk_pop(ctx);
       } else if (strcmp(property, "speke") == 0) {
           duk_enum(ctx, -1, 1);

           ecspeke_password = AJS_GetStringProp(ctx, -2, "password");
           if (!ecspeke_password) {
               duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "password is required for speke suite");
           } else {
               suites[suitesIdx++] = AUTH_SUITE_ECDHE_SPEKE;
           }

           duk_pop_2(ctx);
       } else if (strcmp(property, "ecdhe_null") == 0) {
           if(duk_is_boolean(ctx, -1) && duk_get_boolean(ctx, -1)) {
                suites[suitesIdx++] = AUTH_SUITE_ECDHE_NULL;
           }
           duk_pop(ctx);
       } else if (strcmp(property, "ecdsa") == 0) {
           duk_enum(ctx, -1, 1);

           pem_prv = AJS_GetStringProp(ctx, -2, "prv_key");
           if (!pem_prv) {
               duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "prv_key is required for ecdsa suit");
           } else {
               pem_x509 = AJS_GetStringProp(ctx, -2, "cert_chain");
               if (!pem_x509) {
                   duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "cert_chain is required for AJ.securityDefinition");
               }

               suites[suitesIdx++] = AUTH_SUITE_ECDHE_ECDSA;
           }
           duk_pop_2(ctx);
       }
       duk_pop(ctx);
   }
   duk_pop(ctx);
   return AJ_OK;
}

AJ_Status AJS_EnableSecurity(duk_context* ctx)
{
   uint16_t state;
   uint16_t capabilities;
   uint16_t infoConfig;
   AJ_Status status = AJ_OK;

   if (!securityRules) {
       return status;
   }


   status = AJ_BusEnableSecurity(AJS_GetBusAttachment(), suites, 3);
   if (status != AJ_OK) {
       duk_error(ctx, DUK_ERR_TYPE_ERROR, "AJ_BusEnableSecurity() failed\n");
   }
   AJ_BusSetAuthListenerCallback(AJS_GetBusAttachment(), AuthListenerCallback);
   AJ_ManifestTemplateSet(securityRules);
   AJ_SecurityGetClaimConfig(&state, &capabilities, &infoConfig);
   /*
    * Set app claimable if not already claimed
    */
   if (APP_STATE_CLAIMED != state) {
        AJ_SecuritySetClaimConfig(AJS_GetBusAttachment(), APP_STATE_CLAIMABLE, CLAIM_CAPABILITY_ECDHE_PSK, 0);
   }


   return status;
}
