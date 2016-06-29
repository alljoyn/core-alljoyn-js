/*
 *    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
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
 */
/**
 * Pin object representing a specific pin on an AllJoyn.js device
 * @typedef Pin
 * @type {object}
 * @property {number} info.physicalPin           - Physical pin number
 * @property {string} info.schematicId           - Schematic ID of the pin
 * @property {string} info.datasheetId           - Datasheet ID of the pin
 * @property {string} info.description           - Description of the pin
 */
/**
 * I/O object returned from require('IO'). This object is used to control supported hardware
 * peripherals on the AllJoyn.js device
 *
 * @name IO
 * @class
 *
 * @property {constant} pullDown                - Constant type to configure a pin as pull down
 * @property {constant} pullUp                  - Constant type to configure a pin as pull up
 * @property {constant} openDrain               - Constant type to configure a pin as open drain
 * @property {constant} fallingEdge             - Constant type to configure an input pin trigger as falling edge
 * @property {constant} risingEdge              - Constant type to configure an input pin trigger as rising edge
 * @property {constant} diable                  - Constant type to disable the trigger mode on an input pin
 */
var IO = {
    /**
     * Array of pin objects for the AllJoyn.js device. Access like IO.pin[x]
     *
     * @type {Pin[]}
     * @example
     * var mypin = IO.pin[10];
     */
    pin:{},
    /**
     * Create a digital input pin object
     *
     * @param {Pin} pin                     - Pin to configure
     * @param {constant} config             - Configuration option for this input: IO.fallingEdge, IO.risingEdge or IO.disable
     * @return {DigitalIn}                  - Digital input object
     *
     * @example
     * var button = IO.digitalIn(IO.pin[10], IO.fallingEdge);
     */
    digitalIn: function(pin, config) {},
    /**
     * Create a digital output pin object
     *
     * @param {Pin} pin                     - Pin to configure
     * @param {constant} [value]            - Initial value of the pin. If not supplied the pin will start low
     * @return {DigitalOut}                 - Digital out object
     *
     * @example
     * var led = IO.digitalOut(IO.pin[10], 1);
     */
    digitalOut: function(pin, value) {},
    /**
     * Create an analog input object
     *
     * @param {Pin} pin                     - Pin to configure
     * @return {AnalogIn}                   - Analog input object
     *
     * @example
     * var sensor = IO.analogIn(IO.pin[10]);
     */
    analogIn: function(pin) {},
    /**
     * Create an analog output object
     *
     * @param {Pin} pin                     - Pin to configure
     * @return {AnalogOut}                  - Analog output object
     *
     * @example
     * var motor = IO.analogOut(IO.pin[10]);
     * motor.value = 100;
     */
    analogOut: function(pin) {},
    /**
     * Create a UART object
     *
     * @param {Pin} tx                      - TX pin
     * @param {Pin} rx                      - RX pin
     * @param {number} baud                 - Baud rate
     * @return {Uart}                       - A UART object
     *
     * @example
     * var u = IO.uart(IO.pin[4], IO.pin[5], 115200);
     *
     * function RxReadyCB(data) {
     *     data = u.read(16);
     *     u.write(data);
     * }
     *
     * u.setTrigger(IO.rxReady, RxReadyCB);
     */
    uart: function(tx, rx, baud) {},
    /**
     * Create a SPI pin
     *
     * @param {Pin} mosi                    - MOSI pin
     * @param {Pin} miso                    - MISO pin
     * @param {Pin} cs                      - Chip select pin
     * @param {Pin} clk                     - Clock pin
     * @param {Object} config               - Configuration parameters of the SPI peripheral
     * @param {number} config.clock         - Clock that will run the SPI peripheral
     * @param {boolean} config.master       - Configure as master or slave
     * @param {number} config.cpol          - Clock polarity (0=low, 1=high)
     * @param {number} config.cpha          - Clock phase (0 = 1 edge, 1 = 2 Edge)
     * @param {number} config.data          - Number of data bits per SPI transfer (platform dependent)
     * @return {Spi}                        - A SPI object
     *
     * @example
     * var s = IO.spi(IO.pin[4], IO.pin[5], IO.pin[6], IO.pin[7], {clock:100000, master:true, cpol:0, cpha:1, data:8});
     *
     * s.write("Write to SPI");
     *
     * // Print out 40 bytes read back
     * print(s.read(40));
     */
    spi: function(mosi, miso, cs, clk, prescaler, master, polarity, phase, databits) {},
    /**
     * Create an I2C slave object
     *
     * @param {Pin} sda                     - SDA pin
     * @param {Pin} scl                     - SCL pin
     * @param {number} clock                - Clock rate
     * @return {I2cSlave}                   - I2C Slave object
     */
    i2cSlave: function(sda, scl, clock) {},
    /**
     * Create an I2C master object
     *
     * @param {Pin} sda                     - SDA pin
     * @param {Pin} scl                     - SCL pin
     * @param {number} clock                - Clock rate
     * @return {I2cMaster}                  - I2C Master object
     *
     * @example
     * var i = IO.i2cMaster(IO.pin5, IO.pin6, 100000);
     *
     * // Send the device 2 bytes then receive 2 bytes
     * var send = 0x1234;
     * var recv;
     * var bytes_received;
     * i.transfer(0x4A, send, 2, recv, 2, bytes_received);
     */
    i2cMaster: function(sda, scl, clock) {},
    /**
     * Make a system call to the underlying OS
     *
     * @param {string} cmd                  - String to execute
     * @return {string}                     - Result of the system call
     *
     * @example
     * var echo = IO.system("echo 'Hello World'");
     *
     * print(echo);
     * >> Hello World
     */
    system: function(cmd) {},
}
/**
 * I2C slave object. This can only be created by calling IO.i2cSlave().
 *
 * @namespace
 */
