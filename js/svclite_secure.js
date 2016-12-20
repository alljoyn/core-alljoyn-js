/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
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

AJ.securityDefinition = {
    ecdsa: {
        prv_key: "-----BEGIN EC PRIVATE KEY-----MDECAQEEICCRJMbxSiWUqj4Zs7jFQRXDJdBRPWX6fIVqE1BaXd08oAoGCCqGSM49AwEH-----END EC PRIVATE KEY-----",
        cert_chain: "-----BEGIN CERTIFICATE-----MIIBuDCCAV2gAwIBAgIHMTAxMDEwMTAKBggqhkjOPQQDAjBCMRUwEwYDVQQLDAxvcmdhbml6YXRpb24xKTAnBgNVBAMMIDgxM2FkZDFmMWNiOTljZTk2ZmY5MTVmNTVkMzQ4MjA2MB4XDTE1MDcyMjIxMDYxNFoXDTE2MDcyMTIxMDYxNFowQjEVMBMGA1UECwwMb3JnYW5pemF0aW9uMSkwJwYDVQQDDCAzOWIxZGNmMjBmZDJlNTNiZGYzMDU3NzMzMjBlY2RjMzBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABGJ/9F4xHn3Klw7z6LREmHJgzu8yJ4i09b4EWX6a5MgUpQoGKJcjWgYGWb86bzbciMCFpmKzfZ42Hg+kBJs2ZWajPjA8MAwGA1UdEwQFMAMBAf8wFQYDVR0lBA4wDAYKKwYBBAGC3nwBATAVBgNVHSMEDjAMoAoECELxjRK/fVhaMAoGCCqGSM49BAMCA0kAMEYCIQDixoulcO7Sdf6Iz6lvt2CDy0sjt/bfuYVW3GeMLNK1LAIhALNklms9SP8ZmTkhCKdpC+/fuwn0+7RX8CMop11eWCih-----END CERTIFICATE-----"
    },
    expiration: 5000
};

AJ.interfaceDefinition['org.alljoyn.alljoyn_test'] =
{
    my_ping:{ type:AJ.METHOD, args:[{inStr:'s'}], returns:[{outStr:'s'}] },
    my_signal:{ type:AJ.SIGNAL, args:["a{ys}"]}
};

AJ.interfaceDefinition['org.alljoyn.alljoyn_test.values'] =
{
    int_val:{ type:AJ.PROPERTY, signature:'i' },
    str_val:{ type:AJ.PROPERTY, signature:'s' },
    ro_val:{ type:AJ.PROPERTY, signature:'s', access:'R' }
};

AJ.objectDefinition['/org/alljoyn/alljoyn_test'] = {
    interfaces:['org.alljoyn.alljoyn_test', 'org.alljoyn.alljoyn_test.values'],
    flags: [AJ.SECURE]
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

AJ.onPeerConnected = function(peer)
{
    peer.authenticate('/org/alljoyn/alljoyn_test');
    return true;
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
    print(this.member);
    if (this.member == 'my_ping') {
        print('my_ping ', arg);
        this.reply(arg);
    } else {
        throw('rejected');
    }
}

AJ.onSignal = function() {
    if (this.member == 'my_signal') {
        print('my_signal received');
    }
}

print('svclite test program initialized\n');