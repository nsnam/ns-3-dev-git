# ndm-sys project module — Apache License 2.0 (see LICENSE-PROJECT).
#
# ndm-failure: planned/scripted failure scheduler (AGENTS.md D6):
# link down/up at t=T with explicit detection and convergence delays,
# in-flight packet policy, optional planned switch failure, and a
# convergence-gated reroute hook (no instantaneous global recomputation).

build_lib(
  LIBNAME ndm-failure
  SOURCE_FILES
    model/ndm-failure-scheduler.cc
  HEADER_FILES
    model/ndm-failure-scheduler.h
  LIBRARIES_TO_LINK ${libcore} ${libnetwork} ${libpoint-to-point} ${libndm-topology}
  TEST_SOURCES
    test/ndm-failure-test.cc
)
