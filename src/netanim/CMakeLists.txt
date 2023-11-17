build_lib(
  LIBNAME netanim
  SOURCE_FILES model/animation-interface.cc
  HEADER_FILES model/animation-interface.h
  LIBRARIES_TO_LINK
    ${libwimax}
    ${libwifi}
    ${liblte}
    ${libuan}
    ${liblr-wpan}
  TEST_SOURCES test/netanim-test.cc
)
