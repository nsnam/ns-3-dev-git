# Copyright (c) 2023 Universidade de Brasília
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>

# Set INT128 as the default option for INT64X64 and register alternative
# implementations
set(NS3_INT64X64 "INT128" CACHE STRING "Int64x64 implementation")
set_property(CACHE NS3_INT64X64 PROPERTY STRINGS INT128 CAIRO DOUBLE)

# Purposefully hidden options:

# For ease of use, export all libraries and include directories to ns-3 module
# consumers by default
option(NS3_REEXPORT_THIRD_PARTY_LIBRARIES "Export all third-party libraries
and include directories to ns-3 module consumers" ON
)

# Since we can't really do that safely from the CMake side
option(NS3_ENABLE_SUDO
       "Set executables ownership to root and enable the SUID flag" OFF
)

# A flag that controls some aspects related to pip packaging
option(NS3_PIP_PACKAGING "Control aspects related to pip wheel packaging" OFF)

# fPIC (position-independent code) and fPIE (position-independent executable)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Do not create a file-level dependency with shared libraries reducing
# unnecessary relinking
set(CMAKE_LINK_DEPENDS_NO_SHARED TRUE)

# Honor CMAKE_CXX_STANDARD in check_cxx_source_compiles
# https://cmake.org/cmake/help/latest/policy/CMP0067.html
cmake_policy(SET CMP0066 NEW)
cmake_policy(SET CMP0067 NEW)
