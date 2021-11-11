set(name aodv)

set(source_files
    helper/aodv-helper.cc
    model/aodv-dpd.cc
    model/aodv-id-cache.cc
    model/aodv-neighbor.cc
    model/aodv-packet.cc
    model/aodv-routing-protocol.cc
    model/aodv-rqueue.cc
    model/aodv-rtable.cc
)

set(header_files
    helper/aodv-helper.h
    model/aodv-dpd.h
    model/aodv-id-cache.h
    model/aodv-neighbor.h
    model/aodv-packet.h
    model/aodv-routing-protocol.h
    model/aodv-rqueue.h
    model/aodv-rtable.h
)

# link to dependencies
set(libraries_to_link ${libinternet} ${libwifi})

set(test_sources test/aodv-id-cache-test-suite.cc test/aodv-regression.cc test/aodv-test-suite.cc test/loopback.cc
                 test/bug-772.cc
)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")
