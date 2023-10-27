# Copyright (c) 2023 Universidade de Brasília
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License version 2 as published by the Free
# Software Foundation;
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA  02111-1307 USA
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

# Set Linux flag if on Linux
if(UNIX AND NOT APPLE)
  set(LINUX TRUE)
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

if(WIN32)
  set(NS3_PRECOMPILE_HEADERS OFF
      CACHE BOOL "Precompile module headers to speed up compilation" FORCE
  )

  # For whatever reason getting M_PI and other math.h definitions from cmath
  # requires this definition
  # https://docs.microsoft.com/en-us/cpp/c-runtime-library/math-constants?view=vs-2019
  add_definitions(/D_USE_MATH_DEFINES)
endif()

set(cat_command cat)

if(CMAKE_XCODE_BUILD_SYSTEM)
  set(XCODE True)
else()
  set(XCODE False)
endif()

if(${XCODE})
  # Is that so hard not to break people's CI, AAPL? Why would you output the
  # targets to a Debug/Release subfolder? Why?
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
  endforeach()
endif()
