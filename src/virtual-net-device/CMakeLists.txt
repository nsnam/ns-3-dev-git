set(name virtual-net-device)

set(source_files model/virtual-net-device.cc)

set(header_files model/virtual-net-device.h)

set(libraries_to_link ${libnetwork})

set(test_sources)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")
