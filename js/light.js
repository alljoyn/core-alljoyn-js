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
var IO = require('IO');

/* 
 * Configure light sensor ADC
 */
var pin = IO.digitalOut(IO.pin[10], 1)
var lightSensor = IO.analogIn(IO.pin[11]);

function lightCheck()
{
    var level = lightSensor.value;
    print("Light level=", level);
    if (level > 10000 && this.bright != true) {
        this.bright = true;
        this.dark = false;
        AJ.notification(AJ.notification.Info, "Light is too bright " + level).send(100);
    } else if (level < 1000 && this.dark != true) {
        this.bright = false;
        this.dark = true;
        AJ.notification(AJ.notification.Emergency, "Help mommy it's dark").send(100);
    }
}

setInterval(lightCheck, 1000);