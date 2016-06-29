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

var service;

var credentials = {
    ecdsa: {
        prv_key: "-----BEGIN EC PRIVATE KEY-----MHcCAQEEIKat8BpfQX564DelHiY1N1vnYt7YajWpMbvTkQesspahoAoGCCqGSM49AwEHoUQDQgAE0gYtSZooJjisy6sTff2CfZex6P2pvTQeHa2iV5+H0dlK4IiSiVDq8KrbgtJ7ZXbd0pqKdMpvndU4wI7RLL//KQ==-----END EC PRIVATE KEY-----",
        cert_chain: "-----BEGIN CERTIFICATE-----MIIBgjCCASmgAwIBAgIJAJpCye0O/Y+gMAoGCCqGSM49BAMCMCQxIjAgBgNVBAoMGUFsbEpveW5UZXN0U2VsZlNpZ25lZE5hbWUwHhcNMTYwMzIyMDA0NjQ5WhcNMjkxMTI5MDA0NjQ5WjAgMR4wHAYDVQQKDBVBbGxKb3luVGVzdENsaWVudE5hbWUwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAATSBi1JmigmOKzLqxN9/YJ9l7Ho/am9NB4draJXn4fR2UrgiJKJUOrwqtuC0ntldt3Smop0ym+d1TjAjtEsv/8po0gwRjAVBgNVHSUEDjAMBgorBgEEAYLefAEBMAwGA1UdEwEB/wQCMAAwHwYDVR0jBBgwFoAUkQEQWrUzrnQhny8N9nL/9Bh4aa8wCgYIKoZIzj0EAwIDRwAwRAIgUFs5b6nz/h6d3rHZ8lVrUeIHbc+u1GSWguNuPm9TNDgCICNvsHZuawB5JDjqEyKO33PMg5Z213FeOYr1NZMee1GR-----END CERTIFICATE----------BEGIN CERTIFICATE-----MIIBlDCCATqgAwIBAgIJANac6neqRFiOMAoGCCqGSM49BAMCMCQxIjAgBgNVBAoMGUFsbEpveW5UZXN0U2VsZlNpZ25lZE5hbWUwHhcNMTYwMzIyMDA0NjQ5WhcNMjkxMTI5MDA0NjQ5WjAkMSIwIAYDVQQKDBlBbGxKb3luVGVzdFNlbGZTaWduZWROYW1lMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEAajtlbhkMUEokbISF/zCkatuizbCh0fa7AYRQD6q0xXvmyt44/bPlgeVsvdMAICBIx93O5lCE0aIEt1112HK+aNVMFMwIQYDVR0lBBowGAYKKwYBBAGC3nwBAQYKKwYBBAGC3nwBBTAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQWBBSRARBatTOudCGfLw32cv/0GHhprzAKBggqhkjOPQQDAgNIADBFAiEAxkCRXc6OorELrOKpYKB6h2HLqOPFRKZvSGGuSTtT4wQCIC2J/HuoDH1nohbO6CGZ2+ZANU6zqToDfVrM2Bh/Lgxd-----END CERTIFICATE-----"
    },
    expiration: 5000
};

function callPing(svc, n)
{
    var my_method = svc.method('my_ping').call("ping message " + n).onReply = function(arg) {
        print("Ping replied with ", arg);
        /*
         * Send another ping one second after we get a reply from the setProp call
         */
        svc.setProp('int_val', n).onReply = function(){ setTimeout(function(){ callPing(svc, n + 1); }, 500);}
    }

}

function onSecuredComplete(svc, success)
{
    if (success) {
        callPing(svc, 0);
    } else {
        alert("authentication failed");
    }
}

AJ.onAttach = function()
{
    print("AJ.onAttach");
    AJ.findSecureService('org.alljoyn.alljoyn_test', '/org/alljoyn/alljoyn_test', credentials, function(svc) {
        svc.enableSecurity(onSecuredComplete);
    });
}


AJ.onDetach = function()
{
    print("AJ.onDetach");
}

print("clientlite test program initialized\n");