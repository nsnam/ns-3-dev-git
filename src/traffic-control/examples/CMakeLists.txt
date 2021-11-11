set(name red-tests)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libpoint-to-point} ${libinternet} ${libapplications} ${libflow-monitor} ${libtraffic-control})
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")

set(name red-vs-ared)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libpoint-to-point} ${libpoint-to-point-layout} ${libinternet} ${libapplications}
                      ${libtraffic-control}
)
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")

set(name adaptive-red-tests)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libpoint-to-point} ${libinternet} ${libapplications} ${libflow-monitor} ${libtraffic-control})
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")

set(name pfifo-vs-red)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libpoint-to-point} ${libpoint-to-point-layout} ${libinternet} ${libapplications}
                      ${libtraffic-control}
)
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")

set(name codel-vs-pfifo-basic-test)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libpoint-to-point} ${libnetwork} ${libinternet} ${libapplications} ${libtraffic-control})
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")

set(name codel-vs-pfifo-asymmetric)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libpoint-to-point} ${libnetwork} ${libinternet} ${libapplications} ${libtraffic-control})
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")

set(name pie-example)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libpoint-to-point} ${libinternet} ${libapplications} ${libflow-monitor} ${libtraffic-control})
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")

set(name fqcodel-l4s-example)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libpoint-to-point} ${libinternet} ${libinternet-apps} ${libapplications} ${libflow-monitor}
                      ${libtraffic-control}
)
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")
