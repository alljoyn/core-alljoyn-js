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

AJ.interfaceDefinition["org.alljoyn.Bus.test.bastress"] =
{
    cat:{ type:AJ.METHOD, args:["ss"], returns:["s"] },
};

AJ.objectDefinition['/sample'] = {
    interfaces:['org.alljoyn.Bus.test.bastress']
};

AJ.onAttach = function()
{
    print("Attached");
}

AJ.onDetach = function()
{
    print("Detached");
}

AJ.onMethodCall = function()
{
    if (this.member == 'cat') {
        print('Concat: ', arguments[0], ' and ', arguments[1]);
        this.reply(arguments[0] + arguments[1]);
    }
}

AJ.onPeerConnected = function(svc)
{
    print(JSON.stringify(svc));
    return true;
}