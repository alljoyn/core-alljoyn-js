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
#ifndef AJS_STORAGE_H_
#define AJS_STORAGE_H_

#include "ajs.h"

/**
 * Open a script for reading or writing
 *
 * @param length            Length of the script (zero implies a read)
 * @param ctx[out]          Out parameter containing the context for the opened script
 * @return                  AJ_OK if opened
 *                          AJ_ERR_RESOURCES if the script was too large
 *                          AJ_ERR_FAILURE for some other failure
 */
AJ_Status AJS_OpenScript(uint32_t length, void** ctx);

/**
 * Write a portion (or all) of a script to persistant storage
 *
 * @param script            Section of script to write
 * @param length            Length of script section
 * @param ctx               Context returned from AJS_OpenScript()
 * @return                  AJ_OK if written
 */
AJ_Status AJS_WriteScript(uint8_t* script, uint32_t length, void* ctx);

/**
 * Read a script out of persistant storage
 *
 * @param[out] script       Pointer that will contain script buffer
 * @param[out] length       Pointer that will contain scripts length
 * @param ctx               Context returned from AJS_OpenScript()
 * @return                  AJ_OK if read
 */
AJ_Status AJS_ReadScript(uint8_t** script, uint32_t* length, void* ctx);

/**
 * Close a script
 *
 * @param ctx               Context of script to close
 * @return                  AJ_OK if closed
 */
AJ_Status AJS_CloseScript(void* ctx);

/**
 * Delete a script from persistant storage
 *
 * @return                  AJ_OK if deleted
 */
AJ_Status AJS_DeleteScript(void);

/**
 * Returns the maximum space to allocate for a script. The value returned is target specific
 * and depends on available resources.
 *
 * @return  The maximum permitted script length in bytes.
 */
uint32_t AJS_MaxScriptLen(void);

#endif /* AJS_STORAGE_H_ */