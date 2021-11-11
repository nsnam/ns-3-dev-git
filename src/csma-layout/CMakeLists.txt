set(name csma-layout)

set(source_files model/csma-star-helper.cc)

set(header_files model/csma-star-helper.h)

set(libraries_to_link ${libnetwork} ${libinternet} ${libcsma} ${libpoint-to-point})

set(test_sources)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")
