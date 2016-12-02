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

var pbA=IO.digitalIn(IO.pin[6], IO.pullDown);

AJ.onAttach = function()
{
    print('attached');
    pbA.setTrigger(IO.fallingEdge, function(){ AJ.notification(AJ.notification.Warning, "Button A pressed").send(200); })
}

AJ.onDetach = function()
{
    pbA.setTrigger(IO.disable);
}