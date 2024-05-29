.. include:: replace.txt
.. highlight:: bash
.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

.. _Quick start:

Quick Start
-----------

This chapter summarizes the |ns3| installation process for C++ users interested in trying a
generic install of the main simulator.  Python bindings installation is not covered.

Some of this chapter is redundant with the
|ns3| `tutorial <https://www.nsnam.org/docs/tutorial/html/>`_, which also covers similar steps.

The steps are:

1. Download a source archive, or make a git clone, of |ns3| to a location on your file
   system (usually somewhere under your home directory).

2. Use a C++ compiler to compile the software into a set of shared libraries, executable example
   programs, and tests

|ns3| uses the CMake build system to manage the C++ compilation, and CMake itself calls on
a lower-level build system such as ``make`` to perform the actual compilation.

Prerequisites
*************

Make sure that your system has these prerequisites.  Download can be via either ``git`` or via
source archive download (via a web browser, ``wget``, or ``curl``).

  +--------------------+--------------------------------------+------------------------------+
  | **Purpose**        | **Tool**                             | **Minimum Version**          |
  +====================+==================+===================+==============================+
  | **Download**       | ``git`` (for Git download)           | No minimum version           |
  +                    +                                      +                              +
  |                    | or ``tar`` and ``bunzip2``           | No minimum version           |
  |                    | (for Web download)                   |                              |
  +--------------------+--------------------------------------+------------------------------+
  | **Compiler**       | ``g++``                              | >= 9                         |
  +                    +                                      +                              +
  |                    | or ``clang++``                       | >= 10                        |
  +--------------------+--------------------------------------+------------------------------+
  | **Configuration**  | ``python3``                          | >= 3.5                       |
  +--------------------+--------------------------------------+------------------------------+
  | **Build system**   | ``cmake``,                           | >= 3.10                      |
  +                    +                                      +                              +
  |                    | and at least one of:                 | No minimum version           |
  |                    | ``make``, ``ninja``, or ``Xcode``    |                              |
  +--------------------+--------------------------------------+------------------------------+

.. note::

  If you are using an older version of ns-3, other tools may be needed (such as
  ``python2`` instead of ``python3`` and ``Waf`` instead of ``cmake``).  Check the file
  ``RELEASE_NOTES`` in the top-level directory for requirements for older releases.

From the command line, you can check the version of each of the above tools with version
requirements as follows:

  +--------------------------------------+------------------------------------+
  | **Tool**                             | **Version check command**          |
  +======================================+====================================+
  | ``g++``                              | ``$ g++ --version``                |
  +--------------------------------------+------------------------------------+
  | ``clang++``                          | ``$ clang++ --version``            |
  +--------------------------------------+------------------------------------+
  | ``python3``                          | ``$ python3 -V``                   |
  +--------------------------------------+------------------------------------+
  | ``cmake``                            | ``$ cmake --version``              |
  +--------------------------------------+------------------------------------+

Download
********

There are two main options:

1. Download a release tarball.  This will unpack to a directory such as ``ns-allinone-3.42``
containing |ns3| and some other programs.  Below is a command-line download using ``wget``,
but a browser download will also work::

  $ wget https://www.nsnam.org/releases/ns-allinone-3.42.tar.bz2
  $ tar xfj ns-allinone-3.42.tar.bz2
  $ cd ns-allinone-3.42/ns-3.42

2. Clone |ns3| from the Git repository.  The ``ns-3-allinone`` can be cloned, as well as
``ns-3-dev`` by itself.  Below, we illustrate the latter::

  $ git clone https://gitlab.com/nsnam/ns-3-dev.git
  $ cd ns-3-dev

Note that if you select option 1), your directory name will contain the release number.  If
you clone |ns3|, your directory will be named ``ns-3-dev``.  By default, Git will check out
the |ns3| ``master`` branch, which is a development branch.  All |ns3| releases are tagged
in Git, so if you would then like to check out a past release, you can do so as follows::

  $ git checkout -b ns-3.42-release ns-3.42

In this quick-start, we are omitting download and build instructions for optional |ns3| modules,
the ``NetAnim`` animator, Python bindings, and ``NetSimulyzer``.  The
`ns-3 Tutorial <https://www.nsnam.org/docs/tutorial/html/getting-started.html>`_ has some
instructions on optional components, or else the documentation associated with the extension
should be consulted.

