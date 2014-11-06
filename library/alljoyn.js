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
/*
 * Do not reference these objects directly, instead create
 * an IO object and reference them that way.
 */
function digitalOut() {
	digitalOut.level = {};
	digitalOut.toggle = function(){};
	digitalOut.pwm = function(duty, freq){};
}

function digitalIn() {
	digitalOut.setTrigger = function(trigger, func){};
}

function analogIn() {
	analogIn.value = {};
}

function analogOut() {
	analogOut.value = {};
}

function uart() {
	uart.read = function(numBytes){};
	uart.write = function(value){};
}

function i2c() {
	i2c.transfer = function(address, txBuf, txLength, rxBuf, rxLength, rxBytes){};
}

function spi() {
	spi.write = function(){};
	spi.read = function(numBytes){};
}

function system() {};

/*
 * IO object definition
 */
function IO() {
	IO.pullDown = {};
	IO.pullUp = {};
	IO.openDrain = {};
	IO.fallingEdge = {};
	IO.risingEdge = {};
	IO.disable = {};
	IO.digitalOut = function(pin, initial) {
		return new digitalOut;
	};
	IO.digitalIn = function(pin, pull){
		return new digitalIn;
	};
	IO.analogOut = function(pin){
		return new analogOut;
	};
	IO.analogIn = function(pin){
		return new analogIn;
	};
	IO.uart = function(pinTx, pinRx, baud) {
		return new uart;
	};
	IO.i2cMaster = function(sda, scl, clk) {
		return new i2c;
	}
	IO.i2cSlave = function(sda, scl, address) {
		return new i2c;
	}
	IO.spi = function(mosi, miso, cs, clock, presclaer, master, cpol, cpha, data_bits) {
		return new spi;
	}
	IO.system = function(command) {
		return new system;
	}
}

/*
 * The following object definitions should not be called from JavaScript
 * directly. Always reference them from the "AJ" object first
 */
function actionWidget() {
	actionWidget.label = {};
	actionWidget.enabled = {};
	actionWidget.path = {};
}

function propertyWidget() {
	propertyWidget.enumeration = {};
	propertyWidget.range = {};
	propertyWidget.label = {};
	propertyWidget.value = {};
	propertyWidget.choices = {};
	propertyWidget.path = {};
	propertyWidget.enabled = {};
	propertyWidget.onValueChanged = function(){};

}

function dialogWidget() {
	dialogWidget.buttons = {};
	dialogWidget.path = {};
}

function labelWidget() {
	labelWidget.enabled = {};
	labelWidget.path = {};
}

function containerWidget() {
	containerWidget.enabled = {};
	containerWidget.path = {};
	containerWidget.labelWidget = function() {
		return new labelWidget;
	};
	containerWidget.propertyWidget = function() {
		return new propertyWidget;
	};
	containerWidget.dialogWidget = function() {
		return new dialogWidget;
	};
	containerWidget.actionWidget = function() {
		return new actionWidget;
	};
}

function controlPanel() {
	controlPanel.VERTICAL = {};
	controlPanel.HORIZONTAL = {};
	controlPanel.SLIDER = {};
	controlPanel.SWITCH = {};
	controlPanel.CHECK_BOX = {};
	controlPanel.SPINNER = {};
	controlPanel.RADIO_BUTTON = {};
	controlPanel.TIME_PICKER = {};
	controlPanel.DATE_PICKER = {};
	controlPanel.NUMBER_PICKER = {};
	controlPanel.KEYPAD = {};
	controlPanel.ROTARY_KNOB = {};
	controlPanel.TEXT_VIEW = {};
	controlPanel.NUMERIC_VIEW = {};
	controlPanel.EDIT_TEXT = {};
	controlPanel.color = {};
	controlPanel.label = {};
	controlPanel.path = {};
	controlPanel.load = function() {};
	controlPanel.propertyWidget = function() {
		return new propertyWidget;
	};
	controlPanel.containerWidget = function() {
		return new containerWidget;
	};
	controlPanel.labelWidget = function() {
		return new labelWidget;
	};
	controlPanel.dialogWidget = function() {
		return new dialogWidget;
	};
	controlPanel.actionWidget = function() {
		return new actionWidget;
	};
}

function notification() {
	notification.text = {};
	notification.type = {};
	notification.send = function() {};
	notification.cancel = function() {};
	notification.audioUrls = {};
	notification.audioPath = {};
	notification.iconUrl = {};
	notification.iconPath = {};
	notification.controlPanelPath = {};
	notification.attributes = {};
}

function signal() {
	signal.sessonless = {};
	signal.timeToLive = {};
	signal.send = function() {};
}

/*
 * AJ object definition's
 */
function AJ() {
	AJ.METHOD = {};
	AJ.SIGNAL = {};
	AJ.PROPERTY = {};
	AJ.onAttach = {};
	AJ.onDetach = {};
	AJ.onMethodCall = {};
	AJ.onSignal = {};
	AJ.onPropSet = {};
	AJ.objectDefinition = [];
	AJ.interfaceDefinition = [];
	AJ.findService = function() {};
	AJ.onPeerConnected = function() {};
	AJ.method = function(name) {};
	AJ.call = function() {};
	AJ.onReply = function() {};
	AJ.signal = function(objDef, ifaceDef, name) {
		return new signal;
	};
	AJ.getProp = function(name) {};
	AJ.setProp = function() {};
	AJ.getAllProps = function() {};
	AJ.onMethodCall = function() {};
	AJ.onPropGet = function() {};
	AJ.onPropGetAll = function() {};
	AJ.addMatch = function(iface, name) {};
	AJ.controlPanel = function() {
		return new controlPanel;
	};
	AJ.notification = function() {
		return new notification;
	};
}


