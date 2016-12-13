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
 