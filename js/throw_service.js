/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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
