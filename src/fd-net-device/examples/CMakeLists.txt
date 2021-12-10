include_directories(${DPDK_INCLUDE_DIRS})

set(name dummy-network)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libfd-net-device} ${libinternet} ${libinternet-apps})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name fd2fd-onoff)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libfd-net-device} ${libinternet} ${libapplications})
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

if(${ENABLE_REALTIME})
  set(name realtime-dummy-network)
  set(source_files ${name}.cc)
  set(header_files)
  set(libraries_to_link ${libfd-net-device} ${libinternet} ${libinternet-apps})
  build_lib_example(
    "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
  )

  set(name realtime-fd2fd-onoff)
  set(source_files ${name}.cc)
  set(header_files)
  set(libraries_to_link ${libfd-net-device} ${libinternet} ${libapplications})
  build_lib_example(
    "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
  )
endif()

if(${ENABLE_EMU})
  set(name fd-emu-ping)
  set(source_files ${name}.cc)
  set(header_files)
  set(libraries_to_link ${libfd-net-device} ${libinternet} ${libinternet-apps})
  build_lib_example(
    "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
  )

  set(name fd-emu-udp-echo)
  set(source_files ${name}.cc)
  set(header_files)
  set(libraries_to_link ${libfd-net-device} ${libinternet} ${libapplications})
  build_lib_example(
    "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
  )

  set(name fd-emu-onoff)
  set(source_files ${name}.cc)
  set(header_files)
  set(libraries_to_link ${libfd-net-device} ${libinternet} ${libapplications})
  build_lib_example(
    "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
  )

  set(name fd-emu-send)
  set(source_files ${name}.cc)
  set(header_files)
  set(libraries_to_link ${libfd-net-device} ${libinternet} ${libapplications}
                        ${libinternet-apps} ${libtraffic-control}
  )
  build_lib_example(
    "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
  )

  set(name fd-emu-tc)
  set(source_files ${name}.cc)
  set(header_files)
  set(libraries_to_link ${libfd-net-device} ${libinternet} ${libapplications}
                        ${libinternet-apps} ${libtraffic-control}
  )
  build_lib_example(
    "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
  )
endif()

if(${ENABLE_TAP})
  set(name fd-tap-ping)
  set(source_files ${name}.cc)
  set(header_files)
  set(libraries_to_link ${libfd-net-device} ${libinternet} ${libinternet-apps})
  build_lib_example(
    "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
  )

  set(name fd-tap-ping6)
  set(source_files ${name}.cc)
  set(header_files)
  set(libraries_to_link ${libfd-net-device} ${libinternet} ${libinternet-apps}
                        ${libcsma}
  )
  build_lib_example(
    "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
  )
endif()
