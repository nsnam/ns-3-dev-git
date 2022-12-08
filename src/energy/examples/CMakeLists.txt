build_lib_example(
  NAME generic-battery-discharge-example
  SOURCE_FILES generic-battery-discharge-example.cc
  LIBRARIES_TO_LINK ${libcore}
                    ${libenergy}
)

build_example(
  NAME generic-battery-wifiradio-example
  SOURCE_FILES generic-battery-wifiradio-example.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libconfig-store}
    ${libenergy}
    ${libinternet}
    ${libmobility}
    ${libwifi}
)

build_lib_example(
  NAME li-ion-energy-source-example
  SOURCE_FILES li-ion-energy-source-example.cc
  LIBRARIES_TO_LINK ${libcore}
                    ${libenergy}
)

build_lib_example(
  NAME rv-battery-model-test
  SOURCE_FILES rv-battery-model-test.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libenergy}
    ${libwifi}
)

build_lib_example(
  NAME basic-energy-model-test
  SOURCE_FILES basic-energy-model-test.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libenergy}
    ${libwifi}
)
