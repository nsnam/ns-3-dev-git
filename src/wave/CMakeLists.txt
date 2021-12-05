set(name wave)

set(source_files
    helper/wave-bsm-helper.cc
    helper/wave-bsm-stats.cc
    helper/wave-helper.cc
    helper/wave-mac-helper.cc
    helper/wifi-80211p-helper.cc
    model/bsm-application.cc
    model/channel-coordinator.cc
    model/channel-manager.cc
    model/channel-scheduler.cc
    model/default-channel-scheduler.cc
    model/higher-tx-tag.cc
    model/ocb-wifi-mac.cc
    model/vendor-specific-action.cc
    model/vsa-manager.cc
    model/wave-frame-exchange-manager.cc
    model/wave-net-device.cc
)

set(header_files
    helper/wave-bsm-helper.h
    helper/wave-bsm-stats.h
    helper/wave-helper.h
    helper/wave-mac-helper.h
    helper/wifi-80211p-helper.h
    model/bsm-application.h
    model/channel-coordinator.h
    model/channel-manager.h
    model/channel-scheduler.h
    model/default-channel-scheduler.h
    model/higher-tx-tag.h
    model/ocb-wifi-mac.h
    model/vendor-specific-action.h
    model/vsa-manager.h
    model/wave-frame-exchange-manager.h
    model/wave-net-device.h
)

set(libraries_to_link ${libcore} ${libpropagation} ${libwifi} ${libinternet})

set(test_sources test/mac-extension-test-suite.cc test/ocb-test-suite.cc)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
          "${test_sources}"
)
