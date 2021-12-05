set(name dynamic-global-routing)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libpoint-to-point} ${libcsma} ${libinternet}
                      ${libapplications}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name static-routing-slash32)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libpoint-to-point} ${libcsma} ${libinternet}
                      ${libapplications}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name global-routing-slash32)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libpoint-to-point} ${libcsma} ${libinternet}
                      ${libapplications}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name global-injection-slash32)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libpoint-to-point} ${libcsma} ${libinternet}
                      ${libapplications}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name simple-global-routing)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libpoint-to-point} ${libinternet} ${libapplications}
                      ${libflow-monitor}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name simple-alternate-routing)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libpoint-to-point} ${libinternet} ${libapplications})
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name mixed-global-routing)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libpoint-to-point} ${libinternet} ${libcsma}
                      ${libapplications}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name simple-routing-ping6)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcsma} ${libinternet} ${libinternet-apps})
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name manet-routing-compare)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link
    ${libwifi}
    ${libdsr}
    ${libdsdv}
    ${libaodv}
    ${libolsr}
    ${libinternet}
    ${libapplications}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name ripng-simple-network)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcsma} ${libinternet} ${libinternet-apps})
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name rip-simple-network)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcsma} ${libinternet} ${libinternet-apps})
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name global-routing-multi-switch-plus-router)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link
    ${libcore}
    ${libnetwork}
    ${libapplications}
    ${libinternet}
    ${libbridge}
    ${libcsma}
    ${libpoint-to-point}
    ${libcsma}
    ${libinternet}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)

set(name simple-multicast-flooding)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libnetwork} ${libapplications}
                      ${libinternet}
)
build_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)
