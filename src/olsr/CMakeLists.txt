set(name olsr)

set(source_files helper/olsr-helper.cc model/olsr-header.cc model/olsr-routing-protocol.cc model/olsr-state.cc)

set(header_files helper/olsr-helper.h model/olsr-header.h model/olsr-repositories.h model/olsr-routing-protocol.h
                 model/olsr-state.h
)

set(libraries_to_link ${libinternet})

set(test_sources test/regression-test-suite.cc test/bug780-test.cc test/hello-regression-test.cc
                 test/olsr-header-test-suite.cc test/olsr-routing-protocol-test-suite.cc test/tc-regression-test.cc
)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")
