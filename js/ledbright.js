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

var cp = AJ.controlPanel();

var c1 = cp.containerWidget(cp.VERTICAL, cp.HORIZONTAL);
var rate = c1.propertyWidget(cp.SLIDER, 50, "Brightness:");
rate.range = { min:0, max:100, increment:1 };

var led = IO.digitalOut(IO.pin[0]);

rate.onValueChanged = function(val) { led.pwm((100 - val) / 100, 100); }

AJ.onAttach = function() { cp.load(); }