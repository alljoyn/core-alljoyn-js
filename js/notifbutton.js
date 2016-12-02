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

var pbA=IO.digitalIn(IO.pin[8], IO.pullDown);
var pbB=IO.digitalIn(IO.pin[9], IO.pullDown);

AJ.onAttach = function()
{
    print('attached');
    pbA.setTrigger(IO.fallingEdge, function(){ AJ.notification(AJ.notification.Warning, "Button A pressed").send(200); })
    pbB.setTrigger(IO.fallingEdge, function(){ AJ.notification(AJ.notification.Emergency, "Button B pressed").send(200); })
}

AJ.onDetach = function()
{
    pbA.setTrigger(IO.disable);
    pbB.setTrigger(IO.disable);
}