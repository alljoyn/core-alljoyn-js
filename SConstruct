# *****************************************************************************
#  Copyright (c) 2014, AllSeen Alliance. All rights reserved.
#
#     Permission to use, copy, modify, and/or distribute this software for any
#     purpose with or without fee is hereby granted, provided that the above
#     copyright notice and this permission notice appear in all copies.
#
#     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
#     WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
#     MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
#     ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
#     WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
#     ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
#     OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
# *****************************************************************************

import platform
import os
import string

#
# Initialize our build environment
#
if platform.system() == 'Linux':
    default_target = 'linux'
    default_msvc_version = None
elif platform.system() == 'Windows':
    default_target = 'win32'
    default_msvc_version = '11.0'

vars = Variables()

# Common build variables
vars.Add(EnumVariable('TARG', 'Target platform variant', default_target, allowed_values=('win32', 'linux')))
vars.Add(EnumVariable('VARIANT', 'Build variant', 'debug', allowed_values=('debug', 'release')))
vars.Add(PathVariable('GTEST_DIR', 'The path to googletest sources', os.environ.get('GTEST_DIR'), PathVariable.PathIsDir))
vars.Add(EnumVariable('WS', 'Whitespace Policy Checker', 'off', allowed_values=('check', 'detail', 'fix', 'off')))
vars.Add(EnumVariable('FORCE32', 'Force building 32 bit on 64 bit architecture', 'false', allowed_values=('false', 'true')))
vars.Add(EnumVariable('POOL_MALLOC', 'Use pool based memory allocation - default is native malloc', 'false', allowed_values=('false', 'true')))
vars.Add(EnumVariable('DUKTAPE_SEPARATE', 'Use seperate rather than combined duktape source files', 'false', allowed_values=('false', 'true')))

if default_msvc_version:
    vars.Add(EnumVariable('MSVC_VERSION', 'MSVC compiler version - Windows', default_msvc_version, allowed_values=('8.0', '9.0', '10.0', '11.0', '11.0Exp')))

if ARGUMENTS.get('TARG', default_target) == 'win32':
    msvc_version = ARGUMENTS.get('MSVC_VERSION')
    env = Environment(variables = vars, MSVC_VERSION=msvc_version, TARGET_ARCH='x86')
else:
    env = Environment(variables = vars)

Help(vars.GenerateHelpText(env))

# Allows preprocessor defines on the command line
#
# define="FOO=1" define="BAR=2"
#
cppdefines = []
for key, value in ARGLIST:
   if key == 'define':
       cppdefines.append(value)

env.Append(CPPDEFINES=cppdefines)

#
# Where to find stuff
#

if ((os.environ.has_key('AJTCL_ROOT'))):
    env['ajtcl_root'] = os.environ.get('AJTCL_ROOT')
else:
    env['ajtcl_root'] = Dir('../ajtcl').abspath

if ((os.environ.has_key('SVCS_ROOT'))):
    env['svcs_root'] = os.environ.get('SVCS_ROOT')
else:
    env['svcs_root'] = Dir('../../services/base_tcl').abspath

if ARGUMENTS.get('DUKTAPE_DIST', '') != '':
    env['duktape_dist'] = ARGUMENTS.get('DUKTAPE_DIST');
else:
    if ((os.environ.has_key('DUKTAPE_DIST'))):
        env['duktape_dist'] = os.environ.get('DUKTAPE_DIST')
    else:
        env['duktape_dist'] = Dir('./external/duktape/dist').abspath

if not(os.path.isdir(env['duktape_dist'])):
    print "Duktape distribution dir (DUKTAPE_DIST) not set or invalid"
    Exit(1)

####################################
# 
# Platform and target setup
#
####################################

CPU=platform.machine()

if env['TARG'] == 'win32':
    env.Append(LIBS=['wsock32', 'advapi32'])
    # Compiler flags
    env.Append(CFLAGS=['/nologo'])
    env.Append(CPPDEFINES=['_CRT_SECURE_NO_WARNINGS', 'snprintf=_snprintf'])
    # Linker flags
    env.Append(LINKFLAGS=['/NODEFAULTLIB:libcmt.lib'])
    # Target variable
    env['os'] = 'win32'
    # Debug/Release variants
    if env['VARIANT'] == 'debug':
       # Compiler flags for DEBUG builds
       env.Append(CPPDEFINES=['_DEBUG', ('_ITERATOR_DEBUG_LEVEL', 2)])
       # Linker flags for DEBUG builds
       env.Append(CFLAGS=['/J', '/W3', '/LD', '/MD', '/Z7', '/Od'])
       env.Append(LINKFLAGS=['/debug'])
    else:
       # Compiler flags for RELEASE builds
       env.Append(CPPDEFINES=['NDEBUG'])
       env.Append(CPPDEFINES=[('_ITERATOR_DEBUG_LEVEL', 0)])
       env.Append(CFLAGS=['/MD', '/Gy', '/O1', '/Ob2', '/W3'])
       # Linker flags for RELEASE builds
       env.Append(LINKFLAGS=['/opt:ref'])
 
