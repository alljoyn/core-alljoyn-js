import os
import platform
import re
import urlparse

#######################################################
# Default Duktape version
#######################################################
duktape_version = '1.2.1'
duktape_md5sum = '86362304a347fd88bcbcdfc00ff3663c'

duktape_tarball = 'duktape-%s.tar.xz' % duktape_version
duktape_urlbase = 'http://duktape.org/'
duktape_default_url = duktape_urlbase + duktape_tarball

#######################################################
# Custom Configure functions
#######################################################
def CheckCommand(context, cmd):
    context.Message('Checking for %s command...' % cmd)
    r = WhereIs(cmd)
    context.Result(r is not None)
    return r

def CheckAJLib(context, ajlib, ajheader, sconsvarname, ajdistpath, subdist, incpath, ext):
    prog = "#include <%s>\nint main(void) { return 0; }" % ajheader
    context.Message('Checking for AllJoyn library %s...' % ajlib)
    distpath = os.path.join(ajdistpath, subdist)
    prevLIBS = list(context.env.get('LIBS', []))
    prevLIBPATH = list(context.env.get('LIBPATH', []))
    prevCPPPATH = list(context.env.get('CPPPATH', []))

    # Check if library is in standard system locations
    context.env.Append(LIBS = [ajlib])
    defpath = ''  # default path is a system directory
    if not context.TryLink(prog, ext):
        # Check if library is in project default location
        context.env.Append(LIBPATH = os.path.join(distpath, 'lib'),
                           CPPPATH = os.path.join(distpath, incpath))
        if context.TryLink(prog, ext):
            defpath = str(Dir(ajdistpath))  # default path is the dist directory
        # Remove project default location from LIBPATH and CPPPATH
        context.env.Replace(LIBPATH = prevLIBPATH, CPPPATH = prevCPPPATH)

    vars = Variables()
    vars.Add(PathVariable(sconsvarname,
                          'Path to %s dist directory' % ajlib,
                          os.environ.get('AJ_%s' % sconsvarname, defpath),
                          lambda k, v, e : v == '' or PathVariable.PathIsDir(k, v, e)))
    vars.Update(context.env)
    Help(vars.GenerateHelpText(context.env))

    # Get the actual library path to use ('' == system path, may be same as distpath)
    libpath = context.env.get(sconsvarname, '')

    if libpath is not '':
        libpath = str(context.env.Dir(libpath))
        # Add the user specified (or distpath) to LIBPATH and CPPPATH
        context.env.Append(LIBPATH = os.path.join(libpath, subdist, 'lib'),
                           CPPPATH = os.path.join(libpath, subdist, incpath))

    # The real test for the library
    r = context.TryLink(prog, ext)
    if not r:
        context.env.Replace(LIBS = prevLIBS, LIBPATH = prevLIBPATH, CPPPATH = prevCPPPATH)
    context.Result(r)
    return r

def CheckAJCLib(context, ajlib, ajheader, sconsvarname, ajdistpath):
    return CheckAJLib(context, ajlib, ajheader, sconsvarname, ajdistpath, '', 'include', '.c')

#######################################################
# Initialize our build environment
#######################################################
env = Environment(tools = ['default', 'JSDoc', 'URLDownload', 'Unpack'],
                  toolpath = ['tools/scons', 'external/scons'],
                  URLDOWNLOAD_USEURLFILENAME = False)
Export('env', 'CheckAJLib')

#######################################################
# Default target platform
#######################################################
if platform.system() == 'Linux':
    default_target = 'linux'
elif platform.system() == 'Windows':
    default_target = 'win32'
elif platform.system() == 'Darwin':
    default_target = 'darwin'

#######################################################
# Build variables
#######################################################
target_options = [ t.split('.')[-1] for t in os.listdir('.') if re.match('^SConscript\.target\.[-_0-9A-Za-z]+$', t) ]

