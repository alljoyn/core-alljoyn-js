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
