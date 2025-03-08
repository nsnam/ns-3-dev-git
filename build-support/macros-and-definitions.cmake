# Copyright (c) 2017-2021 Universidade de Brasília
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>

# Export compile time variable setting the directory to the NS3 root folder
add_definitions(-DPROJECT_SOURCE_PATH="${PROJECT_SOURCE_DIR}")

# Add custom and 3rd-part cmake modules to the path
list(APPEND CMAKE_MODULE_PATH
     "${PROJECT_SOURCE_DIR}/build-support/custom-modules"
)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/build-support/3rd-party")

macro(disable_cmake_warnings)
  set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE BOOL "" FORCE)
endmacro()

macro(enable_cmake_warnings)
  set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 0 CACHE BOOL "" FORCE)
endmacro()

# Set options that are not really meant to be changed
include(ns3-hidden-settings)

# Replace default CMake messages (logging) with custom colored messages as early
# as possible
include(colored-messages)

# Get installation folder default values
include(GNUInstallDirs)

# Define output directories
include(ns3-output-directory)

# Configure packaging related settings
include(ns3-cmake-package)

# Windows, Linux and Mac related checks
include(ns3-platform-support)

# Perform compiler and linker checks
include(ns3-compiler-and-linker-support)

# Include CMake file with common find_program HINTS
include(find-program-hints)

# Custom find_package alternative
include(ns3-find-external-library)

# Custom check_deps to search for packages, executables and python dependencies
include(ns3-check-dependencies)

