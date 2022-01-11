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

"""!
Test suite for the ns3 wrapper script
"""

import glob
import os
import re
import shutil
import subprocess
import sys
import unittest
from functools import partial

# Get path containing ns3
ns3_path = os.path.dirname(os.path.abspath(os.sep.join([__file__, "../../"])))
ns3_script = os.sep.join([ns3_path, "ns3"])
ns3rc_script = os.sep.join([ns3_path, ".ns3rc"])
usual_outdir = os.sep.join([ns3_path, "build"])
usual_build_status_script = os.sep.join([usual_outdir, "build-status.py"])
usual_c4che_script = os.sep.join([usual_outdir, "c4che", "_cache.py"])
usual_lib_outdir = os.sep.join([usual_outdir, "lib"])

# Move the current working directory to the ns-3-dev folder
os.chdir(ns3_path)

# Cmake commands
cmake_build_project_command = "cmake --build . -j".format(ns3_path=ns3_path)
cmake_build_target_command = partial("cmake --build . -j {jobs} --target {target}".format,
                                     jobs=max(1, os.cpu_count() - 1)
                                     )


def run_ns3(args, env=None):
    """!
    Runs the ns3 wrapper script with arguments
    @param args: string containing arguments that will get split before calling ns3
    @param env: environment variables dictionary
    @return tuple containing (error code, stdout and stderr)
    """
    return run_program(ns3_script, args, python=True, env=env)


# Adapted from https://github.com/metabrainz/picard/blob/master/picard/util/__init__.py
def run_program(program, args, python=False, cwd=ns3_path, env=None):
    """!
    Runs a program with the given arguments and returns a tuple containing (error code, stdout and stderr)
    @param program: program to execute (or python script)
    @param args: string containing arguments that will get split before calling the program
    @param python: flag indicating whether the program is a python script
    @param cwd: the working directory used that will be the root folder for the execution
    @param env: environment variables dictionary
    @return tuple containing (error code, stdout and stderr)
    """
    if type(args) != str:
        raise Exception("args should be a string")

    # Include python interpreter if running a python script
    if python:
        arguments = [sys.executable, program]
    else:
        arguments = [program]

    if args != "":
        arguments.extend(re.findall("(?:\".*?\"|\S)+", args))

    for i in range(len(arguments)):
        arguments[i] = arguments[i].replace("\"", "")

    # Forward environment variables used by the ns3 script
    current_env = os.environ.copy()

    # Add different environment variables
    if env:
        current_env.update(env)

    # Call program with arguments
    ret = subprocess.run(
        arguments,
        stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=cwd,  # run process from the ns-3-dev path
        env=current_env
    )
    # Return (error code, stdout and stderr)
    return ret.returncode, ret.stdout.decode(sys.stdout.encoding), ret.stderr.decode(sys.stderr.encoding)


def get_programs_list(build_status_script_path=usual_build_status_script):
    """!
    Extracts the programs list from build-status.py
    @param build_status_script_path: path containing build-status.py
    @return list of programs.
    """
    values = {}
    with open(build_status_script_path) as f:
        exec(f.read(), globals(), values)
    return values["ns3_runnable_programs"]


def get_libraries_list(lib_outdir=usual_lib_outdir):
    """!
    Gets a list of built libraries
    @param lib_outdir: path containing libraries
    @return list of built libraries.
    """
    return glob.glob(lib_outdir + '/*', recursive=True)


def get_headers_list(outdir=usual_outdir):
    """!
    Gets a list of header files
    @param outdir: path containing headers
    @return list of headers.
    """
    return glob.glob(outdir + '/**/*.h', recursive=True)


def read_c4che_entry(entry, c4che_script_path=usual_c4che_script):
    """!
    Read interesting entries from the c4che/_cache.py file
    @param entry: entry to read from c4che/_cache.py
    @param c4che_script_path: path containing _cache.py
    @return value of the requested entry.
    """
    values = {}
    with open(c4che_script_path) as f:
        exec(f.read(), globals(), values)
    return values.get(entry, None)


def get_test_enabled():
    """!
    Check if tests are enabled in the c4che/_cache.py
    @return bool.
    """
    return read_c4che_entry("ENABLE_TESTS")


def get_enabled_modules():
    """
    Check if tests are enabled in the c4che/_cache.py
    @return list of enabled modules (prefixed with 'ns3-').
    """
    return read_c4che_entry("NS3_ENABLED_MODULES")


class NS3RunWafTargets(unittest.TestCase):
    """!
    ns3 tests related to compatibility with Waf-produced binaries
    """

    ## when cleaned_once is False, clean up build artifacts and reconfigure
    cleaned_once = False

    def setUp(self):
        """!
        Clean the default build directory, then configure and build ns-3 with waf
        @return None
        """
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
        """!
        Try to load executables built by waf
        @return None
        """
        # Check if build-status.py exists, then read to get list of executables.
        self.assertTrue(os.path.exists(usual_build_status_script))
        ## ns3_executables holds a list of executables in build-status.py
        self.ns3_executables = get_programs_list()
        self.assertGreater(len(self.ns3_executables), 0)

    def test_02_loadModules(self):
        """!
        Try to load modules built by waf
        @return None
        """
        # Check if c4che.py exists than read to get the list of enabled modules.
        self.assertTrue(os.path.exists(usual_c4che_script))
        ## ns3_modules holds a list to the modules enabled stored in c4che.py
        self.ns3_modules = get_enabled_modules()
        self.assertGreater(len(self.ns3_modules), 0)

    def test_03_runNobuildScratchSim(self):
        """!
        Try to run an executable built by waf
        @return None
        """
        return_code, stdout, stderr = run_ns3("run scratch-simulator --no-build")
        self.assertEqual(return_code, 0)
        self.assertIn("Scratch Simulator", stderr)

    def test_04_runNobuildExample(self):
        """!
        Try to run a different executable built by waf
        @return None
        """
        return_code, stdout, stderr = run_ns3("run command-line-example --no-build")
        self.assertEqual(return_code, 0)
        self.assertIn("command-line-example", stdout)

    def test_05_runTestCaseCoreExampleSimulator(self):
        """!
        Try to run a test case built by waf that calls the ns3 wrapper script
        @return None
        """
        return_code, stdout, stderr = run_program("test.py", "--no-build -s core-example-simulator", True)
        self.assertEqual(return_code, 0)
        self.assertIn("PASS", stdout)

    def test_06_runTestCaseExamplesAsTestsTestSuite(self):
        """!
        Try to run a different test case built by waf that calls the ns3 wrapper script
        @return None
        """
        return_code, stdout, stderr = run_program("test.py", "--no-build -s examples-as-tests-test-suite", True)
        self.assertEqual(return_code, 0)
        self.assertIn("PASS", stdout)

    def test_07_runCoreExampleSimulator(self):
        """!
        Try to run test cases built by waf that calls the ns3 wrapper script
        when the output directory is set to a different path
        @return None
        """
        run_ns3("clean")

        return_code, stdout, stderr = run_program("waf",
                                                  "configure --enable-examples --enable-tests --out build/debug",
                                                  python=True)
        self.assertEqual(return_code, 0)
        self.assertIn("finished successfully", stdout)

        return_code, stdout, stderr = run_program("waf",
                                                  '--run "test-runner --suite=core-example-simulator --verbose"',
                                                  True)
        self.assertEqual(return_code, 0)
        self.assertIn("PASS", stdout)


