set(name virtual-net-device)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libvirtual-net-device} ${libpoint-to-point}
                      ${libinternet} ${libapplications}
)
build_lib_example(
  "${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
)
