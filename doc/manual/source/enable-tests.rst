.. include:: replace.txt
.. highlight:: bash


Enabling/disabling |ns3| Tests and Examples
-------------------------------------------

The |ns3| distribution includes many examples and tests that are used to validate the |ns3| system.  Users, however, may not always want these examples and tests to be run for their installation of |ns3|.

This chapter discusses how to build |ns3| with or without its examples and tests.

How to enable/disable examples and tests in |ns3|
*************************************************

There are 3 ways to enable/disable examples and tests in |ns3|:

#. Using build.py when |ns3| is built for the first time
#. Using ns3 once |ns3| has been built
#. Using the |ns3| configuration file once |ns3| has been built

Enable/disable examples and tests using build.py
++++++++++++++++++++++++++++++++++++++++++++++++

You can use build.py to enable/disable examples and tests when |ns3| is built for the first time.

By default, examples and tests are not built in |ns3|.

From the ns-3-allinone directory, you can build |ns3| without any
examples or tests simply by doing: ::

  $ ./build.py

Running test.py in the top level |ns3| directory now will cause no examples or tests to be run:

.. sourcecode:: text

  0 of 0 tests passed (0 passed, 0 skipped, 0 failed, 0 crashed, 0 valgrind errors)

If you would like build |ns3| with examples and tests, then do the following from the ns-3-allinone directory: ::

  $ ./build.py --enable-examples --enable-tests

Running test.py in the top level |ns3| directory will cause all of the examples and tests to be run:

.. sourcecode:: text

  170 of 170 tests passed (170 passed, 0 skipped, 0 failed, 0 crashed, 0 valgrind errors)

Enable/disable examples and tests using ns3
+++++++++++++++++++++++++++++++++++++++++++

You can use ns3 to enable/disable examples and tests once |ns3| has been built.

By default, examples and tests are not built in |ns3|.

From the top level |ns3| directory, you can build |ns3| without any
examples or tests simply by doing: ::

  $ ./ns3 configure
  $ ./ns3 build

Running test.py now will cause no examples or tests to be run:

.. sourcecode:: text

  0 of 0 tests passed (0 passed, 0 skipped, 0 failed, 0 crashed, 0 valgrind errors)

If you would like build |ns3| with examples and tests, then do the following from the top level |ns3| directory: ::

  $ ./ns3 configure --enable-examples --enable-tests
  $ ./ns3 build

Running test.py will cause all of the examples and tests to be run:

.. sourcecode:: text

  170 of 170 tests passed (170 passed, 0 skipped, 0 failed, 0 crashed, 0 valgrind errors)

Enable/disable examples and tests using the |ns3| configuration file
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

A configuration file, .ns3rc, has been added to |ns3| that allows users to specify whether examples and tests should be built or not.  You can use this file to enable/disable examples and tests once |ns3| has been built.

When enabling disabling examples and tests, the precedence rules are as follows:

#. the --enable-examples/--disable-examples configure strings override any .ns3rc file
#. the --enable-tests/--disable-tests configure strings override any .ns3rc file
#. the .ns3rc file in the top level |ns3| directory is next consulted, if present
#. the system searches for ~/.ns3rc if the .ns3rc file was not found in the previous step

If none of the above exists, then examples and tests will not be built.

The maintained version of the .ns3rc file in the |ns3| source code repository resides in the ``utils`` directory.  The reason for this is if it were in the top-level directory of the repository, it would be prone to accidental checkins from maintainers that enable the modules they want to use.  Therefore, users need to manually copy the .ns3rc from the ``utils`` directory to their preferred place (top level directory or their home directory) to enable persistent enabling of examples and tests.

Assuming that you are in the top level |ns3| directory, you can get a copy of the .ns3rc file that is in the ``utils`` directory as follows: ::

  $ cp utils/.ns3rc .

The .ns3rc file should now be in your top level |ns3| directory, and it contains the following:

