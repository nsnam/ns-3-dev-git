.. include:: replace.txt
.. highlight:: console

.. Section Separators:
   ----
   ****
   ++++
   ====
   ~~~~

.. _Working with CMake:

Working with CMake
------------------

The |ns3| project used Waf build system in the past, but it has moved to
CMake for the ns-3.36 release.

CMake is very verbose and commands can be very long for basic operations.

The wrapper script ``ns3`` hides most of verbosity from CMake and provide a
Waf-like interface for command-line users.

.. _CLion : https://www.jetbrains.com/clion/
.. _Visual Studio : https://visualstudio.microsoft.com/
.. _Code : https://code.visualstudio.com/Download
.. _Xcode : https://developer.apple.com/xcode/
.. _CodeBlocks : https://www.codeblocks.org/
.. _Eclipse : https://www.eclipse.org/cdt/
.. _CMake generated : https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html
.. _generator options : https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html

It is the recommended way to work on |ns3|, except if you are using an
IDE that supports projects that can be generated with CMake or CMake projects.

Here is a non-exhaustive list of IDEs that can be used:

* Support CMake projects:

  * JetBrains's `CLion`_
  * Microsoft `Visual Studio`_ and Visual Studio `Code`_

* Supported IDEs via `CMake generated`_ projects:

  * Apple's `XCode`_ : ``ns3 configure -G Xcode``
  * `CodeBlocks`_ : ``ns3 configure -G "CodeBlocks - Ninja"``
  * `Eclipse`_ CDT4 : ``ns3 configure -G "Eclipse CDT4 - Ninja"``

Note: Ninja was used for brevity.
Both CodeBlocks and Eclipse have additional `generator options`_.

General instructions on how to setup and use IDEs are available
in the Tutorial and will not be detailed here.

Configuring the project
***********************

After getting the code, either cloning the ns-3-dev repository or
downloading the release tarball, you will need to configure the
project to work on it.

There are two ways to configure the project: the easiest way
is using the ``ns3`` script and the other way directly with CMake.

Configuring the project with ``ns3``
++++++++++++++++++++++++++++++++++++

Navigate to the ns-3-dev directory, then run ``./ns3 configure --help`` to
print the configuration options:

.. sourcecode:: console

  ~$ cd ns-3-dev
  ~/ns-3-dev$ ./ns3 configure --help
  usage: ns3 configure [-h] [-d {debug,release,optimized}] [-G G]
                       [--cxx-standard CXX_STANDARD] [--enable-asserts]
                       [--disable-asserts] [--enable-examples]
                       [--disable-examples] [--enable-logs]
                       [--disable-logs] [--enable-tests]
                       [--disable-tests] [--enable-verbose]
                       [--disable-verbose]
                       ...

  positional arguments:
    configure

  optional arguments:
    -h, --help            show this help message and exit
    -d {debug,release,optimized}, --build-profile {debug,release,optimized}
                          Build profile
    -G G                  CMake generator (e.g.
                          https://cmake.org/cmake/help/latest/manual/cmake-
                          generators.7.html)
    ...

Note: the command output was trimmed to the most used options.

To configure |ns3| in release mode, while enabling examples and tests,
run ``./ns3 configure -d release --enable-examples --enable-tests``.
To check what underlying commands dare being executed, add the
``--dry-run`` option:

.. sourcecode:: console

  ~/ns-3-dev$ ./ns3 --dry-run configure -d release --enable-examples --enable-tests
  The following commands would be executed:
  mkdir cmake-cache
  cd cmake-cache; /usr/bin/cmake -DCMAKE_BUILD_TYPE=release -DNS3_NATIVE_OPTIMIZATIONS=OFF -DNS3_EXAMPLES=ON -DNS3_TESTS=ON -G Unix Makefiles .. ; cd ..

Now we run it for real:

.. sourcecode:: console

  ~/ns-3-dev$ ./ns3 configure -d release --enable-examples --enable-tests
  -- CCache is enabled. Precompiled headers are disabled by default.
  -- The CXX compiler identification is GNU 11.2.0
  -- The C compiler identification is GNU 11.2.0
  -- Detecting CXX compiler ABI info
  -- Detecting CXX compiler ABI info - done
  -- Check for working CXX compiler: /usr/bin/c++ - skipped
  -- Detecting CXX compile features
  -- Detecting CXX compile features - done
  ...
  -- Processing src/wifi
  -- Processing src/wimax
  -- ---- Summary of optional ns-3 features:
  Build profile                 : release
  Build directory               : /ns-3-dev/build
  ...
  Examples                      : ON
  ...
  Tests                         : ON
  Threading Primitives          : ON


  Modules configured to be built:
  antenna                   aodv                      applications
  bridge                    buildings                 config-store
  core                      csma                      csma-layout
  ...
  wifi                      wimax

  Modules that cannot be built:
  brite                     click                     openflow
  visualizer


  -- Configuring done
  -- Generating done
  -- Build files have been written to: /ns-3-dev/cmake-cache
  Finished executing the following commands:
  mkdir cmake-cache
  cd cmake-cache; /usr/bin/cmake -DCMAKE_BUILD_TYPE=release -DNS3_NATIVE_OPTIMIZATIONS=OFF -DNS3_EXAMPLES=ON -DNS3_TESTS=ON -G Unix Makefiles .. ; cd ..

Notice that CCache is automatically used (if installed) for your convenience.

The summary with enabled feature shows both the ``release`` build type, along with
enabled examples and tests.

Below is a list of enabled modules and modules that cannot be built.

At the end, notice we print the same commands from ``--dry-run``. This is done
to familiarize Waf users with CMake and how the options names changed.

The mapping of the ``ns3`` build profiles into the CMake build types is the following:

+--------------------------------------------------------------------------------------------------------------------+
| Equivalent build profiles                                                                                          |
+-------------------------+--------------------------------------------------------+---------------------------------+
| ``ns3 --build-profile`` | CMake                                                  | Equivalent GCC compiler flags   |
|                         +------------------------+-------------------------------+---------------------------------+
|                         | CMAKE_BUILD_TYPE       | Additional flags              |                                 |
+=========================+========================+===============================+=================================+
| debug                   | debug                  |                               | -g                              |
+-------------------------+------------------------+-------------------------------+---------------------------------+
| default                 | default                |                               | -Os -g                          |
+-------------------------+------------------------+-------------------------------+---------------------------------+
| release                 | release                |                               | -O3                             |
+-------------------------+------------------------+-------------------------------+---------------------------------+
| optimized               | release                | -DNS3_NATIVE_OPTIMIZATIONS=ON | -O3 -march=native -mtune=native |
+-------------------------+------------------------+-------------------------------+---------------------------------+
| minsizerel              | minsizerel             |                               | -Os                             |
+-------------------------+------------------------+-------------------------------+---------------------------------+

In addition to setting compiler flags each build type also controls whether certain features are enabled or not:

+-------------------------+-----------------+-------------+----------------------------+
| ``ns3 --build-profile`` |  ``NS3_ASSERT`` | ``NS3_LOG`` | ``NS3_WARNINGS_AS_ERRORS`` |
+=========================+=================+=============+============================+
| debug                   |   ON            |   ON        |   ON                       |
+-------------------------+-----------------+-------------+----------------------------+
| default                 |   ON            |   ON        |   OFF                      |
+-------------------------+-----------------+-------------+----------------------------+
| release                 |   OFF           |   OFF       |   OFF                      |
+-------------------------+-----------------+-------------+----------------------------+
| optimized               |   OFF           |   OFF       |   OFF                      |
+-------------------------+-----------------+-------------+----------------------------+
| minsizerel              |   OFF           |   OFF       |   OFF                      |
+-------------------------+-----------------+-------------+----------------------------+

``NS3_ASSERT`` and ``NS_LOG`` control whether the assert or logging macros
are functional or compiled out.
``NS3_WARNINGS_AS_ERRORS`` controls whether compiler warnings are treated
as errors and stop the build, or whether they are only warnings and
allow the build to continue.


Configuring the project with CMake
++++++++++++++++++++++++++++++++++

.. _CMake: https://cmake.org/cmake/help/latest/manual/cmake.1.html
.. _CMAKE_BUILD_TYPE: https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html
.. _Effects (g++): https://github.com/Kitware/CMake/blob/master/Modules/Compiler/GNU.cmake
.. _generator: https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html

Navigate to the ns-3-dev directory, create a CMake cache folder,
navigate to it and run `CMake`_ pointing to the ns-3-dev folder.

.. sourcecode:: console

  ~$ cd ns-3-dev
  ~/ns-3-dev$ mkdir cmake-cache
  ~/ns-3-dev$ cd cmake-cache
  ~/ns-3-dev/cmake-cache$ cmake ..

You can pass additional arguments to the CMake command, to configure it. To change variable values,
you should use the -D option followed by the variable name.

As an example, the build type is stored in the variable named `CMAKE_BUILD_TYPE`_. Setting it to one
of the CMake build types shown in the table below will change compiler settings associated with those
build types and output executable and libraries names, which will receive a suffix.

+------------------+-------------------+
| CMAKE_BUILD_TYPE | `Effects (g++)`_  |
+==================+===================+
| DEBUG            | -g                |
+------------------+-------------------+
| DEFAULT          | -Os -g -DNDEBUG   |
+------------------+-------------------+
| RELWITHDEBINFO   | -Os -g -DNDEBUG   |
+------------------+-------------------+
| RELEASE          | -O3 -DNDEBUG      |
+------------------+-------------------+
| MINSIZEREL       | -Os -DNDEBUG      |
+------------------+-------------------+

You can set the build type with the following command, which assumes your terminal is inside the cache folder
created previously.

.. sourcecode:: console

  ~/ns-3-dev/cmake-cache$ cmake -DCMAKE_BUILD_TYPE=DEBUG ..

Another common option to change is the `generator`_, which is the real underlying build system called by CMake.
There are many generators supported by CMake, including the ones listed in the table below.

+------------------------------------------------+
| Generators                                     |
+================================================+
| MinGW Makefiles                                |
+------------------------------------------------+
| Unix Makefiles                                 |
+------------------------------------------------+
| MSYS Makefiles                                 |
+------------------------------------------------+
| CodeBlocks - *one of the previous Makefiles*   |
+------------------------------------------------+
| Eclipse CDT4 - *one of the previous Makefiles* |
+------------------------------------------------+
| Ninja                                          |
+------------------------------------------------+
| Xcode                                          |
+------------------------------------------------+

To change the generator, you will need to pass one of these generators with the -G option. For example, if we
prefer Ninja to Makefiles, which are the default, we need to run the following command:

.. sourcecode:: console

  ~/ns-3-dev/cmake-cache$ cmake -G Ninja ..

This command may fail if there are different generator files in the same CMake cache folder. It is recommended to clean up
the CMake cache folder, then recreate it and reconfigure setting the generator in the first run.

.. sourcecode:: console

  ~/ns-3-dev/cmake-cache$ cd ..
  ~/ns-3-dev$ rm -R cmake-cache && mkdir cmake-cache && cd cmake-cache
  ~/ns-3-dev/cmake-cache$ cmake -DCMAKE_BUILD_TYPE=release -G Ninja ..

After configuring for the first time, settings will be initialized to their
default values, and then you can use the ``ccmake`` command to manually change them:

.. sourcecode:: console

  ~/ns-3-dev/cmake-cache$ ccmake .
  CMAKE_BUILD_TYPE                 release
  CMAKE_INSTALL_PREFIX             /usr/local
  NS3_ASSERT                       OFF
  ...
  NS3_EXAMPLES                     ON
  ...
  NS3_LOG                          OFF
  NS3_TESTS                        ON
  NS3_VERBOSE                      OFF
  ...

  CMAKE_BUILD_TYPE: Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel ...
  Keys: [enter] Edit an entry [d] Delete an entry                                                                                             CMake Version 3.22.1
        [l] Show log output   [c] Configure
        [h] Help              [q] Quit without generating
        [t] Toggle advanced mode (currently off)

After moving the cursor and setting the desired values, type ``c`` to configure CMake.

If you prefer doing everything with a non-interactive command, look at the main ``CMakeLists.txt``
file in the ns-3-dev directory. It contains most of the option flags and their default values.
To enable both examples and tests, run:

.. sourcecode:: console

  ~/ns-3-dev/cmake-cache$ cmake -DNS3_EXAMPLES=ON -DNS3_TESTS=ON ..


.. _Manually refresh the CMake cache:

Manually refresh the CMake cache
********************************

After the project has been configured, calling ``CMake`` will
:ref:`refresh the CMake cache<Manually refresh the CMake cache>`.
The refresh is required to discover new targets: libraries, executables and/or modules
that were created since the last run.

The refresh is done by running the CMake command from the CMake cache folder.

.. sourcecode:: console

  ~/ns-3-dev/cmake-cache$ cmake ..

Previous settings stored in the CMakeCache.txt will be preserved, while new modules will be
scanned and targets will be added.

The cache can also be refreshed with the ``ns3`` wrapper script:

.. sourcecode:: console

  ~/ns-3-dev$ ./ns3 configure


Building the project
********************

There are three ways of building the project:
using the ``ns3`` script, calling ``CMake`` and
calling the underlying build system (e.g. Ninja) directly.
The last way is omitted, since each underlying build system
has its own unique command-line syntax.

Building the project with ``ns3``
+++++++++++++++++++++++++++++++++

The ``ns3`` wrapper script makes life easier for command line users, accepting module names without
the ``lib`` prefix and scratch files without the ``scratch_`` prefix. The following command can
be used to build the entire project:

.. sourcecode:: console

  ~/ns-3-dev$ ./ns3 build


To build specific targets, run:

.. sourcecode:: console

  ~/ns-3-dev$ ./ns3 build target_name


Building the project with CMake
+++++++++++++++++++++++++++++++

The build process of targets (either libraries, executables or custom tasks) can be done
invoking CMake build. To build all the targets, run:

.. sourcecode:: console

  ~/ns-3-dev/cmake-cache$ cmake --build .

Notice the single dot now refers to the ``cmake-cache`` directory, where the underlying
build system files are stored (referred inside CMake as ``PROJECT_BINARY_DIR`` or
``CMAKE_BINARY_DIR``, which have slightly different uses if working with sub-projects).

.. _PROJECT_BINARY_DIR: https://cmake.org/cmake/help/latest/variable/PROJECT_BINARY_DIR.html
.. _CMAKE_BINARY_DIR: https://cmake.org/cmake/help/latest/variable/CMAKE_BINARY_DIR.html

To build specific targets, run:

.. sourcecode:: console

  ~/ns-3-dev/cmake-cache$ cmake --build . --target target_name

Where target_name is a valid target name.
Since ns-3.43, module libraries CMake targets are named the same as the module name (e.g. core, wifi, lte).
From ns-3.36 to 3.42, module library targets were prefixed with ``lib`` (e.g. libcore, libwifi, liblte).
Executables from the scratch folder are prefixed with ``scratch_`` (e.g. scratch_scratch-simulator).
Executables targets are named the same as their source file containing the `main` function,
without the ".cc" prefix (e.g. sample-simulator.cc => sample-simulator).


Adding a new module
*******************

Adding a module is the only case where
:ref:`manually refreshing the CMake cache<Manually refresh the CMake cache>` is required.

More information on how to create a new module are provided in :ref:`Adding a New Module to ns3`.

Note: Advanced users who wish to organize their custom contrib modules outside the
`ns-3-dev/contrib` directory can take advantage of a feature introduced in ns-3.44.
The build system now also scans for contrib modules in a dedicated ns-3-external-contrib folder.
This approach simplifies managing a top-level project that handles multiple repositories
without requiring explicit dependencies between them.

You should have a source tree like the following:

.. sourcecode:: console

   $ tree -d -L 3
   .
   ├── ns-3-dev
   │   ├── LICENSES
   │   ├── bindings
   │   │   └── python
   │   ├── build-support
   │   │   ├── 3rd-party
   │   │   ├── custom-modules
   │   │   ├── pip-wheel
   │   │   └── test-files
   │   ├── contrib
   │   ├── doc
   │   │   ├── contributing
   │   │   ├── ...
   │   │   └── tutorial
   │   ├── examples
   │   │   ├── channel-models
   │   │   ├── ...
   │   │   └── wireless
   │   ├── scratch
   │   │   ├── nested-subdir
   │   │   ├── ...
   │   │   └── subdir2
   │   ├── src
   │   │   ├── antenna
   │   │   ├── ...
   │   │   ├── wifi
   │   │   └── wimax
   │   ├── third-party
   │   └── utils
   │       ├── perf
   │       └── tests
   └── ns-3-external-contrib
       └── nr
           ├── LICENSES
           ├── doc
           ├── examples
           ├── helper
           ├── model
           ├── test
           ├── tools
           ├── tutorial
           └── utils

The module will be automatically mapped to `ns-3-dev/contrib`, as if it was part
of the typical contrib module location. No copying or symlink required.

Note: For that to always work, you may need to adjust paths dependent on
CMAKE_CURRENT_SOURCE_DIR, if using custom CMake constructs instead of the
ns-3 macros.

