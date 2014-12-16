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

import os

Import('env')

if env['DUKTAPE_SEPARATE'] == 'true':
    duktape_sources = Glob(env['duktape_dist'] + '/src-separate/*.c')
else:
    duktape_sources = Glob(env['duktape_dist'] + '/src/*.c')

svcs_sources = []
svcs_sources.append(Glob(env['svcs_root'] + '/services_common/src/*.c'))
svcs_sources.append(Glob(env['svcs_root'] + '/notification/src/NotificationCommon/*.c'))
svcs_sources.append(Glob(env['svcs_root'] + '/notification/src/NotificationProducer/*.c'))
svcs_sources.append(Glob(env['svcs_root'] + '/config/src/*.c'))
svcs_sources.append(Glob(env['svcs_root'] + '/controlpanel/src/CPSControllee/*.c'))
svcs_sources.append(Glob(env['svcs_root'] + '/controlpanel/src/CPSControllee/Common/*.c'))
svcs_sources.append(Glob(env['svcs_root'] + '/controlpanel/src/CPSControllee/Widgets/*.c'))
#svcs_sources.append(Glob(env['svcs_root'] + '/onboarding/src/*.c'))
#svcs_sources.append(Glob(env['svcs_root'] + '/notification/src/NotificationConsumer/*.c'))

ajs_sources = [
     'ajs.c',
     'ajs_attach.c',
     'ajs_console.c',
     'ajs_cps.c',
     'ajs_ctrlpanel.c',
     'ajs_extstr.c',
     'ajs_io.c',
     'ajs_handlers.c',
     'ajs_heap.c',
     'ajs_marshal.c',
     'ajs_msgloop.c',
     'ajs_notif.c',
     'ajs_propstore.c',
     'ajs_services.c',
     'ajs_sessions.c',
     'ajs_tables.c',
     'ajs_timer.c',
     'ajs_translations.c',
     'ajs_unmarshal.c',
     'ajs_util.c',
     env['os'] + '/ajs_main.c',
     env['os'] + '/ajs_malloc.c',
     env['os'] + '/ajs_obs_stubs.c'
]

if os.environ.get('YUN_BUILD', '0') == '1':
    io_sources = [Glob(env['os'] + '/lininoio/*.c')]
else:
    io_sources = [env['os'] + '/io/io_info.c', env['os'] + '/io/io_simulation.c', env['os'] + '/io/io_stubs.c']


sources = []
sources.append(ajs_sources)
sources.append(io_sources)
sources.append(svcs_sources)
sources.append(duktape_sources)

if env['WS'] != 'off' and not env.GetOption('clean') and not env.GetOption('help'):
    # Set the location of the uncrustify config file
    env['uncrustify_cfg'] = env['ajtcl_root'] + '/ajuncrustify.cfg'

    import sys
    bin_dir = env['ajtcl_root'] + '/tools'
    sys.path.append(bin_dir)
    import whitespace

    def wsbuild(target, source, env):
        print "Evaluating whitespace compliance..."
        return whitespace.main([env['WS'], env['uncrustify_cfg']])

    env.Command('#/ws_ajtcl', Dir('$DISTDIR'), wsbuild)

prog = env.Program('alljoynjs', sources)

Return('prog')
