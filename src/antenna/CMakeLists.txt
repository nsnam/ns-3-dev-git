# Check for dependencies and add sources accordingly
check_cxx_source_compiles(
  "#include <cmath>
    int main()
    {
      return std::cyl_bessel_j(1, 1);
    }"
  HAVE_STD_BESSEL_FUNC
)

set(circular_aperture_antenna_sources)
set(circular_aperture_antenna_headers)
set(circular_aperture_antenna_test_sources)

if(${HAVE_STD_BESSEL_FUNC})
  set(circular_aperture_antenna_sources
      model/circular-aperture-antenna-model.cc
  )
  set(circular_aperture_antenna_headers
      model/circular-aperture-antenna-model.h
  )
  set(circular_aperture_antenna_test_sources
      test/test-circular-aperture-antenna.cc
  )
  message(STATUS "Standard library Bessel function has been found")
else()
  check_include_files(
    "boost/math/special_functions/bessel.hpp"
    HAVE_BOOST_BESSEL_FUNC
    LANGUAGE
    CXX
  )
  if(${HAVE_BOOST_BESSEL_FUNC})
    set(circular_aperture_antenna_sources
        model/circular-aperture-antenna-model.cc
    )
    set(circular_aperture_antenna_headers
        model/circular-aperture-antenna-model.h
    )
    set(circular_aperture_antenna_test_sources
        test/test-circular-aperture-antenna.cc
    )
    add_definitions(-DNEED_AND_HAVE_BOOST_BESSEL_FUNC)
    message(STATUS "Boost Bessel function has been found.")
  else()
    message(
      STATUS
        "Boost Bessel function is required for CircularApertureAntennaModel"
        " on platforms using Clang libc++ such as macOS."
        " You may need to clean up the CMake cache after installing it to pass this check."
    )
  endif()
endif()

build_lib(
  LIBNAME antenna
  SOURCE_FILES
    ${circular_aperture_antenna_sources}
    model/angles.cc
    model/antenna-model.cc
    model/cosine-antenna-model.cc
    model/isotropic-antenna-model.cc
    model/parabolic-antenna-model.cc
    model/phased-array-model.cc
    model/three-gpp-antenna-model.cc
    model/uniform-planar-array.cc
  HEADER_FILES
    ${circular_aperture_antenna_headers}
    model/angles.h
    model/antenna-model.h
    model/cosine-antenna-model.h
    model/isotropic-antenna-model.h
    model/parabolic-antenna-model.h
    model/phased-array-model.h
    model/three-gpp-antenna-model.h
    model/uniform-planar-array.h
    utils/symmetric-adjacency-matrix.h
  LIBRARIES_TO_LINK ${libcore}
  TEST_SOURCES
    ${circular_aperture_antenna_test_sources}
    test/test-angles.cc
    test/test-cosine-antenna.cc
    test/test-degrees-radians.cc
    test/test-isotropic-antenna.cc
    test/test-parabolic-antenna.cc
    test/test-uniform-planar-array.cc
    test/test-adjacency-matrix.cc
)
