set(NS3_WITH_OPENFLOW "" CACHE PATH "Build with Openflow support")
set(NS3_OPENFLOW "OFF" CACHE INTERNAL
                             "ON if Openflow is found in NS3_WITH_OPENFLOW"
)

find_external_library_header_and_library(
  "openflow" "openflow.h" "openflow" "${NS3_WITH_OPENFLOW}"
)
if(NOT (openflow_library AND openflow_header))
  message(STATUS "Openflow was not found")
  return()
endif()

check_include_file_cxx(boost/static_assert.hpp BOOST_STATIC_ASSERT)
if(NOT BOOST_STATIC_ASSERT)
  message(STATUS "Openflow requires Boost static_assert.hpp")
  return()
endif()

include_directories(${openflow_include_directories})
add_definitions(-DNS3_OPENFLOW -DENABLE_OPENFLOW)
set(NS3_OPENFLOW "ON" CACHE INTERNAL
                            "ON if Openflow is found in NS3_WITH_OPENFLOW"
)

set(name openflow)

set(source_files helper/openflow-switch-helper.cc model/openflow-interface.cc
                 model/openflow-switch-net-device.cc
)

set(header_files helper/openflow-switch-helper.h model/openflow-interface.h
                 model/openflow-switch-net-device.h
)

set(libraries_to_link ${libinternet} ${openflow_library})

set(test_sources test/openflow-switch-test-suite.cc)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
          "${test_sources}"
)