Migrating a Waf module to CMake
*******************************

If your module does not have external dependencies, porting is very easy.
Start by copying the module Wscript, rename them to CMakeLists.txt and then open it.

We are going to use the aodv module as an example:

.. sourcecode:: python3

  def build(bld):
    module = bld.create_ns3_module('aodv', ['internet', 'wifi'])
    module.includes = '.'
    module.source = [
        'model/aodv-id-cache.cc',
        'model/aodv-dpd.cc',
        'model/aodv-rtable.cc',
        'model/aodv-rqueue.cc',
        'model/aodv-packet.cc',
        'model/aodv-neighbor.cc',
        'model/aodv-routing-protocol.cc',
        'helper/aodv-helper.cc',
        ]

    aodv_test = bld.create_ns3_module_test_library('aodv')
    aodv_test.source = [
        'test/aodv-id-cache-test-suite.cc',
        'test/aodv-test-suite.cc',
        'test/aodv-regression.cc',
        'test/bug-772.cc',
        'test/loopback.cc',
        ]

    # Tests encapsulating example programs should be listed here
    if (bld.env['ENABLE_EXAMPLES']):
        aodv_test.source.extend([
        #   'test/aodv-examples-test-suite.cc',
            ])

    headers = bld(features='ns3header')
    headers.module = 'aodv'
    headers.source = [
        'model/aodv-id-cache.h',
        'model/aodv-dpd.h',
        'model/aodv-rtable.h',
        'model/aodv-rqueue.h',
        'model/aodv-packet.h',
        'model/aodv-neighbor.h',
        'model/aodv-routing-protocol.h',
        'helper/aodv-helper.h',
        ]

    if bld.env['ENABLE_EXAMPLES']:
        bld.recurse('examples')

    bld.ns3_python_bindings()


We can see the module name is ``aodv`` and it depends on the ``internet`` and the ``wifi`` libraries,
plus the lists of files (``module.source``, ``headers.source`` and ``module_test.source``).

This translates to the following CMake lines:

.. sourcecode:: cmake

  build_lib(
    LIBNAME aodv # aodv module, which can later be linked to examples and modules with ${libaodv}
    SOURCE_FILES # equivalent to module.source
      helper/aodv-helper.cc
      model/aodv-dpd.cc
      model/aodv-id-cache.cc
      model/aodv-neighbor.cc
      model/aodv-packet.cc
      model/aodv-routing-protocol.cc
      model/aodv-rqueue.cc
      model/aodv-rtable.cc
    HEADER_FILES # equivalent to headers.source
      helper/aodv-helper.h
      model/aodv-dpd.h
      model/aodv-id-cache.h
      model/aodv-neighbor.h
      model/aodv-packet.h
      model/aodv-routing-protocol.h
      model/aodv-rqueue.h
      model/aodv-rtable.h
    LIBRARIES_TO_LINK ${libinternet} # depends on internet and wifi,
                      ${libwifi}     # but both are prefixed with lib in CMake
    TEST_SOURCES # equivalent to module_test.source
      test/aodv-id-cache-test-suite.cc
      test/aodv-regression.cc
      test/aodv-test-suite.cc
      test/loopback.cc
      test/bug-772.cc
  )

If your module depends on external libraries, check the section
`Linking third-party libraries`_.

Python bindings are generated at runtime for all built modules
if NS3_PYTHON_BINDINGS is enabled.

Next, we need to port the examples wscript. Repeat the copy, rename and open
steps. We should have something like the following:

.. sourcecode:: python3

  def build(bld):
      obj = bld.create_ns3_program('aodv',
                                   ['wifi', 'internet', 'aodv', 'internet-apps'])
      obj.source = 'aodv.cc'

This means we create an example named ``aodv`` which depends on ``wifi``, ``internet``,
``aodv`` and ``internet-apps`` module, and has a single source file ``aodv.cc``.
This translates into the following CMake:

.. sourcecode:: cmake

  build_lib_example(
    NAME aodv # example named aodv
    SOURCE_FILES aodv.cc # single source file aodv.cc
    LIBRARIES_TO_LINK # depends on wifi, internet, aodv and internet-apps
      ${libwifi}
      ${libinternet}
      ${libaodv}
      ${libinternet-apps}
  )

Migrating definitions, compilation and linking options
++++++++++++++++++++++++++++++++++++++++++++++++++++++

If your Waf modules had additional definitions, compilation or linking flags,
you also need to translate them to CMake. The easiest way to accomplish that
is using the CMake counterparts *BEFORE* defining your target.

If you, for example, had the following:

.. sourcecode:: python

   conf.env.append_value("CXXFLAGS", ["-fopenmp", "-I/usr/local/include/e2sim"])
   conf.env.append_value("CXXDEFINES", ["LAPACK", "LUSOLVER=LAPACK"])
   conf.env.append_value("LINKFLAGS", ["-llapack", "-L/usr/local/src/GoToBLAS2", "-lblas", "-Lsrc/common", "-lthyme"])
   conf.env.append_value("LIB", ["e2sim"])

You would need to replace it with the following counterparts:

.. sourcecode:: cmake

   # The settings below will impact all future target declarations
   # in the current subdirectory and its subdirectories
   #
   # a.k.a. the module, its examples and tests will have the definitions,
   # compilation options and will be linked to the specified libraries
   add_compile_options(-fopenmp) # CXXFLAGS counterpart
   include_directories(/usr/local/include/e2sim) # CXXFLAGS -I counterpart
   add_definitions(-DLAPACK -DLUSOLVER=LAPACK) # CXXDEFINES counterpart
   link_directories(/usr/local/src/GoToBLAS2 src/common) # LINKFLAGS -L counterpart
   link_libraries(lapack blas thyme e2sim) # LINKFLAGS -l or LIB counterpart

   # Target definition after changing settings
   build_lib_example(
       NAME hypothetical-module
       SOURCE_FILES hypothetical-module-source.cc
       LIBRARIES_TO_LINK
         # depends on wifi, internet, aodv and internet-apps modules
         ${libwifi}
         ${libinternet}
         ${libaodv}
         ${libinternet-apps}
         # and lapack, blas, thyme, e2sim external libraries
     )

Running programs
****************

Running programs with the ``ns3`` wrapper script is pretty simple. To run the
scratch program produced by ``scratch/scratch-simulator.cc``, you need the following:

.. sourcecode:: console

  ~/ns-3-dev$ ./ns3 run scratch-simulator --no-build

Notice the ``--no-build`` indicates that the program should only be executed, and not built
before execution.

To familiarize users with CMake, ``ns3`` can also print the underlying CMake
and command line commands used by adding the ``--dry-run`` flag.
Removing the ``--no-build`` flag and adding ``--dry-run`` to the same example,
produces the following:

.. sourcecode:: console

  ~/ns-3-dev$ ./ns3 --dry-run run scratch-simulator
  The following commands would be executed:
  cd cmake-cache; cmake --build . -j 15 --target scratch_scratch-simulator ; cd ..
  export PATH=$PATH:~/ns-3-dev/build/lib
  export PYTHONPATH=~/ns-3-dev/build/bindings/python
  export LD_LIBRARY_PATH=~/ns-3-dev/build/lib
  ./build/scratch/ns3-dev-scratch-simulator


In the CMake build command line, notice the scratch-simulator has a ``scratch_`` prefix.
That is true for all the CMake scratch targets. This is done to guarantee globally unique names.

.. _RPATH: https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_RPATH.html

The next few lines exporting variables guarantee the executable can find python dependencies
(``PYTHONPATH``) and linked libraries (``LD_LIBRARY_PATH`` and ``PATH`` on Unix-like, and
``PATH`` on Windows). This is not necessary in platforms that support `RPATH`_.

Notice that when the scratch-simulator program is called on the last line, it has
a ``ns3-<version>`` prefix and could also have a build type suffix.
This is valid for all libraries and executables, but omitted in ``ns3`` for simplicity.

Debugging can be done with GDB. Again, we have the two ways to run the program.
Using the ``ns3`` wrapper:

.. sourcecode:: console

  ~/ns-3-dev$ ./ns3 run scratch-simulator --no-build --gdb

Or directly:

.. sourcecode:: console

  ~/ns-3-dev/cmake-cache$ export PATH=$PATH:~/ns-3-dev/build/lib
  ~/ns-3-dev/cmake-cache$ export PYTHONPATH=~/ns-3-dev/build/bindings/python
  ~/ns-3-dev/cmake-cache$ export LD_LIBRARY_PATH=~/ns-3-dev/build/lib
  ~/ns-3-dev/cmake-cache$ gdb ../build/scratch/ns3-dev-scratch-simulator


Modifying files
***************

As CMake is not a build system on itself, but a meta build system, it requires
frequent refreshes, also known as reconfigurations. Those refreshes are triggered
automatically in the following cases:

* Changes in linked libraries
* Changes in the CMake code
* Header changes
* Header/source file name changes
* Module name changes

The following sections will detail some of these cases assuming a hypothetical module defined below.
Notice that the build_lib is the fundamental piece of every |ns3| module, while user-settable
options and external libraries checking are optional.

.. sourcecode:: cmake

    build_lib(
      LIBNAME hypothetical
      SOURCE_FILES  helper/hypothetical-helper.cc
                    model/hypothetical.cc
      HEADER_FILES
        helper/hypothetical-helper.h
        model/hypothetical.h
        model/colliding-header.h
      LIBRARIES_TO_LINK ${libcore}
      )


Module name changes
+++++++++++++++++++

Changing the module name requires changing the value of ``LIBNAME``.
In the following example the name of the module seen previously is
changed from ``hypothetical`` to ``new-hypothetical-name``:

.. sourcecode:: cmake

    build_lib(
      LIBNAME new-hypothetical-name
      # ...
    )

If the module was already scanned, saving the changes and trying to build will trigger the
automatic CMake refresh. Otherwise, reconfigure the project to
:ref:`manually refresh it<Manually refresh the CMake cache>`.


Header/source file name changes
+++++++++++++++++++++++++++++++

Assuming the hypothetical module defined previously has a header name that collides
with a header of a different module.

The name of the ``colliding-header.h`` can be changed via the filesystem to
``non-colliding-header.h``, and the ``CMakeLists.txt`` path needs to be updated to match
the new name. Some IDEs can do this automatically through refactoring tools.

.. sourcecode:: cmake

    build_lib(
      LIBNAME new-hypothetical-name
      # ...
      HEADER_FILES
          helper/hypothetical-helper.h
          model/hypothetical.h
          model/non-colliding-header.h
      # ...
    )


Linking |ns3| modules
+++++++++++++++++++++

Adding a dependency to another |ns3| module just requires adding ``${lib${modulename}}``
to the ``LIBRARIES_TO_LINK`` list, where modulename contains the value of the |ns3|
module which will be depended upon.

Note: All |ns3| module libraries are prefixed with ``lib``,
as CMake requires unique global target names.

.. sourcecode:: cmake

    # now ${libnew-hypothetical-name} will depend on both core and internet modules
    build_lib(
      LIBNAME new-hypothetical-name
      # ...
      LIBRARIES_TO_LINK ${libcore}
                        ${libinternet}
      # ...
    )


.. _Linking third-party libraries:

Linking third-party libraries
+++++++++++++++++++++++++++++

Depending on a third-party library is a bit more complicated as we have multiple
ways to handle that within CMake.

Here is a short version on how to find and use third-party libraries
that should work in most cases:

.. sourcecode:: cmake

    # DEPENDENCY_NAME is used as a prefix to variables set by the find_external_library macro
    # HEADER_NAME(S) is(are) the name(s) of the header(s) you want to include
    # LIBRARY_NAME(S) is(are) the name(s) of the library(ies) you want to link
    # SEARCH_PATHS are the custom paths you can give if your library is not on a system path
    find_external_library(DEPENDENCY_NAME SQLite3
                          HEADER_NAME sqlite3.h
                          LIBRARY_NAME sqlite3
                          SEARCH_PATHS /optional/search/path/to/custom/sqlite3/library)

    # If the header(s) and library(ies) are not found, a message will be printed during the configuration
    # If the header(s) and the library(ies) are found, we can use the information found by the buildsystem
    if(${SQLite3_FOUND}) # Notice that the contents of DEPENDENCY_NAME became a prefix for the _FOUND variable
        # The compiler will not be able to find the include that is not on
        # a system include path, unless we explicitly inform it

        # This is the equivalent of -I/optional/search/path/to/custom/sqlite3/include
        # and AFFECTS ALL the targets in the CURRENT DIRECTORY and ITS SUBDIRECTORIES
        include_directories(${SQLite3_INCLUDE_DIRS})

        # The compiler should be able to locate the headers, but it still needs to be
        # informed of the libraries that should be linked

        # This is the equivalent of -l/optional/search/path/to/custom/sqlite3/library/libsqlite3.so
        # and AFFECTS ALL the targets in the CURRENT DIRECTORY and ITS SUBDIRECTORIES
        link_libraries(${SQLite3_LIBRARIES})
    endif()

If you do not want to link the library against all the targets
(executables and other libraries) use one of the following patterns.

**If the third-party library is required**

.. sourcecode:: cmake

    # if the third-party library is required
    if(${SQLite3_FOUND})
        # define your target
        build_lib(
            LIBNAME example
            LIBRARIES_TO_LINK ${SQLite3_LIBRARIES}
            ...
        )

        # The LIBRARIES_TO_LINK will be translated into CMake's
        # target_link_libraries(${libexample} PUBLIC ${SQLite3_LIBRARIES})
        # which is equivalent to -l${SQLite3_LIBRARIES}
    endif()

**If the third-party library is optional**

.. sourcecode:: cmake

    set(sqlite_libraries)
    if(${SQLite3_FOUND})
        set(sqlite_libraries ${SQLite3_LIBRARIES})
    endif()

    # And then define your target
    build_lib(
        LIBNAME example
        LIBRARIES_TO_LINK ${sqlite_libraries} # variable can be empty
        ...
    )

More details on how find_external_library works and the other
ways to import third-party libraries are presented next.

It is recommended to use a system package managers to install libraries,
but ns-3 also supports vcpkg and CPM. More information on how to
use them is available in `Using C++ library managers`_.


.. _Linking third-party libraries without CMake or PkgConfig support:

Linking third-party libraries without CMake or PkgConfig support
================================================================

When the third-party library you want to use do not export CMake files to use
``find_package`` or PkgConfig files to use ``pkg_check_modules``, we need to
search for the headers and libraries manually. To simplify this process,
we include the macro ``find_external_library`` that searches for libraries and
header include directories, exporting results similarly to ``find_package``.

Here is how it works:

