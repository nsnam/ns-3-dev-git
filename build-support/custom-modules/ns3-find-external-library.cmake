# Copyright (c) 2023 Universidade de Brasília
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>

# This is our own variant of find_package, for CMake and non-CMake based
# projects
function(log_find_searched_paths)
  # Parse arguments
  set(options)
  set(oneValueArgs TARGET_TYPE TARGET_NAME SEARCH_RESULT SEARCH_SYSTEM_PREFIX)
  set(multiValueArgs SEARCH_PATHS SEARCH_SUFFIXES)
  cmake_parse_arguments(
    "LOGFIND" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
  )

  # Get searched paths and add cmake_system_prefix_path if not explicitly marked
  # not to include it
  set(tsearch_paths ${LOGFIND_SEARCH_PATHS})
  if("${LOGFIND_SEARCH_SYSTEM_PREFIX}" STREQUAL "")
    list(APPEND tsearch_paths "${CMAKE_SYSTEM_PREFIX_PATH}")
  endif()

  set(log_find
      "Looking for ${LOGFIND_TARGET_TYPE} ${LOGFIND_TARGET_NAME} in:\n"
  )
  # For each search path and suffix combination, print a line
  foreach(tsearch_path ${tsearch_paths})
    foreach(suffix ${LOGFIND_SEARCH_SUFFIXES})
      string(APPEND log_find
             "\t${tsearch_path}${suffix}/${LOGFIND_TARGET_NAME}\n"
      )
    endforeach()
  endforeach()

  # Add a final line saying if the file was found and where, or if it was not
  # found
  if("${${LOGFIND_SEARCH_RESULT}}" STREQUAL "${LOGFIND_SEARCH_RESULT}-NOTFOUND")
    string(APPEND log_find
           "\n\t${LOGFIND_TARGET_TYPE} ${LOGFIND_TARGET_NAME} was not found\n"
    )
  else()
    string(
      APPEND
      log_find
      "\n\t${LOGFIND_TARGET_TYPE} ${LOGFIND_TARGET_NAME} was found in ${${LOGFIND_SEARCH_RESULT}}\n"
    )
  endif()

  # Replace duplicate path separators
  string(REPLACE "//" "/" log_find "${log_find}")

  # Print find debug message similar to the one produced by
  # CMAKE_FIND_DEBUG_MODE=true in CMake >= 3.17
  message(STATUS ${log_find})
endfunction()

