/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
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
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
******************************************************************************/
var AJ = require('AllJoyn');

AJ.interfaceDefinition["org.alljoyn.alljoyn_test"] =
{
    my_ping:{ type:AJ.METHOD, args:[{inStr:"s"}], returns:[{outStr:"s"}] }
};

AJ.interfaceDefinition["org.alljoyn.alljoyn_test.values"] =
{
    int_val:{ type:AJ.PROPERTY, signature:"i" },
    str_val:{ type:AJ.PROPERTY, signature:"s" },
    ro_val:{ type:AJ.PROPERTY, signature:"s", access:"R" }
};

function callPing(svc, n)
{
    svc.method('my_ping').call("ping message " + n).onReply = function(arg) {
        print("Ping replied with ", arg);
        /*
         * Send another ping one second after we get a reply from the setProp call
         */
        svc.setProp('int_val', n).onReply = function(){ setTimeout(function(){ callPing(svc, n + 1); }, 5);}
    }
}

AJ.onAttach = function()
{
    print("AJ.onAttach");
    AJ.findService('org.alljoyn.alljoyn_test', function(svc) { callPing(svc, 0); });
}

AJ.onDetach = function()
{
    print("AJ.onDetach");
}

print("clientlite test program initialized\n");