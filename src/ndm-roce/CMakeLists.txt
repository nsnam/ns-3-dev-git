# ndm-sys project module — Apache License 2.0 (see LICENSE-PROJECT).
#
# ndm-roce: RoCE/RDMA baseline transport (Phase 2, D10): RC queue pair with
# cumulative ACK / go-back-N retransmission / RTO, DCQCN rate-based
# congestion control, CNP generation, RoCEv2-style framing
# (IPv6 + UDP 4791 + simulator BTH) moved over the ndm-topology path
# forwarder. Concept donor: astra-network-ns3 (donors/, GPLv2).

build_lib(
  LIBNAME ndm-roce
  SOURCE_FILES
    model/ndm-roce-bth.cc
    model/ndm-roce-dcqcn.cc
    model/ndm-roce-qpair.cc
    model/ndm-roce-rx-qpair.cc
    model/ndm-roce-endpoint.cc
  HEADER_FILES
    model/ndm-roce-bth.h
    model/ndm-roce-dcqcn.h
    model/ndm-roce-qpair.h
    model/ndm-roce-rx-qpair.h
    model/ndm-roce-endpoint.h
  LIBRARIES_TO_LINK ${libcore} ${libnetwork} ${libpoint-to-point} ${libinternet} ${libndm-topology}
)
