include_directories(${MPI_CXX_INCLUDE_DIRS})
link_libraries(${MPI_CXX_LIBRARIES})

set(name simple-distributed)
set(source_files ${name}.cc mpi-test-fixtures.cc)
set(header_files)
set(libraries_to_link ${libmpi} ${libpoint-to-point} ${libinternet} ${libnix-vector-routing} ${libapplications})
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")

set(name third-distributed)
set(source_files ${name}.cc mpi-test-fixtures.cc)
set(header_files)
set(libraries_to_link
    ${libmpi}
    ${libpoint-to-point}
    ${libinternet}
    ${libmobility}
    ${libwifi}
    ${libcsma}
    ${libapplications}
)
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")

set(name nms-p2p-nix-distributed)
set(source_files ${name}.cc mpi-test-fixtures.cc)
set(header_files)
set(libraries_to_link ${libmpi} ${libpoint-to-point} ${libinternet} ${libnix-vector-routing} ${libapplications})
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")

set(name simple-distributed-empty-node)
set(source_files ${name}.cc mpi-test-fixtures.cc)
set(header_files)
set(libraries_to_link ${libmpi} ${libpoint-to-point} ${libinternet} ${libnix-vector-routing} ${libapplications})
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")

set(name simple-distributed-mpi-comm)
set(source_files ${name}.cc mpi-test-fixtures.cc)
set(header_files)
set(libraries_to_link ${libmpi} ${libpoint-to-point} ${libinternet} ${libnix-vector-routing} ${libapplications})
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")
