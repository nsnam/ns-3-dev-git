#! /usr/bin/env python3
# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
#
# Copyright (c) 2021 Universidade de Brasília
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation;
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>

import itertools
import shutil
from functools import partial
import sys
import unittest
import os
import subprocess
import re
import glob

# Get path containing ns3
ns3_path = os.path.dirname(os.path.abspath(os.sep.join([__file__, "../../"])))
ns3_script = os.sep.join([ns3_path, "ns3"])
ns3rc_script = os.sep.join([ns3_path, ".ns3rc"])
build_status_script = os.sep.join([ns3_path, "build", "build-status.py"])
c4che_script = os.sep.join([ns3_path, "build", "c4che", "_cache.py"])

# Move the current working directory to the ns-3-dev/utils/tests folder
os.chdir(os.path.dirname(os.path.abspath(__file__)))

# Cmake commands
cmake_refresh_cache_command = "cd {ns3_path}/cmake_cache; /usr/bin/cmake ..".format(ns3_path=ns3_path)
cmake_build_project_command = "cd {ns3_path}/cmake_cache; cmake --build . -j".format(ns3_path=ns3_path)
cmake_build_target_command = partial("cd {ns3_path}/cmake_cache; cmake --build . -j {jobs} --target {target}".format,
                                     ns3_path=ns3_path,
                                     jobs=max(1, os.cpu_count() - 1)
                                     )


def run_ns3(args):
    return run_program(ns3_script, args, True)


# Adapted from https://github.com/metabrainz/picard/blob/master/picard/util/__init__.py
def run_program(program, args, python=False):
    if type(args) != str:
        raise Exception("args should be a string")

    # Include python interpreter if running a python script
    if python:
        arguments = [sys.executable, program]
    else:
        arguments = [program]

    if args != "":
        arguments.extend(re.findall("(?:\".*?\"|\S)+", args))

    # Call program with arguments
    ret = subprocess.run(
        arguments,
        stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=None,
        cwd=ns3_path  # run process from the ns-3-dev path
    )

    # Return (error code, stdout and stderr)
    return ret.returncode, ret.stdout.decode(sys.stdout.encoding), ret.stderr.decode(sys.stderr.encoding)


def get_programs_list():
    values = {}
    with open(build_status_script) as f:
        exec(f.read(), globals(), values)
    return values["ns3_runnable_programs"]


def read_c4che_entry(entry):
    values = {}
    with open(c4che_script) as f:
        exec(f.read(), globals(), values)
    return values[entry]


def get_test_enabled():
    return read_c4che_entry("ENABLE_TESTS")


def get_enabled_modules():
    return read_c4che_entry("NS3_ENABLED_MODULES")


class NS3RunWafTargets(unittest.TestCase):

    cleaned_once = False

    def setUp(self):
        if not NS3RunWafTargets.cleaned_once:
            NS3RunWafTargets.cleaned_once = True
            run_ns3("clean")
            return_code, stdout, stderr = run_program("waf", "configure --enable-examples --enable-tests", python=True)
            self.assertEqual(return_code, 0)
            self.assertIn("finished successfully", stdout)

            return_code, stdout, stderr = run_program("waf", "build", python=True)
            self.assertEqual(return_code, 0)
            self.assertIn("finished successfully", stdout)

    def test_01_loadExecutables(self):
        # Check if build-status.py exists, then read to get list of executables
        self.assertTrue(os.path.exists(build_status_script))
        self.ns3_executables = get_programs_list()
        self.assertGreater(len(self.ns3_executables), 0)

    def test_02_loadModules(self):
        # Check if c4che.py exists than read to get the list of enabled modules
        self.assertTrue(os.path.exists(c4che_script))
        self.ns3_modules = get_enabled_modules()
        self.assertGreater(len(self.ns3_modules), 0)

    def test_03_runNobuildScratchSim(self):
        return_code, stdout, stderr = run_ns3("--run-no-build scratch-simulator")
        self.assertEqual(return_code, 0)
        self.assertIn("Scratch Simulator", stderr)

    def test_04_runNobuildExample(self):
        return_code, stdout, stderr = run_ns3("--run-no-build command-line-example")
        self.assertEqual(return_code, 0)
        self.assertIn("command-line-example", stdout)

    def test_05_runTestCaseCoreExampleSimulator(self):
        return_code, stdout, stderr = run_program("test.py", "--nowaf -s core-example-simulator", True)
        self.assertEqual(return_code, 0)
        self.assertIn("PASS", stdout)

    def test_06_runTestCaseExamplesAsTestsTestSuite(self):
        return_code, stdout, stderr = run_program("test.py", "--nowaf -s examples-as-tests-test-suite", True)
        self.assertEqual(return_code, 0)
        self.assertIn("PASS", stdout)


