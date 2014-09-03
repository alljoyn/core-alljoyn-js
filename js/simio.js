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

var led1=IO.digitalOut(IO.pin1);
var led2=IO.digitalOut(IO.pin2, 0);
var led3=IO.digitalOut(IO.pin3);
var led4=IO.digitalOut(IO.pin4, 0);

var in1=IO.digitalIn(IO.pin5, IO.pullUp);
var in2=IO.digitalIn(IO.pin6, IO.pullUp);
var in3=IO.digitalIn(IO.pin7, IO.pullUp);
var in4=IO.digitalIn(IO.pin8, IO.pullUp);

var pb1=IO.digitalIn(IO.pin9, IO.pullUp);
var pb2=IO.digitalIn(IO.pin10, IO.pullUp);
var pb3=IO.digitalIn(IO.pin11, IO.pullUp);
var pb4=IO.digitalIn(IO.pin12, IO.pullUp);

setInterval(function() {
    if (in1.level == 1) { led1.toggle() }
    if (in2.level == 1) { led2.toggle() }
    if (in3.level == 1) { led3.toggle() }
    if (in4.level == 1) { led4.pwm(Math.round(saw()*100)/100, 1)  }
}, 500);

pb1.setTrigger(IO.fallingEdge, function(){ print(pb1.pin.info.description); led1.toggle()});
pb2.setTrigger(IO.fallingEdge, function(){ print(pb2.pin.info.description); led2.toggle()});
pb3.setTrigger(IO.risingEdge, function(){ print(pb3.pin.info.description); led3.toggle()});
pb4.setTrigger(IO.risingEdge, function(){ print(pb4.pin.info.description); led4.toggle()});

var saw = sawTooth(1.0, 0.1);
function sawTooth(peak, step) {
    var i = peak-step;
    return function() {
        return Math.abs((i += step)%(2*peak)-peak)
    }
}

