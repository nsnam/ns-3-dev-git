# Copyright (c) 2017-2021 Universidade de Brasília
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

function(cache_cmake_flag cmake_flag cache_entry output_string)
  if(${${cmake_flag}})
    set(${output_string} "${${output_string}}${cache_entry} = True\n"
        PARENT_SCOPE
    )
  else()
    set(${output_string} "${${output_string}}${cache_entry} = False\n"
        PARENT_SCOPE
    )
  endif()
endfunction(cache_cmake_flag)

function(generate_c4che_cachepy)
  # Build _cache.py file consumed by test.py
  set(cache_contents "")

  string(APPEND cache_contents "NS3_ENABLED_MODULES = [")
  foreach(module_library ${ns3-libs}) # fetch core module libraries
    string(APPEND cache_contents "'")
    string(REPLACE "lib" "" module_name ${module_library}) # lib${libname} into
                                                           # libname
    string(APPEND cache_contents "ns3-${module_name}', ")
  endforeach()
  string(APPEND cache_contents "]\n")

  string(APPEND cache_contents "NS3_ENABLED_CONTRIBUTED_MODULES = [")
  foreach(module_library ${ns3-contrib-libs}) # fetch core module libraries
    string(APPEND cache_contents "'")
    string(REPLACE "lib" "" module_name ${module_library}) # lib${libname} into
                                                           # libname
    string(APPEND cache_contents "ns3-${module_name}', ")
  endforeach()
  string(APPEND cache_contents "]\n")

  string(REPLACE ":" "', '" PATH_LIST $ENV{PATH})
  string(
    APPEND
    cache_contents
    "NS3_MODULE_PATH = ['${PATH_LIST}', '${CMAKE_OUTPUT_DIRECTORY}', '${CMAKE_LIBRARY_OUTPUT_DIRECTORY}']\n"
  )

  cache_cmake_flag(ENABLE_REALTIME "ENABLE_REAL_TIME" cache_contents)
  cache_cmake_flag(NS3_PTHREAD "ENABLE_THREADING" cache_contents)
  cache_cmake_flag(ENABLE_EXAMPLES "ENABLE_EXAMPLES" cache_contents)
  cache_cmake_flag(ENABLE_TESTS "ENABLE_TESTS" cache_contents)
  cache_cmake_flag(NS3_OPENFLOW "ENABLE_OPENFLOW" cache_contents)
  cache_cmake_flag(NS3_CLICK "NSCLICK" cache_contents)
  cache_cmake_flag(NS3_BRITE "ENABLE_BRITE" cache_contents)
  cache_cmake_flag(NS3_PYTHON_BINDINGS "ENABLE_PYTHON_BINDINGS" cache_contents)

  string(APPEND cache_contents "EXAMPLE_DIRECTORIES = [")
  foreach(example_folder ${ns3-example-folders})
    string(APPEND cache_contents "'${example_folder}', ")
  endforeach()
  string(APPEND cache_contents "]\n")

  string(APPEND cache_contents "APPNAME = 'ns'\n")
  string(APPEND cache_contents "BUILD_PROFILE = '${build_profile}'\n")
  string(APPEND cache_contents "VERSION = '${NS3_VER}' \n")
  string(APPEND cache_contents "PYTHON = ['${Python3_EXECUTABLE}']\n")

  mark_as_advanced(VALGRIND)
  find_program(VALGRIND valgrind)
  if("${VALGRIND}" STREQUAL "VALGRIND-NOTFOUND")
    string(APPEND cache_contents "VALGRIND_FOUND = False \n")
  else()
    string(APPEND cache_contents "VALGRIND_FOUND = True \n")
  endif()
  file(WRITE ${CMAKE_OUTPUT_DIRECTORY}/c4che/_cache.py "${cache_contents}")
endfunction(generate_c4che_cachepy)
