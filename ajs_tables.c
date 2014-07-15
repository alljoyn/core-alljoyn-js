/**
 * @file
 */
/******************************************************************************
 * Copyright (c) 2013, 2014, AllSeen Alliance. All rights reserved.
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
#include "ajs_ctrlpanel.h"
#include "ajs_translations.h"

#define JS_OBJ_INDEX AJAPP_OBJECTS_LIST_INDEX

/*
 * Table of all interfaces
 */
static AJ_InterfaceDescription* interfaceTable;

/*
 * Local and proxy object tables
 */
static AJ_Object* objectList;
static AJ_Object proxyList[2];

static void AddArgs(duk_context* ctx, duk_idx_t strIdx, char inout)
{
    int i;
    int numArgs = duk_get_length(ctx, -1);

    if (!duk_is_array(ctx, -1)) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Argument list must be an array");
    }
    for (i = 0; i < numArgs; ++i) {
        const char* sig;
        const char* name = "";
        duk_get_prop_index(ctx, -1, i);
        if (duk_is_string(ctx, -1)) {
            sig = duk_get_string(ctx, -1);
        } else {
            duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
            duk_next(ctx, -1, 1);
            sig = duk_require_string(ctx, -1);
            name = duk_require_string(ctx, -2);
            duk_pop_3(ctx);
        }
        duk_pop(ctx);
        duk_dup(ctx, strIdx);
        duk_push_sprintf(ctx, " %s%c%s", name, inout, sig);
        AJ_InfoPrintf(("Arg: %s\n", duk_get_string(ctx, -1)));
        duk_concat(ctx, 2);
        duk_replace(ctx, strIdx);
    }
}

static const char* ConstructMemberString(duk_context* ctx)
{
    const char* memberString;
    const char* member = duk_get_string(ctx, -2);
    int type = AJS_GetIntProp(ctx, -1, "type");

    AJ_InfoPrintf(("BuildMemberList %s\n", member));

    if ((type < 0) || (type > 2)) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Member type must be AJ.SIGNAL, AJ.METHOD, or AJ.PROPERTY");
    }
    /*
     * Member strings must be stable so we add them to a global stash array
     */
    AJS_GetGlobalStashArray(ctx, "members");
    /*
     * Start building the member string
     */
    if (type == 2) {
        char achar = '=';
        /* Property */
        const char* sig = AJS_GetStringProp(ctx, -2, "signature");
        const char* access = AJS_GetStringProp(ctx, -2, "access");

        if (!sig) {
            duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "Signature is required for type AJ.PROPERTY");
        }
        if (access) {
            if (strcmp(access, "R") == 0) {
                achar = '<';
            } else if (strcmp(access, "W") == 0) {
                achar = '>';
            } else if (strcmp(access, "RW") != 0) {
                duk_error(ctx, DUK_ERR_TYPE_ERROR, "Access must be 'R', 'w', or 'RW'");
            }
        }
        duk_push_sprintf(ctx, "@%s%c%s", member, achar, sig);
    } else {
        duk_idx_t strIdx;
        duk_push_sprintf(ctx, "%c%s", type ? '!' : '?', member);
        strIdx = duk_get_top_index(ctx);
        /* Method or Signal */
        duk_get_prop_string(ctx, -3, "args");
        if (!duk_is_undefined(ctx, -1)) {
            AddArgs(ctx, strIdx, (type == 0) ? '<' : '>');
        }
        duk_pop(ctx);
        if (type == 0) {
            /* Method only */
            duk_get_prop_string(ctx, -3, "returns");
            if (!duk_is_undefined(ctx, -1)) {
                AddArgs(ctx, strIdx, '>');
            }
            duk_pop(ctx);
        }
    }
    memberString = duk_get_string(ctx, -1);
    /*
     * Add the member string to the "members" array
     */
    duk_put_prop_index(ctx, -2, duk_get_length(ctx, -2));
    duk_pop(ctx);
    AJ_InfoPrintf(("Added: \"%s\"\n", memberString));
    return memberString;
}

