# Copyright (c) 2022 Universidade de Brasília
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>

include(CheckCXXSourceCompiles)

# Some versions of clang (17) have issues with stdlibc++, so we check if we can
# fallback to libc++ instead
if(CLANG)
  try_compile(
    CLANG_STDLIBCPP_WORKS
    SOURCES ${PROJECT_SOURCE_DIR}/build-support/test-clang17-stdlibcpp-map.cc
    COMPILE_DEFINITIONS -stdlib=libstdc++
    LINK_OPTIONS -stdlib=libstdc++
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED TRUE
    CXX_EXTENSIONS FALSE
  )

  try_compile(
    CLANG_LIBCPP_WORKS
    SOURCES ${PROJECT_SOURCE_DIR}/build-support/test-clang17-stdlibcpp-map.cc
    COMPILE_DEFINITIONS -stdlib=libc++
    LINK_OPTIONS -stdlib=libc++
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED TRUE
    CXX_EXTENSIONS FALSE
  )
  if((NOT ${CLANG_STDLIBCPP_WORKS}) AND (NOT ${CLANG_LIBCPP_WORKS}))
    message(
      FATAL_ERROR
        "This clang version ${CMAKE_CXX_COMPILER_VERSION} is not compatible with using std::tuple as std::map keys"
    )
  elseif(${CLANG_STDLIBCPP_WORKS})
    message(
      STATUS
        "This clang version ${CMAKE_CXX_COMPILER_VERSION} is compatible with using std::tuple as std::map keys"
    )
  else()
    message(
      ${HIGHLIGHTED_STATUS}
      "This clang version ${CMAKE_CXX_COMPILER_VERSION} requires libc++ to use std::tuple as std::map keys"
    )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  endif()
endif()

# Some compilers (e.g. GCC <= 15) do not link std::stacktrace by default. If the
# sample program can be linked, it means it is indeed linked by default.
# Otherwise, we link it manually.
# https://en.cppreference.com/w/cpp/header/stacktrace
set(CMAKE_REQUIRED_FLAGS ${CMAKE_CXX_FLAGS} -std=c++23)
check_cxx_source_compiles(
  "
  #if __has_include(<stacktrace>)
  int main()
  {
    return 0;
  }
  #endif
  "
  STACKTRACE_LIBRARY_ENABLED
)
mark_as_advanced(stacktrace_flags)
set(stacktrace_flags "" CACHE INTERNAL "")
if(STACKTRACE_LIBRARY_ENABLED)
  if(GCC)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "12.0.0")
      # GCC does not support stacktracing
    elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "14.0.0")
      set(stacktrace_flags -lstdc++_libbacktrace CACHE INTERNAL "")
    else()
      set(stacktrace_flags -lstdc++exp CACHE INTERNAL "")
    endif()
  elseif(CLANG AND (NOT ("${CMAKE_CXX_SIMULATE_ID}" MATCHES "MSVC")))
    set(stacktrace_flags -lstdc++_libbacktrace CACHE INTERNAL "")
  else()
    # Most likely MSVC, which does not need custom flags for this
  endif()
  set(CMAKE_REQUIRED_LIBRARIES ${stacktrace_flags})
  string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  check_cxx_source_compiles(
    "
    #include <iostream>
    #include <stacktrace>

    int main()
    {
      std::cout << std::stacktrace::current() << std::endl;
      return 0;
    }
    "
    STACKTRACE_LIBRARY_IS_LINKED
  )

  if(STACKTRACE_LIBRARY_IS_LINKED)
    add_definitions(-DSTACKTRACE_LIBRARY_IS_LINKED=1)
  else()
    set(stacktrace_flags "" CACHE INTERNAL "")
  endif()
endif()