class NS3CommonSettingsTestCase(unittest.TestCase):
    """!
    ns3 tests related to generic options
    """

    def setUp(self):
        """!
        Clean configuration/build artifacts before common commands
        @return None
        """
        super().setUp()
        # No special setup for common test cases other than making sure we are working on a clean directory.
        run_ns3("clean")

    def test_01_NoOption(self):
        """!
        Test not passing any arguments to
        @return None
        """
        return_code, stdout, stderr = run_ns3("")
        self.assertEqual(return_code, 1)
        self.assertIn("You need to configure ns-3 first: try ./ns3 configure", stdout)

    def test_02_NoTaskLines(self):
        """!
        Test only passing --quiet argument to ns3
        @return None
        """
        return_code, stdout, stderr = run_ns3("--quiet")
        self.assertEqual(return_code, 1)
        self.assertIn("You need to configure ns-3 first: try ./ns3 configure", stdout)

    def test_03_CheckConfig(self):
        """!
        Test only passing --check-config argument to ns3
        @return None
        """
        return_code, stdout, stderr = run_ns3("--check-config")
        self.assertEqual(return_code, 1)
        self.assertIn("You need to configure ns-3 first: try ./ns3 configure", stdout)

    def test_04_CheckProfile(self):
        """!
        Test only passing --check-profile argument to ns3
        @return None
        """
        return_code, stdout, stderr = run_ns3("--check-profile")
        self.assertEqual(return_code, 1)
        self.assertIn("You need to configure ns-3 first: try ./ns3 configure", stdout)

    def test_05_CheckVersion(self):
        """!
        Test only passing --check-version argument to ns3
        @return None
        """
        return_code, stdout, stderr = run_ns3("--check-version")
        self.assertEqual(return_code, 1)
        self.assertIn("You need to configure ns-3 first: try ./ns3 configure", stdout)


class NS3ConfigureBuildProfileTestCase(unittest.TestCase):
    """!
    ns3 tests related to build profiles
    """

    def setUp(self):
        """!
        Clean configuration/build artifacts before testing configuration settings
        @return None
        """
        super().setUp()
        # No special setup for common test cases other than making sure we are working on a clean directory.
        run_ns3("clean")

    def test_01_Debug(self):
        """!
        Test the debug build
        @return None
        """
        return_code, stdout, stderr = run_ns3("configure -d debug --enable-verbose")
        self.assertEqual(return_code, 0)
        self.assertIn("Build profile                 : debug", stdout)
        self.assertIn("Build files have been written to", stdout)

        # Build core to check if profile suffixes match the expected.
        return_code, stdout, stderr = run_ns3("build core")
        self.assertEqual(return_code, 0)
        self.assertIn("Built target libcore", stdout)

        libraries = get_libraries_list()
        self.assertGreater(len(libraries), 0)
        self.assertIn("core-debug", libraries[0])

    def test_02_Release(self):
        """!
        Test the release build
        @return None
        """
        return_code, stdout, stderr = run_ns3("configure -d release")
        self.assertEqual(return_code, 0)
        self.assertIn("Build profile                 : release", stdout)
        self.assertIn("Build files have been written to", stdout)

    def test_03_Optimized(self):
        """!
        Test the optimized build
        @return None
        """
        return_code, stdout, stderr = run_ns3("configure -d optimized --enable-verbose")
        self.assertEqual(return_code, 0)
        self.assertIn("Build profile                 : optimized", stdout)
        self.assertIn("Build files have been written to", stdout)

        # Build core to check if profile suffixes match the expected
        return_code, stdout, stderr = run_ns3("build core")
        self.assertEqual(return_code, 0)
        self.assertIn("Built target libcore", stdout)

        libraries = get_libraries_list()
        self.assertGreater(len(libraries), 0)
        self.assertIn("core-optimized", libraries[0])

    def test_04_Typo(self):
        """!
        Test a build type with a typo
        @return None
        """
        return_code, stdout, stderr = run_ns3("configure -d Optimized")
        self.assertEqual(return_code, 2)
        self.assertIn("invalid choice: 'Optimized'", stderr)

    def test_05_TYPO(self):
        """!
        Test a build type with another typo
        @return None
        """
        return_code, stdout, stderr = run_ns3("configure -d OPTIMIZED")
        self.assertEqual(return_code, 2)
        self.assertIn("invalid choice: 'OPTIMIZED'", stderr)


class NS3BaseTestCase(unittest.TestCase):
    """!
    Generic test case with basic function inherited by more complex tests.
    """

    ## when cleaned_once is False, clean up build artifacts and reconfigure
    cleaned_once = False

    def config_ok(self, return_code, stdout):
        """!
        Check if configuration for release mode worked normally
        @param return_code: return code from CMake
        @param stdout: output from CMake.
        @return None
        """
        self.assertEqual(return_code, 0)
        self.assertIn("Build profile                 : release", stdout)
        self.assertIn("Build files have been written to", stdout)

    def setUp(self):
        """!
        Clean configuration/build artifacts before testing configuration and build settings
        After configuring the build as release,
        check if configuration worked and check expected output files.
        @return None
        """
        super().setUp()

        if os.path.exists(ns3rc_script):
            os.remove(ns3rc_script)

        # We only clear it once and then update the settings by changing flags or consuming ns3rc.
        if not NS3BaseTestCase.cleaned_once:
            NS3BaseTestCase.cleaned_once = True
            run_ns3("clean")
            return_code, stdout, stderr = run_ns3("configure -d release --enable-verbose")
            self.config_ok(return_code, stdout)

        # Check if build-status.py exists, then read to get list of executables.
        self.assertTrue(os.path.exists(usual_build_status_script))
        ## ns3_executables holds a list of executables in build-status.py
        self.ns3_executables = get_programs_list()

        # Check if c4che.py exists than read to get the list of enabled modules.
        self.assertTrue(os.path.exists(usual_c4che_script))
        ## ns3_modules holds a list to the modules enabled stored in c4che.py
        self.ns3_modules = get_enabled_modules()