class NS3CommonSettingsTestCase(unittest.TestCase):
    def setUp(self):
        super().setUp()
        # No special setup for common test cases other than making sure we are working on a clean directory
        run_ns3("clean")

    def test_01_NoOption(self):
        return_code, stdout, stderr = run_ns3("")
        self.assertEqual(return_code, 0)
        self.assertIn("You need to configure ns-3 first: try ./ns3 configure", stdout)

    def test_02_Verbose(self):
        return_code, stdout, stderr = run_ns3("--verbose")
        self.assertEqual(return_code, 0)
        self.assertIn("You need to configure ns-3 first: try ./ns3 configure", stdout)

    def test_03_NoTaskLines(self):
        return_code, stdout, stderr = run_ns3("--no-task-lines")
        self.assertEqual(return_code, 0)
        self.assertIn("You need to configure ns-3 first: try ./ns3 configure", stdout)

    def test_04_CheckConfig(self):
        return_code, stdout, stderr = run_ns3("--check-config")
        self.assertEqual(return_code, 1)
        self.assertIn("Project was not configured", stderr)


class NS3ConfigureBuildProfileTestCase(unittest.TestCase):
    def setUp(self):
        super().setUp()
        # No special setup for common test cases other than making sure we are working on a clean directory
        run_ns3("clean")

    def test_01_Debug(self):
        return_code, stdout, stderr = run_ns3("configure -d debug")
        self.assertEqual(return_code, 0)
        self.assertIn("Build profile                 : debug", stdout)
        self.assertIn("Build files have been written to", stdout)

        # Build core to check if profile suffixes match the expected
        return_code, stdout, stderr = run_ns3("build core")
        self.assertEqual(return_code, 0)
        self.assertIn("Built target libcore", stdout)

        libcore = glob.glob(os.sep.join([ns3_path, "build", "lib"]) + '/*', recursive=True)
        self.assertGreater(len(libcore), 0)
        self.assertIn("core-debug", libcore[0])

    def test_02_Release(self):
        return_code, stdout, stderr = run_ns3("configure -d release")
        self.assertEqual(return_code, 0)
        self.assertIn("Build profile                 : release", stdout)
        self.assertIn("Build files have been written to", stdout)

    def test_03_Optimized(self):
        return_code, stdout, stderr = run_ns3("configure -d optimized")
        self.assertEqual(return_code, 0)
        self.assertIn("Build profile                 : optimized", stdout)
        self.assertIn("Build files have been written to", stdout)

        # Build core to check if profile suffixes match the expected
        return_code, stdout, stderr = run_ns3("build core")
        self.assertEqual(return_code, 0)
        self.assertIn("Built target libcore", stdout)

        libcore = glob.glob(os.sep.join([ns3_path, "build", "lib"]) + '/*', recursive=True)
        self.assertGreater(len(libcore), 0)
        self.assertIn("core-optimized", libcore[0])

    def test_04_Typo(self):
        return_code, stdout, stderr = run_ns3("configure -d Optimized")
        self.assertEqual(return_code, 2)
        self.assertIn("invalid choice: 'Optimized'", stderr)

    def test_05_TYPO(self):
        return_code, stdout, stderr = run_ns3("configure -d OPTIMIZED")
        self.assertEqual(return_code, 2)
        self.assertIn("invalid choice: 'OPTIMIZED'", stderr)


class NS3BaseTestCase(unittest.TestCase):

    cleaned_once = False

    def config_ok(self, return_code, stdout):
        self.assertEqual(return_code, 0)
        self.assertIn("Build profile                 : release", stdout)
        self.assertIn("Build files have been written to", stdout)

    def setUp(self):
        super().setUp()

        if os.path.exists(ns3rc_script):
            os.remove(ns3rc_script)

        # We only clear it once and then update the settings by changing flags or consuming ns3rc
        if not NS3BaseTestCase.cleaned_once:
            NS3BaseTestCase.cleaned_once = True
            run_ns3("clean")
            return_code, stdout, stderr = run_ns3("configure -d release")
            self.config_ok(return_code, stdout)

        # Check if build-status.py exists, then read to get list of executables
        self.assertTrue(os.path.exists(build_status_script))
        self.ns3_executables = get_programs_list()

        # Check if c4che.py exists than read to get the list of enabled modules
        self.assertTrue(os.path.exists(c4che_script))
        self.ns3_modules = get_enabled_modules()


