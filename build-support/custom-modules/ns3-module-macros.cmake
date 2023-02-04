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

# cmake-format: off
#
# This macro processes a ns-3 module
#
# Arguments:
# LIBNAME = core, wifi, contributor_module
# SOURCE_FILES = "list;of;.cc;files;"
# HEADER_FILES = "list;of;public;.h;files;"
# LIBRARIES_TO_LINK = "list;of;${library_names};"
# TEST_SOURCES = "list;of;.cc;test;files;"
#
# DEPRECATED_HEADER_FILES = "list;of;deprecated;.h;files", copy won't get triggered if DEPRECATED_HEADER_FILES isn't set
# IGNORE_PCH = TRUE or FALSE, prevents the PCH from including undesired system libraries (e.g. custom GLIBC for DCE)
# MODULE_ENABLED_FEATURES = "list;of;enabled;features;for;this;module" (used by fd-net-device)
# cmake-format: on

function(build_lib)
  # Argument parsing
  set(options IGNORE_PCH)
  set(oneValueArgs LIBNAME)
  set(multiValueArgs
      SOURCE_FILES
      HEADER_FILES
      LIBRARIES_TO_LINK
      TEST_SOURCES
      DEPRECATED_HEADER_FILES
      MODULE_ENABLED_FEATURES
      PRIVATE_HEADER_FILES
  )
  cmake_parse_arguments(
    "BLIB" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
  )

  # Get path src/module or contrib/module
  string(REPLACE "${PROJECT_SOURCE_DIR}/" "" FOLDER
                 "${CMAKE_CURRENT_SOURCE_DIR}"
  )

  # Add library to a global list of libraries
  if("${FOLDER}" MATCHES "src")
    set(ns3-libs "${lib${BLIB_LIBNAME}};${ns3-libs}"
        CACHE INTERNAL "list of processed upstream modules"
    )
  else()
    set(ns3-contrib-libs "${lib${BLIB_LIBNAME}};${ns3-contrib-libs}"
        CACHE INTERNAL "list of processed contrib modules"
    )
  endif()

  if(NOT ${XCODE})
    # Create object library with sources and headers, that will be used in
    # lib-ns3-static and the shared library
    add_library(
      ${lib${BLIB_LIBNAME}-obj} OBJECT "${BLIB_SOURCE_FILES}"
                                       "${BLIB_HEADER_FILES}"
    )

    if(${PRECOMPILE_HEADERS_ENABLED} AND (NOT ${BLIB_IGNORE_PCH}))
      target_precompile_headers(${lib${BLIB_LIBNAME}-obj} REUSE_FROM stdlib_pch)
    endif()

    # Create shared library with previously created object library (saving
    # compilation time for static libraries)
    add_library(
      ${lib${BLIB_LIBNAME}} SHARED $<TARGET_OBJECTS:${lib${BLIB_LIBNAME}-obj}>
    )
  else()
    # Xcode and CMake don't play well when using object libraries, so we have a
    # specific path for that
    add_library(${lib${BLIB_LIBNAME}} SHARED "${BLIB_SOURCE_FILES}")

    if(${PRECOMPILE_HEADERS_ENABLED} AND (NOT ${BLIB_IGNORE_PCH}))
      target_precompile_headers(${lib${BLIB_LIBNAME}} REUSE_FROM stdlib_pch)
    endif()
  endif()

  add_library(ns3::${lib${BLIB_LIBNAME}} ALIAS ${lib${BLIB_LIBNAME}})

  # Associate public headers with library for installation purposes
  if("${BLIB_LIBNAME}" STREQUAL "core")
    set(config_headers ${CMAKE_HEADER_OUTPUT_DIRECTORY}/config-store-config.h
                       ${CMAKE_HEADER_OUTPUT_DIRECTORY}/core-config.h
    )
    if(${ENABLE_BUILD_VERSION})
      list(APPEND config_headers
           ${CMAKE_HEADER_OUTPUT_DIRECTORY}/version-defines.h
      )
    endif()

    if(NOT FILESYSTEM_LIBRARY_IS_LINKED)
      list(APPEND BLIB_LIBRARIES_TO_LINK -lstdc++fs)
    endif()

    # Enable examples as tests suites
    if(${ENABLE_EXAMPLES} AND ${ENABLE_TESTS})
      if(NOT ${XCODE})
        target_compile_definitions(
          ${lib${BLIB_LIBNAME}}-obj PRIVATE NS3_ENABLE_EXAMPLES
        )
      else()
        target_compile_definitions(
          ${lib${BLIB_LIBNAME}} PRIVATE NS3_ENABLE_EXAMPLES
        )
      endif()
    endif()
  endif()
  set_target_properties(
    ${lib${BLIB_LIBNAME}}
    PROPERTIES
      PUBLIC_HEADER
      "${BLIB_HEADER_FILES};${BLIB_DEPRECATED_HEADER_FILES};${config_headers};${CMAKE_HEADER_OUTPUT_DIRECTORY}/${BLIB_LIBNAME}-module.h"
      PRIVATE_HEADER "${BLIB_PRIVATE_HEADER_FILES}"
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} # set output
                                                                 # directory for
                                                                 # DLLs
  )

  if(${NS3_CLANG_TIMETRACE})
    add_dependencies(timeTraceReport ${lib${BLIB_LIBNAME}})
  endif()

  # Split ns and non-ns libraries to manage their propagation properly
  set(non_ns_libraries_to_link ${CMAKE_THREAD_LIBS_INIT})

  set(ns_libraries_to_link)

  foreach(library ${BLIB_LIBRARIES_TO_LINK})
    remove_lib_prefix("${library}" module_name)

    # Check if the module exists in the ns-3 modules list or if it is a
    # 3rd-party library
    if(${module_name} IN_LIST ns3-all-enabled-modules)
      list(APPEND ns_libraries_to_link ${library})
    else()
      list(APPEND non_ns_libraries_to_link ${library})
    endif()
    unset(module_name)
  endforeach()

  if(NOT ${NS3_REEXPORT_THIRD_PARTY_LIBRARIES})
    # ns-3 libraries are linked publicly, to make sure other modules can find
    # each other without being directly linked
    set(exported_libraries PUBLIC ${LIB_AS_NEEDED_PRE} ${ns_libraries_to_link}
                           ${LIB_AS_NEEDED_POST}
    )

    # non-ns-3 libraries are linked privately, not propagating unnecessary
    # libraries such as pthread, librt, etc
    set(private_libraries PRIVATE ${LIB_AS_NEEDED_PRE}
                          ${non_ns_libraries_to_link} ${LIB_AS_NEEDED_POST}
    )

    # we don't re-export included libraries from 3rd-party modules
    set(exported_include_directories)
  else()
    # we export everything by default when NS3_REEXPORT_THIRD_PARTY_LIBRARIES=ON
    set(exported_libraries PUBLIC ${LIB_AS_NEEDED_PRE} ${ns_libraries_to_link}
                           ${non_ns_libraries_to_link} ${LIB_AS_NEEDED_POST}
    )
    set(private_libraries)

    # with NS3_REEXPORT_THIRD_PARTY_LIBRARIES, we export all 3rd-party library
    # include directories, allowing consumers of this module to include and link
    # the 3rd-party code with no additional setup
    get_target_includes(${lib${BLIB_LIBNAME}} exported_include_directories)
    string(REPLACE "-I" "" exported_include_directories
                   "${exported_include_directories}"
    )
    string(REPLACE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/include" ""
                   exported_include_directories
                   "${exported_include_directories}"
    )
  endif()

  target_link_libraries(
    ${lib${BLIB_LIBNAME}} ${exported_libraries} ${private_libraries}
  )

  # set output name of library
  set_target_properties(
    ${lib${BLIB_LIBNAME}}
    PROPERTIES OUTPUT_NAME ns${NS3_VER}-${BLIB_LIBNAME}${build_profile_suffix}
  )

  # export include directories used by this library so that it can be used by
  # 3rd-party consumers of ns-3 using find_package(ns3) this will automatically
  # add the build/include path to them, so that they can ns-3 headers with
  # <ns3/something.h>
  target_include_directories(
    ${lib${BLIB_LIBNAME}}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_OUTPUT_DIRECTORY}/include>
           $<INSTALL_INTERFACE:include>
    INTERFACE ${exported_include_directories}
  )

  set(ns3-external-libs "${non_ns_libraries_to_link};${ns3-external-libs}"
      CACHE INTERNAL
            "list of non-ns libraries to link to NS3_STATIC and NS3_MONOLIB"
  )
  if(${NS3_STATIC} OR ${NS3_MONOLIB})
    set(lib-ns3-static-objs
        "$<TARGET_OBJECTS:${lib${BLIB_LIBNAME}-obj}>;${lib-ns3-static-objs}"
        CACHE
          INTERNAL
          "list of object files from module used by NS3_STATIC and NS3_MONOLIB"
    )
  endif()

  # Write a module header that includes all headers from that module
  write_module_header("${BLIB_LIBNAME}" "${BLIB_HEADER_FILES}")

  # Check if headers actually exist to prevent copying errors during
  # installation
  get_target_property(headers_to_check ${lib${BLIB_LIBNAME}} PUBLIC_HEADER)
  set(missing_headers)
  foreach(header ${headers_to_check})
    if(NOT ((EXISTS ${header}) OR (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${header})
           )
    )
      list(APPEND missing_headers ${header})
    endif()
  endforeach()
  if(missing_headers)
    message(
      FATAL_ERROR "Missing header files for ${BLIB_LIBNAME}: ${missing_headers}"
    )
  endif()

  # Copy all header files to outputfolder/include before each build
  copy_headers_before_building_lib(
    ${BLIB_LIBNAME} ${CMAKE_HEADER_OUTPUT_DIRECTORY} "${BLIB_HEADER_FILES}"
    public
  )
  if(BLIB_PRIVATE_HEADER_FILES)
    copy_headers_before_building_lib(
      ${BLIB_LIBNAME} ${CMAKE_HEADER_OUTPUT_DIRECTORY}
      "${BLIB_PRIVATE_HEADER_FILES}" private
    )
  endif()

  if(BLIB_DEPRECATED_HEADER_FILES)
    copy_headers_before_building_lib(
      ${BLIB_LIBNAME} ${CMAKE_HEADER_OUTPUT_DIRECTORY}
      "${BLIB_DEPRECATED_HEADER_FILES}" deprecated
    )
  endif()

  # Build lib examples if requested
  set(examples_before ${ns3-execs-clean})
  foreach(example_folder example;examples)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${example_folder})
      if(${ENABLE_EXAMPLES})
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${example_folder}/CMakeLists.txt)
          add_subdirectory(${example_folder})
        endif()
      endif()
      scan_python_examples(${CMAKE_CURRENT_SOURCE_DIR}/${example_folder})
    endif()
  endforeach()
  set(module_examples ${ns3-execs-clean})

  # Filter only module examples
  foreach(example ${examples_before})
    list(REMOVE_ITEM module_examples ${example})
  endforeach()
  unset(examples_before)

  # Check if the module tests should be built
  set(filtered_in ON)
  if(NS3_FILTER_MODULE_EXAMPLES_AND_TESTS)
    set(filtered_in OFF)
    if(${BLIB_LIBNAME} IN_LIST NS3_FILTER_MODULE_EXAMPLES_AND_TESTS)
      set(filtered_in ON)
    endif()
  endif()

  # Build tests if requested
  if(${ENABLE_TESTS} AND ${filtered_in})
    list(LENGTH BLIB_TEST_SOURCES test_source_len)
    if(${test_source_len} GREATER 0)
      # Create BLIB_LIBNAME of output library test of module
      set(test${BLIB_LIBNAME} lib${BLIB_LIBNAME}-test CACHE INTERNAL "")

      # Create shared library containing tests of the module on UNIX and just
      # the object file that will be part of test-runner on Windows
      if(WIN32)
        set(ns3-libs-tests
            "$<TARGET_OBJECTS:${test${BLIB_LIBNAME}}>;${ns3-libs-tests}"
            CACHE INTERNAL "list of test libraries"
        )
        add_library(${test${BLIB_LIBNAME}} OBJECT "${BLIB_TEST_SOURCES}")
      else()
        set(ns3-libs-tests "${test${BLIB_LIBNAME}};${ns3-libs-tests}"
            CACHE INTERNAL "list of test libraries"
        )
        add_library(${test${BLIB_LIBNAME}} SHARED "${BLIB_TEST_SOURCES}")

        # Link test library to the module library
        if(${NS3_MONOLIB})
          target_link_libraries(
            ${test${BLIB_LIBNAME}} ${LIB_AS_NEEDED_PRE} ${lib-ns3-monolib}
            ${LIB_AS_NEEDED_POST}
          )
        else()
          target_link_libraries(
            ${test${BLIB_LIBNAME}} ${LIB_AS_NEEDED_PRE} ${lib${BLIB_LIBNAME}}
            "${BLIB_LIBRARIES_TO_LINK}" ${LIB_AS_NEEDED_POST}
          )
        endif()
        set_target_properties(
          ${test${BLIB_LIBNAME}}
          PROPERTIES OUTPUT_NAME
                     ns${NS3_VER}-${BLIB_LIBNAME}-test${build_profile_suffix}
        )
      endif()
      target_compile_definitions(
        ${test${BLIB_LIBNAME}} PRIVATE NS_TEST_SOURCEDIR="${FOLDER}/test"
      )
      if(${PRECOMPILE_HEADERS_ENABLED} AND (NOT ${BLIB_IGNORE_PCH}))
        target_precompile_headers(${test${BLIB_LIBNAME}} REUSE_FROM stdlib_pch)
      endif()

      # Add dependency between tests and examples used as tests
      if(${ENABLE_EXAMPLES})
        foreach(source_file ${BLIB_TEST_SOURCES})
          file(READ ${source_file} source_file_contents)
          foreach(example_as_test ${module_examples})
            string(FIND "${source_file_contents}" "${example_as_test}"
                        is_sub_string
            )
            if(NOT (${is_sub_string} EQUAL -1))
              add_dependencies(test-runner-examples-as-tests ${example_as_test})
            endif()
          endforeach()
        endforeach()
      endif()
    endif()
  endif()

  # Handle package export
  install(
    TARGETS ${lib${BLIB_LIBNAME}}
    EXPORT ns3ExportTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ns3"
    PRIVATE_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ns3"
  )
  if(${NS3_VERBOSE})
    message(STATUS "Processed ${FOLDER}")
  endif()
