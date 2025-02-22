#! /usr/bin/env python3
import argparse
import os
import re
import shutil
import sys
from pathlib import Path

CMAKELISTS_TEMPLATE = """\
check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

set(examples_as_tests_sources)
if(${{ENABLE_EXAMPLES}})
    set(examples_as_tests_sources
        #test/{MODULE}-examples-test-suite.cc
        )
endif()

build_lib(
    LIBNAME {MODULE}
    SOURCE_FILES model/{MODULE}.cc
                 helper/{MODULE}-helper.cc
    HEADER_FILES model/{MODULE}.h
                 helper/{MODULE}-helper.h
    LIBRARIES_TO_LINK ${{libcore}}
    TEST_SOURCES test/{MODULE}-test-suite.cc
                 ${{examples_as_tests_sources}}
)
"""


MODEL_CC_TEMPLATE = """\
#include "{MODULE}.h"

namespace ns3
{{

/* ... */

}} // namespace ns3
"""


MODEL_H_TEMPLATE = """\
#ifndef {INCLUDE_GUARD}
#define {INCLUDE_GUARD}

// Add a doxygen group for this module.
// If you have more than one file, this should be in only one of them.
/**
 * @defgroup {MODULE} Description of the {MODULE}
 */

namespace ns3
{{

// Each class should be documented using Doxygen,
// and have an @ingroup {MODULE} directive

/* ... */

}} // namespace ns3

#endif // {INCLUDE_GUARD}
"""


HELPER_CC_TEMPLATE = """\
#include "{MODULE}-helper.h"

namespace ns3
{{

/* ... */

}} // namespace ns3
"""


HELPER_H_TEMPLATE = """\
#ifndef {INCLUDE_GUARD}
#define {INCLUDE_GUARD}

#include "ns3/{MODULE}.h"

namespace ns3
{{

// Each class should be documented using Doxygen,
// and have an @ingroup {MODULE} directive

/* ... */

}} // namespace ns3

#endif // {INCLUDE_GUARD}
"""


EXAMPLES_CMAKELISTS_TEMPLATE = """\
build_lib_example(
    NAME {MODULE}-example
    SOURCE_FILES {MODULE}-example.cc
    LIBRARIES_TO_LINK ${{lib{MODULE}}}
)
"""

EXAMPLE_CC_TEMPLATE = """\
#include "ns3/core-module.h"
#include "ns3/{MODULE}-helper.h"

/**
 * @file
 *
 * Explain here what the example does.
 */

using namespace ns3;

int
main(int argc, char* argv[])
{{
    bool verbose = true;

    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "Tell application to log if true", verbose);

    cmd.Parse(argc, argv);

    /* ... */

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}}
"""


TEST_CC_TEMPLATE = """\
// Include a header file from your module to test.
#include "ns3/{MODULE}.h"

// An essential include is test.h
#include "ns3/test.h"

// Do not put your test classes in namespace ns3.  You may find it useful
// to use the using directive to access the ns3 namespace directly
using namespace ns3;

// Add a doxygen group for tests.
// If you have more than one test, this should be in only one of them.
/**
 * @defgroup {MODULE}-tests Tests for {MODULE}
 * @ingroup {MODULE}
 * @ingroup tests
 */

// This is an example TestCase.
/**
 * @ingroup {MODULE}-tests
 * Test case for feature 1
 */
class {CAPITALIZED}TestCase1 : public TestCase
{{
  public:
    {CAPITALIZED}TestCase1();
    ~{CAPITALIZED}TestCase1() override;

  private:
    void DoRun() override;
}};

// Add some help text to this case to describe what it is intended to test
{CAPITALIZED}TestCase1::{CAPITALIZED}TestCase1()
    : TestCase("{CAPITALIZED} test case (does nothing)")
{{
}}

// This destructor does nothing but we include it as a reminder that
// the test case should clean up after itself
{CAPITALIZED}TestCase1::~{CAPITALIZED}TestCase1()
{{
}}

//
// This method is the pure virtual method from class TestCase that every
// TestCase must implement
//
void
{CAPITALIZED}TestCase1::DoRun()
{{
    // A wide variety of test macros are available in src/core/test.h
    NS_TEST_ASSERT_MSG_EQ(true, true, "true doesn't equal true for some reason");
    // Use this one for floating point comparisons
    NS_TEST_ASSERT_MSG_EQ_TOL(0.01, 0.01, 0.001, "Numbers are not equal within tolerance");
}}

// The TestSuite class names the TestSuite, identifies what type of TestSuite,
// and enables the TestCases to be run.  Typically, only the constructor for
// this class must be defined

/**
 * @ingroup {MODULE}-tests
 * TestSuite for module {MODULE}
 */
class {CAPITALIZED}TestSuite : public TestSuite
{{
  public:
    {CAPITALIZED}TestSuite();
}};

// Type for TestSuite can be UNIT, SYSTEM, EXAMPLE, or PERFORMANCE
{CAPITALIZED}TestSuite::{CAPITALIZED}TestSuite()
    : TestSuite("{MODULE}", Type::UNIT)
{{
    // Duration for TestCase can be QUICK, EXTENSIVE or TAKES_FOREVER
    AddTestCase(new {CAPITALIZED}TestCase1, TestCase::Duration::QUICK);
}}

// Do not forget to allocate an instance of this TestSuite
/**
 * @ingroup {MODULE}-tests
 * Static variable for test initialization
 */
static {CAPITALIZED}TestSuite s{COMPOUND}TestSuite;
"""


