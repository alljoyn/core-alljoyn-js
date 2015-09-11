/******************************************************************************
 *  * Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
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
//Include the IO library for HW access
var IO = require('IO');
//Include the AllJoyn.js library
var AJ = require('AllJoyn');
//Force the device name
AJ.store("DeviceName", "SpeakerLight");
AJ.store("Manufacturer", "QCE");
AJ.store("Description", "Speaker & light board for AllJoyn.js demos");

//Create interface for remote interaction
AJ.interfaceDefinition['org.example.speaker'] =
{
    playing: { type:AJ.SIGNAL, description:'audio is playing' },              
    playbackDone: { type:AJ.SIGNAL, description: 'audio is done playing' },
    frequency: { type:AJ.PROPERTY, signature:'u'},
    play: { type:AJ.METHOD, description:'play audio out of the speaker' },
    alwaysPlaying: { type:AJ.PROPERTY, signature:'b'}
};
AJ.interfaceDefinition['org.example.simplelight'] =
{
    redDuty: { type:AJ.PROPERTY, signature:'u'},
    greenDuty: { type:AJ.PROPERTY, signature:'u'},
    blink: { type:AJ.METHOD, description:'blink the led' }
};

//Add the interface to an object path
AJ.objectDefinition['/speaker'] = {
    interfaces:['org.example.speaker']
};

AJ.objectDefinition['/light'] = {
    interfaces:['org.example.simplelight']
};

//Setup the handler for methods
AJ.onMethodCall = function(arg) {
    print("Received a method call");
    if (this.member == 'play' && this.iface == 'org.example.speaker' && this.path == '/speaker') {
        play();
    } else if (this.member == 'blink') {
        blink();
    }
}

// Properties set
AJ.onPropSet = function(iface, prop, value)
{
    print("Set property called "+iface+", "+prop+", "+value);
    if (iface == 'org.example.speaker' && prop == 'frequency') {
        frequency = value;
        this.reply();
    } else if (iface == 'org.example.speaker' && prop == 'alwaysPlaying') {
        keepPlayingAudio = value;
        if (keepPlayingAudio) { play(); }
        this.reply();
    } else if (iface == 'org.example.simplelight' && prop == 'redDuty') {
        redLed.pwm(value/100, 100000);
        this.reply();
    } else {
        throw('rejected');
    }
}

// Properties Get
AJ.onPropGet = function(iface, prop)
{
    print("Get property called "+iface+", "+prop);
    if (iface == 'org.example.speaker' && prop == 'frequency') {
        this.reply(frequency);
    } else if (iface == 'org.example.speaker' && prop == 'alwaysPlaying') {
        this.reply(keepPlayingAudio);
    } else if (iface == 'org.example.simplelight' && prop == 'redDuty') {
        this.reply(redLed.value);
    } else {
        throw('rejected');
    }
}

// SET UP I/O's for spkr & LED's
//Set our speaker to be ouput on GPIO13 (digitalOut makes this a PWM)
var speaker = IO.digitalOut(IO.pin[13]);
//Set our light to be a pwm on GPIO 6,7 & 8
var redLed = IO.digitalOut(IO.pin[6]);
var greenLed = IO.digitalOut(IO.pin[7]);
//var blueLed = IO.digitalOut(IO.pin[8]);
redLed.level = 0;
greenLed.level = 0;
//blueLed.level = 0;

//Variable used to allow console to dynamically change values
//Frequency used for audio playback
var frequency = 100000;
//Message to be sent in a notifications.
var notifMsg = 'Speaker is playing audio.';
//Number of times to blink on/off
var NUM_TIMES_TO_BLINK = 6;
//Variable to keep the speaker playing
var keepPlayingAudio = false;

//Create a control panel
var cp = AJ.controlPanel();
//Specify the container that can hold vertical and horizontal containers/widgets
var c1 = cp.containerWidget(cp.VERTICAL, cp.HORIZONTAL);
//Create a button to test playback
var playButton = c1.actionWidget('Play');
var label = c1.labelWidget('Settings:');
//Create a switch to turn on/off AllJoyn Event sending
var eventToggle = c1.propertyWidget(cp.SWITCH, false, 'Send AllJoyn Event on Playback');
//Create a switch to turn on/off AllJoyn Notification sending
var notifToggle = c1.propertyWidget(cp.SWITCH, false, 'Send AllJoyn Notification on Playback');
//Create our light controls
label = c1.labelWidget('Light color configuration');
//Create the RGB Sliders and set the range
var redDuty = c1.propertyWidget(cp.SLIDER, 0.5, 'Red Duty Cycle:');
redDuty.range = {min:0, max:100, increment:1, units:'%'};
var greenDuty = c1.propertyWidget(cp.SLIDER, 0.5, 'Green Duty Cycle:');
greenDuty.range = {min:0, max:100, increment:1, units:'%'};

//Set the click to call the play() method
playButton.onClick = function(val) { play(); }
//Handle the color sliders changes
redDuty.onValueChanged = function(val) { redLed.pwm(val/100, 100000); 
    print("red slider @" + val); }
    greenDuty.onValueChanged = function(val) { greenLed.pwm(val/100, 100000);
        print("green slider @" + val); }
        //blueDuty.onValueChanged = function(val) { blueLed.pwm(val/100, 100000); }

function play()
{
    blink();

    var duty = 0.1;
    //create an interval that will end up running 10 times with 100ms wait
    //Audio will play for 1 second
    var i = setInterval(function() {
        if (duty == 0.1) {
            sendEvent({playing:'org.example.speaker'});
            sendNotification();
        }
        if (duty >= 1) {
            if (keepPlayingAudio) {
                duty = 0.1;
            } else {
                //set the pwm back to 0
                speaker.pwm(0, frequency);

                //remove this interval
                clearInterval(i);
                sendEvent({playbackDone:'org.example.speaker'});
                return;
            }
        }
        //send a pulse at 'duty' @ frequency
        speaker.pwm(keepPlayingAudio ? .1 : duty, frequency);
        duty += 0.1;
    }, 100);
}

function blink() {
    var numTimes = 0;
    var lightOn = false;
    var i = setInterval(function() {
        redLed.pwm(lightOn ? 1 : 0,100000);
        lightOn = !lightOn;
        numTimes++;
        if (numTimes > NUM_TIMES_TO_BLINK * 2) {//so we always end up with LED off
            clearInterval(i);
            // set LED's back to slider values
            greenLed.pwm(greenDuty.value/100, 100000);
            redLed.pwm(redDuty.value/100, 100000);
        }    
    }, 100);
}

function sendEvent(whatEvent) {
    if (eventToggle.value) { //If we are checked for this feature then send
        var sig = AJ.signal('/speaker', whatEvent);
        //This makes it as a true Event so a session is not required
        sig.sessionless = true;
        sig.timeToLive = 60; //60 seconds
        sig.send(); 
    }
}

function sendNotification() {
    if (notifToggle.value) { //If we are checked for this feature then send
        AJ.notification(AJ.notification.Info, notifMsg).send(30); //30 seconds
    }
}

//When we have found an AllJoyn Router finalize the Control Panel
AJ.onAttach = function()
{
    greenLed.pwm(0.7, 100000);
    redLed.pwm(0, 100000);
    //Finalize the Control Panel
    cp.load();
}

redLed.pwm(0.5, 100000);
setInterval(function() { print('.'); }, 1000);
