set(header_files)
set(libraries_to_link ${libpoint-to-point} ${libinternet} ${libapplications}
                      ${libnix-vector-routing}
)

set(nix_examples nix-simple nix-simple-multi-address nms-p2p-nix)

foreach(example ${nix_examples})
  set(source_files ${example}.cc)
  build_lib_example(
    "${example}" "${source_files}" "${header_files}" "${libraries_to_link}"
  )
endforeach()