DOC_RST_TEMPLATE = """Example Module Documentation
----------------------------

.. include:: replace.txt
.. highlight:: cpp

.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

This is a suggested outline for adding new module documentation to |ns3|.
See ``src/click/doc/click.rst`` for an example.

The introductory paragraph is for describing what this code is trying to
model.

For consistency (italicized formatting), please use |ns3| to refer to
ns-3 in the documentation (and likewise, |ns2| for ns-2).  These macros
are defined in the file ``replace.txt``.

Model Description
*****************

The source code for the new module lives in the directory ``{MODULE_DIR}``.

Add here a basic description of what is being modeled.

Design
======

Briefly describe the software design of the model and how it fits into
the existing ns-3 architecture.

Scope and Limitations
=====================

What can the model do?  What can it not do?  Please use this section to
describe the scope and limitations of the model.

References
==========

Add academic citations here, such as if you published a paper on this
model, or if readers should read a particular specification or other work.

Usage
*****

This section is principally concerned with the usage of your model, using
the public API.  Focus first on most common usage patterns, then go
into more advanced topics.

Building New Module
===================

Include this subsection only if there are special build instructions or
platform limitations.

Helpers
=======

What helper API will users typically use?  Describe it here.

Attributes
==========

What classes hold attributes, and what are the key ones worth mentioning?

Output
======

What kind of data does the model generate?  What are the key trace
sources?   What kind of logging output can be enabled?

Advanced Usage
==============

Go into further details (such as using the API outside of the helpers)
in additional sections, as needed.

Examples
========

What examples using this new code are available?  Describe them here.

Troubleshooting
===============

Add any tips for avoiding pitfalls, etc.

Validation
**********

Describe how the model has been tested/validated.  What tests run in the
test suite?  How much API and code is covered by the tests?  Again,
references to outside published work may help here.
"""


def create_file(path, template, **kwargs):
    artifact_path = Path(path)

    # open file for (w)rite and in (t)ext mode
    with artifact_path.open("wt", encoding="utf-8") as f:
        f.write(template.format(**kwargs))


def make_cmakelists(moduledir, modname):
    path = Path(moduledir, "CMakeLists.txt")
    macro = "build_lib"
    create_file(path, CMAKELISTS_TEMPLATE, MODULE=modname)

    return True


def make_model(moduledir, modname):
    modelpath = Path(moduledir, "model")
    modelpath.mkdir(parents=True)

    srcfile_path = modelpath.joinpath(modname).with_suffix(".cc")
    create_file(srcfile_path, MODEL_CC_TEMPLATE, MODULE=modname)

    hfile_path = modelpath.joinpath(modname).with_suffix(".h")
    guard = "{}_H".format(modname.replace("-", "_").upper())
    create_file(hfile_path, MODEL_H_TEMPLATE, MODULE=modname, INCLUDE_GUARD=guard)

    return True


