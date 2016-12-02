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
var AJ = require('AllJoyn');
var IO = require('IO');

AJ.interfaceDefinition['org.allseen.DoorBell'] = { ding_dong:{ type:AJ.SIGNAL } };

AJ.objectDefinition['/DoorBell'] = { interfaces:['org.allseen.DoorBell'] };

AJ.onSignal = function()
{
    if (this.member == 'ding_dong') {
        alert('ding dong');
        IO.system('aplay DoorBell.wav');
    }
}