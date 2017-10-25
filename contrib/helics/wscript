# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

import os

from waflib import Options

def options(opt):
   opt.add_option('--with-zmq',
                   help=('Installation prefix for ZMQ'),
                   dest='with_zmq', default='/usr/local')
   opt.add_option('--with-helics',
                   help=('Path to HELICS for federated simulator integration'),
                   default='', dest='with_helics')

REQUIRED_BOOST_LIBS = ['system', 'filesystem', 'program_options']

def required_boost_libs(conf):
    conf.env['REQUIRED_BOOST_LIBS'] += REQUIRED_BOOST_LIBS


def configure(conf):
    if not conf.env['LIB_BOOST']:
        conf.report_optional_feature("fskit", "fskit integration", False,
                                     "Required boost libraries not found")
        return;

    present_boost_libs = []
    for boost_lib_name in conf.env['LIB_BOOST']:
        if boost_lib_name.startswith("boost_"):
            boost_lib_name = boost_lib_name[6:]
        if boost_lib_name.endswith("-mt"):
            boost_lib_name = boost_lib_name[:-3]
        present_boost_libs.append(boost_lib_name)

    missing_boost_libs = [lib for lib in REQUIRED_BOOST_LIBS if lib not in present_boost_libs]
    if missing_boost_libs != []:
        conf.report_optional_feature("helics", "helics integration", False,
                                     "Required boost libraries not found, missing: %s" % ', '.join(missing_boost_libs))
        # Add this module to the list of modules that won't be built
        # if they are enabled.
        conf.env['MODULES_NOT_BUILT'].append('helics')
        return

    if Options.options.with_zmq:
        if os.path.isdir(Options.options.with_zmq):
            conf.msg("Checking for libzmq.so location", ("%s (given)" % Options.options.with_zmq))
            conf.env['WITH_ZMQ'] = os.path.abspath(Options.options.with_zmq)

    if (not conf.env['WITH_ZMQ']):
        conf.env['MODULES_NOT_BUILT'].append('helics')
        return

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

    zmq_test_code = '''
#include <zmq.h>
int main()
{
    void *context = zmq_ctx_new();
    (void) zmq_term(context);
    return 0;
}
'''

    helics_test_code = '''
int main()
{
    return 0;
}

'''

    conf.env.append_value('NS3_MODULE_PATH', os.path.abspath(os.path.join(conf.env['WITH_ZMQ'], 'build', 'default')))
    conf.env.append_value('NS3_MODULE_PATH', os.path.abspath(os.path.join(conf.env['WITH_ZMQ'], 'lib')))
    conf.env.append_value('NS3_MODULE_PATH', os.path.abspath(os.path.join(conf.env['WITH_ZMQ'], 'lib64')))
    conf.env['INCLUDES_ZMQ'] = [os.path.abspath(os.path.join(conf.env['WITH_ZMQ'], 'include'))]
    conf.env['LIBPATH_ZMQ'] = [
            os.path.abspath(os.path.join(conf.env['WITH_ZMQ'], 'build', 'default')),
            os.path.abspath(os.path.join(conf.env['WITH_ZMQ'], 'lib')),
            os.path.abspath(os.path.join(conf.env['WITH_ZMQ'], 'lib64'))
        ]
    conf.env['DEFINES_ZMQ'] = ['HAVE_ZMQ']
    conf.env['ZMQ'] = conf.check(fragment=zmq_test_code, lib='zmq', libpath=conf.env['LIBPATH_ZMQ'], use='ZMQ')
    conf.env.append_value('LIB_ZMQ', 'zmq')
    conf.report_optional_feature("zmq", "ZMQ Integration", conf.env['ZMQ'], "zmq library not found")


    conf.env.append_value('NS3_MODULE_PATH', os.path.abspath(os.path.join(conf.env['WITH_HELICS'], 'build', 'default')))
    conf.env.append_value('NS3_MODULE_PATH', os.path.abspath(os.path.join(conf.env['WITH_HELICS'], 'lib')))

    conf.env['INCLUDES_HELICS'] = [
            os.path.abspath(os.path.join(conf.env['WITH_HELICS'], 'include')),
            os.path.abspath(os.path.join(conf.env['WITH_HELICS'], 'include/helics'))
        ]
    conf.env['LIBPATH_HELICS'] = [
            os.path.abspath(os.path.join(conf.env['WITH_HELICS'], 'build', 'default')),
            os.path.abspath(os.path.join(conf.env['WITH_HELICS'], 'lib'))
        ]

    conf.env['DEFINES_HELICS'] = ['NS3_HELICS']

    conf.env['HELICS'] = conf.check(fragment=helics_test_code, lib='helics', libpath=conf.env['LIBPATH_HELICS'], use='HELICS')

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
        'model/helics-simulator-impl.cc',
        'helper/helics-helper.cc',
        ]
    module_test = bld.create_ns3_module_test_library('helics')
    module_test.source = [
        'test/helics-test-suite.cc',
        ]

    if bld.env['HELICS']:
        module.use.extend(['HELICS', 'BOOST', 'ZMQ'])

    headers = bld(features='ns3header')
    headers.module = 'helics'
    headers.source = [
        'model/helics.h',
        'model/helics-simulator-impl.h',
        'helper/helics-helper.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()

