/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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

var AJ = require('AllJoyn');

var store_obj = {
    int_val:100,
    str_val:"string",
    obj_val:{
        obj_int:10,
        obj_str:"obj_string"
    }
}

var store_int = 60;
var store_str = "simple_string"

    
    
print("===== Starting Unit Tests =====");
/*
 * Store/Load tests
 */
AJ.store('int_store', store_int);
AJ.store('str_store', store_str);
AJ.store('obj_store', store_obj);

var out_int = AJ.load('int_store');
if (out_int != store_int) {
    alert('Store/Load test error: integer value did not match');
}
var out_str = AJ.load('str_store');
if (out_str != store_str) {
    alert('Store/Load test error: string value did not match');
}
var out_obj = AJ.load('obj_store');
if (out_obj.int_val != store_obj.int_val) {
    alert('Store/Load test error: object.int_val value did not match');
}
if (out_obj.str_val != store_obj.str_val) {
    alert('Store/Load test error: object.str_val value did not match');
}
if (out_obj.obj_val.obj_int != store_obj.obj_val.obj_int) {
    alert('Store/Load test error: object.obj_val.obj_int value did not match');
}
if (out_obj.obj_val.obj_str != store_obj.obj_val.obj_str) {
    alert('Store/Load test error: object.obj_val.obj_str value did not match');
}

/*
 * Time tests
 */
var counter = 0;

function checkCounter()
{
    if (counter > 5) {
        print("Error, setInterval was called too many times");
        return;
    }
    clearInterval(interval);
    print('Counter was correct');
}

function incCounter()
{
    print('counter++');
    counter++;
}
/*
 * Set a counter to increment every 100ms
 * Set a timeout for 500ms
 * if the counter is > 5 then timers don't work correctly.
 */
var interval = setInterval(incCounter, 100);
setTimeout(checkCounter, 500);

