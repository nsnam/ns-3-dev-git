set(name mesh)

set(source_files
    helper/dot11s/dot11s-installer.cc
    helper/flame/flame-installer.cc
    helper/mesh-helper.cc
    helper/mesh-stack-installer.cc
    model/dot11s/airtime-metric.cc
    model/dot11s/dot11s-mac-header.cc
    model/dot11s/hwmp-protocol-mac.cc
    model/dot11s/hwmp-protocol.cc
    model/dot11s/hwmp-rtable.cc
    model/dot11s/hwmp-tag.cc
    model/dot11s/ie-dot11s-beacon-timing.cc
    model/dot11s/ie-dot11s-configuration.cc
    model/dot11s/ie-dot11s-id.cc
    model/dot11s/ie-dot11s-metric-report.cc
    model/dot11s/ie-dot11s-peer-management.cc
    model/dot11s/ie-dot11s-peering-protocol.cc
    model/dot11s/ie-dot11s-perr.cc
    model/dot11s/ie-dot11s-prep.cc
    model/dot11s/ie-dot11s-preq.cc
    model/dot11s/ie-dot11s-rann.cc
    model/dot11s/peer-link-frame.cc
    model/dot11s/peer-link.cc
    model/dot11s/peer-management-protocol-mac.cc
    model/dot11s/peer-management-protocol.cc
    model/flame/flame-header.cc
    model/flame/flame-protocol-mac.cc
    model/flame/flame-protocol.cc
    model/flame/flame-rtable.cc
    model/mesh-information-element-vector.cc
    model/mesh-l2-routing-protocol.cc
    model/mesh-point-device.cc
    model/mesh-wifi-beacon.cc
    model/mesh-wifi-interface-mac.cc
)

set(header_files
    helper/dot11s/dot11s-installer.h
    helper/flame/flame-installer.h
    helper/mesh-helper.h
    helper/mesh-stack-installer.h
    model/dot11s/dot11s-mac-header.h
    model/dot11s/hwmp-protocol.h
    model/dot11s/hwmp-rtable.h
    model/dot11s/ie-dot11s-beacon-timing.h
    model/dot11s/ie-dot11s-configuration.h
    model/dot11s/ie-dot11s-id.h
    model/dot11s/ie-dot11s-metric-report.h
    model/dot11s/ie-dot11s-peer-management.h
    model/dot11s/ie-dot11s-peering-protocol.h
    model/dot11s/ie-dot11s-perr.h
    model/dot11s/ie-dot11s-prep.h
    model/dot11s/ie-dot11s-preq.h
    model/dot11s/ie-dot11s-rann.h
    model/dot11s/peer-link-frame.h
    model/dot11s/peer-link.h
    model/dot11s/peer-management-protocol.h
    model/flame/flame-header.h
    model/flame/flame-protocol-mac.h
    model/flame/flame-protocol.h
    model/flame/flame-rtable.h
    model/mesh-information-element-vector.h
    model/mesh-l2-routing-protocol.h
    model/mesh-point-device.h
    model/mesh-wifi-beacon.h
    model/mesh-wifi-interface-mac-plugin.h
    model/mesh-wifi-interface-mac.h
)

set(libraries_to_link ${libinternet} ${libwifi} ${libapplications})

set(test_sources
    test/dot11s/dot11s-test-suite.cc
    test/dot11s/hwmp-proactive-regression.cc
    test/dot11s/hwmp-reactive-regression.cc
    test/dot11s/hwmp-simplest-regression.cc
    test/dot11s/hwmp-target-flags-regression.cc
    test/dot11s/pmp-regression.cc
    test/dot11s/regression.cc
    test/flame/flame-regression.cc
    test/flame/flame-test-suite.cc
    test/flame/regression.cc
    test/mesh-information-element-vector-test-suite.cc
)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")
