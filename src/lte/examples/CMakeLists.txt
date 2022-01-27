set(base_examples
    lena-cqi-threshold
    lena-dual-stripe
    lena-fading
    lena-intercell-interference
    lena-pathloss-traces
    lena-profiling
    lena-rem
    lena-rem-sector-antenna
    lena-rlc-traces
    lena-simple
    lena-simple-epc
    lena-deactivate-bearer
    lena-x2-handover
    lena-x2-handover-measures
    lena-frequency-reuse
    lena-distributed-ffr
    lena-uplink-power-control
    lena-ipv6-addr-conf
    lena-ipv6-ue-rh
    lena-ipv6-ue-ue
    lena-radio-link-failure
)

foreach(
  example
  ${base_examples}
)
  build_lib_example(
    NAME ${example}
    SOURCE_FILES ${example}.cc
    LIBRARIES_TO_LINK ${liblte}
  )
endforeach()

if(${ENABLE_EMU})
  build_lib_example(
    NAME lena-simple-epc-emu
    SOURCE_FILES lena-simple-epc-emu.cc
    LIBRARIES_TO_LINK ${liblte}
                      ${fd-net-device}
  )
endif()
