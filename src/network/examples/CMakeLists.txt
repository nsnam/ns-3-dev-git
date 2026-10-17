set(base_examples
    bit-serializer
    lollipop-comparisons
    main-packet-header
    main-packet-tag
    packet-socket-apps
    sequence-number-example
)

foreach(
  example
  ${base_examples}
)
  build_lib_example(
    NAME ${example}
    SOURCE_FILES ${example}.cc
    LIBRARIES_TO_LINK ${libnetwork}
                      ${libcore}
  )
endforeach()
