/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

AJ.interfaceDefinition["org.allseen.LSF.LampService"] = {
    Version:{type:AJ.PROPERTY, signature: "u", access: "R"},
    LampServiceVersion:{type:AJ.PROPERTY, signature: "u", access: "R"},
    ClearLampFault:{type:AJ.METHOD, args:[{LampFaultCode:"u"}], returns:[{LampResponseCode:"u"}, {LampFaultCode:"u"}]},
    LampFaults:{type:AJ.PROPERTY, signature:"au", access:"R"}  
}

AJ.interfaceDefinition["org.allseen.LSF.LampParameters"] = {
    Version:{type:AJ.PROPERTY, signature:"u", access:"R"},
    Energy_Usage_Milliwatts:{type:AJ.PROPERTY, signature:"u", access:"R"},
    Brightness_Lumens:{type:AJ.PROPERTY, signature:"u", access:"R"}
}

AJ.interfaceDefinition["org.allseen.LSF.LampDetails"] = {
    Version:{type:AJ.PROPERTY, signature:"u", access:"R"},
    Make:{type:AJ.PROPERTY, signature:"u", access:"R"},
    Model:{type:AJ.PROPERTY, signature:"u", access:"R"},
    Type:{type:AJ.PROPERTY, signature:"u", access:"R"},
    LampType:{type:AJ.PROPERTY, signature:"u", access:"R"},
    LampBaseType:{type:AJ.PROPERTY, signature:"u", access:"R"},
    LampBeamAngle:{type:AJ.PROPERTY, signature:"u", access:"R"},
    Dimmable:{type:AJ.PROPERTY, signature:"b", access:"R"},
    Color:{type:AJ.PROPERTY, signature:"b", access:"R"},
    VariableColorTemp:{type:AJ.PROPERTY, signature:"b", access:"R"},
    HasEffects:{type:AJ.PROPERTY, signature:"b", access:"R"},
    MinVoltage:{type:AJ.PROPERTY, signature:"u", access:"R"},
    MaxVoltage:{type:AJ.PROPERTY, signature:"u", access:"R"},
    Wattage:{type:AJ.PROPERTY, signature:"u", access:"R"},
    IncandescentEquivalent:{type:AJ.PROPERTY, signature:"u", access:"R"},
    MaxLumens:{type:AJ.PROPERTY, signature:"u", access:"R"},
    MinTemperature:{type:AJ.PROPERTY, signature:"u", access:"R"},
    MaxTemperature:{type:AJ.PROPERTY, signature:"u", access:"R"},
    ColorRenderingIndex:{type:AJ.PROPERTY, signature:"u", access:"R"},
    LampID:{type:AJ.PROPERTY, signature:"s", access:"R"}
}

AJ.interfaceDefinition["org.allseen.LSF.LampState"] = {
    Version:{type:AJ.PROPERTY, signature:"u", access:"R"},
    TransitionLampState:{type:AJ.METHOD, args:[{Timestamp:"t"}, {NewState:"a{sv}"}, {TransitionPeriod:"u"}], returns:[{LampResponseCode:"u"}]},
    ApplyPulseEffect:{type:AJ.METHOD, args:[{FromState:"a{sv}"}, {ToState:"a{sv}"}, {period:"u"}, {duration:"u"}, {numPulses:"u"}, {timestamp:"t"}], returns:[{LampResponseCode:"u"}]},
    LampStateChanged:{type:AJ.SIGNAL, args:[{LampID:"s"}]},
    OnOff:{type:AJ.PROPERTY, signature:"b", access:"RW"},
    Hue:{type:AJ.PROPERTY, signature:"u", access:"RW"},
    Saturation:{type:AJ.PROPERTY, signature:"u", access:"RW"},
    ColorTemp:{type:AJ.PROPERTY, signature:"u", access:"RW"},
    Brightness:{type:AJ.PROPERTY, signature:"u", access:"RW"}
}

AJ.objectDefinition["/org/allseen/LSF/Lamp"] = {
    interfaces:["org.allseen.LSF.LampService", "org.allseen.LSF.LampParameters", "org.allseen.LSF.LampDetails", "org.allseen.LSF.LampState"],
    properties:["LampServiceProperties", "LampParametersProperties", "LampDetailsProperties", "LampStateProperties"]
};

LampServiceProperties = {
    Version:1,
    LampServiceVersion:1,
    LampFaults:[0, 1]
};

LampParametersProperties = {
    Version:1,
    Energy_Usage_Milliwatts:1,
    Brightness_Lumens:1
};

LampDetailsProperties = {
    Version:1,
    Make:0,
    Model:0,
    Type:0,
    LampType:0,
    LampBaseType:5,
    LampBeamAngle:100,
    Dimmable:true,
    Color:true,
    VariableColorTemp:true,
    HasEffects:true,
    MinVoltage:120,
    MaxVoltage:120,
    Wattage:9,
    IncandescentEquivalent:75,
    MaxLumens:900,
    MinTemperature:2700,
    MaxTemperature:5000,
    ColorRenderingIndex:0,
    LampID:"4d10cbaecbb1dd83a3e7668825eb2b92"
};

LampStateProperties = {
    OnOff:true,
    Hue:0,
    Saturation:0,
    Brightness:100,
    ColorTemp:2700,
    Version:1
};

var R, G, B;
var lampSvc;