.. sourcecode:: cmake

  function(find_external_library)
    # Parse arguments
    set(options QUIET)
    set(oneValueArgs DEPENDENCY_NAME HEADER_NAME LIBRARY_NAME)
    set(multiValueArgs HEADER_NAMES LIBRARY_NAMES PATH_SUFFIXES SEARCH_PATHS)
    cmake_parse_arguments(
      "FIND_LIB" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
    )

    # Set the external package/dependency name
    set(name ${FIND_LIB_DEPENDENCY_NAME})

    # We process individual and list of headers and libraries by transforming them
    # into lists
    set(library_names "${FIND_LIB_LIBRARY_NAME};${FIND_LIB_LIBRARY_NAMES}")
    set(header_names "${FIND_LIB_HEADER_NAME};${FIND_LIB_HEADER_NAMES}")

    # Just changing the parsed argument name back to something shorter
    set(search_paths ${FIND_LIB_SEARCH_PATHS})
    set(path_suffixes "${FIND_LIB_PATH_SUFFIXES}")

    set(not_found_libraries)
    set(library_dirs)
    set(libraries)
    # Paths and suffixes where libraries will be searched on
    set(library_search_paths
            ${search_paths}
            ${CMAKE_OUTPUT_DIRECTORY} # Search for libraries in ns-3-dev/build
            ${CMAKE_INSTALL_PREFIX} # Search for libraries in the install directory (e.g. /usr/)
            $ENV{LD_LIBRARY_PATH} # Search for libraries in LD_LIBRARY_PATH directories
            $ENV{PATH} # Search for libraries in PATH directories
            )
    set(suffixes /build /lib /build/lib / /bin ${path_suffixes})

    # For each of the library names in LIBRARY_NAMES or LIBRARY_NAME
    foreach(library ${library_names})
      # We mark this value is advanced not to pollute the configuration with
      # ccmake with the cache variables used internally
      mark_as_advanced(${name}_library_internal_${library})

      # We search for the library named ${library} and store the results in
      # ${name}_library_internal_${library}
      find_library(
        ${name}_library_internal_${library} ${library}
        HINTS ${library_search_paths}
        PATH_SUFFIXES ${suffixes}
      )
      # cmake-format: off
      # Note: the PATH_SUFFIXES above apply to *ALL* PATHS and HINTS Which
      # translates to CMake searching on standard library directories
      # CMAKE_SYSTEM_PREFIX_PATH, user-settable CMAKE_PREFIX_PATH or
      # CMAKE_LIBRARY_PATH and the directories listed above
      #
      # e.g.  from Ubuntu 22.04 CMAKE_SYSTEM_PREFIX_PATH =
      # /usr/local;/usr;/;/usr/local;/usr/X11R6;/usr/pkg;/opt
      #
      # Searched directories without suffixes
      #
      # ${CMAKE_SYSTEM_PREFIX_PATH}[0] = /usr/local/
      # ${CMAKE_SYSTEM_PREFIX_PATH}[1] = /usr
      # ${CMAKE_SYSTEM_PREFIX_PATH}[2] = /
      # ...
      # ${CMAKE_SYSTEM_PREFIX_PATH}[6] = /opt
      # ${LD_LIBRARY_PATH}[0]
      # ...
      # ${LD_LIBRARY_PATH}[m]
      # ...
      #
      # Searched directories with suffixes include all of the directories above
      # plus all suffixes
      # PATH_SUFFIXES /build /lib /build/lib / /bin # ${path_suffixes}
      #
      # /usr/local/build
      # /usr/local/lib
      # /usr/local/build/lib
      # /usr/local/bin
      # ...
      #
      # cmake-format: on
      # Or enable NS3_VERBOSE to print the searched paths

      # Print tested paths to the searched library and if it was found
      if(${NS3_VERBOSE})
        log_find_searched_paths(
                TARGET_TYPE Library
                TARGET_NAME ${library}
                SEARCH_RESULT ${name}_library_internal_${library}
                SEARCH_PATHS ${library_search_paths}
                SEARCH_SUFFIXES ${suffixes}
        )
      endif()

      # After searching the library, the internal variable should have either the
      # absolute path to the library or the name of the variable appended with
      # -NOTFOUND
      if("${${name}_library_internal_${library}}" STREQUAL
         "${name}_library_internal_${library}-NOTFOUND"
      )
        # We keep track of libraries that were not found
        list(APPEND not_found_libraries ${library})
      else()
        # We get the name of the parent directory of the library and append the
        # library to a list of found libraries
        get_filename_component(
          ${name}_library_dir_internal ${${name}_library_internal_${library}}
          DIRECTORY
        ) # e.g. lib/openflow.(so|dll|dylib|a) -> lib
        list(APPEND library_dirs ${${name}_library_dir_internal})
        list(APPEND libraries ${${name}_library_internal_${library}})
      endif()
    endforeach()

    # For each library that was found (e.g. /usr/lib/pthread.so), get their parent
    # directory (/usr/lib) and its parent (/usr)
    set(parent_dirs)
    foreach(libdir ${library_dirs})
      get_filename_component(parent_libdir ${libdir} DIRECTORY)
      get_filename_component(parent_parent_libdir ${parent_libdir} DIRECTORY)
      list(APPEND parent_dirs ${libdir} ${parent_libdir} ${parent_parent_libdir})
    endforeach()

    # If we already found a library somewhere, limit the search paths for the header
    if(parent_dirs)
      set(header_search_paths ${parent_dirs})
      set(header_skip_system_prefix NO_CMAKE_SYSTEM_PATH)
    else()
      set(header_search_paths
              ${search_paths}
              ${CMAKE_OUTPUT_DIRECTORY} # Search for headers in ns-3-dev/build
              ${CMAKE_INSTALL_PREFIX} # Search for headers in the install
              )
    endif()

    set(not_found_headers)
    set(include_dirs)
    foreach(header ${header_names})
      # The same way with libraries, we mark the internal variable as advanced not
      # to pollute ccmake configuration with variables used internally
      mark_as_advanced(${name}_header_internal_${header})
      set(suffixes
              /build
              /include
              /build/include
              /build/include/${name}
              /include/${name}
              /${name}
              /
              ${path_suffixes}
              )
      # cmake-format: off
      # Here we search for the header file named ${header} and store the result in
      # ${name}_header_internal_${header}
      #
      # The same way we did with libraries, here we search on
      # CMAKE_SYSTEM_PREFIX_PATH, along with user-settable ${search_paths}, the
      # parent directories from the libraries, CMAKE_OUTPUT_DIRECTORY and
      # CMAKE_INSTALL_PREFIX
      #
      # And again, for each of them, for every suffix listed /usr/local/build
      # /usr/local/include
      # /usr/local/build/include
      # /usr/local/build/include/${name}
      # /usr/local/include/${name}
      # ...
      #
      # cmake-format: on
      # Or enable NS3_VERBOSE to get the searched paths printed while configuring

      find_file(
        ${name}_header_internal_${header} ${header}
        HINTS ${header_search_paths} # directory (e.g. /usr/)
        ${header_skip_system_prefix}
        PATH_SUFFIXES ${suffixes}
      )

      # Print tested paths to the searched header and if it was found
      if(${NS3_VERBOSE})
        log_find_searched_paths(
                TARGET_TYPE Header
                TARGET_NAME ${header}
                SEARCH_RESULT ${name}_header_internal_${header}
                SEARCH_PATHS ${header_search_paths}
                SEARCH_SUFFIXES ${suffixes}
                SEARCH_SYSTEM_PREFIX ${header_skip_system_prefix}
        )
      endif()

      # If the header file was not found, append to the not-found list
      if("${${name}_header_internal_${header}}" STREQUAL
         "${name}_header_internal_${header}-NOTFOUND"
      )
        list(APPEND not_found_headers ${header})
      else()
        # If the header file was found, get their directories and the parent of
        # their directories to add as include directories
        get_filename_component(
          header_include_dir ${${name}_header_internal_${header}} DIRECTORY
        ) # e.g. include/click/ (simclick.h) -> #include <simclick.h> should work
        get_filename_component(
          header_include_dir2 ${header_include_dir} DIRECTORY
        ) # e.g. include/(click) -> #include <click/simclick.h> should work
        list(APPEND include_dirs ${header_include_dir} ${header_include_dir2})
      endif()
    endforeach()

    # Remove duplicate include directories
    if(include_dirs)
      list(REMOVE_DUPLICATES include_dirs)
    endif()

    # If we find both library and header, we export their values
    if((NOT not_found_libraries}) AND (NOT not_found_headers))
      set(${name}_INCLUDE_DIRS "${include_dirs}" PARENT_SCOPE)
      set(${name}_LIBRARIES "${libraries}" PARENT_SCOPE)
      set(${name}_HEADER ${${name}_header_internal} PARENT_SCOPE)
      set(${name}_FOUND TRUE PARENT_SCOPE)
      set(status_message "find_external_library: ${name} was found.")
    else()
      set(${name}_INCLUDE_DIRS PARENT_SCOPE)
      set(${name}_LIBRARIES PARENT_SCOPE)
      set(${name}_HEADER PARENT_SCOPE)
      set(${name}_FOUND FALSE PARENT_SCOPE)
      set(status_message
          "find_external_library: ${name} was not found. Missing headers: \"${not_found_headers}\" and missing libraries: \"${not_found_libraries}\"."
      )
    endif()

    if(NOT ${FIND_LIB_QUIET})
      message(STATUS "${status_message}")
    endif()
  endfunction()

Debugging why a header or a library cannot be found is fairly tricky.
For ``find_external_library`` users, enabling the ``NS3_VERBOSE`` switch
will enable the logging of search path directories for both headers and
libraries.

.. _CMAKE_FIND_DEBUG_MODE: https://cmake.org/cmake/help/latest/variable/CMAKE_FIND_DEBUG_MODE.html

Note: The logging provided by find_external_library is an alternative to
CMake's own ``CMAKE_FIND_DEBUG_MODE=true`` introduced in CMake 3.17,
which gets used by *ALL* ``find_file``, ``find_library``, ``find_header``,
``find_package`` and ``find_path`` calls throughout CMake and its modules.
If you are using a recent version of CMake, it is recommended to use
`CMAKE_FIND_DEBUG_MODE`_ instead.

A commented version of the Openflow module ``CMakeLists.txt`` has an
example of ``find_external_library`` usage.

.. sourcecode:: cmake

    # Export a user option to specify the path to a custom
    # openflow build directory.
    set(NS3_WITH_OPENFLOW
        ""
        CACHE PATH
              "Build with Openflow support"
    )
    # We use this variable later in the ns-3-dev scope, but
    # the value would be lost if we saved it to the
    # parent scope ns-3-dev/src or ns-3-dev/contrib.
    # We set it as an INTERNAL CACHE variable to make it globally available.
    set(NS3_OPENFLOW
        "OFF"
        CACHE INTERNAL
              "ON if Openflow is found"
    )

    # This is the macro that searches for headers and libraries.
    # The DEPENDENCY_NAME is the equivalent of the find_package package name.
    # Resulting variables will be prefixed with DEPENDENCY_NAME.
    # - openflow_FOUND will be set to True if both headers and libraries
    #     were found and False otherwise
    # - openflow_LIBRARIES will contain a list of absolute paths to the
    #     libraries named in LIBRARY_NAME|LIBRARY_NAMES
    # - openflow_INCLUDE_DIRS will contain a list of include directories that contain
    #     headers named in HEADER_NAME|HEADER_NAMES and directories that contain
    #     those directories.
    #     e.g. searching for core-module.h will return
    #     both ns-3-dev/build/include/ns3 and ns-3-dev/build/include,
    #     allowing users to include both <core-module.h> and <ns3/core-module.h>
    # If a user-settable variable was created, it can be searched too by
    # adding it to the SEARCH_PATHS
    find_external_library(
      DEPENDENCY_NAME openflow
      HEADER_NAME openflow.h
      LIBRARY_NAME openflow
      SEARCH_PATHS ${NS3_WITH_OPENFLOW} # user-settable search path, empty by default
    )

    # Before testing if the header and library were found ${openflow_FOUND},
    # test if openflow_FOUND was defined
    # If openflow_FOUND was not defined, the dependency name above doesn't match
    # the tested values below
    # If openflow_FOUND is set to FALSE, stop processing the module by returning
    # to the parent directory with return()
    if((NOT
        openflow_FOUND)
       AND (NOT
            ${openflow_FOUND})
    )
      message(STATUS "Openflow was not found")
      return()
    endif()

    # Check for the Boost header used by the openflow module
    check_include_file_cxx(
      boost/static_assert.hpp
      BOOST_STATIC_ASSERT
    )

    # Stop processing the module if it was not found
    if(NOT
      BOOST_STATIC_ASSERT
    )
      message(STATUS "Openflow requires Boost static_assert.hpp")
      return()
    endif()

    # Here we consume the include directories found by
    # find_external_library
    #
    # This will make the following work:
    #  include<openflow/openflow.h>
    #  include<openflow.h>
    include_directories(${openflow_INCLUDE_DIRS})

    # Manually set definitions
    add_definitions(
      -DNS3_OPENFLOW
      -DENABLE_OPENFLOW
    )

    # Set the cache variable indicating Openflow is enabled as
    # all dependencies were met
    set(NS3_OPENFLOW
        "ON"
        CACHE INTERNAL
              "ON if Openflow is found in NS3_WITH_OPENFLOW"
    )

    # Additional compilation flag to ignore a specific warning
    add_compile_options(-Wno-stringop-truncation)

    # Call macro to create the module target
    build_lib(
      LIBNAME openflow
      SOURCE_FILES
        helper/openflow-switch-helper.cc
        model/openflow-interface.cc
        model/openflow-switch-net-device.cc
      HEADER_FILES
        helper/openflow-switch-helper.h
        model/openflow-interface.h
        model/openflow-switch-net-device.h
      LIBRARIES_TO_LINK ${libinternet}
                        # Here we consume the list of libraries
                        # exported by find_external_library
                        ${openflow_LIBRARIES}
      TEST_SOURCES test/openflow-switch-test-suite.cc
    )


Linking third-party libraries using CMake's find_package
========================================================

Assume we have a module with optional features that rely on a third-party library
that provides a FindThirdPartyPackage.cmake. This ``Find${Package}.cmake`` file can be distributed
by `CMake itself`_, via library/package managers (APT, Pacman,
`vcpkg`_), or included to the project tree in the build-support/3rd-party directory.

.. _CMake itself: https://github.com/Kitware/CMake/tree/master/Modules
.. _Vcpkg: https://github.com/Microsoft/vcpkg#using-vcpkg-with-cmake

When ``find_package(${Package})`` is called, the ``Find${Package}.cmake`` file gets processed,
and multiple variables are set. There is no hard standard in the name of those variables, nor if
they should follow the modern CMake usage, where just linking to the library will include
associated header directories, forward compile flags and so on.

We assume the old CMake style is the one being used, which means we need to include the include
directories provided by the ``Find${Package}.cmake module``, usually exported as a variable
``${Package}_INCLUDE_DIRS``, and get a list of libraries for that module so that they can be
added to the list of libraries to link of the |ns3| modules. Libraries are usually exported as
the variable ``${Package}_LIBRARIES``.

As an example for the above, we use the Boost library
(excerpt from ``macros-and-definitions.cmake`` and ``src/core/CMakeLists.txt``):

.. sourcecode:: cmake

  # https://cmake.org/cmake/help/v3.10/module/FindBoost.html?highlight=module%20find#module:FindBoost
  find_package(Boost)

  # It is recommended to create either an empty list that is conditionally filled
  # and later included in the LIBRARIES_TO_LINK list unconditionally
  set(boost_libraries)

  # If Boost is found, Boost_FOUND will be set to true, which we can then test
  if(${Boost_FOUND})
    # This will export Boost include directories to ALL subdirectories
    # of the current CMAKE_CURRENT_SOURCE_DIR
    #
    # If calling this from the top-level directory (ns-3-dev), it will
    # be used by all contrib/src modules, examples, etc
    include_directories(${Boost_INCLUDE_DIRS})

    # This is a trick for Boost
    # Sometimes you want to check if specific Boost headers are available,
    # but they would not be found if they're not in system include directories
    set(CMAKE_REQUIRED_INCLUDES ${Boost_INCLUDE_DIRS})

    # We get the list of Boost libraries and save them in the boost_libraries list
    set(boost_libraries ${Boost_LIBRARIES})
  endif()

  # If Boost was found earlier, we will be able to check if Boost headers are available
  check_include_file_cxx(
    "boost/units/quantity.hpp"
    HAVE_BOOST_UNITS_QUANTITY
  )
  check_include_file_cxx(
    "boost/units/systems/si.hpp"
    HAVE_BOOST_UNITS_SI
  )
  if(${HAVE_BOOST_UNITS_QUANTITY}
    AND ${HAVE_BOOST_UNITS_SI}
  )
    # Activate optional features that rely on Boost
    add_definitions(
      -DHAVE_BOOST
      -DHAVE_BOOST_UNITS
    )
    # In this case, the Boost libraries are header-only,
    # but in case we needed real libraries, we could add
    # boost_libraries to either the auxiliary libraries_to_link list
    # or the build_lib's LIBRARIES_TO_LINK list
    message(STATUS "Boost Units have been found.")
  else()
    message(
      STATUS
        "Boost Units are an optional feature of length.cc."
    )
  endif()


If ``Find${Package}.cmake`` does not exist in your module path, CMake will warn you that is the case.
If ``${Package_FOUND}`` is set to ``False``, other variables such as the ones related to libraries and
include directories might not be set, and can result in CMake failures to configure if used.

In case the ``Find${Package}.cmake`` you need is not distributed by the upstream CMake project,
you can create your own and add it to ``build-support/3rd-party``. This directory is included
to the ``CMAKE_MODULE_PATH`` variable, making it available for calls without needing to include
the file with the absolute path to it. To add more directories to the ``CMAKE_MODULE_PATH``,
use the following:

.. sourcecode:: cmake

  # Excerpt from build-support/macros-and-definitions.cmake

  # Add ns-3 custom modules to the module path
  list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/build-support/custom-modules")

  # Add the 3rd-party modules to the module path
  list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/build-support/3rd-party")

  # Add your new modules directory to the module path
  # (${PROJECT_SOURCE_DIR} is /path/to/ns-3-dev/)
  list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/build-support/new-modules")


One of the custom Find files currently shipped by |ns3| is the ``FindGTK3.cmake`` file.
GTK3 requires Harfbuzz, which has its own ``FindHarfBuzz.cmake`` file. Both of them
are in the ``build-support/3rd-party`` directory.

.. sourcecode:: cmake

  # You don't need to keep adding this, this is just a demonstration
  list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/build-support/3rd-party")

  # If the user-settable NS3_GTK3 is set, look for HarfBuzz and GTK
  if(${NS3_GTK3})
    # Use FindHarfBuzz.cmake to find HarfBuzz
    find_package(HarfBuzz QUIET)

    # If HarfBuzz is not found
    if(NOT ${HarfBuzz_FOUND})
      message(STATUS "Harfbuzz is required by GTK3 and was not found.")
    else()
      # FindGTK3.cmake does some weird tricks and results in warnings,
      # that we can only suppress this way
      set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE BOOL "")

      # If HarfBuzz is found, search for GTK
      find_package(GTK3 QUIET)

      # Remove suppressions needed for quiet operations
      unset(CMAKE_SUPPRESS_DEVELOPER_WARNINGS CACHE)

      # If GTK3 is not found, inform the user
      if(NOT ${GTK3_FOUND})
        message(STATUS "GTK3 was not found. Continuing without it.")
      else()
        # If an incompatible version is found, set the GTK3_FOUND flag to false,
        # to make sure it won't be used later
        if(${GTK3_VERSION} VERSION_LESS 3.22)
          set(GTK3_FOUND FALSE)
          message(STATUS "GTK3 found with incompatible version ${GTK3_VERSION}")
        else()
          # A compatible GTK3 version was found
          message(STATUS "GTK3 was found.")
        endif()
      endif()
    endif()
  endif()

