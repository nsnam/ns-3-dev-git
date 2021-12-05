set(name main-callback)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name sample-simulator)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name main-ptr)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name main-random-variable-stream)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libconfig-store} ${libstats})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name sample-random-variable)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name sample-random-variable-stream)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name command-line-example)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name fatal-example)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name hash-example)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name sample-log-time-format)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name test-string-value-formatting)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name sample-show-progress)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

if(${NS3_ENABLE_BUILD_VERSION})
  set(name build-version-example)
  set(source_files ${name}.cc)
  set(header_files)
  set(libraries_to_link ${libcore})
  build_lib_example(
    "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
  )
endif()

if(${ENABLE_REALTIME})
  set(name main-test-sync)
  set(source_files ${name}.cc)
  set(header_files)
  set(libraries_to_link ${libnetwork})
  build_lib_example(
    "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
  )
endif()

set(name length-example)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name empirical-random-variable-example)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libflow-monitor})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name system-path-examples)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)
