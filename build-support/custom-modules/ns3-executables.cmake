function(set_runtime_outputdirectory target_name output_directory target_prefix)
  # Prevent duplicate '/' in EXECUTABLE_DIRECTORY_PATH, since it gets translated
  # to doubled underlines and will cause the ns3 script to fail
  string(REPLACE "//" "/" output_directory "${output_directory}")

  set(ns3-exec-outputname ns${NS3_VER}-${target_name}${build_profile_suffix})
  set(ns3-execs "${output_directory}${ns3-exec-outputname};${ns3-execs}"
      CACHE INTERNAL "list of c++ executables"
  )
  set(ns3-execs-clean "${target_prefix}${target_name};${ns3-execs-clean}"
      CACHE INTERNAL
            "list of c++ executables without version prefix and build suffix"
  )

  set_target_properties(
    ${target_prefix}${target_name}
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${output_directory}
               RUNTIME_OUTPUT_NAME ${ns3-exec-outputname}
  )
  if(${MSVC} OR ${XCODE})
    # Prevent multi-config generators from placing output files into per
    # configuration directory
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
      string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
      set_target_properties(
        ${target_prefix}${target_name}
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${output_directory}
                   RUNTIME_OUTPUT_NAME_${OUTPUTCONFIG} ${ns3-exec-outputname}
      )
    endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)
  endif()

  if(${ENABLE_TESTS})
    add_dependencies(all-test-targets ${target_prefix}${target_name})
    # Create a CTest entry for each executable
    if(WIN32)
      # Windows require this workaround to make sure the DLL files are located
      add_test(
        NAME ctest-${target_prefix}${target_name}
        COMMAND
          ${CMAKE_COMMAND} -E env
          "PATH=$ENV{PATH};${CMAKE_RUNTIME_OUTPUT_DIRECTORY};${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
          ${ns3-exec-outputname}
        WORKING_DIRECTORY ${output_directory}
      )
    else()
      add_test(NAME ctest-${target_prefix}${target_name}
               COMMAND ${ns3-exec-outputname}
               WORKING_DIRECTORY ${output_directory}
      )
    endif()
  endif()

  if(${NS3_CLANG_TIMETRACE})
    add_dependencies(timeTraceReport ${target_prefix}${target_name})
  endif()
endfunction(set_runtime_outputdirectory)

function(get_scratch_prefix prefix)
  # /path/to/ns-3-dev/scratch/nested-subdir
  set(temp ${CMAKE_CURRENT_SOURCE_DIR})
  # remove /path/to/ns-3-dev/ to get scratch/nested-subdir
  string(REPLACE "${PROJECT_SOURCE_DIR}/" "" temp "${temp}")
  # replace path separators with underlines
  string(REPLACE "/" "_" temp "${temp}")
  # save the prefix value to the passed variable
  set(${prefix} ${temp}_ PARENT_SCOPE)
endfunction()

