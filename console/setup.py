# Copyright AllSeen Alliance. All rights reserved.
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
import platform

platform = platform.system()

if platform == 'Linux':
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
                               distdir + 'cpp/lib/libajrouter.a',
                               distdir + 'cpp/lib/liballjoyn.a',
                               distdir + 'cpp/lib/liballjoyn_about.a',
                           ],
                           libraries = ['crypto', 'ssl', 'rt'],
                           sources = ['ajs_console.cc', 'ajs_pyconsole.cc'])

elif platform == 'Windows':
    # Windows must be built in release mode due to python being built in release mode
    distdir = os.path.abspath('../../alljoyn/build/win7/x86_64/release/dist/') + '/'
    thismodule = Extension('AJSConsole',
                           define_macros = [('QCC_OS_GROUP_WINDOWS', None),
                                            ('QCC_CPU_X86_64', None),
                                            ('_ITERATOR_DEBUG_LEVEL', 0),
                                            ('_GLIBCXX_USE_C99_FP_MACROS_DYNAMIC', None)
                                            ],
                           extra_compile_args = ['/vmm', '/vmg', '/Zi'],
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