
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

var container = cp.containerWidget();

var time = container.propertyWidget(cp.TIME_PICKER);
var date = container.propertyWidget(cp.DATE_PICKER);

time.onValueChanged=function(newTime) { this.value=newTime; }
date.onValueChanged=function(newDate) { this.value=newDate; }

cp.load();