var I2cSlave = {
    /**
     * Send/Receive data over I2C.
     *
     * @param {number} address              - Address of the device to send/receive data to/from
     * @param {Buffer} txdata               - Data to send
     * @param {number} txlen                - Length of the data to send
     * @param {Buffer} rxdata               - Variable to receive data into
     * @param {number} rxlen                - Requested bytes to receive
     * @param {number} rxactual             - Actual number of bytes received
     *
     * @example
     * var send = 0x1234;
     * var recv;
     * var bytes;
     * var master = IO.i2cMaster(IO.pin[1], IO.pin[2], 100000);
     *
     * master.transfer(0x4A, send, 2, recv, 2, bytes);
     */
    transfer: function(address, txdata, txlen, rxdata, rxlen, rxactual) {}
}
/**
 * I2C master object. This can only be created by calling IO.i2cMaster().
 *
 * @namespace
 */
var I2cMaster = {
    /**
     * Send/Receive data over I2C.
     *
     * @param {number} address              - Address of the device to send/receive data to/from
     * @param {Buffer} txdata               - Data to send
     * @param {number} txlen                - Length of the data to send
     * @param {Buffer} rxdata               - Variable to receive data into
     * @param {number} rxlen                - Requested bytes to receive
     * @param {number} rxactual             - Actual number of bytes received
     *
     * @example
     * var send = 0x1234;
     * var recv;
     * var bytes;
     * var master = IO.i2cMaster(IO.pin[1], IO.pin[2], 100000);
     *
     * master.transfer(0x4A, send, 2, recv, 2, bytes);
     */
    transfer: function(address, txdata, txlen, rxdata, rxlen, rxactual) {}
}
/**
 * SPI object. This can only be created by calling IO.spi().
 *
 * @namespace
 */
var Spi = {
    /**
     * Read from the SPI peripheral
     *
     * @param {number} num                  - Number of bytes
     * @return {Buffer}                     - Buffer containing the data
     */
    read: function(num) {},
    /**
     * Write data to the SPI peripheral
     *
     * @param {Buffer} data                 - Data to write
     */
    write: function(data) {},
}
/**
 * Uart object. This can only be created by calling IO.uart()
 *
 * @namespace
 *
 * @example
 * var u = IO.uart(IO.pin[4], IO.pin[5], 115200);
 *
 * function RxReadyCB(data) {
 *     data = u.read(16);
 *     u.write(data);
 * }
 *
 * u.setTrigger(IO.rxReady, RxReadyCB);
 *
 */
var Uart = {
    /**
     * Read from the UART peripheral
     *
     * @param {number} num                  - Number of bytes
     * @return {Buffer}                     - Buffer containing the data
     */
    read: function(num) {},
    /**
     * Write data to the UART peripheral. You can write most kinds of data types:
     * Strings, Numbers, Booleans, Arrays of numbers, and Duktape buffers.
     *
     * @param {string|number|boolean|number[]|Duktape.Buffer} data      - Data to write
     */
    write: function(data) {},
    /**
     * Set a trigger function callback for RX data available
     *
     * @param {constant} type               - Type of trigger. Should always be IO.rxReady for UART
     * @param {TriggerCallback} callback    - Callback function to be called when data is ready
     */
    setTrigger: function(type, callback) {}
}
/**
 * Analog input object. This can only be created by calling IO.analogIn()
 *
 * @namespace
 * @property {number} value                 - Value of the analog input
 */
var AnalogIn = {}
/**
 * Analog output object. This can only be created by calling IO.analogOut()
 *
 * @namespace
 * @property {number} value                 - Value to set the analog output to
 */
var AnalogOut = {}
/**
 * Digital output object. This can only be created by calling IO.digitalOut()
 *
 * @namespace
 * @property {number} level                 - Set the level of the digital out pin (1=high, 0=low)
 */
