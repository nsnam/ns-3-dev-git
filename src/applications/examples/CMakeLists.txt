set(name three-gpp-http-example)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libapplications} ${libpoint-to-point} ${libinternet}
                      ${libnetwork}
)
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)
