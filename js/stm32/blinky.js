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

function blinky() {
    var led0 = IO.digitalOut(IO.pin[12], 1);
    var led1 = IO.digitalOut(IO.pin[13], 0);
    var led2 = IO.digitalOut(IO.pin[14], 0);
    var led3 = IO.digitalOut(IO.pin[15], 0);
    var led4 = IO.digitalOut(IO.pin[16], 1);

    return function() { led0.toggle(); 
                        led1.toggle();
                        led2.toggle(); 
                        led3.toggle();
                        led4.toggle();}
}

var t = setInterval(blinky(), 200);