The Stats module can use the same ``find_package`` macro to search for SQLite3.

Note: we currently use a custom macro to find Python3 and SQLite3 since
``FindPython3.cmake`` and ``FindSQLite3.cmake`` were included in CMake 3.12 and 3.14.
More details on how to use the macro are listed in
`Linking third-party libraries without CMake or PkgConfig support`_.

.. sourcecode:: cmake

    # Set enable flag to false before checking
    set(ENABLE_SQLITE False)

    # In this case, SQLite presence is only checked if the user sets
    # NS3_SQLITE to ON, but your case may be different
    if(${NS3_SQLITE})
      # FindSQLite3.cmake is used by CMake to find SQLite3
      # QUIET flag silences most warnings from the module and let us write our own
      find_package(SQLite3 QUIET) # FindSQLite3.cmake was included in CMake 3.14

      # If SQLite3 was found, SQLite3_FOUND will be set to True, otherwise to False
      if(${SQLite3_FOUND})
        set(ENABLE_SQLITE True)
      else()
        message(STATUS "SQLite was not found")
      endif()
    endif()

    # Here we declare empty lists, that only hold values if ENABLE_SQLITE is set to ON
    set(sqlite_sources)
    set(sqlite_header)
    set(sqlite_libraries)
    if(${ENABLE_SQLITE})
      # If SQLite was found, add the optional source files to the lists
      set(sqlite_sources
          model/sqlite-data-output.cc
      )
      set(sqlite_headers
          model/sqlite-data-output.h
      )
      # Include the include directories containing the sqlite3.h header
      include_directories(${SQLite3_INCLUDE_DIRS})
      # Copy the list of sqlite3 libraries
      set(sqlite_libraries
          ${SQLite3_LIBRARIES}
      )

      # If the semaphore header is also found,
      # append additional optional source files to
      # the sqlite sources and headers lists
      if(HAVE_SEMAPHORE_H)
        list(
          APPEND
          sqlite_sources
          model/sqlite-output.cc
        )
        list(
          APPEND
          sqlite_headers
          model/sqlite-output.h
        )
      endif()
    endif()

    # Sources and headers file lists for stats are quite long,
    # so we use these auxiliary lists
    # The optional sqlite_sources and sqlite_headers can be empty or not
    set(source_files
        ${sqlite_sources}
        # ...
        model/uinteger-8-probe.cc
    )

    set(header_files
        ${sqlite_headers}
        # ...
        model/uinteger-8-probe.h
    )

    # Create the stats module consuming source files
    build_lib(
      LIBNAME stats
      SOURCE_FILES ${source_files}
      HEADER_FILES ${header_files}
      LIBRARIES_TO_LINK ${libcore}
                        # Here we either have an empty list or
                        # a list with the sqlite library
                        ${sqlite_libraries}
      TEST_SOURCES
        test/average-test-suite.cc
        test/basic-data-calculators-test-suite.cc
        test/double-probe-test-suite.cc
        test/histogram-test-suite.cc
    )



Linking third-party libraries with PkgConfig support
====================================================

Assume we have a module with optional features that rely on a third-party library
that uses PkgConfig. We can look for the PkgConfig module and add the optional
source files similarly to the previous cases, as shown in the example below:

.. sourcecode:: cmake

    # Include CMake script to use pkg-config
    include(FindPkgConfig)

    # If pkg-config was found, search for library you want
    if(PKG_CONFIG_FOUND)
      pkg_check_modules(THIRD_PARTY libthird-party)
    endif()

    set(third_party_sources)
    set(third_party_libs)
    # Set cached variable if both pkg-config and libthird-party are found
    if(PKG_CONFIG_FOUND AND THIRD_PARTY)
      # Include third-party include directories for
      # consumption of the current module and its examples
      include_directories(${THIRD_PARTY_INCLUDE_DIRS})

      # Use exported CFLAGS required by the third-party library
      add_compile_options(${THIRD_PARTY_CFLAGS})

      # Copy the list of third-party libraries
      set(third_party_libs ${THIRD_PARTY_LIBRARIES})

      # Add optional source files that depend on the third-party library
      set(third_party_sources model/optional-feature.cc)
    endif()

    # Create module using the optional source files and libraries
    build_lib(
      LIBNAME hypothetical
      SOURCE_FILES model/hypothetical.cc
                   ${third_party_sources}
      HEADER_FILES model/hypothetical.h
      LIBRARIES_TO_LINK ${libcore}
                        # Here we either have an empty list or
                        # a list with the third-party library
                        ${third_party_libs}
      TEST_SOURCES
        test/hypothetical.cc
    )

.. _Using C++ library managers:

Using C++ library managers
++++++++++++++++++++++++++

It is not rare to try using a library that is not available
on a certain platform or does not have a CMake-friendly
interface for us to use.

.. _CPM: https://github.com/cpm-cmake/CPM.cmake

Some C++ package managers are fairly easy to use with CMake,
such as `Vcpkg`_ and `CPM`_.

vcpkg
=====

Vcpkg requires ``git``, ``curl``, ``zip``, ``unzip`` and ``tar``,
along with the default ns-3 dependencies. The setup downloads
and builds vcpkg from their Git repository. Telemetry is disabled
by default.

.. sourcecode:: console

  ~$ ./ns3 configure -- -DNS3_VCPKG=ON
  ...
  -- vcpkg: setting up support
  Cloning into 'vcpkg'...
  Updating files: 100% (10376/10376), done.
  Downloading vcpkg-glibc...
  vcpkg package management program version 2023-07-19-814b7ec837b59f1c8778f72351c1dd7605983cd2
  ...

Configuration will finish successfully.
For example, now we can try using the Armadillo library.
To do that, we use the following CMake statements:

.. sourcecode:: cmake

    # Check this is not a fluke
    find_package(Armadillo)
    message(STATUS "Armadillo was found? ${ARMADILLO_FOUND}")

Reconfigure ns-3 to check if Armadillo is available.

.. sourcecode:: console

  ~$ ./ns3 configure
  ...
  -- vcpkg: setting up support
  -- vcpkg: folder already exists, skipping git download
  -- vcpkg: already bootstrapped
  ...
  -- Could NOT find Armadillo (missing: ARMADILLO_INCLUDE_DIR)
  -- Armadillo was found? FALSE

As you can see, no Armadillo found.
We can now use vcpkg to install it, using the CMake
function ``add_package(package_name)``. CMake
will then be able to find the installed package using ``find_package``.

Note: some packages may require additional dependencies.
The Armadillo package requires ``pkg-config`` and a fortran compiler.
You will be prompted with a CMake error when a missing dependency is found.

.. sourcecode:: cmake

    # Install Armadillo and search for it again
    add_package(Armadillo) # Installs Armadillo with vcpkg
    find_package(Armadillo) # Loads vcpkg installation of Armadillo
    message(STATUS "Armadillo was found? ${ARMADILLO_FOUND}")

Sadly, we will need to reconfigure ns-3 from the scratch,
since CMake ``find_package`` caches are problematic.
Installing the packages can take a while, and it can look like it hanged.

.. sourcecode:: console

  ~$ ./ns3 clean
  ~$ ./ns3 configure -- -DNS3_VCPKG=ON
  ...
  -- vcpkg: setting up support
  -- vcpkg: folder already exists, skipping git download
  -- vcpkg: already bootstrapped
  ...
  -- vcpkg: Armadillo will be installed
  -- vcpkg: Armadillo was installed
  -- Armadillo was found? TRUE

As shown above, the Armadillo library gets installed by vcpkg
and it can be found by CMake's ``find_package`` function.
We can then use it for our targets.

.. sourcecode:: cmake

    # Install Armadillo
    add_package(Armadillo) # Installs Armadillo with vcpkg
    find_package(Armadillo) # Loads vcpkg installation of Armadillo
    message(STATUS "Armadillo was found? ${ARMADILLO_FOUND}")

    # Include and link Armadillo to targets
    include_directories(${ARMADILLO_INCLUDE_DIRS})
    link_libraries(${ARMADILLO_LIBRARIES})

.. _vcpkg manifests: https://learn.microsoft.com/en-us/vcpkg/users/manifests

An alternative to manually installing packages with ``add_package`` is
placing all packages into a ``vcpkg.json`` file in the ns-3 main directory.
This mode is known as the "manifest mode" in the Vcpkg manual.
Packages there will be automatically installed at the beginning of the configuration.
More information about the manifest mode can be found in `vcpkg manifests`_ website.

Let us see an example of this mode starting with the ``vcpkg.json`` file.

.. sourcecode:: json

    {
      "dependencies": [
        "sqlite3",
        "eigen3",
        "libxml2",
        "gsl",
        "boost-units"
      ]
    }

These are some of the optional dependencies used by the upstream ns-3 modules.
When configuring ns-3 with the Vcpkg support, we will see the following.

.. sourcecode:: console

    /ns-3-dev$ ./ns3 clean
    /ns-3-dev$ ./ns3 configure -- -DNS3_VCPKG=ON
    ...
    -- vcpkg: setting up support
    Cloning into 'vcpkg'...
    Updating files: 100% (10434/10434), done.
    Downloading vcpkg-glibc...
    vcpkg package management program version 2023-08-02-6d13efa755f9b5e101712d210199e4139b4c29f6

    See LICENSE.txt for license information.
    -- vcpkg: detected a vcpkg manifest file: /ns-3-dev/vcpkg.json
    A suitable version of cmake was not found (required v3.27.1) Downloading portable cmake 3.27.1...
    Downloading cmake...
    https://github.com/Kitware/CMake/releases/download/v3.27.1/cmake-3.27.1-linux-x86_64.tar.gz->/ns-3-dev/vcpkg/downloads/cmake-3.27.1-linux-x86_64.tar.gz
    Extracting cmake...
    Detecting compiler hash for triplet x64-linux...
    The following packages will be built and installed:
      * boost-array:x64-linux -> 1.82.0#2
      ...
      * boost-winapi:x64-linux -> 1.82.0#2
        eigen3:x64-linux -> 3.4.0#2
        gsl:x64-linux -> 2.7.1#3
      * libiconv:x64-linux -> 1.17#1
      * liblzma:x64-linux -> 5.4.3#1
        libxml2[core,iconv,lzma,zlib]:x64-linux -> 2.10.3#1
        sqlite3[core,json1]:x64-linux -> 3.42.0#1
      * vcpkg-cmake:x64-linux -> 2023-05-04
      * vcpkg-cmake-config:x64-linux -> 2022-02-06#1
      * vcpkg-cmake-get-vars:x64-linux -> 2023-03-02
      * zlib:x64-linux -> 1.2.13
    Additional packages (*) will be modified to complete this operation.
    Restored 0 package(s) from /root/.cache/vcpkg/archives in 98.7 us. Use --debug to see more details.
    Installing 1/58 boost-uninstall:x64-linux...
    ...
    Installing 50/58 boost-units:x64-linux...
    Building boost-units:x64-linux...
    -- Downloading https://github.com/boostorg/units/archive/boost-1.82.0.tar.gz -> boostorg-units-boost-1.82.0.tar.gz...
    -- Extracting source /ns-3-dev/vcpkg/downloads/boostorg-units-boost-1.82.0.tar.gz
    -- Using source at /ns-3-dev/vcpkg/buildtrees/boost-units/src/ost-1.82.0-a9fdcc40b2.clean
    -- Copying headers
    -- Copying headers done
    -- Installing: /ns-3-dev/vcpkg/packages/boost-units_x64-linux/share/boost-units/usage
    -- Installing: /ns-3-dev/vcpkg/packages/boost-units_x64-linux/share/boost-units/copyright
    -- Performing post-build validation
    Stored binaries in 1 destinations in 276 ms.
    Elapsed time to handle boost-units:x64-linux: 3.8 s
    Installing 51/58 vcpkg-cmake-config:x64-linux...
    Building vcpkg-cmake-config:x64-linux...
    -- Installing: /ns-3-dev/vcpkg/packages/vcpkg-cmake-config_x64-linux/share/vcpkg-cmake-config/vcpkg_cmake_config_fixup.cmake
    -- Installing: /ns-3-dev/vcpkg/packages/vcpkg-cmake-config_x64-linux/share/vcpkg-cmake-config/vcpkg-port-config.cmake
    -- Installing: /ns-3-dev/vcpkg/packages/vcpkg-cmake-config_x64-linux/share/vcpkg-cmake-config/copyright
    -- Performing post-build validation
    Stored binaries in 1 destinations in 8.58 ms.
    Elapsed time to handle vcpkg-cmake-config:x64-linux: 144 ms
    Installing 52/58 eigen3:x64-linux...
    Building eigen3:x64-linux...
    -- Downloading https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz -> libeigen-eigen-3.4.0.tar.gz...
    -- Extracting source /ns-3-dev/vcpkg/downloads/libeigen-eigen-3.4.0.tar.gz
    -- Applying patch remove_configure_checks.patch
    -- Applying patch fix-vectorized-reductions-half.patch
    -- Using source at /ns-3-dev/vcpkg/buildtrees/eigen3/src/3.4.0-74a8d62212.clean
    -- Configuring x64-linux
    -- Building x64-linux-dbg
    -- Building x64-linux-rel
    -- Fixing pkgconfig file: /ns-3-dev/vcpkg/packages/eigen3_x64-linux/lib/pkgconfig/eigen3.pc
    CMake Error at scripts/cmake/vcpkg_find_acquire_program.cmake:163 (message):
      Could not find pkg-config.  Please install it via your package manager:

          sudo apt-get install pkg-config
    Call Stack (most recent call first):
      scripts/cmake/vcpkg_fixup_pkgconfig.cmake:203 (vcpkg_find_acquire_program)
      ports/eigen3/portfile.cmake:30 (vcpkg_fixup_pkgconfig)
      scripts/ports.cmake:147 (include)


    error: building eigen3:x64-linux failed with: BUILD_FAILED
    Elapsed time to handle eigen3:x64-linux: 19 s
    Please ensure you're using the latest port files with `git pull` and `vcpkg update`.
    Then check for known issues at:
        https://github.com/microsoft/vcpkg/issues?q=is%3Aissue+is%3Aopen+in%3Atitle+eigen3
    You can submit a new issue at:
        https://github.com/microsoft/vcpkg/issues/new?title=[eigen3]+Build+error&body=Copy+issue+body+from+%2Fns-3-dev%2Fvcpkg%2Finstalled%2Fvcpkg%2Fissue_body.md

    CMake Error at build-support/3rd-party/colored-messages.cmake:82 (_message):
      vcpkg: packages defined in the manifest failed to be installed
    Call Stack (most recent call first):
      build-support/custom-modules/ns3-vcpkg-hunter.cmake:138 (message)
      build-support/custom-modules/ns3-vcpkg-hunter.cmake:183 (setup_vcpkg)
      build-support/macros-and-definitions.cmake:743 (include)
      CMakeLists.txt:149 (process_options)

As we can see above, the setup failed during the eigen3 setup due to a missing dependency.
In this case, pkg-config. We can install it using the system package manager and then
resume the ns-3 configuration.


