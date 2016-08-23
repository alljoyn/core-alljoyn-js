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

var cp = AJ.controlPanel();

function InitControlPanel(cp) {
    var c1 = cp.containerWidget(cp.VERTICAL);
    var l1 = c1.labelWidget("one");
    var l2 = c1.labelWidget("two");

    var p1 = c1.propertyWidget(cp.RADIO_BUTTON, 2);
    p1.choices = [ "cool", "warm", "hot" ];

    var p2 = c1.propertyWidget(cp.SLIDER, 50);
    p2.range = { min:0, max:100 };

    var a1 = c1.actionWidget("What Now?");
    var d1 = a1.dialogWidget("So do you want to do it?");
    d1.buttons = [
        { label:"ok", onClick: function() { print("ok"); } },
        { label:"cancel", onClick: function() { print("cancel"); } },
        { label:"apply", onClick: function() { print("apply"); } }
    ];

    c1.color={red:255,green:0,blue:128};
}

InitControlPanel(cp);

AJ.onAttach = function() { cp.load(); }
