set(NS3_WITH_OPENFLOW "" CACHE PATH "Build with Openflow support")
set(NS3_OPENFLOW "OFF" CACHE INTERNAL
                             "ON if Openflow is found in NS3_WITH_OPENFLOW"
)

if(NOT NS3_WITH_OPENFLOW)
  return()
endif()

find_library(
  openflow_dep openflow PATHS ${NS3_WITH_OPENFLOW} PATH_SUFFIXES /build /lib
                                                                 /build/lib
)
find_file(openflow_header openflow.h HINTS ${NS3_WITH_OPENFLOW}
          PATH_SUFFIXES /build /include /build/include /build/include/openflow
                        /include/openflow
)

if(NOT (openflow_dep AND openflow_header))
  message(STATUS "Openflow was not found in ${NS3_WITH_OPENFLOW}")
  return()
endif()

get_filename_component(
  openflow_header_include_folder ${openflow_header} DIRECTORY
) # include/openflow/ (openflow.h)
get_filename_component(
  openflow_header_include_folder ${openflow_header_include_folder} DIRECTORY
) # include/(openflow)
include_directories(${openflow_header_include_folder})
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

set(libraries_to_link ${libinternet} ${openflow_dep})

set(test_sources test/openflow-switch-test-suite.cc)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
          "${test_sources}"
)
