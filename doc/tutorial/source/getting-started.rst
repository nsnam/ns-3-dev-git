.. include:: replace.txt
.. highlight:: bash

Getting Started
---------------

This section is aimed at getting a user to a working state starting
with a machine that may never have had |ns3| installed.  It covers
supported platforms, prerequisites, ways to obtain |ns3|, ways to
build |ns3|, and ways to verify your build and run simple programs.

Overview
********

|ns3| is built as a system of software libraries that work together.
User programs can be written that links with (or imports from) these
libraries.  User programs are written in either the C++ or Python 
programming languages.

|ns3| is distributed as source code, meaning that the target system
needs to have a software development environment to build the libraries
first, then build the user program.  |ns3| could in principle be 
distributed as pre-built libraries for selected systems, and in the
future it may be distributed that way, but at present, many users
actually do their work by editing |ns3| itself, so having the source
code around to rebuild the libraries is useful.  If someone would like 
to undertake the job of making pre-built libraries and packages for 
operating systems, please contact the ns-developers mailing list.

In the following, we'll look at three ways of downloading and building
|ns3|.  The first is to download and build an official release
from the main web site.  The second is to fetch and build development
copies of a basic |ns3| installation.  The third is to use an additional
build tool to download more extensions for |ns3|.  We'll walk through each
since the tools involved are slightly different.

Experienced Linux users may wonder at this point why |ns3| is not provided
like most other libraries using a package management tool?  Although there
exist some binary packages for various Linux distributions (e.g. Debian),
most users end up editing and having to rebuild the |ns3| libraries
themselves, so having the source code available is more convenient.
We will therefore focus on a source installation in this tutorial.

For most uses of |ns3|, root permissions are not needed, and the use of
a non-privileged user account is recommended.

Prerequisites
*************

The entire set of available |ns3| libraries has a number of dependencies   
on third-party libraries, but most of |ns3| can be built and used with
support for a few common (often installed by default) components:  a
C++ compiler, an installation of Python, a source code editor (such as vim,
emacs, or Eclipse) and, if using the development repositories, an 
installation of Git source code control system.  Most beginning users
need not concern themselves if their configuration reports some missing
optional features of |ns3|, but for those wishing a full installation,
the project provides a wiki that includes pages with many useful hints
and tips.  One such page is the "Installation" page, with install instructions
for various systems, available at
https://www.nsnam.org/wiki/Installation.

The "Prerequisites" section of this wiki page explains which packages are 
required to support common |ns3| options, and also provides the 
commands used to install them for common Linux or macOS variants.

You may want to take this opportunity to explore the |ns3| wiki 
a bit, or the main web site at https://www.nsnam.org, since there is a 
wealth of information there. 

As of the most recent |ns3| release (ns-3.30), the following tools
are needed to get started with |ns3|:

============  ===========================================================
Prerequisite  Package/version
============  ===========================================================
C++ compiler  ``clang++`` or ``g++`` (g++ version 4.9 or greater)
Python        ``python2`` version >= 2.7.10 and ``python3`` version >=3.4 
Git           any recent version (to access |ns3| from `GitLab.com <https://gitlab.com/nsnam/ns-3-dev/>`_)
tar           any recent version (to unpack an `ns-3 release <https://www.nsnam.org/releases/>`_)
bunzip2       any recent version (to uncompress an |ns3| release)
============  ===========================================================

To check the default version of Python, type ``python -V``.  To check
the default version of g++, type ``g++ -v``.  If your installation is
missing or too old, please consult the |ns3| installation wiki for guidance.

From this point forward, we are going to assume that the reader is working in
Linux, macOS, or a Linux emulation environment, and has at least the above
prerequisites.

Downloading a release of ns-3 as a source archive
+++++++++++++++++++++++++++++++++++++++++++++++++

This option is for the new user who wishes to download and experiment with 
the most recently released and packaged version of |ns3|.
|ns3| publishes its releases as compressed source archives, sometimes
referred to as a tarball.
A tarball is a particular format of software archive where multiple
files are bundled together and the archive is usually compressed.
The process for downloading |ns3| via tarball is simple; you just
have to pick a release, download it and uncompress it.

Let's assume that you, as a user, wish to build |ns3| in a local
directory called ``workspace``. 
If you adopt the ``workspace`` directory approach, you can 
get a copy of a release by typing the following into your Linux shell 
(substitute the appropriate version numbers, of course)

.. sourcecode:: bash

  $ cd
  $ mkdir workspace
  $ cd workspace
  $ wget https://www.nsnam.org/release/ns-allinone-3.30.tar.bz2
  $ tar xjf ns-allinone-3.30.tar.bz2

Notice the use above of the ``wget`` utility, which is a command-line
tool to fetch objects from the web; if you do not have this installed,
you can use a browser for this step.
 
Following these steps, if you change into the directory 
``ns-allinone-3.30``, you should see a number of files and directories

.. sourcecode:: text

  $ cd ns-allinone-3.30
  $ ls
  bake      constants.py   ns-3.30                            README
  build.py  netanim-3.108  pybindgen-0.20.0                   util.py

You are now ready to build the base |ns3| distribution and may skip ahead
to the section on building |ns3|.

Downloading ns-3 using Git
**************************

