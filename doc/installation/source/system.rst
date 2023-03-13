.. include:: replace.txt
.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

.. _General list:

System Prerequisites
--------------------

This chapter describes the various required, recommended, and optional system prerequisites
for installing and using |ns3|.  Optional prerequisites depend on whether an optional feature of
|ns3| is desired by the user.  The chapter is written to be generally applicable to all operating
systems, and subsequent chapters describe the specific packages for different operating systems.

The list of requirements depends on which version of ns-3 you are trying to build, and on which
extensions you need.

.. note::
   **"Do I need to install all of these packages?"**  Some users want to install everything so
   that their configuration output shows that every feature is enabled.  However, there is
   no real need to install prerequisites related to features you are not yet using; you can always
   come back later and install more prerequisites as needed.  The build system should warn
   you if you are missing a prerequisite.

In the following, we have classified the prerequisites as either being required, recommended
for all users, or optional depending on use cases.

.. note::
   **"Is there a maintained list of all prerequisites?"**  We use GitLab.com's continuous
   integration system for testing; the configuration YAML files for these jobs can be found
   in the directory ``utils/tests/``.  So, for instance, if you want to look at what packages
   the CI is installing for Alpine Linux, look at ``utils/tests/gitlab-ci-alpine.yml``.  The
   default CI (Arch Linux) ``pacman`` commands are in ``utils/tests/gitlab-ci.yml``.

Requirements
************

Minimal requirements for release 3.36 and later
===============================================

A C++ compiler (``g++`` or ``clang++``), Python 3, the `CMake <https://cmake.org>`_ build
system, and a separate C++ building tool such as ``make``, ``ninja-build``, or ``Xcode`` are
the minimal requirements for compiling the software.

The ``tar`` and ``bunzip2`` utilities are needed to unpack source file archives.
If you want to instead use `Git <https://git-scm.com/>`_ to fetch code, rather than downloading
a source archive, then ``git`` is required instead.

Minimal requirements for release 3.30-3.35
==========================================

If you are not using Python bindings, since the Waf build system is provided as part of |ns3|,
there are only two build requirements (a C++ compiler, and Python3) for a minimal install of these
older |ns3| releases.

The ``tar`` and ``bunzip2`` utilities are needed to unpack source file archives.
If you want to instead use `Git <https://git-scm.com/>`_ to fetch code, rather than downloading
a source archive, then ``git`` is required instead.

Minimal requirements for release 3.29 and earlier
=================================================

Similarly, only a C++ compiler and Python2 were strictly required for building the |ns3|
releases 3.29 and earlier.

The ``tar`` and ``bunzip2`` utilities are needed to unpack source file archives.
If you want to instead use `Git <https://git-scm.com/>`_ to fetch code, rather than downloading
a source archive, then ``git`` is required instead.

Recommended
***********

The following are recommended for most users of |ns3|.

compiler cache optimization (for ns-3.37 and later)
===================================================

`Ccache <https://ccache.dev>`_ is a compiler cache optimization that will speed up builds across
multiple |ns3| directories, at the cost of up to an extra 5 GB of disk space used in the cache.

Code linting
============

Since ns-3.37 release, `Clang-Format <https://clang.llvm.org/docs/ClangFormat.html>`_ and
`Clang-Tidy <https://clang.llvm.org/extra/clang-tidy/>`_ are used to enforce the coding-style
adopted by |ns3|.

Users can invoke these tools directly from the command-line or through the
(``utils/check-style-clang-format.py``) check program.
Moreover, clang-tidy is integrated with CMake, enabling code scanning during the build phase.

.. note::
  clang-format-14 through clang-format-16 version is required.

clang-format is strongly recommended to write code that follows the ns-3 code conventions, but
might be skipped for simpler tasks (e.g., writing a simple simulation script for yourself).

clang-tidy is recommended when writing a module, to both follow code conventions and to provide
hints on possible bugs in code.

Both are used in the CI system, and a merge request will likely fail if you did not use them.

Debugging
=========

`GDB <https://www.sourceware.org/gdb/>`_ and `Valgrind <https://valgrind.org>`_ are useful
for debugging and recommended if you are doing C++ development of new models or scenarios.
Both of the above tools are available for Linux and BSD systems; for macOS,
`LLDB <https://lldb.llvm.org>`_ is similar to GDB, but Valgrind doesn't appear to be available
for M1 machines.

