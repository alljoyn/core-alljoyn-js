/******************************************************************************
 * Copyright (c) 2016 Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright 2016 Open Connectivity Foundation and Contributors to
 *    AllSeen Alliance. All rights reserved.
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
var IO = require('IO');

/* Map AllJoyn-JS IO pins to Arduino Yun nomenclature. */

var D0 = IO.pin[0];
var D1 = IO.pin[1];
var D2 = IO.pin[2];
var D3 = IO.pin[3];
var D4 = IO.pin[4];
var D5 = IO.pin[5];
var D6 = IO.pin[6];
var D7 = IO.pin[7];
var D8 = IO.pin[8];
var D9 = IO.pin[9];
var D10 = IO.pin[10];
var D11 = IO.pin[11];
var D12 = IO.pin[12];
var D13 = IO.pin[13];

var A0 = IO.pin[14];
var A1 = IO.pin[15];
var A2 = IO.pin[16];
var A3 = IO.pin[17];
var A4 = IO.pin[18];
var A5 = IO.pin[19];


/* Joystick pin mapping for Open Jumper Joystick shield */
var xAxis    = IO.analogIn(A1);
var yAxis    = IO.analogIn(A0);
var red      = IO.digitalIn(D3, IO.pullUp);
var green    = IO.digitalIn(D4, IO.pullUp);
var blue     = IO.digitalIn(D5, IO.pullUp);
var yellow   = IO.digitalIn(D6, IO.pullUp);
var joystick = IO.digitalIn(D7, IO.pullUp);
var pin[12]LED = IO.digitalOut(D13);

function readJoystick()
{
    var redStr = "";
    var greenStr = "";
    var blueStr = "";
    var yellowStr = "";
    var jsStr = "";

    var rb = red.level;
    var gb = green.level;
    var bb = blue.level;
    var yb = yellow.level;
    var jsb = joystick.level;

    var xp = xAxis.value;
    var yp = yAxis.value;

    if (rb == 0) {
        redStr = " RED";
    }

    if (gb == 0) {
        greenStr = " GREEN";
    }

    if (bb == 0) {
        blueStr = " BLUE";
    }

    if (yb == 0) {
        yellowStr = " YELLOW";
    }

    if (jsb == 0) {
        jsStr = " JOYSTICK";
    }

    print("x = " + xp + "   y = " + yp + redStr + greenStr + blueStr + yellowStr + jsStr);
}

//red.setTrigger(IO.risingEdge, function(){ print("RED button up"); });
red.setTrigger(IO.fallingEdge, function(){ print("RED button down"); pin[12]LED.toggle(); });

//green.setTrigger(IO.risingEdge, function(){ print("GREEN button up"); });
green.setTrigger(IO.fallingEdge, function(){ print("GREEN button down"); });

//blue.setTrigger(IO.risingEdge, function(){ print("BLUE button up"); });
blue.setTrigger(IO.fallingEdge, function(){ print("BLUE button down"); });

//yellow.setTrigger(IO.risingEdge, function(){ print("YELLOW button up"); });
yellow.setTrigger(IO.fallingEdge, function(){ print("YELLOW button down"); });

//joystick.setTrigger(IO.risingEdge, function(){ print("JOYSTICK button up"); });
joystick.setTrigger(IO.fallingEdge, function(){ print("JOYSTICK button down"); });

setInterval(readJoystick, 1000);