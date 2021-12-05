set(name example-sixlowpan)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libnetwork} ${libsixlowpan} ${libinternet} ${libcsma}
                      ${libinternet-apps}
)
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name example-ping-lr-wpan)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libnetwork} ${libsixlowpan} ${libinternet}
                      ${liblr-wpan} ${libinternet-apps}
)
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name example-ping-lr-wpan-beacon)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libnetwork} ${libsixlowpan} ${libinternet}
                      ${liblr-wpan} ${libinternet-apps}
)
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name example-ping-lr-wpan-mesh-under)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libnetwork} ${libsixlowpan} ${libinternet}
                      ${liblr-wpan} ${libinternet-apps} ${libcsma}
)
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)
