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

AJ.interfaceDefinition['org.alljoyn.action_test'] =
{
    my_action:{ type:AJ.METHOD, description:'This is a test action' },
};

AJ.objectDefinition['/org/alljoyn/action_test'] = {
    interfaces:['org.alljoyn.action_test']
};

AJ.onMethodCall = function(arg) {
    if (this.member == 'my_action') {
        print('Action received');
    }
}

AJ.onAttach = function()
{
    print('Attached');
}
