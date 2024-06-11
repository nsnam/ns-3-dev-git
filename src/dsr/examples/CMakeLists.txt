build_lib_example(
  NAME dsr-example
  SOURCE_FILES dsr-example.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libinternet}
    ${libapplications}
    ${libmobility}
    ${libconfig-store}
    ${libwifi}
    ${libdsr}
)
