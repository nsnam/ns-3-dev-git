set(name tap-bridge)

set(source_files helper/tap-bridge-helper.cc model/tap-bridge.cc
                 model/tap-encode-decode.cc
)

set(header_files helper/tap-bridge-helper.h model/tap-bridge.h
                 model/tap-encode-decode.h
)

set(libraries_to_link ${libcore} ${libinternet} ${libnetwork})

set(test_sources)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
          "${test_sources}"
)

add_definitions(
  -DTAP_CREATOR="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/src/tap-bridge/ns${NS3_VER}-tap-creator${build_profile_suffix}"
)
add_executable(tap-creator model/tap-creator.cc model/tap-encode-decode.cc)
set_runtime_outputdirectory(
  tap-creator ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/src/tap-bridge/ ""
)