vars = Variables()
vars.Add(BoolVariable('V',                  'Build verbosity',                     False))
vars.Add(EnumVariable('TARG',               'Target platform variant',             os.environ.get('AJ_TARG',               default_target), allowed_values = target_options))
vars.Add(EnumVariable('VARIANT',            'Build variant',                       os.environ.get('AJ_VARIANT',            'debug'),        allowed_values = ('debug', 'release')))
vars.Add(BoolVariable('EXT_STRINGS',        'Enable external string support',      os.environ.get('AJ_EXT_STRINGS',        False)))
vars.Add(BoolVariable('POOL_MALLOC',        'Use pool based memory allocation',    os.environ.get('AJ_POOL_MALLOC',        False)))
vars.Add(BoolVariable('SHORT_SIZES',        'Use 16 bit sizes and pointers - only when POOL_MALLOC == True', os.environ.get('AJ_SHORT_SIZES', True)))
vars.Add(BoolVariable('DUK_DEBUG',          'Turn on duktape logging and print debug messages', os.environ.get('AJ_DUK_DEBUG',   False)))
vars.Add(BoolVariable('CONSOLE_LOCKDOWN',   'Removes all debugger and console code', os.environ.get('AJ_CONSOLE_LOCKDOWN', False)))
vars.Add('DUKTAPE_SRC', 'URL/Path to Duktape generated source', os.environ.get('AJ_DUCTAPE_SRC', duktape_default_url))
vars.Add('CC',  'C Compiler override')
vars.Add('CXX', 'C++ Compiler override')
vars.Update(env)
Help(vars.GenerateHelpText(env))

#######################################################
# Setup non-verbose output
#######################################################
if not env['V']:
    env.Replace( CCCOMSTR =          '\t[CC]       $SOURCE',
                 SHCCCOMSTR =        '\t[CC-SH]    $SOURCE',
                 CXXCOMSTR =         '\t[CXX]      $SOURCE',
                 SHCXXCOMSTR =       '\t[CXX-SH]   $SOURCE',
                 LINKCOMSTR =        '\t[LINK]     $TARGET',
                 SHLINKCOMSTR =      '\t[LINK-SH]  $TARGET',
                 JAVACCOMSTR =       '\t[JAVAC]    $SOURCE',
                 JARCOMSTR =         '\t[JAR]      $TARGET',
                 ARCOMSTR =          '\t[AR]       $TARGET',
                 ASCOMSTR =          '\t[AS]       $TARGET',
                 RANLIBCOMSTR =      '\t[RANLIB]   $TARGET',
                 INSTALLSTR =        '\t[INSTALL]  $TARGET',
                 JSDOCCOMSTR =       '\t[JSDOC]    $TARGET.dir',
                 UNPACKCOMSTR =      '\t[UNPACK]   $SOURCE',
                 URLDOWNLOADCOMSTR = '\t[DOWNLOAD] $SOURCE',
                 WSCOMSTR =          '\t[WS]       $WS' )

#######################################################
# Load target setup
#######################################################
env['build'] = True
env['build_shared'] = False
env['build_unit_tests'] = True

env.SConscript('SConscript.target.$TARG')

jsenv = env.Clone()
Export('jsenv')

#######################################################
# Check dependencies
#######################################################
config = Configure(jsenv, custom_tests = { 'CheckCommand' : CheckCommand,
                                           'CheckAJLib' : CheckAJCLib })
found_ws = config.CheckCommand('uncrustify')
found_jsdoc = config.CheckCommand('jsdoc')

dep_libs = [
    config.CheckAJLib('ajtcl',          'ajtcl/aj_bus.h',                 'AJTCL_DIST', '../ajtcl/dist'),
    config.CheckAJLib('ajtcl_services', 'ajtcl/services/ConfigService.h', 'SVCS_DIST',  '../../services/base_tcl/dist')
]

