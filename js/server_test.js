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

AJ.objectDefinition['/marshal_test'] = {
    interfaces:['org.alljoyn.marshal_test']
};

AJ.onAttach = function()
{
    AJ.addMatch('org.alljoyn.marshal_test', 'test8');
    print("AJ.onAttach");
}

AJ.onDetach = function()
{
    print("AJ.onDetach");
}

AJ.onSignal = function()
{
	if (this.member == 'test8') {
		print("Signal Received " + arguments[0]);
	}
}

AJ.onMethodCall = function()
{
    if (this.member == 'test') {
        print(JSON.stringify(arguments[0]));
        this.reply("Test 1 Passed");
    } else if (this.member == 'test2') {
        print(JSON.stringify(arguments[0]));
        this.reply("Test 2 Passed");
	} else if (this.member == 'test3') {
		print(JSON.stringify(arguments[0]));
		this.reply("Test 3 Passed");
	} else if(this.member == 'test4') {
		print(JSON.stringify(arguments[0]));
		this.reply("Test 4 Passed");
	} else if(this.member == 'test5') {
		print(JSON.stringify(arguments[0]));
		this.reply("Test 5 Passed");
	} else if(this.member == 'test6') {
		print(JSON.stringify(arguments[0]));
		this.reply("Test 6 Passed");
	}else if(this.member == 'test7') {
		print(Duktape.enc("jx", arguments[0]));
		this.reply("Test 7 Passed");

    } else {
        throw('rejected');
    }
}
