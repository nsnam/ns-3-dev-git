# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

import os

from waflib import Options

def options(opt):
    opt.add_option('--with-helics',
                   help=('Path to HELICS for federated simulator integration'),
                   default='', dest='with_helics')

def configure(conf):
    if Options.options.with_helics:
        if os.path.isdir(Options.options.with_helics):
            conf.msg("Checking for HELICS location", ("%s (given)" % Options.options.with_helics))
            conf.env['WITH_HELICS'] = os.path.abspath(Options.options.with_helics)
    # else:
        # bake.py uses ../../build, while ns-3-dev uses ../helics
    if not conf.env['WITH_HELICS']:
        conf.msg("Checking for HELICS location", False)
        conf.report_optional_feature("helics", "NS-3 HELICS Integration", False,
                                     "HELICS not enabled (see option --with-helics)")
        # Add this module to the list of modules that won't be built
        # if they are enabled.
        conf.env['MODULES_NOT_BUILT'].append('helics')

        return

    test_code = '''
int main()
{
    return 0;
}

'''

    conf.env.append_value('NS3_MODULE_PATH', os.path.abspath(os.path.join(conf.env['WITH_HELICS'], 'build', 'default')))
    conf.env.append_value('NS3_MODULE_PATH', os.path.abspath(os.path.join(conf.env['WITH_HELICS'], 'lib')))

    conf.env['INCLUDES_HELICS'] = [
            os.path.abspath(os.path.join(conf.env['WITH_HELICS'], 'include'))
        ]
    conf.env['LIBPATH_HELICS'] = [
            os.path.abspath(os.path.join(conf.env['WITH_HELICS'], 'build', 'default')),
            os.path.abspath(os.path.join(conf.env['WITH_HELICS'], 'lib'))
        ]

    conf.env['DEFINES_HELICS'] = ['NS3_HELICS']

    conf.env['HELICS'] = conf.check(fragment=test_code, lib='helics', libpath=conf.env['LIBPATH_HELICS'], use='HELICS')

    conf.env.append_value('LIB_HELICS', 'helics')
    conf.report_optional_feature("helics", "NS-3 HELICS Integration", conf.env['HELICS'], "HELICS library not found")

    if conf.env['HELICS']:
        conf.env['ENABLE_HELICS'] = True
    else:
        # Add this module to the list of modules that won't be built
        # if they are enabled.
        conf.env['MODULES_NOT_BUILT'].append('helics')

def build(bld):
    if 'helics' in bld.env['MODULES_NOT_BUILT']:
        return

    module = bld.create_ns3_module('helics', ['core'])
    module.source = [
        'model/helics.cc',
        'helper/helics-helper.cc',
        ]
    module_test = bld.create_ns3_module_test_library('helics')
    module_test.source = [
        'test/helics-test-suite.cc',
        ]

    if bld.env['HELICS']:
        module.use.extend(['HELICS'])

    headers = bld(features='ns3header')
    headers.module = 'helics'
    headers.source = [
        'model/helics.h',
        'helper/helics-helper.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()