config_check_svc_stub = """
int AJSVC_PropertyStore_LoadAll() { return 0; }
int AJSVC_PropertyStore_GetValueForLang() { return 0; }
int AJSVC_PropertyStore_GetFieldIndex() { return 0; }
int AJSVC_PropertyStore_Reset() { return 0; }
int AJSVC_PropertyStore_GetValue() { return 0; }
int AJSVC_PropertyStore_ReadAll() { return 0; }
int AJSVC_PropertyStore_GetLanguageIndex() { return 0; }
int AJSVC_PropertyStore_GetFieldName() { return 0; }
int AJSVC_PropertyStore_SaveAll() { return 0; }
int AJSVC_PropertyStore_GetMaxValueLength() { return 0; }
int AJSVC_PropertyStore_Update() { return 0; }
"""
include_onboarding = config.CheckFunc('AJOBS_ClearInfo', config_check_svc_stub)

jsenv = config.Finish()

#######################################################
# Find Duktape source
#######################################################
tarball = None
tarball_match = re.match('.*/(?P<basename>.*)(?P<suffix>\.(tar(\.(gz|gzip|bz2?|bzip2?|xz))?|tgz|tbz|txz|zip))$',
                         jsenv['DUKTAPE_SRC'])

if tarball_match:
    if bool(urlparse.urlparse(jsenv['DUKTAPE_SRC']).netloc):
        # Download a tarball from a URL
        tarball = jsenv.URLDownload('#external/dl/' + duktape_tarball, jsenv['DUKTAPE_SRC'])
    else:
        # Use an existing tarball on the local filesystem
        tarball = [jsenv.File(jsenv['DUKTAPE_SRC'])]

    duktape_srcdir = '#external/%s/src' % tarball_match.groupdict()['basename']
    jsenv.Append(UNPACK = {'EXTRACTDIR': jsenv.Dir('#external') })
    jsenv['duktape_src'], hfile = jsenv.Unpack(tarball, UNPACKLIST = [ os.path.join(duktape_srcdir, 'duktape.c'),
                                                                       os.path.join(duktape_srcdir, 'duktape.h') ])
    if os.path.basename(str(tarball[0])) == duktape_tarball:
        # Using default tarball -- verify MD5SUM of file
        def md5sum_check(target, source, env):
            if source[0].get_content_hash() != duktape_md5sum:
                return 'MD5SUM mismatch for %s (%s vs %s)' % (source[0], source[0].get_content_hash(), duktape_md5sum)
            return None
        jsenv.AddPreAction([jsenv['duktape_src'], hfile], Action(md5sum_check, '\t[MD5SUM]   $SOURCE') )

    # Tell SCons to include the extracted tarball code when cleaning.
    duktape_dir = jsenv.Dir(os.path.dirname(duktape_srcdir))
    jsenv.Clean(os.path.dirname(str(duktape_dir)), duktape_dir)
else:
    duktape_srcdir = jsenv['DUKTAPE_SRC']
    jsenv['duktape_src'] = jsenv.File(os.path.join(duktape_srcdir, 'duktape.c'))

jsenv.Append(CPPPATH = jsenv.Dir(duktape_srcdir))


#######################################################
# Compilation defines
#######################################################
jsenv.Append(CPPDEFINES = [
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
    'DDUK_OPT_FASTINT',
    'DUK_OPT_HAVE_CUSTOM_H',
    'DUK_OPT_NO_FILE_IO',
    'DUK_OPT_SEGFAULT_ON_PANIC',
    'DUK_OPT_SHORT_LENGTHS',
    'DUK_OPT_DEBUGGER_SUPPORT',
    'DUK_OPT_INTERRUPT_COUNTER',
    'DUK_CMDLINE_DEBUGGER_SUPPORT',
    ( '"DUK_OPT_EXEC_TIMEOUT_CHECK(u)"', '"AJS_ExecTimeoutCheck(u)"'),
    # AllJoyn-JS defines
    'ALLJOYN_JS',
    'BIG_HEAP' ])