The |ns3| code is available in Git repositories on the GitLab.com service
at https://gitlab.com/nsnam/.  The group name ``nsnam`` organizes the
various repositories used by the open source project.

The simplest way to get started using Git repositories is to fork or clone
the ``ns-3-allinone`` environment.  This is a set of scripts that manages the 
downloading and building of the most commonly used subsystems of |ns3|
for you.  If you are new to Git, the terminology of ``fork`` and ``clone``
may be foreign to you; if so, we recommend that you simply ``clone``
(create your own replica) of the repository found on GitLab.com, as
follows:

.. sourcecode:: bash

  $ cd
  $ mkdir workspace
  $ cd workspace
  $ git clone https://gitlab.com/nsnam/ns-3-allinone.git
  $ cd ns-3-allinone

At this point, your view of the ns-3-allinone directory is slightly
different than described above with a release archive; it should look
something like this:

.. sourcecode:: bash

  $ ls
  build.py  constants.py   download.py  README  util.py

Note the presence of the ``download.py`` script, which will further fetch
the |ns3| and related sourcecode.  At this point, you have a choice, to
either download the most recent development snapshot of |ns3|:

.. sourcecode:: bash

  $ python download.py

or to specify a release of |ns3|, using the ``-n`` flag to specify a
release number:

.. sourcecode:: bash

  $ python download.py -n ns-3.30

After this step, the additional repositories of |ns3|, bake, pybindgen,
and netanim will be downloaded to the ``ns-3-allinone`` directory.
 
Downloading ns-3 Using Bake
+++++++++++++++++++++++++++

The above two techniques (source archive, or ns-3-allinone repository
via Git) are useful to get the most basic installation of |ns3| with a
few addons (pybindgen for generating Python bindings, and netanim
for network animiations).  The third repository provided by default in
ns-3-allinone is called ``bake``.

Bake is a tool for coordinated software building from multiple repositories,
developed for the |ns3| project.  Bake can be used to fetch development
versions of the |ns3| software, and to download and build extensions to the 
base |ns3| distribution, such as the Direct Code Execution environment,
Network Simulation Cradle, ability to create new Python bindings, and
various |ns3| "apps".  If you envision that your |ns3| installation may
use advanced or optional features, you may wish to follow this installation
path.

In recent |ns3| releases, Bake has been included in the release
tarball.  The configuration file included in the released version
will allow one to download any software that was current at the
time of the release.  That is, for example, the version of Bake that
is distributed with the ``ns-3.30`` release can be used to fetch components
for that |ns3| release or earlier, but can't be used to fetch components
for later releases (unless the ``bakeconf.xml`` package description file 
is updated).

You can also get the most recent copy of ``bake`` by typing the 
following into your Linux shell (assuming you have installed Git)::

  $ cd
  $ mkdir workspace
  $ cd workspace
  $ git clone https://gitlab.com/nsnam/bake.git

As the git command executes, you should see something like the 
following displayed:

.. sourcecode:: bash

  Cloning into 'bake'...
  remote: Enumerating objects: 2086, done.
  remote: Counting objects: 100% (2086/2086), done.
  remote: Compressing objects: 100% (649/649), done.
  remote: Total 2086 (delta 1404), reused 2078 (delta 1399)
  Receiving objects: 100% (2086/2086), 2.68 MiB | 3.82 MiB/s, done.
  Resolving deltas: 100% (1404/1404), done.

After the clone command completes, you should have a directory called 
``bake``, the contents of which should look something like the following:

.. sourcecode:: bash

  $ cd bake
  $ ls
  bake  bakeconf.xml  bake.py  doc  examples  generate-binary.py  test  TODO

Notice that you have downloaded some Python scripts, a Python
module called ``bake``, and an XML configuration file.  The next step
will be to use those scripts to download and build the |ns3|
distribution of your choice.

There are a few configuration targets available:

1.  ``ns-3.30``:  the module corresponding to the release; it will download
    components similar to the release tarball.
2.  ``ns-3-dev``:  a similar module but using the development code tree
3.  ``ns-allinone-3.30``:  the module that includes other optional features
    such as Click routing, Openflow for |ns3|, and the Network Simulation
    Cradle
4.  ``ns-3-allinone``:  similar to the released version of the allinone
    module, but for development code.

The current development snapshot (unreleased) of |ns3| may be found 
at https://gitlab.com/nsnam/ns-3-dev.git.  The 
developers attempt to keep these repositories in consistent, working states but
they are in a development area with unreleased code present, so you may want 
to consider staying with an official release if you do not need newly-
introduced features.

You can find the latest version  of the
code either by inspection of the repository list or by going to the 
`"ns-3 Releases"
<https://www.nsnam.org/releases>`_
web page and clicking on the latest release link.  We'll proceed in
this tutorial example with ``ns-3.30``.

We are now going to use the bake tool to pull down the various pieces of 
|ns3| you will be using.  First, we'll say a word about running bake.

bake works by downloading source packages into a source directory,
and installing libraries into a build directory.  bake can be run
by referencing the binary, but if one chooses to run bake from
outside of the directory it was downloaded into, it is advisable
to put bake into your path, such as follows (Linux bash shell example).
First, change into the 'bake' directory, and then set the following
environment variables:
 
