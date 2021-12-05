set(name main-propagation-loss)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libpropagation} ${libconfig-store}
                      ${libstats} ${libmobility} ${libbuildings}
)
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name jakes-propagation-model-example)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libpropagation} ${libbuildings})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)
