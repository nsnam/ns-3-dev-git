build_lib_example(
  NAME virtual-net-device-example
  SOURCE_FILES virtual-net-device-example.cc
  LIBRARIES_TO_LINK
    ${libvirtual-net-device}
    ${libpoint-to-point}
    ${libinternet}
    ${libapplications}
)