var DigitalOut = {
    /**
     * Toggle the digital out pin, reversing the current pin direction
     */
    toggle: function() {},
    /**
     * Start PWM on the digial output pin
     *
     * @param {number} duty                 - Duty cycle between 0 and 1
     * @param {number} freq                 - Frequency in Hz
     */
    pwm: function(duty, freq) {}
}
/**
 * Digital input object. This can only be created by calling IO.digitalIn()
 *
 * @namespace
 * @property {number} level                 - Read the level of the input pin
 */
var DigitalIn = {
    /**
     * Set the trigger type and callback function
     *
     * @param {constant} mode               - Trigger mode
     * @param {TriggerCallback} callback    - Callback function for when the pin has been triggered
     */
    setTrigger: function(mode, callback) {}
}

/**
 * Callback function for peripheral triggers. This is used for digial inputs
 * and UART inputs.
 *
 * @namespace
 * @param val       - Received trigger value. In the case of digital inputs this is the value of
 *                    the level of the pin that was triggered.
 */
var TriggerCallback = function(val) {};

/**
 * Pin info object. Pin info objects are members of the IO.pin[] array.
 *
 * @namespace
 * @property {number} physicalPin           - Physical pin number
 * @property {string} schematicId           - Schematic ID of the pin
 * @property {string} datasheetId           - Datasheet ID of the pin
 * @property {string} description           - Description of the pin
 */
var PinInfo = {}
/**
 * AllJoyn interface definition
 * @typedef InterfaceDefinition
 * @type {object}
 * @property {constant} type                - Type designation (AJ.SIGNAL, AJ.METHOD or AJ.PROPERTY)
 * @property {string[]} args                - Signature arguments (singnals and methods only)
 * @property {string[]} returns             - Reply signature (methods only)
 * @property {string} signature             - Signature for properties only
 * @property {string} access                - Access rights (properties only)
 */

/**
 * Security Object
 * @typedef SecurityObject
 * @type {object}
 * @property {object} ecdhe_ecdsa            - ECDHE_ECDSA suite credentials field
 * @property {string} ecdhe_ecdsa.prv_key    - Private key for ECDHE_ECDSA authentication
 * @property {string} ecdhe_ecdsa.cert_chain - Reply signature (methods only)
 * @property {object} ecdhe_speke            - ECDHE_SPEKE suite credentials field
 * @property {string} ecdhe_speke.password   - Password for ECDHE_SPEKE authentication
 * @property {bool}   ecdhe_null             - Enables or disables ECDHE_NULL suite
 * @propetry {constant[]} claim_with         - Array of claiming mechanisms to use (AJ.CLAIM_ECDSA, AJ.CLAIM_SPEKE, AJ.CLAIM_NULL)
 */
/*
 * AllJoyn object definition
 * @typedef ObjectDefinition
 * @type {object}
 * @property {string[]} interfaces          - List of interfaces for this object
 */
/**
 * AllJoyn object. This object is used to make method calls, send signals, and get/set properties.
 * The AllJoyn object is returned from a call to require('AllJoyn'). For the purposes of this API
 * documentation the AllJoyn object will be refered to as AJ.
 *
 * @class
 *
 * @property {constant} METHOD                  - Constant type to designate a method
 * @property {constant} SIGNAL                  - Constant type to designate a property
 * @property {constant} PROPERTY                - Constant type to designate a property
 * @property {constant} notification.Info       - Constant type to designate an informational notification
 * @property {constant} notification.Warning    - Constant type to designate a warning notification
 * @property {constant} notification.Emergency  - Constant type to designate an emergency notification
 *
 * @example
 * var AJ = require('AllJoyn');
 */
