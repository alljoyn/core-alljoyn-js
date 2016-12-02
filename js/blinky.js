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
var IO = require('IO');

/*
 *Toggle the LED on pin 'PinID'
 */
function blinky(pinID) {
    var led = IO.digitalOut(IO.pin[pinID]);
    return function() { led.toggle(); }
}


/*
 * On Linino the Red LED marked L13 is mapped to pin 13
 *
 * The call below will make that LED blink once a second.
 */
var t = setInterval(blinky(13), 1000);

/*
 * to clear the interval (i.e. stop the blinking)
 *    clearInterval(t);
 */