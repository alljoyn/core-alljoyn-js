import os
import platform

default_ajtcl_dist = '../ajtcl/dist'
default_svcs_dist = '../../services/base_tcl/dist'
exclude_onboarding = False

if platform.system() == 'Linux':
    default_target = 'linux'
    root = os.environ.get('STAGING_DIR', '/')
    if os.path.exists(root + 'usr/lib/libajtcl.so'):
        default_ajtcl_dist = ''
    if os.path.exists(root + 'usr/lib/libajtcl_services.so'):
        default_svcs_dist = ''
    exclude_onboarding = True
elif platform.system() == 'Windows':
    default_target = 'win32'
elif platform.system() == 'Darwin':
    default_target = 'darwin'

#######################################################
# Build variables
#######################################################
target_options = [ t.split('.')[1] for t in os.listdir('.') if t.startswith('SConscript.') ]
vars = Variables()
vars.Add(BoolVariable('V',                  'Build verbosity',                     False))
vars.Add(EnumVariable('TARG',               'Target platform variant',             os.environ.get('AJ_TARG',               default_target),     allowed_values = target_options))
vars.Add(EnumVariable('VARIANT',            'Build variant',                       os.environ.get('AJ_VARIANT',            'debug'),            allowed_values = ('debug', 'release')))
vars.Add(EnumVariable('WS',                 'Whitespace Policy Checker',           os.environ.get('AJ_WS',                 'check'),            allowed_values = ('check', 'detail', 'fix', 'off')))
vars.Add(BoolVariable('EXCLUDE_ONBOARDING', 'Exclude Onboarding support',          os.environ.get('AJ_EXCLUDE_ONBOARDING', exclude_onboarding)))
vars.Add(PathVariable('AJTCL_DIST',         'Path to AJTCL',                       os.environ.get('AJ_AJTCL_DIST',         default_ajtcl_dist), PathVariable.PathIsDir))
vars.Add(PathVariable('SVCS_DIST',          'Path to AJTCL Base Services Library', os.environ.get('AJ_SVCS_DIST',          default_svcs_dist),  PathVariable.PathIsDir))
vars.Add(PathVariable('DUKTAPE_SRC',        'Path to Duktape generated source',    os.environ.get('AJ_DUCTAPE_SRC'),                            PathVariable.PathIsDir))
vars.Add(BoolVariable('EXT_STRINGS',        'Enable external string support',      os.environ.get('AJ_EXT_STRINGS',        False)))
vars.Add(BoolVariable('POOL_MALLOC',        'Use pool based memory allocation',    os.environ.get('AJ_POOL_MALLOC',        False)))
vars.Add(BoolVariable('SHORT_SIZES',        'Use 16 bit sizes and pointers - only applies when using pool malloc', os.environ.get('AJ_SHORT_SIZES', True)))
vars.Add(BoolVariable('DUK_DEBUG',          'Turn on duktape logging and print debug messages', os.environ.get('AJ_DUK_DEBUG',   False)))
vars.Add('CC',  'C Compiler override')
vars.Add('CXX', 'C++ Compiler override')

#######################################################
# Initialize our build environment
#######################################################
env = Environment(variables = vars)
Help(vars.GenerateHelpText(env))

#######################################################
# Setup non-verbose output
#######################################################
if not env['V']:
    env.Replace( CCCOMSTR =     '\t[CC]      $SOURCE',
                 SHCCCOMSTR =   '\t[CC-SH]   $SOURCE',
                 CXXCOMSTR =    '\t[CXX]     $SOURCE',
                 SHCXXCOMSTR =  '\t[CXX-SH]  $SOURCE',
                 LINKCOMSTR =   '\t[LINK]    $TARGET',
                 SHLINKCOMSTR = '\t[LINK-SH] $TARGET',
                 JAVACCOMSTR =  '\t[JAVAC]   $SOURCE',
                 JARCOMSTR =    '\t[JAR]     $TARGET',
                 ARCOMSTR =     '\t[AR]      $TARGET',
                 RANLIBCOMSTR = '\t[RANLIB]  $TARGET',
                 INSTALLSTR =   '\t[INSTALL] $TARGET',
                 WSCOMSTR =     '\t[WS]      $WS' )

#######################################################
# Compilation defines
#######################################################
env.Append(CPPDEFINES = [
    # Base Services defines
    'CONFIG_SERVICE',
    'CONTROLPANEL_SERVICE',
    'NOTIFICATION_SERVICE_CONSUMER',
    'NOTIFICATION_SERVICE_PRODUCER',
    # Duktape defines
    'DUK_FORCE_ALIGNED_ACCESS',
    ( 'DUK_OPT_DEBUG_BUFSIZE', '256' ),
    'DUK_OPT_DPRINT_COLORS',
    ( 'DUK_OPT_FORCE_ALIGN', '4' ),
    'DDUK_OPT_LIGHTFUNC_BUILTINS',
    'DUK_OPT_HAVE_CUSTOM_H',
    'DUK_OPT_NO_FILE_IO',
    'DUK_OPT_SEGFAULT_ON_PANIC',
    'DUK_OPT_SHORT_LENGTHS',

    # Enable timeout checks
    'DUK_OPT_INTERRUPT_COUNTER',
    ( '"DUK_OPT_EXEC_TIMEOUT_CHECK(u)"', '"AJS_ExecTimeoutCheck(u)"'),

    # AllJoyn-JS defines
    'ALLJOYN_JS',
    'BIG_HEAP' ])

