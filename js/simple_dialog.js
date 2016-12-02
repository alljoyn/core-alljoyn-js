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
var a1 = c1.actionWidget("Press me");

var d1 = a1.dialogWidget("What do you want to do?");

d1.buttons = [
{ label:"ok", onClick: function() { print("ok"); } },
{ label:"cancel", onClick: function() { print("cancel"); } },
{ label:"apply", onClick: function() { print("apply"); } }
];

AJ.onAttach = function() { cp.load(); }