set(name csma-bridge)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libbridge} ${libcsma} ${libinternet} ${libapplications})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name csma-bridge-one-hop)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libbridge} ${libcsma} ${libinternet} ${libapplications})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)
