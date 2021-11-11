set(name bridge)

set(source_files helper/bridge-helper.cc model/bridge-channel.cc model/bridge-net-device.cc)

set(header_files helper/bridge-helper.h model/bridge-channel.h model/bridge-net-device.h)

# link to dependencies
set(libraries_to_link ${libnetwork})

set(test_sources)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")
