# Copyright (c) 2023 Universidade de Brasília
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>

# This is where we define where executables, libraries and headers will end up

# Test if python is available
execute_process(
  COMMAND ${CMAKE_COMMAND} -E env python3 -c "exit(0)"
  RESULT_VARIABLE PYTHON_AVAILABLE
)

function(make_directory directory_path)
  if("${PYTHON_AVAILABLE}" EQUAL 0)
    # In case python is available, check if we have write permissions on
    # directory_path before trying to create it
    get_filename_component(parent ${directory_path} DIRECTORY)
    get_filename_component(grandparent ${parent} DIRECTORY)
    execute_process(
      COMMAND
        ${CMAKE_COMMAND} -E env python3 -c
        "import os; exit(0 if sum(map(lambda x: os.path.exists(x) and os.access(x, os.W_OK), ['${parent}', '${grandparent}'])) else 1)"
      RESULT_VARIABLE RESULT
    )
    if(NOT (${RESULT} EQUAL 0))
      message(
        FATAL_ERROR
          "Failed to create directory: ${directory_path}. Check for write permissions."
      )
    endif()
  else()
    # In case python is not available, warn users
    message(
      WARNING
        "Python3 executable was not found and write permissions won't be checked. Build may break for file permission issues."
    )
  endif()
  file(MAKE_DIRECTORY ${directory_path})
endfunction()

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

  message(
    STATUS
      "Creating user-defined output directory \"${NS3_OUTPUT_DIRECTORY}\". In case it fails, try changing the value of NS3_OUTPUT_DIRECTORY or check the directory permissions."
  )
  make_directory(${absolute_ns3_output_directory})

  # If this directory is not inside the ns-3-dev folder, alert users tests may
  # break
  if(NOT ("${absolute_ns3_output_directory}" MATCHES "${PROJECT_SOURCE_DIR}"))
    message(
      WARNING
        "User-defined output directory \"${absolute_ns3_output_directory}\" is outside "
        " of the ns-3 directory ${PROJECT_SOURCE_DIR}, which will break some tests"
    )
  endif()
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
make_directory(${CMAKE_OUTPUT_DIRECTORY})
make_directory(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
make_directory(${CMAKE_HEADER_OUTPUT_DIRECTORY})