if env['TARG'] == 'linux':
    if os.environ.has_key('CROSS_PREFIX'):
        env.Replace(CC = os.environ['CROSS_PREFIX'] + 'gcc')
        env.Replace(CXX = os.environ['CROSS_PREFIX'] + 'g++')
        env.Replace(LINK = os.environ['CROSS_PREFIX'] + 'gcc')
        env.Replace(AR = os.environ['CROSS_PREFIX'] + 'ar')
        env.Replace(RANLIB = os.environ['CROSS_PREFIX'] + 'ranlib')
        env['ENV']['STAGING_DIR'] = os.environ.get('STAGING_DIR', '')

    if os.environ.has_key('CROSS_PATH'):
        env['ENV']['PATH'] = ':'.join([ os.environ['CROSS_PATH'], env['ENV']['PATH'] ] )

    if os.environ.has_key('CROSS_CFLAGS'):
        env.Append(CFLAGS=os.environ['CROSS_CFLAGS'].split())

    if os.environ.has_key('CROSS_LINKFLAGS'):
        env.Append(LINKFLAGS=os.environ['CROSS_LINKFLAGS'].split())

    # Platform libraries
    env.Append(LIBS = ['libm', 'libcrypto', 'libpthread', 'librt'])
    # Compiler flags
    env.Append(CFLAGS = [
               '-std=gnu99',
               '-Wall',
               '-Wformat=0',
               '-fstrict-aliasing'])

    if env['FORCE32'] == 'true':
        env.Append(CFLAGS = ['-m32'])
        env.Append(LINKFLAGS=['-m32'])

    # Target variable
    env['os'] = 'linux'
    # Debug/Release Variants
    if env['VARIANT'] == 'debug':
        env.Append(CFLAGS=['-g'])
        env.Append(CFLAGS=['-ggdb'])
        env.Append(CFLAGS=['-O0'])
        env.Append(CPPDEFINES=['AJ_DEBUG_RESTRICT=5'])
        env.Append(CPPDEFINES=['DBGAll'])
    else:
        env.Append(CPPDEFINES=['NDEBUG'])
        env.Append(CFLAGS=['-Os'])
        env.Append(LINKFLAGS=['-s'])

#######################################################
# Compile time options for duktape
#######################################################
env.Append(CPPDEFINES=['DUK_OPT_DPRINT_COLORS'])
env.Append(CPPDEFINES=['DUK_OPT_DEBUG_BUFSIZE=256'])
env.Append(CPPDEFINES=['DUK_OPT_HAVE_CUSTOM_H'])
env.Append(CPPDEFINES=['DUK_OPT_NO_FILE_IO'])
env.Append(CPPDEFINES=['DUK_OPT_FORCE_ALIGN=4'])

# Additional duktape options when building for debug mode
if env['VARIANT'] == 'debug':
    env.Append(CPPDEFINES=['DUK_OPT_ASSERTIONS'])
    #env.Append(CPPDEFINES=['DUK_OPT_DEBUG'])

# AllJoyn.js added defines
env.Append(CPPDEFINES=['DUK_OPT_SHORT_SIZES'])

if env['POOL_MALLOC'] == 'false':
    env.Append(CPPDEFINES=['AJS_USE_NATIVE_MALLOC'])

#######################################################
# Services defines
#######################################################
env.Append(CPPDEFINES=['CONFIG_SERVICE'])
env.Append(CPPDEFINES=['NOTIFICATION_SERVICE_PRODUCER'])
env.Append(CPPDEFINES=['CONTROLPANEL_SERVICE'])
env.Append(CPPDEFINES=['ONBOARDING_SERVICE'])

#######################################################
# AJS defines
#######################################################
env.Append(CPPDEFINES=['ALLJOYN_JS'])
env.Append(CPPDEFINES=['BIG_HEAP'])

#######################################################
# Include paths
#######################################################
env.Append(CPPPATH=['.'])
env.Append(CPPPATH=[env['os']])
env.Append(CPPPATH=[env['ajtcl_root'] + '/inc'])
env.Append(CPPPATH=[env['ajtcl_root'] + '/target/' + env['os']])
env.Append(CPPPATH=[env['svcs_root'] + '/config/inc'])
env.Append(CPPPATH=[env['svcs_root'] + '/services_common/inc'])
env.Append(CPPPATH=[env['svcs_root'] + '/notification/inc'])
env.Append(CPPPATH=[env['svcs_root'] + '/controlpanel/inc'])
env.Append(CPPPATH=[env['svcs_root'] + '/onboarding/inc'])
env.Append(CPPPATH=[env['svcs_root'] + '/sample_apps/AppsCommon/inc'])

if env['DUKTAPE_SEPARATE'] == 'true':
    env.Append(CPPPATH=[env['duktape_dist'] + '/src-separate'])
else:
    env.Append(CPPPATH=[env['duktape_dist'] + '/src'])

#
# Libraries
# 
env.Append(LIBPATH = env['ajtcl_root'])

if env['PLATFORM'] == 'win32':
    env.Append(LIBS = ['ajtcl_st'])

if env['PLATFORM'] == 'posix':
    env.Append(LIBS = ['libajtcl'])



progs = env.SConscript('SConscript', 'env', variant_dir='build/$VARIANT', duplicate=0)

env.Install('.', progs)
