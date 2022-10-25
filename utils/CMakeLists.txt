if(${ENABLE_TESTS} AND (test IN_LIST libs_to_build))
  if(WIN32)
    # DLL linking shenanigans prevent loading symbols unused by a certain program,
    # so link the tests libraries (here built as objects) directly to the test runner
    add_executable(test-runner test-runner.cc ${ns3-libs-tests} $<TARGET_OBJECTS:${libtest}>)
    if(${NS3_MONOLIB})
      target_link_libraries(
              test-runner ${lib-ns3-monolib} ${ns3-contrib-libs}
      )
    else()
      target_link_libraries(
              test-runner ${ns3-libs} ${ns3-contrib-libs}
      )
    endif()
  else()
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
  endif()
  set_runtime_outputdirectory(
    test-runner ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utils/ ""
  )
  add_dependencies(test-runner test-runner-examples-as-tests)
  add_dependencies(all-test-targets test-runner)
endif()

build_exec(
        EXECNAME bench-scheduler
        SOURCE_FILES bench-scheduler.cc
        LIBRARIES_TO_LINK ${libcore}
        EXECUTABLE_DIRECTORY_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utils/
      )

if(network IN_LIST libs_to_build)
  build_exec(
        EXECNAME bench-packets
        SOURCE_FILES bench-packets.cc
        LIBRARIES_TO_LINK ${libnetwork}
        EXECUTABLE_DIRECTORY_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utils/
      )

  build_exec(
      EXECNAME print-introspected-doxygen
      SOURCE_FILES print-introspected-doxygen.cc
      LIBRARIES_TO_LINK ${ns3-libs} ${ns3-contrib-libs}
      EXECUTABLE_DIRECTORY_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utils/
    )
endif()

if(core IN_LIST ns3-all-enabled-modules)
  build_exec(
    EXECNAME perf-io
    SOURCE_FILES perf/perf-io.cc
    LIBRARIES_TO_LINK ${libcore}
    EXECUTABLE_DIRECTORY_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/utils/perf/
  )
endif()
