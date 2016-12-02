#ifndef _DUK_CUSTOM_H
#define _DUK_CUSTOM_H
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

#include "ajs_duk.h"

/*
 * Force alignment for testing
 */
#ifdef DUK_FORCE_ALIGNED_ACCESS
#undef DUK_USE_UNALIGNED_ACCESSES_POSSIBLE
#undef DUK_USE_HASHBYTES_UNALIGNED_U32_ACCESS
#undef DUK_USE_HOBJECT_UNALIGNED_LAYOUT
#endif

#endif