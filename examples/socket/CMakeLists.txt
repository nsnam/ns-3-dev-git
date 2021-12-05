set(name socket-bound-static-routing)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libnetwork} ${libcsma} ${libpoint-to-point}
                      ${libinternet}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name socket-bound-tcp-static-routing)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libnetwork} ${libcsma} ${libpoint-to-point}
                      ${libinternet} ${libapplications}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name socket-options-ipv4)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libnetwork} ${libcsma} ${libpoint-to-point}
                      ${libinternet}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name socket-options-ipv6)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libnetwork} ${libcsma} ${libpoint-to-point}
                      ${libinternet}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)