class NS3ConfigureTestCase(NS3BaseTestCase):
    """!
    Test ns3 configuration options
    """

    ## when cleaned_once is False, clean up build artifacts and reconfigure
    cleaned_once = False

    def setUp(self):
        """!
        Reuse cleaning/release configuration from NS3BaseTestCase if flag is cleaned
        @return None
        """
        if not NS3ConfigureTestCase.cleaned_once:
            NS3ConfigureTestCase.cleaned_once = True
            NS3BaseTestCase.cleaned_once = False
        super().setUp()

    def test_01_Examples(self):
        """!
        Test enabling and disabling examples
        @return None
        """
        return_code, stdout, stderr = run_ns3("configure --enable-examples")

        # This just tests if we didn't break anything, not that we actually have enabled anything.
        self.config_ok(return_code, stdout)

        # If nothing went wrong, we should have more executables in the list after enabling the examples.
        self.assertGreater(len(get_programs_list()), len(self.ns3_executables))

        # Now we disabled them back.
        return_code, stdout, stderr = run_ns3("configure --disable-examples")

        # This just tests if we didn't break anything, not that we actually have enabled anything.
        self.config_ok(return_code, stdout)

        # Then check if they went back to the original list.
        self.assertEqual(len(get_programs_list()), len(self.ns3_executables))

    def test_02_Tests(self):
        """!
        Test enabling and disabling tests
        @return None
        """
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

        # Then check if they went back to the original list
        self.assertGreater(len(stderr), 0)

    def test_03_EnableModules(self):
        """!
        Test enabling specific modules
        @return None
        """
        # Try filtering enabled modules to network+Wi-Fi and their dependencies
        return_code, stdout, stderr = run_ns3("configure --enable-modules='network;wifi'")
        self.config_ok(return_code, stdout)

        # At this point we should have fewer modules
        enabled_modules = get_enabled_modules()
        self.assertLess(len(get_enabled_modules()), len(self.ns3_modules))
        self.assertIn("ns3-network", enabled_modules)
        self.assertIn("ns3-wifi", enabled_modules)

        # Try cleaning the list of enabled modules to reset to the normal configuration.
        return_code, stdout, stderr = run_ns3("configure --enable-modules=''")
        self.config_ok(return_code, stdout)

        # At this point we should have the same amount of modules that we had when we started.
        self.assertEqual(len(get_enabled_modules()), len(self.ns3_modules))

    def test_04_DisableModules(self):
        """!
        Test disabling specific modules
        @return None
        """
        # Try filtering disabled modules to disable lte and modules that depend on it.
        return_code, stdout, stderr = run_ns3("configure --disable-modules='lte;mpi'")
        self.config_ok(return_code, stdout)

        # At this point we should have fewer modules.
        enabled_modules = get_enabled_modules()
        self.assertLess(len(enabled_modules), len(self.ns3_modules))
        self.assertNotIn("ns3-lte", enabled_modules)
        self.assertNotIn("ns3-mpi", enabled_modules)

        # Try cleaning the list of enabled modules to reset to the normal configuration.
        return_code, stdout, stderr = run_ns3("configure --disable-modules=''")
        self.config_ok(return_code, stdout)

        # At this point we should have the same amount of modules that we had when we started.
        self.assertEqual(len(get_enabled_modules()), len(self.ns3_modules))

    def test_05_EnableModulesComma(self):
        """!
        Test enabling comma-separated (waf-style) examples
        @return None
        """
        # Try filtering enabled modules to network+Wi-Fi and their dependencies.
        return_code, stdout, stderr = run_ns3("configure --enable-modules='network,wifi'")
        self.config_ok(return_code, stdout)

        # At this point we should have fewer modules.
        enabled_modules = get_enabled_modules()
        self.assertLess(len(get_enabled_modules()), len(self.ns3_modules))
        self.assertIn("ns3-network", enabled_modules)
        self.assertIn("ns3-wifi", enabled_modules)

        # Try cleaning the list of enabled modules to reset to the normal configuration.
        return_code, stdout, stderr = run_ns3("configure --enable-modules=''")
        self.config_ok(return_code, stdout)

        # At this point we should have the same amount of modules that we had when we started.
        self.assertEqual(len(get_enabled_modules()), len(self.ns3_modules))

    def test_06_DisableModulesComma(self):
        """!
        Test disabling comma-separated (waf-style) examples
        @return None
        """
        # Try filtering disabled modules to disable lte and modules that depend on it.
        return_code, stdout, stderr = run_ns3("configure --disable-modules='lte,mpi'")
        self.config_ok(return_code, stdout)

        # At this point we should have fewer modules.
        enabled_modules = get_enabled_modules()
        self.assertLess(len(enabled_modules), len(self.ns3_modules))
        self.assertNotIn("ns3-lte", enabled_modules)
        self.assertNotIn("ns3-mpi", enabled_modules)

        # Try cleaning the list of enabled modules to reset to the normal configuration.
        return_code, stdout, stderr = run_ns3("configure --disable-modules=''")
        self.config_ok(return_code, stdout)

        # At this point we should have the same amount of modules that we had when we started.
        self.assertEqual(len(get_enabled_modules()), len(self.ns3_modules))

    def test_07_Ns3rc(self):
        """!
        Test loading settings from the ns3rc config file
        @return None
        """
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

        # Now we repeat the command line tests but with the ns3rc file.
        with open(ns3rc_script, "w") as f:
            f.write(ns3rc_template.format(modules="'lte'", examples="False", tests="True"))

        # Reconfigure.
        return_code, stdout, stderr = run_ns3("configure")
        self.config_ok(return_code, stdout)

        # Check.
        enabled_modules = get_enabled_modules()
        self.assertLess(len(get_enabled_modules()), len(self.ns3_modules))
        self.assertIn("ns3-lte", enabled_modules)
        self.assertTrue(get_test_enabled())
        self.assertEqual(len(get_programs_list()), len(self.ns3_executables))

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

    def test_08_DryRun(self):
        """!
        Test dry-run (printing commands to be executed instead of running them)
        @return None
        """
        run_ns3("clean")

        # Try dry-run before and after the positional commands (outputs should match)
        for positional_command in ["configure", "build", "clean"]:
            return_code, stdout, stderr = run_ns3("--dry-run %s" % positional_command)
            return_code1, stdout1, stderr1 = run_ns3("%s --dry-run" % positional_command)

            self.assertEqual(return_code, return_code1)
            self.assertEqual(stdout, stdout1)
            self.assertEqual(stderr, stderr1)

        # Build target before using below
        run_ns3("configure -d release --enable-verbose")
        run_ns3("build scratch-simulator")

        # Run all cases and then check outputs
        return_code0, stdout0, stderr0 = run_ns3("--dry-run run scratch-simulator")
        return_code1, stdout1, stderr1 = run_ns3("run scratch-simulator")
        return_code2, stdout2, stderr2 = run_ns3("--dry-run run scratch-simulator --no-build")
        return_code3, stdout3, stderr3 = run_ns3("run scratch-simulator --no-build ")

        # Return code and stderr should be the same for all of them.
        self.assertEqual(sum([return_code0, return_code1, return_code2, return_code3]), 0)
        self.assertEqual([stderr0, stderr1, stderr2, stderr3], [""] * 4)

        scratch_path = None
        for program in get_programs_list():
            if "scratch-simulator" in program and "subdir" not in program:
                scratch_path = program
                break

        # Scratches currently have a 'scratch_' prefix in their CMake targets
        # Case 0: dry-run + run (should print commands to build target and then run)
        self.assertIn(cmake_build_target_command(target="scratch_scratch-simulator"), stdout0)
        self.assertIn(scratch_path, stdout0)

        # Case 1: run (should print all the commands of case 1 plus CMake output from build)
        self.assertIn(cmake_build_target_command(target="scratch_scratch-simulator"), stdout1)
        self.assertIn("Built target", stdout1)
        self.assertIn(scratch_path, stdout1)

        # Case 2: dry-run + run-no-build (should print commands to run the target)
        self.assertIn(scratch_path, stdout2)

        # Case 3: run-no-build (should print the target output only)
        self.assertEqual("", stdout3)

    def test_09_PropagationOfReturnCode(self):
        """!
        Test if ns3 is propagating back the return code from the executables called with the run command
        @return None
        """
        # From this point forward we are reconfiguring in debug mode
        return_code, _, _ = run_ns3("clean")
        self.assertEqual(return_code, 0)

        return_code, _, _ = run_ns3("configure --enable-examples --enable-tests")
        self.assertEqual(return_code, 0)

        # Build necessary executables
        return_code, stdout, stderr = run_ns3("build command-line-example test-runner")
        self.assertEqual(return_code, 0)

        # Now some tests will succeed normally
        return_code, stdout, stderr = run_ns3("run \"test-runner --test-name=command-line\" --no-build")
        self.assertEqual(return_code, 0)

        # Now some tests will fail during NS_COMMANDLINE_INTROSPECTION
        return_code, stdout, stderr = run_ns3("run \"test-runner --test-name=command-line\" --no-build",
                                              env={"NS_COMMANDLINE_INTROSPECTION": ".."}
                                              )
        self.assertNotEqual(return_code, 0)

    def test_10_CheckConfig(self):
        """!
        Test passing --check-config argument to ns3 to get the configuration table
        @return None
        """
        return_code, stdout, stderr = run_ns3("--check-config")
        self.assertEqual(return_code, 0)
        self.assertIn("Summary of optional NS-3 features", stdout)

    def test_11_CheckProfile(self):
        """!
        Test passing --check-profile argument to ns3 to get the build profile
        @return None
        """
        return_code, stdout, stderr = run_ns3("--check-profile")
        self.assertEqual(return_code, 0)
        self.assertIn("Build profile: debug", stdout)

    def test_12_CheckVersion(self):
        """!
        Test passing --check-version argument to ns3 to get the build version
        @return None
        """
        return_code, stdout, stderr = run_ns3("--check-version")
        self.assertEqual(return_code, 0)
        self.assertIn("ns-3 version:", stdout)


