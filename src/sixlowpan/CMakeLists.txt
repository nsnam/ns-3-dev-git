set(name sixlowpan)

set(source_files helper/sixlowpan-helper.cc model/sixlowpan-header.cc model/sixlowpan-net-device.cc)

set(header_files helper/sixlowpan-helper.h model/sixlowpan-header.h model/sixlowpan-net-device.h)

set(libraries_to_link ${libinternet} ${libinternet} ${libcore})

set(example_as_test_suite)
if(${NS3_EXAMPLES})
  set(example_as_test_suite test/sixlowpan-examples-test-suite.cc)
endif()

set(test_sources ${example_as_test_suite} test/mock-net-device.cc test/sixlowpan-fragmentation-test.cc
                 test/sixlowpan-hc1-test.cc test/sixlowpan-iphc-stateful-test.cc test/sixlowpan-iphc-test.cc
)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")