class NS3ConfigureTestCase(NS3BaseTestCase):

    cleaned_once = False

    def setUp(self):
        if not NS3ConfigureTestCase.cleaned_once:
            NS3ConfigureTestCase.cleaned_once = True
            NS3BaseTestCase.cleaned_once = False
        super().setUp()

    def test_01_Examples(self):
        return_code, stdout, stderr = run_ns3("configure --enable-examples")

        # This just tests if we didn't break anything, not that we actually have enabled anything
        self.config_ok(return_code, stdout)

        # If nothing went wrong, we should have more executables in the list after enabling the examples
        self.assertGreater(len(get_programs_list()), len(self.ns3_executables))

        # Now we disabled them back
        return_code, stdout, stderr = run_ns3("configure --disable-examples")

        # This just tests if we didn't break anything, not that we actually have enabled anything
        self.config_ok(return_code, stdout)

        # And check if they went back to the original list
        self.assertEqual(len(get_programs_list()), len(self.ns3_executables))

    def test_02_Tests(self):
        # Try enabling tests
        return_code, stdout, stderr = run_ns3("configure --enable-tests")
        self.config_ok(return_code, stdout)

        # Then try building the libcore test
        return_code, stdout, stderr = run_ns3("build core-test")

        # If nothing went wrong, this should have worked
        self.assertEqual(return_code, 0)
        self.assertIn("Built target libcore-test", stdout)

        # Now we disabled the tests
        return_code, stdout, stderr = run_ns3("configure --disable-tests")
        self.config_ok(return_code, stdout)

        # Now building the library test should fail
        return_code, stdout, stderr = run_ns3("build core-test")

        # And check if they went back to the original list
        self.assertGreater(len(stderr), 0)

    def test_03_EnableModules(self):
        # Try filtering enabled modules to network+wifi and their dependencies
        return_code, stdout, stderr = run_ns3("configure --enable-modules='network;wifi'")
        self.config_ok(return_code, stdout)

        # At this point we should have fewer modules
        enabled_modules = get_enabled_modules()
        self.assertLess(len(get_enabled_modules()), len(self.ns3_modules))
        self.assertIn("ns3-network", enabled_modules)
        self.assertIn("ns3-wifi", enabled_modules)

        # Try cleaning the list of enabled modules to reset to the normal configuration
        return_code, stdout, stderr = run_ns3("configure --enable-modules=''")
        self.config_ok(return_code, stdout)

        # At this point we should have the same amount of modules that we had when we started
        self.assertEqual(len(get_enabled_modules()), len(self.ns3_modules))

    def test_04_DisableModules(self):
        # Try filtering disabled modules to disable lte and modules that depend on it
        return_code, stdout, stderr = run_ns3("configure --disable-modules='lte;mpi'")
        self.config_ok(return_code, stdout)

        # At this point we should have fewer modules
        enabled_modules = get_enabled_modules()
        self.assertLess(len(enabled_modules), len(self.ns3_modules))
        self.assertNotIn("ns3-lte", enabled_modules)
        self.assertNotIn("ns3-mpi", enabled_modules)

        # Try cleaning the list of enabled modules to reset to the normal configuration
        return_code, stdout, stderr = run_ns3("configure --disable-modules=''")
        self.config_ok(return_code, stdout)

        # At this point we should have the same amount of modules that we had when we started
        self.assertEqual(len(get_enabled_modules()), len(self.ns3_modules))

    def test_05_Ns3rc(self):
        ns3rc_template = "# ! /usr/bin/env python\
                          \
                          # A list of the modules that will be enabled when ns-3 is run.\
                          # Modules that depend on the listed modules will be enabled also.\
                          #\
                          # All modules can be enabled by choosing 'all_modules'.\
                          modules_enabled = [{modules}]\
                          \
                          # Set this equal to true if you want examples to be run.\
                          examples_enabled = {examples}\
                          \
                          # Set this equal to true if you want tests to be run.\
                          tests_enabled = {tests}\
                          "

        # Now we repeat the command line tests but with the ns3rc file
        with open(ns3rc_script, "w") as f:
            f.write(ns3rc_template.format(modules="'lte'", examples="False", tests="True"))

        # Reconfigure
        return_code, stdout, stderr = run_ns3("configure")
        self.config_ok(return_code, stdout)

        # Check
        enabled_modules = get_enabled_modules()
        self.assertLess(len(get_enabled_modules()), len(self.ns3_modules))
        self.assertIn("ns3-lte", enabled_modules)
        self.assertTrue(get_test_enabled())
        self.assertEqual(len(get_programs_list()) + 1, len(self.ns3_executables))  # the +1 is due to test-runner

        # Replace the ns3rc file
        with open(ns3rc_script, "w") as f:
            f.write(ns3rc_template.format(modules="'wifi'", examples="True", tests="False"))

        # Reconfigure
        return_code, stdout, stderr = run_ns3("configure")
        self.config_ok(return_code, stdout)

        # Check
        enabled_modules = get_enabled_modules()
        self.assertLess(len(get_enabled_modules()), len(self.ns3_modules))
        self.assertIn("ns3-wifi", enabled_modules)
        self.assertFalse(get_test_enabled())
        self.assertGreater(len(get_programs_list()), len(self.ns3_executables))

        # Then we roll back by removing the ns3rc config file
        os.remove(ns3rc_script)

        # Reconfigure
        return_code, stdout, stderr = run_ns3("configure")
        self.config_ok(return_code, stdout)

        # Check
        self.assertEqual(len(get_enabled_modules()), len(self.ns3_modules))
        self.assertFalse(get_test_enabled())
        self.assertEqual(len(get_programs_list()), len(self.ns3_executables))

    def test_06_DryRun(self):
        # todo: collect commands from CMake and test them
        pass