static void BuildMemberList(duk_context* ctx, const char** ifc)
{
    AJ_InfoPrintf(("BuildMemberList\n"));

    duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
    while (duk_next(ctx, -1, 1)) {
        if (duk_is_object(ctx, -1)) {
            *ifc++ = ConstructMemberString(ctx);
        }
        duk_pop_2(ctx);
    }
    duk_pop(ctx);
}

static void BuildInterfaceTable(duk_context* ctx)
{
    size_t i;
    size_t allocSz;

    AJ_ASSERT(interfaceTable == NULL);

    AJ_InfoPrintf(("BuildInterfaceTables\n"));

    duk_get_prop_string(ctx, -1, "interfaceDefinition");
    allocSz = (AJS_NumProps(ctx, -1) + 2) * sizeof(AJ_InterfaceDescription*);
    /*
     * Allocate space for a NULL terminated interface table
     */
    interfaceTable = duk_alloc(ctx, allocSz);
    memset((void*)interfaceTable, 0, allocSz);
    /*
     * Always add the properties interface to the list
     */
    interfaceTable[0] = AJ_PropertiesIface;
    duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
    for (i = 1; duk_next(ctx, -1, 1); ++i) {
        size_t numMembers = AJS_NumProps(ctx, -1);
        const char* ifcName = duk_require_string(ctx, -2);
        /*
         * Allocate space for a NULL terminated interface description
         */
        const char** ifc = duk_alloc(ctx, (2 + numMembers) * sizeof(char*));
        memset((void*)ifc, 0, (2 + numMembers) * sizeof(char*));
        /*
         * First entry is the interface name
         */
        ifc[0] = ifcName;
        AJ_InfoPrintf(("%s\n", ifc[0]));
        BuildMemberList(ctx, &ifc[1]);
        ifc[numMembers + 1] = NULL;
        duk_pop_2(ctx);
        interfaceTable[i] = ifc;
    }
    duk_pop_2(ctx); // enum + interfaces
}

static AJ_InterfaceDescription LookupInterface(duk_context* ctx, const char* iface)
{
    size_t i = 0;
    for (i = 0; interfaceTable[i]; ++i) {
        if (strcmp(interfaceTable[i][0], iface) == 0) {
            return interfaceTable[i];
        }
    }
    return NULL;
}

static void BuildLocalObjects(duk_context* ctx)
{
    size_t numObjects = AJS_NumProps(ctx, -1) + 1;
    size_t allocSz;
    AJ_Object* jsObjects;

    AJ_ASSERT(objectList == NULL);

    AJ_InfoPrintf(("BuildLocalObjects\n"));

    duk_get_prop_string(ctx, -1, "objectDefinition");
    allocSz = numObjects * sizeof(AJ_Object);

    objectList = duk_alloc(ctx, allocSz);
    memset(objectList, 0, allocSz);

    /*
     * Add objects declared by the script
     */
    jsObjects = objectList;
    duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
    while (duk_next(ctx, -1, 1)) {
        size_t i;
        size_t numInterfaces;
        AJ_InterfaceDescription* interfaces;

        duk_get_prop_string(ctx, -1, "interfaces");
        if (!duk_is_array(ctx, -1)) {
            AJ_ErrPrintf(("Object definition requires an array of interfaces\n"));
            duk_pop(ctx);
            continue;
        }
        numInterfaces = duk_get_length(ctx, -1);
        interfaces = duk_alloc(ctx, (numInterfaces + 2) * sizeof(AJ_InterfaceDescription*));
        memset(interfaces, 0, (numInterfaces + 2) * sizeof(AJ_InterfaceDescription*));
        /*
         * Locate the interfaces in the interface table
         */
        for (i = 0; i < numInterfaces; ++i) {
            duk_get_prop_index(ctx, -1, i);
            interfaces[i] = LookupInterface(ctx, duk_get_string(ctx, -1));
            duk_pop(ctx);
        }
        /*
         * Done with the interfaces
         */
        duk_pop(ctx);
        interfaces[numInterfaces++] = AJ_PropertiesIface;
        interfaces[numInterfaces] = NULL;

        jsObjects->path = duk_require_string(ctx, -2);
        AJ_InfoPrintf(("%s\n", jsObjects->path));
        jsObjects->interfaces = interfaces;
        jsObjects->flags = AJ_OBJ_FLAG_ANNOUNCED;
        duk_pop_2(ctx);
        ++jsObjects;
    }
    duk_pop(ctx); // enum
    duk_pop(ctx); // JavaScript objects
}

