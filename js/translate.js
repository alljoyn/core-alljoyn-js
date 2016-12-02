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

AJ.translations = {
    en: { },
    fr: { "red":"rouge",
          "green":"vert",
          "blue":"bleu",
          "yellow":"jaune" },
    de: { "red":"rot",
          "green":"grün",
          "blue":"blau",
          "yellow":"gelb" }
};

AJ.store("DefaultLanguage", "en");
print(AJ.translate("red"));
print(AJ.translate("red", "fr"));
AJ.store("DefaultLanguage", "de");
print(AJ.translate("red"));
print(AJ.translate("green"));
print(AJ.translate("purple"));
print(AJ.load("DeviceName"));