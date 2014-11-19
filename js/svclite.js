/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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
require("AJ");

AJ.interfaceDefinition['org.alljoyn.alljoyn_test'] =
{
    my_ping:{ type:AJ.METHOD, args:[{inStr:'s'}], returns:[{outStr:'s'}] },
};

AJ.interfaceDefinition['org.alljoyn.alljoyn_test.values'] =
{
    int_val:{ type:AJ.PROPERTY, signature:'i' },
    str_val:{ type:AJ.PROPERTY, signature:'s' },
    ro_val:{ type:AJ.PROPERTY, signature:'s', access:'R' }
};

AJ.objectDefinition['/org/alljoyn/alljoyn_test'] = {
    interfaces:['org.alljoyn.alljoyn_test', 'org.alljoyn.alljoyn_test.values']
};

properties = {
    int_val:123,
    str_val:'hello',
    ro_val:'a read only value'
};

AJ.onAttach = function()
{
    print('Attached');
}

AJ.onPropSet = function(iface, prop, value)
{
    if (iface == 'org.alljoyn.alljoyn_test.values') {
        properties[prop] = value;
        this.reply();
    } else {
        throw('rejected');
    }
}

AJ.onPropGet = function(iface, prop)
{
    if (iface == 'org.alljoyn.alljoyn_test.values') {
        this.reply(properties[prop]);
    } else {
        throw('rejected');
    }
}

AJ.onMethodCall = function(arg)
{
    if (this.member == 'my_ping') {
        print('my_ping ', arg);
        this.reply(arg);
    } else {
        throw('rejected');
    }
}

print('svclite test program initialized\n');