void AJS_ResetTables(duk_context* ctx)
{
    AJ_RegisterObjectList(NULL, JS_OBJ_INDEX);
    AJS_CP_Terminate();
    duk_free(ctx, objectList);
    objectList = NULL;
    memset(proxyList, 0, sizeof(proxyList));
    duk_free(ctx, (void*)interfaceTable);
    interfaceTable = NULL;
}

void ExtractMember(duk_context* ctx, const char* member)
{
    int32_t len = AJ_StringFindFirstOf(member, " ");
    if (len > 0) {
        duk_push_lstring(ctx, member + 1, len);
    } else {
        duk_push_string(ctx, member + 1);
    }

}

static duk_context* dukContext;

static const char* DescriptionFinder(uint32_t descId, const char* lang)
{
    duk_context* ctx = dukContext;
    const char* description = NULL;
    uint8_t objIndex = descId >> 24;
    uint8_t ifcIndex = descId >> 16;
    const AJ_Object* obj = &objectList[objIndex];
    duk_idx_t topIdx = duk_get_top_index(ctx);

    if (ifcIndex) {
        const AJ_InterfaceDescription ifc = obj->interfaces[ifcIndex - 1];
        uint8_t memIndex = descId >> 8;

        AJS_GetAllJoynProperty(ctx, "interfaceDefinition");
        duk_get_prop_string(ctx, -1, ifc[0]);
        duk_remove(ctx, -2);

        if (duk_is_object(ctx, -1) && memIndex) {
            uint8_t argIndex = descId;
            ExtractMember(ctx, ifc[memIndex]);
            duk_get_prop(ctx, -2);
            duk_remove(ctx, -2);

            if (duk_is_object(ctx, -1) && argIndex) {
                duk_get_prop_string(ctx, -1, "args");
                duk_remove(ctx, -2);
                if (duk_is_object(ctx, -1)) {
                    duk_get_prop_index(ctx, -1, argIndex - 1);
                    duk_remove(ctx, -2);
                }
            }
        }
    } else {
        AJS_GetAllJoynProperty(ctx, "objectDefinition");
        duk_get_prop_string(ctx, -1, obj->path);
        duk_remove(ctx, -2);
    }
    if (duk_is_object(ctx, -1)) {
        /*
         * We have an object see if it has a description
         */
        duk_get_prop_string(ctx, -1, "description");
        if (duk_is_string(ctx, -1)) {
            description = AJS_GetTranslatedString(ctx, -1, AJS_GetLanguageIndex(ctx, lang));
        }
        duk_pop(ctx);
    }
    duk_pop(ctx);

    if (topIdx != duk_get_top_index(ctx)) {
        AJS_DumpStack(ctx);
        AJ_ASSERT(topIdx == duk_get_top_index(ctx));
    }
    return description;
}

AJ_Status AJS_InitTables(duk_context* ctx)
{
    BuildInterfaceTable(ctx);
    BuildLocalObjects(ctx);
    AJ_RegisterObjectList(proxyList, AJ_PRX_ID_FLAG);
    /*
     * Have to register this global so DescriptionFinder has access to the duktape context
     */
    dukContext = ctx;
    AJ_RegisterObjectListWithDescriptions(objectList, JS_OBJ_INDEX, DescriptionFinder);
    AJ_PrintXMLWithDescriptions(objectList, "en");
    return AJ_OK;
}

void AJS_SetObjectPath(const char* path)
{
    proxyList[0].path = path;
    if (path) {
        proxyList[0].interfaces = interfaceTable;
    } else {
        proxyList[0].interfaces = NULL;
    }
}
