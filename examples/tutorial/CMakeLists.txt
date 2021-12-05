set(name hello-simulator)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore})
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name first)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libpoint-to-point} ${libinternet}
                      ${libapplications}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name second)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libpoint-to-point} ${libcsma} ${libinternet}
                      ${libapplications}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name third)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libpoint-to-point} ${libcsma} ${libwifi}
                      ${libinternet} ${libapplications}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name fourth)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore})
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name fifth)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libpoint-to-point} ${libinternet}
                      ${libapplications}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name sixth)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libpoint-to-point} ${libinternet}
                      ${libapplications}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name seventh)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libstats} ${libpoint-to-point}
                      ${libinternet} ${libapplications}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)
