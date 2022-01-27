if(${ENABLE_EXAMPLES})
  subdirlist(examples_to_build ${CMAKE_CURRENT_SOURCE_DIR})

  # Process subdirectories
  foreach(examplefolder ${examples_to_build})
    add_subdirectory(${examplefolder})

    set(ns3-example-folders
        "${examplefolder};${ns3-example-folders}"
        CACHE INTERNAL "list of example folders"
    )
  endforeach()
  scan_python_examples(${CMAKE_CURRENT_SOURCE_DIR})
endif()