def make_test(moduledir, modname):
    testpath = Path(moduledir, "test")
    testpath.mkdir(parents=True)

    file_path = testpath.joinpath(modname + "-test-suite").with_suffix(".cc")
    name_parts = modname.split("-")
    create_file(
        file_path,
        TEST_CC_TEMPLATE,
        MODULE=modname,
        CAPITALIZED="".join([word.capitalize() for word in name_parts]),
        COMPOUND="".join(
            [word.capitalize() if index > 0 else word for index, word in enumerate(name_parts)]
        ),
    )

    return True


def make_helper(moduledir, modname):
    helperpath = Path(moduledir, "helper")
    helperpath.mkdir(parents=True)

    srcfile_path = helperpath.joinpath(modname + "-helper").with_suffix(".cc")
    create_file(srcfile_path, HELPER_CC_TEMPLATE, MODULE=modname)

    h_file_path = helperpath.joinpath(modname + "-helper").with_suffix(".h")
    guard = "{}_HELPER_H".format(modname.replace("-", "_").upper())
    create_file(h_file_path, HELPER_H_TEMPLATE, MODULE=modname, INCLUDE_GUARD=guard)

    return True


def make_examples(moduledir, modname):
    examplespath = Path(moduledir, "examples")
    examplespath.mkdir(parents=True)

    cmakelistspath = Path(examplespath, "CMakeLists.txt")
    create_file(cmakelistspath, EXAMPLES_CMAKELISTS_TEMPLATE, MODULE=modname)

    examplesfile_path = examplespath.joinpath(modname + "-example").with_suffix(".cc")
    create_file(examplesfile_path, EXAMPLE_CC_TEMPLATE, MODULE=modname)

    return True


def make_doc(moduledir, modname):
    docpath = Path(moduledir, "doc")
    docpath.mkdir(parents=True)

    # the module_dir template parameter must be a relative path
    # instead of an absolute path
    mod_relpath = os.path.relpath(str(moduledir))

    file_name = "{}.rst".format(modname)
    file_path = Path(docpath, file_name)
    create_file(file_path, DOC_RST_TEMPLATE, MODULE=modname, MODULE_DIR=mod_relpath)

    return True


def make_module(modpath, modname):
    modulepath = Path(modpath, modname)

    if modulepath.exists():
        print("Module {!r} already exists".format(modname), file=sys.stderr)
        return False

    print("Creating module {}".format(modulepath))

    functions = (make_cmakelists, make_model, make_test, make_helper, make_examples, make_doc)

    try:
        modulepath.mkdir(parents=True)

        success = all(func(modulepath, modname) for func in functions)

        if not success:
            raise ValueError("Generating module artifacts failed")

    except Exception as e:
        if modulepath.exists():
            shutil.rmtree(modulepath)

        print("Creating module {!r} failed: {}".format(modname, str(e)), file=sys.stderr)

        return False

    return True


