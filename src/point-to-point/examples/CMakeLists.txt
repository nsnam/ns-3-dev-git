set(name main-attribute-value)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libnetwork} ${libpoint-to-point})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)
