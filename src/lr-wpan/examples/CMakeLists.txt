set(name lr-wpan-data)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblr-wpan})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lr-wpan-error-distance-plot)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblr-wpan} ${libstats})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lr-wpan-error-model-plot)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblr-wpan} ${libstats})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lr-wpan-packet-print)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblr-wpan})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lr-wpan-phy-test)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblr-wpan})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)