.. sourcecode:: console

    /ns-3-dev$ apt install -y pkg-config
    /ns-3-dev$ ./ns3 configure -- -DNS3_VCPKG=ON
    ...
    -- vcpkg: folder already exists, skipping git download
    -- vcpkg: already bootstrapped
    -- vcpkg: detected a vcpkg manifest file: /ns-3-dev/vcpkg.json
    Detecting compiler hash for triplet x64-linux...
    The following packages will be built and installed:
        eigen3:x64-linux -> 3.4.0#2
        gsl:x64-linux -> 2.7.1#3
      * libiconv:x64-linux -> 1.17#1
      * liblzma:x64-linux -> 5.4.3#1
        libxml2[core,iconv,lzma,zlib]:x64-linux -> 2.10.3#1
        sqlite3[core,json1]:x64-linux -> 3.42.0#1
      * zlib:x64-linux -> 1.2.13
    Additional packages (*) will be modified to complete this operation.
    Restored 0 package(s) from /root/.cache/vcpkg/archives in 97.6 us. Use --debug to see more details.
    Installing 1/7 eigen3:x64-linux...
    Building eigen3:x64-linux...
    -- Using cached libeigen-eigen-3.4.0.tar.gz.
    -- Cleaning sources at /ns-3-dev/vcpkg/buildtrees/eigen3/src/3.4.0-74a8d62212.clean. Use --editable to skip cleaning for the packages you specify.
    -- Extracting source /ns-3-dev/vcpkg/downloads/libeigen-eigen-3.4.0.tar.gz
    -- Applying patch remove_configure_checks.patch
    -- Applying patch fix-vectorized-reductions-half.patch
    -- Using source at /ns-3-dev/vcpkg/buildtrees/eigen3/src/3.4.0-74a8d62212.clean
    -- Configuring x64-linux
    -- Building x64-linux-dbg
    -- Building x64-linux-rel
    -- Fixing pkgconfig file: /ns-3-dev/vcpkg/packages/eigen3_x64-linux/lib/pkgconfig/eigen3.pc
    -- Fixing pkgconfig file: /ns-3-dev/vcpkg/packages/eigen3_x64-linux/debug/lib/pkgconfig/eigen3.pc
    -- Installing: /ns-3-dev/vcpkg/packages/eigen3_x64-linux/share/eigen3/copyright
    -- Performing post-build validation
    Stored binaries in 1 destinations in 1.7 s.
    Elapsed time to handle eigen3:x64-linux: 28 s
    Installing 2/7 gsl:x64-linux...
    ...
    Installing 7/7 sqlite3:x64-linux...
    Building sqlite3[core,json1]:x64-linux...
    -- Downloading https://sqlite.org/2023/sqlite-amalgamation-3420000.zip -> sqlite-amalgamation-3420000.zip...
    -- Extracting source /ns-3-dev/vcpkg/downloads/sqlite-amalgamation-3420000.zip
    -- Applying patch fix-arm-uwp.patch
    -- Applying patch add-config-include.patch
    -- Using source at /ns-3-dev/vcpkg/buildtrees/sqlite3/src/on-3420000-e624a7f335.clean
    -- Configuring x64-linux
    -- Building x64-linux-dbg
    -- Building x64-linux-rel
    -- Fixing pkgconfig file: /ns-3-dev/vcpkg/packages/sqlite3_x64-linux/lib/pkgconfig/sqlite3.pc
    -- Fixing pkgconfig file: /ns-3-dev/vcpkg/packages/sqlite3_x64-linux/debug/lib/pkgconfig/sqlite3.pc
    -- Installing: /ns-3-dev/vcpkg/packages/sqlite3_x64-linux/share/sqlite3/usage
    -- Performing post-build validation
    Stored binaries in 1 destinations in 430 ms.
    Elapsed time to handle sqlite3:x64-linux: 42 s
    Total install time: 2.5 min
    The package boost is compatible with built-in CMake targets:

        find_package(Boost REQUIRED [COMPONENTS <libs>...])
        target_link_libraries(main PRIVATE Boost::boost Boost::<lib1> Boost::<lib2> ...)

    eigen3 provides CMake targets:

        # this is heuristically generated, and may not be correct
        find_package(Eigen3 CONFIG REQUIRED)
        target_link_libraries(main PRIVATE Eigen3::Eigen)

    The package gsl is compatible with built-in CMake targets:

        find_package(GSL REQUIRED)
        target_link_libraries(main PRIVATE GSL::gsl GSL::gslcblas)

    The package libxml2 is compatible with built-in CMake targets:

        find_package(LibXml2 REQUIRED)
        target_link_libraries(main PRIVATE LibXml2::LibXml2)

    sqlite3 provides pkgconfig bindings.
    sqlite3 provides CMake targets:

        find_package(unofficial-sqlite3 CONFIG REQUIRED)
        target_link_libraries(main PRIVATE unofficial::sqlite3::sqlite3)

    -- vcpkg: packages defined in the manifest were installed
    -- find_external_library: SQLite3 was found.
    ...
    -- LibXML2 was found.
    ...
    -- Found Boost: /ns-3-dev/vcpkg/installed/x64-linux/include (found version "1.82.0")
    ...
    -- Looking for include files boost/units/quantity.hpp, boost/units/systems/si.hpp
    -- Looking for include files boost/units/quantity.hpp, boost/units/systems/si.hpp - found
    -- Boost Units have been found.
    ...
    -- ---- Summary of ns-3 settings:
    Build profile                 : default
    Build directory               : /ns-3-dev/build
    Build with runtime asserts    : ON
    ...
    GNU Scientific Library (GSL)  : ON
    ...
    LibXml2 support               : ON
    ...
    SQLite support                : ON
    Eigen3 support                : ON
    ...

From the above, we can see that the headers and libraries installed by the packages
were correctly found by CMake and the optional features were successfully enabled.

.. _Armadillo's port on vcpkg: https://github.com/microsoft/vcpkg/blob/master/ports/armadillo/usage

Note: not every vcpkg package (also known as a port) obeys
the same pattern for usage. The user of the package needs
to look into the usage file of said port for instructions.
In the case of Armadillo, the corresponding file can be found
in `Armadillo's port on vcpkg`_.

Vcpkg is installed to a ``vcpkg`` directory inside the ns-3 main directory (e.g. ``ns-3-dev``).
Packages installed via vcpkg are installed to ``ns-3-dev/vcpkg/installed/${VCPKG_TRIPLET}``,
which is automatically added to the ``CMAKE_PREFIX_PATH``, making headers, libraries, and
pkg-config and CMake packages discoverable via ``find_file``, ``find_library``, ``find_package``
and ``pkg_check_modules``.

CPM
===

`CPM`_ is a package manager made for CMake projects consuming CMake projects.
Some CMake projects however, create files during the installation step, which
is not supported by CPM, which treats the package as a CMake subproject that
we can then depend upon. CPM may require dependencies such as ``git`` and ``tar``,
depending on the package sources used.

Let's see an example trying to find the Armadillo library via CMake.

.. sourcecode:: cmake

    # Check this is not a fluke
    find_package(Armadillo)
    message(STATUS "Armadillo was found? ${ARMADILLO_FOUND}")

Reconfigure ns-3 to check if Armadillo is available.

.. sourcecode:: console

  ~$ ./ns3 configure
  ...
  -- Could NOT find Armadillo (missing: ARMADILLO_INCLUDE_DIR)
  -- Armadillo was found? FALSE

As you can see, no Armadillo found.
We can now use CPM to install it, using the CMake
function ``CPMAddPackage(package_info)``.

.. sourcecode:: cmake

    # Install Armadillo and search for it again
    CPMAddPackage(
        NAME ARMADILLO
        GIT_TAG 6cada351248c9a967b137b9fcb3d160dad7c709b
        GIT_REPOSITORY https://gitlab.com/conradsnicta/armadillo-code.git
    )
    find_package(Armadillo) # Loads CPM installation of Armadillo
    message(STATUS "Armadillo was found? ${ARMADILLO_FOUND}")

Sadly, we will need to reconfigure ns-3 from the scratch,
since CMake ``find_package`` caches are problematic.
Installing the packages can take a while, and it can look like it hanged.

.. sourcecode:: console

  ~$ ./ns3 clean
  ~$ ./ns3 configure -- -DNS3_CPM=ON
  ...
  -- CPM: Adding package ARMADILLO@0 (6cada351248c9a967b137b9fcb3d160dad7c709b)
  -- *** set cmake policy CMP0025 to NEW
  -- CMAKE_CXX_STANDARD = 11
  -- Configuring Armadillo 12.6.1
  --
  -- *** WARNING: variable 'CMAKE_CXX_FLAGS' is not empty; this may cause problems!
  --
  -- Detected Clang 6.0 or newer
  -- ARMA_USE_EXTERN_RNG = true
  -- CMAKE_SYSTEM_NAME          = Linux
  -- CMAKE_CXX_COMPILER_ID      = Clang
  -- CMAKE_CXX_COMPILER_VERSION = 15.0.7
  -- CMAKE_COMPILER_IS_GNUCXX   =
  --
  -- *** Options:
  -- BUILD_SHARED_LIBS         = ON
  -- OPENBLAS_PROVIDES_LAPACK  = OFF
  -- ALLOW_FLEXIBLAS_LINUX     = ON
  -- ALLOW_OPENBLAS_MACOS      = OFF
  -- ALLOW_BLAS_LAPACK_MACOS   = OFF
  -- BUILD_SMOKE_TEST          = ON
  --
  -- *** Looking for external libraries
  -- Found OpenBLAS: /usr/lib/x86_64-linux-gnu/libopenblas.so
  -- Found BLAS: /usr/lib/x86_64-linux-gnu/libblas.so
  -- Found LAPACK: /usr/lib/x86_64-linux-gnu/liblapack.so
  -- FlexiBLAS_FOUND = NO
  --       MKL_FOUND = NO
  --  OpenBLAS_FOUND = YES
  --     ATLAS_FOUND = NO
  --      BLAS_FOUND = YES
  --    LAPACK_FOUND = YES
  --
  -- *** NOTE: found both OpenBLAS and BLAS; BLAS will not be used
  --
  -- *** NOTE: if OpenBLAS is known to provide LAPACK functions, recommend to
  -- *** NOTE: rerun cmake with the OPENBLAS_PROVIDES_LAPACK option enabled:
  -- *** NOTE: cmake -D OPENBLAS_PROVIDES_LAPACK=true .
  --
  -- *** If the OpenBLAS library is installed in
  -- *** /usr/local/lib or /usr/local/lib64
  -- *** make sure the run-time linker can find it.
  -- *** On Linux systems this can be done by editing /etc/ld.so.conf
  -- *** or modifying the LD_LIBRARY_PATH environment variable.
  --
  -- Found ARPACK: /usr/lib/x86_64-linux-gnu/libarpack.so
  -- ARPACK_FOUND = YES
  -- Looking for SuperLU version 5
  -- Found SuperLU: /usr/lib/x86_64-linux-gnu/libsuperlu.so
  -- SuperLU_FOUND = YES
  -- SuperLU_INCLUDE_DIR = /usr/include/superlu
  --
  -- *** Result of configuration:
  -- *** ARMA_USE_WRAPPER    = true
  -- *** ARMA_USE_LAPACK     = true
  -- *** ARMA_USE_BLAS       = true
  -- *** ARMA_USE_ATLAS      = false
  -- *** ARMA_USE_ARPACK     = true
  -- *** ARMA_USE_EXTERN_RNG = true
  -- *** ARMA_USE_SUPERLU    = true
  --
  -- *** Armadillo wrapper library will use the following libraries:
  -- *** ARMA_LIBS = /usr/lib/x86_64-linux-gnu/libopenblas.so;/usr/lib/x86_64-linux-gnu/liblapack.so;/usr/lib/x86_64-linux-gnu/libarpack.so;/usr/lib/x86_64-linux-gnu/libsuperlu.so
  --
  -- Copying /ns-3-dev/cmake-build-release/_deps/armadillo-src/include/ to /ns-3-dev/cmake-build-release/_deps/armadillo-build/tmp/include/
  -- Generating /ns-3-dev/cmake-build-release/_deps/armadillo-build/tmp/include/config.hpp
  -- CMAKE_CXX_FLAGS           =  -fsanitize=address,leak,undefined -Os
  -- CMAKE_SHARED_LINKER_FLAGS =  -Wl,--no-as-needed
  -- CMAKE_REQUIRED_INCLUDES   = /usr/include;/usr/include/superlu
  --
  -- CMAKE_INSTALL_PREFIX     = /usr
  -- CMAKE_INSTALL_LIBDIR     = lib/x86_64-linux-gnu
  -- CMAKE_INSTALL_INCLUDEDIR = include
  -- CMAKE_INSTALL_DATADIR    = share
  -- CMAKE_INSTALL_BINDIR     = bin
  -- Generating '/ns-3-dev/cmake-build-release/_deps/armadillo-build/ArmadilloConfig.cmake'
  -- Generating '/ns-3-dev/cmake-build-release/_deps/armadillo-build/ArmadilloConfigVersion.cmake'
  -- Generating '/ns-3-dev/cmake-build-release/_deps/armadillo-build/InstallFiles/ArmadilloConfig.cmake'
  -- Generating '/ns-3-dev/cmake-build-release/_deps/armadillo-build/InstallFiles/ArmadilloConfigVersion.cmake'
  -- Copying /ns-3-dev/cmake-build-release/_deps/armadillo-src/misc/ to /ns-3-dev/cmake-build-release/_deps/armadillo-build/tmp/misc/
  -- Generating '/ns-3-dev/cmake-build-release/_deps/armadillo-build/tmp/misc/armadillo.pc'
  -- *** configuring smoke_test
  -- Armadillo was found? TRUE
  ...

As shown above, the Armadillo library gets installed by CPM
and it can be found by CMake's ``find_package`` function.
Differently from other packages found via ``find_package``,
CPM creates native CMake targets from the subprojects.
In the case of Armadillo, the target is called ``armadillo``,
which we can link to our targets.

.. sourcecode:: cmake

    # Install Armadillo
    CPMAddPackage(
        NAME ARMADILLO
        GIT_TAG 6cada351248c9a967b137b9fcb3d160dad7c709b
        GIT_REPOSITORY https://gitlab.com/conradsnicta/armadillo-code.git
    )
    find_package(Armadillo) # Loads CPM installation of Armadillo
    message(STATUS "Armadillo was found? ${ARMADILLO_FOUND}")

    # CPM is kind of jenky. It could get the ARMADILLO_INCLUDE_DIRS
    # from the ArmadilloConfig.cmake file in ${CMAKE_BINARY_DIR}/_deps/armadillo-build,
    # but it doesn't... So add its include directories directly from the source directory
    include_directories(${CMAKE_BINARY_DIR}/_deps/armadillo-src/include)

    # Link to Armadillo and
    link_libraries(armadillo)

.. _`CPM's examples`: https://github.com/cpm-cmake/CPM.cmake/wiki/More-Snippets

Note: using CPM can be challenging. Users are recommended
to look at `CPM's examples`_.

For example, the libraries of installed packages will be placed by default in
the ``ns-3-dev/build/lib`` directory. On the other hand, header placement depends
on how the CMake project was setup.

If the package CMakeLists.txt was made to build in-source, headers will be
along the source files, which will be placed in
``${PROJECT_BINARY_DIR}/_deps/packageName-src``. When configured with the
ns3 script, ``PROJECT_BINARY_DIR corresponds`` to ``ns-3-dev/cmake-cache``.

If the package CMakeLists.txt copies the headers to an output directory
(like ns-3 does), it will be placed in
``${PROJECT_BINARY_DIR}/_deps/packageName-build``,
possibly in an ``include`` subdirectory.

In case it was configured to copy the headers to ``${CMAKE_BINARY_DIR}/include``,
the headers will land on ``${PROJECT_BINARY_DIR}/include`` of the most top-level
project. In our case, the top-level project is the NS3 project.

Since the packages get installed into the ns-3 cache directory
(``PROJECT_BINARY_DIR``), using ``./ns3 clean`` will delete them,
requiring them to be rebuilt.

Inclusion of options
++++++++++++++++++++

There are two ways of managing module options: option switches or cached variables.
Both are present in the main CMakeLists.txt in the ns-3-dev directory and the
build-support/macros-and-definitions.cmake file.


.. sourcecode:: cmake

    # Here are examples of ON and OFF switches
    # option(
    #        NS3_SWITCH # option switch prefixed with NS3\_
    #        "followed by the description of what the option does"
    #        ON # and the default value for that option
    #        )
    option(NS3_EXAMPLES "Enable examples to be built" OFF)
    option(NS3_TESTS "Enable tests to be built" OFF)

    # Now here is how to let the user indicate a path
    # set( # declares a value
    #     NS3_PREFIXED_VALUE # stores the option value
    #     "" # default value is empty in this case
    #     CACHE # stores that NS3_PREFIXED_VALUE in the CMakeCache.txt file
    #     STRING # type of the cached variable
    #     "description of what this value is used for"
    #     )
    set(NS3_OUTPUT_DIRECTORY "" CACHE PATH "Directory to store built artifacts")

    # The last case are options that can only assume predefined values
    # First we cache the default option
    set(NS3_INT64X64 "INT128" CACHE STRING "Int64x64 implementation")

    # Then set a cache property for the variable indicating it can assume
    # specific values
    set_property(CACHE NS3_INT64X64 PROPERTY STRINGS INT128 CAIRO DOUBLE)


More details about these commands can be found in the following links:
`option`_, `set`_, `set_property`_.

.. _option: https://cmake.org/cmake/help/latest/command/option.html
.. _set: https://cmake.org/cmake/help/latest/command/set.html
.. _set_property: https://cmake.org/cmake/help/latest/command/set_property.html


Changes in CMake macros and functions
+++++++++++++++++++++++++++++++++++++

In order for CMake to feel more familiar to Waf users, a few macros and functions
were created.

The most frequently used macros them can be found in
``build-support/macros-and-definitions.cmake``. This file includes build type checking,
compiler family and version checking, enabling and disabling features based
on user options, checking for dependencies of enabled features,
pre-compiling headers, filtering enabled/disabled modules and dependencies,
and more.

Executable macros
=================

