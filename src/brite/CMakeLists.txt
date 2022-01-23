set(NS3_WITH_BRITE "" CACHE PATH "Build with brite support")
set(NS3_BRITE "OFF" CACHE INTERNAL "ON if Brite is found in NS3_WITH_BRITE")

find_external_library_header_and_library(
  "brite" "Brite.h" "brite" "${NS3_WITH_BRITE}"
)
if(NOT (brite_library AND brite_header))
  message(STATUS "Brite was not found")
  return()
endif()

# Only process module if include folder and library have been found
include_directories(${brite_include_directories})
set(NS3_BRITE "ON" CACHE INTERNAL "ON if Brite is found in NS3_WITH_BRITE")

set(name brite)

set(source_files helper/brite-topology-helper.cc)

set(header_files helper/brite-topology-helper.h)

# link to dependencies
set(libraries_to_link ${libnetwork} ${libcore} ${libinternet}
                      ${libpoint-to-point} ${brite_library}
)

set(test_sources test/brite-test-topology.cc)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
          "${test_sources}"
)
