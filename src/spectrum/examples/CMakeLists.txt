build_lib_example(
  NAME adhoc-aloha-ideal-phy
  SOURCE_FILES adhoc-aloha-ideal-phy.cc
  LIBRARIES_TO_LINK
    ${libapplications}
    ${libinternet}
    ${libmobility}
    ${libspectrum}
)

build_lib_example(
  NAME adhoc-aloha-ideal-phy-matrix-propagation-loss-model
  SOURCE_FILES adhoc-aloha-ideal-phy-matrix-propagation-loss-model.cc
  LIBRARIES_TO_LINK
    ${libapplications}
    ${libinternet}
    ${libmobility}
    ${libspectrum}
)

build_lib_example(
  NAME adhoc-aloha-ideal-phy-with-microwave-oven
  SOURCE_FILES adhoc-aloha-ideal-phy-with-microwave-oven.cc
  LIBRARIES_TO_LINK
    ${libapplications}
    ${libinternet}
    ${libmobility}
    ${libspectrum}
)

build_lib_example(
  NAME tv-trans-example
  SOURCE_FILES tv-trans-example.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libmobility}
    ${libspectrum}
)

build_lib_example(
  NAME tv-trans-regional-example
  SOURCE_FILES tv-trans-regional-example.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libmobility}
    ${libspectrum}
)

build_lib_example(
  NAME three-gpp-channel-example
  SOURCE_FILES three-gpp-channel-example.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${liblte}
    ${libmobility}
    ${libspectrum}
)

build_lib_example(
  NAME three-gpp-two-ray-channel-calibration
  SOURCE_FILES three-gpp-two-ray-channel-calibration.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${liblte}
    ${libmobility}
    ${libspectrum}
)
