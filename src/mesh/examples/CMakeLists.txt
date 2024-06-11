build_lib_example(
  NAME mesh-example
  SOURCE_FILES mesh-example.cc
  LIBRARIES_TO_LINK
    ${libinternet}
    ${libmobility}
    ${libwifi}
    ${libmesh}
    ${libapplications}
)
