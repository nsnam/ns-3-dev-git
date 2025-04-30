# Copyright (c) 2022 Universidade de Brasília
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>

include(CheckCXXSourceCompiles)

# Some compilers (e.g. GCC <= 15) do not link std::stacktrace by default. If the
# sample program can be linked, it means it is indeed linked by default.
# Otherwise, we link it manually.
# https://en.cppreference.com/w/cpp/header/stacktrace
set(CMAKE_REQUIRED_FLAGS -std=c++23)
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
if(STACKTRACE_LIBRARY_ENABLED)
  set(stacktrace_flags)
  if(GCC)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "12.0.0")
      # GCC does not support stacktracing
    elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "14.0.0")
      set(stacktrace_flags -lstdc++_libbacktrace)
    else()
      set(stacktrace_flags -lstdc++exp)
    endif()
  elseif(CLANG)
    set(stacktrace_flags -lstdc++_libbacktrace)
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
    link_libraries(${stacktrace_flags})
  endif()
endif()
