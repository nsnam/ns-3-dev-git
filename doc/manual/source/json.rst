.. include:: replace.txt
.. highlight:: cpp

JSON support
------------

|ns3| vendors the `nlohmann/json`_ header-only library (version 3.12.0) under
``third-party/nlohmann/``. At configure time the header is copied into the
build-tree include directory (``build/include/nlohmann/``), so any |ns3| module,
as well as the ``contrib`` and ``scratch`` directories, can use it without
additional CMake changes.

.. _nlohmann/json: https://github.com/nlohmann/json

.. warning::

   Use the library only from implementation (``.cc``) files, not from installed
   |ns3| public headers. Exposing ``nlohmann/json`` types through an |ns3| public
   header makes the vendored copy part of the |ns3| ABI, which can clash with a
   different version of the library used by downstream code. Keeping it confined
   to implementation files avoids that conflict.

Why vendored
************

JSON is increasingly used in |ns3| for configuration files, exported
statistics, and exchange with external tools. A single in-tree copy keeps the
build reproducible across platforms and avoids the need for users to install
the library system-wide.

The vendored header is copied into the build-tree include directory
(``build/include/nlohmann/``) at configure time and installed alongside the
|ns3| public headers, so it is available both in the build tree and from an
installed |ns3|. To refresh the copy after updating the file under
``third-party/nlohmann/``, re-run ``./ns3 configure``.

Basic usage
***********

Include the header using the standard upstream path:

.. sourcecode:: cpp

  #include <nlohmann/json.hpp>

  // Parse a JSON document from a string.
  auto doc = nlohmann::json::parse(R"({ "name": "ns-3", "version": 3 })");

  std::string name = doc["name"].get<std::string>();
  int         ver  = doc["version"].get<int>();

The full API is documented at https://json.nlohmann.me/.

Linting and style
*****************

The vendored header is excluded from |ns3| style and static-analysis checks:

* ``utils/check-style-clang-format.py`` skips the ``third-party/`` directory
  entirely (it is listed in ``DIRECTORIES_TO_SKIP``).
* ``clang-tidy`` excludes the directory via the
  ``ExcludeHeaderFilterRegex`` setting in ``.clang-tidy``.

When upgrading the library, replace the file under ``third-party/nlohmann/``
in a single, isolated commit -- do not reformat or otherwise modify it.

Tests
*****

A trivial smoke test that parses an inline JSON document lives at
``src/core/test/nlohmann-json-test-suite.cc`` and can be executed with::

  $ ./test.py -s nlohmann-json
