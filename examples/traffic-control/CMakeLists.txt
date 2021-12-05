set(name traffic-control)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libinternet} ${libpoint-to-point} ${libapplications}
                      ${libtraffic-control} ${libflow-monitor}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name queue-discs-benchmark)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link
    ${libinternet} ${libpoint-to-point} ${libapplications} ${libinternet-apps}
    ${libtraffic-control} ${libflow-monitor}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name red-vs-fengadaptive)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link
    ${libinternet} ${libpoint-to-point} ${libpoint-to-point-layout}
    ${libapplications} ${libtraffic-control}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name red-vs-nlred)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link
    ${libinternet} ${libpoint-to-point} ${libpoint-to-point-layout}
    ${libapplications} ${libtraffic-control}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name tbf-example)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libinternet} ${libpoint-to-point} ${libapplications}
                      ${libtraffic-control}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name cobalt-vs-codel)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libinternet} ${libpoint-to-point} ${libapplications}
                      ${libtraffic-control}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)
