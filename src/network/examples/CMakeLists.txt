set(header_files)

set(name bit-serializer)
set(source_files ${name}.cc)
set(libraries_to_link ${libcore} ${libnetwork})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name main-packet-header)
set(source_files ${name}.cc)
set(libraries_to_link ${libnetwork})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name main-packet-tag)
set(source_files ${name}.cc)
set(libraries_to_link ${libnetwork})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name packet-socket-apps)
set(source_files ${name}.cc)
set(libraries_to_link ${libnetwork} ${libcore})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lollipop-comparisions)
set(source_files ${name}.cc)
set(libraries_to_link ${libnetwork} ${libcore})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)