function(find_external_library)
  # Parse arguments
  set(options QUIET)
  set(oneValueArgs DEPENDENCY_NAME HEADER_NAME LIBRARY_NAME OUTPUT_VARIABLE)
  set(multiValueArgs HEADER_NAMES LIBRARY_NAMES PATH_SUFFIXES SEARCH_PATHS)
  cmake_parse_arguments(
    "FIND_LIB" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
  )

  # Set the external package/dependency name
  set(name ${FIND_LIB_DEPENDENCY_NAME})

  # We process individual and list of headers and libraries by transforming them
  # into lists
  set(library_names "${FIND_LIB_LIBRARY_NAME};${FIND_LIB_LIBRARY_NAMES}")
  set(header_names "${FIND_LIB_HEADER_NAME};${FIND_LIB_HEADER_NAMES}")

  # Just changing the parsed argument name back to something shorter
  set(search_paths ${FIND_LIB_SEARCH_PATHS})
  set(path_suffixes "${FIND_LIB_PATH_SUFFIXES}")

  set(not_found_libraries)
  set(library_dirs)
  set(libraries)

  # Include parent directories in the search paths to handle Bake cases
  get_filename_component(parent_project_dir ${PROJECT_SOURCE_DIR} DIRECTORY)
  get_filename_component(
    grandparent_project_dir ${parent_project_dir} DIRECTORY
  )
  set(project_parent_dirs ${parent_project_dir} ${grandparent_project_dir})

  # Paths and suffixes where libraries will be searched on
  disable_cmake_warnings()
  set(library_search_paths
      ${search_paths}
      ${project_parent_dirs}
      ${CMAKE_OUTPUT_DIRECTORY} # Search for libraries in ns-3-dev/build
      ${CMAKE_INSTALL_PREFIX} # Search for libraries in the install directory
      # (e.g. /usr/)
      $ENV{LD_LIBRARY_PATH} # Search for libraries in LD_LIBRARY_PATH
      # directories
      $ENV{PATH} # Search for libraries in PATH directories
  )
  if(library_search_paths)
    list(REMOVE_DUPLICATES library_search_paths)
  endif()
  enable_cmake_warnings()
  # cmake-format: off
    #
    # Split : separated entries from environment variables
    # by replacing separators with ;
    #
    # cmake-format: on
  string(REPLACE ":" ";" library_search_paths "${library_search_paths}")

  set(suffixes /build /lib /build/lib / /bin ${path_suffixes})

  # For each of the library names in LIBRARY_NAMES or LIBRARY_NAME
  foreach(library ${library_names})
    # We mark this value is advanced not to pollute the configuration with
    # ccmake with the cache variables used internally
    mark_as_advanced(${name}_library_internal_${library})

    # We search for the library named ${library} and store the results in
    # ${name}_library_internal_${library}
    find_library(
      ${name}_library_internal_${library} ${library}
      HINTS ${library_search_paths} PATH_SUFFIXES ${suffixes}
    )
    # cmake-format: off
        # Note: the PATH_SUFFIXES above apply to *ALL* PATHS and HINTS Which
        # translates to CMake searching on standard library directories
        # CMAKE_SYSTEM_PREFIX_PATH, user-settable CMAKE_PREFIX_PATH or
        # CMAKE_LIBRARY_PATH and the directories listed above
        #
        # e.g.  from Ubuntu 22.04 CMAKE_SYSTEM_PREFIX_PATH =
        # /usr/local;/usr;/;/usr/local;/usr/X11R6;/usr/pkg;/opt
        #
        # Searched directories without suffixes
        #
        # ${CMAKE_SYSTEM_PREFIX_PATH}[0] = /usr/local/
        # ${CMAKE_SYSTEM_PREFIX_PATH}[1] = /usr
        # ${CMAKE_SYSTEM_PREFIX_PATH}[2] = /
        # ...
        # ${CMAKE_SYSTEM_PREFIX_PATH}[6] = /opt
        # ${LD_LIBRARY_PATH}[0]
        # ...
        # ${LD_LIBRARY_PATH}[m]
        # ${PATH}[0]
        # ...
        # ${PATH}[m]
        #
        # Searched directories with suffixes include all of the directories above
        # plus all suffixes
        # PATH_SUFFIXES /build /lib /build/lib / /bin # ${path_suffixes}
        #
        # /usr/local/build
        # /usr/local/lib
        # /usr/local/build/lib
        # /usr/local/bin
        # ...
        #
        # cmake-format: on
    # Or enable NS3_VERBOSE to print the searched paths

    # Print tested paths to the searched library and if it was found
    if(${NS3_VERBOSE} AND (${CMAKE_VERSION} VERSION_LESS "3.17.0"))
      log_find_searched_paths(
        TARGET_TYPE
        Library
        TARGET_NAME
        ${library}
        SEARCH_RESULT
        ${name}_library_internal_${library}
        SEARCH_PATHS
        ${library_search_paths}
        SEARCH_SUFFIXES
        ${suffixes}
      )
    endif()

    # After searching the library, the internal variable should have either the
    # absolute path to the library or the name of the variable appended with
    # -NOTFOUND
    if("${${name}_library_internal_${library}}" STREQUAL
       "${name}_library_internal_${library}-NOTFOUND"
    )
      # We keep track of libraries that were not found
      list(APPEND not_found_libraries ${library})
    else()
      # We get the name of the parent directory of the library and append the
      # library to a list of found libraries
      get_filename_component(
        ${name}_library_dir_internal ${${name}_library_internal_${library}}
        DIRECTORY
      ) # e.g. lib/openflow.(so|dll|dylib|a) -> lib
      list(APPEND library_dirs ${${name}_library_dir_internal})
      list(APPEND libraries ${${name}_library_internal_${library}})
    endif()
  endforeach()

  # For each library that was found (e.g. /usr/lib/pthread.so), get their parent
  # directory (/usr/lib) and its parent (/usr)
  set(parent_dirs)
  foreach(libdir ${library_dirs})
    get_filename_component(parent_libdir ${libdir} DIRECTORY)
    get_filename_component(parent_parent_libdir ${parent_libdir} DIRECTORY)
    list(APPEND parent_dirs ${libdir} ${parent_libdir} ${parent_parent_libdir})
  endforeach()

  set(header_search_paths
      ${search_paths}
      ${parent_dirs}
      ${project_parent_dirs}
      ${CMAKE_OUTPUT_DIRECTORY} # Search for headers in
      # ns-3-dev/build
      ${CMAKE_INSTALL_PREFIX} # Search for headers in the install
  )
  if(header_search_paths)
    list(REMOVE_DUPLICATES header_search_paths)
  endif()

  set(not_found_headers)
  set(include_dirs)
  foreach(header ${header_names})
    # The same way with libraries, we mark the internal variable as advanced not
    # to pollute ccmake configuration with variables used internally
    mark_as_advanced(${name}_header_internal_${header})
    set(suffixes
        /build
        /include
        /build/include
        /build/include/${name}
        /include/${name}
        /${name}
        /
        ${path_suffixes}
    )

    # cmake-format: off
        # Here we search for the header file named ${header} and store the result in
        # ${name}_header_internal_${header}
        #
        # The same way we did with libraries, here we search on
        # CMAKE_SYSTEM_PREFIX_PATH, along with user-settable ${search_paths}, the
        # parent directories from the libraries, CMAKE_OUTPUT_DIRECTORY and
        # CMAKE_INSTALL_PREFIX
        #
        # And again, for each of them, for every suffix listed /usr/local/build
        # /usr/local/include
        # /usr/local/build/include
        # /usr/local/build/include/${name}
        # /usr/local/include/${name}
        # ...
        #
        # cmake-format: on
    # Or enable NS3_VERBOSE to get the searched paths printed while configuring

    find_file(${name}_header_internal_${header} ${header}
              HINTS ${header_search_paths} # directory (e.g. /usr/)
              PATH_SUFFIXES ${suffixes}
    )

    # Print tested paths to the searched header and if it was found
    if(${NS3_VERBOSE} AND (${CMAKE_VERSION} VERSION_LESS "3.17.0"))
      log_find_searched_paths(
        TARGET_TYPE
        Header
        TARGET_NAME
        ${header}
        SEARCH_RESULT
        ${name}_header_internal_${header}
        SEARCH_PATHS
        ${header_search_paths}
        SEARCH_SUFFIXES
        ${suffixes}
      )
    endif()

    # If the header file was not found, append to the not-found list
    if("${${name}_header_internal_${header}}" STREQUAL
       "${name}_header_internal_${header}-NOTFOUND"
    )
      list(APPEND not_found_headers ${header})
    else()
      # If the header file was found, get their directories and the parent of
      # their directories to add as include directories
      get_filename_component(
        header_include_dir ${${name}_header_internal_${header}} DIRECTORY
      ) # e.g. include/click/ (simclick.h) -> #include <simclick.h> should work
      get_filename_component(
        header_include_dir2 ${header_include_dir} DIRECTORY
      ) # e.g. include/(click) -> #include <click/simclick.h> should work
      list(APPEND include_dirs ${header_include_dir} ${header_include_dir2})
    endif()
  endforeach()

  # Remove duplicate include directories
  if(include_dirs)
    list(REMOVE_DUPLICATES include_dirs)
  endif()

  # If we find both library and header, we export their values
  if((NOT not_found_libraries) AND (NOT not_found_headers))
    set(${name}_INCLUDE_DIRS "${include_dirs}" PARENT_SCOPE)
    set(${name}_LIBRARIES "${libraries}" PARENT_SCOPE)
    set(${name}_FOUND TRUE PARENT_SCOPE)
    if(NOT ${FIND_LIB_QUIET})
      message(STATUS "find_external_library: ${name} was found.")
    endif()
  else()
    set(${name}_INCLUDE_DIRS PARENT_SCOPE)
    set(${name}_LIBRARIES PARENT_SCOPE)
    set(${name}_FOUND FALSE PARENT_SCOPE)

    set(missing_components
        "Missing headers: \"${not_found_headers}\" and missing libraries: \"${not_found_libraries}\""
    )

    # Propagate missing components to the specified variable
    if(DEFINED FIND_LIB_OUTPUT_VARIABLE)
      mark_as_advanced(${FIND_LIB_OUTPUT_VARIABLE})
      set(${FIND_LIB_OUTPUT_VARIABLE} "${missing_components}" CACHE INTERNAL "")
      return()
    endif()

    # If no variable is specified, log it instead
    if(NOT ${FIND_LIB_QUIET})
      message(
        ${HIGHLIGHTED_STATUS}
        "find_external_library: ${name} was not found. ${missing_components}."
      )
    endif()
  endif()
endfunction()