if include_onboarding:
    jsenv.Append(CPPDEFINES = 'ONBOARDING_SERVICE')
if jsenv['VARIANT'] == 'release':
    jsenv.Append(CPPDEFINES = [ 'NDEBUG' ])
else:
    jsenv.Append(CPPDEFINES = [ 'DUK_OPT_ASSERTIONS',
                              ( 'AJ_DEBUG_RESTRICT', '5' ),
                              'DBGAll' ])
if jsenv['DUK_DEBUG']:
    jsenv.Append(CPPDEFINES = [ 'DBG_PRINT_CHUNKS',
                              'DUK_OPT_DEBUG',
                              'DUK_OPT_DPRINT' ])

if not jsenv['POOL_MALLOC']:
    jsenv.Append(CPPDEFINES=['AJS_USE_NATIVE_MALLOC'])
elif jsenv['SHORT_SIZES']:
    jsenv.Append(CPPDEFINES = [ 'DUK_OPT_REFCOUNT16',
                              'DUK_OPT_STRHASH16',
                              'DUK_OPT_STRLEN16',
                              'DUK_OPT_BUFLEN16',
                              'DUK_OPT_OBJSIZES16',
                              'DUK_OPT_HEAPPTR16',
                              ('"DUK_OPT_HEAPPTR_ENC16(u,p)"', '"AJS_EncodePtr16(u,p)"'),
                              ('"DUK_OPT_HEAPPTR_DEC16(u,x)"', '"AJS_DecodePtr16(u,x)"') ])

if jsenv['EXT_STRINGS']:
    jsenv.Append(CPPDEFINES = [ 'DUK_OPT_EXTERNAL_STRINGS',
                              ('"DUK_OPT_EXTSTR_INTERN_CHECK(u,p,l)"', '"AJS_ExternalStringCheck(u,p,l)"'),
                              ('"DUK_OPT_EXTSTR_FREE(u,p)"', '"AJS_ExternalStringFree(u,p)"'),
                              'DUK_OPT_STRTAB_CHAIN',
                              'DUK_OPT_STRTAB_CHAIN_SIZE=128' ])

if jsenv['CONSOLE_LOCKDOWN'] :
    jsenv.Append(CPPDEFINES = [ 'AJS_CONSOLE_LOCKDOWN' ])

#######################################################
# Include path
#######################################################

#######################################################
# Process commandline defines
#######################################################
jsenv.Append(CPPDEFINES = [ v for k, v in ARGLIST if k.lower() == 'define' ])

#######################################################
# Setup target specific options and build AllJoyn portion of aj_duk
#######################################################
if not jsenv.GetOption('help') and not all(dep_libs):
    print '*** Missing required external libraries'
    Exit(1)

jsenv.SConscript('src/SConscript', variant_dir='#build/src/$VARIANT', duplicate = 0)
jsenv.SConscript('console/SConscript', variant_dir='#build/console/$VARIANT', duplicate = 0)


#######################################################
# Run the whitespace checker
#######################################################
# Set the location of the uncrustify config file
if found_ws:
    import sys
    sys.path.append(os.getcwd() + '/tools')
    import whitespace

    def wsbuild(target, source, env):
        return whitespace.main([ env['WS'], os.getcwd() + '/tools/ajuncrustify.cfg' ])

    vars = Variables()
    vars.Add(EnumVariable('WS', 'Whitespace Policy Checker', os.environ.get('AJ_WS', 'check'), allowed_values = ('check', 'detail', 'fix', 'off')))

    vars.Update(jsenv)
    Help(vars.GenerateHelpText(jsenv))

    if jsenv.get('WS', 'off') != 'off':
        jsenv.Command('#ws_ajtcl', '#dist', Action(wsbuild, '$WSCOMSTR'))

if found_jsdoc:
    doc_out = jsenv.JSDoc(jsenv.Dir('#dist/doc/jsdoc'), 'doc/jsdoc/jsdocs')
