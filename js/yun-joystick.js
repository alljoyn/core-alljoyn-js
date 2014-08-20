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

/* Map AllJoyn-JS IO pins to Arduino Yun nomenclature. */

var D0 = IO.pin1;
var D1 = IO.pin2;
var D2 = IO.pin3;
var D3 = IO.pin4;
var D4 = IO.pin5;
var D5 = IO.pin6;
var D6 = IO.pin7;
var D7 = IO.pin8;
var D8 = IO.pin9;
var D9 = IO.pin10;
var D10 = IO.pin11;
var D11 = IO.pin12;
var D12 = IO.pin13;
var D13 = IO.pin14;

var A0 = IO.pin15;
var A1 = IO.pin16;
var A2 = IO.pin17;
var A3 = IO.pin18;
var A4 = IO.pin19;
var A5 = IO.pin20;


/* Joystick pin mapping for Open Jumper Joystick shield */
var xAxis    = IO.analogIn(A1);
var yAxis    = IO.analogIn(A0);
var red      = IO.digitalIn(D3, IO.pullUp);
var green    = IO.digitalIn(D4, IO.pullUp);
var blue     = IO.digitalIn(D5, IO.pullUp);
var yellow   = IO.digitalIn(D6, IO.pullUp);
var joystick = IO.digitalIn(D7, IO.pullUp);
var pin13LED = IO.digitalOut(D13);

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
red.setTrigger(IO.fallingEdge, function(){ print("RED button down"); pin13LED.toggle(); });

//green.setTrigger(IO.risingEdge, function(){ print("GREEN button up"); });
green.setTrigger(IO.fallingEdge, function(){ print("GREEN button down"); });

//blue.setTrigger(IO.risingEdge, function(){ print("BLUE button up"); });
blue.setTrigger(IO.fallingEdge, function(){ print("BLUE button down"); });

//yellow.setTrigger(IO.risingEdge, function(){ print("YELLOW button up"); });
yellow.setTrigger(IO.fallingEdge, function(){ print("YELLOW button down"); });

//joystick.setTrigger(IO.risingEdge, function(){ print("JOYSTICK button up"); });
joystick.setTrigger(IO.fallingEdge, function(){ print("JOYSTICK button down"); });

setInterval(readJoystick, 1000);