.. sourcecode:: cmake

    # A list of the modules that will be enabled when ns-3 is run.
    # Modules that depend on the listed modules will be enabled also.
    #
    # All modules can be enabled by emptying the list.
    set(ns3rc_enabled_modules)

    # A list of the modules that will be disabled when ns-3 is run.
    # Modules that depend on the listed modules will be disabled also.
    #
    # If the list is empty, no module will be disabled.
    set(ns3rc_disabled_modules)

    # Set this equal to ON if you want examples to be run.
    set(ns3rc_examples_enabled OFF)

    # Set this equal to ON if you want tests to be run.
    set(ns3rc_tests_enabled OFF)

    # Override other ns-3 settings by setting their values below
    # Note: command-line settings will also be overridden.
    #set(NS3_LOG ON)

From the top level |ns3| directory, you can build |ns3| without any
examples or tests simply by doing: ::

  $ ./ns3 configure
  $ ./ns3 build

Running test.py now will cause no examples or tests to be run:

.. sourcecode:: text

  0 of 0 tests passed (0 passed, 0 skipped, 0 failed, 0 crashed, 0 valgrind errors)

If you would like build |ns3| with examples and tests, use your
favorite editor to change the values in the .ns3rc file for
ns3rc_examples_enabled and ns3rc_tests_enabled file to be True:

.. sourcecode:: cmake

    # A list of the modules that will be enabled when ns-3 is run.
    # Modules that depend on the listed modules will be enabled also.
    #
    # All modules can be enabled by emptying the list.
    set(ns3rc_enabled_modules)

    # A list of the modules that will be disabled when ns-3 is run.
    # Modules that depend on the listed modules will be disabled also.
    #
    # If the list is empty, no module will be disabled.
    set(ns3rc_disabled_modules)

    # Set this equal to ON if you want examples to be run.
    set(ns3rc_examples_enabled ON)

    # Set this equal to ON if you want tests to be run.
    set(ns3rc_tests_enabled ON)

    # Override other ns-3 settings by setting their values below
    # Note: command-line settings will also be overridden.
    #set(NS3_LOG ON)

From the top level |ns3| directory, you can build |ns3| with examples
and tests simply by doing: ::

  $ ./ns3 configure
  $ ./ns3 build

Running test.py will cause all of the examples and tests to be run:

.. sourcecode:: text

  170 of 170 tests passed (170 passed, 0 skipped, 0 failed, 0 crashed, 0 valgrind errors)


Enable examples and tests that depend on a set of modules using |ns3|
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

As seen above, the following command only builds the requested modules (core and wifi),
plus any modules that are implicitly needed (e.g., network), and the resulting compatible examples and tests:

.. sourcecode:: terminal

  ./ns3 configure --enable-modules="wifi;core" --enable-examples --enable-tests

However, when developing a new module, you may prefer to use the following alternative, which builds
all module libraries, but will filter out any examples and tests from modules that are not explicitly listed.

.. sourcecode:: terminal

  ./ns3 configure --filter-module-examples-and-tests="wifi;core" --enable-examples --enable-tests

The first command will generally lead to a shorter build time, but the second option will provide better
coverage, by building additional test cases and examples directly related to the specified modules.

Enable examples and tests that depend on a set of modules using the |ns3| configuration file
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

As seen above, examples and tests can be enabled for just a subset of the available modules via the
``ns3`` script. The same can be accomplished via the ``.ns3rc`` configuration file.

.. sourcecode:: cmake

    # A list of the modules that will be enabled when ns-3 is run.
    # Modules that depend on the listed modules will be enabled also.
    #
    # All modules can be enabled by emptying the list.
    set(ns3rc_enabled_modules)

    # ...

    # Override other ns-3 settings by setting their values below
    # Note: command-line settings will also be overridden.
    #set(NS3_LOG ON)

    # The following will build core and wifi tests, plus their examples
    # and examples on /ns-3-dev/examples that depend on either of them
    set(NS3_FILTER_MODULE_EXAMPLES_AND_TESTS wifi core) # <==
