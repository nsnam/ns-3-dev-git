build_lib(
  LIBNAME visualizer
  SOURCE_FILES model/pyviz.cc
               model/visual-simulator-impl.cc
  HEADER_FILES model/pyviz.h
  LIBRARIES_TO_LINK
    ${Python3_LIBRARIES}
    ${libcore}
    ${libinternet}
    ${libwifi}
    ${libpoint-to-point}
    ${libcsma}
    ${libbridge}
    ${libwimax}
    ${liblte}
    ${libmesh}
)

# move visualizer folder to build/bindings/python, which allows us to add only
# PYTHONPATH=ns-3-dev/build/bindings/python
file(
  GLOB_RECURSE
  visualizer_files
  ${CMAKE_CURRENT_SOURCE_DIR}/visualizer/*
)
foreach(
  file
  ${visualizer_files}
)
  string(
    REPLACE "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_OUTPUT_DIRECTORY}/bindings/python"
            destination
            ${file}
  )
  configure_file(
    ${file}
    ${destination}
    COPYONLY
  )
endforeach()

if(NOT
   NS3_BINDINGS_INSTALL_DIR
)
  message(
    ${HIGHLIGHTED_STATUS}
    "NS3_BINDINGS_INSTALL_DIR was not set. The visualizer python module won't be installed with ./ns3 install."
  )
else()
  install(
    DIRECTORY ./visualizer
    DESTINATION ${NS3_BINDINGS_INSTALL_DIR}/
  )

  add_custom_target(
    uninstall_visualizer
    COMMAND rm -R ${NS3_BINDINGS_INSTALL_DIR}/visualizer
  )
  add_dependencies(
    uninstall
    uninstall_visualizer
  )
endif()
