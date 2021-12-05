set(name netanim)

set(source_files model/animation-interface.cc)

set(header_files model/animation-interface.h)

set(libraries_to_link
    ${libinternet}
    ${libmobility}
    ${libwimax}
    ${libwifi}
    ${libcsma}
    ${liblte}
    ${libuan}
    ${libenergy}
    ${liblr-wpan}
    ${libwave}
    ${libpoint-to-point-layout}
)

set(test_sources test/netanim-test.cc)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
          "${test_sources}"
)