def create_argument_parser():
    description = """Generate scaffolding for ns-3 modules

Generates the directory structure and skeleton files required for an ns-3
module.  All of the generated files are valid C/C++ and will compile successfully
out of the box.  ns3 configure must be run after creating new modules in order
to integrate them into the ns-3 build system.

The following directory structure is generated under the contrib directory:
<modname>
 |-- CMakeLists.txt
 |-- doc
     |-- <modname>.rst
 |-- examples
     |-- <modname>-example.cc
     |-- CMakeLists.txt
 |-- helper
     |-- <modname>-helper.cc
     |-- <modname>-helper.h
 |-- model
     |-- <modname>.cc
     |-- <modname>.h
 |-- test
     |-- <modname>-test-suite.cc


<modname> is the name of the module and is restricted to the following
character groups: letters, numbers, -, _
The script validates the module name and skips modules that have characters
outside of the above groups.  One exception to the naming rule is that src/
or contrib/ may be added to the front of the module name to indicate where the
module scaffold should be created.  If the module name starts with src/, then
the module is placed in the src directory.  If the module name starts with
contrib/, then the module is placed in the contrib directory.  If the module
name does not start with src/ or contrib/, then it defaults to contrib/.
See the examples section for use cases.


In some situations it can be useful to group multiple related modules under one
directory.  Use the --project option to specify a common parent directory where
the modules should be generated.  The value passed to --project is treated
as a relative path.  The path components have the same naming requirements as
the module name: letters, numbers, -, _
The project directory is placed under the contrib directory and any parts of the
path that do not exist will be created.  Creating projects in the src directory
is not supported.  Module names that start with src/ are not allowed when
--project is used.  Module names that start with contrib/ are treated the same
as module names that don't start with contrib/ and are generated under the
project directory.
"""

    epilog = """Examples:
  %(prog)s module1
  %(prog)s contrib/module1

      Creates a new module named module1 under the contrib directory

  %(prog)s src/module1

      Creates a new module named module1 under the src directory

  %(prog)s src/module1 contrib/module2, module3

      Creates three modules, one under the src directory and two under the
      contrib directory

  %(prog)s --project myproject module1 module2

      Creates two modules under contrib/myproject

  %(prog)s --project myproject/sub_project module1 module2

      Creates two modules under contrib/myproject/sub_project

"""

    formatter = argparse.RawDescriptionHelpFormatter

    parser = argparse.ArgumentParser(
        description=description, epilog=epilog, formatter_class=formatter
    )

    parser.add_argument(
        "--project",
        default="",
        help=(
            "Specify a relative path under the contrib directory "
            "where the new modules will be generated. The path "
            "will be created if it does not exist."
        ),
    )

    parser.add_argument(
        "modnames",
        nargs="+",
        help=(
            "One or more modules to generate.  Module names "
            "are limited to the following: letters, numbers, -, "
            "_. Modules are generated under the contrib directory "
            "except when the module name starts with src/. Modules "
            "that start with src/ are generated under the src "
            "directory."
        ),
    )

    return parser


def main(argv):
    parser = create_argument_parser()

    args = parser.parse_args(argv[1:])

    project = args.project
    modnames = args.modnames

    base_path = Path.cwd()

    src_path = base_path.joinpath("src")
    contrib_path = base_path.joinpath("contrib")

    for p in (src_path, contrib_path):
        if not p.is_dir():
            parser.error(
                "Cannot find the directory '{}'.\nPlease run this "
                "script from the top level of the ns3 directory".format(p)
            )

    #
    # Error check the arguments
    #

    # Alphanumeric and '-' only
    allowedRE = re.compile(r"^(\w|-)+$")

    project_path = None

    if project:
        # project may be a path in the form a/b/c
        # remove any leading or trailing path separators
        project_path = Path(project)

        if project_path.is_absolute():
            # remove leading separator
            project_path = project_path.relative_to(os.sep)

        if not all(allowedRE.match(part) for part in project_path.parts):
            parser.error("Project path may only contain the characters [a-zA-Z0-9_-].")
    #
    # Create each module, if it doesn't exist
    #
    modules = []
    for name in modnames:
        if name:
            # remove any leading or trailing directory separators
            name = name.strip(os.sep)

        if not name:
            # skip empty modules
            continue

        name_path = Path(name)

        if len(name_path.parts) > 2:
            print("Skipping {}: module name can not be a path".format(name))
            continue

        # default target directory is contrib
        modpath = contrib_path

        if name_path.parts[0] == "src":
            if project:
                parser.error(
                    "{}: Cannot specify src/ in a module name when --project option is used".format(
                        name
                    )
                )

            modpath = src_path

            # create a new path without the src part
            name_path = name_path.relative_to("src")

        elif name_path.parts[0] == "contrib":
            modpath = contrib_path

            # create a new path without the contrib part
            name_path = name_path.relative_to("contrib")

        if project_path:
            # if a project path was specified, that overrides other paths
            # project paths are always relative to the contrib path
            modpath = contrib_path.joinpath(project_path)

        modname = name_path.parts[0]

        if not allowedRE.match(modname):
            print(
                "Skipping {}: module name may only contain the characters [a-zA-Z0-9_-]".format(
                    modname
                )
            )
            continue

        modules.append((modpath, modname))

    if all(make_module(*module) for module in modules):
        print()
        print("Successfully created new modules")
        print("Run './ns3 configure' to include them in the build")

    return 0


if __name__ == "__main__":
    return_value = 0
    try:
        return_value = main(sys.argv)
    except Exception as e:
        print("Exception: '{}'".format(e), file=sys.stderr)
        return_value = 1

    sys.exit(return_value)
