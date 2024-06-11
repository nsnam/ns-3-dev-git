build_lib_example(
  NAME aodv-example
  SOURCE_FILES aodv-example.cc
  LIBRARIES_TO_LINK
    ${libwifi}
    ${libinternet}
    ${libaodv}
    ${libinternet-apps}
)
