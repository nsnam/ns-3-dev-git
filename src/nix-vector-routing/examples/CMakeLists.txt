set(base_examples
    nix-simple
    nix-simple-multi-address
    nms-p2p-nix
)

foreach(
  example
  ${base_examples}
)
  build_lib_example(
    NAME ${example}
    SOURCE_FILES ${example}.cc
    LIBRARIES_TO_LINK
      ${libpoint-to-point}
      ${libinternet}
      ${libapplications}
      ${libnix-vector-routing}
  )
endforeach()
