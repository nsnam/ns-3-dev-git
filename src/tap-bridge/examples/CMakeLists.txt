if(${NS3_TAP})
  set(name tap-csma)
  set(source_files ${name}.cc)
  set(header_files)
  set(libraries_to_link ${libcsma} ${libtap-bridge} ${libinternet} ${libwifi})
  build_lib_example(
    "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
  )

  set(name tap-csma-virtual-machine)
  set(source_files ${name}.cc)
  set(header_files)
  set(libraries_to_link ${libcsma} ${libtap-bridge} ${libinternet})
  build_lib_example(
    "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
  )

  set(name tap-wifi-virtual-machine)
  set(source_files ${name}.cc)
  set(header_files)
  set(libraries_to_link ${libcsma} ${libtap-bridge} ${libinternet} ${libwifi}
                        ${libmobility}
  )
  build_lib_example(
    "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
  )

  set(name tap-wifi-dumbbell)
  set(source_files ${name}.cc)
  set(header_files)
  set(libraries_to_link ${libwifi} ${libcsma} ${libpoint-to-point}
                        ${libtap-bridge} ${libinternet} ${libapplications}
  )
  build_lib_example(
    "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
  )

endif()