.. sourcecode:: bash
 
  $ export BAKE_HOME=`pwd`
  $ export PATH=$PATH:$BAKE_HOME:$BAKE_HOME/build/bin
  $ export PYTHONPATH=$PYTHONPATH:$BAKE_HOME:$BAKE_HOME/build/lib

This will put the bake.py program into the shell's path, and will allow
other programs to find executables and libraries created by bake.  Although
several bake use cases do not require setting PATH and PYTHONPATH as above,
full builds of ns-3-allinone (with the optional packages) typically do.

Step into the workspace directory and type the following into your shell:

.. sourcecode:: bash

  $ ./bake.py configure -e ns-3.30

Next, we'll ask bake to check whether we have enough tools to download
various components.  Type:

.. sourcecode:: bash

  $ ./bake.py check

You should see something like the following:

.. sourcecode:: text

  > Python - OK
  > GNU C++ compiler - OK
  > Mercurial - OK
  > Git - OK
  > Tar tool - OK
  > Unzip tool - OK
  > Make - OK
  > cMake - OK
  > patch tool - OK
  > Path searched for tools: /usr/local/sbin /usr/local/bin /usr/sbin /usr/bin /sbin /bin ...

In particular, download tools such as Mercurial, CVS, Git, and Bazaar
are our principal concerns at this point, since they allow us to fetch
the code.  Please install missing tools at this stage, in the usual
way for your system (if you are able to), or contact your system 
administrator as needed to install these tools.

Next, try to download the software:

.. sourcecode:: bash

  $ ./bake.py download

should yield something like:

.. sourcecode:: text

  >> Searching for system dependency setuptools - OK
  >> Searching for system dependency libgoocanvas2 - OK
  >> Searching for system dependency gi-cairo - OK
  >> Searching for system dependency pygobject - OK
  >> Searching for system dependency pygraphviz - OK
  >> Searching for system dependency python-dev - OK
  >> Searching for system dependency qt - OK
  >> Searching for system dependency g++ - OK
  >> Downloading pybindgen-0.20.0 (target directory:pybindgen) - OK
  >> Downloading netanim-3.108 - OK
  >> Downloading ns-3.30 - OK

The above suggests that three sources have been downloaded.  Check the
``source`` directory now and type ``ls``; one should see:

.. sourcecode:: bash

  $ cd source
  $ ls
  netanim-3.108  ns-3.30  pybindgen

You are now ready to build the |ns3| distribution.

Building ns-3
*************

As with downloading |ns3|, there are a few ways to build |ns3|.  The main
thing that we wish to emphasize is the following.  |ns3| is built with
a build tool called ``Waf``, described below.  Most users will end up
working most directly with Waf, but there are some convenience scripts
to get you started or to orchestrate more complicated builds.  Therefore,
please have a look at ``build.py`` and building with ``bake``, before
reading about Waf below.

Building with ``build.py``
++++++++++++++++++++++++++

**Note:** This build step is only available from a source archive release
described above; not from downloading via git or bake.

When working from a released tarball, a convenience script available as
part of ``ns-3-allinone`` can orchestrate a simple build of components.
This program is called ``build.py``.  This 
program will get the project configured for you
in the most commonly useful way.  However, please note that more advanced
configuration and work with |ns3| will typically involve using the
native |ns3| build system, Waf, to be introduced later in this tutorial.  

If you downloaded
using a tarball you should have a directory called something like 
``ns-allinone-3.30`` under your ``~/workspace`` directory.  
Type the following:

.. sourcecode:: bash

  $ ./build.py --enable-examples --enable-tests

Because we are working with examples and tests in this tutorial, and
because they are not built by default in |ns3|, the arguments for
build.py tells it to build them for us.  The program also defaults to
building all available modules.  Later, you can build
|ns3| without examples and tests, or eliminate the modules that
are not necessary for your work, if you wish.

You will see lots of compiler output messages displayed as the build
script builds the various pieces you downloaded.  First, the script will
attempt to build the netanim animator, then the pybindgen bindings generator,
and finally |ns3|.  Eventually you should see the following::

   Waf: Leaving directory '/path/to/workspace/ns-allinone-3.30/ns-3.30/build'
   'build' finished successfully (6m25.032s)
  
   Modules built:
   antenna                aodv                      applications              
   bridge                 buildings                 config-store              
   core                   csma                      csma-layout               
   dsdv                   dsr                       energy                    
   fd-net-device          flow-monitor              internet                  
   internet-apps          lr-wpan                   lte                       
   mesh                   mobility                  mpi                       
   netanim (no Python)    network                   nix-vector-routing        
   olsr                   point-to-point            point-to-point-layout     
   propagation            sixlowpan                 spectrum                  
   stats                  tap-bridge                test (no Python)          
   topology-read          traffic-control           uan                       
   virtual-net-device     visualizer                wave                      
   wifi                   wimax                     

   Modules not built (see ns-3 tutorial for explanation):
   brite                  click                     openflow                  

   Leaving directory ./ns-3.30

Regarding the portion about modules not built::

  Modules not built (see ns-3 tutorial for explanation):
  brite                     click                     

This just means that some |ns3| modules that have dependencies on
outside libraries may not have been built, or that the configuration
specifically asked not to build them.  It does not mean that the 
simulator did not build successfully or that it will provide wrong 
results for the modules listed as being built.

Building with bake
++++++++++++++++++

