/******************************************************************************
 *  *    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
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