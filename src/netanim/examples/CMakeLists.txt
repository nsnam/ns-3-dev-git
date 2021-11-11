set(name dumbbell-animation)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libnetanim} ${libapplications} ${libpoint-to-point-layout})
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")

set(name grid-animation)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libnetanim} ${libapplications} ${libpoint-to-point-layout})
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")

set(name star-animation)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libnetanim} ${libapplications} ${libpoint-to-point-layout})
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")

set(name wireless-animation)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link
    ${libnetanim}
    ${libapplications}
    ${libpoint-to-point}
    ${libcsma}
    ${libwifi}
    ${libmobility}
    ${libnetwork}
)
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")

set(name uan-animation)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libnetanim} ${libinternet} ${libmobility} ${libapplications} ${libuan})
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")

set(name colors-link-description)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libnetanim} ${libapplications} ${libpoint-to-point-layout})
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")

set(name resources-counters)
set(source_files ${name}.cc)
set(header_files)
set(libraries_to_link ${libnetanim} ${libapplications} ${libpoint-to-point-layout})
build_lib_example("${name}" "${source_files}" "${header_files}" "${libraries_to_link}")
