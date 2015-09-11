/******************************************************************************
 * Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
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

/*
 * This demonstrates AllJoyn.js' capability to reset your
 * devices access point information (SSID, Passphrase) using
 * offboarding. The blinking LED is your devices function
 * and the reset push button is a "network reset" button.
 *
 * Say you get this "blinking" device in your home and you
 * have onboarded it to your home router and it is blinking
 * away as it should. You then change your routers password
 * or SSID and you need to reconfigure the "blinking" device
 * to the new credentials. Since the device is head-less you
 * must have a way to reset these credentials. To do this
 * you call AJ.offboard() which will clear any old SSID
 * credentials and cause the device to go back into Soft AP
 * mode and wait to be onboarded again. Linking this
 * functionality to a button is the most realistic use case
 * since most devices like this always have a reset button.
 */

var AJ = require('AllJoyn');
var IO = require('IO');

var reset_pb = IO.digitalIn(IO.pin[22], IO.pullDown);
var blink = IO.digitalOut(IO.pin[7]);

/* This devices only function, blink */
function blink() {
    blink.toggle();
}

/* Function to reset the device */
function reset() {
    AJ.offboard();
}

reset_pb.trigger(IO.FallingEdge, reset);

setInterval(blink, 1000);