.. include:: replace.txt
.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

Overview
--------

This guide documents the different ways that users can download, build, and install |ns3| from source code.  All of these actions (download, build, install) have variations
or options, and can be customized or extended by users; this document attempts to inform readers
about different possibilities.

Please note a few important details:

* |ns3| is supported for Linux, macOS, and Windows systems.  Linux is the primary system
  supported, and (almost) all |ns3| features are supported on Linux.  However, most features can
  also be used on macOS and Windows.  Windows is probably the least used and least supported
  system.
* |ns3| has minimal prerequisites for its most basic installation; namely, a **C++** compiler,
  **Python3** support, the **CMake** build system, and at least one of **make**, **ninja**, or
  **Xcode** build systems.  However, some users will want to install optional packages
  to make use of the many optional extensions.
* |ns3| installation requirements vary from release to release, and also as underlying operating
  systems evolve.  This document is version controlled, so if you are using the *ns-3.X* release,
  try to read the *ns-3.X* version of this document.  Even with this guidance, you may encounter
  issues if you are trying to use an old version of |ns3| on a much newer system.  For instance,
  |ns3| versions until around 2020 relied on Python2, which is now end-of-life and not installed
  by default on many Linux distributions.  Compilers also become more pedantic and issue more
  warnings as they evolve.  Solutions to some of these forward compatibility problems exist and
  are discussed herein or might be found in the ns-3-users mailing list archives.

If you find any stale or inaccurate information in this document, please report it to maintainers,
on our `Zulip chat <https://ns-3.zulipchat.com>`_, in the GitLab.com
`Issue tracker <https://gitlab.com/nsnam/ns-3-dev/-/issues/>`_, (or better yet, a patch to fix
in the `Merge Request tracker <https://gitlab.com/nsnam/ns-3-dev/-/merge_requests>`_), or on our
developers mailing list.

We also will accept patches to this document to provide installation guidance for the
FreeBSD operating system and other operating systems being used in practice.

Software organization
*********************

|ns3| is a set of C++ libraries (usually compiled as shared libraries) that can be used by C++
or Python programs to construct simulation scenarios and execute simulations.  Users can also
write programs that link other C++ shared libraries (or import other Python modules).  Users can
choose to use a subset of the available libraries; only the ``core`` library is strictly required.

|ns3| uses the CMake build system (until release 3.36, the Waf build system was used).  It can be
built from command-line or via a code editor program.

Most users write C++ ns-3 programs; Python support is less frequently used.
As of *ns-3.37*, |ns3| uses `cppyy` to generate runtime Python
bindings, and |ns3| is available in the Pip repositories as of ns-3.39 release.

Many users may be familiar with how software is packaged and installed on Linux and other systems
using package managers.  For example, to install a given Linux development library such as
OpenSSL, a package manager is typically used (e.g., ``apt install openssl libssl-dev``), which leads
to shared libraries being installed under ``/usr/lib/``, development headers being installed
under ``/usr/include/``, etc.  Programs that wish to use these libraries must link the system
libraries and include the development headers.  |ns3| is capabile of being installed in exactly
the same way, and some downstream package maintainers have packaged |ns3| for some systems
such as Ubuntu.  However, as of this writing, the |ns3| project has not prioritized or
standardized such package distribution, favoring instead to recommend a *source download* without
a system-level install.  This is mainly because most |ns3| users prefer to slightly or extensively
edit or extend the |ns3| libraries, or to build them in specific ways (for debugging, or optimized
for large-scale simulation campaign).  Our build system provides an ``install`` command that can
be used to install libraries and headers to system locations (usually requiring administrator
privileges), but usually the libraries are just built and used from within the |ns3| ``build``
directory.

Document organization
*********************

The rest of this document is organized as follows:

* :ref:`Quick start` (for any operating system)
* :ref:`General list` of requirements, recommendations, and optional prerequisites
* :ref:`Linux` installation
* :ref:`macOS` installation
* :ref:`Windows` installation
* :ref:`Bake` installation
