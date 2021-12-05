set(name dsr)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link
    ${libcore}
    ${libnetwork}
    ${libinternet}
    ${libapplications}
    ${libmobility}
    ${libconfig-store}
    ${libwifi}
    ${libdsr}
)
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)