function(build_exec)
  # Argument parsing
  set(options IGNORE_PCH STANDALONE)
  set(oneValueArgs EXECNAME EXECNAME_PREFIX EXECUTABLE_DIRECTORY_PATH
                   INSTALL_DIRECTORY_PATH
  )
  set(multiValueArgs SOURCE_FILES HEADER_FILES LIBRARIES_TO_LINK DEFINITIONS)
  cmake_parse_arguments(
    "BEXEC" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
  )

  # Resolve nested scratch prefixes without user intervention
  string(REPLACE "${PROJECT_SOURCE_DIR}" "" relative_path
                 "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  if("${relative_path}" MATCHES "scratch" AND "${BEXEC_EXECNAME_PREFIX}"
                                              STREQUAL ""
  )
    get_scratch_prefix(BEXEC_EXECNAME_PREFIX)
  endif()

  add_executable(
    ${BEXEC_EXECNAME_PREFIX}${BEXEC_EXECNAME} "${BEXEC_SOURCE_FILES}"
  )

  target_compile_definitions(
    ${BEXEC_EXECNAME_PREFIX}${BEXEC_EXECNAME} PUBLIC ${BEXEC_DEFINITIONS}
  )

  if(${PRECOMPILE_HEADERS_ENABLED} AND (NOT ${BEXEC_IGNORE_PCH}))
    target_precompile_headers(
      ${BEXEC_EXECNAME_PREFIX}${BEXEC_EXECNAME} REUSE_FROM stdlib_pch_exec
    )
  endif()

  # Check if library is not a ns-3 module library, and if it is, link when
  # building static or monolib builds
  set(libraries_to_always_link)
  set(modules "${libs_to_build};${contrib_libs_to_build}")
  foreach(lib ${BEXEC_LIBRARIES_TO_LINK})
    # Remove the lib prefix if one exists
    set(libless ${lib})
    string(SUBSTRING "${libless}" 0 3 prefix)
    if(prefix STREQUAL "lib")
      string(SUBSTRING "${libless}" 3 -1 libless)
    endif()
    # Check if the library is not a ns-3 module
    if(libless IN_LIST modules)
      continue()
    endif()
    list(APPEND libraries_to_always_link ${lib})
  endforeach()

  if(${NS3_STATIC} AND (NOT BEXEC_STANDALONE))
    target_link_libraries(
      ${BEXEC_EXECNAME_PREFIX}${BEXEC_EXECNAME} ${libraries_to_always_link}
      ${LIB_AS_NEEDED_PRE_STATIC} ${lib-ns3-static}
    )
  elseif(${NS3_MONOLIB} AND (NOT BEXEC_STANDALONE))
    target_link_libraries(
      ${BEXEC_EXECNAME_PREFIX}${BEXEC_EXECNAME} ${libraries_to_always_link}
      ${LIB_AS_NEEDED_PRE} ${lib-ns3-monolib} ${LIB_AS_NEEDED_POST}
    )
  else()
    target_link_libraries(
      ${BEXEC_EXECNAME_PREFIX}${BEXEC_EXECNAME} ${LIB_AS_NEEDED_PRE}
      "${BEXEC_LIBRARIES_TO_LINK}" ${LIB_AS_NEEDED_POST}
    )
  endif()

  # Handle ns-3-external-contrib/module (which will have output binaries mapped
  # to contrib/module)
  if(${BEXEC_EXECUTABLE_DIRECTORY_PATH} MATCHES "ns-3-external-contrib")
    string(
      REGEX
      REPLACE ".*ns-3-external-contrib" "${PROJECT_SOURCE_DIR}/build/contrib"
              BEXEC_EXECUTABLE_DIRECTORY_PATH
              "${BEXEC_EXECUTABLE_DIRECTORY_PATH}"
    )
  endif()

  set_runtime_outputdirectory(
    "${BEXEC_EXECNAME}" "${BEXEC_EXECUTABLE_DIRECTORY_PATH}/"
    "${BEXEC_EXECNAME_PREFIX}"
  )

  if(BEXEC_INSTALL_DIRECTORY_PATH)
    install(TARGETS ${BEXEC_EXECNAME_PREFIX}${BEXEC_EXECNAME}
            EXPORT ns3ExportTargets
            RUNTIME DESTINATION ${BEXEC_INSTALL_DIRECTORY_PATH}
    )
    get_property(
      filename TARGET ${BEXEC_EXECNAME_PREFIX}${BEXEC_EXECNAME}
      PROPERTY RUNTIME_OUTPUT_NAME
    )
    add_custom_target(
      uninstall_${BEXEC_EXECNAME_PREFIX}${BEXEC_EXECNAME}
      COMMAND
        rm ${CMAKE_INSTALL_PREFIX}/${BEXEC_INSTALL_DIRECTORY_PATH}/${filename}
    )
    add_dependencies(
      uninstall uninstall_${BEXEC_EXECNAME_PREFIX}${BEXEC_EXECNAME}
    )
  endif()
endfunction(build_exec)

function(scan_python_examples path)
  # Skip python examples search in case the bindings are disabled
  if(NOT ${ENABLE_PYTHON_BINDINGS})
    return()
  endif()

  # Search for example scripts in the directory and its immediate children
  # directories
  set(python_examples)
  file(GLOB examples_subdirs LIST_DIRECTORIES TRUE ${path}/*)
  foreach(subdir ${path} ${examples_subdirs})
    if(NOT (IS_DIRECTORY ${subdir}))
      continue()
    endif()
    file(GLOB python_examples_subdir ${subdir}/*.py)
    list(APPEND python_examples ${python_examples_subdir})
  endforeach()

  # Add example scripts to the list
  list(REMOVE_DUPLICATES python_examples)
  foreach(python_example ${python_examples})
    if(NOT (${python_example} MATCHES "examples-to-run"))
      set(ns3-execs-py "${python_example};${ns3-execs-py}"
          CACHE INTERNAL "list of python scripts"
      )
    endif()
  endforeach()
endfunction()
