.. include:: replace.txt
.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

.. _Linux:

Linux
-----

This chapter describes Linux-specific installation commands to install the options described
in the previous chapter.  The chapter is written initially with Ubuntu (Debian-based) Linux
examples (Ubuntu is the most frequently used Linux distribution by |ns3| users) but should
translate fairly well to derivatives (e.g. Linux Mint).

The list of packages depends on which version of ns-3 you are trying to build, and on which
extensions you need; please review the previous chapter if you need more information.

Requirements
************

The minimum supported version of Ubuntu is Ubuntu 18.04 LTS (as long as a modern compiler
version such as g++ version 9 or later is added).

  +--------------------+---------------------------------------------------------------------+
  | **ns-3 Version**   | **apt Packages**                                                    |
  +====================+==================+==================================================+
  | 3.36 and later     | ``g++ python3 cmake ninja-build git``                               |
  +--------------------+---------------------------------------------------------------------+
  | 3.30-3.35          | ``g++ python3 git``                                                 |
  +--------------------+---------------------------------------------------------------------+
  | 3.29 and earlier   | ``g++ python2``                                                     |
  +--------------------+---------------------------------------------------------------------+

.. note::
  As of July 2023 (ns-3.39 release and later), the minimum g++ version is g++-9.
  Older ns-3 releases may work with older versions of g++; check the RELEASE_NOTES.

Recommended
***********

  +-----------------------------+------------------------------------------------------------+
  | **Feature**                 | **apt Packages**                                           |
  +=============================+============================================================+
  | Compiler cache optimization | ``ccache``                                                 |
  +-----------------------------+------------------------------------------------------------+
  | Code linting                | ``clang-format clang-tidy``                                |
  +-----------------------------+------------------------------------------------------------+
  | Debugging                   | ``gdb valgrind``                                           |
  +-----------------------------+------------------------------------------------------------+

.. note::
  For Ubuntu 20.04 release and earlier, the version of ccache provided by apt
  (3.7.7 or earlier) may not provide performance benefits, and users are recommended to install
  version 4 or later, possibly as a source install. For Ubuntu 22.04 and later, ccache can be
  installed using apt.

.. note::
  clang-format-14 through clang-format-16 version is required.

Optional
********

Please see below subsections for Python-related package requirements.

  +-----------------------------+------------------------------------------------------------+
  | **Feature**                 | **apt Packages**                                           |
  +=============================+============================================================+
  | Reading pcap traces         | ``tcpdump wireshark``                                      |
  +-----------------------------+------------------------------------------------------------+
  | Database support            | ``sqlite sqlite3 libsqlite3-dev``                          |
  +-----------------------------+------------------------------------------------------------+
  | NetAnim animator            | ``qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools``      |
  +-----------------------------+------------------------------------------------------------+
  | MPI-based distributed       |                                                            |
  | simulation                  | ``openmpi-bin openmpi-common openmpi-doc libopenmpi-dev``  |
  +-----------------------------+------------------------------------------------------------+
  | Building Doxygen            | ``doxygen graphviz imagemagick``                           |
  +-----------------------------+------------------------------------------------------------+
  | Sphinx documentation        | ``python3-sphinx dia imagemagick texlive dvipng latexmk``  |
  |                             | ``texlive-extra-utils texlive-latex-extra``                |
  |                             | ``texlive-font-utils``                                     |
  +-----------------------------+------------------------------------------------------------+
  | Eigen3                      | ``libeigen3-dev``                                          |
  +-----------------------------+------------------------------------------------------------+
  | GNU Scientific Library      | ``gsl-bin libgsl-dev libgslcblas0``                        |
  +-----------------------------+------------------------------------------------------------+
  | XML config store            | ``libxml2 libxml2-dev``                                    |
  +-----------------------------+------------------------------------------------------------+
  | GTK-based config store      | ``libgtk-3-dev``                                           |
  +-----------------------------+------------------------------------------------------------+
  | Emulation with virtual      |                                                            |
  | machines and tap bridge     | ``lxc-utils lxc-templates iproute2 iptables``              |
  +-----------------------------+------------------------------------------------------------+
  | Support for openflow        | ``libxml2 libxml2-dev libboost-all-dev``                   |
  +-----------------------------+------------------------------------------------------------+

.. note::
  For Ubuntu 20.10 and earlier, the single 'qt5-default' package suffices for NetAnim (``apt install qt5-default``)

Python bindings
===============

Python requires `Cppyy, <https://cppyy.readthedocs.io/en/latest/installation.html>` and specifically,
version 3.1.2 is the latest version known to work with ns-3 at this time.

ns-3.42 and newer::

  python3 -m pip install --user cppyy==3.1.2

ns-3.37-3.41::

  python3 -m pip install --user cppyy==2.4.2

ns-3.30-3.36 (also requires pybindgen, found in the ``allinone`` directory)::

  apt install python3-dev pkg-config python3-setuptools

PyViz visualizer
================

The PyViz visualizer uses a variety of Python packages supporting GraphViz.::

  apt install gir1.2-goocanvas-2.0 python3-gi python3-gi-cairo python3-pygraphviz gir1.2-gtk-3.0 ipython3

For Ubuntu 18.04 and later, python-pygoocanvas is no longer provided. The ns-3.29 release and later upgrades the support to GTK+ version 3, and requires these packages:

For ns-3.28 and earlier releases, PyViz is based on Python2, GTK+ 2, GooCanvas, and GraphViz::

  apt install python-pygraphviz python-kiwi python-pygoocanvas libgoocanvas-dev ipython

Generating modified python bindings (ns-3.36 and earlier)
=========================================================

To modify the Python bindings found in release 3.36 and earlier (not needed for modern releases,
or if you do not use Python).::

  apt install cmake libc6-dev libc6-dev-i386 libclang-dev llvm-dev automake python3-pip
  python3 -m pip install --user cxxfilt

and you will want to install castxml and pygccxml as per the instructions for python bindings (or
through the bake build tool as described in the tutorial). The 'castxml' and 'pygccxml' packages
provided by Ubuntu 18.04 and earlier are not recommended; a source build (coordinated via bake)
is recommended. If you plan to work with bindings or rescan them for any ns-3 C++ changes you
might make, please read the chapter in the manual on this topic.

Caveats and troubleshooting
***************************

When building documentation, if you get an error such as
``convert ... not authorized source-temp/figures/lena-dual-stripe.eps``, see
`this post <https://cromwell-intl.com/open-source/pdf-not-authorized.html>`_ about editing
ImageMagick's security policy configuration.  In brief, you will want to make this kind of
change to ImageMagick security policy::

   --- ImageMagick-6/policy.xml.bak 2020-04-28 21:10:08.564613444 -0700
   +++ ImageMagick-6/policy.xml 2020-04-28 21:10:29.413438798 -0700
   @@ -87,10 +87,10 @@
      <policy domain="path" rights="none" pattern="@*"/>
   -  <policy domain="coder" rights="none" pattern="PS" />
   +  <policy domain="coder" rights="read|write" pattern="PS" />
      <policy domain="coder" rights="none" pattern="PS2" />
      <policy domain="coder" rights="none" pattern="PS3" />
      <policy domain="coder" rights="none" pattern="EPS" />
   -  <policy domain="coder" rights="none" pattern="PDF" />
   +  <policy domain="coder" rights="read|write" pattern="PDF" />
      <policy domain="coder" rights="none" pattern="XPS" />
    </policymap>

