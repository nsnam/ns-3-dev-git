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
# folder = src or contrib/contributor_module
# libname = core, wifi, contributor_module
# source_files = "list;of;.cc;files;"
# header_files = "list;of;public;.h;files;"
# libraries_to_link = "list;of;${library_names};"
# test_sources = "list;of;.cc;test;files;"
#
# Hidden argument (this is not a function, so you don't really need to pass arguments explicitly)
# deprecated_header_files = "list;of;deprecated;.h;files", copy won't get triggered if deprecated_header_files isn't set
# ignore_pch = TRUE or FALSE, prevents the PCH from including undesired system libraries (e.g. custom GLIBC for DCE)
# module_enabled_features = "list;of;enabled;features;for;this;module" (used by fd-net-device)

macro(
  build_lib_impl
  folder
  libname
  source_files
  header_files
  libraries_to_link
  test_sources
  #deprecated_header_files
  #ignore_pch
  #module_enabled_features
)
  # cmake-format: on

  # Add library to a global list of libraries
  if("${folder}" MATCHES "src")
    set(ns3-libs "${lib${libname}};${ns3-libs}"
        CACHE INTERNAL "list of processed upstream modules"
    )
  else()
    set(ns3-contrib-libs "${lib${libname}};${ns3-contrib-libs}"
        CACHE INTERNAL "list of processed contrib modules"
    )
  endif()

  if(NOT ${XCODE})
    # Create object library with sources and headers, that will be used in
    # lib-ns3-static and the shared library
    add_library(${lib${libname}-obj} OBJECT "${source_files}" "${header_files}")

    if(${PRECOMPILE_HEADERS_ENABLED} AND (NOT ${ignore_pch}))
      target_precompile_headers(${lib${libname}-obj} REUSE_FROM stdlib_pch)
    endif()

    # Create shared library with previously created object library (saving
    # compilation time for static libraries)
    add_library(${lib${libname}} SHARED $<TARGET_OBJECTS:${lib${libname}-obj}>)
  else()
    # Xcode and CMake don't play well when using object libraries, so we have a
    # specific path for that
    add_library(${lib${libname}} SHARED "${source_files}")

    if(${PRECOMPILE_HEADERS_ENABLED} AND (NOT ${ignore_pch}))
      target_precompile_headers(${lib${libname}} REUSE_FROM stdlib_pch)
    endif()
  endif()

  add_library(ns3::${lib${libname}} ALIAS ${lib${libname}})

  # Associate public headers with library for installation purposes
  if("${libname}" STREQUAL "core")
    set(config_headers ${CMAKE_HEADER_OUTPUT_DIRECTORY}/config-store-config.h
                       ${CMAKE_HEADER_OUTPUT_DIRECTORY}/core-config.h
    )
    if(${NS3_ENABLE_BUILD_VERSION})
      list(APPEND config_headers
           ${CMAKE_HEADER_OUTPUT_DIRECTORY}/version-defines.h
      )
    endif()
  endif()
  set_target_properties(
    ${lib${libname}}
    PROPERTIES
      PUBLIC_HEADER
      "${header_files};${deprecated_header_files};${config_headers};${CMAKE_HEADER_OUTPUT_DIRECTORY}/${libname}-module.h"
  )

  if(${NS3_CLANG_TIMETRACE})
    add_dependencies(timeTraceReport ${lib${libname}})
  endif()

  # Split ns and non-ns libraries to manage their propagation properly
  set(non_ns_libraries_to_link)
  set(ns_libraries_to_link)

  foreach(library ${libraries_to_link})
    # Remove lib prefix from module name (e.g. libcore -> core)
    string(REPLACE "lib" "" module_name "${library}")
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
    get_target_includes(${lib${libname}} exported_include_directories)
    string(REPLACE "-I" "" exported_include_directories
                   "${exported_include_directories}"
    )
    string(REPLACE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/include" ""
                   exported_include_directories
                   "${exported_include_directories}"
    )
  endif()

  target_link_libraries(
    ${lib${libname}} ${exported_libraries} ${private_libraries}
  )

  # set output name of library
  set_target_properties(
    ${lib${libname}} PROPERTIES OUTPUT_NAME
                                ns${NS3_VER}-${libname}${build_profile_suffix}
  )

  # export include directories used by this library so that it can be used by
  # 3rd-party consumers of ns-3 using find_package(ns3) this will automatically
  # add the build/include path to them, so that they can ns-3 headers with
  # <ns3/something.h>
  target_include_directories(
    ${lib${libname}} PUBLIC $<BUILD_INTERFACE:${CMAKE_OUTPUT_DIRECTORY}/include>
                            $<INSTALL_INTERFACE:include>
    INTERFACE ${exported_include_directories}
  )

  set(ns3-external-libs "${non_ns_libraries_to_link};${ns3-external-libs}"
      CACHE INTERNAL
            "list of non-ns libraries to link to NS3_STATIC and NS3_MONOLIB"
  )
  if(${NS3_STATIC} OR ${NS3_MONOLIB})
    set(lib-ns3-static-objs
        "$<TARGET_OBJECTS:${lib${libname}-obj}>;${lib-ns3-static-objs}"
        CACHE
          INTERNAL
          "list of object files from module used by NS3_STATIC and NS3_MONOLIB"
    )
  endif()

  # Write a module header that includes all headers from that module
  write_module_header("${libname}" "${header_files}")

  # Copy all header files to outputfolder/include before each build
  copy_headers_before_building_lib(
    ${libname} ${CMAKE_HEADER_OUTPUT_DIRECTORY} "${header_files}" public
  )
  if(deprecated_header_files)
    copy_headers_before_building_lib(
      ${libname} ${CMAKE_HEADER_OUTPUT_DIRECTORY} "${deprecated_header_files}"
      deprecated
    )
  endif()

  # Build tests if requested
  if(${ENABLE_TESTS})
    list(LENGTH test_sources test_source_len)
    if(${test_source_len} GREATER 0)
      # Create libname of output library test of module
      set(test${libname} lib${libname}-test CACHE INTERNAL "")
      set(ns3-libs-tests "${test${libname}};${ns3-libs-tests}"
          CACHE INTERNAL "list of test libraries"
      )

      # Create shared library containing tests of the module
      add_library(${test${libname}} SHARED "${test_sources}")

      # Link test library to the module library
      if(${NS3_MONOLIB})
        target_link_libraries(
          ${test${libname}} ${LIB_AS_NEEDED_PRE} ${lib-ns3-monolib}
          ${LIB_AS_NEEDED_POST}
        )
      else()
        target_link_libraries(
          ${test${libname}} ${LIB_AS_NEEDED_PRE} ${lib${libname}}
          "${libraries_to_link}" ${LIB_AS_NEEDED_POST}
        )
      endif()
      set_target_properties(
        ${test${libname}}
        PROPERTIES OUTPUT_NAME
                   ns${NS3_VER}-${libname}-test${build_profile_suffix}
      )

      target_compile_definitions(
        ${test${libname}} PRIVATE NS_TEST_SOURCEDIR="${folder}/${libname}/test"
      )
      if(${PRECOMPILE_HEADERS_ENABLED} AND (NOT ${ignore_pch}))
        target_precompile_headers(${test${libname}} REUSE_FROM stdlib_pch)
      endif()
    endif()
  endif()

  # Build lib examples if requested
  if(${ENABLE_EXAMPLES})
    foreach(example_folder example;examples)
      if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${example_folder})
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${example_folder}/CMakeLists.txt)
          add_subdirectory(${example_folder})
        endif()
        scan_python_examples(${CMAKE_CURRENT_SOURCE_DIR}/${example_folder})
      endif()
    endforeach()
  endif()

  # Get architecture pair for python bindings
  if((${CMAKE_SIZEOF_VOID_P} EQUAL 8) AND (NOT APPLE))
    set(arch gcc_LP64)
    set(arch_flags -m64)
  else()
    set(arch gcc_ILP32)
    set(arch_flags)
  endif()

  # Add target to scan python bindings
  if(${ENABLE_SCAN_PYTHON_BINDINGS}
     AND EXISTS ${CMAKE_HEADER_OUTPUT_DIRECTORY}/${libname}-module.h
  )
    set(bindings_output_folder
        ${PROJECT_SOURCE_DIR}/${folder}/${libname}/bindings
    )
    file(MAKE_DIRECTORY ${bindings_output_folder})
    set(module_api_ILP32 ${bindings_output_folder}/modulegen__gcc_ILP32.py)
    set(module_api_LP64 ${bindings_output_folder}/modulegen__gcc_LP64.py)

    set(modulescan_modular_command
        ${Python_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/bindings/python/ns3modulescan-modular.py
    )

    set(header_map "")
    # We need a python map that takes header.h to module e.g. "ptr.h": "core"
    foreach(header ${header_files})
      # header is a relative path to the current working directory
      get_filename_component(
        header_name ${CMAKE_CURRENT_SOURCE_DIR}/${header} NAME
      )
      string(APPEND header_map "\"${header_name}\":\"${libname}\",")
    endforeach()

    set(ns3-headers-to-module-map "${ns3-headers-to-module-map}${header_map}"
        CACHE INTERNAL "Map connecting headers to their modules"
    )

    # API scan needs the include directories to find a few headers (e.g. mpi.h)
    get_target_includes(${lib${libname}} modulegen_include_dirs)

    set(module_to_generate_api ${module_api_ILP32})
    set(LP64toILP32)
    if("${arch}" STREQUAL "gcc_LP64")
      set(module_to_generate_api ${module_api_LP64})
      set(LP64toILP32
          ${Python_EXECUTABLE}
          ${PROJECT_SOURCE_DIR}/buildsupport/pybindings_LP64_to_ILP32.py
          ${module_api_LP64} ${module_api_ILP32}
      )
    endif()

    add_custom_target(
      ${lib${libname}}-apiscan
      COMMAND
        ${modulescan_modular_command} ${CMAKE_OUTPUT_DIRECTORY} ${libname}
        ${PROJECT_BINARY_DIR}/header_map.json ${module_to_generate_api}
        \"${arch_flags} ${modulegen_include_dirs}\"
      COMMAND ${LP64toILP32}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      DEPENDS ${lib${libname}}
    )
    add_dependencies(apiscan-all ${lib${libname}}-apiscan)
  endif()

  # Build pybindings if requested and if bindings subfolder exists in
  # NS3/src/libname
  if(${ENABLE_PYTHON_BINDINGS} AND EXISTS
                                   "${CMAKE_CURRENT_SOURCE_DIR}/bindings"
  )
    set(bindings_output_folder
        ${CMAKE_OUTPUT_DIRECTORY}/${folder}/${libname}/bindings
    )
    file(MAKE_DIRECTORY ${bindings_output_folder})
    set(module_src ${bindings_output_folder}/ns3module.cc)
    set(module_hdr ${bindings_output_folder}/ns3module.h)

    string(REPLACE "-" "_" libname_sub ${libname}) # '-' causes problems (e.g.
                                                   # csma-layout), replace with
                                                   # '_' (e.g. csma_layout)

    # Set prefix of binding to _ if a ${libname}.py exists, and copy the
    # ${libname}.py to the output folder
    set(prefix)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bindings/${libname}.py)
      set(prefix _)
      file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/bindings/${libname}.py
           DESTINATION ${CMAKE_OUTPUT_DIRECTORY}/bindings/python/ns
      )
    endif()

    # Run modulegen-modular to generate the bindings sources
    if((NOT EXISTS ${module_hdr}) OR (NOT EXISTS ${module_src})) # OR TRUE) # to
                                                                 # force
                                                                 # reprocessing
      string(REPLACE ";" "," ENABLED_FEATURES
                     "${ns3-libs};${module_enabled_features}"
      )
      set(modulegen_modular_command
          GCC_RTTI_ABI_COMPLETE=True NS3_ENABLED_FEATURES="${ENABLED_FEATURES}"
          ${Python_EXECUTABLE}
          ${PROJECT_SOURCE_DIR}/bindings/python/ns3modulegen-modular.py
      )
      execute_process(
        COMMAND
          ${CMAKE_COMMAND} -E env
          PYTHONPATH=${CMAKE_OUTPUT_DIRECTORY}:$ENV{PYTHONPATH}
          ${modulegen_modular_command} ${CMAKE_CURRENT_SOURCE_DIR} ${arch}
          ${prefix}${libname_sub} ${module_src}
        TIMEOUT 60
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_FILE ${module_hdr}
        ERROR_FILE ${bindings_output_folder}/ns3modulegen.log
        RESULT_VARIABLE error_code
      )
      if(${error_code} OR NOT (EXISTS ${module_hdr}))
        message(
          FATAL_ERROR
            "Something went wrong during processing of the python bindings of module ${libname}."
            " Make sure you have the latest version of Pybindgen."
        )
        if(EXISTS ${module_src})
          file(REMOVE ${module_src})
        endif()
      endif()
    endif()

    # Add core module helper sources
    set(python_module_files ${module_hdr} ${module_src})
    if(${libname} STREQUAL "core")
      list(APPEND python_module_files
           ${CMAKE_CURRENT_SOURCE_DIR}/bindings/module_helpers.cc
           ${CMAKE_CURRENT_SOURCE_DIR}/bindings/scan-header.h
      )
    endif()
    set(bindings-name lib${libname}-bindings)
    add_library(${bindings-name} SHARED "${python_module_files}")
    target_include_directories(
      ${bindings-name} PUBLIC ${Python_INCLUDE_DIRS} ${bindings_output_folder}
    )
    target_compile_options(${bindings-name} PRIVATE -Wno-error)

    # If there is any, remove the "lib" prefix of libraries (search for
    # "set(lib${libname}")
    list(LENGTH ns_libraries_to_link num_libraries)
    if(num_libraries GREATER "0")
      string(REPLACE ";" "-bindings;" bindings_to_link
                     "${ns_libraries_to_link};"
      ) # add -bindings suffix to all lib${name}
    endif()
    target_link_libraries(
      ${bindings-name}
      PUBLIC ${LIB_AS_NEEDED_PRE} ${lib${libname}} "${bindings_to_link}"
             "${libraries_to_link}" ${LIB_AS_NEEDED_POST}
      PRIVATE ${Python_LIBRARIES}
    )
    target_include_directories(
      ${bindings-name} PRIVATE ${PROJECT_SOURCE_DIR}/src/core/bindings
    )

    set(suffix)
    if(APPLE)
      # Python doesn't like Apple's .dylib and will refuse to load bindings
      # unless its an .so
      set(suffix SUFFIX .so)
    endif()

    # Set binding library name and output folder
    set_target_properties(
      ${bindings-name}
      PROPERTIES OUTPUT_NAME ${prefix}${libname_sub}
                 PREFIX ""
                 ${suffix} LIBRARY_OUTPUT_DIRECTORY
                 ${CMAKE_OUTPUT_DIRECTORY}/bindings/python/ns
    )

    set(ns3-python-bindings-modules
        "${bindings-name};${ns3-python-bindings-modules}"
        CACHE INTERNAL "list of modules python bindings"
    )

    # Make sure all bindings are built before building the visualizer module
    # that makes use of them
    if(${ENABLE_VISUALIZER})
      if(NOT (${name} STREQUAL visualizer))
        add_dependencies(${libvisualizer} ${bindings-name})
      endif()
    endif()
  endif()

  # Handle package export
  install(
    TARGETS ${lib${name}}
    EXPORT ns3ExportTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ns3"
  )
  if(${NS3_VERBOSE})
    message(STATUS "Processed ${folder}/${libname}")
  endif()
