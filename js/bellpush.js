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
var IO = require('IO');
var AJ = require('AllJoyn');

AJ.interfaceDefinition['org.allseen.DoorBell'] = { ding_dong:{ type:AJ.SIGNAL } };

AJ.objectDefinition['/pushbutton'] = { interfaces:['org.allseen.DoorBell'] };

var pb = IO.digitalIn(IO.pin[8], IO.pullDown);

AJ.onAttach = function()
{
    AJ.findService('org.allseen.DoorBell', function(svc) {
        var dingdong = svc.signal('/pushbutton', 'ding_dong');
        pb.setTrigger(IO.fallingEdge, function() { dingdong.send() });
    });
}

AJ.onDetach = function()
{
    pb.setTrigger(IO.disable);
}