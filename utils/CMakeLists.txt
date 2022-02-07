if(${ENABLE_TESTS} AND (test IN_LIST libs_to_build))
  add_executable(test-runner $<TARGET_OBJECTS:${libtest}> test-runner.cc)

  if(${NS3_MONOLIB})
    target_link_libraries(
      test-runner ${LIB_AS_NEEDED_PRE} ${ns3-libs-tests} ${LIB_AS_NEEDED_POST}
      ${lib-ns3-monolib} ${ns3-contrib-libs}
    )
  else()
    target_link_libraries(
      test-runner ${LIB_AS_NEEDED_PRE} ${ns3-libs-tests} ${LIB_AS_NEEDED_POST}
      ${ns3-libs} ${ns3-contrib-libs}
    )
  endif()
  set_runtime_outputdirectory(
    test-runner ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utils/ ""
  )
  add_dependencies(all-test-targets test-runner)
endif()

add_executable(bench-simulator bench-simulator.cc)
target_link_libraries(bench-simulator ${libcore})
set_runtime_outputdirectory(
  bench-simulator ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utils/ ""
)

if(network IN_LIST libs_to_build)
  add_executable(bench-packets bench-packets.cc)
  target_link_libraries(bench-packets ${libnetwork})
  set_runtime_outputdirectory(
    bench-packets ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utils/ ""
  )

  add_executable(print-introspected-doxygen print-introspected-doxygen.cc)
  target_link_libraries(
    print-introspected-doxygen
    PRIVATE ${LIB_AS_NEEDED_PRE} ${ns3-libs} ${ns3-contrib-libs}
            ${LIB_AS_NEEDED_POST}
  )
  set_runtime_outputdirectory(
    print-introspected-doxygen ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utils/ ""
  )
endif()

if(core IN_LIST ns3-all-enabled-modules)
  add_executable(perf-io perf/perf-io.cc)
  target_link_libraries(perf-io PRIVATE ${libcore})
  set_runtime_outputdirectory(perf-io ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utils/perf/ "")
endif()
