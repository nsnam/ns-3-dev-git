set(name csma-one-subnet)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcsma} ${libinternet} ${libapplications})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name csma-broadcast)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcsma} ${libinternet} ${libapplications})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name csma-packet-socket)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcsma} ${libinternet} ${libapplications})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name csma-multicast)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcsma} ${libinternet} ${libapplications})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name csma-raw-ip-socket)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcsma} ${libinternet} ${libapplications}
                      ${libinternet-apps}
)
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name csma-ping)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcsma} ${libinternet} ${libapplications}
                      ${libinternet-apps}
)
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)
