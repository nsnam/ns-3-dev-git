# Copyright (c) 2023 Universidade de Brasília
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>

# Parse .ns3rc
macro(parse_ns3rc enabled_modules disabled_modules examples_enabled
      tests_enabled
)
  # Try to find .ns3rc
  disable_cmake_warnings()
  find_file(NS3RC .ns3rc PATHS /etc $ENV{HOME} $ENV{USERPROFILE}
                               ${PROJECT_SOURCE_DIR} NO_CACHE
  )
  enable_cmake_warnings()

  # Set variables with default values (all modules, no examples nor tests)
  set(${enabled_modules} "")
  set(${disabled_modules} "")
  set(${examples_enabled} "FALSE")
  set(${tests_enabled} "FALSE")

  if(NOT (${NS3RC} STREQUAL "NS3RC-NOTFOUND"))
    message(${HIGHLIGHTED_STATUS}
            "Configuration file .ns3rc being used : ${NS3RC}"
    )
    file(READ ${NS3RC} ns3rc_contents)
    # Check if ns3rc file is CMake or Python based and act accordingly
    if(ns3rc_contents MATCHES "ns3rc_*")
      include(${NS3RC})
    else()
      parse_python_ns3rc(
        "${ns3rc_contents}" ${enabled_modules} ${examples_enabled}
        ${tests_enabled} ${NS3RC}
      )
    endif()
  endif()
endmacro(parse_ns3rc)

function(parse_python_ns3rc ns3rc_contents enabled_modules examples_enabled
         tests_enabled ns3rc_location
)
  # Save .ns3rc backup
  file(WRITE ${ns3rc_location}.backup ${ns3rc_contents})

  # Match modules_enabled list
  if(ns3rc_contents MATCHES "modules_enabled.*\\[(.*).*\\]")
    set(${enabled_modules} ${CMAKE_MATCH_1})
    if(${enabled_modules} MATCHES "all_modules")
      # If all modules, just clean the filter and all modules will get built by
      # default
      set(${enabled_modules})
    else()
      # If modules are listed, remove quotes and replace commas with semicolons
      # transforming a string into a cmake list
      string(REPLACE "," ";" ${enabled_modules} "${${enabled_modules}}")
      string(REPLACE "'" "" ${enabled_modules} "${${enabled_modules}}")
      string(REPLACE "\"" "" ${enabled_modules} "${${enabled_modules}}")
      string(REPLACE " " "" ${enabled_modules} "${${enabled_modules}}")
      string(REPLACE "\n" ";" ${enabled_modules} "${${enabled_modules}}")
      list(SORT ${enabled_modules})

      # Remove possibly empty entry
      list(REMOVE_ITEM ${enabled_modules} "")
      foreach(element ${${enabled_modules}})
        # Inspect each element for comments
        if(${element} MATCHES "#.*")
          list(REMOVE_ITEM ${enabled_modules} ${element})
        endif()
      endforeach()
    endif()
  endif()

  string(REPLACE "True" "ON" ns3rc_contents ${ns3rc_contents})
  string(REPLACE "False" "OFF" ns3rc_contents ${ns3rc_contents})

  # Match examples_enabled flag
  if(ns3rc_contents MATCHES "examples_enabled = (ON|OFF)")
    set(${examples_enabled} ${CMAKE_MATCH_1})
  endif()

  # Match tests_enabled flag
  if(ns3rc_contents MATCHES "tests_enabled = (ON|OFF)")
    set(${tests_enabled} ${CMAKE_MATCH_1})
  endif()

  # Save variables to parent scope
  set(${enabled_modules} "${${enabled_modules}}" PARENT_SCOPE)
  set(${examples_enabled} "${${examples_enabled}}" PARENT_SCOPE)
  set(${tests_enabled} "${${tests_enabled}}" PARENT_SCOPE)

  # Save updated .ns3rc file
  message(
    ${HIGHLIGHTED_STATUS}
    "The python-based .ns3rc file format is deprecated and was updated to the CMake format"
  )
  configure_file(
    ${PROJECT_SOURCE_DIR}/build-support/.ns3rc-template ${ns3rc_location} @ONLY
  )
endfunction(parse_python_ns3rc)