Creating an executable in CMake requires a few different macro calls.
Some of these calls are related to setting the target and built executable name,
indicating which libraries that should be linked to the executable,
where the executable should be placed after being built and installed.

Note that if you are trying to add a new example to your module, you should
look at the `build_lib_example`_ macro section.

If you are trying to add a new example to ``~/ns-3-dev/examples``, you should
look at the `build_example`_ macro section.

While both of the previously mentioned macros are meant to be used for examples,
in some cases additional utilities are required. Those utilities can be helpers,
such as the ``raw-sock-creator`` in the ``fd-net-device`` module, or benchmark
tools in the ``~/ns-3-dev/utils`` directory. In those cases, the `build_exec`_
macro is recommended instead of direct CMake calls.

.. _build_exec:

Executable macros: build_exec
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``build_exec`` macro bundles a series of direct CMake calls into a single macro.
The example below shows the creation of an executable named ``example``, that will
later receive a version prefix (e.g. ``ns3.37-``) and a build type suffix
(e.g. ``-debug``), resulting in an executable file named ``ns3.37-example-debug``.

The list of source and header files can be passed in the ``SOURCE_FILES`` and
``HEADER_FILES`` arguments, followed by the ``LIBRARIES_TO_LINK`` that will be linked
to the executable.

That executable will be saved by default to the ``CMAKE_RUNTIME_OUTPUT_DIRECTORY``
(e.g. /ns-3-dev/build/bin). To change its destination, set ``EXECUTABLE_DIRECTORY_PATH``
to the desired path. The path is relative to the ``CMAKE_OUTPUT_DIRECTORY``
(e.g. /ns-3-dev/build).

In case this executable should be installed, set ``INSTALL_DIRECTORY_PATH`` to the
desired destination. In case this value is empty, the executable will not be installed.
The path is relative to the ``CMAKE_INSTALL_PREFIX`` (e.g. /usr).

To set custom compiler defines for that specific executable, defines can be passed
to the ``DEFINITIONS`` argument.

Add the ``STANDALONE`` option to prevent linking the |ns3| static library
(``NS3_STATIC``) and single shared library (``NS3_MONOLIB``) to the executable.
This may be necessary in case the executable redefine symbols which are part
of the |ns3| library. This is the case for the fd-net-device creators and the tap-creator,
which include the source file ``encode-decode.cc``, which is also part of fd-net-device module
and tap-bridge module, respectively.

Finally, to ignore precompiled headers, include ``IGNORE_PCH`` to the list of parameters.
You can find more information about ``IGNORE_PCH`` at the `PCH side-effects`_ section.

.. sourcecode:: cmake

   build_exec(
     # necessary
     EXECNAME example                       # executable name = example (plus version prefix and build type suffix)
     SOURCE_FILES example.cc example-complement.cc
     HEADER_FILES example.h
     LIBRARIES_TO_LINK ${libcore}           # links to core
     EXECUTABLE_DIRECTORY_PATH scratch          # build/scratch
     # optional
     EXECNAME_PREFIX scratch_subdir_prefix_ # target name = scratch_subdir_prefix_example
     INSTALL_DIRECTORY_PATH ${CMAKE_INSTALL_BIN}/   # e.g. /usr/bin/ns3.37-scratch_subdir_prefix_example-debug
     DEFINITIONS -DHAVE_FEATURE=1           # defines for this specific target
     [STANDALONE]                           # set in case you don't want the executable to be linked to ns3-static/ns3-monolib
     IGNORE_PCH
   )

The same executable can be built by directly calling the following CMake macros:

.. sourcecode:: cmake

   set(target_prefix scratch_subdir_prefix_)
   set(target_name example)
   set(output_directory scratch)

   # Creates a target named "example" (target_name) prefixed with "scratch_subdir_prefix_" (target_prefix)
   # e.g. scratch_subdir_prefix_example
   add_executable(${target_prefix}${target_name} example.cc example-complement.cc)
   target_link_libraries(${target_prefix}${target_name} PUBLIC ${libcore})

   # Create a variable with the target name prefixed with
   # the version and suffixed with the build profile suffix
   # e.g. ns3.37-scratch_subdir_prefix_example-debug
   set(ns3-exec-outputname ns${NS3_VER}-${target_prefix}${target_name}${build_profile_suffix})

   # Append the binary name to the executables list later written to the lock file,
   # which is consumed by the ns3 script and test.py
   set(ns3-execs "${output_directory}${ns3-exec-outputname};${ns3-execs}"
      CACHE INTERNAL "list of c++ executables"
   )
   # Modify the target properties to change the binary name to ns3-exec-outputname contents
   # and modify its output directory (e.g. scratch). The output directory is relative to the build directory.
   set_target_properties(
     ${target_prefix}${target_name}
     PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${output_directory}
                RUNTIME_OUTPUT_NAME ${ns3-exec-outputname}
   )
   # Create a dependency between the target and the all-test-targets
   # (used by ctest, coverage and doxygen targets)
   add_dependencies(all-test-targets ${target_prefix}${target_name})

   # Create a dependency between the target and the timeTraceReport
   # (used by Clang TimeTrace to collect compilation statistics)
   add_dependencies(timeTraceReport ${target_prefix}${target_name}) # target used to track compilation time

   # Set target-specific compile definitions
   target_compile_definitions(${target_prefix}${target_name} PUBLIC definitions)

   # Check whether the target should reuse or not the precompiled headers
   if(NOT ${IGNORE_PCH})
       target_precompile_headers(
             ${target_prefix}${target_name} REUSE_FROM stdlib_pch_exec
          )
   endif()


.. _build_example:

Executable macros: build_example
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``build_example`` macro sets some of ``build_exec``'s arguments based on the current
example directory (output directory) and adds the optional visualizer module as a dependency
in case it is enabled. It also performs dependency checking on the libraries passed.

In case one of the dependencies listed is not found, the example target will not be created.
If you are trying to add an example or a dependency to an existing example and it is not
listed by ``./ns3 show targets`` or your IDE, check if all its dependencies were found.

.. sourcecode:: cmake

   macro(build_example)
     set(options IGNORE_PCH)
     set(oneValueArgs NAME)
     set(multiValueArgs SOURCE_FILES HEADER_FILES LIBRARIES_TO_LINK)
     # Parse arguments
     cmake_parse_arguments(
       "EXAMPLE" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
     )

     # Filter examples out if they don't contain one of the filtered in modules
     set(filtered_in ON)
     if(NS3_FILTER_MODULE_EXAMPLES_AND_TESTS)
       set(filtered_in OFF)
       foreach(filtered_module NS3_FILTER_MODULE_EXAMPLES_AND_TESTS)
         if(${filtered_module} IN_LIST EXAMPLE_LIBRARIES_TO_LINK)
           set(filtered_in ON)
         endif()
       endforeach()
     endif()

     # Check if any of the LIBRARIES_TO_LINK is missing to prevent configuration errors
     check_for_missing_libraries(
       missing_dependencies "${EXAMPLE_LIBRARIES_TO_LINK}"
     )

     if((NOT missing_dependencies) AND ${filtered_in})
       # Convert boolean into text to forward argument
       if(${EXAMPLE_IGNORE_PCH})
         set(IGNORE_PCH IGNORE_PCH)
       endif()
       # Create example library with sources and headers
       # cmake-format: off
       build_exec(
         EXECNAME ${EXAMPLE_NAME}
         SOURCE_FILES ${EXAMPLE_SOURCE_FILES}
         HEADER_FILES ${EXAMPLE_HEADER_FILES}
         LIBRARIES_TO_LINK ${EXAMPLE_LIBRARIES_TO_LINK} ${ns3-optional-visualizer-lib}
         EXECUTABLE_DIRECTORY_PATH
           ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/examples/${examplefolder}/
         ${IGNORE_PCH}
       )
     # cmake-format: on
     endif()
   endmacro()

An example on how it is used can be found in ``~/ns-3-dev/examples/tutorial/CMakeLists.txt``:

.. sourcecode:: cmake

   build_example(
     NAME first
     SOURCE_FILES first.cc
     LIBRARIES_TO_LINK
       ${libcore}
       ${libpoint-to-point}
       ${libinternet}
       ${libapplications}
       # If visualizer is available, the macro will add the module to this list automatically
     # build_exec's EXECUTABLE_DIRECTORY_PATH will be set to build/examples/tutorial/
   )

Module macros
=============

Module macros are located in ``build-support/custom-modules/ns3-module-macros.cmake``.
This file contains macros defining a library (``build_lib``), the associated test library,
examples (``build_lib_example``) and more. It also contains the macro that builds the
module header (``write_module_header``) that includes all headers from the module
for user scripts.

These macros are responsible for easing the porting of modules from Waf to CMake.

.. _build_lib:

Module macros: build_lib
~~~~~~~~~~~~~~~~~~~~~~~~

As ``build_lib`` is the most important of the macros, we detail what it does here,
block by block.

The first block declares the arguments received by the macro (in CMake, the
only difference is that a function has its own scope). Notice that there are
different types of arguments. Options that can only be set to ON/OFF.
Options are ``OFF`` by default, and are set to ``ON`` if the option name is added to
the arguments list (e.g. ``build_lib(... IGNORE_PCH)``).

Note: You can find more information about ``IGNORE_PCH`` at the `PCH side-effects`_ section.

One value arguments that receive a single value
(usually a string) and in this case used to receive the module name (``LIBNAME``).

Multiple value arguments receive a list of values, which we use to parse lists
of source (for the module itself and for the module tests) and
header files, plus libraries that should be linked and module features.

The call to ``cmake_parse_arguments`` will parse ``${ARGN}`` into these values.
The variables containing the parsing results will be prefixed with ``BLIB_``
(e.g. ``LIBNAME`` -> ``BLIB_LIBNAME``).

.. sourcecode:: cmake

  function(build_lib)
    # Argument parsing
    set(options IGNORE_PCH)
    set(oneValueArgs LIBNAME)
    set(multiValueArgs SOURCE_FILES HEADER_FILES LIBRARIES_TO_LINK TEST_SOURCES
                      DEPRECATED_HEADER_FILES MODULE_ENABLED_FEATURES
    )
    cmake_parse_arguments(
      "BLIB" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
    )
    # ...
  endfunction()

In the following block, we add modules in the src folder to a list
and modules in the contrib folder to a different list.

.. sourcecode:: cmake

  function(build_lib)
    # ...
    # Get path src/module or contrib/module
    string(REPLACE "${PROJECT_SOURCE_DIR}/" "" FOLDER
                  "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    # Add library to a global list of libraries
    if("${FOLDER}" MATCHES "src")
      set(ns3-libs "${BLIB_LIBNAME};${ns3-libs}"
          CACHE INTERNAL "list of processed upstream modules"
      )
    else()
      set(ns3-contrib-libs "${BLIB_LIBNAME};${ns3-contrib-libs}"
          CACHE INTERNAL "list of processed contrib modules"
      )
    endif()

We build a shared library using CMake's ``add_library(${BLIB_LIBNAME} SHARED ...)``.

Notice that we can also reuse precompiled headers created previously to speed up the
parsing phase of the compilation.


.. sourcecode:: cmake

  function(build_lib)
    # ...
    add_library(${BLIB_LIBNAME} SHARED "${BLIB_SOURCE_FILES}")

    if(${PRECOMPILE_HEADERS_ENABLED} AND (NOT ${IGNORE_PCH}))
      target_precompile_headers(${BLIB_LIBNAME} REUSE_FROM stdlib_pch)
    endif()
    # ...
  endfunction()

In the next code block, we create an alias to ``module``, ``ns3::module``,
which can later be used when importing |ns3| with CMake's ``find_package(ns3)``.

And finally associate all of the public headers of the module to that library,
to make sure CMake will be refreshed in case one of them changes.

.. sourcecode:: cmake

  function(build_lib)
    # ...
    add_library(ns3::${BLIB_LIBNAME} ALIAS ${BLIB_LIBNAME})

    # Associate public headers with library for installation purposes
    set_target_properties(
      ${BLIB_LIBNAME}
      PROPERTIES
        PUBLIC_HEADER
        "${BLIB_HEADER_FILES};${BLIB_DEPRECATED_HEADER_FILES};${CMAKE_HEADER_OUTPUT_DIRECTORY}/${BLIB_LIBNAME}-module.h"
    )
    # ...
  endfunction()

In the next code block, we make the library a dependency to the ClangAnalyzer's time trace report,
which measures which step of compilation took most time and which files were responsible for that.

.. sourcecode:: cmake

  function(build_lib)
    # ...

    build_lib_reexport_third_party_libraries(
      "${BLIB_LIBNAME}" "${BLIB_LIBRARIES_TO_LINK}"
    )

    # ...
  endfunction(build_lib)

The ``build_lib_reexport_third_party_libraries`` macro then separates |ns3| libraries from non-|ns3| libraries.
The non-|ns3| can be automatically propagated, or not, to libraries/executables linked to the current |ns3| module being processed.

.. sourcecode:: cmake

    function(build_lib_reexport_third_party_libraries libname libraries_to_link)
      # Separate ns-3 and non-ns-3 libraries to manage their propagation properly
      separate_ns3_from_non_ns3_libs(
        "${libname}" "${libraries_to_link}" ns_libraries_to_link
        non_ns_libraries_to_link
      )

      set(ns3-external-libs "${non_ns_libraries_to_link};${ns3-external-libs}"
          CACHE INTERNAL
                "list of non-ns libraries to link to NS3_STATIC and NS3_MONOLIB"
      )
      # ...
    endfunction()

The default is propagating these third-party libraries and their include directories, but this
can be turned off by setting ``NS3_REEXPORT_THIRD_PARTY_LIBRARIES=OFF``

.. sourcecode:: cmake

  function(build_lib_reexport_third_party_libraries libname libraries_to_link)
    # ...

    if(NOT ${NS3_REEXPORT_THIRD_PARTY_LIBRARIES})
      # ns-3 libraries are linked publicly, to make sure other modules can find
      # each other without being directly linked
      set(exported_libraries PUBLIC ${LIB_AS_NEEDED_PRE} ${ns_libraries_to_link}
                             ${LIB_AS_NEEDED_POST}
      )

      # non-ns-3 libraries are linked privately, not propagating unnecessary
      # libraries such as pthread, librt, etc
      set(private_libraries PRIVATE ${LIB_AS_NEEDED_PRE}
                            ${non_ns_libraries_to_link} ${LIB_AS_NEEDED_POST}
      )

      # we don't re-export included libraries from 3rd-party modules
      set(exported_include_directories)
    else()
      # we export everything by default when NS3_REEXPORT_THIRD_PARTY_LIBRARIES=ON
      set(exported_libraries PUBLIC ${LIB_AS_NEEDED_PRE} ${ns_libraries_to_link}
                             ${non_ns_libraries_to_link} ${LIB_AS_NEEDED_POST}
      )
      set(private_libraries)

      # with NS3_REEXPORT_THIRD_PARTY_LIBRARIES, we export all 3rd-party library
      # include directories, allowing consumers of this module to include and link
      # the 3rd-party code with no additional setup
      get_target_includes(${libname} exported_include_directories)

      string(REPLACE "-I" "" exported_include_directories
                     "${exported_include_directories}"
      )

      # include directories prefixed in the source or binary directory need to be
      # treated differently
      set(new_exported_include_directories)
      foreach(directory ${exported_include_directories})
        string(FIND "${directory}" "${PROJECT_SOURCE_DIR}" is_prefixed_in_subdir)
        if(${is_prefixed_in_subdir} GREATER_EQUAL 0)
          string(SUBSTRING "${directory}" ${is_prefixed_in_subdir} -1
                           directory_path
          )
          list(APPEND new_exported_include_directories
               $<BUILD_INTERFACE:${directory_path}>
          )
        else()
          list(APPEND new_exported_include_directories ${directory})
        endif()
      endforeach()
      set(exported_include_directories ${new_exported_include_directories})

      string(REPLACE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/include" ""
                     exported_include_directories
                     "${exported_include_directories}"
      )
    endif()
    # ...
  endfunction()

After the lists of libraries to link that should be exported (``PUBLIC``) and
not exported (``PRIVATE``) are built, we can link them with ``target_link_libraries``.

.. sourcecode:: cmake

  function(build_lib_reexport_third_party_libraries libname libraries_to_link)
    # ...

    # Set public and private headers linked to the module library
    target_link_libraries(${libname} ${exported_libraries} ${private_libraries})

    # ...
  endfunction()

Next we export include directories, to let library consumers importing |ns3| via CMake
use them just by linking to one of the |ns3| modules.

.. sourcecode:: cmake

  function(build_lib_reexport_third_party_libraries libname libraries_to_link)
    # ...
    # export include directories used by this library so that it can be used by
    # 3rd-party consumers of ns-3 using find_package(ns3) this will automatically
    # add the build/include path to them, so that they can ns-3 headers with
    # <ns3/something.h>
    target_include_directories(
      ${libname} PUBLIC $<BUILD_INTERFACE:${CMAKE_OUTPUT_DIRECTORY}/include>
                        $<INSTALL_INTERFACE:include>
      INTERFACE ${exported_include_directories}
    )
    # ...
  endfunction()

The following block creates the ``${BLIB_LIBNAME}-module.h`` header for user scripts,
and copies header files from ``src/module`` and ``contrib/module`` to the ``include/ns3`` directory.

.. sourcecode:: cmake

  function(build_lib)
    # ...
    # Write a module header that includes all headers from that module
    write_module_header("${BLIB_LIBNAME}" "${BLIB_HEADER_FILES}")

    # ...

    # Copy all header files to outputfolder/include before each build
    copy_headers(
      PUBLIC_HEADER_OUTPUT_DIR ${CMAKE_HEADER_OUTPUT_DIRECTORY}
      PUBLIC_HEADER_FILES ${BLIB_HEADER_FILES}
      DEPRECATED_HEADER_OUTPUT_DIR ${CMAKE_HEADER_OUTPUT_DIRECTORY}
      DEPRECATED_HEADER_FILES ${BLIB_DEPRECATED_HEADER_FILES}
      PRIVATE_HEADER_OUTPUT_DIR ${CMAKE_HEADER_OUTPUT_DIRECTORY}
      PRIVATE_HEADER_FILES ${BLIB_PRIVATE_HEADER_FILES}
    )
    # ...
  endfunction()

The following block creates the test library for the module currently being processed.
Note that it handles Windows vs non-Windows differently, since Windows requires all
used linked symbols to be used, which results in our test-runner being unable to
be dynamically linked to ns-3 modules. To solve that, we create the test libraries
for the different modules as object libraries instead, and statically link to the
test-runner executable.

.. sourcecode:: cmake

  function(build_lib)
    # ...
    build_lib_tests(
        "${BLIB_LIBNAME}" "${BLIB_IGNORE_PCH}" "${FOLDER}" "${BLIB_TEST_SOURCES}"
      )
    # ...
  endfunction()

  # This macro builds the test library for the module library
  #
  # Arguments: libname (e.g. core), ignore_pch (TRUE/FALSE), folder (src/contrib),
  # sources (list of .cc's)
  function(build_lib_tests libname ignore_pch folder test_sources)
    if(${ENABLE_TESTS})
      # Check if the module tests should be built
      build_lib_check_examples_and_tests_filtered_in(${libname} filtered_in)
      if(NOT ${filtered_in})
        return()
      endif()
      list(LENGTH test_sources test_source_len)
      if(${test_source_len} GREATER 0)
        # Create libname of output library test of module
        set(test${libname} ${libname}-test CACHE INTERNAL "")

        # Create shared library containing tests of the module on UNIX and just
        # the object file that will be part of test-runner on Windows
        if(WIN32)
          set(ns3-libs-tests
              "$<TARGET_OBJECTS:${test${libname}}>;${ns3-libs-tests}"
              CACHE INTERNAL "list of test libraries"
          )
          add_library(${test${libname}} OBJECT "${test_sources}")
        else()
          set(ns3-libs-tests "${test${libname}};${ns3-libs-tests}"
              CACHE INTERNAL "list of test libraries"
          )
          add_library(${test${libname}} SHARED "${test_sources}")

          # Link test library to the module library
          if(${NS3_MONOLIB})
            target_link_libraries(
              ${test${libname}} ${LIB_AS_NEEDED_PRE} ${lib-ns3-monolib}
              ${LIB_AS_NEEDED_POST}
            )
          else()
            target_link_libraries(
              ${test${libname}} ${LIB_AS_NEEDED_PRE} ${libname}
              "${BLIB_LIBRARIES_TO_LINK}" ${LIB_AS_NEEDED_POST}
            )
          endif()
          set_target_properties(
            ${test${libname}}
            PROPERTIES OUTPUT_NAME
                       ns${NS3_VER}-${libname}-test${build_profile_suffix}
          )
        endif()
        target_compile_definitions(
          ${test${libname}} PRIVATE NS_TEST_SOURCEDIR="${folder}/test"
        )
        if(${PRECOMPILE_HEADERS_ENABLED} AND (NOT ${ignore_pch}))
          target_precompile_headers(${test${libname}} REUSE_FROM stdlib_pch)
        endif()

        # Add dependency between tests and examples used as tests
        examples_as_tests_dependencies("${module_examples}" "${test_sources}")
      endif()
    endif()
  endfunction()


The following block checks for examples subdirectories and add them to parse their
CMakeLists.txt file, creating the examples. It also scans for python examples.

.. sourcecode:: cmake

  function(build_lib)
    # ...

    # Scan for C++ and Python examples and return a list of C++ examples (which
    # can be set as dependencies of examples-as-test test suites)
    build_lib_scan_examples(module_examples)

    # ...
  endfunction()

  # This macro scans for C++ and Python examples for a given module and return a
  # list of C++ examples
  #
  # Arguments: module_cpp_examples = return list of C++ examples
  function(build_lib_scan_examples module_cpp_examples)
    # Build lib examples if requested
    set(examples_before ${ns3-execs-clean})
    foreach(example_folder example;examples)
      if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${example_folder})
        if(${ENABLE_EXAMPLES})
          if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${example_folder}/CMakeLists.txt)
            add_subdirectory(${example_folder})
          endif()
        endif()
        scan_python_examples(${CMAKE_CURRENT_SOURCE_DIR}/${example_folder})
      endif()
    endforeach()
    set(module_examples ${ns3-execs-clean})

    # Return a list of module c++ examples (current examples - previous examples)
    list(REMOVE_ITEM module_examples ${examples_before})
    set(${module_cpp_examples} ${module_examples} PARENT_SCOPE)
  endfunction()

