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

/*
 * Put name extension here. This should be the same as the command
 * line arguments fed to the SCL chat sample (./chat -s <your name>)
 */
var nameExt = "name";

AJ.interfaceDefinition["org.alljoyn.bus.samples.chat"] = { Chat:{ type:AJ.SIGNAL, args:["s"] }, };

AJ.objectDefinition['/chatService'] = { interfaces:['org.alljoyn.bus.samples.chat'] };

var cp = AJ.controlPanel();
var cw = cp.containerWidget(cp.VERTICAL, cp.HORIZONTAL);
var edit_text = cw.propertyWidget(cp.EDIT_TEXT, "", "Chat:");
var text_label = cw.propertyWidget(cp.TEXT_VIEW, "Message:");
/*
 * Need to hold a reference to the service otherwise once found()
 * returns you will lose the service object and no longer receive
 * any signals/method calls from that service.
 */
var service;

edit_text.onValueChanged = function (val) {
   var chat_sig = service.signal('/chatService', {Chat:'org.alljoyn.bus.samples.chat'});
   chat_sig.send(val);
}

function found(svc)
{
    print('Found service');
    service = svc;
}

var nameObject = {
    interfaces:['org.alljoyn.bus.samples.chat'],
    path:'/chatService',
    port:27
}

AJ.onSignal = function()
{
    /*
     * Print out the chat message received
     */
    if (this.member == 'Chat') {
        print(arguments[0]);
        text_label.value = arguments[0];
    }
}

AJ.onAttach = function()
{
    cp.load();
    AJ.addMatch('org.alljoyn.bus.samples.chat', 'Chat');
    /*
     * Legacy API used to find a service by name, rather than using About
     */
    AJ.findServiceByName("org.alljoyn.bus.samples.chat." + nameExt, nameObject, found); 
}

AJ.onDetach = function()
{ 
    print("AJ.onDetach");
}