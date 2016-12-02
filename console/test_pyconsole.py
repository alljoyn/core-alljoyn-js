#!/usr/bin/env python
# # 
# Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
# Source Project Contributors and others.
# 
# All rights reserved. This program and the accompanying materials are
# made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0

import AJSConsole
import time

def cb(cbtype, *args):
    print cbtype, repr(args)

AJSConsole.SetCallback(cb)

print AJSConsole.Connect()
print AJSConsole.Eval('AJ.notification(AJ.notification.Warning, "Door is unlocked").send(100);')
print AJSConsole.Eval('print("Printing something");')
print AJSConsole.Eval('alert("Alerting");')
#print AJSConsole.Reboot()