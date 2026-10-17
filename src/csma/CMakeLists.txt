# csma-system-test-suite.cc exercises CSMA over an IPv4/IPv6 internet stack with
# bridged star topologies, so it is built only when applications, csma-layout
# and internet-apps are also enabled.
set(csma_system_test_sources)
set(csma_system_test_libs)
if((applications
    IN_LIST
    ns3-all-enabled-modules
   )
   AND (csma-layout
        IN_LIST
        ns3-all-enabled-modules
       )
   AND (internet-apps
        IN_LIST
        ns3-all-enabled-modules
       )
)
  set(csma_system_test_sources
      test/csma-system-test-suite.cc
  )
  set(csma_system_test_libs
      ${libapplications}
      ${libcsma-layout}
      ${libinternet}
      ${libinternet-apps}
  )
endif()

build_lib(
  LIBNAME csma
  SOURCE_FILES
    helper/csma-helper.cc
    model/backoff.cc
    model/csma-channel.cc
    model/csma-net-device.cc
  HEADER_FILES
    helper/csma-helper.h
    model/backoff.h
    model/csma-channel.h
    model/csma-net-device.h
  LIBRARIES_TO_LINK ${libnetwork}
  TEST_SOURCES test/csma-link-change-test-suite.cc
               ${csma_system_test_sources}
  TEST_LIBRARIES_TO_LINK ${csma_system_test_libs}
)