If you used bake above to fetch source code from project repositories, you
may continue to use it to build |ns3|.  Type: 

.. sourcecode:: bash

  $ ./bake.py build

and you should see something like:

.. sourcecode:: text

  >> Building pybindgen-0.20.0 - OK
  >> Building netanim-3.108 - OK
  >> Building ns-3.30 - OK

*Hint:  you can also perform both steps, download and build, by calling ``bake.py deploy``.*

There may be failures to build all components, but the build will proceed
anyway if the component is optional.  For example, a recent portability issue
has been that castxml may not build via the bake build tool on all 
platforms; in this case, the line will show something like::

  >> Building castxml - Problem
  > Problem: Optional dependency, module "castxml" failed
    This may reduce the  functionality of the final build. 
    However, bake will continue since "castxml" is not an essential dependency.
    For more information call bake with -v or -vvv, for full verbose mode.

However, castxml is only needed if one wants to generate updated Python
bindings, and most users do not need to do so (or to do so until they are
more involved with ns-3 changes), so such warnings might be safely ignored
for now.

If there happens to be a failure, please have a look at what the following
command tells you; it may give a hint as to a missing dependency:

.. sourcecode:: bash

  $ ./bake.py show

This will list out the various dependencies of the packages you are
trying to build.

Building with Waf
+++++++++++++++++

Up to this point, we have used either the `build.py` script, or the
`bake` tool, to get started with building |ns3|.  These tools are useful
for building |ns3| and supporting libraries, and they call into
the |ns3| directory to call the Waf build tool to do the actual building.  
An installation of Waf is bundled with the |ns3| source code.
Most users quickly transition to using Waf directly to configure and 
build |ns3|.  So, to proceed, please change your working directory to 
the |ns3| directory that you have initially built.

It's not 
strictly required at this point, but it will be valuable to take a slight
detour and look at how to make changes to the configuration of the project.
Probably the most useful configuration change you can make will be to 
build the optimized version of the code.  By default you have configured
your project to build the debug version.  Let's tell the project to 
make an optimized build.  To explain to Waf that it should do optimized
builds that include the examples and tests, you will need to execute the 
following commands:

.. sourcecode:: bash

  $ ./waf clean
  $ ./waf configure --build-profile=optimized --enable-examples --enable-tests

