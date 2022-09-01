build_lib_example(
  NAME main-simple
  SOURCE_FILES main-simple.cc
  LIBRARIES_TO_LINK
    ${libnetwork}
    ${libinternet}
    ${libapplications}
)

build_lib_example(
  NAME neighbor-cache-example
  SOURCE_FILES neighbor-cache-example.cc
  LIBRARIES_TO_LINK
    ${libapplications}
    ${libcsma}
    ${libinternet}
    ${libnetwork}
)
