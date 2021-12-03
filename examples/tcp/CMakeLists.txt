set(name tcp-large-transfer)
set(source_files ${name}.cc)
set(libraries_to_link ${libpoint-to-point} ${libapplications} ${libinternet})
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name tcp-star-server)
set(source_files ${name}.cc)
set(libraries_to_link ${libpoint-to-point} ${libapplications} ${libinternet})
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name star)
set(source_files ${name}.cc)
set(libraries_to_link
    ${libnetanim} ${libpoint-to-point} ${libpoint-to-point-layout}
    ${libapplications} ${libinternet}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name tcp-bbr-example)
set(source_files ${name}.cc)
set(libraries_to_link
    ${libpoint-to-point}
    ${libinternet}
    ${libapplications}
    ${libtraffic-control}
    ${libnetwork}
    ${libinternet-apps}
    ${libflow-monitor}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name tcp-bulk-send)
set(source_files ${name}.cc)
set(libraries_to_link ${libpoint-to-point} ${libapplications} ${libinternet})
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name tcp-pcap-nanosec-example)
set(source_files ${name}.cc)
set(libraries_to_link ${libpoint-to-point} ${libapplications} ${libinternet})
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name tcp-variants-comparison)
set(source_files ${name}.cc)
set(libraries_to_link ${libpoint-to-point} ${libinternet} ${libapplications}
                      ${libflow-monitor}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name tcp-pacing)
set(source_files ${name}.cc)
set(libraries_to_link ${libpoint-to-point} ${libinternet} ${libapplications}
                      ${libflow-monitor}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name tcp-linux-reno)
set(source_files ${name}.cc)
set(libraries_to_link ${libpoint-to-point} ${libinternet} ${libapplications}
                      ${libtraffic-control} ${libnetwork}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name tcp-validation)
set(source_files ${name}.cc)
set(libraries_to_link ${libpoint-to-point} ${libinternet} ${libapplications}
                      ${libtraffic-control} ${libnetwork} ${libinternet-apps}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name dctcp-example)
set(source_files ${name}.cc)
set(libraries_to_link
    ${libcore} ${libnetwork} ${libinternet} ${libpoint-to-point}
    ${libapplications} ${libtraffic-control}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)
