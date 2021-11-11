set(name flow-monitor)

set(source_files
    helper/flow-monitor-helper.cc
    model/flow-classifier.cc
    model/flow-monitor.cc
    model/flow-probe.cc
    model/ipv4-flow-classifier.cc
    model/ipv4-flow-probe.cc
    model/ipv6-flow-classifier.cc
    model/ipv6-flow-probe.cc
)

set(header_files
    helper/flow-monitor-helper.h
    model/flow-classifier.h
    model/flow-monitor.h
    model/flow-probe.h
    model/ipv4-flow-classifier.h
    model/ipv4-flow-probe.h
    model/ipv6-flow-classifier.h
    model/ipv6-flow-probe.h
)

set(libraries_to_link ${libinternet} ${libconfig-store})

set(test_sources)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")
