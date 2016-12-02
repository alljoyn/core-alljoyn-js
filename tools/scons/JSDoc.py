# # 
#    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
#    Source Project Contributors and others.
#    
#    All rights reserved. This program and the accompanying materials are
#    made available under the terms of the Apache License, Version 2.0
#    which accompanies this distribution, and is available at
#    http://www.apache.org/licenses/LICENSE-2.0

#
import SCons.Builder
import SCons.Action
import os

def _jsdoc3_scanner(node, env, path):
    source = []
    if env.get('JSDOC_TEMPLATE'):
        # walk the JSDOC_TEMPLATE if specified dir and add the files in the
        # template dir to the source list.  This way documentation will be
        # rebuilt if a template file is modified
        template_dir = env.Dir('$JSDOC_TEMPLATE')
        for root, dir, filenames in os.walk(str(template_dir)):
            for f in filenames:
                source.append(os.path.abspath(os.path.join(root, f)))
    return source

def _jsdoc3_emitter(target, source, env):
    if len(target) > 1:
        print('scons: *** Only one target may be specified for the jsdoc3 Builder.')
        exit(1)
    if type(target[0]) != SCons.Node.FS.Dir:
        print('scons: *** Target MUST be a Dir node.')
        exit(1);
    outputDir = target[0];
    # SCons considers directory nodes upto date if they exist. Past the fist run
    # of the script the directory exist preventing it from generating a
    # dependency graph on subsiquent runs. By adding a file to the targets we
    # are able to workaround this issue. Since index.html is always updated with
    # the build date it was chosen.
    target[0] = env.File(str(target[0]) + '/index.html')
    # make sure the output directory is cleaned.
    env.Clean(env.File(source), outputDir)
    return target, source

def generate(env):
    jsdoc3_scanner = env.Scanner(name = 'jsdoc3_scanner',
                                 function = _jsdoc3_scanner)
    jsdoc3_action = SCons.Action.Action('${JSDOCCOM}', '${JSDOCCOMSTR}')
    jsdoc3_builder = SCons.Builder.Builder(
        action = jsdoc3_action,
        src_suffix = '$JSDOC_SUFFIX',
        emitter = _jsdoc3_emitter,
        source_scanner = jsdoc3_scanner
    )

    env.Append(BUILDERS = {
        'JSDoc': jsdoc3_builder,
    })

    env['JSDOCCOM'] = 'jsdoc ${JSDOC_FLAGS} ${_JSDOC_TEMPLATE_OPT} -d=${TARGET.dir} ${SOURCES}'
    # Suffixes/prefixes
    env['JSDOC_SUFFIX'] = '.js'
    # JSDoc 2 build flags
    env['JSDOC_FLAGS'] = ''
    # directory containing the publish.js and other template files.
    if env.get('JSDOC_TEMPLATE'):
        env['_JSDOC_TEMPLATE_OPT'] = '-t $(JSDOC_TEMPLATE}'


def exists(env):
    # This function must exist but is only called by ToolInitializer.  It is not
    # clear what causes ToolInitializer to be invoked so this function will be a
    # stub like for all other custom builders.
    return 1