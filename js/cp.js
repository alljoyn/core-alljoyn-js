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
