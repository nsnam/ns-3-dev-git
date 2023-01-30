# Create a custom library
add_library(
  scratch-nested-subdir-lib
  lib/scratch-nested-subdir-library-source.cc
)

# Link the custom library to the scratch executable
build_exec(
  EXECNAME scratch-nested-subdir-executable
  SOURCE_FILES scratch-nested-subdir-executable.cc
  LIBRARIES_TO_LINK scratch-nested-subdir-lib
                    ${libcore}
  # use "${ns3-libs}" "${ns3-contrib-libs}" in case you want to link to all
  # enabled modules
  EXECUTABLE_DIRECTORY_PATH ${CMAKE_OUTPUT_DIRECTORY}/scratch/nested-subdir
)
