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

AJ.aboutDefinition = {
    DeviceName: "AllJoyn.js",
    AppName: "MyPing",
    Description: "Custom About Info in AllJoyn.js",
    Manufacturer: "AllseenAlliance",
    ModelNumber: "12",
    DateOfManufacture: "2016-06-04",
    SoftwareVersion: "0.0.1",
    SupportUrl: "https://allseenalliance.org/framework/documentation/develop/run-sample-apps/alljoyn-js",
};

AJ.interfaceDefinition['org.alljoyn.alljoyn_ping'] =
{
    my_ping:{ type:AJ.METHOD, args:[{inStr:'s'}], returns:[{outStr:'s'}] }
};

AJ.objectDefinition['/org/alljoyn/alljoyn_announced'] = {
    interfaces:['org.alljoyn.alljoyn_ping'],
    flags: [AJ.ANNOUCE]
};

AJ.objectDefinition['/org/alljoyn/alljoyn_hidden'] = {
    interfaces:['org.alljoyn.alljoyn_ping'],
    flags: [AJ.HIDDEN]
};

AJ.onAttach = function()
{
    print('Attached');
}

print('about test program initialized\n');