class NS3BuildBaseTestCase(NS3BaseTestCase):
    """!
    Tests ns3 regarding building the project
    """

    ## when cleaned_once is False, clean up build artifacts and reconfigure
    cleaned_once = False

    def setUp(self):
        """!
        Reuse cleaning/release configuration from NS3BaseTestCase if flag is cleaned
        @return None
        """
        if not NS3BuildBaseTestCase.cleaned_once:
            NS3BuildBaseTestCase.cleaned_once = True
            NS3BaseTestCase.cleaned_once = False
        super().setUp()

        self.ns3_libraries = get_libraries_list()

    def test_01_BuildExistingTargets(self):
        """!
        Try building the core library
        @return None
        """
        return_code, stdout, stderr = run_ns3("build core")
        self.assertEqual(return_code, 0)
        self.assertIn("Built target libcore", stdout)

    def test_02_BuildNonExistingTargets(self):
        """!
        Try building core-test library without tests enabled
        @return None
        """
        # tests are not enabled, so the target isn't available
        return_code, stdout, stderr = run_ns3("build core-test")
        self.assertGreater(len(stderr), 0)

    def test_03_BuildProject(self):
        """!
        Try building the project:
        @return None
        """
        return_code, stdout, stderr = run_ns3("build")
        self.assertEqual(return_code, 0)
        self.assertIn("Built target", stdout)
        for program in get_programs_list():
            self.assertTrue(os.path.exists(program))
        self.assertIn(cmake_build_project_command, stdout)

    def test_04_BuildProjectNoTaskLines(self):
        """!
        Try hiding task lines
        @return None
        """
        return_code, stdout, stderr = run_ns3("--quiet build")
        self.assertEqual(return_code, 0)
        self.assertIn(cmake_build_project_command, stdout)

    def test_05_BreakBuild(self):
        """!
        Try removing an essential file to break the build
        @return None
        """
        # change an essential file to break the build.
        attribute_cc_path = os.sep.join([ns3_path, "src", "core", "model", "attribute.cc"])
        attribute_cc_bak_path = attribute_cc_path + ".bak"
        shutil.move(attribute_cc_path, attribute_cc_bak_path)

        # build should break.
        return_code, stdout, stderr = run_ns3("build")
        self.assertNotEqual(return_code, 0)

        # move file back.
        shutil.move(attribute_cc_bak_path, attribute_cc_path)

        # build should work again.
        return_code, stdout, stderr = run_ns3("build")
        self.assertEqual(return_code, 0)

    def test_06_TestVersionFile(self):
        """!
        Test if changing the version file affects the library names
        @return None
        """
        version_file = os.sep.join([ns3_path, "VERSION"])
        with open(version_file, "w") as f:
            f.write("3-00\n")

        # Reconfigure.
        return_code, stdout, stderr = run_ns3("configure")
        self.config_ok(return_code, stdout)

        # Build.
        return_code, stdout, stderr = run_ns3("build")
        self.assertEqual(return_code, 0)
        self.assertIn("Built target", stdout)

        # Programs with new versions.
        new_programs = get_programs_list()

        # Check if they exist.
        for program in new_programs:
            self.assertTrue(os.path.exists(program))

        # Check if we still have the same number of binaries.
        self.assertEqual(len(new_programs), len(self.ns3_executables))

        # Check if versions changed from 3-dev to 3-00.
        libraries = get_libraries_list()
        new_libraries = list(set(libraries).difference(set(self.ns3_libraries)))
        self.assertEqual(len(new_libraries), len(self.ns3_libraries))
        for library in new_libraries:
            self.assertNotIn("libns3-dev", library)
            self.assertIn("libns3-00", library)
            self.assertTrue(os.path.exists(library))

        # Restore version file.
        with open(version_file, "w") as f:
            f.write("3-dev\n")

        # Reset flag to let it clean the build.
        NS3BuildBaseTestCase.cleaned_once = False

    def test_07_OutputDirectory(self):
        """!
        Try setting a different output directory and if everything is
        in the right place and still working correctly
        @return None
        """
        # Re-build to return to the original state.
        run_ns3("build")

        ## ns3_libraries holds a list of built module libraries
        self.ns3_libraries = get_libraries_list()

        ## ns3_executables holds a list of executables in build-status.py
        self.ns3_executables = get_programs_list()

        # Delete built programs and libraries to check if they were restored later.
        for program in self.ns3_executables:
            os.remove(program)
        for library in self.ns3_libraries:
            os.remove(library)

        # Reconfigure setting the output folder to ns-3-dev/build/release (both as an absolute path or relative).
        absolute_path = os.sep.join([ns3_path, "build", "release"])
        relative_path = os.sep.join(["build", "release"])
        for different_out_dir in [absolute_path, relative_path]:
            return_code, stdout, stderr = run_ns3("configure --out=%s" % different_out_dir)
            self.config_ok(return_code, stdout)
            self.assertIn("Build directory               : %s" % absolute_path, stdout)

            # Build
            run_ns3("build")

            # Check if we have the same number of binaries and that they were built correctly.
            new_programs = get_programs_list(os.sep.join([absolute_path, "build-status.py"]))
            self.assertEqual(len(new_programs), len(self.ns3_executables))
            for program in new_programs:
                self.assertTrue(os.path.exists(program))

            # Check if we have the same number of libraries and that they were built correctly.
            libraries = get_libraries_list(os.sep.join([absolute_path, "lib"]))
            new_libraries = list(set(libraries).difference(set(self.ns3_libraries)))
            self.assertEqual(len(new_libraries), len(self.ns3_libraries))
            for library in new_libraries:
                self.assertTrue(os.path.exists(library))

            # Remove files in the different output dir.
            shutil.rmtree(absolute_path)

        # Restore original output directory.
        return_code, stdout, stderr = run_ns3("configure --out=''")
        self.config_ok(return_code, stdout)
        self.assertIn("Build directory               : %s" % usual_outdir, stdout)

        # Try re-building.
        run_ns3("build")

        # Check if we have the same binaries we had at the beginning.
        new_programs = get_programs_list()
        self.assertEqual(len(new_programs), len(self.ns3_executables))
        for program in new_programs:
            self.assertTrue(os.path.exists(program))

        # Check if we have the same libraries we had at the beginning.
        libraries = get_libraries_list()
        self.assertEqual(len(libraries), len(self.ns3_libraries))
        for library in libraries:
            self.assertTrue(os.path.exists(library))

    def test_08_InstallationAndUninstallation(self):
        """!
        Tries setting a ns3 version, then installing it.
        After that, tries searching for ns-3 with CMake's find_package(ns3).
        Finally, tries using core library in a 3rd-party project
        @return None
        """
        # Remove existing libraries from the previous step.
        libraries = get_libraries_list()
        for library in libraries:
            os.remove(library)

        # 3-dev version format is not supported by CMake, so we use 3.01.
        version_file = os.sep.join([ns3_path, "VERSION"])
        with open(version_file, "w") as f:
            f.write("3-01\n")

        # Reconfigure setting the installation folder to ns-3-dev/build/install.
        install_prefix = os.sep.join([ns3_path, "build", "install"])
        return_code, stdout, stderr = run_ns3("configure --prefix=%s" % install_prefix)
        self.config_ok(return_code, stdout)

        # Build.
        run_ns3("build")
        libraries = get_libraries_list()
        headers = get_headers_list()

        # Install.
        run_ns3("install")

        # Find out if libraries were installed to lib or lib64 (Fedora thing).
        lib64 = os.path.exists(os.sep.join([install_prefix, "lib64"]))
        installed_libdir = os.sep.join([install_prefix, ("lib64" if lib64 else "lib")])

        # Make sure all libraries were installed.
        installed_libraries = get_libraries_list(installed_libdir)
        installed_libraries_list = ";".join(installed_libraries)
        for library in libraries:
            library_name = os.path.basename(library)
            self.assertIn(library_name, installed_libraries_list)

        # Make sure all headers were installed.
        installed_headers = get_headers_list(install_prefix)
        missing_headers = list(set([os.path.basename(x) for x in headers])
                               - (set([os.path.basename(x) for x in installed_headers]))
                               )
        self.assertEqual(len(missing_headers), 0)

        # Now create a test CMake project and try to find_package ns-3.
        test_main_file = os.sep.join([install_prefix, "main.cpp"])
        with open(test_main_file, "w") as f:
            f.write("""
            #include <ns3/core-module.h>
            using namespace ns3;
            int main () 
            { 
                Simulator::Stop (Seconds (1.0));
                Simulator::Run ();
                Simulator::Destroy ();
                return 0;
            }
            """)

        # We try to use this library without specifying a version,
        # specifying ns3-01 (text version with 'dev' is not supported)
        # and specifying ns3-00 (a wrong version)
        for version in ["", "3.01", "3.00"]:
            test_cmake_project = """
            cmake_minimum_required(VERSION 3.10..3.10)
            project(ns3_consumer CXX)
            
            list(APPEND CMAKE_PREFIX_PATH ./{lib}/cmake/ns3)
            find_package(ns3 {version} COMPONENTS libcore)
            add_executable(test main.cpp)
            target_link_libraries(test PRIVATE ns3::libcore)
            """.format(lib=("lib64" if lib64 else "lib"), version=version)

            test_cmake_project_file = os.sep.join([install_prefix, "CMakeLists.txt"])
            with open(test_cmake_project_file, "w") as f:
                f.write(test_cmake_project)

            # Configure the test project
            cmake = shutil.which("cmake")
            return_code, stdout, stderr = run_program(cmake,
                                                      "-DCMAKE_BUILD_TYPE=debug .",
                                                      cwd=install_prefix)

            if version == "3.00":
                self.assertEqual(return_code, 1)
                self.assertIn('Could not find a configuration file for package "ns3" that is compatible',
                              stderr.replace("\n", ""))
            else:
                self.assertEqual(return_code, 0)
                self.assertIn("Build files", stdout)

            # Build the test project making use of import ns-3
            return_code, stdout, stderr = run_program("cmake", "--build .", cwd=install_prefix)

            if version == "3.00":
                self.assertEqual(return_code, 2)
                self.assertGreater(len(stderr), 0)
            else:
                self.assertEqual(return_code, 0)
                self.assertIn("Built target", stdout)

                # Try running the test program that imports ns-3
                return_code, stdout, stderr = run_program("./test", "", cwd=install_prefix)
                self.assertEqual(return_code, 0)

        # Uninstall
        return_code, stdout, stderr = run_ns3("uninstall")
        self.assertIn("Built target uninstall", stdout)

        # Restore 3-dev version file
        with open(version_file, "w") as f:
            f.write("3-dev\n")

        # Reset flag to let it clean the build
        NS3BuildBaseTestCase.cleaned_once = False

    def test_09_Scratches(self):
        """!
        Tries to build scratch-simulator and subdir/scratch-simulator-subdir
        @return None
        """
        # Build.
        targets = {"scratch/scratch-simulator": "scratch-simulator",
                   "scratch-simulator": "scratch-simulator",
                   "scratch/subdir/scratch-simulator-subdir": "subdir_scratch-simulator-subdir",
                   "subdir/scratch-simulator-subdir": "subdir_scratch-simulator-subdir",
                   "scratch-simulator-subdir": "subdir_scratch-simulator-subdir",
                   }
        for (target_to_run, target_cmake) in targets.items():
            # Test if build is working.
            build_line = "target scratch_%s" % target_cmake
            return_code, stdout, stderr = run_ns3("build %s" % target_to_run)
            self.assertEqual(return_code, 0)
            self.assertIn(build_line, stdout)

            # Test if run is working
            return_code, stdout, stderr = run_ns3("run %s" % target_to_run)
            self.assertEqual(return_code, 0)
            self.assertIn(build_line, stdout)
            stdout = stdout.replace("scratch_%s" % target_cmake, "")  # remove build lines
            self.assertIn(target_to_run.split("/")[-1], stdout)


