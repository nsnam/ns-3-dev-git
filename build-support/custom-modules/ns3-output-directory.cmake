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

# This is where we define where executables, libraries and headers will end up

if("${NS3_OUTPUT_DIRECTORY}" STREQUAL "")
  message(STATUS "Using default output directory ${PROJECT_SOURCE_DIR}/build")
  set(CMAKE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build) # default output
  # folder
else()
  # Check if NS3_OUTPUT_DIRECTORY is a relative path
  set(absolute_ns3_output_directory "${NS3_OUTPUT_DIRECTORY}")
  if(NOT IS_ABSOLUTE ${NS3_OUTPUT_DIRECTORY})
    set(absolute_ns3_output_directory
        "${PROJECT_SOURCE_DIR}/${NS3_OUTPUT_DIRECTORY}"
    )
  endif()

  # Transform backward slash into forward slash Not the best way to do it since
  # \ is a scape thing and can be used before whitespaces
  string(REPLACE "\\" "/" absolute_ns3_output_directory
                 "${absolute_ns3_output_directory}"
  )

  if(NOT (EXISTS ${absolute_ns3_output_directory}))
    message(
      STATUS
        "User-defined output directory \"${NS3_OUTPUT_DIRECTORY}\" doesn't exist. Trying to create it"
    )
    file(MAKE_DIRECTORY ${absolute_ns3_output_directory})
    if(NOT (EXISTS ${absolute_ns3_output_directory}))
      message(
        FATAL_ERROR
          "User-defined output directory \"${absolute_ns3_output_directory}\" could not be created. "
          "Try changing the value of NS3_OUTPUT_DIRECTORY"
      )
    endif()

    # If this directory is not inside the ns-3-dev folder, alert users tests may
    # break
    if(NOT ("${absolute_ns3_output_directory}" MATCHES "${PROJECT_SOURCE_DIR}"))
      message(
        WARNING
          "User-defined output directory \"${absolute_ns3_output_directory}\" is outside "
          " of the ns-3 directory ${PROJECT_SOURCE_DIR}, which will break some tests"
      )
    endif()
  endif()
  message(
    STATUS
      "User-defined output directory \"${absolute_ns3_output_directory}\" will be used"
  )
  set(CMAKE_OUTPUT_DIRECTORY ${absolute_ns3_output_directory})
endif()

set(libdir "lib")
if(${CMAKE_INSTALL_LIBDIR} MATCHES "lib64")
  set(libdir "lib64")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/${libdir})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/${libdir})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY})
set(CMAKE_HEADER_OUTPUT_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/include/ns3)
set(THIRD_PARTY_DIRECTORY ${PROJECT_SOURCE_DIR}/3rd-party)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
link_directories(${CMAKE_OUTPUT_DIRECTORY}/${libdir})
file(MAKE_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY})