var AJ = {
    /**
     * Definition of about information to announce. This is an object composed of named fields or objects,
     * where the name is an about property and each object is a value and an access type. The access
     * type can be one of four types: AJ.READONLY, AJ.ANNOUNCE, AJ.LOCALIZED, or AJ.PRIVATE. If an
     * access flag is not given, AJ.ANNOUNCE will be implied. The field name can only be one of the
     * About Data Interface fields. See https://allseenalliance.org/framework/documentation/learn/core/about-announcement/interface
     *
     * @example
     * Aj.aboutDefinition = {
     *              AppName: "AllJoyn.js",
     *              Description:     { value: "AllJoyn.js Example" },
     *              HardwareVersion: { value: "0.0.1", access: AJ.HIDDEN }
     * }
     */

    aboutDefinition: {
        /**
         * Value for the field
         */
        value: "",
        /**
         * Access type for the About field
         */
        access: {}
    }

    /**
     * Definition for security and credentials to use in alljoyn communications for the application.
     * A SecurityObject should be assigned to this to define the enabled security suites and their
     * credentials.
     *
     * @sample
     * AJ.securityDefinition = {
     *              ecdhe_speke: {
     *                  password: "1234"
     *              },
     *              ecdhe_ecdsa: {
                        prv_key: "-----BEGIN EC PRIVATE KEY-----.....",
                        cert_chain:  "-----BEGIN CERTIFICATE-----....."
     *              },
     *              ecdhe_null: true,
     *              claimWith: [AJ.CLAIM_NULL, AJ.CLAIM_SPEKE],
     *              expiration: 5000
     * }
     *
     */
    securityDefinition = {
        ecdhe_speke: {
            password: ""
        },
        ecdhe_ecdsa: {
            prv_key: "",
            cert_chain:  ""
        },
        ecdhe_null: true,
        claimWith: [AJ],
        expiration: 1

    }
    /**
     * Definition of interfaces, signals, methods and properties to be advertised. This is an
     * array of named objects, indexed by interface, where each object is either a method, signal, or
     * property, designated by a "type" element. The "type" element is set to either AJ.METHOD,
     * AJ.SIGNAL, or AJ.PROPERTY. Methods and signals will contain an "args" element which is the
     * signature for the input parameters to the method or signal. The method reply signature is
     * defined by the "returns" element. A property type is defined by the "signature" element.
     * Properties also have an optional element "access" which defines if the property can be read,
     * written, or both.
     *
     * @type {InterfaceDefinition[]}
     * @example
     * AJ.interfaceDefinition['org.alljoyn.sample'] = {
     *              myMethod:{ type:AJ.METHOD, args:["i"], returns:["s"] },
     *              mySignal:{ type:AJ.SIGNAL, args:["t"] },
     *              myProp:{ type:AJ.PROPERTY, signature:"u", access:"RW" }
     * }
     */
    interfaceDefinition: {
        /**
         * Type of this element, either AJ.METHOD, AJ.SIGNAL, or AJ.PROPERTY
         */
        type:{},
        /**
         * Signature for input arguments
         */
        args:[""],
        /**
         * Methods will have a return signature type
         */
        returns:"",
        /**
         * Properties type is defined by the "signature" element
         */
        signature:"",
        /**
         * Properties can have an optional "access" element
         */
        access:""
    },
    /**
     * Definition of objects and the interfaces they possess. This is an array of objects indexed
     * by an object path. Each object should contain one element, "interfaces", which is an array of
     * strings that correspond to elements defined in the programs "interfaceDefinition". Each object
     * can also optionally have the element flags that defines the visibility of the object on the
     * alljoyn bus. The flags element is an array that can contain the following types: AJ.SECURE,
     * AJ.HIDDEN, AJ.DISABLED, AJ.ANNOUNCED and AJ.PROXY. Note: if securityDefinition is defined and
     * and object needs to have security then the AJ.SECURE flag needs to be given.
     *
     * @type {ObjectDefinition[]}
     * @example
     * objectDefinition['path/to/sample/object'] = {
     *     interfaces['org.alljoyn.sample', 'org.alljoyn.test_iface']
     * }
     */
    objectDefinition: {
        /**
         * List of interfaces this object represents
         */
        interfaces:[""]
    },
    /**
     * Callback for becoming attached onto the AllJoyn bus
     *
     * @example
     * AJ.onAttach = function() {
     *     print('Attached to bus');
     * }
     */
    onAttach: function() {},
    /**
     * Callback for becoming detached from the AllJoyn bus
     *
     * @example
     * AJ.onDetach = function() {
     *     print('Detached from bus');
     * }
     */
    onDetach: function() {},
    /**
     * Callback when a peer connects to an announced/advertised service. Return true or
     * false to accept or deny the connection.
     *
     * @param {Service} svc     Service object
     * @return {boolean}        true: to accept the connection to the peer
     *                          false: to deny the conenction to the peer
     *
     * @example
     * var service;
     *
     * AJ.onPeerConnected = function(svc) {
     *     // Save this service otherwise you will not be able to make calls upon return
     *     service = svc;
     *     print('Peer has connected');
     *     return true;
     * }
     */
    onPeerConnected: function(svc) {},
    /**
     * Callback when a peer disconnects from a hosted service
     *
     * @param {String} peer      Name of the peer that has disconnected
     *
     * @example
     * AJ.onPeerDisconnected = function(peer) {
     *     print('Peer:' + peer + ' has disconnected');
     * }
     */
    onPeerDisconnected: function(peer) {},
    /**
     * Callback for a method call being received. In this callback you have access to 'this' object.
     * The 'this' object represents the AllJoyn signal. Using 'this' you can check values such
     * as 'member' (signal name), 'sender', 'iface', 'path', and a boolean 'fromSelf' which
     * tells you if the signal was sent from yourself.
     *
     * @param arg       Argument to the method call
     *
     * @example
     * AJ.onMethodCall = function(arg) {
     *     if (this.member == 'my_method') {
     *         this.reply('success');
     *     } else {
     *         throw('rejected');
     *     }
     * }
     */
    onMethodCall: function(arg) {},
    /**
     * Callback when security policy changes.
     *
     * @example
     * AJ.onPolicyChanged = function() {
     *     print("Security policy has changed");
     * }
     */
    onPolicyChanged: function() {},
    /**
     * Callback when a signal is received. In this callback you have access to 'this' object.
     * The 'this' object represents the AllJoyn signal. Using 'this' you can check values such
     * as 'member' (signal name), 'sender', 'iface', 'path', and a boolean 'fromSelf' which
     * tells you if the signal was sent from yourself.
     *
     * @example
     * AJ.onSignal = function() {
     *     if (this.member == 'my_signal') {
     *         print('Signal received');
     *     } else {
     *         throw('rejected');
     *     }
     * }
     */
    onSignal: function() {},
    /**
     * Callback for a peer getting a services property. In this function you need to reply with the property value the peer has asked for
     *
     * @param {String} iface    Interface the property exists on
     * @param {String} prop     Name of the property
     *
     * @example
     * var properties = {
     *     int_prop:1234,
     *     str_prop:"Hello World"
     * }
     *
     *  AJ.onPropGet = function(iface, prop) {
     *      if (iface == 'org.alljoyn.sample') {
     *          this.reply(properties[prop]);
     *      } else {
     *          throw('rejected');
     *      }
     *  }
     */
    onPropGet: function(iface, prop) {},
    /**
     * Callback for a peer setting a services property. In this function you need to set the value of the property the peer asked to set.
     *
     * @param {String} iface    Interface the property exists on
     * @param {String} prop     Name of the property being set
     * @param value             Value the property is being set to.
     *
     * @example
     * var properties = {
     *     int_prop:1234,
     *     str_prop:"Hello World"
     * }
     *
     *  AJ.onPropSet = function(iface, prop, value) {
     *      if (iface == 'org.alljoyn.sample') {
     *          properties[prop] = value;
     *          this.reply();
     *      } else {
     *          throw('rejected');
     *      }
     *  }
     */
    onPropSet: function(iface, prop, value) {},
    /**
     * Callback for a peer getting all properties. In this function you need to reply with all the properties defined for the interface.
     *
     * @param {String} iface    Interface to get all properties from
     *
     * @example
     * var properties = {
     *     int_prop:1234,
     *     str_prop:"Hello World"
     * }
     *
     *  AJ.onPropGetAll = function(iface) {
     *      if (iface == 'org.alljoyn.sample') {
     *          this.reply(properties);
     *      } else {
     *          throw('rejected');
     *      }
     *  }
     */
    onPropGetAll: function(iface) {},

    /**
     * Get the AllJoyn unique name
     *
     * @return {String} Current AllJoyn bus unique name
     *
     */
    getUniqueName: function() {},

    /**
     * Clear saved credentials from NVRAM
     */
    clearCredentials: function() {},

    /**
     * Find a service advertised by a different peer using About
     *
     * @param {String} iface        Interface being found
     * @param {ServiceCallback} callback    Callback function for when the service is found
     *
     * @example
     * var service;
     *
     * function found(svc) {
     *     // Save the service to make calls on it later
     *     service = svc;
     * }
     *
     * AJ.onAttach = function() {
     *     print('Attached');
     *     AJ.findService('org.alljoyn.sample', found);
     * }
     */
    findService: function(iface, callback) {},

    /**
     * Find a secure service advertised by a different peer using About
     *
     * @param {String} iface                Interface being found
     * @param {SecurityObject}  credentials Security credentials to authenticate with the service
     * @param {ServiceCallback} callback    Callback function for when the service is found
     *
     * @example
     * var service;
     *
     * function found(svc) {
     *     // Save the service to make calls on it later
     *     service = svc;
     * }
     *
     * AJ.onAttach = function() {
     *     print('Attached');
     *     AJ.findService('org.alljoyn.sample', found);
     * }
     */
    findService: function(iface, callback) {},

    /**
     * Find a service using legacy name-based discovery
     *
     * @example
     * var nameObj = {
     *     interfaces:['org.alljoyn.sample'],
     *     path:'/path/to/object',
     *     port:1234
     * }
     *
     * var service;
     *
     * function foundService(svc) {
     *     // Save the service to make calls on it later
     *     service = svc;
     *     print('service was found');
     * }
     *
     * AJ.findServiceByName('org.alljoyn.myname', nameObj, foundService);
     *
     * @param {String} name                     Service name to find
     * @param {Object} nameObject               Object containing interfaces, path, and port number
     * @param {string[]} nameObject.interfaces  Array of interfaces of interest
     * @param {string} nameObject.path          Object path for this service
     * @param {number} nameObject.port          Port number for this service
     * @param {ServiceCallback} callback        Callback function called when the service is found
     */
    findServiceByName: function(name, nameObject, callback) {},
    /**
     * Add a match rule for a signal. Use this to receive signals not defined in the interfaceDefinition
     *
     * @param {String} iface                Interface the signal is being sent on
     * @param {String} name                 Name of the signal
     *
     * @example
     *  // Add match to listen for notifications
     * AJ.addMatch('org.alljoyn.Notification', 'notif');
     */
    addMatch: function(iface, name) {},
    /**
     * Create a Signal object without a service object. This can be used to send a
     * sessionless signal (i.e. send a signal while not being in a session).
     *
     * @param {String} name                 Name of the signal
     * @return {Signal}                     Signal object
     *
     * @example
     * var sig = AJ.signal('my_signal');
     * sig.sessionless = true;
     * sig.timeToLive = 100;
     * sig.send();
     */
    signal: function(name) {},
    /**
     * Create a notification object
     *
     * @param type                          Type of notification
     * @param {String} text                 Notification text
     *
     * @return {Notification}               A notification object
     *
     * @example
     * var notif = AJ.notification(AJ.notification.Warning, "This is a warning");
     *
     * notif.timeToLive = 100;
     *
     * notif.send(100);
     */
    notification: function(type, text) {},
    /**
     * Create a control panel object.
     *
     * @return {ControlPanel}
     */
    controlPanel: function() {},
    /**
     * Store a value into persistant storage. This can be used for app storage or
     * to change values of the config service.
     *
     * @param {string} entry                Data entry name
     * @param data                          Any data you wish to store
     */
    store: function(entry, data) {},
    /**
     * Load any stored value from persistant storage. This can be used to access
     * values of the config service or for app storage.
     *
     * @param {string} entry                Data entry to load
     * @return                              Data in persistant storage
     */
    load: function(entry) {},
    /**
     * Reset a devices SSID and passphrase credentials and reboot. After the device
     * reboots it will go into soft AP mode, waiting to be onboarded.
     */
    offboard: function() {}
};
/**
 * Set a function to be called at a millisecond interval
 *
 * @namespace
 * @type {function}
 * @method
 * @param {function} func                   Function to be called
 * @param {number} interval                 Interval to call the function at
 * @return {Context}                        A context representing the setInterval call
 *
 * @example
 * var i = setInterval(function() { print('Hello'); }, 1000);
 */