if not env['EXCLUDE_ONBOARDING']:
    env.Append(CPPDEFINES = 'ONBOARDING_SERVICE')
if env['VARIANT'] == 'release':
    env.Append(CPPDEFINES = [ 'NDEBUG' ])
else:
    env.Append(CPPDEFINES = [ 'DUK_OPT_ASSERTIONS',
                              ( 'AJ_DEBUG_RESTRICT', '5' ),
                              'DBGAll',
                              'DUK_OPT_DEBUGGER_SUPPORT',
                              'DUK_OPT_INTERRUPT_COUNTER',
                              'DUK_CMDLINE_DEBUGGER_SUPPORT' ])
if env['DUK_DEBUG']:
    env.Append(CPPDEFINES = [ 'DBG_PRINT_CHUNKS',
                              'DUK_OPT_DEBUG',
                              'DUK_OPT_DPRINT' ])

if not env['POOL_MALLOC']:
    env.Append(CPPDEFINES=['AJS_USE_NATIVE_MALLOC'])
elif env['SHORT_SIZES']:
    env.Append(CPPDEFINES = [ 'DUK_OPT_REFCOUNT16',
                              'DUK_OPT_STRHASH16',
                              'DUK_OPT_STRLEN16',
                              'DUK_OPT_BUFLEN16',
                              'DUK_OPT_OBJSIZES16',
                              'DUK_OPT_HEAPPTR16',
                              ('"DUK_OPT_HEAPPTR_ENC16(u,p)"', '"AJS_EncodePtr16(u,p)"'),
                              ('"DUK_OPT_HEAPPTR_DEC16(u,x)"', '"AJS_DecodePtr16(u,x)"') ])

if env['EXT_STRINGS']:
    env.Append(CPPDEFINES = [ 'DUK_OPT_EXTERNAL_STRINGS',
                              ('"DUK_OPT_EXTSTR_INTERN_CHECK(u,p,l)"', '"AJS_ExternalStringCheck(u,p,l)"'),
                              ('"DUK_OPT_EXTSTR_FREE(u,p)"', '"AJS_ExternalStringFree(u,p)"'),
                              'DUK_OPT_STRTAB_CHAIN',
                              'DUK_OPT_STRTAB_CHAIN_SIZE=128' ])

#######################################################
# Setup references to dependent projects
#######################################################
if env.has_key('AJTCL_DIST'):
    env.Append(CPPPATH = [ Dir(env['AJTCL_DIST'] + '/include') ])
    env.Append(LIBPATH = [ Dir(env['AJTCL_DIST'] + '/lib') ])

if env.has_key('SVCS_DIST'):
    env.Append(CPPPATH = [ Dir(env['SVCS_DIST'] + '/include') ])
    env.Append(LIBPATH = [ Dir(env['SVCS_DIST'] + '/lib') ])

external_path = Glob('external/duktape*')
if external_path:
    external_path = str(external_path[-1])
else:
    external_path = None

if env.has_key('DUKTAPE_SRC'):
    env.Append(CPPPATH = Dir(env['DUKTAPE_SRC']))
    env['duktape_src'] = File(env['DUKTAPE_SRC'] + '/duktape.c')
elif external_path and os.path.exists(external_path + '/src/duktape.c'):
    env.Append(CPPPATH = Dir(external_path + '/src/'))
    env['duktape_src'] = File(external_path + '/src/duktape.c')
else:
    env.Append(CPPPATH = Dir('/usr/src/duktape/'))
    env['duktape_src'] = File('/usr/src/duktape/duktape.c')

#######################################################
# Additional libraries
#######################################################
env.Append(LIBS = ['ajtcl_services', 'ajtcl'])

#######################################################
# Process commandline defines
#######################################################
env.Append(CPPDEFINES = [ v for k, v in ARGLIST if k.lower() == 'define' ])

#######################################################
# Whitespace checker
#######################################################
# Set the location of the uncrustify config file
import sys
sys.path.append(os.getcwd() + '/tools')
import whitespace

def wsbuild(target, source, env):
    return whitespace.main([ env['WS'], os.getcwd() + '/tools/ajuncrustify.cfg' ])

if env['WS'] != 'off':
    env.Command('#ws_ajtcl', '#dist', Action(wsbuild, '$WSCOMSTR'))

#######################################################
# Setup target specific options and build AllJoyn portion of aj_duk
#######################################################
Export('env')
env.SConscript('SConscript.$TARG')
env.SConscript('src/SConscript', variant_dir='#build/$VARIANT', duplicate = 0)
