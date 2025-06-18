# Copyright (c) 2023 Universidade de Brasília
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>

# function used to search for package and program dependencies than store list
# of missing dependencies in the list whose name is stored in missing_deps
function(check_deps missing_deps)
  set(multiValueArgs CMAKE_PACKAGES EXECUTABLES PYTHON_PACKAGES)
  cmake_parse_arguments("DEPS" "" "" "${multiValueArgs}" ${ARGN})

  set(local_missing_deps)
  # Search for package dependencies
  foreach(package ${DEPS_CMAKE_PACKAGES})
    find_package(${package})
    if(NOT ${${package}_FOUND})
      list(APPEND local_missing_deps ${package})
    endif()
  endforeach()

  # And for program dependencies
  foreach(program ${DEPS_EXECUTABLES})
    # CMake likes to cache find_* to speed things up, so we can't reuse names
    # here or it won't check other dependencies
    string(TOUPPER ${program} upper_${program})
    mark_as_advanced(${upper_${program}})
    find_program(
      ${upper_${program}} ${program} HINTS ${3RD_PARTY_FIND_PROGRAM_HINTS}
    )
    if("${${upper_${program}}}" STREQUAL "${upper_${program}}-NOTFOUND")
      list(APPEND local_missing_deps ${program})
    endif()
  endforeach()

  foreach(package ${DEPS_PYTHON_PACKAGES})
    execute_process(
      COMMAND ${Python3_EXECUTABLE} -c "import ${package}"
      RESULT_VARIABLE return_code OUTPUT_QUIET ERROR_QUIET
    )
    if(NOT (${return_code} EQUAL 0))
      list(APPEND local_missing_deps ${package})
    else()
      # To make sure CMake import files can be found from venv site packages, we
      # manually add them to CMAKE_PREFIX_PATH
      execute_process(
        COMMAND
          ${Python3_EXECUTABLE} -c
          "import os; import ${package}; print(os.path.abspath(os.path.dirname(${package}.__file__)))"
        OUTPUT_VARIABLE venv_site_packages_path
      )
      # Remove newlines (\n, \r, \r\n)
      string(REGEX REPLACE "[\r\n]+$" "" venv_site_packages_path
                           "${venv_site_packages_path}"
      )
      if(EXISTS ${venv_site_packages_path})
        if(NOT (DEFINED CMAKE_PREFIX_PATH))
          set(CMAKE_PREFIX_PATH "" PARENT_SCOPE)
        endif()
        set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${venv_site_packages_path}"
            PARENT_SCOPE
        )
      endif()
    endif()
  endforeach()

  # Store list of missing dependencies in the parent scope
  set(${missing_deps} ${local_missing_deps} PARENT_SCOPE)
endfunction()
