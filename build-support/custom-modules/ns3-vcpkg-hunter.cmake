# Copyright (c) 2017-2023 Universidade de Brasília
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>

# Set default installation directory
set(VCPKG_DIR "${PROJECT_SOURCE_DIR}/vcpkg")

# Check if there is an existing vcpkg installation in the user directory
if(WIN32)
  set(user_dir $ENV{USERPROFILE})
else()
  set(user_dir $ENV{HOME})
endif()

# Override the default vcpkg installation directory in case it does
if(EXISTS "${user_dir}/vcpkg")
  set(VCPKG_DIR "${user_dir}/vcpkg")
endif()

find_package(Git)

if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
  set(VCPKG_TARGET_ARCH x64)
else()
  set(VCPKG_TARGET_ARCH x86)
endif()

if(WIN32)
  set(VCPKG_EXEC vcpkg.exe)
  set(VCPKG_TRIPLET ${VCPKG_TARGET_ARCH}-windows)
else()
  set(VCPKG_EXEC vcpkg)
  if(NOT APPLE) # LINUX
    set(VCPKG_TRIPLET ${VCPKG_TARGET_ARCH}-linux)
  else()
    set(VCPKG_TRIPLET ${VCPKG_TARGET_ARCH}-osx)
  endif()
endif()

# https://learn.microsoft.com/en-us/vcpkg/users/buildsystems/cmake-integration
set(VCPKG_TARGET_TRIPLET ${VCPKG_TRIPLET})
set(VCPKG_MANIFEST ${PROJECT_SOURCE_DIR}/vcpkg.json)

function(setup_vcpkg)
  message(STATUS "vcpkg: setting up support in ${VCPKG_DIR}")

  # Check if vcpkg was downloaded previously
  if(EXISTS "${VCPKG_DIR}")
    # Vcpkg already downloaded
    message(STATUS "vcpkg: folder already exists, skipping git download")
  else()
    if(NOT ${Git_FOUND})
      message(FATAL_ERROR "vcpkg: Git is required, but it was not found")
    endif()
    get_filename_component(VCPKG_PARENT_DIR ${VCPKG_DIR} DIRECTORY)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} clone --depth 1
              https://github.com/microsoft/vcpkg.git
      WORKING_DIRECTORY "${VCPKG_PARENT_DIR}/"
    )
  endif()

  if(${MSVC})
    message(FATAL_ERROR "vcpkg: Visual Studio is unsupported")
  else()
    # Check if required packages are installed (unzip curl tar)
    if(WIN32)
      find_program(ZIP_PRESENT zip.exe)
      find_program(UNZIP_PRESENT unzip.exe)
      find_program(CURL_PRESENT curl.exe)
      find_program(TAR_PRESENT tar.exe)
    else()
      find_program(ZIP_PRESENT zip)
      find_program(UNZIP_PRESENT unzip)
      find_program(CURL_PRESENT curl)
      find_program(TAR_PRESENT tar)
    endif()

    if(${ZIP_PRESENT} STREQUAL ZIP_PRESENT-NOTFOUND)
      message(FATAL_ERROR "vcpkg: Zip is required, but is not installed")
    endif()

    if(${UNZIP_PRESENT} STREQUAL UNZIP_PRESENT-NOTFOUND)
      message(FATAL_ERROR "vcpkg: Unzip is required, but is not installed")
    endif()

    if(${CURL_PRESENT} STREQUAL CURL_PRESENT-NOTFOUND)
      message(FATAL_ERROR "vcpkg: Curl is required, but is not installed")
    endif()

    if(${TAR_PRESENT} STREQUAL TAR_PRESENT-NOTFOUND)
      message(FATAL_ERROR "vcpkg: Tar is required, but is not installed")
    endif()
  endif()

  # message(WARNING "Checking VCPKG bootstrapping") Check if vcpkg was
  # bootstrapped previously
  if(EXISTS "${VCPKG_DIR}/${VCPKG_EXEC}")
    message(STATUS "vcpkg: already bootstrapped")
  else()
    # message(WARNING "vcpkg: bootstrapping")
    set(COMPILER_ENFORCING)

    if(WIN32)
      set(command bootstrap-vcpkg.bat -disableMetrics)
    else()
      # if(NOT APPLE) #linux/bsd
      set(command bootstrap-vcpkg.sh -disableMetrics)
      # else() set(command bootstrap-vcpkg.sh)# --allowAppleClang) endif()
    endif()

    execute_process(
      COMMAND ${COMPILER_ENFORCING} ${VCPKG_DIR}/${command}
      WORKING_DIRECTORY ${VCPKG_DIR}
    )
    # message(STATUS "vcpkg: bootstrapped") include_directories(${VCPKG_DIR})
    set(ENV{VCPKG_ROOT} ${VCPKG_DIR})
  endif()

  if(NOT WIN32)
    execute_process(COMMAND chmod +x ${VCPKG_DIR}/${VCPKG_EXEC})
  endif()

  disable_cmake_warnings()
  set(CMAKE_PREFIX_PATH
      "${VCPKG_DIR}/installed/${VCPKG_TRIPLET}/;${CMAKE_PREFIX_PATH}"
      PARENT_SCOPE
  )
  enable_cmake_warnings()

  # Install packages in manifest mode
  if(EXISTS ${VCPKG_MANIFEST})
    message(STATUS "vcpkg: detected a vcpkg manifest file: ${VCPKG_MANIFEST}")
    execute_process(
      COMMAND ${VCPKG_DIR}/${VCPKG_EXEC} install --triplet ${VCPKG_TRIPLET}
              --x-install-root=${VCPKG_DIR}/installed RESULT_VARIABLE res
    )
    if(${res} EQUAL 0)
      message(STATUS "vcpkg: packages defined in the manifest were installed")
    else()
      message(
        FATAL_ERROR
          "vcpkg: packages defined in the manifest failed to be installed"
      )
    endif()
  endif()
endfunction()

function(add_package package_name)
  # Early exit in case vcpkg support is not enabled
  if(NOT ${NS3_VCPKG})
    message(${HIGHLIGHTED_STATUS}
            "vcpkg: support is disabled. Not installing: ${package_name}"
    )
    return()
  endif()

  # Early exit in case a vcpkg.json manifest file is present
  if(EXISTS ${VCPKG_MANIFEST})
    file(READ ${VCPKG_MANIFEST} contents)
    if(${contents} MATCHES ${package_name})
      message(STATUS "vcpkg: ${package_name} was installed")
      return()
    else()
      message(
        FATAL_ERROR
          "vcpkg: manifest mode in use, but ${package_name} is not listed in ${VCPKG_MANIFEST}"
      )
    endif()
  endif()

  # Normal exit
  message(STATUS "vcpkg: ${package_name} will be installed")
  execute_process(
    COMMAND ${VCPKG_DIR}/${VCPKG_EXEC} install ${package_name} --triplet
            ${VCPKG_TRIPLET} RESULT_VARIABLE res
  )
  if(${res} EQUAL 0)
    message(STATUS "vcpkg: ${package_name} was installed")
  else()
    message(FATAL_ERROR "vcpkg: ${package_name} failed to be installed")
  endif()
endfunction()

if(${NS3_VCPKG})
  setup_vcpkg()
  include_directories(${VCPKG_DIR}/installed/${VCPKG_TRIPLET}/include)
  link_directories(${VCPKG_DIR}/installed/${VCPKG_TRIPLET}/lib)
endif()
