set(name buildings)

set(source_files
    helper/building-allocator.cc
    helper/building-container.cc
    helper/building-position-allocator.cc
    helper/buildings-helper.cc
    model/building-list.cc
    model/building.cc
    model/buildings-channel-condition-model.cc
    model/buildings-propagation-loss-model.cc
    model/hybrid-buildings-propagation-loss-model.cc
    model/itu-r-1238-propagation-loss-model.cc
    model/mobility-building-info.cc
    model/oh-buildings-propagation-loss-model.cc
    model/random-walk-2d-outdoor-mobility-model.cc
    model/three-gpp-v2v-channel-condition-model.cc
)

set(header_files
    helper/building-allocator.h
    helper/building-container.h
    helper/building-position-allocator.h
    helper/buildings-helper.h
    model/building-list.h
    model/building.h
    model/buildings-channel-condition-model.h
    model/buildings-propagation-loss-model.h
    model/hybrid-buildings-propagation-loss-model.h
    model/itu-r-1238-propagation-loss-model.h
    model/mobility-building-info.h
    model/oh-buildings-propagation-loss-model.h
    model/random-walk-2d-outdoor-mobility-model.h
    model/three-gpp-v2v-channel-condition-model.h
)

set(libraries_to_link ${libmobility} ${libpropagation} ${libconfig-store})

set(test_sources
    test/building-position-allocator-test.cc
    test/buildings-helper-test.cc
    test/buildings-pathloss-test.cc
    test/buildings-shadowing-test.cc
    test/buildings-channel-condition-model-test.cc
    test/outdoor-random-walk-test.cc
    test/three-gpp-v2v-channel-condition-model-test.cc
)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
          "${test_sources}"
)
