set(base_examples
    lr-wpan-data
    lr-wpan-beacon-mode
    lr-wpan-packet-print
    lr-wpan-phy-test
    lr-wpan-ed-scan
    lr-wpan-active-scan
    lr-wpan-orphan-scan
)

foreach(
  example
  ${base_examples}
)
  build_lib_example(
    NAME ${example}
    SOURCE_FILES ${example}.cc
    LIBRARIES_TO_LINK ${liblr-wpan}
  )
endforeach()

build_lib_example(
  NAME lr-wpan-error-distance-plot
  SOURCE_FILES lr-wpan-error-distance-plot.cc
  LIBRARIES_TO_LINK ${liblr-wpan}
                    ${libstats}
)

build_lib_example(
  NAME lr-wpan-error-model-plot
  SOURCE_FILES lr-wpan-error-model-plot.cc
  LIBRARIES_TO_LINK ${liblr-wpan}
                    ${libstats}
)

build_lib_example(
  NAME lr-wpan-bootstrap
  SOURCE_FILES lr-wpan-bootstrap.cc
  LIBRARIES_TO_LINK ${liblr-wpan}
                    ${libnetanim}
)

build_lib_example(
  NAME lr-wpan-per-plot
  SOURCE_FILES lr-wpan-per-plot.cc
  LIBRARIES_TO_LINK ${liblr-wpan}
                    ${libstats}
)
