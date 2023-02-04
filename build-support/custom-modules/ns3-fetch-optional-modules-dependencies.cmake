include(ExternalProject)

ExternalProject_Add(
  brite_dep
  URL https://code.nsnam.org/BRITE/archive/30338f4f63b9.zip
  URL_HASH MD5=b36ecf8f6b5f2cfae936ba1f1bfcff5c
  PREFIX brite_dep
  BUILD_IN_SOURCE TRUE
  CONFIGURE_COMMAND make
  BUILD_COMMAND make
  INSTALL_COMMAND make install PREFIX=${CMAKE_OUTPUT_DIRECTORY}
)

ExternalProject_Add(
  click_dep
  GIT_REPOSITORY https://github.com/kohler/click.git
  GIT_TAG 9197a594b1c314264935106297aff08d97cbe7ee
  PREFIX click_dep
  BUILD_IN_SOURCE TRUE
  UPDATE_DISCONNECTED TRUE
  CONFIGURE_COMMAND ./configure --disable-linuxmodule --enable-nsclick
                    --enable-wifi --prefix ${CMAKE_OUTPUT_DIRECTORY}
  BUILD_COMMAND make -j${NumThreads}
  INSTALL_COMMAND make install
)

ExternalProject_Add(
  openflow_dep
  URL https://code.nsnam.org/openflow/archive/d45e7d184151.zip
  URL_HASH MD5=a068cdaec5523586921b2f1f81f10916
  PREFIX openflow_dep
  BUILD_IN_SOURCE TRUE
  CONFIGURE_COMMAND ./waf configure --prefix ${CMAKE_OUTPUT_DIRECTORY}
  BUILD_COMMAND ./waf build
  INSTALL_COMMAND ./waf install
)

install(
  DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/include/openflow
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include
  USE_SOURCE_PERMISSIONS
  PATTERN "openflow/*"
)

find_file(
  BOOST_STATIC_ASSERT
  NAMES static_assert.hpp
  PATH_SUFFIXES boost
  HINTS /usr/local
)
get_filename_component(boost_dir ${BOOST_STATIC_ASSERT} DIRECTORY)

install(
  DIRECTORY ${boost_dir}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  USE_SOURCE_PERMISSIONS
  PATTERN "boost/*"
)
install(
  DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/lib/
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  USE_SOURCE_PERMISSIONS
  PATTERN "lib/*"
)

macro(add_dependency_to_optional_modules_dependencies)
  add_dependencies(${libbrite} brite_dep)
  add_dependencies(${libclick} click_dep)
  add_dependencies(${libopenflow} openflow_dep)
  if(NOT ${XCODE})
    add_dependencies(${libbrite}-obj brite_dep)
    add_dependencies(${libclick}-obj click_dep)
    add_dependencies(${libopenflow}-obj openflow_dep)
  endif()
endmacro()
