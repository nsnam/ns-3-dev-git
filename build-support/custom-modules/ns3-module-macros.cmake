# Copyright (c) 2017-2021 Universidade de Brasília
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>

add_custom_target(copy_all_headers)
function(copy_headers_before_building_lib libname outputdir headers visibility)
  foreach(header ${headers})
    # Copy header to output directory on changes -> too darn slow
    # configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${header} ${outputdir}/
    # COPYONLY)

    get_filename_component(
      header_name ${CMAKE_CURRENT_SOURCE_DIR}/${header} NAME
    )

    # If output directory does not exist, create it
    if(NOT (EXISTS ${outputdir}))
      make_directory(${outputdir})
    endif()

    # If header already exists, skip symlinking/stub header creation
    if(EXISTS ${outputdir}/${header_name})
      continue()
    endif()

    # Create a stub header in the output directory, including the real header
    # inside their respective module
    get_filename_component(
      ABSOLUTE_HEADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${header}" ABSOLUTE
    )
    file(WRITE ${outputdir}/${header_name}
         "#include \"${ABSOLUTE_HEADER_PATH}\"\n"
    )
  endforeach()
endfunction(copy_headers_before_building_lib)

function(copy_headers)
  # Argument parsing
  set(options)
  set(oneValueArgs PUBLIC_HEADER_OUTPUT_DIR DEPRECATED_HEADER_OUTPUT_DIR
                   PRIVATE_HEADER_OUTPUT_DIR
  )
  set(multiValueArgs PUBLIC_HEADER_FILES DEPRECATED_HEADER_FILES
                     PRIVATE_HEADER_FILES
  )
  cmake_parse_arguments(
    "CP" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
  )
  if((DEFINED CP_PUBLIC_HEADER_OUTPUT_DIR) AND (DEFINED CP_PUBLIC_HEADER_FILES))
    copy_headers_before_building_lib(
      "" "${CP_PUBLIC_HEADER_OUTPUT_DIR}" "${CP_PUBLIC_HEADER_FILES}" public
    )
  endif()
  if((DEFINED CP_DEPRECATED_HEADER_OUTPUT_DIR) AND (DEFINED
                                                    CP_DEPRECATED_HEADER_FILES)
  )
    copy_headers_before_building_lib(
      "" "${CP_DEPRECATED_HEADER_OUTPUT_DIR}" "${CP_DEPRECATED_HEADER_FILES}"
      deprecated
    )
  endif()
  if((DEFINED CP_PRIVATE_HEADER_OUTPUT_DIR) AND (DEFINED CP_PRIVATE_HEADER_FILES
                                                )
  )
    copy_headers_before_building_lib(
      "" "${CP_PRIVATE_HEADER_OUTPUT_DIR}" "${CP_PRIVATE_HEADER_FILES}" private
    )
  endif()
endfunction()

function(remove_lib_prefix prefixed_library library)
  # Check if there is a lib prefix
  string(FIND "${prefixed_library}" "lib" lib_pos)

  # If there is a lib prefix, try to remove it
  if(${lib_pos} EQUAL 0)
    # Check if we still have something remaining after removing the "lib" prefix
    string(LENGTH ${prefixed_library} len)
    if(${len} LESS 4)
      message(FATAL_ERROR "Invalid library name: ${prefixed_library}")
    endif()

    # Remove lib prefix from module name (e.g. libcore -> core)
    string(SUBSTRING "${prefixed_library}" 3 -1 unprefixed_library)
  else()
    set(unprefixed_library ${prefixed_library})
  endif()

  # Save the unprefixed library name to the parent scope
  set(${library} ${unprefixed_library} PARENT_SCOPE)
endfunction()

