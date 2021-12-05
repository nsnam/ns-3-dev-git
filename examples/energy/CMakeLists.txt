set(name energy-model-example)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libmobility} ${libwifi} ${libenergy}
                      ${libinternet} ${libconfig-store}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name energy-model-with-harvesting-example)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libmobility} ${libwifi} ${libenergy}
                      ${libinternet} ${libconfig-store}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)
