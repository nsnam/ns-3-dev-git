file(GLOB_RECURSE scratches ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

set(DONT_BUILD)

foreach(scratch_src ${scratches})
  # Get source filename without path or extension
  get_filename_component(scratch_name ${scratch_src} NAME)
  string(REGEX REPLACE "\\.[^.]*$" "" scratch_name ${scratch_name})

  # Get source absolute path and transform into relative path
  get_filename_component(scratch_absolute_directory ${scratch_src} DIRECTORY)
  string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" scratch_directory ${scratch_absolute_directory})

  # Build scratch if not listed as a DONT_BUILD
  string(FIND "${DONT_BUILD}" "${scratch_name}" res)
  if(res LESS 0)
    add_executable(${scratch_name} "${scratch_src}")
    if(${NS3_STATIC})
      target_link_libraries(${scratch_name} ${LIB_AS_NEEDED_PRE_STATIC} ${lib-ns3-static})
    else()
      target_link_libraries(${scratch_name} "${ns3-libs}" "${ns3-contrib-libs}")
    endif()
    set_runtime_outputdirectory(${scratch_name} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scratch/${scratch_directory}/)
  endif()
endforeach()
