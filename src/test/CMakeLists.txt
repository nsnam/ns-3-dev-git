set(name test)

set(nsc_sources)
if(${NS3_NSC})
  set(nsc_sources ns3tcp/ns3tcp-cwnd-test-suite.cc ns3tcp/ns3tcp-interop-test-suite.cc ns3tcp/nsctcp-loss-test-suite.cc)
endif()

set(csma-layout_sources)
if(csma-layout IN_LIST ns3-all-modules)
  set(csma-layout_sources csma-system-test-suite.cc)
endif()

set(dsr_sources)
if(dsr IN_LIST ns3-all-modules)
  set(dsr_sources traced/traced-callback-typedef-test-suite.cc)
endif()

set(application_sources)
set(wifi_sources)
if(applications IN_LIST ns3-all-modules)
  if(point-to-point IN_LIST ns3-all-modules)
    set(applications_sources
        ns3tcp/ns3tcp-cwnd-test-suite.cc
        ns3tcp/ns3tcp-interop-test-suite.cc
        ns3tcp/ns3tcp-loss-test-suite.cc
        ns3tcp/ns3tcp-no-delay-test-suite.cc
        ns3tcp/ns3tcp-socket-test-suite.cc
        ns3tcp/ns3tcp-state-test-suite.cc
        ns3tcp/nsctcp-loss-test-suite.cc
    )
  endif()
  if(wifi IN_LIST ns3-all-modules)
    set(wifi_sources ns3wifi/wifi-issue-211-test-suite.cc ns3wifi/wifi-ac-mapping-test-suite.cc
                     ns3wifi/wifi-msdu-aggregator-test-suite.cc
    )
  endif()
endif()

set(internet_sources)
if(internet IN_LIST ns3-all-modules)
  set(internet_sources ns3tcp/ns3tcp-socket-writer.cc)

endif()

set(traffic-control_sources)
if(traffic-control IN_LIST ns3-all-modules)
  set(traffic-control_sources ns3tc/fq-cobalt-queue-disc-test-suite.cc ns3tc/fq-codel-queue-disc-test-suite.cc
                              ns3tc/fq-pie-queue-disc-test-suite.cc ns3tc/pfifo-fast-queue-disc-test-suite.cc
  )
endif()

add_library(
  ${lib${name}} OBJECT
  ${applications_sources}
  ${csma-layout_sources}
  ${dsr_sources}
  ${internet_sources}
  ${nsc_sources}
  ${traffic-control_sources}
  traced/traced-value-callback-typedef-test-suite.cc
  ${wifi_sources}
)

add_dependencies(${libtest} copy_all_headers)
