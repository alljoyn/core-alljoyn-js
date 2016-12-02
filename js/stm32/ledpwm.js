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
 
var cp = AJ.controlPanel(); 
var c1 = cp.containerWidget(cp.VERTICAL, cp.HORIZONTAL); 

var freq = c1.propertyWidget(cp.SLIDER, 10000, "Frequency:");
var duty = c1.propertyWidget(cp.SLIDER, 50, "Duty Cycle:");

freq.range = {min:100, max:100000, increment:100, units:"Hz"};
duty.range = {min:1, max:100, increment:5, units:"%"};

var led = IO.digitalOut(IO.pin[16]);

var bright = led.pwm(duty.value / 100, freq.value);

freq.onValueChanged = function(val) { led.pwm(duty.value / 100, val);}
duty.onValueChanged = function(val) { led.pwm(val / 100, freq.value); }

AJ.onAttach = function() { cp.load();} 
 