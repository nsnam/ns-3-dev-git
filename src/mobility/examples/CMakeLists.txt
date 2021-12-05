set(name bonnmotion-ns2-example)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libmobility})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name main-grid-topology)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libmobility} ${libnetwork})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name main-random-topology)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libmobility})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name main-random-walk)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libmobility})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name mobility-trace-example)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libmobility} ${libnetwork})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name ns2-mobility-trace)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libmobility})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name reference-point-group-mobility-example)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libnetwork} ${libmobility})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)
