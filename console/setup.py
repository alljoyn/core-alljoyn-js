# # 
# Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
# Source Project Contributors and others.
# 
# All rights reserved. This program and the accompanying materials are
# made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0

from distutils.core import setup, Extension
import os
import sys
import platform

platform = platform.system()

aj_path = os.environ.get('ALLJOYN_DISTDIR')

if aj_path == None:
    print ('ALLJOYN_DISTDIR must be defined to build')
    quit()


if platform == 'Linux':
    distdir = os.path.abspath(aj_path) + '/'
    thismodule = Extension('AJSConsole',
                           define_macros = [('QCC_OS_GROUP_POSIX', None),
                                            ('_GLIBCXX_USE_C99_FP_MACROS_DYNAMIC', None)],
                           extra_compile_args = ['-Wall',
                                                 '-Werror=non-virtual-dtor',
                                                 '-pipe',
                                                 '-std=gnu++0x',
                                                 '-fno-exceptions',
                                                 '-fno-strict-aliasing',
                                                 '-fno-asynchronous-unwind-tables',
                                                 '-fno-unwind-tables',
                                                 '-ffunction-sections',
                                                 '-fdata-sections',
                                                 '-Wno-long-long',
                                                 '-Wno-deprecated',
                                                 '-Wno-unknown-pragmas'],
                           include_dirs = [distdir + 'cpp/inc',
                                           distdir + 'about/inc'],
                           library_dirs = [distdir + 'cpp/lib',
                                           distdir + 'about/lib'],
                           extra_objects = [
                               distdir + 'cpp/lib/libajrouter.a',
                               distdir + 'cpp/lib/liballjoyn.a',
                               distdir + 'cpp/lib/liballjoyn_about.a',
                           ],
                           libraries = ['crypto', 'ssl', 'rt'],
                           sources = ['ajs_console.cc', 'ajs_pyconsole.cc'])

elif platform == 'Windows':
    # Windows must be built in release mode due to python being built in release mode
    distdir = os.path.abspath(aj_path) + '/'
    thismodule = Extension('AJSConsole',
                           define_macros = [('QCC_OS_GROUP_WINDOWS', None),
                                            ('QCC_CPU_X86_64', None),
                                            ('_ITERATOR_DEBUG_LEVEL', 0),
                                            ('_GLIBCXX_USE_C99_FP_MACROS_DYNAMIC', None),
                                            ('_CRT_SECURE_NO_WARNINGS', None)
                                            ],
                           extra_compile_args = ['/vmm', '/vmg', '/Zi', '/EHsc'],
                           include_dirs = [distdir + 'cpp/inc',
                                           distdir + 'about/inc'],
                           library_dirs = [distdir + 'cpp/lib',
                                           distdir + 'about/lib'],
                           extra_objects = [
                               distdir + 'cpp/lib/ajrouter.lib',
                               distdir + 'cpp/lib/alljoyn.lib',
                               distdir + 'cpp/lib/alljoyn_about.lib',
                           ],
                           libraries = ['setupapi','user32','winmm','ws2_32','iphlpapi','secur32','Advapi32','crypt32','bcrypt','ncrypt'],
                           sources = ['ajs_console.cc', 'ajs_pyconsole.cc'])

setup(name='AJS_Console',
      version='0.0',
      description='AllJoyn.JS Console',
      url='http://www.allseenaliance.org/',
      ext_modules=[thismodule])