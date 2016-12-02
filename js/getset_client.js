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

AJ.interfaceDefinition["org.alljoyn.alljoyn_test.values"] =
{
    int_val:{ type:AJ.PROPERTY, signature:"i" },
    str_val:{ type:AJ.PROPERTY, signature:"s" },
    ro_val:{ type:AJ.PROPERTY, signature:"s", access:"R" }
};

function startGetSet(svc, n)
{
    var int_val = svc.getProp('int_val');
    var str_val = svc.getProp('str_val');
    var ro_val = svc.getProp('ro_val');
    print('int: ' + int_val + ' str: ' + str_val + ' ro: ' + ro_val);
    svc.getAllProps('org.alljoyn.alljoyn_test.values').onReply = function() {
        print(JSON.stringify(arguments));
        svc.setProp('int_val', n).onReply = function() { setTimeout(function() { startGetSet(svc, n + 1); }, 100)}
    }
}

AJ.onAttach = function()
{
    print("AJ.onAttach");
    AJ.findService('org.alljoyn.alljoyn_test.values', function(svc) { startGetSet(svc, 0); });
}

AJ.onDetach = function()
{
    print("AJ.onDetach");
}

print("getset client test program initialized\n");