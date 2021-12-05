set(NS3_WITH_CLICK "" CACHE PATH "Build with click support")
set(NS3_CLICK "OFF" CACHE INTERNAL "ON if Click is found in NS3_WITH_CLICK")

if(NOT NS3_WITH_CLICK)
  return()
endif()

find_library(
  click_dep click PATHS ${NS3_WITH_CLICK} PATH_SUFFIXES /build /build/lib /lib
)
find_file(click_header simclick.h HINTS ${NS3_WITH_CLICK}
          PATH_SUFFIXES /build /include /build/include /build/include/click
                        /include/click
)

if(NOT (click_dep AND click_header))
  message(STATUS "Click was not found in ${NS3_WITH_CLICK}")
  return()
endif()

get_filename_component(
  openflow_header_include_folder ${openflow_header} DIRECTORY
) # include/click/ (simclick.h)
get_filename_component(
  openflow_header_include_folder ${openflow_header_include_folder} DIRECTORY
) # include/(click)
include_directories(${openflow_header_include_folder})
set(NS3_CLICK "ON" CACHE INTERNAL "ON if Click is found in NS3_WITH_CLICK")
add_definitions(-DNS3_CLICK)

set(name click)

set(source_files helper/click-internet-stack-helper.cc
                 model/ipv4-click-routing.cc model/ipv4-l3-click-protocol.cc
)

set(header_files helper/click-internet-stack-helper.h
                 model/ipv4-click-routing.h model/ipv4-l3-click-protocol.h
)

set(libraries_to_link ${libcore} ${libnetwork} ${libinternet} ${click_dep})

set(test_sources test/ipv4-click-routing-test.cc)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
          "${test_sources}"
)

# click install headers when finished and we can't start compiling before that
# happens
if(NOT ${XCODE})
  add_dependencies(${libclick-obj} ${click_dep})
endif()
