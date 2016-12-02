
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
var b1 = c1.actionWidget("Button 1");
var b2 = c1.actionWidget("Button 2");

b1.onClick = function() { print("Button 1 pressed") }
b2.onClick = function() { print("Button 2 pressed") }

AJ.onAttach = function() { cp.load(); }