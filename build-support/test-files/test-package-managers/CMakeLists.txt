if(NOT
   TEST_PACKAGE_MANAGER
)
  message(
    STATUS "Skipping test module because TEST_PACKAGE_MANAGER is not set."
  )
  return()
endif()

set(armadillo)
if(${TEST_PACKAGE_MANAGER}
   STREQUAL
   "CPM"
)
  set(armadillo
      armadillo
  )
  include_directories(${CMAKE_BINARY_DIR}/_deps/armadillo-src/include)
elseif(
  ${TEST_PACKAGE_MANAGER}
  STREQUAL
  "VCPKG"
)
  set(armadillo
      ${ARMADILLO_LIBRARIES}
  )
  include_directories(${ARMADILLO_INCLUDE_DIRS})
else()
  message(FATAL_ERROR "zoincs")
  message(
    STATUS
      "Skipping test module because TEST_PACKAGE_MANAGER is set to an unsupported value."
  )
  return()
endif()

build_lib(
  LIBNAME test-package-managers
  SOURCE_FILES src.cc
  LIBRARIES_TO_LINK ${libcore}
                    ${armadillo}
)
