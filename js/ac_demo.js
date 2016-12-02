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

var root = cp.containerWidget(cp.VERTICAL, cp.HORIZONTAL);

// The temperature & humidity settings
var comfort = root.containerWidget(cp.VERTICAL);
var temperature = comfort.propertyWidget(cp.NUMERIC_VIEW, 65, "Current Temperature:");
var humidity = comfort.propertyWidget(cp.NUMERIC_VIEW, 50, "Current Humidity:");

// The controls
var controls = root.containerWidget(cp.HORIZONTAL);

var acMode = controls.propertyWidget(cp.SPINNER, 0, "Mode");
acMode.choices = [ "Auto", "Cool", "Heat", "Fan", "Off" ];
acMode.onValueChanged = function(mode) { tempSet.writeable = (mode <= 2); fanSet.writeable = (mode != 4); }

var fanSet = controls.propertyWidget(cp.SPINNER, 1, "Fan Speed:");
fanSet.choices = [ "Low", "Medium", "High" ];

var acStatus = controls.propertyWidget(cp.TEXT_VIEW, "Ready", "Status:");

function sign(x) { return x > 0 ? 1 : (x < 0 ? -1 : 0); }

function updateTargetTemp(t) {
    var current = temperature.value;

    // check for heating or cooling
    if ((t > current && acMode.value == 1) || (t < current && acMode.value == 2)) {
        tempSet.value = current;
        return;
    }
    // t is null if we were called from the timeout callback
    if (t != null) {
        this.target = t;
        acStatus.value = (t < current) ? "Cooling" : "Heating";
        if (this.to != undefined) {
            clearTimeout(this.to);
        }
    } else {
        current += sign(this.target - current);
        temperature.value = current;
    }
    this.to = undefined;
    if (current != this.target) {
        var me = this;
        this.to = setTimeout(function() { updateTargetTemp.apply(me, null); }, 900 - 300 * fanSet.value);
    } else {
        acStatus.value = "Ready";
    }
}

var tempSet = root.propertyWidget(cp.SLIDER, temperature.value, "Temperature:");
tempSet.range = { min:50, max:90, increment:1, units:"Degrees" };
tempSet.onValueChanged = updateTargetTemp;

AJ.translations = {
    en: { },
    fr: { "Current Temperature:":"Température Ambiante:",
          "Current Humidity:":"Humidité:",
          "Temperature:":"Température Réglée:",
          "Fan Speed:":"Ventilateur Réglée",
          "Low":"Faible",
          "Medium":"Mediun",
          "High":"Haut",
          "Status":"Statut" }
};

AJ.onAttach = function() { cp.load(); }