This runs Waf out of the local directory (which is provided as a convenience
for you).  The first command to clean out the previous build is not 
typically strictly necessary but is good practice (but see `Build Profiles`_,
below); it will remove the
previously built libraries and object files found in directory ``build/``. 
When the project is reconfigured and the build system checks for various 
dependencies, you should see
output that looks similar to the following::

   Setting top to                           : /home/ns3user/workspace/bake/source/ns-3-dev 
   Setting out to                           : /home/ns3user/workspace/bake/source/ns-3-dev/build 
   Checking for 'gcc' (C compiler)          : /usr/bin/gcc 
   Checking for cc version                  : 7.3.0 
   Checking for 'g++' (C++ compiler)        : /usr/bin/g++ 
   Checking for compilation flag -march=native support : ok 
   Checking for compilation flag -Wl,--soname=foo support : ok 
   Checking for compilation flag -std=c++11 support       : ok 
   Checking boost includes                                : headers not found, please provide a --boost-includes argument (see help) 
   Checking boost includes                                : headers not found, please provide a --boost-includes argument (see help) 
   Checking for program 'python'                          : /usr/bin/python 
   Checking for python version >= 2.3                     : 2.7.15 
   python-config                                          : /usr/bin/python-config 
   Asking python-config for pyembed '--cflags --libs --ldflags' flags : yes 
   Testing pyembed configuration                                      : yes 
   Asking python-config for pyext '--cflags --libs --ldflags' flags   : yes 
   Testing pyext configuration                                        : yes 
   Checking for compilation flag -fvisibility=hidden support          : ok 
   Checking for compilation flag -Wno-array-bounds support            : ok 
   Checking for pybindgen location                                    : ../pybindgen (guessed) 
   Checking for python module 'pybindgen'                             : 0.20.0 
   Checking for pybindgen version                                     : 0.20.0 
   Checking for code snippet                                          : yes 
   Checking for types uint64_t and unsigned long equivalence          : no 
   Checking for code snippet                                          : no 
   Checking for types uint64_t and unsigned long long equivalence     : yes 
   Checking for the apidefs that can be used for Python bindings      : gcc-LP64 
   Checking for internal GCC cxxabi                                   : complete 
   Checking for python module 'pygccxml'                              : not found 
   Checking for click location                                        : not found 
   Checking for program 'pkg-config'                                  : /usr/bin/pkg-config 
   Checking for 'gtk+-3.0'                                            : not found 
   Checking for 'libxml-2.0'                                          : yes 
   checking for uint128_t                                             : not found 
   checking for __uint128_t                                           : yes 
   Checking high precision implementation                             : 128-bit integer (default) 
   Checking for header stdint.h                                       : yes 
   Checking for header inttypes.h                                     : yes 
   Checking for header sys/inttypes.h                                 : not found 
   Checking for header sys/types.h                                    : yes 
   Checking for header sys/stat.h                                     : yes 
   Checking for header dirent.h                                       : yes 
   Checking for header stdlib.h                                       : yes 
   Checking for header signal.h                                       : yes 
   Checking for header pthread.h                                      : yes 
   Checking for header stdint.h                                       : yes 
   Checking for header inttypes.h                                     : yes 
   Checking for header sys/inttypes.h                                 : not found 
   Checking for library rt                                            : yes 
   Checking for header sys/ioctl.h                                    : yes 
   Checking for header net/if.h                                       : yes 
   Checking for header net/ethernet.h                                 : yes 
   Checking for header linux/if_tun.h                                 : yes 
   Checking for header netpacket/packet.h                             : yes 
   Checking for NSC location                                          : not found 
   Checking for 'sqlite3'                                             : not found 
   Checking for header linux/if_tun.h                                 : yes 
   Checking for python module 'gi'                                    : 3.26.1 
   Checking for python module 'gi.repository.GObject'                 : ok 
   Checking for python module 'cairo'                                 : ok 
   Checking for python module 'pygraphviz'                            : 1.4rc1 
   Checking for python module 'gi.repository.Gtk'                     : ok 
   Checking for python module 'gi.repository.Gdk'                     : ok 
   Checking for python module 'gi.repository.Pango'                   : ok 
   Checking for python module 'gi.repository.GooCanvas'               : ok 
   Checking for program 'sudo'                                        : /usr/bin/sudo 
   Checking for program 'valgrind'                                    : not found 
   Checking for 'gsl'                                                 : not found 
   python-config                                                      : not found 
   Checking for compilation flag -fstrict-aliasing support            : ok 
   Checking for compilation flag -fstrict-aliasing support            : ok 
   Checking for compilation flag -Wstrict-aliasing support            : ok 
   Checking for compilation flag -Wstrict-aliasing support            : ok 
   Checking for program 'doxygen'                                     : /usr/bin/doxygen 
   ---- Summary of optional NS-3 features:
   Build profile                 : optimized
   Build directory               : 
   BRITE Integration             : not enabled (BRITE not enabled (see option --with-brite))
   DES Metrics event collection  : not enabled (defaults to disabled)
   Emulation FdNetDevice         : enabled
   Examples                      : enabled
   File descriptor NetDevice     : enabled
   GNU Scientific Library (GSL)  : not enabled (GSL not found)
   Gcrypt library                : not enabled (libgcrypt not found: you can use libgcrypt-config to find its location.)
   GtkConfigStore                : not enabled (library 'gtk+-3.0 >= 3.0' not found)
   MPI Support                   : not enabled (option --enable-mpi not selected)
   NS-3 Click Integration        : not enabled (nsclick not enabled (see option --with-nsclick))
   NS-3 OpenFlow Integration     : not enabled (Required boost libraries not found)
   Network Simulation Cradle     : not enabled (NSC not found (see option --with-nsc))
   PlanetLab FdNetDevice         : not enabled (PlanetLab operating system not detected (see option --force-planetlab))
   PyViz visualizer              : enabled
   Python API Scanning Support   : not enabled (Missing 'pygccxml' Python module)
   Python Bindings               : enabled
   Real Time Simulator           : enabled
   SQlite stats data output      : not enabled (library 'sqlite3' not found)
   Tap Bridge                    : enabled
   Tap FdNetDevice               : enabled
   Tests                         : enabled
   Threading Primitives          : enabled
   Use sudo to set suid bit      : not enabled (option --enable-sudo not selected)
   XmlIo                         : enabled
   'configure' finished successfully (6.387s)

Note the last part of the above output.  Some |ns3| options are not enabled by
default or require support from the underlying system to work properly.
For instance, to enable XmlTo, the library libxml-2.0 must be found on the
system.  If this library were not found, the corresponding |ns3| feature 
would not be enabled and a message would be displayed.  Note further that there is 
a feature to use the program ``sudo`` to set the suid bit of certain programs.
This is not enabled by default and so this feature is reported as "not enabled."
Finally, to reprint this summary of which optional features are enabled, use
the ``--check-config`` option to waf.

Now go ahead and switch back to the debug build that includes the examples and tests.

.. sourcecode:: bash

  $ ./waf clean
  $ ./waf configure --build-profile=debug --enable-examples --enable-tests

The build system is now configured and you can build the debug versions of 
the |ns3| programs by simply typing:

.. sourcecode:: bash

  $ ./waf

Although the above steps made you build the |ns3| part of the system twice,
now you know how to change the configuration and build optimized code.

A command exists for checking which profile is currently active
for an already configured project:

