set(name lena-cqi-threshold)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblte})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lena-dual-stripe)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblte})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lena-fading)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblte})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lena-intercell-interference)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblte})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lena-pathloss-traces)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblte})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lena-profiling)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblte})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lena-rem)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblte})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lena-rem-sector-antenna)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblte})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lena-rlc-traces)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblte})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lena-simple)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblte})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lena-simple-epc)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblte})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lena-deactivate-bearer)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblte})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lena-x2-handover)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblte})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lena-x2-handover-measures)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblte})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lena-frequency-reuse)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblte})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lena-distributed-ffr)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblte})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lena-uplink-power-control)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblte})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lena-ipv6-addr-conf)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblte})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lena-ipv6-ue-rh)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblte})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lena-ipv6-ue-ue)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblte})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name lena-radio-link-failure)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${liblte})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

if(${NS3_EMU})
  set(name lena-simple-epc-emu)
  set(source_files ${name}.cc)
  set(header_files)
  set(libraries_to_link ${liblte} ${fd-net-device})
  build_lib_example(
    "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
  )
endif()
