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

AJ.interfaceDefinition['org.alljoyn.event_test'] =
{
    my_event:{ type:AJ.SIGNAL, description:'This is a test event' },
};

AJ.objectDefinition['/org/alljoyn/event_test'] = {
    interfaces:['org.alljoyn.event_test']
};

var sendInterval;

AJ.onAttach = function()
{
    print('Attached');
    sendInterval = setInterval(function() {
           var myEvent = AJ.signal('/org/alljoyn/event_test', 'my_event');
           myEvent.sessionless = true;
           myEvent.timeToLive = 30;
           myEvent.send();
        }     
        , 5000);
}

AJ.onDetach = function()
{
    clearInterval(sendInterval);
}