function setInterval(func, interval) {}
/**
 * Clear a previously set interval.
 *
 * @namespace
 * @type {function}
 * @method
 * @param {Context} intervalCtx             Context returned from setInterval()
 *
 * @example
 * var count = 0;
 *
 * function increment(ctx) {
 *     count++;
 *     if (count > 10) {
 *         clearInterval(ctx);
 *     }
 * }
 *
 * var i = setInterval(increment(i), 100);
 */
function clearInterval(intervalCtx) {}
/**
 * Reset a previously set interval
 *
 * @namespace
 * @type {function}
 * @method
 * @param {Context} intervalCtx             Context returned from setInterval()
 *
 * @example
 * var count = 0;
 *
 * function increment(ctx) {
 *     count++;
 *     if (count > 10) {
 *         resetInterval(ctx);
 *     }
 * }
 *
 * var i = setInterval(increment(i), 100);
 */
function resetInterval(intervalCtx) {}
/**
 * Set a function to be called after a millisecond timeout
 *
 * @namespace
 * @type {function}
 * @method
 * @param {function} func                   Function to be called
 * @param {number} timeout                  Time to wait before calling the function
 * @return {Context}                        A context representing the setTimeout call
 *
 * @example
 * var t = setTimeout(function() { print('timeout'); }, 1000);
 */