.. sourcecode:: bash

  $ ./waf --check-profile
  Waf: Entering directory \`/path/to/ns-3-allinone/ns-3.30/build\'
  Build profile: debug

The build.py script discussed above supports also the ``--enable-examples``
and ``enable-tests`` arguments, but in general, does not directly support
other waf options; for example, this will not work:

.. sourcecode:: bash

  $ ./build.py --disable-python

will result in:

.. sourcecode:: bash

  build.py: error: no such option: --disable-python

However, the special operator ``--`` can be used to pass additional
options through to waf, so instead of the above, the following will work:

.. sourcecode:: bash

  $ ./build.py -- --disable-python   

as it generates the underlying command ``./waf configure --disable-python``.

Here are a few more introductory tips about Waf.

Handling build errors
=====================

|ns3| releases are tested against the most recent C++ compilers available
in the mainstream Linux and macOS distributions at the time of the release.
However, over time, newer distributions are released, with newer compilers,
and these newer compilers tend to be more pedantic about warnings.  |ns3|
configures its build to treat all warnings as errors, so it is sometimes
the case, if you are using an older release version on a newer system,
that a compiler warning will cause the build to fail.

For instance, ns-3.28 was released prior to Fedora 28, which included
a new major version of gcc (gcc-8).  Building ns-3.28 or older releases
on Fedora 28, when Gtk2+ is installed, will result in an error such as::

  /usr/include/gtk-2.0/gtk/gtkfilechooserbutton.h:59:8: error: unnecessary parentheses in declaration of ‘__gtk_reserved1’ [-Werror=parentheses]
   void (*__gtk_reserved1);

In releases starting with ns-3.28.1, an option is available in Waf to work
around these issues.  The option disables the inclusion of the '-Werror' 
flag to g++ and clang++.  The option is '--disable-werror' and must be
used at configure time; e.g.:

.. sourcecode:: bash

  ./waf configure --disable-werror --enable-examples --enable-tests

Configure vs. Build
===================

Some Waf commands are only meaningful during the configure phase and some commands are valid
in the build phase.  For example, if you wanted to use the emulation 
features of |ns3|, you might want to enable setting the suid bit using
sudo as described above.  This turns out to be a configuration-time command, and so 
you could reconfigure using the following command that also includes the examples and tests.

.. sourcecode:: bash

  $ ./waf configure --enable-sudo --enable-examples --enable-tests

If you do this, Waf will have run sudo to change the socket creator programs of the
emulation code to run as root.

There are many other configure- and build-time options
available in Waf.  To explore these options, type:

.. sourcecode:: bash

  $ ./waf --help

We'll use some of the testing-related commands in the next section.

Build Profiles
==============

We already saw how you can configure Waf for ``debug`` or ``optimized`` builds:

.. sourcecode:: bash

  $ ./waf --build-profile=debug

There is also an intermediate build profile, ``release``.  ``-d`` is a
synonym for ``--build-profile``.

The build profile controls the use of logging, assertions, and compiler optimization:

.. table:: Build profiles
    :widths: 10 28 30 32

    +----------+---------------------------------+-----------------------------------------------------------------+
    | Feature  | Build Profile                                                                                     |
    +          +---------------------------------+-------------------------------+---------------------------------+
    |          | ``debug``                       | ``release``                   | ``optimized``                   |
    +==========+=================================+===============================+=================================+
    | Enabled  | |  ``NS3_BUILD_PROFILE_DEBUG``  | ``NS3_BUILD_PROFILE_RELEASE`` | ``NS3_BUILD_PROFILE_OPTIMIZED`` |
    | Features | |  ``NS_LOG...``                |                               |                                 |
    |          | |  ``NS_ASSERT...``             |                               |                                 |
    +----------+---------------------------------+-------------------------------+---------------------------------+
    | Code     | ``NS_BUILD_DEBUG(code)``        |  ``NS_BUILD_RELEASE(code)``   | ``NS_BUILD_OPTIMIZED(code)``    |
    | Wrapper  |                                 |                               |                                 |
    | Macro    |                                 |                               |                                 |
    +----------+---------------------------------+-------------------------------+---------------------------------+
    | Compiler | ``-O0 -ggdb -g3``               | ``-O3 -g0``                   | ``-O3 -g``                      |
    | Flags    |                                 | ``-fomit-frame-pointer``      | ``-fstrict-overflow``           |
    |          |                                 |                               | ``-march=native``               |
    +----------+---------------------------------+-------------------------------+---------------------------------+
    
As you can see, logging and assertions are only available in debug builds.
Recommended practice is to develop your scenario in debug mode, then
conduct repetitive runs (for statistics or changing parameters) in
optimized build profile.

If you have code that should only run in specific build profiles,
use the indicated Code Wrapper macro:

.. sourcecode:: cpp

  NS_BUILD_DEBUG (std::cout << "Part of an output line..." << std::flush; timer.Start ());
  DoLongInvolvedComputation ();
  NS_BUILD_DEBUG (timer.Stop (); std::cout << "Done: " << timer << std::endl;)

By default Waf puts the build artifacts in the ``build`` directory.  
You can specify a different output directory with the ``--out``
option, e.g.

.. sourcecode:: bash

  $ ./waf configure --out=my-build-dir

Combining this with build profiles lets you switch between the different
compile options in a clean way:

.. sourcecode:: bash

  $ ./waf configure --build-profile=debug --out=build/debug
  $ ./waf build
  ...
  $ ./waf configure --build-profile=optimized --out=build/optimized
  $ ./waf build
  ...

This allows you to work with multiple builds rather than always
overwriting the last build.  When you switch, Waf will only compile
what it has to, instead of recompiling everything.

When you do switch build profiles like this, you have to be careful
to give the same configuration parameters each time.  It may be convenient
to define some environment variables to help you avoid mistakes:

.. sourcecode:: bash

  $ export NS3CONFIG="--enable-examples --enable-tests"
  $ export NS3DEBUG="--build-profile=debug --out=build/debug"
  $ export NS3OPT=="--build-profile=optimized --out=build/optimized"

  $ ./waf configure $NS3CONFIG $NS3DEBUG
  $ ./waf build
  ...
  $ ./waf configure $NS3CONFIG $NS3OPT
  $ ./waf build

Compilers and Flags
===================

In the examples above, Waf uses the GCC C++ compiler, ``g++``, for
building |ns3|. However, it's possible to change the C++ compiler used by Waf
by defining the ``CXX`` environment variable.
For example, to use the Clang C++ compiler, ``clang++``,

.. sourcecode:: bash

  $ CXX="clang++" ./waf configure
  $ ./waf build

One can also set up Waf to do distributed compilation with ``distcc`` in
a similar way:

.. sourcecode:: bash

  $ CXX="distcc g++" ./waf configure
  $ ./waf build

More info on ``distcc`` and distributed compilation can be found on it's
`project page
<https://code.google.com/p/distcc/>`_
under Documentation section.

