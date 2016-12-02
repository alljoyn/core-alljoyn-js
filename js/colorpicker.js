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

var c1 = cp.containerWidget();

var p1 = c1.propertyWidget(cp.RADIO_BUTTON);
p1.label = "Color picker";
p1.choices = [ "red", "orange", "yellow", "green", "blue", "indigo", "violet", "white", "black" ];

cp.load();