Moreover, in this guide we will assume that you are using ns-3.36 or later. Earlier
versions had different configuration, build, and run command and options.

Building and testing ns-3
*************************

Once you have obtained the source either by downloading a release or by cloning a Git repository,
the next step is to configure the build using the *CMake* build system.  The below commands
make use of a Python wrapper around CMake, called ``ns3``, that simplifies the command-line
syntax, resembling *Waf* syntax.  There are several options to control the build, but enabling
the example programs and the tests, for a default build profile (with asserts enabled and
and support for |ns3| logging) is what is usually done at first::

  $ ./ns3 configure --enable-examples --enable-tests

Depending on how fast your CPU is, the configuration command can take anywhere from a few
seconds to a minute.

Then, use the ``ns3`` program to build the |ns3| module libraries and executables::

  $ ./ns3 build

Build times vary based on the number of CPU cores, the speed of the CPU and memory, and the mode
of the build (whether debug mode, which is faster, or the default or optimized modes, which are
slower).  Additional configuration (not covered here) can be used to limit the scope of the
build, and the ``ccache``, if installed, can speed things up.  In general, plan on the build
taking a few minutes on faster workstations.

You should see some output such as below, if successful::

  'build' finished successfully (44.159s)

  Modules built:
  antenna                   aodv                      applications
  bridge                    buildings                 config-store
  core                      csma                      csma-layout
  dsdv                      dsr                       energy
  fd-net-device             flow-monitor              internet
  internet-apps             lr-wpan                   lte
  mesh                      mobility                  mpi
  netanim (no Python)       network                   nix-vector-routing
  olsr                      point-to-point            point-to-point-layout
  propagation               sixlowpan                 spectrum
  stats                     tap-bridge                test (no Python)
  topology-read             traffic-control           uan
  virtual-net-device        visualizer                wifi
  wimax

  Modules not built (see ns-3 tutorial for explanation):
  brite                     click                     openflow


Once complete, you can run the unit tests to check your build::

  $ ./test.py

This command should run several hundred unit tests.  If they pass, you have made a successful
initial build of |ns3|.  Read further in this manual for instructions about building
optional components, or else consult the |ns3| Tutorial or other documentation to get started
with the base |ns3|.

If you prefer to code with an code editor, consult the documentation in the |ns3| Manual
on `Working with CMake <https://www.nsnam.org/docs/manual/html/working-with-cmake.html>`_,
since CMake enables |ns3| integration with a variety of code editors, including:

* JetBrains's CLion
* Microsoft Visual Studio and Visual Studio Code
* Apple's XCode
* CodeBlocks
* Eclipse CDT4

Installing ns-3
***************

Most users do not install |ns3| libraries to typical system library directories; they instead
just leave the libraries in the ``build`` directory, and the ``ns3`` Python program will
find these libraries.  However, it is possible to perform an installation step-- ``ns3 install``--
with the following caveats.

The location of the installed libraries is set by the ``--prefix`` option specified at the
configure step.  The prefix defaults to ``/usr/local``.  For a given ``--prefix=$PREFIX``,
the installation step will install headers to a ``$PREFIX/include`` directory, libraries
and pkgconfig files to a ``$PREFIX/lib`` directory, and a few binaries to a
``$PREFIX/libexec`` directory.  For example, ``./ns3 configure --prefix=/tmp``, followed
by ``./ns3 build`` and ``./ns3 install``, will lead to files being installed in
``/tmp/include``, ``/tmp/lib``, and ``/tmp/libexec``.

Note that the ``ns3`` script prevents running the script as root (or as a sudo user).  As a
result, with the default prefix of ``/usr/local``, the installation will fail unless the
user has write privileges in that directory.  Attempts to force this with
``sudo ./ns3 install`` will fail due to a check in the ``ns3`` program that prevents running
as root.  This check was installed by |ns3| maintainers for the safety of novice users who may
run ``./ns3`` in a root shell and later in a normal shell, and become confused about errors
resulting in lack of privileges to modify files.   For users who know what they are doing and
who want to install to a privileged directory, users can comment out the statement
``refuse_run_as_root()`` in the ``ns3`` program (around line 1400), and then run
``sudo ./ns3 install``.
