set(NS3_WITH_CLICK
    ""
    CACHE PATH
          "Build with click support"
)
set(NS3_CLICK
    "OFF"
    CACHE INTERNAL
          "ON if Click is found in NS3_WITH_CLICK"
)

find_external_library_header_and_library(
  "click"
  "simclick.h"
  "click"
  "${NS3_WITH_CLICK}"
)
if(NOT
   (click_library
    AND click_header)
)
  message(STATUS "Click was not found")
  return()
endif()

include_directories(${click_include_directories})
set(NS3_CLICK
    "ON"
    CACHE INTERNAL
          "ON if Click is found in NS3_WITH_CLICK"
)
add_definitions(-DNS3_CLICK)

build_lib(
  LIBNAME click
  SOURCE_FILES
    helper/click-internet-stack-helper.cc
    model/ipv4-click-routing.cc
    model/ipv4-l3-click-protocol.cc
  HEADER_FILES
    helper/click-internet-stack-helper.h
    model/ipv4-click-routing.h
    model/ipv4-l3-click-protocol.h
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libinternet}
    ${click_library}
  TEST_SOURCES test/ipv4-click-routing-test.cc
)