function setTimeout(func, timeout) {}
/**
 * Clear a previously set timeout
 *
 * @namespace
 * @type {function}
 * @method
 * @param {Context} timeoutCtx              Context returned from setTimeout()
 *
 * @example
 *
 * var count = 0;
 *
 * function increment(ctx) {
 *     count++;
 *     if (count > 10) {
 *         clearTimeout(ctx);
 *     }
 * }
 * function timeout() {
 *     print('Timeout has been met');
 * }
 * var t = setTimeout(timeout, 10000);
 * // The interval count will be met before the timeout, resulting in the timeout being cancelled
 * var i = setInterval(increment(t), 10);
 */
function clearTimeout(timeoutCtx) {}
/**
 * Method object. This can only be created after a Service object is available by using
 * the 'method' function of the Service object.
 *
 * @namespace
 * @property {number} timeout   - Timeout for this method
 *
 * @example
 * var method = svc.method('my_method');
 */
var Method = {
    /**
     * Used to make a call on this method object
     *
     * @param data          Raw data to send as the method arguments
     */
    call: function(data) {},
    /**
     * Callback thats called when the method reply is received
     *
     * @param arg           Argument data in the method reply
     */
    onReply: function(arg) {}
}
/**
 * Signal object. This can only be created after a Service object is available by using
 * the 'signal' function of the Service object.
 *
 * @namespace
 * @property {boolean} sessionless - Set if the signal is sessionless
 * @property {number} timeToLive   - Set the time to live (for sessionless signals)
 *
 * @example
 * var signal = svc.signal('my_signal');
 */