class NS3ExpectedUseTestCase(NS3BaseTestCase):
    """!
    Tests ns3 usage in more realistic scenarios
    """

    ## when cleaned_once is False, clean up build artifacts and reconfigure
    cleaned_once = False

    def setUp(self):
        """!
        Reuse cleaning/release configuration from NS3BaseTestCase if flag is cleaned
        Here examples, tests and documentation are also enabled.
        @return None
        """
        if not NS3ExpectedUseTestCase.cleaned_once:
            NS3ExpectedUseTestCase.cleaned_once = True
            NS3BaseTestCase.cleaned_once = False
            super().setUp()

            # On top of the release build configured by NS3ConfigureTestCase, also enable examples, tests and docs.
            return_code, stdout, stderr = run_ns3("configure --enable-examples --enable-tests")
            self.config_ok(return_code, stdout)

        # Check if build-status.py exists, then read to get list of executables.
        self.assertTrue(os.path.exists(usual_build_status_script))

        ## ns3_executables holds a list of executables in build-status.py
        self.ns3_executables = get_programs_list()

        # Check if c4che.py exists than read to get the list of enabled modules.
        self.assertTrue(os.path.exists(usual_c4che_script))

        ## ns3_modules holds a list to the modules enabled stored in c4che.py
        self.ns3_modules = get_enabled_modules()

    def test_01_BuildProject(self):
        """!
        Try to build the project
        @return None
        """
        return_code, stdout, stderr = run_ns3("build")
        self.assertEqual(return_code, 0)
        self.assertIn("Built target", stdout)
        for program in get_programs_list():
            self.assertTrue(os.path.exists(program))
        libraries = get_libraries_list()
        for module in get_enabled_modules():
            self.assertIn(module.replace("ns3-", ""), ";".join(libraries))
        self.assertIn(cmake_build_project_command, stdout)

    def test_02_BuildAndRunExistingExecutableTarget(self):
        """!
        Try to build and run test-runner
        @return None
        """
        return_code, stdout, stderr = run_ns3('run "test-runner --list"')
        self.assertEqual(return_code, 0)
        self.assertIn("Built target test-runner", stdout)
        self.assertIn(cmake_build_target_command(target="test-runner"), stdout)

    def test_03_BuildAndRunExistingLibraryTarget(self):
        """!
        Try to build and run a library
        @return None
        """
        return_code, stdout, stderr = run_ns3("run core")  # this should not work
        self.assertEqual(return_code, 1)
        self.assertIn("Couldn't find the specified program: core", stderr)

    def test_04_BuildAndRunNonExistingTarget(self):
        """!
        Try to build and run an unknown target
        @return None
        """
        return_code, stdout, stderr = run_ns3("run nonsense")  # this should not work
        self.assertEqual(return_code, 1)
        self.assertIn("Couldn't find the specified program: nonsense", stderr)

    def test_05_RunNoBuildExistingExecutableTarget(self):
        """!
        Try to run test-runner without building
        @return None
        """
        return_code, stdout, stderr = run_ns3('run "test-runner --list" --no-build ')
        self.assertEqual(return_code, 0)
        self.assertNotIn("Built target test-runner", stdout)
        self.assertNotIn(cmake_build_target_command(target="test-runner"), stdout)

    def test_06_RunNoBuildExistingLibraryTarget(self):
        """!
        Test ns3 fails to run a library
        @return None
        """
        return_code, stdout, stderr = run_ns3("run core --no-build")  # this should not work
        self.assertEqual(return_code, 1)
        self.assertIn("Couldn't find the specified program: core", stderr)

    def test_07_RunNoBuildNonExistingExecutableTarget(self):
        """!
        Test ns3 fails to run an unknown program
        @return None
        """
        return_code, stdout, stderr = run_ns3("run nonsense --no-build")  # this should not work
        self.assertEqual(return_code, 1)
        self.assertIn("Couldn't find the specified program: nonsense", stderr)

    def test_08_RunNoBuildGdb(self):
        """!
        Test if scratch simulator is executed through gdb
        @return None
        """
        return_code, stdout, stderr = run_ns3("run scratch-simulator --gdb --no-build")
        self.assertEqual(return_code, 0)
        self.assertIn("scratch-simulator", stdout)
        self.assertIn("No debugging symbols found", stdout)

    def test_09_RunNoBuildValgrind(self):
        """!
      Test if scratch simulator is executed through valgrind
      @return None
      """
        return_code, stdout, stderr = run_ns3("run scratch-simulator --valgrind --no-build")
        self.assertEqual(return_code, 0)
        self.assertIn("scratch-simulator", stderr)
        self.assertIn("Memcheck", stderr)

    def test_10_DoxygenWithBuild(self):
        """!
        Test the doxygen target that does trigger a full build
        @return None
        """
        doc_folder = os.path.abspath(os.sep.join([".", "doc"]))

        doxygen_files = ["introspected-command-line.h", "introspected-doxygen.h"]
        for filename in doxygen_files:
            file_path = os.sep.join([doc_folder, filename])
            if os.path.exists(file_path):
                os.remove(file_path)

        # Rebuilding dot images is super slow, so not removing doxygen products
        # doxygen_build_folder = os.sep.join([doc_folder, "html"])
        # if os.path.exists(doxygen_build_folder):
        #     shutil.rmtree(doxygen_build_folder)

        return_code, stdout, stderr = run_ns3("docs doxygen")
        self.assertEqual(return_code, 0)
        self.assertIn(cmake_build_target_command(target="doxygen"), stdout)
        self.assertIn("Built target doxygen", stdout)

    def test_11_DoxygenWithoutBuild(self):
        """!
        Test the doxygen target that doesn't trigger a full build
        @return None
        """
        # Rebuilding dot images is super slow, so not removing doxygen products
        # doc_folder = os.path.abspath(os.sep.join([".", "doc"]))
        # doxygen_build_folder = os.sep.join([doc_folder, "html"])
        # if os.path.exists(doxygen_build_folder):
        #     shutil.rmtree(doxygen_build_folder)

        return_code, stdout, stderr = run_ns3("docs doxygen-no-build")
        self.assertEqual(return_code, 0)
        self.assertIn(cmake_build_target_command(target="doxygen-no-build"), stdout)
        self.assertIn("Built target doxygen-no-build", stdout)

    def test_12_SphinxDocumentation(self):
        """!
        Test every individual target for Sphinx-based documentation
        @return None
        """
        doc_folder = os.path.abspath(os.sep.join([".", "doc"]))

        # First we need to clean old docs, or it will not make any sense.
        for target in ["manual", "models", "tutorial"]:
            doc_build_folder = os.sep.join([doc_folder, target, "build"])
            if os.path.exists(doc_build_folder):
                shutil.rmtree(doc_build_folder)

        # For each sphinx doc target.
        for target in ["manual", "models", "tutorial"]:
            # Build
            return_code, stdout, stderr = run_ns3("docs %s" % target)
            self.assertEqual(return_code, 0)
            self.assertIn(cmake_build_target_command(target="sphinx_%s" % target), stdout)
            self.assertIn("Built target sphinx_%s" % target, stdout)

            # Check if the docs output folder exists
            doc_build_folder = os.sep.join([doc_folder, target, "build"])
            self.assertTrue(os.path.exists(doc_build_folder))

            # Check if the all the different types are in place (latex, split HTML and single page HTML)
            for build_type in ["latex", "html", "singlehtml"]:
                self.assertTrue(os.path.exists(os.sep.join([doc_build_folder, build_type])))

    def test_13_Documentation(self):
        """!
        Test the documentation target that builds
        both doxygen and sphinx based documentation
        @return None
        """
        doc_folder = os.path.abspath(os.sep.join([".", "doc"]))

        # First we need to clean old docs, or it will not make any sense.

        # Rebuilding dot images is super slow, so not removing doxygen products
        # doxygen_build_folder = os.sep.join([doc_folder, "html"])
        # if os.path.exists(doxygen_build_folder):
        #     shutil.rmtree(doxygen_build_folder)

        for target in ["manual", "models", "tutorial"]:
            doc_build_folder = os.sep.join([doc_folder, target, "build"])
            if os.path.exists(doc_build_folder):
                shutil.rmtree(doc_build_folder)

        return_code, stdout, stderr = run_ns3("docs all")
        self.assertEqual(return_code, 0)
        self.assertIn(cmake_build_target_command(target="sphinx"), stdout)
        self.assertIn("Built target sphinx", stdout)
        self.assertIn(cmake_build_target_command(target="doxygen"), stdout)
        self.assertIn("Built target doxygen", stdout)

    def test_14_Check(self):
        """!
        Test if ns3 --check is working as expected
        @return None
        """
        return_code, stdout, stderr = run_ns3("--check")
        self.assertEqual(return_code, 0)

    def test_15_EnableSudo(self):
        """!
        Try to set ownership of scratch-simulator from current user to root,
        and change execution permissions
        @return None
        """

        # Test will be skipped if not defined
        sudo_password = os.getenv("SUDO_PASSWORD", None)

        # Skip test if variable containing sudo password is the default value
        if sudo_password is None:
            return

        enable_sudo = read_c4che_entry("ENABLE_SUDO")
        self.assertFalse(enable_sudo is True)

        # First we run to ensure the program was built
        return_code, stdout, stderr = run_ns3('run scratch-simulator')
        self.assertEqual(return_code, 0)
        self.assertIn("Built target scratch_scratch-simulator", stdout)
        self.assertIn(cmake_build_target_command(target="scratch_scratch-simulator"), stdout)
        scratch_simulator_path = list(filter(lambda x: x if "scratch-simulator" in x else None,
                                             self.ns3_executables
                                             )
                                      )[-1]
        prev_fstat = os.stat(scratch_simulator_path)  # we get the permissions before enabling sudo

        # Now try setting the sudo bits from the run subparser
        return_code, stdout, stderr = run_ns3('run scratch-simulator --enable-sudo',
                                              env={"SUDO_PASSWORD": sudo_password})
        self.assertEqual(return_code, 0)
        self.assertIn("Built target scratch_scratch-simulator", stdout)
        self.assertIn(cmake_build_target_command(target="scratch_scratch-simulator"), stdout)
        fstat = os.stat(scratch_simulator_path)

        import stat
        # If we are on Windows, these permissions mean absolutely nothing,
        # and on Fuse builds they might not make any sense, so we need to skip before failing
        likely_fuse_mount = ((prev_fstat.st_mode & stat.S_ISUID) == (fstat.st_mode & stat.S_ISUID)) and \
                            prev_fstat.st_uid == 0

        if sys.platform == "win32" or likely_fuse_mount:
            return

        # If this is a valid platform, we can continue
        self.assertEqual(fstat.st_uid, 0)  # check the file was correctly chown'ed by root
        self.assertEqual(fstat.st_mode & stat.S_ISUID, stat.S_ISUID)  # check if normal users can run as sudo

        # Now try setting the sudo bits as a post-build step (as set by configure subparser)
        return_code, stdout, stderr = run_ns3('configure --enable-sudo')
        self.assertEqual(return_code, 0)

        # Check if it was properly set in the c4che file
        enable_sudo = read_c4che_entry("ENABLE_SUDO")
        self.assertTrue(enable_sudo)

        # Remove old executables
        for executable in self.ns3_executables:
            if os.path.exists(executable):
                os.remove(executable)

        # Try to build and then set sudo bits as a post-build step
        return_code, stdout, stderr = run_ns3('build', env={"SUDO_PASSWORD": sudo_password})
        self.assertEqual(return_code, 0)

        # Check if commands are being printed for every target
        self.assertIn("chown root", stdout)
        self.assertIn("chmod u+s", stdout)
        for executable in self.ns3_executables:
            self.assertIn(os.path.basename(executable), stdout)

        # Check scratch simulator yet again
        fstat = os.stat(scratch_simulator_path)
        self.assertEqual(fstat.st_uid, 0)  # check the file was correctly chown'ed by root
        self.assertEqual(fstat.st_mode & stat.S_ISUID, stat.S_ISUID)  # check if normal users can run as sudo

    def test_16_CommandTemplate(self):
        """!
        Check if command template is working
        @return None
        """

        # Command templates that are empty or do not have a %s should fail
        return_code0, stdout0, stderr0 = run_ns3('run sample-simulator --command-template')
        self.assertEqual(return_code0, 2)
        self.assertIn("argument --command-template: expected one argument", stderr0)

        return_code1, stdout1, stderr1 = run_ns3('run sample-simulator --command-template=" "')
        return_code2, stdout2, stderr2 = run_ns3('run sample-simulator --command-template " "')
        return_code3, stdout3, stderr3 = run_ns3('run sample-simulator --command-template "echo "')
        self.assertEqual((return_code1, return_code2, return_code3), (1, 1, 1))
        self.assertIn("not all arguments converted during string formatting", stderr1)
        self.assertEqual(stderr1, stderr2)
        self.assertEqual(stderr2, stderr3)

        # Command templates with %s should at least continue and try to run the target
        return_code4, stdout4, stderr4 = run_ns3('run sample-simulator --command-template "%s --PrintVersion"')
        return_code5, stdout5, stderr5 = run_ns3('run sample-simulator --command-template="%s --PrintVersion"')
        self.assertEqual((return_code4, return_code5), (0, 0))
        self.assertIn("sample-simulator --PrintVersion", stdout4)
        self.assertIn("sample-simulator --PrintVersion", stdout5)

    def test_17_ForwardArgumentsToRunTargets(self):
        """!
        Check if all flavors of different argument passing to
        executable targets are working
        @return None
        """

        # Test if all argument passing flavors are working
        return_code0, stdout0, stderr0 = run_ns3('run "sample-simulator --help"')
        return_code1, stdout1, stderr1 = run_ns3('run sample-simulator --command-template="%s --help"')
        return_code2, stdout2, stderr2 = run_ns3('run sample-simulator -- --help')

        self.assertEqual((return_code0, return_code1, return_code2), (0, 0, 0))
        self.assertIn("sample-simulator --help", stdout0)
        self.assertIn("sample-simulator --help", stdout1)
        self.assertIn("sample-simulator --help", stdout2)

        # Test if the same thing happens with an additional run argument (e.g. --no-build)
        return_code0, stdout0, stderr0 = run_ns3('run "sample-simulator --help" --no-build')
        return_code1, stdout1, stderr1 = run_ns3('run sample-simulator --command-template="%s --help" --no-build')
        return_code2, stdout2, stderr2 = run_ns3('run sample-simulator --no-build -- --help')
        self.assertEqual((return_code0, return_code1, return_code2), (0, 0, 0))
        self.assertEqual(stdout0, stdout1)
        self.assertEqual(stdout1, stdout2)
        self.assertEqual(stderr0, stderr1)
        self.assertEqual(stderr1, stderr2)

        # Now collect results for each argument individually
        return_code0, stdout0, stderr0 = run_ns3('run "sample-simulator --PrintGlobals"')
        return_code1, stdout1, stderr1 = run_ns3('run "sample-simulator --PrintGroups"')
        return_code2, stdout2, stderr2 = run_ns3('run "sample-simulator --PrintTypeIds"')

        self.assertEqual((return_code0, return_code1, return_code2), (0, 0, 0))
        self.assertIn("sample-simulator --PrintGlobals", stdout0)
        self.assertIn("sample-simulator --PrintGroups", stdout1)
        self.assertIn("sample-simulator --PrintTypeIds", stdout2)

        # Then check if all the arguments are correctly merged by checking the outputs
        cmd = 'run "sample-simulator --PrintGlobals" --command-template="%s --PrintGroups" -- --PrintTypeIds'
        return_code, stdout, stderr = run_ns3(cmd)
        self.assertEqual(return_code, 0)

        # The order of the arguments is command template,
        # arguments passed with the target itself
        # and forwarded arguments after the -- separator
        self.assertIn("sample-simulator --PrintGroups --PrintGlobals --PrintTypeIds", stdout)

        # Check if it complains about the missing -- separator
        cmd0 = 'run sample-simulator --command-template="%s " --PrintTypeIds'
        cmd1 = 'run sample-simulator --PrintTypeIds'

        return_code0, stdout0, stderr0 = run_ns3(cmd0)
        return_code1, stdout1, stderr1 = run_ns3(cmd1)
        self.assertEqual((return_code0, return_code1), (1, 1))
        self.assertIn("To forward configuration or runtime options, put them after '--'", stderr0)
        self.assertIn("To forward configuration or runtime options, put them after '--'", stderr1)


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
