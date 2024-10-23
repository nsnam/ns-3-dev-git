# Copyright (c) 2017-2021 Universidade de Brasília
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>

if(${NS3_COVERAGE})
  mark_as_advanced(GCOV)
  find_program(GCOV gcov)
  if(NOT ("${GCOV}" STREQUAL "GCOV-NOTFOUND"))
    add_definitions(--coverage)
    link_libraries(-lgcov)
  endif()

  mark_as_advanced(LCOV)
  find_program(LCOV lcov)
  if("${LCOV}" STREQUAL "LCOV-NOTFOUND")
    message(FATAL_ERROR "LCOV is required but it is not installed.")
  endif()

  set(zero_counters)
  if(${NS3_COVERAGE_ZERO_COUNTERS})
    set(zero_counters "--lcov-zerocounters")
  endif()
  # The following target will run test.py --no-build to generate the code
  # coverage files .gcno and .gcda output will be in ${CMAKE_BINARY_DIR} a.k.a.
  # cmake-cache or cmake-build-${build_suffix}

  # Create output directory for coverage info and html
  make_directory(${CMAKE_OUTPUT_DIRECTORY}/coverage)

  # Extract code coverage results and build html report
  add_custom_target(
    coverage_gcc
    COMMAND lcov -o ns3.info -c --directory ${CMAKE_BINARY_DIR} ${zero_counters}
    WORKING_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/coverage
    DEPENDS run_test_py
  )

  add_custom_target(
    coverage_html
    COMMAND genhtml ns3.info
    WORKING_DIRECTORY ${CMAKE_OUTPUT_DIRECTORY}/coverage
    DEPENDS coverage_gcc
  )

  # Convert lcov results to cobertura (compatible with gitlab)
  check_deps(cobertura_deps EXECUTABLES c++filt PYTHON_PACKAGES lcov_cobertura)
  if(cobertura_deps)
    message(
      WARNING
        "Code coverage conversion from LCOV to Cobertura requires missing dependencies: ${cobertura_deps}"
    )
  else()
    add_custom_target(
      coverage_cobertura
      COMMAND
        lcov_cobertura ${CMAKE_OUTPUT_DIRECTORY}/coverage/ns3.info --output
        ${CMAKE_OUTPUT_DIRECTORY}/coverage/cobertura.xml --demangle
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      DEPENDS coverage_gcc
    )
  endif()
endif()
