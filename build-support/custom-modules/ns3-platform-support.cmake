# Copyright (c) 2023 Universidade de Brasília
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>

# This is where we check for platform specifics

# WSLv1 doesn't support tap features
if(EXISTS "/proc/version")
  file(READ "/proc/version" CMAKE_LINUX_DISTRO)
  string(FIND "${CMAKE_LINUX_DISTRO}" "Microsoft" res)
  if(res EQUAL -1)
    set(WSLv1 False)
  else()
    set(WSLv1 True)
  endif()
endif()

# cmake-format: off
# Windows injects **BY DEFAULT** its %PATH% into WSL $PATH
# Detect and warn users how to disable path injection
# cmake-format: on
if("$ENV{PATH}" MATCHES "/mnt/c/")
  message(
    WARNING
      "To prevent Windows path injection on WSL, append the following to /etc/wsl.conf, then use `wsl --shutdown` to restart the WSL VM:
[interop]
appendWindowsPath = false"
  )
endif()

# Set Linux flag if on Linux
if(UNIX AND NOT APPLE)
  if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    set(LINUX TRUE)
  else()
    set(BSD TRUE)
  endif()
  add_definitions(-D__LINUX__)
endif()

if(APPLE)
  add_definitions(-D__APPLE__)
  # cmake-format: off
    # Configure find_program to search for AppBundles only if programs are not found in PATH.
    # This prevents Doxywizard from being launched when the Doxygen.app is installed.
    # https://gitlab.kitware.com/cmake/cmake/-/blob/master/Modules/FindDoxygen.cmake
    # cmake-format: on
  set(CMAKE_FIND_APPBUNDLE "LAST")
endif()

set(cat_command cat)
if(WIN32)
  # Precompiled headers work fine with ClangCL and give a large compile-time win
  # there (the std headers, which on the MSVC STL are very heavy under
  # /std:c++latest, are parsed once instead of in every translation unit). Only
  # force them off for the other Windows compilers (MSVC cl.exe), where they
  # have historically misbehaved.
  if(NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    set(NS3_PRECOMPILE_HEADERS OFF
        CACHE BOOL "Precompile module headers to speed up compilation" FORCE
    )
  endif()

  # For whatever reason getting M_PI and other math.h definitions from cmath
  # requires this definition
  # https://docs.microsoft.com/en-us/cpp/c-runtime-library/math-constants?view=vs-2019
  add_definitions(/D_USE_MATH_DEFINES)
  set(cat_command type) # required until we move to CMake >= 3.18
endif()

if(CMAKE_XCODE_BUILD_SYSTEM)
  set(XCODE True)
else()
  set(XCODE False)
endif()

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC" OR "${CMAKE_CXX_SIMULATE_ID}"
                                                MATCHES "MSVC"
)
  set(MSVC True)
  set(NS3_INT64X64 "CAIRO" CACHE STRING "Int64x64 implementation" FORCE)
else()
  set(MSVC False)
endif()

if(${MSVC} OR ${XCODE})
  # Prevent multi-config generators from placing output files into per
  # configuration directory
  if(NOT (DEFINED CMAKE_CONFIGURATION_TYPES))
    set(CMAKE_CONFIGURATION_TYPES DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
  endif()
  foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG}
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG}
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    )
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG}
        ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
    )
  endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)
endif()

if(${MSVC})
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

  # Enable exceptions
  add_definitions(/EHs)

  # Account for lack of valgrind and binary compatibility restrictions on
  # Windows https://github.com/microsoft/STL/wiki/Changelog#vs-2022-1710
  add_compile_definitions(
    __WIN32__ NVALGRIND _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR
    _CRT_SECURE_NO_WARNINGS NS_MSVC NOMINMAX
  )

  # Disable inlining (trust me, it is either that or moving every single inlined
  # code to .cc)
  add_compile_options(/Ob0)

  # ClangCL only. The wifi module manually exports its public API (it disables
  # CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS to stay under the 65535 PE export-ordinal
  # limit) by annotating its classes with WIFI_EXPORT, which expands to
  # __declspec(dllexport) when building the library and __declspec(dllimport)
  # when consuming it. -fno-dllexport-inlines drops that attribute from inline
  # member functions, so they are emitted on first use instead of being placed
  # in (library) / imported from (consumer) the DLL export table.
  #
  # This flag rewrites BOTH the dllexport and dllimport sides, so it must be
  # seen by every translation unit, not just the wifi library. Applying it only
  # to the library makes consumers keep importing inline members (default ctors,
  # destructors, move-assignment, SimpleRefCount<>::Unref, ...) that the library
  # no longer exports, which fails at link time with lld-link: error: undefined
  # symbol: __declspec(dllimport) ... ns3::Ssid::~Ssid It is a no-op for the
  # modules that still rely on CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS, since those
  # carry no dllexport annotations.
  #
  # -fno-dllexport-inlines is a clang cc1 flag, so it is forwarded with -Xclang;
  # the SHELL: prefix keeps the two tokens together and unsorted, and the
  # COMPILE_LANGUAGE:CXX guard avoids feeding a C++-only flag to any C source.
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
      "$<$<COMPILE_LANGUAGE:CXX>:SHELL:-Xclang -fno-dllexport-inlines>"
    )
  endif()

  # Prevent linker from eliminating unused functions and increase stack size to
  # 8MB to match Linux
  add_link_options(/OPT:NOREF /STACK:8388608)
endif()
