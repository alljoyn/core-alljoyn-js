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
#include "ajs_target.h"
#include "aj_nvram.h"

extern uint8_t dbgMSG;
extern uint8_t dbgHELPER;
extern uint8_t dbgBUS;
extern uint8_t dbgABOUT;
extern uint8_t dbgINTROSPECT;
extern uint8_t dbgAJCPS;
extern uint8_t dbgAJS;
extern uint8_t dbgAJOBS;
extern uint8_t dbgHEAP;
extern uint8_t dbgDISCO;
extern uint8_t dbgCONNECT;
extern uint8_t dbgPEER;
extern uint8_t dbgAJSVC;

void _AJ_Reboot(void)
{

}

int AJ_Main()
{
    AJ_Status status;

#ifndef NDEBUG
    AJ_DbgLevel = AJ_DEBUG_DUMP;
    dbgMSG = 1;
    dbgHELPER = 1;
    dbgABOUT = 1;
    dbgBUS = 1;
    dbgINTROSPECT = 1;
    dbgAJCPS = 1;
    dbgAJS = 1;
    dbgAJOBS = 1;
    dbgCONNECT = 1;
    dbgDISCO = 1;
    dbgAJSVC = 1;
    dbgHEAP = 0;
    dbgPEER = 1;
#endif

    AJ_Initialize();

    /*
     * This is just for testing onboarding and config
     */
#ifdef NVRAM_CLEAR_ON_START
    AJ_NVRAM_Clear();
#endif

    while (1) {
        status = AJS_Main("freedom");
        if (status != AJ_ERR_RESTART) {
            AJ_Reboot();
        }
    }
}