set(base_examples
    zigbee-nwk-direct-join
    zigbee-nwk-association-join
    zigbee-nwk-routing
    zigbee-nwk-routing-grid
)

foreach(
  example
  ${base_examples}
)
  build_lib_example(
    NAME ${example}
    SOURCE_FILES ${example}.cc
    LIBRARIES_TO_LINK ${libzigbee}
                      ${liblr-wpan}
  )
endforeach()
