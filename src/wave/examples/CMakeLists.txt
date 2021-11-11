set(name wave-simple-80211p)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libapplications} ${libmobility} ${libnetwork} ${libwifi} ${libwave})
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")

set(name wave-simple-device)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libcore} ${libapplications} ${libmobility} ${libnetwork} ${libwifi} ${libwave})
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")

set(name vanet-routing-compare)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link
    ${libcore}
    ${libaodv}
    ${libapplications}
    ${libdsr}
    ${libdsdv}
    ${libflow-monitor}
    ${libmobility}
    ${libnetwork}
    ${libolsr}
    ${libpropagation}
    ${libwifi}
    ${libwave}
)
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")
