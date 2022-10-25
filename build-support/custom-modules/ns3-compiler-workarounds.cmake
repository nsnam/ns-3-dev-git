# Copyright (c) 2022 Universidade de Brasília
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

include(CheckCXXSourceCompiles)

# Clang 6, 7 and 8 shipped with incomplete C++17 features and do not handle
# ostream& operator<<(ostream& os, nullptr_t ptr)
# https://gitlab.com/nsnam/ns-3-dev/-/issues/730
check_cxx_source_compiles(
  "
    #include <iostream>
    #include <cstddef>
    inline std::ostream& operator << (std::ostream& os, std::nullptr_t ptr)
    {
      return os << \"nullptr\"; //whatever you want nullptr to show up as in the console
    }
    int main()
    {
        std::ostream os(NULL);
        os << std::nullptr_t();
        return 0;
    }
    "
  MISSING_OSTREAM_NULLPTR_OPERATOR
)

if(${MISSING_OSTREAM_NULLPTR_OPERATOR})
  message(
    ${HIGHLIGHTED_STATUS}
    "Using compiler workaround: compiling in \"ostream& operator<<(ostream&, nullptr_t)\""
  )
  add_definitions(
    -include
    ${CMAKE_CURRENT_SOURCE_DIR}/build-support/compiler-workarounds/ostream-operator-nullptr.h
  )
endif()

# Some compilers (e.g. GCC < 9.1 and Clang < 9) do not link
# std::filesystem/std::experimental::filesystem by default. If the sample
# program can be linked, it means it is indeed linked by default. Otherwise, we
# link it manually. https://en.cppreference.com/w/cpp/filesystem
check_cxx_source_compiles(
  "
  #ifdef __has_include
    #if __has_include(<filesystem>)
      #include <filesystem>
      namespace fs = std::filesystem;
    #elif __has_include(<experimental/filesystem>)
      #include <experimental/filesystem>
      namespace fs = std::experimental::filesystem;
    #else
      #error \"No support for filesystem library\"
    #endif
  #endif
  int main()
  {
    std::string path = \"/\";
    return !fs::exists(path);
  }
  "
  FILESYSTEM_LIBRARY_IS_LINKED
)
