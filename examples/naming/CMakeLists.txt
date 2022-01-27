set(name
    object-names
)

set(libraries_to_link)
build_example(
  NAME object-names
  SOURCE_FILES object-names.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libcsma}
    ${libinternet}
    ${libapplications}
)