In the next code block we add the library to the ``ns3ExportTargets``, later used for installation.
We also print an additional message the folder just finished being processed if ``NS3_VERBOSE`` is set to ``ON``.

.. sourcecode:: cmake

  function(build_lib)
    # ...
    # Handle package export
    install(
      TARGETS ${BLIB_LIBNAME}
      EXPORT ns3ExportTargets
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/
      PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ns3"
    )
    if(${NS3_VERBOSE})
      message(STATUS "Processed ${FOLDER}")
    endif()
  endfunction()


.. _build_lib_example:

Module macros: build_lib_example
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The second most important macro from a module author perspective is the ``build_lib_example``, which
builds the examples for their module. As with ``build_lib`` we explain what it does block-by-block.

In the first block, arguments are parsed and we check whether the current module is in the contrib
or the src folder.

.. sourcecode:: cmake

  function(build_lib_example)
    # Argument parsing
    set(options IGNORE_PCH)
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCE_FILES HEADER_FILES LIBRARIES_TO_LINK)
    cmake_parse_arguments("BLIB_EXAMPLE" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Get path src/module or contrib/module
    string(REPLACE "${PROJECT_SOURCE_DIR}/" "" FOLDER "${CMAKE_CURRENT_SOURCE_DIR}")
    get_filename_component(FOLDER ${FOLDER} DIRECTORY)
    # ...
  endfunction()

Then we check if the |ns3| modules required by the example are enabled to be built.
If the list ``missing_dependencies`` is empty, we create the example. Otherwise, we skip it.
The example can be linked to the current module (``${lib${BLIB_EXAMPLE_LIBNAME}}``) and
other libraries to link (``${BLIB_EXAMPLE_LIBRARIES_TO_LINK}``) and optionally to the visualizer
module (``${ns3-optional-visualizer-lib}``).
If the visualizer module is not enabled, ``ns3-optional-visualizer-lib`` is empty.

The example can also be linked to a single |ns3| shared library (``lib-ns3-monolib``) or
a single |ns3| static library (``lib-ns3-static``), if either ``NS3_MONOLIB=ON`` or ``NS3_STATIC=ON``.
Note that both of these options are handled by the ``build_exec`` macro.

.. sourcecode:: cmake

  function(build_lib_example)
    # ...
    check_for_missing_libraries(missing_dependencies "${BLIB_EXAMPLE_LIBRARIES_TO_LINK}")

    # Check if a module example should be built
    set(filtered_in ON)
    if(NS3_FILTER_MODULE_EXAMPLES_AND_TESTS)
      set(filtered_in OFF)
      if(${BLIB_LIBNAME} IN_LIST NS3_FILTER_MODULE_EXAMPLES_AND_TESTS)
        set(filtered_in ON)
      endif()
    endif()

    if((NOT missing_dependencies) AND ${filtered_in})
       # Convert boolean into text to forward argument
       if(${BLIB_EXAMPLE_IGNORE_PCH})
         set(IGNORE_PCH IGNORE_PCH)
       endif()
       # Create executable with sources and headers
       # cmake-format: off
       build_exec(
         EXECNAME ${BLIB_EXAMPLE_NAME}
         SOURCE_FILES ${BLIB_EXAMPLE_SOURCE_FILES}
         HEADER_FILES ${BLIB_EXAMPLE_HEADER_FILES}
         LIBRARIES_TO_LINK
           ${lib${BLIB_EXAMPLE_LIBNAME}} ${BLIB_EXAMPLE_LIBRARIES_TO_LINK}
           ${ns3-optional-visualizer-lib}
         EXECUTABLE_DIRECTORY_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${FOLDER}/
         ${IGNORE_PCH}
       )
       # cmake-format: on
    endif()
  endfunction()

The `build_exec`_ macro will also set resulting folder where the example will end up
after built (e.g. build/src/module/examples). It does that by forwarding the
``EXECUTABLE_DIRECTORY_PATH`` to the macro ``set_runtime_outputdirectory``, which also
adds the proper |ns3| version prefix and build type suffix to the executable.

As with the module libraries, we can also reuse precompiled headers here to speed up
the parsing step of compilation.
You can find more information about ``IGNORE_PCH`` at the `PCH side-effects`_ section.

User options and header checking
================================

User-settable options should be prefixed with ``NS3_``, otherwise
they will not be preserved by ``./ns3 configure --force-refresh``.

After checking if the pre-requisites of the user-settable options
are met, set the same option now prefixed with ``ENABLE_``. The
following example demonstrates this pattern:

.. sourcecode:: cmake

  # Option() means the variable NS3_GSL will be set to ON/OFF
  # The second argument is a comment explaining what this option does
  # The last argument is the default value for the user-settable option
  option(NS3_GSL "Enable GSL related features" OFF)

  # Set the ENABLE\_ counterpart to FALSE by default
  set(ENABLE_GSL FALSE)
  if(${NS3_GSL})
    # If the user enabled GSL, check if GSL is available
    find_package(GSL)
    if(${GSL_FOUND})
      set(ENABLE_GSL TRUE)
      message(STATUS "GSL was requested by the user and was found")
    else()
      message(STATUS "GSL was not found and GSL features will continue disabled")
    endif()
  else()
    message(STATUS "GSL features were not requested by the user")
  endif()

  # Now the module can check for ENABLE_GSL before being processed
  if(NOT ${ENABLE_GSL})
    return()
  endif()

  # Or to enable optional features
  set(gsl_sources)
  if(${ENABLE_GSL})
    set(gsl_sources model/gsl_features.cc)
  endif()

Here are examples of how to do the options and header checking,
followed by a header configuration:

.. sourcecode:: cmake

  # We always set the ENABLE\_ counterpart of NS3\_ option to FALSE before checking
  #
  # If this variable is created inside your module, use
  # set(ENABLE_MPI FALSE CACHE INTERNAL "")
  # instead, to make it globally available
  set(ENABLE_MPI FALSE)

  # If the user option switch is set to ON, we check
  if(${NS3_MPI})
    # Use find_package to look for MPI
    find_package(MPI QUIET)

    # If the package is optional, which is the case for MPI,
    # we can proceed if it is not found
    if(NOT ${MPI_FOUND})
      message(STATUS "MPI was not found. Continuing without it.")
    else()
      # If it is false, we add necessary C++ definitions (e.g. NS3_MPI)
      message(STATUS "MPI was found.")
      target_compile_definitions(MPI::MPI_CXX INTERFACE NS3_MPI)

      # Then set ENABLE_MPI to TRUE, which can be used to check
      # if NS3_MPI is enabled AND MPI was found
      #
      # If this variable is created inside your module, use
      # set(ENABLE_MPI TRUE CACHE INTERNAL "")
      # instead, to make it globally available
      set(ENABLE_MPI TRUE)
    endif()
  endif()

  # ...

  # These two standard CMake modules allow for header and function checking
  include(CheckIncludeFileCXX)
  include(CheckFunctionExists)

  # Check for required headers and functions,
  # set flags on the right argument if header in the first argument is found
  # if they are not found, a warning is emitted
  check_include_file_cxx("stdint.h" "HAVE_STDINT_H")
  check_include_file_cxx("inttypes.h" "HAVE_INTTYPES_H")
  check_include_file_cxx("sys/types.h" "HAVE_SYS_TYPES_H")
  check_include_file_cxx("stat.h" "HAVE_SYS_STAT_H")
  check_include_file_cxx("dirent.h" "HAVE_DIRENT_H")
  check_include_file_cxx("stdlib.h" "HAVE_STDLIB_H")
  check_include_file_cxx("signal.h" "HAVE_SIGNAL_H")
  check_include_file_cxx("netpacket/packet.h" "HAVE_PACKETH")
  check_function_exists("getenv" "HAVE_GETENV")

  # This is the CMake command to open up a file template (in this case a header
  # passed as the first argument), then fill its fields with values stored in
  # CMake variables and save the resulting file to the target destination
  # (in the second argument)
  configure_file(
    build-support/core-config-template.h
    ${CMAKE_HEADER_OUTPUT_DIRECTORY}/core-config.h
  )

The configure_file command is not very clear by itself, as you do not know which
values are being used. So we need to check the template.

.. sourcecode:: cpp

    #ifndef NS3_CORE_CONFIG_H
    #define NS3_CORE_CONFIG_H

    // Defined if HAVE_UINT128_T is defined in CMake
    #cmakedefine   HAVE_UINT128_T
    // Set to 1 if HAVE__UINT128_T is defined in CMake, 0 otherwise
    #cmakedefine01 HAVE___UINT128_T
    #cmakedefine   INT64X64_USE_128
    #cmakedefine   INT64X64_USE_DOUBLE
    #cmakedefine   INT64X64_USE_CAIRO
    #cmakedefine01 HAVE_STDINT_H
    #cmakedefine01 HAVE_INTTYPES_H
    #cmakedefine   HAVE_SYS_INT_TYPES_H
    #cmakedefine01 HAVE_SYS_TYPES_H
    #cmakedefine01 HAVE_SYS_STAT_H
    #cmakedefine01 HAVE_DIRENT_H
    #cmakedefine01 HAVE_STDLIB_H
    #cmakedefine01 HAVE_GETENV
    #cmakedefine01 HAVE_SIGNAL_H

    /*
    * #cmakedefine turns into:
    * //#define HAVE_FLAG // if HAVE_FLAG is not defined in CMake (e.g. unset(HAVE_FLAG))
    * #define HAVE_FLAG // if HAVE_FLAG is defined in CMake (e.g. set(HAVE_FLAG))
    *
    * #cmakedefine01 turns into:
    * #define HAVE_FLAG 0 // if HAVE_FLAG is not defined in CMake
    * #define HAVE_FLAG 1 // if HAVE_FLAG is defined in CMake
    */

    #endif //NS3_CORE_CONFIG_H

Custom targets
==============

Another common thing to do is implement custom targets that run specific commands and
manage dependencies. Here is an example for Doxygen:

.. sourcecode:: cmake

  # This command hides DOXYGEN from some CMake cache interfaces
  mark_as_advanced(DOXYGEN)

  # This custom macro checks for dependencies CMake find_package and program
  # dependencies and return the missing dependencies in the third argument
  check_deps(doxygen_docs_missing_deps EXECUTABLES doxygen dot dia python3)

  # If the variable contains missing dependencies, we stop processing doxygen targets
  if(doxygen_docs_missing_deps)
    message(
      STATUS
        "docs: doxygen documentation not enabled due to missing dependencies: ${doxygen_docs_missing_deps}"
    )
  else()
    # We checked this already exists, but we need the path to the executable
    find_package(Doxygen QUIET)

    # Get introspected doxygen
    add_custom_target(
      run-print-introspected-doxygen
      COMMAND
        ${CMAKE_OUTPUT_DIRECTORY}/utils/ns${NS3_VER}-print-introspected-doxygen${build_profile_suffix}
        > ${PROJECT_SOURCE_DIR}/doc/introspected-doxygen.h
      COMMAND
        ${CMAKE_OUTPUT_DIRECTORY}/utils/ns${NS3_VER}-print-introspected-doxygen${build_profile_suffix}
        --output-text > ${PROJECT_SOURCE_DIR}/doc/ns3-object.txt
      DEPENDS print-introspected-doxygen
    )

    # Run test.py with NS_COMMANDLINE_INTROSPECTION=.. to print examples
    # introspected commandline
    add_custom_target(
      run-introspected-command-line
      COMMAND ${CMAKE_COMMAND} -E env NS_COMMANDLINE_INTROSPECTION=..
              ${Python_EXECUTABLE} ./test.py --no-build --constrain=example
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      DEPENDS all-test-targets # all-test-targets only exists if ENABLE_TESTS is
                               # set to ON
    )

    # This file header is written during configuration
    file(
      WRITE ${PROJECT_SOURCE_DIR}/doc/introspected-command-line.h
      "/* This file is automatically generated by
  CommandLine::PrintDoxygenUsage() from the CommandLine configuration
  in various example programs.  Do not edit this file!  Edit the
  CommandLine configuration in those files instead.
  */
  \n"
    )
    # After running test.py for the introspected commandline above,
    # merge outputs and concatenate to the header file created during
    # configuration
    add_custom_target(
      assemble-introspected-command-line
      # works on CMake 3.18 or newer > COMMAND ${CMAKE_COMMAND} -E cat
      # ${PROJECT_SOURCE_DIR}/testpy-output/*.command-line >
      # ${PROJECT_SOURCE_DIR}/doc/introspected-command-line.h
      COMMAND ${cat_command} ${PROJECT_SOURCE_DIR}/testpy-output/*.command-line
              > ${PROJECT_SOURCE_DIR}/doc/introspected-command-line.h 2> NULL
      DEPENDS run-introspected-command-line
    )

    # Create a target that updates the doxygen version
    add_custom_target(
      update_doxygen_version
      COMMAND ${PROJECT_SOURCE_DIR}/doc/ns3_html_theme/get_version.sh
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )

    # Create a doxygen target that builds the documentation and only runs
    # after the version target above was executed, the introspected doxygen
    # and command line were extracted
    add_custom_target(
      doxygen
      COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_SOURCE_DIR}/doc/doxygen.conf
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      DEPENDS update_doxygen_version run-print-introspected-doxygen
              assemble-introspected-command-line
    )

    # Create a doxygen target that only needs to run the version target
    # which doesn't trigger compilation of examples neither the execution of test.py
    # nor print-introspected-doxygen
    add_custom_target(
      doxygen-no-build
      COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_SOURCE_DIR}/doc/doxygen.conf
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      DEPENDS update_doxygen_version
    )
  endif()


Project-wide compiler and linker flags
======================================

Different compilers and links accept different flags, which must be
known during configuration time. Some of these flags are
handled directly by CMake:

.. sourcecode:: cmake

  # equivalent to -fPIC for libraries and -fPIE for executables
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)

  # link-time optimization flags such as -flto and -flto=thin
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

  # C++ standard flag to use
  set(CMAKE_CXX_STANDARD_MINIMUM 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  add_library(static_lib STATIC) # equivalent to -static flag
  add_library(shared_lib SHARED) # equivalent to -shared flags

Other flags need to be handled manually and change based on
the compiler used. The most commonly used are handled in
``build-support/macros-and-definitions.cmake``.

.. sourcecode:: cmake

  set(LIB_AS_NEEDED_PRE)
  set(LIB_AS_NEEDED_POST)
  if(${GCC} AND NOT APPLE)
    # using GCC
    set(LIB_AS_NEEDED_PRE -Wl,--no-as-needed)
    set(LIB_AS_NEEDED_POST -Wl,--as-needed)
    set(LIB_AS_NEEDED_PRE_STATIC -Wl,--whole-archive,-Bstatic)
    set(LIB_AS_NEEDED_POST_STATIC -Wl,--no-whole-archive)
    set(LIB_AS_NEEDED_POST_STATIC_DYN -Wl,-Bdynamic,--no-whole-archive)
  endif()

The ``LIB_AS_NEEDED`` values are used to force linking of all symbols,
and not only those explicitly used by the applications, which is necessary
since simulation scripts don't directly use most of the symbols exported
by the modules. Their use can be seen in the ``utils/CMakeLists.txt``:

.. sourcecode:: cmake

  target_link_libraries(
      test-runner ${LIB_AS_NEEDED_PRE} ${ns3-libs-tests} ${LIB_AS_NEEDED_POST}
      ${ns3-libs} ${ns3-contrib-libs}
    )

This will ensure test-runner linking to ``ns3-libs-tests`` (list containing all
module test libraries) with all symbols, which will make it able to find and run
all tests. The other two lists ``ns3-libs`` (src modules) and ``ns3-contrib-libs``
(contrib modules) don't need to be completely linked since the tests libraries are
already linked to them.

Other project-wide compiler-dependent flags can be set during compiler checking.

.. sourcecode:: cmake

  # Check if the compiler is GCC
  set(GCC FALSE)
  if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    # Check if GCC is a supported version
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${GNU_MinVersion})
      message(
        FATAL_ERROR
          "GNU ${CMAKE_CXX_COMPILER_VERSION} ${below_minimum_msg} ${GNU_MinVersion}"
      )
    endif()
    # If GCC is up-to-date, set flag to true and continue
    set(GCC TRUE)

    # Disable semantic interposition
    add_definitions(-fno-semantic-interposition)
  endif()

.. _disables semantic interposition: https://gitlab.com/nsnam/ns-3-dev/-/merge_requests/777

The ``-fno-semantic-interposition`` flag `disables semantic interposition`_, which can
reduce overhead of function calls in shared libraries built with ``-fPIC``.
This is the default behaviour for Clang. The inlined |ns3| calls will not be
correctly interposed by the ``LD_PRELOAD`` trick, which is not know to be used by |ns3| users.
To re-enable semantic interposition, comment out the line and reconfigure the project.

Note: the most common use of the ``LD_PRELOAD`` trick is to use custom memory allocators,
and this continues to work since the interposed symbols are from the standard libraries,
which are compiled with semantic interposition.

Some modules may require special flags. The Openflow module for example
may require ``-Wno-stringop-truncation`` flag to prevent an warning that
is treated as error to prevent the compilation from proceeding. The flag
can be passed to the entire module with the following:

.. sourcecode:: cmake

  add_compile_options(-Wno-stringop-truncation)

  build_lib(
    LIBNAME openflow
    SOURCE_FILES
      helper/openflow-switch-helper.cc
      model/openflow-interface.cc
      model/openflow-switch-net-device.cc
    HEADER_FILES
      helper/openflow-switch-helper.h
      model/openflow-interface.h
      model/openflow-switch-net-device.h
    LIBRARIES_TO_LINK ${libinternet}
                      ${openflow_LIBRARIES}
    TEST_SOURCES test/openflow-switch-test-suite.cc
  )

If a flag prevents your compiler from compiling, wrap the flag inside a
compiler check. The currently checked compilers are ``GCC`` and ``CLANG``
(includes both upstream LLVM Clang and Apple Clang).

.. sourcecode:: cmake

  if(NOT ${FAILING_COMPILER})
    add_compile_options(-Wno-stringop-truncation)
  endif()

  # or

  if(${ONLY_COMPILER_THAT_SUPPORTS_UNIQUE_FLAG})
    add_compile_options(-unique_flag)
  endif()



CCache and Precompiled Headers
******************************

.. _CCache: https://ccache.dev/
.. _PCHs: https://gcc.gnu.org/onlinedocs/gcc/Precompiled-Headers.html


There are a few ways of speeding up the build of |ns3| and its modules.
Partially rebuilding only changed modules is one of the ways, and
this is already handled by the build system.

However, cleaning up the build and cmake cache directories removes
the intermediate and final files that could be used to skip the build
of unchanged modules.

In this case, `CCache`_ is recommended. It acts as a compiler and stores the
intermediate and final object files and libraries on a cache artifact directory.

Note: for ease of use, CCache is enabled by default if found by the build system.

The cache artifact directory of CCACHE can be set by changing the
``CCACHE_BASEDIR`` environment variable.

The CCache artifact cache is separated per directory, to prevent incompatible
artifacts, which may depend on different working directories ``CWD`` to work properly
from getting mixed and producing binaries that will start running from a different directory.

Note: to reuse CCache artifacts from different directories,
set the ``CCACHE_NOHASHDIR`` environment variable to ``true``.

A different way of speeding up builds is by using Precompiled Headers (`PCHs`_).
PCHs drastically reduce parsing times of C and C++ headers by precompiling their symbols,
which are imported instead of re-parsing the same headers again and again,
for each compilation unit (.cc file).

Note: for ease of use, PCH is enabled by default if supported. It can be manually disabled
by setting ``NS3_PRECOMPILE_HEADERS`` to ``OFF``.

Setting up and adding new headers to the PCH
++++++++++++++++++++++++++++++++++++++++++++

When both CCache and PCH are used together, there is a set of settings that must be
properly configured, otherwise timestamps built into the PCH can invalidate the CCache
artifacts, forcing a new build of unmodified modules/programs.

Compiler settings required by PCH and CCache are set in the PCH block in macros-and-definitions.cmake.

.. sourcecode:: cmake

  if(${PRECOMPILE_HEADERS_ENABLED})
    if(CLANG)
      # Clang adds a timestamp to the PCH, which prevents ccache from working
      # correctly
      # https://github.com/ccache/ccache/issues/539#issuecomment-664198545
      add_definitions(-Xclang -fno-pch-timestamp)
    endif()

    if(${XCODE})
      # XCode is weird and messes up with the PCH, requiring this flag
      # https://github.com/ccache/ccache/issues/156
      add_definitions(-Xclang -fno-validate-pch)
    endif()

    # Headers that will be compiled into the PCH
    # Only worth for frequently included headers
    set(precompiled_header_libraries
        <algorithm>
        <cstdlib>
        <cstring>
        <exception>
        <fstream>
        <iostream>
        <limits>
        <list>
        <map>
        <math.h>
        <ostream>
        <set>
        <sstream>
        <stdint.h>
        <stdlib.h>
        <string>
        <unordered_map>
        <vector>
    )

    # PCHs can be reused by similar targets (libraries or executables)
    # We have a PCH for libraries, compiled with the -fPIC flag
    add_library(stdlib_pch OBJECT ${PROJECT_SOURCE_DIR}/build-support/empty.cc)
    target_precompile_headers(
      stdlib_pch PUBLIC "${precompiled_header_libraries}"
    )

    # And another PCH for executables, compiled with the -fPIE flag
    add_executable(
      stdlib_pch_exec ${PROJECT_SOURCE_DIR}/build-support/empty-main.cc
    )
    target_precompile_headers(
      stdlib_pch_exec PUBLIC "${precompiled_header_libraries}"
    )
    set_runtime_outputdirectory(stdlib_pch_exec ${CMAKE_BINARY_DIR}/ "")
  endif()

The CCache settings required to work with PCH are set in the main CMakeLists.txt file:

.. sourcecode:: cmake

  # Use ccache if available
  mark_as_advanced(CCACHE)
  find_program(CCACHE ccache)
  if(NOT ("${CCACHE}" STREQUAL "CCACHE-NOTFOUND"))
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    message(STATUS "CCache is enabled.")

    # Changes user-wide settings from CCache to make it ignore:
    # - PCH definitions,
    # - time related macros that could bake timestamps into cached artifacts,
    # - source file creation and modification timestamps,
    #    forcing it to check for content changes instead
    execute_process(
      COMMAND
        ${CCACHE} --set-config
        sloppiness=pch_defines,time_macros,include_file_mtime,include_file_ctime
    )
  endif()

Note: you can use the following commands to manually check
and restore the CCache sloppiness settings.

.. sourcecode:: console

  ~$ ccache --get-config sloppiness
  include_file_mtime, include_file_ctime, time_macros, pch_defines
  ~$ ccache --set-config sloppiness=""
  ~$ ccache --get-config sloppiness

  ~$

The PCHs can be reused later with one of the following.

.. sourcecode:: cmake

  add_library(example_lib example_lib.cc)
  target_precompile_headers(example_lib REUSE_FROM stdlib_pch)

  add_executable(example_exec example_exec.cc)
  target_precompile_headers(example_exec REUSE_FROM stdlib_pch_exec)

If if you have problems with the build times when the PCH is enabled,
you can diagnose issues with CCache by clearing the cache statistics (``ccache -z``),
then cleaning, configuring, building, and finally printing the CCache statistics (``ccache -s``).

.. sourcecode:: console

  ccache -z
  ./ns3 clean
  ./ns3 configure
  ./ns3 build
  ccache -s

If you have changed any compiler flag, the cache hit rate should be very low.
Repeat the same commands once more.
If the cache hit rate is at 100%, it means everything is working as it should.

.. _PCH side-effects:

Possible side-effects, fixes and IGNORE_PCH
+++++++++++++++++++++++++++++++++++++++++++

Precompiled headers can cause symbol collisions due to includes reordering or unwanted includes,
which can lead to attempts to redefine functions, macros, types or variables.
An example of such side-effect is shown below.

In order to exemplify how precompiled headers can cause issues, assume the following inclusion order from
``ns-3-dev/src/aodv/model/aodv-routing-protocol.cc``:

.. sourcecode:: cpp

   ...
   #define NS_LOG_APPEND_CONTEXT                                   \
   if (m_ipv4) { std::clog << "[node " << m_ipv4->GetObject<Node> ()->GetId () << "] "; }

   #include "aodv-routing-protocol.h"
   #include "ns3/log.h"
   ...

The ``NS_LOG_APPEND_CONTEXT`` macro definition comes before the ``ns3/log.h`` inclusion,
and that is the expected way of using ``NS_LOG_APPEND_CONTEXT``, since we have the following
guards on ``ns3/log-macros-enabled.h``, which is included by ``ns3/log.h`` when logs are enabled.

.. sourcecode:: cpp

   ...
   #ifndef NS_LOG_APPEND_CONTEXT
   #define NS_LOG_APPEND_CONTEXT
   #endif /* NS_LOG_APPEND_CONTEXT */
   ...

By adding ``<ns3/log.h>`` to the list of headers to precompile (``precompiled_header_libraries``)
in ``ns-3-dev/build-support/macros-and-definitions.cmake``, the ``ns3/log.h`` header will
now be part of the PCH, which gets included before any parsing of the code is done.
This means the equivalent inclusion order would be different than what was originally intended,
as shown below:

.. sourcecode:: cpp


   #include "cmake_pch.hxx" // PCH includes ns3/log.h before defining ``NS_LOG_APPEND_CONTEXT`` below
   ...
   #define NS_LOG_APPEND_CONTEXT                                   \
   if (m_ipv4) { std::clog << "[node " << m_ipv4->GetObject<Node> ()->GetId () << "] "; }

   #include "aodv-routing-protocol.h"
   #include "ns3/log.h" // isn't processed since ``NS3_LOG_H`` was already defined by the PCH
   ...

While trying to build with the redefined symbols in the debug build, where warnings are treated
as errors, the build may fail with an error similar to the following from GCC 11.2:

.. sourcecode:: console

   FAILED: src/aodv/CMakeFiles/libaodv-obj.dir/model/aodv-routing-protocol.cc.o
   ccache /usr/bin/c++ ... -DNS3_LOG_ENABLE -Wall -Werror -include /ns-3-dev/cmake-build-debug/CMakeFiles/stdlib_pch.dir/cmake_pch.hxx
   /ns-3-dev/src/aodv/model/aodv-routing-protocol.cc:28: error: "NS_LOG_APPEND_CONTEXT" redefined [-Werror]
      28 | #define NS_LOG_APPEND_CONTEXT                                   \
         |
   In file included from /ns-3-dev/src/core/model/log.h:32,
                    from /ns-3-dev/src/core/model/fatal-error.h:29,
                    from /ns-3-dev/build/include/ns3/assert.h:56,
                    from /ns-3-dev/build/include/ns3/buffer.h:26,
                    from /ns-3-dev/build/include/ns3/packet.h:24,
                    from /ns-3-dev/cmake-build-debug/CMakeFiles/stdlib_pch.dir/cmake_pch.hxx:23,
                    from <command-line>:
   /ns-3-dev/src/core/model/log-macros-enabled.h:146: note: this is the location of the previous definition
     146 | #define NS_LOG_APPEND_CONTEXT
         |
   cc1plus: all warnings being treated as errors

One of the ways to fix this issue in particular is undefining ``NS_LOG_APPEND_CONTEXT`` before redefining it in
``/ns-3-dev/src/aodv/model/aodv-routing-protocol.cc``.

.. sourcecode:: cpp

   #include "cmake_pch.hxx" // PCH includes ns3/log.h before defining NS_LOG_APPEND_CONTEXT below
   ...
   #undef NS_LOG_APPEND_CONTEXT // undefines symbol previously defined in the PCH
   #define NS_LOG_APPEND_CONTEXT                                   \
   if (m_ipv4) { std::clog << "[node " << m_ipv4->GetObject<Node> ()->GetId () << "] "; }

   #include "aodv-routing-protocol.h"
   #include "ns3/log.h" // isn't processed since ``NS3_LOG_H`` was already defined by the PCH
   ...

If the ``IGNORE_PCH`` option is set in the `build_lib`_, `build_lib_example`_, `build_exec`_ and the `build_example`_
macros, the PCH is not included in their targets, continuing to build as we normally would without the PCH.
This is only a workaround for the issue, which can be helpful when the same macro names, class names,
global variables and others are redefined by different components that can't be modified safely.
