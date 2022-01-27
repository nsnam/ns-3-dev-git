set(NS3_WITH_BRITE
    ""
    CACHE PATH
          "Build with brite support"
)
set(NS3_BRITE
    "OFF"
    CACHE INTERNAL
          "ON if Brite is found in NS3_WITH_BRITE"
)

find_external_library_header_and_library(
  "brite"
  "Brite.h"
  "brite"
  "${NS3_WITH_BRITE}"
)
if(NOT
   (brite_library
    AND brite_header)
)
  message(STATUS "Brite was not found")
  return()
endif()

# Only process module if include folder and library have been found
include_directories(${brite_include_directories})
set(NS3_BRITE
    "ON"
    CACHE INTERNAL
          "ON if Brite is found in NS3_WITH_BRITE"
)

build_lib(
  LIBNAME brite
  SOURCE_FILES helper/brite-topology-helper.cc
  HEADER_FILES helper/brite-topology-helper.h
  LIBRARIES_TO_LINK
    ${libnetwork}
    ${libcore}
    ${libinternet}
    ${libpoint-to-point}
    ${brite_library}
  TEST_SOURCES test/brite-test-topology.cc
)
