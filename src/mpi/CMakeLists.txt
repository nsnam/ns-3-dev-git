set(name mpi)

include_directories(${MPI_CXX_INCLUDE_DIRS})

set(source_files
    model/distributed-simulator-impl.cc
    model/granted-time-window-mpi-interface.cc
    model/mpi-interface.cc
    model/mpi-receiver.cc
    model/null-message-mpi-interface.cc
    model/null-message-simulator-impl.cc
    model/parallel-communication-interface.h
    model/remote-channel-bundle-manager.cc
    model/remote-channel-bundle.cc
)

set(header_files model/mpi-interface.h model/mpi-receiver.h model/parallel-communication-interface.h)

set(libraries_to_link ${libcore} ${libnetwork} ${MPI_CXX_LIBRARIES})

set(example_as_test_suite)
if(${NS3_EXAMPLES})
  set(example_as_test_suite test/mpi-test-suite.cc)
endif()

set(test_sources ${example_as_test_suite})

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")
