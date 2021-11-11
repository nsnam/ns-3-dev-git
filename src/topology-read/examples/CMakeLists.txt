if(PCRE_FOUND)
  set(name topology-example-sim)
  set(source_files ${name}.cc)
  set(header_files)
  set(libraries_to_link ${libtopology-read} ${libinternet} ${libnix-vector-routing} ${libpoint-to-point}
                        ${libapplications}
  )
  build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")
endif()