endfunction()

# cmake-format: off
#
# This macro processes a ns-3 module example
#
# Arguments:
# NAME = example name (e.g. command-line-example)
# LIBNAME = parent library (e.g. core)
# SOURCE_FILES = "cmake;list;of;.cc;files;"
# HEADER_FILES = "cmake;list;of;public;.h;files;"
# LIBRARIES_TO_LINK = "cmake;list;of;${library_names};"
#
function(build_lib_example)
  # Argument parsing
  set(options IGNORE_PCH)
  set(oneValueArgs NAME)
  set(multiValueArgs SOURCE_FILES HEADER_FILES LIBRARIES_TO_LINK)
  cmake_parse_arguments("BLIB_EXAMPLE" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # Get path src/module or contrib/module
  string(REPLACE "${PROJECT_SOURCE_DIR}/" "" FOLDER "${CMAKE_CURRENT_SOURCE_DIR}")

  # cmake-format: on
  check_for_missing_libraries(
    missing_dependencies "${BLIB_EXAMPLE_LIBRARIES_TO_LINK}"
  )

  # Check if a module example should be built
  set(filtered_in ON)
  if(NS3_FILTER_MODULE_EXAMPLES_AND_TESTS)
    set(filtered_in OFF)
    if(${BLIB_LIBNAME} IN_LIST NS3_FILTER_MODULE_EXAMPLES_AND_TESTS)
      set(filtered_in ON)
    endif()
  endif()

  if((NOT missing_dependencies) AND ${filtered_in})
    # Convert boolean into text to forward argument
    if(${BLIB_EXAMPLE_IGNORE_PCH})
      set(IGNORE_PCH IGNORE_PCH)
    endif()
    # Create executable with sources and headers
    # cmake-format: off
    build_exec(
      EXECNAME ${BLIB_EXAMPLE_NAME}
      SOURCE_FILES ${BLIB_EXAMPLE_SOURCE_FILES}
      HEADER_FILES ${BLIB_EXAMPLE_HEADER_FILES}
      LIBRARIES_TO_LINK
        ${lib${BLIB_LIBNAME}} ${BLIB_EXAMPLE_LIBRARIES_TO_LINK}
        ${optional_visualizer_lib}
      EXECUTABLE_DIRECTORY_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${FOLDER}/
      ${IGNORE_PCH}
    )
    # cmake-format: on
  endif()
endfunction()

# This macro processes a ns-3 module header file (module_name-module.h)
#
# Arguments: name = module name (e.g. core, wifi) HEADER_FILES =
# "cmake;list;of;public;.h;files;"
function(write_module_header name header_files)
  string(TOUPPER "${name}" uppercase_name)
  string(REPLACE "-" "_" final_name "${uppercase_name}")
  # Common module_header
  list(APPEND contents "#ifdef NS3_MODULE_COMPILATION ")
  list(
    APPEND
    contents
    "
    error \"Do not include ns3 module aggregator headers from other modules; these are meant only for end user scripts.\" "
  )
  list(APPEND contents "
#endif "
  )
  list(APPEND contents "
#ifndef NS3_MODULE_"
  )
  list(APPEND contents ${final_name})
  list(APPEND contents "
    // Module headers: "
  )

  # Write each header listed to the contents variable
  foreach(header ${header_files})
    get_filename_component(head ${header} NAME)
    list(APPEND contents "
    #include <ns3/${head}>"
    )
    # include \"ns3/${head}\"")
  endforeach()

  # Common module footer
  list(APPEND contents "
#endif "
  )
  if(EXISTS ${CMAKE_HEADER_OUTPUT_DIRECTORY}/${name}-module.h)
    file(READ ${CMAKE_HEADER_OUTPUT_DIRECTORY}/${name}-module.h oldcontents)
    string(REPLACE ";" "" contents "${contents}")
    # If the header-module.h already exists and is the exact same, do not
    # overwrite it to preserve timestamps
    if("${contents}" STREQUAL "${oldcontents}")
      return()
    endif()
  endif()
  file(WRITE ${CMAKE_HEADER_OUTPUT_DIRECTORY}/${name}-module.h ${contents})
endfunction()