AJ.onMethodCall = function() {
    print("onMethodCall");
    print(this.member);
    if(this.member == "ClearLampFault") {
        LampServiceProperties.LampFaults = [0];
        this.reply(0, arguments[0]);
    } else if(this.member == "TransitionLampState") {
        print(JSON.stringify(arguments[1]));
        var newState = arguments[1];
        for(key in newState) {
            LampStateProperties[key] = newState[key];
        }
        print(JSON.stringify(LampStateProperties));
        var RGB = convertHSBtoRGB(LampStateProperties.Hue, LampStateProperties.Saturation, LampStateProperties.Brightness);
        sendInfoToLightBulb(RGB);
        this.reply(0);
        print("Sent Reply");
        sendSignal(lampSvc);
        print("Sent Signal");
    } else if(this.member == "ApplyPulseEffect") {
        var fromState = arguments[0];
        var toState = arguments[1];
        var period = arguments[2];
        var numPulses = arguments[4];
        var count = numPulses;
        function toggleRGB() {
            if(count%2 == 0) {
                return convertHSBtoRGB(fromState.Hue, fromState.Saturation, fromState.Brightness);
            } else {
                return convertHSBtoRGB(toState.Hue, toState.Saturation, toState.Brightness);
            }
        }
        var id = setInterval(function() {
            if(--count == -1) {
                clearInterval(id);
                var RGB = convertHSBtoRGB(LampStateProperties.Hue, LampStateProperties.Saturation, LampStateProperties.Brightness);
                sendInfoToLightBulb(RGB);
            } else {
                var RGB = toggleRGB();
                sendInfoToLightBulb(RGB);
            }
        }, period);
        this.reply(0);
    } else {
        throw("rejected");
    }
}

AJ.onPropSet = function(iface, prop, value) {
    if(iface == "org.allseen.LSF.LampService") {
        LampServiceProperties[prop] = value;
        this.reply();
    } else if(iface == "org.allseen.LSF.LampParameters") {
        LampParametersProperties[prop] = value;
        this.reply();
    } else if(iface == "org.allseen.LSF.LampDetails") {
        LampDetailsProperties[prop] = value;
        this.reply();
    } else if(iface == "org.allseen.LSF.LampState") {
        LampStateProperties[prop] = value;
        this.reply();
    } else {
        throw("rejected");
    }
}

AJ.onPropGet = function(iface, prop) {
    if(iface == "org.allseen.LSF.LampService") {
        this.reply(LampServiceProperties[prop]);
    } else if(iface == "org.allseen.LSF.LampParameters") {
        this.reply(LampParametersProperties[prop]);
    } else if(iface == "org.allseen.LSF.LampDetails") {
        this.reply(LampDetailsProperties[prop]);
    } else if(iface == "org.allseen.LSF.LampState") {
        this.reply(LampStateProperties[prop]);
    } else {
        throw("rejected");
    }
}

AJ.onPropGetAll = function(iface) {
    if(iface == "org.allseen.LSF.LampService") {
        this.reply(LampServiceProperties);
    } else if(iface == "org.allseen.LSF.LampParameters") {
        this.reply(LampParametersProperties);
    } else if(iface == "org.allseen.LSF.LampDetails") {
        this.reply(LampDetailsProperties);
    } else if(iface == "org.allseen.LSF.LampState") {
        this.reply(LampStateProperties);
    } else {
        throw("rejected");
    }
}

function sendSignal(svc) {
    print("sendSignal");
    print(JSON.stringify(svc));
    var sig = svc.signal("/org/allseen/LSF/Lamp", "org.allseen.LSF.LampState", "LampStateChanged");
    sig.send(LampDetailsProperties.LampID);
}

AJ.onPeerConnected = function(svc) {
    print("onPeerConnected");
    print(JSON.stringify(svc));
    lampSvc = svc;
    return true;
}

AJ.onAttach = function() {
    print("AJ.onAttach");
    R = IO.digitalOut(IO.pin[1]);
    G = IO.digitalOut(IO.pin[2]);
    B = IO.digitalOut(IO.pin[3]);
    var RGB = convertHSBtoRGB(LampStateProperties.Hue, LampStateProperties.Saturation, LampStateProperties.Brightness);
    sendInfoToLightBulb(RGB);
}

AJ.onDetach = function() {
    print("AJ.onDetach");
}

function convertHSBtoRGB(h, s, v) {
    if(h > 360) {
        h = h/11930464.71;
    }
    if(s > 100) {
        s = s/42949672.96;
    }
    if(v > 100) {
        v = v/42949672.96;
    }
    var r, g, b;
    h = h/360;
    s = s/100;
    v = v/100;
    var i = Math.floor(h * 6);
    var f = h * 6 - i;
    var p = v * (1 - s);
    var q = v * (1 - f * s);
    var t = v * (1 - (1 - f) * s);

    switch(i % 6) {
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }
    return [Math.round(r * 255), Math.round(g * 255), Math.round(b * 255)];
}

function sendInfoToLightBulb(RGB) {
    print(RGB);
    var r = RGB[0]/255;
    var g = RGB[1]/255;
    var b = RGB[2]/255;
    if(LampStateProperties.OnOff != true) {
        R.pwm(0, 1);
        G.pwm(0, 1);
        B.pwm(0, 1);
    } else {
        R.pwm(r, 1);
        G.pwm(g, 1);
        B.pwm(b, 1);
    }
}

