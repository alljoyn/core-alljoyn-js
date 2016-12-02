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

var d1 = cp.dialogWidget("What do you want to do?", "Decision");

d1.buttons = [
{ label:"ok", onClick: function() { print("ok"); } },
{ label:"cancel", onClick: function() { print("cancel"); } },
{ label:"apply", onClick: function() { print("apply"); } }
];

var notif = AJ.notification(AJ.notification.Emergency, "I've fallen and I can't get up!");
print("controlPanelPath " + d1.path);
notif.controlPanelPath = d1.path;

AJ.onAttach = function() { cp.load(); setTimeout(function() {notif.send(100)}, 5000) }