function(check_for_missing_libraries output_variable_name libraries)
  set(missing_dependencies)
  foreach(lib ${libraries})
    # skip check for ns-3 modules if its a path to a library
    if(EXISTS ${lib})
      continue()
    endif()

    # Match external imported targets out
    if(lib MATCHES "^(ns3::)?[^:]+::.+$")
      continue()
    endif()

    # check if the example depends on disabled modules
    remove_lib_prefix("${lib}" lib)

    # Check if the module exists in the ns-3 modules list or if it is a
    # 3rd-party library
    if(NOT (${lib} IN_LIST ns3-all-enabled-modules))
      list(APPEND missing_dependencies ${lib})
    endif()
  endforeach()
  set(${output_variable_name} ${missing_dependencies} PARENT_SCOPE)
endfunction()

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
# GENERATE_EXPORT_HEADER = TRUE or FALSE, auto-generate a header file to set *_EXPORT symbols for Windows builds
# MODULE_ENABLED_FEATURES = "list;of;enabled;features;for;this;module" (used by fd-net-device)
# cmake-format: on

function(build_lib)
  # Argument parsing
  set(options IGNORE_PCH GENERATE_EXPORT_HEADER)
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
    set(ns3-libs "${BLIB_LIBNAME};${ns3-libs}"
        CACHE INTERNAL "list of processed upstream modules"
    )
  else()
    set(ns3-contrib-libs "${BLIB_LIBNAME};${ns3-contrib-libs}"
        CACHE INTERNAL "list of processed contrib modules"
    )
  endif()

  # Create the module shared library
  add_library(${BLIB_LIBNAME} SHARED "${BLIB_SOURCE_FILES}")

  # Set alias
  add_library(ns3::${BLIB_LIBNAME} ALIAS ${BLIB_LIBNAME})

  set(export_file)
  if(${BLIB_GENERATE_EXPORT_HEADER})
    include(GenerateExportHeader)
    string(TOUPPER "${BLIB_LIBNAME}" uppercase_libname)
    string(
      CONCAT force_undef_export
             "\n// Undefine the *_EXPORT symbols for non-Windows based builds"
             "\n#ifndef NS_MSVC"
             "\n#undef ${uppercase_libname}_EXPORT"
             "\n#define ${uppercase_libname}_EXPORT"
             "\n#undef ${uppercase_libname}_NO_EXPORT"
             "\n#define ${uppercase_libname}_NO_EXPORT"
             "\n#endif"
    )
    set(export_file ${CMAKE_HEADER_OUTPUT_DIRECTORY}/${BLIB_LIBNAME}-export.h)

    generate_export_header(
      ${BLIB_LIBNAME} EXPORT_FILE_NAME ${export_file}
      CUSTOM_CONTENT_FROM_VARIABLE force_undef_export
    )

    # In case building on Visual Studio
    if(${MSVC})
      target_compile_definitions(
        ${BLIB_LIBNAME} PRIVATE ${BLIB_LIBNAME}_EXPORTS
      )
    endif()
  endif()

  # Reuse PCH
  if(${PRECOMPILE_HEADERS_ENABLED} AND (NOT ${BLIB_IGNORE_PCH}))
    target_precompile_headers(${BLIB_LIBNAME} REUSE_FROM stdlib_pch)
  endif()

  # Associate public headers with library for installation purposes
  set_target_properties(
    ${BLIB_LIBNAME}
    PROPERTIES
      PUBLIC_HEADER
      "${BLIB_HEADER_FILES};${BLIB_DEPRECATED_HEADER_FILES};${CMAKE_HEADER_OUTPUT_DIRECTORY}/${BLIB_LIBNAME}-module.h;${export_file}"
      PRIVATE_HEADER "${BLIB_PRIVATE_HEADER_FILES}"
      # set output directory for DLLs
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
      RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
      RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
      RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
      RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
  )

  if(${NS3_CLANG_TIMETRACE})
    add_dependencies(timeTraceReport ${BLIB_LIBNAME})
  endif()

  build_lib_reexport_third_party_libraries(
    "${BLIB_LIBNAME}" "${BLIB_LIBRARIES_TO_LINK}"
  )

  # set output name of library
  set_target_properties(
    ${BLIB_LIBNAME}
    PROPERTIES OUTPUT_NAME ns${NS3_VER}-${BLIB_LIBNAME}${build_profile_suffix}
  )

  # Export compile definitions as interface definitions, propagating local
  # definitions to other modules and scratches
  build_lib_export_definitions_as_interface_definitions(${BLIB_LIBNAME})

  # Write a module header that includes all headers from that module
  write_module_header("${BLIB_LIBNAME}" "${BLIB_HEADER_FILES}")

  # Check if headers actually exist to prevent copying errors during
  # installation (includes the module header created above)
  build_lib_check_headers(${BLIB_LIBNAME})

  # Copy all header files to outputfolder/include before each build
  copy_headers(
    PUBLIC_HEADER_OUTPUT_DIR ${CMAKE_HEADER_OUTPUT_DIRECTORY}
    PUBLIC_HEADER_FILES ${BLIB_HEADER_FILES}
    DEPRECATED_HEADER_OUTPUT_DIR ${CMAKE_HEADER_OUTPUT_DIRECTORY}
    DEPRECATED_HEADER_FILES ${BLIB_DEPRECATED_HEADER_FILES}
    PRIVATE_HEADER_OUTPUT_DIR ${CMAKE_HEADER_OUTPUT_DIRECTORY}
    PRIVATE_HEADER_FILES ${BLIB_PRIVATE_HEADER_FILES}
  )

  # Scan for C++ and Python examples and return a list of C++ examples (which
  # can be set as dependencies of examples-as-test test suites)
  build_lib_scan_examples(module_examples)

  # Build tests if requested
  build_lib_tests(
    "${BLIB_LIBNAME}" "${BLIB_IGNORE_PCH}" "${FOLDER}" "${BLIB_TEST_SOURCES}"
  )

  if(${MSVC})
    target_link_options(${lib${BLIB_LIBNAME}} PUBLIC "/OPT:NOREF")
  endif()

  # Handle package export
  install(
    TARGETS ${BLIB_LIBNAME}
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

function(build_lib_reexport_third_party_libraries libname libraries_to_link)
  # Separate ns-3 and non-ns-3 libraries to manage their propagation properly
  separate_ns3_from_non_ns3_libs(
    "${libname}" "${libraries_to_link}" ns_libraries_to_link
    non_ns_libraries_to_link
  )

  set(ns3-external-libs "${non_ns_libraries_to_link};${ns3-external-libs}"
      CACHE INTERNAL
            "list of non-ns libraries to link to NS3_STATIC and NS3_MONOLIB"
  )

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
    get_target_includes(${libname} exported_include_directories)

    string(REPLACE "-I" "" exported_include_directories
                   "${exported_include_directories}"
    )

    # include directories prefixed in the source or binary directory need to be
    # treated differently
    set(new_exported_include_directories)
    foreach(directory ${exported_include_directories})
      string(FIND "${directory}" "${PROJECT_SOURCE_DIR}" is_prefixed_in_subdir)
      if(${is_prefixed_in_subdir} GREATER_EQUAL 0)
        string(SUBSTRING "${directory}" ${is_prefixed_in_subdir} -1
                         directory_path
        )
        list(APPEND new_exported_include_directories
             $<BUILD_INTERFACE:${directory_path}>
        )
      else()
        list(APPEND new_exported_include_directories ${directory})
      endif()
    endforeach()
    set(exported_include_directories ${new_exported_include_directories})

    string(REPLACE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/include" ""
                   exported_include_directories
                   "${exported_include_directories}"
    )
  endif()
  # Set public and private headers linked to the module library
  target_link_libraries(${libname} ${exported_libraries} ${private_libraries})

  # export include directories used by this library so that it can be used by
  # 3rd-party consumers of ns-3 using find_package(ns3) this will automatically
  # add the build/include path to them, so that they can ns-3 headers with
  # <ns3/something.h>
  target_include_directories(
    ${libname} PUBLIC $<BUILD_INTERFACE:${CMAKE_OUTPUT_DIRECTORY}/include>
                      $<INSTALL_INTERFACE:include>
    INTERFACE ${exported_include_directories}
  )
endfunction()

function(build_lib_export_definitions_as_interface_definitions libname)
  get_target_property(target_definitions ${libname} COMPILE_DEFINITIONS)
  if("${target_definitions}" STREQUAL "target_definitions-NOTFOUND")
    set(target_definitions)
  endif()
  get_directory_property(dir_definitions COMPILE_DEFINITIONS)
  set(exported_definitions "${target_definitions};${dir_definitions}")
  list(REMOVE_DUPLICATES exported_definitions)
  list(REMOVE_ITEM exported_definitions "")
  set_target_properties(
    ${libname} PROPERTIES INTERFACE_COMPILE_DEFINITIONS
                          "${exported_definitions}"
  )
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
  build_lib_check_examples_and_tests_filtered_in(${BLIB_LIBNAME} filtered_in)

  if((NOT missing_dependencies) AND ${filtered_in})
    # Convert boolean into text to forward argument
    set(IGNORE_PCH)
    if(${BLIB_EXAMPLE_IGNORE_PCH})
      set(IGNORE_PCH "IGNORE_PCH")
    endif()
    # Create executable with sources and headers
    # cmake-format: off
    build_exec(
      EXECNAME ${BLIB_EXAMPLE_NAME}
      SOURCE_FILES ${BLIB_EXAMPLE_SOURCE_FILES}
      HEADER_FILES ${BLIB_EXAMPLE_HEADER_FILES}
      LIBRARIES_TO_LINK
        ${BLIB_LIBNAME} ${BLIB_EXAMPLE_LIBRARIES_TO_LINK}
        ${ns3-optional-visualizer-lib}
      EXECUTABLE_DIRECTORY_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${FOLDER}/
      ${IGNORE_PCH}
    )
    # cmake-format: on
  endif()
endfunction()

# Check if module examples and tests should be built or not
#
# Arguments: libname (e.g. core, wifi), filtered_in (return boolean)
function(build_lib_check_examples_and_tests_filtered_in libname filtered_in)
  set(in ON)
  if(NS3_FILTER_MODULE_EXAMPLES_AND_TESTS)
    set(in OFF)
    if(${libname} IN_LIST NS3_FILTER_MODULE_EXAMPLES_AND_TESTS)
      set(in ON)
    endif()
  endif()
  set(${filtered_in} ${in} PARENT_SCOPE)
endfunction()

# Separate the LIBRARIES_TO_LINK list into ns-3 modules and external libraries
#
# Arguments: libname (e.g. core, wifi), libraries_to_link (input list),
# ns_libraries_to_link and non_ns_libraries_to_link (output lists)
function(separate_ns3_from_non_ns3_libs libname libraries_to_link
         ns_libraries_to_link non_ns_libraries_to_link
)
  set(non_ns_libs)
  if(DEFINED CMAKE_THREAD_LIBS_INIT)
    list(APPEND non_ns_libs ${CMAKE_THREAD_LIBS_INIT})
  endif()
  set(ns_libs)
  foreach(library ${libraries_to_link})
    remove_lib_prefix("${library}" module_name)

    # In case the dependency library matches the ns-3 module, we are most likely
    # dealing with brite, click and openflow collisions. All the ns-3 module
    # targets used to be prefixed with 'lib' to be differentiable, but now we
    # are dropping it. To disambiguate them two, we assume these external
    # libraries are shared libraries by adding suffixes.
    if("${library}" STREQUAL "${libname}")
      list(APPEND non_ns_libs ${library}${CMAKE_SHARED_LIBRARY_SUFFIX})
      continue()
    endif()

    # Check if the module exists in the ns-3 modules list or if it is a
    # 3rd-party library
    if(${module_name} IN_LIST ns3-all-enabled-modules)
      list(APPEND ns_libs ${library})
    else()
      list(APPEND non_ns_libs ${library})
    endif()
    unset(module_name)
  endforeach()
  set(${ns_libraries_to_link} ${ns_libs} PARENT_SCOPE)
  set(${non_ns_libraries_to_link} ${non_ns_libs} PARENT_SCOPE)
endfunction()

# This macro scans for C++ and Python examples for a given module and return a
# list of C++ examples
#
# Arguments: module_cpp_examples = return list of C++ examples
function(build_lib_scan_examples module_cpp_examples)
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

  # Return a list of module c++ examples (current examples - previous examples)
  list(REMOVE_ITEM module_examples "${examples_before}")
  set(${module_cpp_examples} ${module_examples} PARENT_SCOPE)
endfunction()

# This macro builds the test library for the module library
#
# Arguments: libname (e.g. core), ignore_pch (TRUE/FALSE), folder (src/contrib),
# sources (list of .cc's)
function(build_lib_tests libname ignore_pch folder test_sources)
  if(${ENABLE_TESTS})
    # Check if the module tests should be built
    build_lib_check_examples_and_tests_filtered_in(${libname} filtered_in)
    if(NOT ${filtered_in})
      return()
    endif()
    list(LENGTH test_sources test_source_len)
    if(${test_source_len} GREATER 0)
      # Create libname of output library test of module
      set(test${libname} ${libname}-test CACHE INTERNAL "")

      # Create shared library containing tests of the module on UNIX and just
      # the object file that will be part of test-runner on Windows
      if(WIN32)
        set(ns3-libs-tests
            "$<TARGET_OBJECTS:${test${libname}}>;${ns3-libs-tests}"
            CACHE INTERNAL "list of test libraries"
        )
        add_library(${test${libname}} OBJECT "${test_sources}")
      else()
        set(ns3-libs-tests "${test${libname}};${ns3-libs-tests}"
            CACHE INTERNAL "list of test libraries"
        )
        add_library(${test${libname}} SHARED "${test_sources}")

        # Link test library to the module library
        if(${NS3_MONOLIB})
          target_link_libraries(
            ${test${libname}} ${LIB_AS_NEEDED_PRE} ${lib-ns3-monolib}
            ${LIB_AS_NEEDED_POST}
          )
        else()
          target_link_libraries(
            ${test${libname}} ${LIB_AS_NEEDED_PRE} ${libname}
            "${BLIB_LIBRARIES_TO_LINK}" ${LIB_AS_NEEDED_POST}
          )
        endif()
        set_target_properties(
          ${test${libname}}
          PROPERTIES OUTPUT_NAME
                     ns${NS3_VER}-${libname}-test${build_profile_suffix}
        )
      endif()
      target_compile_definitions(
        ${test${libname}} PRIVATE NS_TEST_SOURCEDIR="${folder}/test"
      )
      if(${PRECOMPILE_HEADERS_ENABLED} AND (NOT ${ignore_pch}))
        target_precompile_headers(${test${libname}} REUSE_FROM stdlib_pch)
      endif()

      # Add dependency between tests and examples used as tests
      examples_as_tests_dependencies("${module_examples}" "${test_sources}")
    endif()
  endif()
endfunction()

# This macro scans for C++ examples used by examples-as-tests suites
#
# Arguments: module_cpp_examples = list of C++ example executable names,
# module_test_sources = list of C++ sources with tests
function(examples_as_tests_dependencies module_cpp_examples module_test_sources)
  if(${ENABLE_EXAMPLES})
    foreach(source_file ${module_test_sources})
      file(READ ${source_file} source_file_contents)
      foreach(example_as_test ${module_cpp_examples})
        string(FIND "${source_file_contents}" "${example_as_test}"
                    is_sub_string
        )
        if(NOT (${is_sub_string} EQUAL -1))
          add_dependencies(test-runner-examples-as-tests ${example_as_test})
        endif()
      endforeach()
    endforeach()
  endif()
endfunction()

# This macro checks if all headers from a module actually exist or are missing
#
# Arguments: target_name = module name (e.g. core, wifi)
function(build_lib_check_headers target_name)
  # Retrieve target properties containing the public (which include deprecated)
  # and private headers
  get_target_property(headers_to_check ${target_name} PUBLIC_HEADER)
  get_target_property(headers_to_check2 ${target_name} PRIVATE_HEADER)
  list(APPEND headers_to_check ${headers_to_check2})
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
      FATAL_ERROR "Missing header files for ${target_name}: ${missing_headers}"
    )
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
