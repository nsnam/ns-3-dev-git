.. include:: replace.txt
.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

.. _macOS:

macOS
-----

This chapter describes installation steps specific to Apple macOS.  macOS installation of |ns3|
requires either the installation of the full
`Xcode IDE <https://developer.apple.com/xcode/features/>`_ or a more minimal install of
`Xcode Command Line Tools <https://www.freecodecamp.org/news/install-xcode-command-line-tools/>`_).

The full Xcode IDE requires 40 GB of disk space.  If you are just interested in getting |ns3|
to run, the full Xcode is not necessary; we recommend Command Line Tools instead.

In addition to Command Line Tools, some |ns3| extensions require third-party libraries;
we recommend either `Homebrew <https://brew.sh/>`_ or `MacPorts <https://www.macports.org/>`_.
If you prefer, you can probably avoid installing Command Line Tools and install the compiler
of your choice and any other tools you may need using Homebrew or MacPorts.

In general, documentation on the web suggests to use either, but not both, Homebrew or MacPorts
on a particular system.  It has been noted that Homebrew tends to install the GUI version of certain
applications without easily supporting the command-line equivalent, such as for the
`dia <https://gitlab.gnome.org/GNOME/dia/>`_ application; see ns-3
`MR 1247 <https://gitlab.com/nsnam/ns-3-dev/-/merge_requests/1247>`_ for discussion about this.

Finally, regarding Python, some |ns3| maintainers prefer to use a
`virtualenv <https://docs.python.org/3/library/venv.html>`_ to guard against incompatibilities
that might arise from the native macOS Python and versions that may be installed by Homebrew
or `Anaconda <https://docs.anaconda.com/anaconda/install/mac-os/>`_.  Some |ns3| users never
use Python bindings or visualizer, but if your |ns3| workflow requires more heavy use
of Python, please keep the possibility of a virtualenv in mind if you run into Python difficulties.
For a short guide on virtual environments, please see
`this link <https://www.dataquest.io/blog/a-complete-guide-to-python-virtual-environments/>`_.

Due to an `upstream limitation with Cppyy <https://github.com/wlav/cppyy/issues/150>`_, Python bindings do not work on macOS machines with Apple silicon (M1 and M2 processors).

Requirements
************

Installing ns-3 on macOS requires two fundamental things:  1) C++/Python development tools, and 2) CMake build system with at least one underlying build tool.  These can either be installed via binary package installation from the macOS App Store (Xcode development tools) or the web (`CMake binary package <https://cmake.org/download/>`_), or from Homebrew or MacPorts.

macOS Xcode uses the Clang/LLVM compiler toolchain.  It is possible to install the GNU compiler gcc/g++ from
Homebrew and MacPorts, but macOS will not provide it due to licensing issues.  If you do not install Xcode you will have to install build tools via Homebrew or MacPorts.  |ns3|
works on recent versions of both ``clang++`` and ``g++``, so for macOS, there is no need
to install ``g++``.

The following table provides package names for installing CMake and Ninja build system from Homebrew or MacPorts.

  +--------------------+-------------------------------+-------------------------------------+
  | **ns-3 Version**   | **Homebrew packages**         | **MacPorts packages**               |
  +====================+===============================+=====================================+
  | 3.36 and later     | ``cmake ninja``               | ``cmake ninja``                     |
  +--------------------+-------------------------------+-------------------------------------+
  | 3.35 and earlier   | None                          | None                                |
  +--------------------+-------------------------------+-------------------------------------+

You will know you are done when you can successfully type `clang++ -v` at the command line, and when you type `cmake --help` and it identifies that you have at least one installed generator (in the below example, Unix Makefiles):

::

  Generators
  The following generators are available on this platform (* marks default):
  * Unix Makefiles               = Generates standard UNIX makefiles.
    Ninja                        = Generates build.ninja files.
    Ninja Multi-Config           = Generates build-<Config>.ninja files.
    Watcom WMake                 = Generates Watcom WMake makefiles.
    Xcode                        = Generate Xcode project files.