Optional
********

The remaining prerequisites listed below are only needed for optional ns-3 components.

.. note::
  As of ns-3.30 release (August 2019), ns-3 uses Python 3 by default, but earlier
  releases depend on Python 2 packages, and at least a Python 2 interpreter is recommended.
  If installing the below prerequisites for an earlier release, one may in general substitute
  'python' where 'python3' is found in the below (e.g., install 'python-dev' instead of
  'python3-dev').

To read pcap packet traces
==========================

Many |ns3| examples generate pcap files that can be viewed by pcap analyzers such as Tcpdump
and `Wireshark <https://www.wireshark.org>`_.

Database support
================

`SQLite <https://www.sqlite.org>`_ is recommended if you are using the statistics framework or
if you are running LTE or NR simulations (which make use of SQLite databases):


Python bindings (ns-3.37 and newer)
===================================

|ns3| Python support now uses `cppyy <https://cppyy.readthedocs.io/en/latest/>`_.

Using Python bindings (release 3.30 to ns-3.36)
===============================================

This pertains to the use of existing Python bindings shipped with ns-3; for updating or
generating new bindings, see further below.

Python bindings used `pybindgen <https://github.com/gjcarneiro/pybindgen>`_ in the past, which
can usually be found in the ``ns-3-allinone`` directory.  Python3 development packages, and
setup tools, are typically needed.

NetAnim animator
================

The `Qt <https://www.qt.io>`_ qt5 development tools are needed for NetAnim animator;
qt4 will also work but we have migrated to qt5. qt6 compatibility is not tested.

PyViz visualizer
================

The PyViz visualizer uses a variety of Python packages supporting GraphViz.
In general, to enable Python support in ns-3, `cppyy <https://cppyy.readthedocs.io/en/latest/>`_ is required.

MPI-based distributed simulation
================================

`Open MPI <https://www.open-mpi.org/>`_ support is needed if you intend to run large, parallel
|ns3| simulations.

Doxygen
=======

`Doxygen <https://www.doxygen.nl>`_ is only needed if you intend to write new Doxygen
documentation for header files.

Sphinx documentation
====================

The ns-3 manual (``doc/manual``), tutorial (``doc/tutorial``) and others are written in
reStructuredText for Sphinx, and figures are typically in dia.  To build PDF versions,
`texlive <https://www.tug.org/texlive/>`_ packages are needed.

Eigen3 support
==============
`Eigen3 <https://gitlab.com/libeigen/eigen>`_ is used to support more efficient calculations
when using the `3GPP propagation loss models <https://www.nsnam.org/docs//models/html/propagation.html#threegpppropagationlossmodel>`_
in LTE and NR simulations.

GNU Scientific Library (GSL)
============================

GNU Scientific Library (GSL) is is only used for more accurate 802.11b (legacy) WiFi error models (not needed for more modern OFDM-based Wi-Fi).

XML-based version of the config store
=====================================

`Libxml2 <https://gitlab.gnome.org/GNOME/libxml2>`_ is needed for the XML-based Config Store feature.

A GTK-based configuration system
================================

`GTK development libraries <https://www.gtk.org>`_ are also related to the (optional) config store,
for graphical desktop support.

Generating modified python bindings (ns-3.36 and earlier)
=========================================================

To modify the Python bindings found in release 3.36 and earlier (not needed for modern releases,
or if you do not use Python, the `LLVM toolchain <https://llvm.org>`_ and
`cxxfilt <https://pypi.org/project/cxxfilt/>`_ are needed.

You will also need to install
`castxml <https://github.com/CastXML/CastXML>`_ and
`pygccxml <https://github.com/CastXML/pygccxml>`_ as per the instructions for Python bindings (or
through the bake build tool as described in the |ns3| tutorial).
If you plan to work with bindings or rescan them for any ns-3 C++ changes you
might make, please read the chapter in the manual (corresponding to the release) on this topic.

To experiment with virtual machines and ns-3
============================================

Linux systems can use `LXC <https://linuxcontainers.org>`_ and
`TUN/TAP device drivers <https://docs.kernel.org/networking/tuntap.html>`_ for emulation support.

Support for openflow module
===========================

`OpenFlow switch support <https://www.nsnam.org/docs/models/html/openflow-switch.html>`_ requires
XML and `Boost <https://www.boost.org/>`_ development libraries.
