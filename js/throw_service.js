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

AJ.interfaceDefinition['org.alljoyn.alljoyn_throw'] =
{
    my_ping:{ type:AJ.METHOD, args:[{inStr:'s'}], returns:[{outStr:'s'}] },
    my_signal:{ type:AJ.SIGNAL, args:["s"]},
    int_val:{ type:AJ.PROPERTY, signature:'i' }
};

AJ.objectDefinition['/org/alljoyn/alljoyn_throw'] = {
    interfaces:['org.alljoyn.alljoyn_throw']
};

AJ.onAttach = function()
{
    print('Attached');
}

AJ.onMethodCall = function(arg)
{
    if (this.member == 'my_ping') {
        throw('Throwing error on method');
    }
}

AJ.onSignal = function() {
    if (this.member == 'my_signal') {
        throw('Throwing error on signal');
    }
}

AJ.onPropSet = function(iface, prop, value)
{
    if (iface == 'org.alljoyn.alljoyn_throw') {
        throw('Throwing error on prop set');
    }
}

AJ.onPropGet = function(iface, prop)
{
    if (iface == 'org.alljoyn.alljoyn_throw') {
        throw('Throwing error on prop get');
    }
}

print('Throw service test program initialized\n');