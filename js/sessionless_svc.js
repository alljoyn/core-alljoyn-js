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

AJ.onAttach = function()
{
    AJ.addMatch('org.alljoyn.sessionless', 'my_signal')
    print('Attached');
}

AJ.onSignal = function() {
    if (this.member == 'my_signal') {
        print('SLS signal received');
    }
}

print('SLS service test program initialized\n');