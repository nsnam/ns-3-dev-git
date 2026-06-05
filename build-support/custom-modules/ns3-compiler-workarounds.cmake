# Copyright (c) 2022 Universidade de Brasília
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>

include(CheckCXXSourceCompiles)

# Some versions of clang (17) have issues with stdlibc++, so we check if we can
# fallback to libc++ instead. This only concerns clang targeting libstdc++ or
# libc++; ClangCL uses the MSVC STL, where the -stdlib= flag is meaningless (it
# is silently ignored, so both probes "succeed" using the MSVC STL anyway). Skip
# them there: they are two compiler invocations (~5s of first-configure time)
# that test nothing relevant on Windows.
if(CLANG AND NOT (CMAKE_CXX_SIMULATE_ID MATCHES "MSVC"))
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
  # std::stacktrace may need an extra backend library, and which one is required
  # depends on the standard library in use, not just on the compiler. libstdc++
  # 13 and newer ships the backend in libstdc++exp; libstdc++ 12 to 13 used
  # libstdc++_libbacktrace; libc++ and the MSVC STL need no extra library. Since
  # clang can be paired with any of these, probe the candidates in order and
  # keep the first that actually links, instead of hard-coding one library per
  # compiler (which left clang + modern libstdc++ without a working backend,
  # silently disabling stacktraces).
  string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  set(stacktrace_candidates "DEFAULT" "-lstdc++exp" "-lstdc++_libbacktrace")
  set(stacktrace_candidate_idx 0)
  foreach(stacktrace_candidate IN LISTS stacktrace_candidates)
    if(stacktrace_candidate STREQUAL "DEFAULT")
      set(CMAKE_REQUIRED_LIBRARIES)
      set(stacktrace_flag_value "")
    else()
      set(CMAKE_REQUIRED_LIBRARIES ${stacktrace_candidate})
      set(stacktrace_flag_value "${stacktrace_candidate}")
    endif()
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
      STACKTRACE_LIBRARY_IS_LINKED_${stacktrace_candidate_idx}
    )
    if(STACKTRACE_LIBRARY_IS_LINKED_${stacktrace_candidate_idx})
      set(stacktrace_flags "${stacktrace_flag_value}" CACHE INTERNAL "" FORCE)
      set(STACKTRACE_LIBRARY_IS_LINKED TRUE)
      break()
    endif()
    math(EXPR stacktrace_candidate_idx "${stacktrace_candidate_idx} + 1")
  endforeach()
  set(CMAKE_REQUIRED_LIBRARIES)
  if(STACKTRACE_LIBRARY_IS_LINKED)
    add_definitions(-DSTACKTRACE_LIBRARY_IS_LINKED=1)
  else()
    set(stacktrace_flags "" CACHE INTERNAL "" FORCE)
  endif()
endif()