endmacro()

# cmake-format: off
#
# This macro processes a ns-3 module example
#
# Arguments: folder = src or contrib/contributor_module libname = core, wifi, contributor_module (this is implicit, as
# it is called by build_lib_impl)
# name = example name (e.g. command-line-example)
# source_files = "cmake;list;of;.cc;files;"
# header_files = "cmake;list;of;public;.h;files;"
# libraries_to_link = "cmake;list;of;${library_names};"
#
macro(
  build_lib_example_impl
  folder
  name
  source_files
  header_files
  libraries_to_link
)
  # cmake-format: on
  set(missing_dependencies FALSE)
  foreach(lib ${libraries_to_link})
    # skip check for ns-3 modules if its a path to a library
    if(EXISTS ${lib})
      continue()
    endif()

    # check if the example depends on disabled modules
    string(REPLACE "lib" "" lib ${lib})
    if(NOT (${lib} IN_LIST ns3-all-enabled-modules))
      set(missing_dependencies TRUE)
    endif()
  endforeach()

  if(NOT missing_dependencies)
    # Create shared library with sources and headers
    add_executable(${name} "${source_files}" "${header_files}")

    if(${NS3_STATIC})
      target_link_libraries(
        ${name} ${LIB_AS_NEEDED_PRE_STATIC} ${lib-ns3-static}
      )
    elseif(${NS3_MONOLIB})
      target_link_libraries(
        ${name} ${LIB_AS_NEEDED_PRE} ${lib-ns3-monolib} ${LIB_AS_NEEDED_POST}
      )
    else()
      target_link_libraries(
        ${name} ${LIB_AS_NEEDED_PRE} ${lib${libname}} ${libraries_to_link}
        ${optional_visualizer_lib} ${LIB_AS_NEEDED_POST}
      )
    endif()

    if(${PRECOMPILE_HEADERS_ENABLED} AND (NOT ${ignore_pch}))
      target_precompile_headers(${name} REUSE_FROM stdlib_pch_exec)
    endif()

    set_runtime_outputdirectory(
      ${name} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${folder}/examples/ ""
    )
  endif()
endmacro()

# This macro processes a ns-3 module header file (module_name-module.h)
#
# Arguments: name = module name (e.g. core, wifi) header_files =
# "cmake;list;of;public;.h;files;"
macro(write_module_header name header_files)
  string(TOUPPER ${name} uppercase_name)
  string(REPLACE "-" "_" final_name ${uppercase_name})
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
  file(WRITE ${CMAKE_HEADER_OUTPUT_DIRECTORY}/${name}-module.h ${contents})
endmacro()
