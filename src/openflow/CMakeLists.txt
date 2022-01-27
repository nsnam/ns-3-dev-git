set(NS3_WITH_OPENFLOW
    ""
    CACHE PATH
          "Build with Openflow support"
)
set(NS3_OPENFLOW
    "OFF"
    CACHE INTERNAL
          "ON if Openflow is found in NS3_WITH_OPENFLOW"
)

find_external_library_header_and_library(
  "openflow"
  "openflow.h"
  "openflow"
  "${NS3_WITH_OPENFLOW}"
)
if(NOT
   (openflow_library
    AND openflow_header)
)
  message(STATUS "Openflow was not found")
  return()
endif()

check_include_file_cxx(
  boost/static_assert.hpp
  BOOST_STATIC_ASSERT
)
if(NOT
   BOOST_STATIC_ASSERT
)
  message(STATUS "Openflow requires Boost static_assert.hpp")
  return()
endif()

include_directories(${openflow_include_directories})
add_definitions(
  -DNS3_OPENFLOW
  -DENABLE_OPENFLOW
)
set(NS3_OPENFLOW
    "ON"
    CACHE INTERNAL
          "ON if Openflow is found in NS3_WITH_OPENFLOW"
)

build_lib(
  LIBNAME openflow
  SOURCE_FILES
    helper/openflow-switch-helper.cc
    model/openflow-interface.cc
    model/openflow-switch-net-device.cc
  HEADER_FILES
    helper/openflow-switch-helper.h
    model/openflow-interface.h
    model/openflow-switch-net-device.h
  LIBRARIES_TO_LINK ${libinternet}
                    ${openflow_library}
  TEST_SOURCES test/openflow-switch-test-suite.cc
)