# Set compiler options and get command to force unused function linkage (useful
# for libraries)
set(CXX_UNSUPPORTED_STANDARDS 98 11 14 17)
set(CMAKE_CXX_STANDARD_MINIMUM 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include CMake files used for compiler checks
include(CheckIncludeFile) # Used to check a single C header at a time
include(CheckIncludeFileCXX) # Used to check a single C++ header at a time
include(CheckIncludeFiles) # Used to check multiple headers at once
include(CheckFunctionExists)

# Check the number of threads
include(ProcessorCount)
ProcessorCount(NumThreads)

macro(SUBDIRLIST result curdir)
  file(GLOB children RELATIVE ${curdir} ${curdir}/*)
  set(dirlist "")
  foreach(child ${children})
    if(IS_DIRECTORY ${curdir}/${child})
      list(APPEND dirlist ${child})
    endif()
  endforeach()
  set(${result} ${dirlist})
endmacro()

macro(library_target_name libname targetname)
  set(${targetname} ${libname})
endmacro()

macro(clear_global_cached_variables)
  # clear cache variables
  unset(build_profile CACHE)
  unset(build_profile_suffix CACHE)
  set(ns3-contrib-libs "" CACHE INTERNAL "list of processed contrib modules")
  set(ns3-example-folders "" CACHE INTERNAL "list of example folders")
  set(ns3-execs "" CACHE INTERNAL "list of c++ executables")
  set(ns3-execs-clean "" CACHE INTERNAL "list of c++ executables")
  set(ns3-execs-py "" CACHE INTERNAL "list of python scripts")
  set(ns3-external-libs ""
      CACHE INTERNAL
            "list of non-ns libraries to link to NS3_STATIC and NS3_MONOLIB"
  )
  set(ns3-libs "" CACHE INTERNAL "list of processed upstream modules")
  set(ns3-libs-tests "" CACHE INTERNAL "list of test libraries")
  set(ns3-optional-visualizer-lib "" CACHE INTERNAL "visualizer library name")

  mark_as_advanced(
    build_profile
    build_profile_suffix
    ns3-contrib-libs
    ns3-example-folders
    ns3-execs
    ns3-execs-clean
    ns3-execs-py
    ns3-external-libs
    ns3-libs
    ns3-libs-tests
    ns3-optional-visualizer-lib
  )
endmacro()

# process all options passed in main cmakeLists
macro(process_options)
  clear_global_cached_variables()

  # make sure to default to RelWithDebInfo if no build type is specified
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "default" CACHE STRING "Choose the type of build."
                                         FORCE
    )
    set(NS3_ASSERT ON CACHE BOOL "Enable assert on failure" FORCE)
    set(NS3_LOG ON CACHE BOOL "Enable logging to be built" FORCE)
    set(NS3_WARNINGS_AS_ERRORS OFF
        CACHE BOOL "Treat warnings as errors. Requires NS3_WARNINGS=ON" FORCE
    )
  endif()

  # process debug switch Used in build-profile-test-suite
  string(TOLOWER ${CMAKE_BUILD_TYPE} cmakeBuildType)
  set(build_profile "${cmakeBuildType}" CACHE INTERNAL "")
  if(${cmakeBuildType} STREQUAL "debug")
    add_definitions(-DNS3_BUILD_PROFILE_DEBUG)
  elseif(${cmakeBuildType} STREQUAL "relwithdebinfo" OR ${cmakeBuildType}
                                                        STREQUAL "default"
  )
    set(cmakeBuildType relwithdebinfo)
    # Do not use optimized for size builds on MacOS See issue #1065:
    # https://gitlab.com/nsnam/ns-3-dev/-/issues/1065
    if(NOT (DEFINED APPLE))
      string(REPLACE "-O2" "-Os" CMAKE_CXX_FLAGS_RELWITHDEBINFO
                     "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}"
      )
    endif()
    # Do not use -Os for gcc 9 default builds due to a bug in gcc that can
    # result  in extreme memory usage. See MR !1955
    # https://gitlab.com/nsnam/ns-3-dev/-/merge_requests/1955
    if(GCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "10.0.0")
      string(REPLACE "-Os" "-O2" CMAKE_CXX_FLAGS_RELWITHDEBINFO
                     "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}"
      )
    endif()
    set(CMAKE_CXX_FLAGS_DEFAULT ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
    add_definitions(-DNS3_BUILD_PROFILE_DEBUG)
  elseif(${cmakeBuildType} STREQUAL "release")
    if(${NS3_NATIVE_OPTIMIZATIONS})
      add_definitions(-DNS3_BUILD_PROFILE_OPTIMIZED)
      set(build_profile "optimized" CACHE INTERNAL "")
    else()
      add_definitions(-DNS3_BUILD_PROFILE_RELEASE)
    endif()
  else()
    add_definitions(-DNS3_BUILD_PROFILE_RELEASE)
  endif()

  # Enable examples if activated via command line (NS3_EXAMPLES) or ns3rc config
  # file
  set(ENABLE_EXAMPLES OFF)
  if(${NS3_EXAMPLES} OR ${ns3rc_examples_enabled})
    set(ENABLE_EXAMPLES ON)
  endif()

  # Enable examples if activated via command line (NS3_TESTS) or ns3rc config
  # file
  set(ENABLE_TESTS OFF)
  if(${NS3_TESTS} OR ${ns3rc_tests_enabled})
    set(ENABLE_TESTS ON)
    # CTest creates a TEST target that conflicts with ns-3 test library
    # enable_testing()
  else()
    list(REMOVE_ITEM libs_to_build test)
  endif()

  set(profiles_without_suffixes release)
  set(build_profile_suffix "" CACHE INTERNAL "")
  if(NOT (${build_profile} IN_LIST profiles_without_suffixes))
    set(build_profile_suffix -${build_profile} CACHE INTERNAL "")
  endif()

  if(${NS3_VERBOSE})
    set_property(GLOBAL PROPERTY TARGET_MESSAGES TRUE)
    set(CMAKE_FIND_DEBUG_MODE TRUE)
    set(CMAKE_VERBOSE_MAKEFILE TRUE CACHE INTERNAL "")
  else()
    set_property(GLOBAL PROPERTY TARGET_MESSAGES OFF)
    unset(CMAKE_FIND_DEBUG_MODE)
    unset(CMAKE_VERBOSE_MAKEFILE CACHE)
  endif()

  if(${NS3_FORCE_LOCAL_DEPENDENCIES})
    set(CMAKE_FIND_FRAMEWORK NEVER)
    set(CMAKE_FIND_APPBUNDLE NEVER)
    set(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH FALSE)
    set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
  endif()

  # Set warning level and warning as errors
  if(${NS3_WARNINGS})
    if(${MSVC})
      add_compile_options(/W3) # /W4 = -Wall + -Wextra
      if(${NS3_WARNINGS_AS_ERRORS})
        add_compile_options(/WX)
      endif()
    else()
      add_compile_options(-Wall) # -Wextra
      if(${GCC_WORKING_PEDANTIC_SEMICOLON})
        add_compile_options(-Wpedantic)
      endif()
      if(${NS3_WARNINGS_AS_ERRORS})
        add_compile_options(-Werror -Wno-error=deprecated-declarations)
      endif()
    endif()
  endif()

  include(ns3-versioning)
  set(ENABLE_BUILD_VERSION False)
  configure_embedded_version()

  if(${NS3_CLANG_FORMAT})
    find_program(CLANG_FORMAT clang-format)
    if("${CLANG_FORMAT}" STREQUAL "CLANG_FORMAT-NOTFOUND")
      message(FATAL_ERROR "Clang-format was not found")
    else()
      file(
        GLOB_RECURSE
        ALL_CXX_SOURCE_FILES
        src/*.cc
        src/*.h
        examples/*.cc
        examples/*.h
        utils/*.cc
        utils/*.h
        scratch/*.cc
        scratch/*.h
      )
      add_custom_target(
        clang-format COMMAND ${CLANG_FORMAT} -style=file -i
                             ${ALL_CXX_SOURCE_FILES}
      )
      unset(ALL_CXX_SOURCE_FILES)
    endif()
  endif()

  if(${NS3_CLANG_TIDY})
    find_program(
      CLANG_TIDY NAMES clang-tidy clang-tidy-15 clang-tidy-16 clang-tidy-17
                       clang-tidy-18 clang-tidy-19
    )
    if("${CLANG_TIDY}" STREQUAL "CLANG_TIDY-NOTFOUND")
      message(FATAL_ERROR "Clang-tidy was not found")
    else()
      set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY}")
    endif()
  else()
    unset(CMAKE_CXX_CLANG_TIDY)
  endif()

  if(${NS3_CLANG_TIMETRACE})
    if(${CLANG})
      # Fetch and build clang build analyzer
      include(ExternalProject)
      ExternalProject_Add(
        ClangBuildAnalyzer
        GIT_REPOSITORY "https://github.com/aras-p/ClangBuildAnalyzer.git"
        GIT_TAG "47406981a1c5a89e8f8c62802b924c3e163e7cb4"
        CMAKE_ARGS -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        INSTALL_COMMAND cmake -E copy_if_different ClangBuildAnalyzer
                        ${PROJECT_BINARY_DIR}
      )

      # Add compiler flag and create target to do everything automatically
      add_definitions(-ftime-trace)
      add_custom_target(
        timeTraceReport
        COMMAND
          ${PROJECT_BINARY_DIR}/ClangBuildAnalyzer --all ${PROJECT_BINARY_DIR}
          ${PROJECT_BINARY_DIR}/clangBuildAnalyzerReport.bin
        COMMAND
          ${PROJECT_BINARY_DIR}/ClangBuildAnalyzer --analyze
          ${PROJECT_BINARY_DIR}/clangBuildAnalyzerReport.bin >
          ${PROJECT_SOURCE_DIR}/ClangBuildAnalyzerReport.txt
        DEPENDS ClangBuildAnalyzer
      )
    else()
      message(
        FATAL_ERROR
          "TimeTrace is a Clang feature, but you're using a different compiler."
      )
    endif()
  endif()

  mark_as_advanced(CMAKE_FORMAT_PROGRAM)
  find_program(CMAKE_FORMAT_PROGRAM cmake-format HINTS ~/.local/bin)
  if("${CMAKE_FORMAT_PROGRAM}" STREQUAL "CMAKE_FORMAT_PROGRAM-NOTFOUND")
    message(${HIGHLIGHTED_STATUS} "Proceeding without cmake-format")
  else()
    file(
      GLOB
      MODULES_CMAKE_FILES
      src/**/CMakeLists.txt
      contrib/**/CMakeLists.txt
      src/**/examples/CMakeLists.txt
      contrib/**/examples/CMakeLists.txt
      examples/**/CMakeLists.txt
      scratch/**/CMakeLists.txt
    )
    file(GLOB_RECURSE SCRATCH_CMAKE_FILES scratch/**/CMakeLists.txt)
    file(
      GLOB
      INTERNAL_CMAKE_FILES
      CMakeLists.txt
      utils/**/CMakeLists.txt
      src/CMakeLists.txt
      build-support/**/*.cmake
      build-support/*.cmake
    )
    add_custom_target(
      cmake-format
      COMMAND
        ${CMAKE_FORMAT_PROGRAM} -c
        ${PROJECT_SOURCE_DIR}/build-support/cmake-format.yaml -i
        ${INTERNAL_CMAKE_FILES}
      COMMAND
        ${CMAKE_FORMAT_PROGRAM} -c
        ${PROJECT_SOURCE_DIR}/build-support/cmake-format-modules.yaml -i
        ${MODULES_CMAKE_FILES} ${SCRATCH_CMAKE_FILES}
    )
    add_custom_target(
      cmake-format-check
      COMMAND
        ${CMAKE_FORMAT_PROGRAM} -c
        ${PROJECT_SOURCE_DIR}/build-support/cmake-format.yaml --check
        ${INTERNAL_CMAKE_FILES}
      COMMAND
        ${CMAKE_FORMAT_PROGRAM} -c
        ${PROJECT_SOURCE_DIR}/build-support/cmake-format-modules.yaml --check
        ${MODULES_CMAKE_FILES}
    )
    unset(MODULES_CMAKE_FILES)
    unset(INTERNAL_CMAKE_FILES)
  endif()

  # If the user has not set a CXX standard version, assume the minimum
  if(NOT (DEFINED CMAKE_CXX_STANDARD))
    set(CMAKE_CXX_STANDARD ${CMAKE_CXX_STANDARD_MINIMUM})
  endif()

  # Search standard provided by the user in the unsupported standards list
  list(FIND CXX_UNSUPPORTED_STANDARDS ${CMAKE_CXX_STANDARD} unsupported)
  if(${unsupported} GREATER -1)
    message(
      FATAL_ERROR
        "You're trying to use the unsupported C++ ${CMAKE_CXX_STANDARD}.\n"
        "Try -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD_MINIMUM}."
    )
  endif()

  # After setting the correct CXX version, we can proceed to check for compiler
  # workarounds
  include(ns3-compiler-workarounds)

  if(${NS3_DES_METRICS})
    add_definitions(-DENABLE_DES_METRICS)
  endif()

  if(${NS3_SANITIZE} AND ${NS3_SANITIZE_MEMORY})
    message(
      FATAL_ERROR
        "The memory sanitizer can't be used with other sanitizers. Disable one of them."
    )
  endif()

  if(${NS3_SANITIZE})
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -fsanitize=address,leak,undefined -fno-sanitize-recover=all"
    )
  endif()

  if(${NS3_SANITIZE_MEMORY})
    if(${CLANG})
      set(blacklistfile ${PROJECT_SOURCE_DIR}/memory-sanitizer-blacklist.txt)
      if(EXISTS ${blacklistfile})
        set(CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS} -fsanitize=memory -fsanitize-blacklist=${blacklistfile}"
        )
      else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory")
      endif()

      if(NOT ($ENV{MSAN_OPTIONS} MATCHES "strict_memcmp=0"))
        message(
          WARNING
            "Please export MSAN_OPTIONS=strict_memcmp=0 "
            "and call the generated buildsystem directly to proceed.\n"
            "Trying to build with cmake or IDEs will probably fail since"
            "CMake can't export environment variables to its parent process."
        )
      endif()
      unset(blacklistfile)
    else()
      message(FATAL_ERROR "The memory sanitizer is only supported by Clang")
    endif()
  endif()

  if(${NS3_NATIVE_OPTIMIZATIONS} AND ${GCC})
    add_compile_options(-march=native -mtune=native)
  endif()

  if(${NS3_LINK_TIME_OPTIMIZATION})
    # Link-time optimization (LTO) if available
    include(CheckIPOSupported)
    check_ipo_supported(RESULT LTO_AVAILABLE OUTPUT output)
    if(LTO_AVAILABLE)
      set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
      message(STATUS "Link-time optimization (LTO) is supported.")
    else()
      message(
        STATUS "Link-time optimization (LTO) is not supported: ${output}."
      )
    endif()
  endif()

  if(${NS3_LINK_WHAT_YOU_USE})
    set(CMAKE_LINK_WHAT_YOU_USE TRUE)
  else()
    set(CMAKE_LINK_WHAT_YOU_USE FALSE)
  endif()

  if(${NS3_INCLUDE_WHAT_YOU_USE})
    # Before using, install iwyu, run "strings /usr/bin/iwyu | grep LLVM" to
    # find the appropriate clang version and install it. If you don't do that,
    # it will fail. Worse than that: it will fail silently. We use the wrapper
    # to get a iwyu.log file in the ns-3-dev folder.
    find_program(INCLUDE_WHAT_YOU_USE_PROG iwyu)
    if("${INCLUDE_WHAT_YOU_USE_PROG}" STREQUAL
       "INCLUDE_WHAT_YOU_USE_PROG-NOTFOUND"
    )
      message(FATAL_ERROR "iwyu (include-what-you-use) was not found.")
    endif()
    message(STATUS "iwyu is enabled")
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE
        ${PROJECT_SOURCE_DIR}/build-support/iwyu-wrapper.sh;${PROJECT_SOURCE_DIR}
    )
  else()
    unset(CMAKE_CXX_INCLUDE_WHAT_YOU_USE)
  endif()

  if(${XCODE})
    if(${NS3_STATIC} OR ${NS3_MONOLIB})
      message(
        FATAL_ERROR
          "Xcode doesn't play nicely with CMake object libraries,"
          "and those are used for NS3_STATIC and NS3_MONOLIB.\n"
          "Disable them or try a different generator"
      )
    endif()
    set(CMAKE_XCODE_GENERATE_TOP_LEVEL_PROJECT_ONLY ON)
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO)
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
  else()
    unset(CMAKE_XCODE_GENERATE_TOP_LEVEL_PROJECT_ONLY)
    unset(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED)
    unset(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY)
  endif()

  # Set common include folder (./build/include, where we find ns3/core-module.h)
  include_directories(${CMAKE_OUTPUT_DIRECTORY}/include)

  # Include our package managers
  # cmake-format: off
  # Starting with a custom cmake file that provides a Hunter-like interface to vcpkg
  # Use add_package(package) to install a package
  # Then find_package(package) to use it
  # cmake-format: on
  include(ns3-vcpkg-hunter)

  # Then the beautiful CPM manager (too bad it doesn't work with everything)
  # https://github.com/cpm-cmake/CPM.cmake
  if(${NS3_CPM})
    set(CPM_DOWNLOAD_VERSION 0.38.2)
    set(CPM_DOWNLOAD_LOCATION
        "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake"
    )
    if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
      message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
      file(
        DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
        ${CPM_DOWNLOAD_LOCATION}
      )
    endif()
    include(${CPM_DOWNLOAD_LOCATION})
    set(CPM_USE_LOCAL_PACKAGES ON)
  endif()
  # Package manager test block
  if(TEST_PACKAGE_MANAGER)
    if(${TEST_PACKAGE_MANAGER} STREQUAL "CPM")
      cpmaddpackage(
        NAME ARMADILLO GIT_TAG 6cada351248c9a967b137b9fcb3d160dad7c709b
        GIT_REPOSITORY https://gitlab.com/conradsnicta/armadillo-code.git
      )
      find_package(ARMADILLO REQUIRED)
      message(STATUS "Armadillo was found? ${ARMADILLO_FOUND}")
    elseif(${TEST_PACKAGE_MANAGER} STREQUAL "VCPKG")
      add_package(Armadillo)
      find_package(Armadillo REQUIRED)
      message(STATUS "Armadillo was found? ${ARMADILLO_FOUND}")
    else()
      find_package(Armadillo REQUIRED)
    endif()
  endif()
  # End of package managers

  set(ENABLE_SQLITE False)
  if(${NS3_SQLITE})
    # find_package(SQLite3 QUIET) # unsupported in CMake 3.10 We emulate the
    # behavior of find_package below
    find_external_library(
      DEPENDENCY_NAME SQLite3
      HEADER_NAME sqlite3.h
      LIBRARY_NAME sqlite3
      OUTPUT_VARIABLE "ENABLE_SQLITE_REASON"
    )

    if(${SQLite3_FOUND})
      set(ENABLE_SQLITE True)
      add_definitions(-DHAVE_SQLITE3)
      if(NOT ${NS3_FORCE_LOCAL_DEPENDENCIES})
        include_directories(${SQLite3_INCLUDE_DIRS})
      endif()
    endif()
  endif()

  set(ENABLE_EIGEN False)
  if(${NS3_EIGEN})
    disable_cmake_warnings()
    find_package(Eigen3 QUIET)
    enable_cmake_warnings()

    if(${EIGEN3_FOUND})
      set(ENABLE_EIGEN True)
      add_definitions(-DHAVE_EIGEN3)
      add_definitions(-DEIGEN_MPL2_ONLY)
      if(NOT ${NS3_FORCE_LOCAL_DEPENDENCIES})
        include_directories(${EIGEN3_INCLUDE_DIR})
      endif()
    else()
      set(ENABLE_EIGEN_REASON "Eigen was not found")
    endif()
  endif()

  # GTK3 Don't search for it if you don't have it installed, as it take an
  # insane amount of time
  set(GTK3_FOUND FALSE)
  if(${NS3_GTK3})
    disable_cmake_warnings()
    find_package(HarfBuzz QUIET)
    enable_cmake_warnings()
    if(NOT ${HarfBuzz_FOUND})
      set(GTK3_FOUND_REASON "Harfbuzz is required by GTK3 and was not found")
    else()
      disable_cmake_warnings()
      find_package(GTK3 QUIET)
      enable_cmake_warnings()
      if(NOT ${GTK3_FOUND})
        set(GTK3_FOUND_REASON "GTK3 was not found")
      else()
        if(${GTK3_VERSION} VERSION_LESS 3.22)
          set(GTK3_FOUND FALSE)
          set(GTK3_FOUND_REASON
              "GTK3 found with incompatible version ${GTK3_VERSION}"
          )
        else()
          if(NOT ${NS3_FORCE_LOCAL_DEPENDENCIES})
            include_directories(${GTK3_INCLUDE_DIRS} ${HarfBuzz_INCLUDE_DIRS})
          endif()
        endif()
      endif()

    endif()
  endif()

  set(LIBXML2_FOUND FALSE)
  if(${NS3_STATIC})
    # Warn users that they may be using shared libraries, which won't produce a
    # standalone static library
    message(
      WARNING "Statically linking 3rd party libraries have not been tested.\n"
              "Disable Brite, Click, Gtk, GSL, Mpi, Openflow and SQLite"
              " if you want a standalone static ns-3 library."
    )
    if(WIN32)
      message(FATAL_ERROR "Static builds are unsupported on Windows"
                          "\nSocket libraries cannot be linked statically"
      )
    endif()
  else()
    find_package(LibXml2 QUIET)
    if(NOT ${LIBXML2_FOUND})
      set(LIBXML2_FOUND_REASON "LibXML2 was not found")
    else()
      add_definitions(-DHAVE_LIBXML2)
      if(NOT ${NS3_FORCE_LOCAL_DEPENDENCIES})
        include_directories(${LIBXML2_INCLUDE_DIR})
      endif()
    endif()
  endif()

  set(THREADS_PREFER_PTHREAD_FLAG)
  find_package(Threads QUIET)
  if(NOT ${Threads_FOUND})
    message(FATAL_ERROR Threads are required by ns-3)
  endif()

  set(Python3_LIBRARIES)
  set(Python3_EXECUTABLE)
  set(Python3_FOUND FALSE)
  set(Python3_INCLUDE_DIRS)
  set(Python3_Interpreter_FOUND FALSE)
  if(${NS3_PYTHON_BINDINGS})
    find_package(Python3 COMPONENTS Interpreter Development)
  else()
    # If Python was not set yet, use the version found by check_deps
    check_deps(python3_deps EXECUTABLES python3)
    if(python3_deps)
      message(FATAL_ERROR "Python3 was not found")
    else()
      set(Python3_EXECUTABLE ${PYTHON3})
    endif()
  endif()

  # Check if both Python interpreter and development libraries were found
  if(${Python3_Interpreter_FOUND})
    if(${Python3_Development_FOUND})
      set(Python3_FOUND TRUE)
      if(APPLE)
        # Apple is very weird and there could be a lot of conflicting python
        # versions which can generate conflicting rpaths preventing the python
        # bindings from working

        # To work around, we extract the /path/to/Frameworks from the library
        # path
        list(GET Python3_LIBRARIES 0 pylib)
        string(REGEX REPLACE "(.*Frameworks)/Python(3.|.)framework.*" "\\1"
                             DEVELOPER_DIR ${pylib}
        )
        if("${DEVELOPER_DIR}" MATCHES "Frameworks")
          set(CMAKE_BUILD_RPATH "${DEVELOPER_DIR}" CACHE STRING "")
          set(CMAKE_INSTALL_RPATH "${DEVELOPER_DIR}" CACHE STRING "")
        endif()
      endif()
      if(NOT ${NS3_FORCE_LOCAL_DEPENDENCIES})
        include_directories(${Python3_INCLUDE_DIRS})
      endif()
    else()
      message(${HIGHLIGHTED_STATUS}
              "Python: development libraries were not found"
      )
      set(ENABLE_PYTHON_BINDINGS_REASON "missing Python development libraries")
    endif()
  else()
    if(${NS3_PYTHON_BINDINGS})
      message(
        ${HIGHLIGHTED_STATUS}
        "Python: an incompatible version of Python was found, python bindings will be disabled"
      )
      set(ENABLE_PYTHON_BINDINGS_REASON "incompatible Python version")
    endif()
  endif()

  set(ENABLE_PYTHON_BINDINGS OFF)
  if(${NS3_PYTHON_BINDINGS})
    if(NOT ${Python3_FOUND})
      message(
        ${HIGHLIGHTED_STATUS}
        "Bindings: python bindings require Python, but it could not be found"
      )
      set(ENABLE_PYTHON_BINDINGS_REASON "missing dependency: python")
    else()
      check_deps(missing_packages PYTHON_PACKAGES cppyy)
      if(missing_packages)
        message(
          ${HIGHLIGHTED_STATUS}
          "Bindings: python bindings disabled due to the following missing dependencies: ${missing_packages}"
        )
        set(ENABLE_PYTHON_BINDINGS_REASON
            "missing dependency: ${missing_packages}"
        )
      elseif(${NS3_MPI})
        message(
          ${HIGHLIGHTED_STATUS}
          "Bindings: python bindings disabled due to an incompatibility with the MPI module"
        )
        set(ENABLE_PYTHON_BINDINGS_REASON "incompatible module enabled: mpi")
      else()
        set(ENABLE_PYTHON_BINDINGS ON)
      endif()

      # Copy the bindings file if we have python, which will prevent python
      # scripts from failing due to the missing ns package
      set(destination_dir ${CMAKE_OUTPUT_DIRECTORY}/bindings/python/ns)
      configure_file(
        bindings/python/ns__init__.py ${destination_dir}/__init__.py COPYONLY
      )

      # And create an install target for the bindings
      if(NOT NS3_BINDINGS_INSTALL_DIR)
        # If the installation directory for the python bindings is not set,
        # suggest the user site-packages directory
        execute_process(
          COMMAND python3 -m site --user-site
          OUTPUT_VARIABLE SUGGESTED_BINDINGS_INSTALL_DIR
        )
        string(STRIP "${SUGGESTED_BINDINGS_INSTALL_DIR}"
                     SUGGESTED_BINDINGS_INSTALL_DIR
        )
        message(
          ${HIGHLIGHTED_STATUS}
          "NS3_BINDINGS_INSTALL_DIR was not set. The python bindings won't be installed with ./ns3 install."
          "This setting is meant for packaging and redistribution."
        )
        message(
          ${HIGHLIGHTED_STATUS}
          "Set NS3_BINDINGS_INSTALL_DIR=\"${SUGGESTED_BINDINGS_INSTALL_DIR}\" to install it to the default location."
        )
      else()
        if(${NS3_BINDINGS_INSTALL_DIR} STREQUAL "INSTALL_PREFIX")
          set(NS3_BINDINGS_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})
        endif()
        install(FILES bindings/python/ns__init__.py
                DESTINATION ${NS3_BINDINGS_INSTALL_DIR}/ns RENAME __init__.py
        )
        add_custom_target(
          uninstall_bindings COMMAND rm -R ${NS3_BINDINGS_INSTALL_DIR}/ns
        )
        add_dependencies(uninstall uninstall_bindings)
      endif()
    endif()
  endif()

  if(${NS3_NINJA_TRACING})
    if(${CMAKE_GENERATOR} STREQUAL Ninja)
      include(ExternalProject)
      ExternalProject_Add(
        NinjaTracing
        GIT_REPOSITORY "https://github.com/nico/ninjatracing.git"
        GIT_TAG "f9d21e973cfdeafa913b83a927fef56258f70b9a"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
      )
      ExternalProject_Get_Property(NinjaTracing SOURCE_DIR)
      set(embed_time_trace)
      if(${NS3_CLANG_TIMETRACE} AND ${CLANG})
        set(embed_time_trace --embed-time-trace)
      endif()
      add_custom_target(
        ninjaTrace
        COMMAND
          ${Python3_EXECUTABLE} ${SOURCE_DIR}/ninjatracing -a
          ${embed_time_trace} ${PROJECT_BINARY_DIR}/.ninja_log >
          ${PROJECT_SOURCE_DIR}/ninja_performance_trace.json
        DEPENDS NinjaTracing
      )
      unset(embed_time_trace)
      unset(SOURCE_DIR)
    else()
      message(FATAL_ERROR "Ninjatracing requires the Ninja generator")
    endif()
  endif()

  # Disable the below warning from bindings built in debug mode with clang++:
  # "expression with side effects will be evaluated despite being used as an
  # operand to 'typeid'"
  if(${ENABLE_PYTHON_BINDINGS} AND ${CLANG})
    add_compile_options(-Wno-potentially-evaluated-expression)
  endif()

  set(ENABLE_VISUALIZER FALSE)
  if(${NS3_VISUALIZER})
    # If bindings are enabled, check if visualizer dependencies are met
    if(${NS3_PYTHON_BINDINGS})
      if(NOT ${Python3_FOUND})
        set(ENABLE_VISUALIZER_REASON "missing Python")
      elseif(NOT ${ENABLE_PYTHON_BINDINGS})
        set(ENABLE_VISUALIZER_REASON "missing Python Bindings")
      else()
        set(ENABLE_VISUALIZER TRUE)
      endif()
      # If bindings are disabled, just say python bindings are disabled
    else()
      set(ENABLE_VISUALIZER_REASON "Python Bindings are disabled")
    endif()
  endif()

  if(${NS3_COVERAGE} AND (NOT ${ENABLE_TESTS} OR NOT ${ENABLE_EXAMPLES}))
    message(
      FATAL_ERROR
        "Code coverage requires examples and tests.\nTry reconfiguring CMake with -DNS3_TESTS=ON -DNS3_EXAMPLES=ON"
    )
  endif()

  if(${ENABLE_TESTS})
    add_custom_target(test-runner-examples-as-tests)
    add_custom_target(all-test-targets)

    # Create a custom target to run test.py --no-build Target is also used to
    # produce code coverage output
    add_custom_target(
      run_test_py
      COMMAND ${Python3_EXECUTABLE} test.py --no-build
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      DEPENDS all-test-targets
    )
    if(${ENABLE_EXAMPLES})
      include(ns3-coverage)
    endif()
  endif()

  set(ENABLE_MPI FALSE)
  if(${NS3_MPI})
    find_package(MPI QUIET)
    if(NOT ${MPI_FOUND})
      message(FATAL_ERROR "MPI was not found.")
    else()
      message(STATUS "MPI was found.")
      target_compile_definitions(MPI::MPI_CXX INTERFACE NS3_MPI)
      set(ENABLE_MPI TRUE)
    endif()
  endif()

  # Use upstream boost package config with CMake 3.30 and above
  if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
  endif()
  mark_as_advanced(Boost_INCLUDE_DIR)
  find_package(Boost)
  if(${Boost_FOUND})
    if(NOT ${NS3_FORCE_LOCAL_DEPENDENCIES})
      include_directories(${Boost_INCLUDE_DIRS})
    endif()
    set(CMAKE_REQUIRED_INCLUDES ${Boost_INCLUDE_DIRS})
  endif()

  set(GSL_FOUND FALSE)
  if(${NS3_GSL})
    find_package(GSL QUIET)
    if(NOT ${GSL_FOUND})
      set(GSL_FOUND_REASON "GSL was not found")
    else()
      message(STATUS "GSL was found.")
      add_definitions(-DHAVE_GSL)
      if(NOT ${NS3_FORCE_LOCAL_DEPENDENCIES})
        include_directories(${GSL_INCLUDE_DIRS})
      endif()
    endif()
  endif()

  # checking for documentation dependencies and creating targets

  # First we check for doxygen dependencies
  mark_as_advanced(DOXYGEN)
  check_deps(doxygen_docs_missing_deps EXECUTABLES doxygen dot dia python3)
  if(doxygen_docs_missing_deps)
    message(
      ${HIGHLIGHTED_STATUS}
      "docs: doxygen documentation not enabled due to missing dependencies: ${doxygen_docs_missing_deps}"
    )
    # cmake-format: off
    set(doxygen_missing_msg
        echo The following Doxygen dependencies are missing: ${doxygen_docs_missing_deps}.
            Reconfigure the project after installing them.
    )
    # cmake-format: on

    # Create stub targets to inform users about missing dependencies
    add_custom_target(
      run-print-introspected-doxygen COMMAND ${doxygen_missing_msg}
    )
    add_custom_target(
      run-introspected-command-line COMMAND ${doxygen_missing_msg}
    )
    add_custom_target(
      assemble-introspected-command-line COMMAND ${doxygen_missing_msg}
    )
    add_custom_target(update_doxygen_version COMMAND ${doxygen_missing_msg})
    add_custom_target(doxygen COMMAND ${doxygen_missing_msg})
    add_custom_target(doxygen-no-build COMMAND ${doxygen_missing_msg})
  else()
    # We checked this already exists, but we need the path to the executable
    set(DOXYGEN_EXECUTABLE ${DOXYGEN})

    add_custom_target(
      update_doxygen_version
      COMMAND bash ${PROJECT_SOURCE_DIR}/doc/ns3_html_theme/get_version.sh
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
    add_custom_target(
      doxygen-no-build
      COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_SOURCE_DIR}/doc/doxygen.conf
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      DEPENDS update_doxygen_version
      USES_TERMINAL
    )
    # The `doxygen` target only really works if we have tests enabled, so emit a
    # warning to use `doxygen-no-build` instead.
    if((NOT ${ENABLE_TESTS}) AND (NOT ${ENABLE_EXAMPLES}))
      # cmake-format: off
      set(doxygen_target_requires_tests_msg
              echo The \\'doxygen\\' target called by \\'./ns3 docs doxygen\\' or \\'./ns3 docs all\\' commands
              require examples and tests to generate introspected documentation.
              Enable examples and tests, or use \\'doxygen-no-build\\'.
              )
      # cmake-format: on
      add_custom_target(doxygen COMMAND ${doxygen_target_requires_tests_msg})
      unset(doxygen_target_requires_tests_msg)
    else()
      # Get introspected doxygen
      add_custom_target(
        run-print-introspected-doxygen
        COMMAND
          ${CMAKE_OUTPUT_DIRECTORY}/utils/ns${NS3_VER}-print-introspected-doxygen${build_profile_suffix}
          > ${PROJECT_SOURCE_DIR}/doc/introspected-doxygen.h
        COMMAND
          ${CMAKE_OUTPUT_DIRECTORY}/utils/ns${NS3_VER}-print-introspected-doxygen${build_profile_suffix}
          --output-text > ${PROJECT_SOURCE_DIR}/doc/ns3-object.txt
        DEPENDS print-introspected-doxygen
      )
      add_custom_target(
        run-introspected-command-line
        COMMAND ${CMAKE_COMMAND} -E env NS_COMMANDLINE_INTROSPECTION=..
                ${Python3_EXECUTABLE} ./test.py --no-build --constrain=example
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        DEPENDS all-test-targets # all-test-targets only exists if ENABLE_TESTS
                                 # is set to ON
      )

      file(
        WRITE ${CMAKE_BINARY_DIR}/introspected-command-line-preamble.h
        "/* This file is automatically generated by
  CommandLine::PrintDoxygenUsage() from the CommandLine configuration
  in various example programs.  Do not edit this file!  Edit the
  CommandLine configuration in those files instead.
  */\n"
      )
      add_custom_target(
        assemble-introspected-command-line
        # works on CMake 3.18 or newer > COMMAND ${CMAKE_COMMAND} -E cat
        # ${PROJECT_SOURCE_DIR}/testpy-output/*.command-line >
        # ${PROJECT_SOURCE_DIR}/doc/introspected-command-line.h
        COMMAND
          ${cat_command}
          ${CMAKE_BINARY_DIR}/introspected-command-line-preamble.h
          ${PROJECT_SOURCE_DIR}/testpy-output/*.command-line >
          ${PROJECT_SOURCE_DIR}/doc/introspected-command-line.h 2> NULL
        DEPENDS run-introspected-command-line
      )

      add_custom_target(
        doxygen
        COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_SOURCE_DIR}/doc/doxygen.conf
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        DEPENDS update_doxygen_version run-print-introspected-doxygen
                assemble-introspected-command-line
        USES_TERMINAL
      )
    endif()
  endif()

  # Now we check for sphinx dependencies
  mark_as_advanced(
    SPHINX_EXECUTABLE SPHINX_OUTPUT_HTML SPHINX_OUTPUT_MAN
    SPHINX_WARNINGS_AS_ERRORS
  )

  # Check deps accepts a list of packages, list of programs and name of the
  # return variable
  check_deps(
    sphinx_docs_missing_deps CMAKE_PACKAGES Sphinx
    EXECUTABLES epstopdf pdflatex latexmk convert dvipng dia
  )
  if(sphinx_docs_missing_deps)
    message(
      ${HIGHLIGHTED_STATUS}
      "docs: sphinx documentation not enabled due to missing dependencies: ${sphinx_docs_missing_deps}"
    )
    # cmake-format: off
    set(sphinx_missing_msg
        echo The following Sphinx dependencies are missing: ${sphinx_docs_missing_deps}.
            Reconfigure the project after installing them.
    )
    # cmake-format: on

    # Create stub targets to inform users about missing dependencies
    add_custom_target(sphinx COMMAND ${sphinx_missing_msg})
    add_custom_target(sphinx_manual COMMAND ${sphinx_missing_msg})
    add_custom_target(sphinx_models COMMAND ${sphinx_missing_msg})
    add_custom_target(sphinx_tutorial COMMAND ${sphinx_missing_msg})
    add_custom_target(sphinx_contributing COMMAND ${sphinx_missing_msg})
    add_custom_target(sphinx_installation COMMAND ${sphinx_missing_msg})
  else()
    add_custom_target(sphinx COMMENT "Building sphinx documents")
    mark_as_advanced(MAKE)
    find_program(MAKE NAMES make mingw32-make)
    if(${MAKE} STREQUAL "MAKE-NOTFOUND")
      message(
        FATAL_ERROR "Make was not found but it is required by Sphinx docs"
      )
    elseif(${MAKE} MATCHES "mingw32-make")
      # This is a super wild hack for MinGW
      #
      # For some reason make is shipped as mingw32-make instead of make, but
      # tons of software rely on it being called make
      #
      # We could technically create an alias, using doskey make=mingw32-make,
      # but we need to redefine that for every new shell or make registry
      # changes to make it permanent
      #
      # Symlinking requires administrative permissions for some reason, so we
      # just copy the entire thing
      get_filename_component(make_directory ${MAKE} DIRECTORY)
      get_filename_component(make_parent_directory ${make_directory} DIRECTORY)
      if(NOT (EXISTS ${make_directory}/make.exe))
        file(COPY ${MAKE} DESTINATION ${make_parent_directory})
        file(RENAME ${make_parent_directory}/mingw32-make.exe
             ${make_directory}/make.exe
        )
      endif()
      set(MAKE ${make_directory}/make.exe)
    else()

    endif()

    function(sphinx_target targetname)
      # cmake-format: off
      add_custom_target(
        sphinx_${targetname}
        COMMAND ${MAKE} SPHINXOPTS=-N -k html singlehtml latexpdf
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/doc/${targetname}
      )
      # cmake-format: on
      add_dependencies(sphinx sphinx_${targetname})
    endfunction()
    sphinx_target(manual)
    sphinx_target(models)
    sphinx_target(tutorial)
    sphinx_target(contributing)
    sphinx_target(installation)
  endif()
  # end of checking for documentation dependencies and creating targets

  # Adding this module manually is required by CMake 3.10
  include(CheckCXXSourceCompiles)

  # Process core-config If INT128 is not found, fallback to CAIRO
  if(${NS3_INT64X64} MATCHES "INT128")
    check_cxx_source_compiles(
      "#include <stdint.h>
       int main()
         {
            if ((uint128_t *) 0) return 0;
            if (sizeof (uint128_t)) return 0;
            return 1;
         }"
      HAVE_UINT128_T
    )
    check_cxx_source_compiles(
      "#include <stdint.h>
       int main()
         {
           if ((__uint128_t *) 0) return 0;
           if (sizeof (__uint128_t)) return 0;
           return 1;
        }"
      HAVE___UINT128_T
    )
    if(HAVE_UINT128_T OR HAVE___UINT128_T)
      set(INT64X64_USE_128 TRUE)
    else()
      message(${HIGHLIGHTED_STATUS}
              "Int128 was not found. Falling back to Cairo."
      )
      set(NS3_INT64X64 "CAIRO")
    endif()
  endif()

  # If long double and double have different sizes, fallback to CAIRO
  if(${NS3_INT64X64} MATCHES "DOUBLE")
    # WSLv1 has a long double issue that will result in a few tests failing
    # https://github.com/microsoft/WSL/issues/830
    include(CheckTypeSize)
    check_type_size("double" SIZEOF_DOUBLE)
    check_type_size("long double" SIZEOF_LONG_DOUBLE)

    if(${SIZEOF_LONG_DOUBLE} EQUAL ${SIZEOF_DOUBLE})
      message(
        STATUS
          "Long double has the wrong size: LD ${SIZEOF_LONG_DOUBLE} vs D ${SIZEOF_DOUBLE}. Falling back to CAIRO."
      )
      set(NS3_INT64X64 "CAIRO")
    else()
      set(INT64X64_USE_DOUBLE TRUE)
    endif()
  endif()

  # Fallback option
  if(${NS3_INT64X64} MATCHES "CAIRO")
    set(INT64X64_USE_CAIRO TRUE)
  endif()

  # Check for required headers and functions, set flags if they're found or warn
  # if they're not found
  check_include_file("stdint.h" "HAVE_STDINT_H")
  check_include_file("inttypes.h" "HAVE_INTTYPES_H")
  check_include_file("sys/types.h" "HAVE_SYS_TYPES_H")
  check_include_file("sys/stat.h" "HAVE_SYS_STAT_H")
  check_include_file("dirent.h" "HAVE_DIRENT_H")
  check_include_file("stdlib.h" "HAVE_STDLIB_H")
  check_include_file("signal.h" "HAVE_SIGNAL_H")
  check_include_file("netpacket/packet.h" "HAVE_PACKETH")
  check_function_exists("getenv" "HAVE_GETENV")

  configure_file(
    ${PROJECT_SOURCE_DIR}/build-support/core-config-template.h
    ${CMAKE_HEADER_OUTPUT_DIRECTORY}/core-config.h
  )

  # Force enable ns-3 logging in debug builds and if requested for other build
  # types
  if(${NS3_LOG} OR (${build_profile} STREQUAL "debug"))
    add_definitions(-DNS3_LOG_ENABLE)
  endif()
  # Force enable ns-3 asserts in debug builds and if requested for other build
  # types
  if(${NS3_ASSERT} OR (${build_profile} STREQUAL "debug"))
    add_definitions(-DNS3_ASSERT_ENABLE)
  endif()

  set(ENABLE_TAP OFF)
  if(${NS3_TAP})
    set(ENABLE_TAP ON)
  endif()

  set(ENABLE_EMU OFF)
  if(${NS3_EMU})
    set(ENABLE_EMU ON)
  endif()

  set(PLATFORM_UNSUPPORTED_PRE "Platform doesn't support")
  set(PLATFORM_UNSUPPORTED_POST "features. Continuing without them.")
  # Remove from libs_to_build all incompatible libraries or the ones that
  # dependencies couldn't be installed
  if(APPLE
     OR WSLv1
     OR WIN32
     OR BSD
  )
    set(ENABLE_TAP OFF)
    set(ENABLE_EMU OFF)
    set(ENABLE_FDNETDEV FALSE)
    list(REMOVE_ITEM libs_to_build fd-net-device)
    message(
      STATUS
        "${PLATFORM_UNSUPPORTED_PRE} TAP and EMU ${PLATFORM_UNSUPPORTED_POST}"
    )
  endif()

  if(NOT ${ENABLE_MPI})
    list(REMOVE_ITEM libs_to_build mpi)
  endif()

  if(NOT ${ENABLE_VISUALIZER})
    list(REMOVE_ITEM libs_to_build visualizer)
  endif()

  if(NOT ${ENABLE_TAP})
    list(REMOVE_ITEM libs_to_build tap-bridge)
  endif()

  # Create library names to solve dependency problems with macros that will be
  # called at each lib subdirectory
  set(ns3-libs)
  set(ns3-all-enabled-modules)
  set(ns3-libs-tests)
  set(ns3-contrib-libs)
  set(ns3-external-libs)

  foreach(libname ${scanned_modules})
    # Create libname of output library of module
    library_target_name(${libname} targetname)
    mark_as_advanced(lib${libname} lib${libname}-obj)
    set(lib${libname} ${targetname} CACHE INTERNAL "")
    set(lib${libname}-obj ${targetname}-obj CACHE INTERNAL "")
  endforeach()

  if(${ENABLE_VISUALIZER} AND (visualizer IN_LIST libs_to_build))
    set(ns3-optional-visualizer-lib "${libvisualizer}"
        CACHE INTERNAL "visualizer library name"
    )
  endif()

  set(PRECOMPILE_HEADERS_ENABLED OFF)
  if(${NS3_PRECOMPILE_HEADERS})
    if(${NS3_CLANG_TIDY})
      message(
        ${HIGHLIGHTED_STATUS}
        "Clang-tidy is incompatible with precompiled headers. Continuing without them."
      )
    elseif(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16.0")
      # If ccache is not enable or was not found, we can continue with
      # precompiled headers
      if((NOT ${NS3_CCACHE}) OR ("${CCACHE}" STREQUAL "CCACHE-NOTFOUND"))
        set(PRECOMPILE_HEADERS_ENABLED ON)
        message(STATUS "Precompiled headers were enabled")
      else()
        # If ccache was found, we need to check if it is ccache >= 4
        execute_process(COMMAND ${CCACHE} -V OUTPUT_VARIABLE CCACHE_OUT)
        # Extract ccache version
        if(CCACHE_OUT MATCHES "ccache version ([0-9\.]*)")
          # If an incompatible version is found, do not enable precompiled
          # headers
          if("${CMAKE_MATCH_1}" VERSION_LESS "4.0.0")
            set(PRECOMPILE_HEADERS_ENABLED OFF)
            message(
              ${HIGHLIGHTED_STATUS}
              "Precompiled headers are incompatible with ccache ${CMAKE_MATCH_1} and will be disabled."
            )
          else()
            set(PRECOMPILE_HEADERS_ENABLED ON)
            message(STATUS "Precompiled headers were enabled.")
          endif()
        else()
          message(
            FATAL_ERROR
              "Failed to extract the ccache version while enabling precompiled headers."
          )
        endif()
      endif()
    else()
      message(
        STATUS
          "CMake ${CMAKE_VERSION} does not support precompiled headers. Continuing without them"
      )
    endif()
  endif()

  if(${PRECOMPILE_HEADERS_ENABLED})
    if(CLANG)
      # Clang adds a timestamp to the PCH, which prevents ccache from working
      # correctly
      # https://github.com/ccache/ccache/issues/539#issuecomment-664198545
      add_definitions(-Xclang -fno-pch-timestamp)
    endif()
    if(${XCODE})
      # XCode is weird and messes up with the PCH, requiring this flag
      # https://github.com/ccache/ccache/issues/156
      add_definitions(-Xclang -fno-validate-pch)
    endif()
    set(precompiled_header_libraries
        <algorithm>
        <cstdlib>
        <cstring>
        <exception>
        <deque>
        <fstream>
        <functional>
        <iostream>
        <limits>
        <list>
        <map>
        <math.h>
        <ostream>
        <queue>
        <set>
        <sstream>
        <stdint.h>
        <stdlib.h>
        <string>
        <tuple>
        <typeinfo>
        <type_traits>
        <unordered_map>
        <utility>
        <vector>
    )
    add_library(
      stdlib_pch${build_profile_suffix} OBJECT
      ${PROJECT_SOURCE_DIR}/build-support/empty.cc
    )
    target_precompile_headers(
      stdlib_pch${build_profile_suffix} PUBLIC
      "${precompiled_header_libraries}"
    )

    # Alias may collide with actual pch in builds without suffix (e.g. release)
    if(NOT TARGET stdlib_pch)
      add_library(stdlib_pch ALIAS stdlib_pch${build_profile_suffix})
    endif()

    add_executable(
      stdlib_pch_exec ${PROJECT_SOURCE_DIR}/build-support/empty-main.cc
    )
    target_precompile_headers(
      stdlib_pch_exec PUBLIC "${precompiled_header_libraries}"
    )
    set_runtime_outputdirectory(stdlib_pch_exec ${CMAKE_BINARY_DIR}/ "")
  endif()

  # Create new lib for NS3 static builds
  set(lib-ns3-static ns${NS3_VER}-static${build_profile_suffix})

  # Create a new lib for NS3 monolibrary shared builds
  set(lib-ns3-monolib ns${NS3_VER}-monolib${build_profile_suffix})

  # All contrib libraries can be linked afterwards linking with
  # ${ns3-contrib-libs}
  process_contribution("${contrib_libs_to_build}")

  # Netanim depends on ns-3 core, so we built it later
  if(${NS3_NETANIM})
    include(ExternalProject)
    ExternalProject_Add(
      netanim_visualizer
      GIT_REPOSITORY https://gitlab.com/nsnam/netanim.git
      GIT_TAG netanim-3.110
      BUILD_IN_SOURCE TRUE
      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_OUTPUT_DIRECTORY}
    )
  endif()

  if(${NS3_FETCH_OPTIONAL_COMPONENTS})
    include(ns3-fetch-optional-modules-dependencies)
  endif()
endmacro()

function(get_target_includes target output)
  set(include_directories)
  get_target_property(include_dirs ${target} INCLUDE_DIRECTORIES)
  list(REMOVE_DUPLICATES include_dirs)
  foreach(include_dir ${include_dirs})
    if(include_dir MATCHES "<")
      # Skip CMake build and install interface includes
      continue()
    else()
      # Append the include directory to a list
      set(include_directories ${include_directories} -I${include_dir})
    endif()
  endforeach()
  set(${output} ${include_directories} PARENT_SCOPE)
endfunction()

# Macros related to the definition of executables
include(ns3-executables)

# Import macros used for modules and define specialized versions for src modules
include(ns3-module-macros)

# Contrib modules counterparts of macros above
include(ns3-contributions)

# Macros for enabled/disabled module filtering
include(ns3-filter-modules)

# .ns3rc configuration file parsers
include(ns3-ns3rc-parser)

# Macro to build examples in ns-3-dev/examples/
macro(build_example)
  set(options IGNORE_PCH)
  set(oneValueArgs NAME)
  set(multiValueArgs SOURCE_FILES HEADER_FILES LIBRARIES_TO_LINK)
  cmake_parse_arguments(
    "EXAMPLE" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
  )

  # Filter examples out if they don't contain one of the filtered in modules
  set(filtered_in ON)
  if(NS3_FILTER_MODULE_EXAMPLES_AND_TESTS)
    set(filtered_in OFF)
    foreach(required_module ${EXAMPLE_LIBRARIES_TO_LINK})
      if(${required_module} IN_LIST NS3_FILTER_MODULE_EXAMPLES_AND_TESTS)
        set(filtered_in ON)
      endif()
    endforeach()
  endif()

  check_for_missing_libraries(
    missing_dependencies "${EXAMPLE_LIBRARIES_TO_LINK}"
  )

  if((NOT missing_dependencies) AND ${filtered_in})
    # Convert boolean into text to forward argument
    set(IGNORE_PCH)
    if(${EXAMPLE_IGNORE_PCH})
      set(IGNORE_PCH "IGNORE_PCH")
    endif()

    set(current_directory ${CMAKE_CURRENT_SOURCE_DIR})
    string(REPLACE "${PROJECT_SOURCE_DIR}" "" current_directory
                   "${current_directory}"
    )
    if("${current_directory}" MATCHES ".*/(src|contrib)/.*")
      message(
        FATAL_ERROR
          "build_example() macro is meant for ns-3-dev/examples, and not for modules. Use build_lib_example() instead."
      )
    endif()

    get_filename_component(examplefolder ${CMAKE_CURRENT_SOURCE_DIR} NAME)

    # Create example library with sources and headers
    # cmake-format: off
    build_exec(
            EXECNAME ${EXAMPLE_NAME}
            SOURCE_FILES ${EXAMPLE_SOURCE_FILES}
            HEADER_FILES ${EXAMPLE_HEADER_FILES}
            LIBRARIES_TO_LINK ${EXAMPLE_LIBRARIES_TO_LINK} ${ns3-optional-visualizer-lib}
            EXECUTABLE_DIRECTORY_PATH
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/examples/${examplefolder}/
            ${IGNORE_PCH}
    )
    # cmake-format: on
  endif()
endmacro()

# Macros to write the lock file
include(ns3-lock)

# Macros to build the config table file
include(ns3-configtable)
