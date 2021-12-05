set(name nix-vector-routing)

set(source_files helper/nix-vector-helper.cc model/nix-vector-routing.cc)

set(header_files helper/nix-vector-helper.h model/nix-vector-routing.h)

set(deprecated_header_files helper/ipv4-nix-vector-helper.h
                            model/ipv4-nix-vector-routing.h
)

set(libraries_to_link ${libinternet})

set(test_sources test/nix-test.cc)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
          "${test_sources}"
)