var Signal = {
    /**
     * Send the signal
     *
     * @param data          Raw data to send as the signal payload
     */
    send: function(data) {}
}
/**
 * Service object. A Service object is made available through a find service method call
 * as the callback parameter. A Service object must be saved during a {ServiceCallback}
 * otherwise the ref count will drop and you will no longer have access the any service
 * properties or methods.
 *
 * @namespace
 * @property {String} iface - Interface for this service object
 *
 * @example
 * var saved_service;   //Used to hold a reference to the service object
 *
 * function foundServiceCB(svc) {
 *     saved_service = svc;
 * }
 */
var Service = {
   /**
    * Used to create a method object on this service
    *
    * @param {String} name          Name of the method
    * @return {Method}              Method object
    */
    method: function(name) {},
   /**
    * Used to create a signal object on this service
    *
    * @param {String} path     Object path for the signal
    * @param {String} iface    Interface to call the signal on
    * @param {String} name     Name of the signal
    *
    * @return {Signal}
    */
    signal: function (path, iface, name) {},
   /**
    * Get a property
    *
    * @param {String} name                  Name of the property
    *
    * @return                               The value of the property
    */
    getProp: function(name) {},
   /**
    * Set a property
    *
    * @param {String} name                  Name of the property to set
    * @param data                           Raw data to set the property to
    */
    setProp: function(name, data) {},
   /**
    * Get all properties on an interface
    *
    * @example
    * svc.getAllProps('org.alljoyn.sample').onReply = function() {
    *     for (var i = 0; i < arguments.length; i++) {
    *         print(arguments[i]);
    *     }
    * }
    *
    * @param {String} iface                 Interface to get all properties from
    *
    * @return {GetAllPropsObject}           Object with an "onReply" callback
    */
    getAllProps: function(iface) {}
}
/**
 * Callback for when a service is found. The Service object is always the parameter
 * to the service callback.
 *
 * @namespace
 * @param {Service} svc             Service object
 */
var ServiceCallback = function(svc) {}
/**
 * Callback from a getAllProps call
 *
 * @namespace
 */
var GetAllPropsObject = {
    /**
     * Called when all properties were retrieved from the remote peer. In this callback
     * you have access to an "arguments" array which contains all the properties.
     */
    onReply: function() {}
}
/**
 * Notification object. This can only be created with a call to AJ.notification()
 *
 * @namespace
 * @property {string} text      - Text that the notification contains.
 * @property {constant} type        - Type of the notification. Emergency, Warning or Info
 * @property {string} audioUrls - Attach a URL to an audio recording to be played when the notification is received
 * @property {string} iconUrls  - Attach an icon URL to the notification that is displayed upon arrival.
 * @property {string} iconPath  - Attach an icon path to be displayed when the notification is received
 * @property {string} controlPanelPath - Attach a control panel path to the notification
 * @property {Object} attributes - Attach custom attributes to the notification
 * @example
 * notif.type = AJ.notification.Warning
 *
 * notif.text = { "EN":"Hello World",
 *                "SP":"Hola Mundo" }
 *
 * notif.attributes = {"IP Address":"192.168.240.124",
 *                     "Name":"Personal Device",
 *                     "Model Number":"XP239-VPN"
 */
var Notification = {
    /**
     * Send the notification
     *
     * @param ttl                   - Time to live in seconds
     */
    send: function(ttl) {},
    /**
     * Cancel a notification after it has been sent (if within the ttl)
     */
    cancel: function() {}
}
/**
 * A container that holds a control panels widgets. This can only be created with a call
 * to containerWidget()
 *
 * @namespace
 * @property {boolean} enabled        - Set whether or not widgets withing this container are enabled
 * @property {string} path            - File path for the control panel
 *
 * @example
 * var cp = AJ.controlPanel()
 * var container_widget = cp.containerWidget(cp.VERTICAL, cp.HORIZONTAL);
 */
var ContainerWidget = {
    /**
     * Creates a lable widget which equates to a text box
     *
     * @param {string} phrase           - Text to be displayed in the label widget
     * @return {LabelWidget}            - A label widget object
     */
    labelWidget: function(phrase) {},
    /**
     * Creates a property widget which is some kind of input widget based on a chosen layout
     *
     * @param {constant} layout             - Type of container widget
     * @return {PropertyWidget}         - A property widget object
     */
    propertyWidget: function(layout) {},
    /**
     * Creates an action widget which is a pop up that requires an action to be taken my the user
     *
     * @param {string} phrase           - Phrase to be displayed
     * @return {ActionWidget}           - An action widget object
     */
    actionWidget: function(phrase) {},
    /**
     * Creates a dialog widget which is a widget that pops up and provides several choices for the user to enter
     *
     * @param {string} phrase           - Phrase to be displayed
     * @return {DialogWidget}           - A dialog widget object
     */
    dialogWidget: function(phrase) {},
}
/**
 * An action widget object. This can only be created with a call to actionWidget()
 *
 * @namespace
 * @property {string} label           - Text label for this action widget
 * @property {boolean} enabled        - Set whether or not this widget is enabled
 *
 * @example
 * var cp = AJ.controlPanel()
 * var cw = cp.containerWidget(cp.VERTICAL, cp.HORIZONTAL);
 *
 * var action_widget = cw.actionWidget('Press me');
 */