Recommended
***********

  +-----------------------------+----------------------------+-----------------------------------+
  | **Feature**                 | **Homebrew packages**      | **MacPorts packages**             |
  +=============================+============================+===================================+
  | Compiler cache optimization | ``ccache``                 | ``ccache``                        |
  +-----------------------------+----------------------------+-----------------------------------+
  | Code linting                | clang-format included with | clang-format included with        |
  |                             | ``llvm``, need to select   | ``clang``, need to select         |
  |                             | ``llvm@XX`` version        | ``clang-XX llvm-XX`` versions     |
  +-----------------------------+----------------------------+-----------------------------------+

.. note::

  macOS development tools are based on clang, so installing llvm and clang using Homebrew or
  MacPorts is typically unnecessary. However, ``clang-tidy`` might be missing, and the
  ``clang-format`` version might be not the expected one. In these cases it is suggested to
  install the ``llvm`` package either though Homebrew or MacPorts.

.. note::

  Homebrew provides a ``clang-format`` package, but its version might be incompatible with
  the one used by |ns3|.

  The ``llvm`` Homebrew package provides ``clang-tidy`` and ``clang-format``, but the binary
  is placed at ``/opt/homebrew/opt/llvm@XX/bin/clang-tidy`` (where ``XX`` is the installed
  version number such as ``18``), so you will need to add this path to your ``$PATH`` variable.

.. note::

  Likewise, when using MacPorts, the ``clang-tidy`` and ``clang-format`` binaries will be
  placed in ``/opt/local/libexec/llvm-XX/bin`` (where ``XX`` is the installed version number
  such as ``18``), so you will need to add this to your ``$PATH`` variable.

.. note::

  For debugging, ``lldb`` is the default debugger for llvm.  Memory checkers such as
  Memory Graph exist for macOS, but the ns-3 team doesn't have experience with it as a
  substitution for ``valgrind`` (which is reported to not work on M1 Macs).

Optional
********

Please see below subsections for Python-related package requirements.

For MacPorts packages we show the most recent package version available as of early 2023.

  +-----------------------------+----------------------------------+--------------------------+
  | **Feature**                 | **Homebrew packages**            | **MacPort packages**     |
  +=============================+==================================+==========================+
  | Reading pcap traces         | ``wireshark``                    | ``wireshark4``           |
  +-----------------------------+----------------------------------+--------------------------+
  | Database support            | ``sqlite``                       | ``sqlite3``              |
  +-----------------------------+----------------------------------+--------------------------+
  | NetAnim animator            | ``qt@5``                         | ``qt513``                |
  +-----------------------------+----------------------------------+--------------------------+
  | MPI-based distributed       |                                  | ``openmpi``              |
  | simulation                  | ``open-mpi``                     |                          |
  +-----------------------------+----------------------------------+--------------------------+
  | Building Doxygen            | ``doxygen graphviz imagemagick`` | ``doxygen graphviz``     |
  |                             |                                  | ``ImageMagick``          |
  +-----------------------------+----------------------------------+--------------------------+
  | Sphinx documentation        | ``sphinx-doc texlive``           | ``dia texlive``          |
  |                             |                                  | ``texlive-fonts-extra``  |
  |                             |                                  | ``texlive-latex-extra``  |
  |                             |                                  | ``py3XX-sphinx``, with   |
  |                             |                                  | `XX`` the Python minor   |
  |                             |                                  | version such as `12`     |
  +-----------------------------+----------------------------------+--------------------------+
  | Eigen3                      | ``eigen``                        | ``eigen3``               |
  +-----------------------------+----------------------------------+--------------------------+
  | GNU Scientific Library      | ``gsl``                          | ``gsl``                  |
  +-----------------------------+----------------------------------+--------------------------+
  | XML config store            | ``libxml2``                      | ``libxml2``              |
  +-----------------------------+----------------------------------+--------------------------+
  | GTK-based config store      | ``gtk+3``                        | ``gtk3`` or ``gtk4``     |
  +-----------------------------+----------------------------------+--------------------------+
  | Emulation with virtual      |                                  |                          |
  | machines                    | Not available for macOS          | Not available for macOS  |
  +-----------------------------+----------------------------------+--------------------------+
  | Support for openflow,       | ``boost``                        | ``boost``                |
  | CircularApertureAntennaModel|                                  |                          |
  +-----------------------------+----------------------------------+--------------------------+

Caveats and troubleshooting
***************************