class NS3BuildBaseTestCase(NS3BaseTestCase):
    cleaned_once = False

    def setUp(self):
        if not NS3BuildBaseTestCase.cleaned_once:
            NS3BuildBaseTestCase.cleaned_once = True
            NS3BaseTestCase.cleaned_once = False
        super().setUp()

    def test_01_BuildExistingTargets(self):
        return_code, stdout, stderr = run_ns3("build core")
        self.assertEqual(return_code, 0)
        self.assertIn("Built target libcore", stdout)

    def test_02_BuildNonExistingTargets(self):
        # tests are not enable, so the target isn't available
        return_code, stdout, stderr = run_ns3("build core-test")
        self.assertGreater(len(stderr), 0)

    def test_03_BuildProject(self):
        return_code, stdout, stderr = run_ns3("build")
        self.assertEqual(return_code, 0)
        self.assertIn("Built target", stdout)
        for program in get_programs_list():
            self.assertTrue(os.path.exists(program))

    def test_04_BuildProjectVerbose(self):
        return_code, stdout, stderr = run_ns3("--verbose build")
        self.assertEqual(return_code, 0)
        self.assertIn("Built target", stdout)
        for program in get_programs_list():
            self.assertTrue(os.path.exists(program))
        self.assertIn(cmake_refresh_cache_command, stdout)
        self.assertIn(cmake_build_project_command, stdout)

    def test_05_BuildProjectNoTaskLines(self):
        return_code, stdout, stderr = run_ns3("--no-task-lines build")
        self.assertEqual(return_code, 0)
        self.assertEqual(len(stderr), 0)
        self.assertEqual(len(stdout), 0)

    def test_06_BuildProjectVerboseAndNoTaskLines(self):
        return_code, stdout, stderr = run_ns3("--no-task-lines --verbose build")
        self.assertEqual(return_code, 0)
        self.assertIn(cmake_refresh_cache_command, stdout)
        self.assertIn(cmake_build_project_command, stdout)


