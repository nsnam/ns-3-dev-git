set(name topology-read)

set(source_files helper/topology-reader-helper.cc model/inet-topology-reader.cc model/orbis-topology-reader.cc
                 model/rocketfuel-topology-reader.cc model/topology-reader.cc
)

set(header_files helper/topology-reader-helper.h model/inet-topology-reader.h model/orbis-topology-reader.h
                 model/rocketfuel-topology-reader.h model/topology-reader.h
)

set(libraries_to_link ${libnetwork} ${PCRE_LIBRARIES})

set(test_sources test/rocketfuel-topology-reader-test-suite.cc)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")
