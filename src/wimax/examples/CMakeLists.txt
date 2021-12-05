set(name wimax-ipv4)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libwimax} ${libinternet} ${libmobility} ${libcsma}
                      ${libapplications}
)
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name wimax-multicast)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libwimax} ${libinternet} ${libcsma} ${libapplications})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name wimax-simple)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libwimax} ${libinternet} ${libmobility}
                      ${libapplications}
)
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)
