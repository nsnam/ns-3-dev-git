set(name brite-generic-example)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libbrite} ${libinternet} ${libpoint-to-point}
                      ${libnix-vector-routing} ${libapplications}
)
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

if(${ENABLE_MPI})
  set(name brite-MPI-example)
  set(source_files ${name}.cc)
  set(header_files)
  set(libraries_to_link
      ${libbrite}
      ${libinternet}
      ${libpoint-to-point}
      ${libnix-vector-routing}
      ${libapplications}
      ${libmpi}
      ${MPI_CXX_LIBRARIES}
  )
  build_lib_example(
    "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
  )
  target_include_directories(${name} PUBLIC ${MPI_CXX_INCLUDE_DIRS})
endif()