var ActionWidget = {};
/**
 * A label widget object. This can only be created with a call to labelWidget()
 *
 * @namespace
 * @property {string} label            - The text that this widget displays
 * @property {string} path             - Control panel path
 * @property {boolean} enabled         - Set if this label widget is visible to the user
 *
 * @example
 * var cp = AJ.controlPanel()
 * var cw = cp.containerWidget(cp.VERTICAL, cp.HORIZONTAL);
 *
 * var label_widget = cw.labelWidget('My Label');
 */
var LabelWidget = {};
/**
 * Property widget object. This can only be created with a call to propertyWidget()
 *
 * @namespace
 * @property {string} label              - Text label for this property widget
 * @property {(number|string)} value     - Value of the property widget
 * @property {(number|string)} choices - Array of choices for this property widget
 * @property {{min:number, max:number, increment:number}} range - Range of values for the property widget
 * @property {boolean} enabled           - Set if the property widget is visible to the user
 *
 * @example
 * var cp = AJ.controlPanel()
 * var cw = cp.containerWidget(cp.VERTICAL, cp.HORIZONTAL);
 *
 * var prop_widget = cw.propertyWidget(cp.SLIDER);
 *
 * prop_widget.onValueChanged = function(val) {
 *     print('New value = ', val);
 * }
 */
var PropertyWidget = {
    /**
     * Callback function for when a property widgets value has changed
     *
     * @param val                          - New value of the property
     */
    onValueChanged: function(val) {}
}
/**
 * A dialog widget object. This can only be created with a call to dialogWidget()
 *
 * @namespace
 * @property {ButtonObject[]} buttons       - Array of buttons objects
 *
 * @example
 * var cp = AJ.controlPanel()
 * var cw = cp.containerWidget(cp.VERTICAL, cp.HORIZONTAL);
 *
 * var dialog_widget = cw.dialogWidget('Press me');
 */
var DialogWidget = {};
/**
 * Button object for dialog widgets. Dialog widgets have an array of ButtonObject's
 * as an element 'buttons'
 *
 * @namespace
 * @property {string} label             - Label for this button
 */
var ButtonObject = {
    /**
     * Function that is called when this button is clicked
     */
    onClick: function() {}
}
/**
 * Control panel object. This can only be created with a call to AJ.controlPanel()
 *
 * @namespace
 * @property {constant} SWITCH          - Two-state buttons allowing the end-user to toggle the state of a single settings option
 * @property {constant} CHECK_BOX       - Widget for multi-select. It allows the end user to select multiple options from a list.
 * @property {constant} SPINNER         - Widget for single-select. It allows the end user to select a single option from a list.
 * @property {constant} RADIO_BUTTON    - Widget for single-select. It allows the end user to select a single option from a list.
 * @property {constant} SLIDER          - Allows the end user to select a value from a continuous or discrete range. The appearance is linear, either horizontal or vertical.
 * @property {constant} TIME_PICKER     - Allows the end user to specify a time value
 * @property {constant} DATE_PICKER     - Allows the end user to specify a date value
 * @property {constant} NUMBER_PICKER   - Allows the end user to specify a numeric value
 * @property {constant} KEYPAD          - Provides the end user with a numeric entry field and buttons for 0-9 digits, to enter a numeric value. Max digit is 32767
 * @property {constant} ROTARY_KNOB     - An alternate way to represent a slider
 * @property {constant} TEXT_VIEW       - Read-only text label
 * @property {constant} NUMERIC_VIEW    - Provides a read-only, numeric field with an optional label and numbers
 * @property {constant} EDIT_TEXT       - Provides the end user with a text entry field and keyboard. Max characters is 64 per word
 * @property {constant} VERTICAL        - Used to create a vertical container widget
 * @property {constant} HORIZONTAL      - Used to create a horizontal container widget
 * @property {string} path          - File path to the control panel
 *
 * @example
 * var cp = AJ.controlPanel();
 * var cw = cp.containerWidget(cp.VERTICAL, cp.HORIZONTAL);
 * cp.load(); // Create an empty control panel
 */
var ControlPanel = {
    /**
     * Load the control panel after widgets have been created
     */
    load: function() {},
    /**
     * Create a container widget to hold widgets
     *
     * @param {constant} direction1      - Direction for the container widget. Either cp.VERTICAL or cp.HORIZONTAL
     * @param {constant} [direction2]    - Optional parameter for specifying a second direction
     * @return {ContainerWidget}     - A container widget to create widgets from
     */
    containerWidget: function(direction1, direction2) {}
}