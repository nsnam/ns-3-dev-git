set(NS3_WITH_BRITE "" CACHE PATH "Build with brite support")
set(NS3_BRITE "OFF" CACHE INTERNAL "ON if Brite is found in NS3_WITH_BRITE")
if(NOT NS3_WITH_BRITE)
  return()
endif()

find_library(brite_dep brite PATHS ${NS3_WITH_BRITE} PATH_SUFFIXES /build /build/lib /lib)
find_file(brite_header Brite.h HINTS ${NS3_WITH_BRITE} PATH_SUFFIXES /build /build/include /include)

if(NOT (brite_dep AND brite_header))
  message(STATUS "Brite was not found in ${NS3_WITH_BRITE}")
  return()
endif()

# Only process module if include folder and library have been found
get_filename_component(brite_include_folder ${brite_header} DIRECTORY)
include_directories(${brite_include_folder})
set(NS3_BRITE "ON" CACHE INTERNAL "ON if Brite is found in NS3_WITH_BRITE")

set(name brite)

set(source_files helper/brite-topology-helper.cc)

set(header_files helper/brite-topology-helper.h)

# link to dependencies
set(libraries_to_link ${libnetwork} ${libcore} ${libinternet} ${libpoint-to-point} ${brite_dep})

set(test_sources test/brite-test-topology.cc)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")