To add compiler flags, use the ``CXXFLAGS_EXTRA`` environment variable when
you configure |ns3|.

Install
=======

Waf may be used to install libraries in various places on the system.
The default location where libraries and executables are built is
in the ``build`` directory, and because Waf knows the location of these
libraries and executables, it is not necessary to install the libraries
elsewhere.

If users choose to install things outside of the build directory, users
may issue the ``./waf install`` command.  By default, the prefix for
installation is ``/usr/local``, so ``./waf install`` will install programs
into ``/usr/local/bin``, libraries into ``/usr/local/lib``, and headers
into ``/usr/local/include``.  Superuser privileges are typically needed
to install to the default prefix, so the typical command would be
``sudo ./waf install``.  When running programs with Waf, Waf will
first prefer to use shared libraries in the build directory, then 
will look for libraries in the library path configured in the local
environment.  So when installing libraries to the system, it is good
practice to check that the intended libraries are being used.

Users may choose to install to a different prefix by passing the ``--prefix``
option at configure time, such as::

  ./waf configure --prefix=/opt/local

If later after the build the user issues the ``./waf install`` command, the 
prefix ``/opt/local`` will be used.

The ``./waf clean`` command should be used prior to reconfiguring 
the project if Waf will be used to install things at a different prefix.

In summary, it is not necessary to call ``./waf install`` to use |ns3|.
Most users will not need this command since Waf will pick up the
current libraries from the ``build`` directory, but some users may find 
it useful if their use case involves working with programs outside
of the |ns3| directory.

One Waf
=======

There is only one Waf script, at the top level of the |ns3| source tree.
As you work, you may find yourself spending a lot of time in ``scratch/``,
or deep in ``src/...``, and needing to invoke Waf.  You could just
remember where you are, and invoke Waf like this:

.. sourcecode:: bash

  $ ../../../waf ...

but that gets tedious, and error prone, and there are better solutions.

One common way when using a text-based editor such as emacs or vim is to 
open two terminal sessions and use one to build |ns3| and one to 
edit source code.

If you only have the tarball, an environment variable can help:

.. sourcecode:: bash

  $ export NS3DIR="$PWD"
  $ function waff { cd $NS3DIR && ./waf $* ; }

  $ cd scratch
  $ waff build

It might be tempting in a module directory to add a trivial ``waf``
script along the lines of ``exec ../../waf``.  Please don't.  It's
confusing to newcomers, and when done poorly it leads to subtle build
errors.  The solutions above are the way to go.


Testing ns-3
************

You can run the unit tests of the |ns3| distribution by running the 
``./test.py`` script:

.. sourcecode:: bash

  $ ./test.py

These tests are run in parallel by Waf. You should eventually
see a report saying that

.. sourcecode:: text

  92 of 92 tests passed (92 passed, 0 failed, 0 crashed, 0 valgrind errors)

This is the important message to check for; failures, crashes, or valgrind
errors indicate problems with the code or incompatibilities between the
tools and the code.

You will also see the summary output from Waf and the test runner
executing each test, which will actually look something like::

  Waf: Entering directory `/path/to/workspace/ns-3-allinone/ns-3-dev/build'
  Waf: Leaving directory `/path/to/workspace/ns-3-allinone/ns-3-dev/build'
  'build' finished successfully (1.799s)
  
  Modules built: 
  aodv                      applications              bridge
  click                     config-store              core
  csma                      csma-layout               dsdv
  emu                       energy                    flow-monitor
  internet                  lte                       mesh
  mobility                  mpi                       netanim
  network                   nix-vector-routing        ns3tcp
  ns3wifi                   olsr                      openflow
  point-to-point            point-to-point-layout     propagation
  spectrum                  stats                     tap-bridge
  template                  test                      tools
  topology-read             uan                       virtual-net-device
  visualizer                wifi                      wimax

  PASS: TestSuite wifi-interference
  PASS: TestSuite histogram

  ...

  PASS: TestSuite object
  PASS: TestSuite random-number-generators
  92 of 92 tests passed (92 passed, 0 failed, 0 crashed, 0 valgrind errors)

This command is typically run by users to quickly verify that an 
|ns3| distribution has built correctly.  (Note the order of the ``PASS: ...``
lines can vary, which is okay.  What's important is that the summary line at
the end report that all tests passed; none failed or crashed.)

Both Waf and ``test.py`` will split up the job on the available CPU cores
of the machine, in parallel.

Running a Script
****************

