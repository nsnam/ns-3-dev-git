set(name antenna)

set(source_files
    model/angles.cc
    model/antenna-model.cc
    model/cosine-antenna-model.cc
    model/isotropic-antenna-model.cc
    model/parabolic-antenna-model.cc
    model/phased-array-model.cc
    model/three-gpp-antenna-model.cc
    model/uniform-planar-array.cc
)

set(header_files
    model/angles.h
    model/antenna-model.h
    model/cosine-antenna-model.h
    model/isotropic-antenna-model.h
    model/parabolic-antenna-model.h
    model/phased-array-model.h
    model/three-gpp-antenna-model.h
    model/uniform-planar-array.h
)

# link to dependencies
set(libraries_to_link ${libcore})

set(test_sources
    test/test-angles.cc test/test-degrees-radians.cc
    test/test-isotropic-antenna.cc test/test-cosine-antenna.cc
    test/test-parabolic-antenna.cc test/test-uniform-planar-array.cc
)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
          "${test_sources}"
)
