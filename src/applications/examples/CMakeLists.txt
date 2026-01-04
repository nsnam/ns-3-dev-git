build_lib_example(
  NAME three-gpp-http-example
  SOURCE_FILES three-gpp-http-example.cc
  LIBRARIES_TO_LINK
    ${libapplications}
    ${libpoint-to-point}
    ${libinternet}
    ${libnetwork}
)

build_lib_example(
  NAME tgax-video-example
  SOURCE_FILES tgax-video-example.cc
  LIBRARIES_TO_LINK
    ${libapplications}
    ${libwifi}
    ${libmobility}
    ${libinternet}
    ${libnetwork}
)

build_lib_example(
  NAME tgax-voip-example
  SOURCE_FILES tgax-voip-example.cc
  LIBRARIES_TO_LINK
    ${libapplications}
    ${libwifi}
    ${libmobility}
    ${libinternet}
    ${libnetwork}
)

build_lib_example(
  NAME tgax-virtual-desktop-example
  SOURCE_FILES tgax-virtual-desktop-example.cc
  LIBRARIES_TO_LINK
    ${libapplications}
    ${libwifi}
    ${libmobility}
    ${libinternet}
    ${libnetwork}
)

build_lib_example(
  NAME rta-tig-mobile-gaming-example
  SOURCE_FILES rta-tig-mobile-gaming-example.cc
  LIBRARIES_TO_LINK
    ${libapplications}
    ${libwifi}
    ${libmobility}
    ${libinternet}
    ${libnetwork}
)