We typically run scripts under the control of Waf.  This allows the build 
system to ensure that the shared library paths are set correctly and that
the libraries are available at run time.  To run a program, simply use the
``--run`` option in Waf.  Let's run the |ns3| equivalent of the
ubiquitous hello world program by typing the following:

.. sourcecode:: bash

  $ ./waf --run hello-simulator

Waf first checks to make sure that the program is built correctly and 
executes a build if required.  Waf then executes the program, which 
produces the following output.

.. sourcecode:: text

  Hello Simulator

Congratulations!  You are now an ns-3 user!

**What do I do if I don't see the output?**

If you see Waf messages indicating that the build was 
completed successfully, but do not see the "Hello Simulator" output, 
chances are that you have switched your build mode to ``optimized`` in 
the `Building with Waf`_ section, but have missed the change back to 
``debug`` mode.  All of the console output used in this tutorial uses a 
special |ns3| logging component that is useful for printing 
user messages to the console.  Output from this component is 
automatically disabled when you compile optimized code -- it is 
"optimized out."  If you don't see the "Hello Simulator" output,
type the following:

.. sourcecode:: bash

  $ ./waf configure --build-profile=debug --enable-examples --enable-tests

to tell Waf to build the debug versions of the |ns3| 
programs that includes the examples and tests.  You must still build 
the actual debug version of the code by typing

.. sourcecode:: bash

  $ ./waf

Now, if you run the ``hello-simulator`` program, you should see the 
expected output.

Program Arguments
+++++++++++++++++

To feed command line arguments to an |ns3| program use this pattern:

.. sourcecode:: bash

  $ ./waf --run <ns3-program> --command-template="%s <args>"

Substitute your program name for ``<ns3-program>``, and the arguments
for ``<args>``.  The ``--command-template`` argument to Waf is
basically a recipe for constructing the actual command line Waf should use
to execute the program.  Waf checks that the build is complete,
sets the shared library paths, then invokes the executable
using the provided command line template, 
inserting the program name for the ``%s`` placeholder.

If you find the above to be syntactically complicated, a simpler variant
exists, which is to include the |ns3| program and its arguments enclosed
by single quotes, such as:

.. sourcecode:: bash

  $ ./waf --run '<ns3-program> --arg1=value1 --arg2=value2 ...'

Another particularly useful example is to run a test suite by itself.
Let's assume that a ``mytest`` test suite exists (it doesn't).
Above, we used the ``./test.py`` script to run a whole slew of
tests in parallel, by repeatedly invoking the real testing program,
``test-runner``.  To invoke ``test-runner`` directly for a single test:

.. sourcecode:: bash

  $ ./waf --run test-runner --command-template="%s --suite=mytest --verbose"

This passes the arguments to the ``test-runner`` program.
Since ``mytest`` does not exist, an error message will be generated.
To print the available ``test-runner`` options:

.. sourcecode:: bash

  $ ./waf --run test-runner --command-template="%s --help"

Debugging
+++++++++

To run |ns3| programs under the control of another utility, such as
a debugger (*e.g.* ``gdb``) or memory checker (*e.g.* ``valgrind``),
you use a similar ``--command-template="..."`` form.

For example, to run your |ns3| program ``hello-simulator`` with the arguments
``<args>`` under the ``gdb`` debugger:

.. sourcecode:: bash

  $ ./waf --run=hello-simulator --command-template="gdb %s --args <args>"

Notice that the |ns3| program name goes with the ``--run`` argument,
and the control utility (here ``gdb``) is the first token
in the ``--command-template`` argument.  The ``--args`` tells ``gdb``
that the remainder of the command line belongs to the "inferior" program.
(Some ``gdb``'s don't understand the ``--args`` feature.  In this case,
omit the program arguments from the ``--command-template``,
and use the ``gdb`` command ``set args``.)

We can combine this recipe and the previous one to run a test under the
debugger:

.. sourcecode:: bash

  $ ./waf --run test-runner --command-template="gdb %s --args --suite=mytest --verbose"

Working Directory
+++++++++++++++++

Waf needs to run from its location at the top of the |ns3| tree.
This becomes the working directory where output files will be written.
But what if you want to keep those files out of the |ns3| source tree?  Use
the ``--cwd`` argument:

.. sourcecode:: bash

  $ ./waf --cwd=...

It may be more convenient to start with your working directory where
you want the output files, in which case a little indirection can help:

.. sourcecode:: bash

  $ function waff {
      CWD="$PWD"
      cd $NS3DIR >/dev/null
      ./waf --cwd="$CWD" $*
      cd - >/dev/null
    }

This embellishment of the previous version saves the current working directory,
``cd``'s to the Waf directory, then instructs Waf to change the working
directory *back* to the saved current working directory before running the
program.

We mention this ``--cwd`` command for completeness; most users will simply
run Waf from the top-level directory and generate the output data files there.

Running without Building
++++++++++++++++++++++++

As of the ns-3.30 release, a new Waf option was introduced to allow the
running of programs while skipping the build step.  This can reduce the time
to run programs when, for example, running the same program repeatedly
through a shell script, or when demonstrating program execution. 
This option, ``--run-no-build``, behaves the same as the ``-run`` option, 
except that the program and ns-3 libraries will not be rebuilt.

.. sourcecode:: bash

  $ ./waf --run-no-build '<ns3-program> --arg1=value1 --arg2=value2 ...'
