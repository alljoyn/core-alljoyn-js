# Copyright (c) 2014 AllSeen Alliance. All rights reserved.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

from distutils.core import setup, Extension
import os.path

variant = 'debug'
distdir = os.path.abspath('../../alljoyn/build/linux/x86_64/debug/dist/') + '/'

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
                           distdir + 'cpp/lib/BundledRouter.o',
                           distdir + 'cpp/lib/libajrouter.a',
                           distdir + 'cpp/lib/liballjoyn.a',
                           distdir + 'cpp/lib/liballjoyn_about.a',
                       ],
                       libraries = ['crypto', 'ssl'],
                                       
                       sources = ['ajs_console.cc', 'ajs_pyconsole.cc'])

setup(name='AJS_Console',
      version='0.0',
      description='AllJoyn.JS Console',
      url='http://www.allseenaliance.org/',
      ext_modules=[thismodule])