class NS3ExpectedUseTestCase(NS3BaseTestCase):
    cleaned_once = False

    def setUp(self):
        if not NS3ExpectedUseTestCase.cleaned_once:
            NS3ExpectedUseTestCase.cleaned_once = True
            NS3BaseTestCase.cleaned_once = False
            super().setUp()

            # On top of the release build configured by NS3ConfigureTestCase, also enable examples, tests and docs
            return_code, stdout, stderr = run_ns3("configure --enable-examples --enable-tests --enable-documentation")
            self.config_ok(return_code, stdout)

        # Check if build-status.py exists, then read to get list of executables
        self.assertTrue(os.path.exists(build_status_script))
        self.ns3_executables = get_programs_list()

        # Check if c4che.py exists than read to get the list of enabled modules
        self.assertTrue(os.path.exists(c4che_script))
        self.ns3_modules = get_enabled_modules()

    def test_01_BuildProjectVerbose(self):
        return_code, stdout, stderr = run_ns3("--verbose build")
        self.assertEqual(return_code, 0)
        self.assertIn("Built target", stdout)
        for program in get_programs_list():
            self.assertTrue(os.path.exists(program))
        libraries = ";".join(glob.glob(os.sep.join([ns3_path, "build", "lib"]) + '/*', recursive=True))
        for module in get_enabled_modules():
            self.assertIn(module.replace("ns3-", ""), libraries)
        self.assertIn(cmake_refresh_cache_command, stdout)
        self.assertIn(cmake_build_project_command, stdout)

    def test_02_BuildAndRunExistingExecutableTarget(self):
        return_code, stdout, stderr = run_ns3('--verbose --run "test-runner --list"')
        self.assertEqual(return_code, 0)
        self.assertIn("Built target test-runner", stdout)
        self.assertIn(cmake_refresh_cache_command, stdout)
        self.assertIn(cmake_build_target_command(target="test-runner"), stdout)

    def test_03_BuildAndRunExistingLibraryTarget(self):
        return_code, stdout, stderr = run_ns3("--run core")  # this should not work
        self.assertEqual(return_code, 1)
        self.assertIn("Couldn't find the specified program: core", stderr)

    def test_04_BuildAndRunNonExistingTarget(self):
        return_code, stdout, stderr = run_ns3("--run nonsense")  # this should not work
        self.assertEqual(return_code, 1)
        self.assertIn("Couldn't find the specified program: nonsense", stderr)

    def test_05_RunNoBuildExistingExecutableTarget(self):
        return_code, stdout, stderr = run_ns3('--verbose --run-no-build "test-runner --list"')
        self.assertEqual(return_code, 0)
        self.assertNotIn("Built target test-runner", stdout)
        self.assertNotIn(cmake_refresh_cache_command, stdout)
        self.assertNotIn(cmake_build_target_command(target="test-runner"), stdout)
        self.assertIn("test-runner", stdout)
        self.assertIn("--list", stdout)

    def test_06_RunNoBuildExistingLibraryTarget(self):
        return_code, stdout, stderr = run_ns3("--verbose --run-no-build core")  # this should not work
        self.assertEqual(return_code, 1)
        self.assertIn("Couldn't find the specified program: core", stderr)

    def test_07_RunNoBuildNonExistingExecutableTarget(self):
        return_code, stdout, stderr = run_ns3("--verbose --run-no-build nonsense")  # this should not work
        self.assertEqual(return_code, 1)
        self.assertIn("Couldn't find the specified program: nonsense", stderr)

    def test_08_RunNoBuildGdb(self):
        return_code, stdout, stderr = run_ns3("--run-no-build scratch-simulator --gdb")
        self.assertEqual(return_code, 0)
        self.assertIn("scratch-simulator", stdout)
        self.assertIn("No debugging symbols found", stdout)

    def test_09_RunNoBuildValgrind(self):
        return_code, stdout, stderr = run_ns3("--run-no-build scratch-simulator --valgrind")
        self.assertEqual(return_code, 0)
        self.assertIn("scratch-simulator", stderr)
        self.assertIn("Memcheck", stderr)

    def test_10_DoxygenWithBuild(self):
        return_code, stdout, stderr = run_ns3("--verbose --doxygen")
        self.assertEqual(return_code, 0)
        self.assertIn(cmake_refresh_cache_command, stdout)
        self.assertIn(cmake_build_target_command(target="doxygen"), stdout)
        self.assertIn("Built target doxygen", stdout)

    def test_11_DoxygenWithoutBuild(self):
        return_code, stdout, stderr = run_ns3("--verbose --doxygen-no-build")
        self.assertEqual(return_code, 0)
        self.assertIn(cmake_refresh_cache_command, stdout)
        self.assertIn(cmake_build_target_command(target="doxygen-no-build"), stdout)
        self.assertIn("Built target doxygen-no-build", stdout)


if __name__ == '__main__':
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()

    # Put tests cases in order
    suite.addTests(loader.loadTestsFromTestCase(NS3RunWafTargets))
    suite.addTests(loader.loadTestsFromTestCase(NS3CommonSettingsTestCase))
    suite.addTests(loader.loadTestsFromTestCase(NS3ConfigureBuildProfileTestCase))
    suite.addTests(loader.loadTestsFromTestCase(NS3ConfigureTestCase))
    suite.addTests(loader.loadTestsFromTestCase(NS3BuildBaseTestCase))
    suite.addTests(loader.loadTestsFromTestCase(NS3ExpectedUseTestCase))

    # Before running, check if ns3rc exists and save it
    ns3rc_script_bak = ns3rc_script + ".bak"
    if os.path.exists(ns3rc_script) and not os.path.exists(ns3rc_script_bak):
        shutil.move(ns3rc_script, ns3rc_script_bak)

    # Run tests and fail as fast as possible
    runner = unittest.TextTestRunner(failfast=True)
    result = runner.run(suite)

    # After completing the tests successfully, restore the ns3rc file
    if os.path.exists(ns3rc_script_bak):
        shutil.move(ns3rc_script_bak, ns3rc_script)
