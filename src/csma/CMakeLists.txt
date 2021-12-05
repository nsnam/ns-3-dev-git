set(name csma)

set(source_files helper/csma-helper.cc model/backoff.cc model/csma-channel.cc
                 model/csma-net-device.cc
)

set(header_files helper/csma-helper.h model/backoff.h model/csma-channel.h
                 model/csma-net-device.h
)

# Set lib csma link dependencies
set(libraries_to_link ${libnetwork})

set(test_sources)

# Build csma lib
build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
          "${test_sources}"
)
