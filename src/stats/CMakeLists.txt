set(name stats)

if(${NS3_SQLITE})
  find_package(SQLite3)
  check_include_file_cxx(semaphore.h HAVE_SEMAPHORE_H)
  if(${SQLite3_FOUND})
    set(sqlite_sources model/sqlite-data-output.cc)
    set(sqlite_headers model/sqlite-data-output.h)

    include_directories(${SQLite3_INCLUDE_DIRS})
    set(sqlite_libraries ${SQLite3_LIBRARIES})

    if(HAVE_SEMAPHORE_H)
      list(APPEND sqlite_sources model/sqlite-output.cc)
      list(APPEND sqlite_headers model/sqlite-output.h)
    endif()
  endif()

  if("${SQLite3_FOUND}")
    set(ENABLE_SQLITE True CACHE INTERNAL "")
  else()
    set(ENABLE_SQLITE False CACHE INTERNAL "")
  endif()
endif()

set(source_files
    ${sqlite_sources}
    helper/file-helper.cc
    helper/gnuplot-helper.cc
    model/boolean-probe.cc
    model/data-calculator.cc
    model/data-collection-object.cc
    model/data-collector.cc
    model/data-output-interface.cc
    model/double-probe.cc
    model/file-aggregator.cc
    model/get-wildcard-matches.cc
    model/gnuplot-aggregator.cc
    model/gnuplot.cc
    model/histogram.cc
    model/omnet-data-output.cc
    model/probe.cc
    model/time-data-calculators.cc
    model/time-probe.cc
    model/time-series-adaptor.cc
    model/uinteger-16-probe.cc
    model/uinteger-32-probe.cc
    model/uinteger-8-probe.cc
)

set(header_files
    ${sqlite_headers}
    helper/file-helper.h
    helper/gnuplot-helper.h
    model/average.h
    model/basic-data-calculators.h
    model/boolean-probe.h
    model/data-calculator.h
    model/data-collection-object.h
    model/data-collector.h
    model/data-output-interface.h
    model/double-probe.h
    model/file-aggregator.h
    model/get-wildcard-matches.h
    model/gnuplot-aggregator.h
    model/gnuplot.h
    model/histogram.h
    model/omnet-data-output.h
    model/probe.h
    model/stats.h
    model/time-data-calculators.h
    model/time-probe.h
    model/time-series-adaptor.h
    model/uinteger-16-probe.h
    model/uinteger-32-probe.h
    model/uinteger-8-probe.h
)

set(libraries_to_link ${libcore} ${sqlite_libraries})

set(test_sources test/average-test-suite.cc test/basic-data-calculators-test-suite.cc test/double-probe-test-suite.cc
                 test/histogram-test-suite.cc
)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")
