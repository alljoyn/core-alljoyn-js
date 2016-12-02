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

AJ.interfaceDefinition["org.alljoyn.sessionless"] =
{
    my_signal:{ type:AJ.SIGNAL, args:[{inStr:"s"}], returns:[{outStr:"s"}] }
};

AJ.objectDefinition['/org/alljoyn/sessionless'] = {
    interfaces:['org.alljoyn.sessionless']
};

function sendSignal()
{
    var sig = AJ.signal('/org/alljoyn/sessionless', {my_signal:'org.alljoyn.sessionless'});
    sig.sessionless = true;
    sig.timeToLive = 100;
    sig.send("signal");
}

var interval;

AJ.onAttach = function()
{
    interval = setInterval(sendSignal, 100);
}

AJ.onDetach = function()
{
    clearInterval(interval);
    print("AJ.onDetach");
}

print("SLS client test program initialized\n");