if(NOT
   ${ENABLE_EMU}
)
  message(
    ${HIGHLIGHTED_STATUS}
    "tap-bridge requires emulation feature"
  )
  message(
    ${HIGHLIGHTED_STATUS}
    "Skipping src/tap-bridge"
  )
  list(
    REMOVE_ITEM
    libs_to_build
    tap-bridge
    PARENT_SCOPE
  )
  return()
endif()

build_lib(
  LIBNAME tap-bridge
  SOURCE_FILES
    helper/tap-bridge-helper.cc
    model/tap-bridge.cc
    model/tap-encode-decode.cc
  HEADER_FILES
    helper/tap-bridge-helper.h
    model/tap-bridge.h
    model/tap-encode-decode.h
  LIBRARIES_TO_LINK ${libinternet}
)

add_definitions(
  -DTAP_CREATOR="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/src/tap-bridge/ns${NS3_VER}-tap-creator${build_profile_suffix}"
)
build_exec(
  EXECNAME tap-creator
  SOURCE_FILES model/tap-creator.cc
               model/tap-encode-decode.cc
  LIBRARIES_TO_LINK ${libnetwork}
  EXECUTABLE_DIRECTORY_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/src/tap-bridge/
  INSTALL_DIRECTORY_PATH ${CMAKE_INSTALL_LIBEXECDIR}/ns3
  STANDALONE
)
