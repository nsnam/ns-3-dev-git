# ndm-sys project module — Apache License 2.0 (see LICENSE-PROJECT).
#
# ndm-topology: explicit host/GPU/NIC/port/link/rail/plane identities,
# (link, port)-keyed adjacency, topology generators, path queries, and the
# path-forwarding test-traffic model used by the failure-framework gates.

build_lib(
  LIBNAME ndm-topology
  SOURCE_FILES
    model/ndm-link.cc
    model/ndm-point-to-point-channel.cc
    model/ndm-rx-guard.cc
    model/ndm-link-loss-model.cc
    model/ndm-topology.cc
    model/ndm-topology-helper.cc
    model/ndm-path-forwarder.cc
  HEADER_FILES
    model/ndm-identity.h
    model/ndm-link.h
    model/ndm-point-to-point-channel.h
    model/ndm-rx-guard.h
    model/ndm-link-loss-model.h
    model/ndm-topology.h
    model/ndm-topology-helper.h
    model/ndm-path-forwarder.h
  LIBRARIES_TO_LINK ${libcore} ${libnetwork} ${libpoint-to-point}
  TEST_SOURCES
    test/ndm-topology-test.cc
)
