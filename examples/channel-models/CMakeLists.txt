build_example(
  NAME three-gpp-v2v-channel-example
  SOURCE_FILES three-gpp-v2v-channel-example.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libmobility}
    ${libpropagation}
    ${libspectrum}
    ${libantenna}
    ${libbuildings}
)

build_example(
  NAME three-gpp-leo-constellation
  SOURCE_FILES three-gpp-leo-constellation.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libmobility}
    ${libpropagation}
    ${libspectrum}
    ${libantenna}
)
