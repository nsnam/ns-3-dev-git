set(name)

set(source_files)

set(header_files)

set(deprecated_header_files)

set(libraries_to_link
    ${libinternet}
)

set(test_sources
    test/nix-test.cc
)

build_lib(
  LIBNAME nix-vector-routing
  SOURCE_FILES helper/nix-vector-helper.cc
               model/nix-vector-routing.cc
  HEADER_FILES helper/nix-vector-helper.h
               model/nix-vector-routing.h
  DEPRECATED_HEADER_FILES helper/ipv4-nix-vector-helper.h
                          model/ipv4-nix-vector-routing.h
  LIBRARIES_TO_LINK ${libinternet}
  TEST_SOURCES test/nix-test.cc
)
