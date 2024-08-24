include(ExternalProject)

ExternalProject_Add(
  brite_dep
  GIT_REPOSITORY https://gitlab.com/nsnam/BRITE.git
  GIT_TAG 2a665ae11740a11afec1b068c2a45f21d901cf55
  PREFIX brite_dep
  BUILD_IN_SOURCE TRUE
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_OUTPUT_DIRECTORY}
)

ExternalProject_Add(
  click_dep
  GIT_REPOSITORY https://github.com/kohler/click.git
  GIT_TAG 9197a594b1c314264935106297aff08d97cbe7ee
  PREFIX click_dep
  BUILD_IN_SOURCE TRUE
  UPDATE_DISCONNECTED TRUE
  CONFIGURE_COMMAND
    ./configure --disable-linuxmodule --enable-nsclick --enable-wifi --prefix
    ${CMAKE_OUTPUT_DIRECTORY} --libdir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
  BUILD_COMMAND make -j${NumThreads}
  INSTALL_COMMAND make install
)

ExternalProject_Add(
  openflow_dep
  GIT_REPOSITORY https://gitlab.com/nsnam/openflow.git
  GIT_TAG 4869d4f6900342440af02ea93a3d8040c8316e5f
  PREFIX openflow_dep
  BUILD_IN_SOURCE TRUE
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_OUTPUT_DIRECTORY}
)

find_file(
  BOOST_STATIC_ASSERT
  NAMES static_assert.hpp
  PATH_SUFFIXES boost
  HINTS /usr/local
)

if(${BOOST_STATIC_ASSERT} STREQUAL "BOOST_STATIC_ASSERT-NOTFOUND")
  message(FATAL_ERROR "Boost static assert is required by openflow")
endif()

get_filename_component(boost_dir ${BOOST_STATIC_ASSERT} DIRECTORY)

install(
  DIRECTORY ${boost_dir}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  USE_SOURCE_PERMISSIONS
  PATTERN "boost/*"
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
  DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/${libdir}/
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  USE_SOURCE_PERMISSIONS
  PATTERN "${libdir}/*"
)

macro(add_dependency_to_optional_modules_dependencies)
  add_dependencies(${libbrite} brite_dep)
  add_dependencies(${libclick} click_dep)
  add_dependencies(${libopenflow} openflow_dep)
endmacro()
