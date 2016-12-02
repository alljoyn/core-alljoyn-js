#ifndef _AJS_TARGET_H
#define _AJS_TARGET_H
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

#include "ajs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Returns maximum size of a script for this target
 */
size_t AJS_GetMaxScriptLen();

/**
 * Delete the currently installed script.
 */
AJ_Status AJS_DeleteScript();

/**
 * Find out if a script is installed
 *
 * @return Returns TRUE if a script is installed.
 */
uint8_t AJS_IsScriptInstalled();

#define AJS_SCRIPT_READ  0  /**< Opens a script is read mode */
#define AJS_SCRIPT_WRITE 1  /**< Opens a script in write mode */

/**
 * Open a script to read or write. Returns a pointer to an opaque structure required by
 * AJS_ReadScript() and AJS_WriteScript(). If the script is opened to write the currently installed
 * script is deleted.
 *
 * @param mode  Indicates if the script is being opened to read it or write it.
 */
void* AJS_OpenScriptFile(uint8_t mode);

/**
 * Writes data to a script file.
 *
 * @param scriptf  A pointer to an opaque data structure that maintains state information
 * @param data     The data to be appended to the script
 * @param len      The length of the data to be appended to the script
 *
 * @return  - AJ_OK if the data was succesfully appended to the script file.
 *          - AJ_ERR_RESOURCES if attempting to write more that AJS_MaxScriptLen() bytes
 */
AJ_Status AJS_WriteScriptFile(void* scriptf, const uint8_t* data, size_t len);

/**
 * Reads a script returning a pointer the entire contiguous script file.
 */
AJ_Status AJS_ReadScriptFile(void* scriptf, const uint8_t** data, size_t* len);

/**
 * Close an open script freeing any resources.
 *
 * @param scriptf  A pointer to an opaque data structure that maintains state information
 *
 * @return - AJ_OK if the script was closed
 *         - AJ_ERR_UNEXPECTED if the script was not open
 */
AJ_Status AJS_CloseScriptFile(void* scriptf);

#ifdef __cplusplus
}
#endif

#endif