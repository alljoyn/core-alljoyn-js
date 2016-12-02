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

AJ.interfaceDefinition["org.alljoyn.marshal_test"] =
{
    test:{ type:AJ.METHOD, args:["av"], returns:["s"] },
    test2:{ type:AJ.METHOD, args:["av"], returns:["s"]},
	test3:{ type:AJ.METHOD, args:["(ii(ss))"], returns:["s"]},
	test4:{ type:AJ.METHOD, args:["ai"], returns:["s"]},
	test5:{ type:AJ.METHOD, args:["a{is}"], returns:["s"]},
	test6:{ type:AJ.METHOD, args:["a{is}"], returns:["s"]},
	test7:{ type:AJ.METHOD, args:["ay"], returns:["s"]},
	test8:{ type:AJ.SIGNAL}
};

function test(svc)
{
    print(JSON.stringify(svc));

    svc.method('test').call([{"s":"July"}, {"i":13}, {"i":2014}]).onReply = function() {

        print("test replied with ", arguments[0]);
        test2(svc);
    }
}

function test2(svc)
{

    svc.method('test2').call([{"s":"July"},{"v":{"i":13}},{"av":[{"i":1995},{"i":2014}]}]).onReply = function() {

        print("test2 replied with ", arguments[0]);
		test3(svc);
    }
}

function test3(svc)
{
    svc.method('test3').call([7, 13, ["July", "Thirteenth"]]).onReply = function() {
		print("test3 replied with ", arguments[0]);
		test4(svc);
    }
}

function test4(svc)
{
	svc.method('test4').call([10, 11, 9]).onReply = function() {
		print("test4 replied with ", arguments[0]);
		test5(svc);
	}
}

function test5(svc)
{
	var array = [];
	array[3] = "Three";
	array[4] = "Four";
	svc.method('test5').call(array).onReply = function() {
		print("test5 replied with ", arguments[0]);
		test6(svc);
	}
}

function test6(svc)
{
	svc.method('test6').call({3:"Three", 4:"Four"}).onReply = function() {
		print("test6 replied with ", arguments[0]);
		test7(svc);
	}
}

function test7(svc)
{
	svc.method('test7').call(Duktape.Buffer('foo')).onReply = function() {
		print("test7 replied with ", arguments[0]);
		test8(svc);
	}
}

function test8(svc){
	AJ.objectDefinition['/test8'] = { interfaces:['org.alljoyn.marshal_test'] };
	var testSignal = AJ.signal('/test8', {test8:'org.alljoyn.marshal_test'});
	testSignal.send();
    print("test8 sent signal");
}

AJ.onAttach = function()
{
    print("AJ.onAttach");
    AJ.findService('org.alljoyn.marshal_test', test);
}

AJ.onDetach = function()
{
    print("AJ.onDetach");
}

