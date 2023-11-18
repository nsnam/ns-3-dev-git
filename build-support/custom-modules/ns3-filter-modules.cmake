# Copyright (c) 2023 Universidade de Brasília
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License version 2 as published by the Free
# Software Foundation;
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA  02111-1307 USA
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>

# This is where recursive module dependency checking is implemented for
# filtering enabled and disabled modules

function(filter_modules modules_to_filter all_modules_list filter_in)
  set(new_modules_to_build)
  # We are receiving variable names as arguments, so we have to "dereference"
  # them first That is why we have the duplicated ${${}}
  foreach(module ${${all_modules_list}})
    if(${filter_in} (${module} IN_LIST ${modules_to_filter}))
      list(APPEND new_modules_to_build ${module})
    endif()
  endforeach()
  set(${all_modules_list} ${new_modules_to_build} PARENT_SCOPE)
endfunction()

function(resolve_dependencies module_name dependencies contrib_dependencies)
  # Create cache variables to hold dependencies list and visited
  set(dependency_visited "" CACHE INTERNAL "")
  set(contrib_dependency_visited "" CACHE INTERNAL "")
  recursive_dependency(${module_name})
  if(${module_name} IN_LIST dependency_visited)
    set(temp ${dependency_visited})
    list(REMOVE_AT temp 0)
    set(${dependencies} ${temp} PARENT_SCOPE)
    set(${contrib_dependencies} ${contrib_dependency_visited} PARENT_SCOPE)
  else()
    set(temp ${contrib_dependency_visited})
    list(REMOVE_AT temp 0)
    set(${dependencies} ${dependency_visited} PARENT_SCOPE)
    set(${contrib_dependencies} ${temp} PARENT_SCOPE)
  endif()
  unset(dependency_visited CACHE)
  unset(contrib_dependency_visited CACHE)
endfunction()

function(filter_libraries cmakelists_contents libraries)
  string(REGEX MATCHALL "{lib[^}]*[^obj]}" matches "${cmakelists_content}")
  list(REMOVE_ITEM matches "{libraries_to_link}")
  string(REPLACE "{lib\${name" "" matches "${matches}") # special case for
  # src/test
  string(REPLACE "{lib" "" matches "${matches}")
  string(REPLACE "}" "" matches "${matches}")
  set(${libraries} ${matches} PARENT_SCOPE)
endfunction()

function(recursive_dependency module_name)
  # Read module CMakeLists.txt and search for ns-3 modules
  set(src_cmakelist ${PROJECT_SOURCE_DIR}/src/${module_name}/CMakeLists.txt)
  set(contrib_cmakelist
      ${PROJECT_SOURCE_DIR}/contrib/${module_name}/CMakeLists.txt
  )
  set(contrib FALSE)
  if(EXISTS ${src_cmakelist})
    file(READ ${src_cmakelist} cmakelists_content)
  elseif(EXISTS ${contrib_cmakelist})
    file(READ ${contrib_cmakelist} cmakelists_content)
    set(contrib TRUE)
  else()
    set(cmakelists_content "")
    message(${HIGHLIGHTED_STATUS}
            "The CMakeLists.txt file for module ${module_name} was not found."
    )
  endif()

  filter_libraries("${cmakelists_content}" matches)

  # Add this visited module dependencies to the dependencies list
  if(contrib)
    set(contrib_dependency_visited
        "${contrib_dependency_visited};${module_name}" CACHE INTERNAL ""
    )
    set(examples_cmakelists ${contrib_cmakelist})
  else()
    set(dependency_visited "${dependency_visited};${module_name}" CACHE INTERNAL
                                                                        ""
    )
    set(examples_cmakelists ${src_cmakelist})
  endif()

  # cmake-format: off
  # Scan dependencies required by this module examples
  set(example_matches)
  #if(${ENABLE_EXAMPLES})
  #  string(REPLACE "${module_name}" "${module_name}/examples" examples_cmakelists ${examples_cmakelists})
  #  if(EXISTS ${examples_cmakelists})
  #    file(READ ${examples_cmakelists} cmakelists_content)
  #    filter_libraries(${cmakelists_content} example_matches)
  #  endif()
  #endif()
  # cmake-format: on

  # For each dependency, call this same function
  set(matches "${matches};${example_matches}")
  foreach(match ${matches})
    if(NOT ((${match} IN_LIST dependency_visited)
            OR (${match} IN_LIST contrib_dependency_visited))
    )
      recursive_dependency(${match})
    endif()
  endforeach()
endfunction()

macro(
  filter_enabled_and_disabled_modules
  libs_to_build
  contrib_libs_to_build
  NS3_ENABLED_MODULES
  NS3_DISABLED_MODULES
  ns3rc_enabled_modules
  ns3rc_disabled_modules
)
  mark_as_advanced(ns3-all-enabled-modules)

  # Before filtering, we set a variable with all scanned modules in the src
  # directory
  set(scanned_modules ${${libs_to_build}})

  # Ensure enabled and disable modules lists are using semicolons
  string(REPLACE "," ";" ${NS3_ENABLED_MODULES} "${${NS3_ENABLED_MODULES}}")
  string(REPLACE "," ";" ${NS3_DISABLED_MODULES} "${${NS3_DISABLED_MODULES}}")

  # Now that scanning modules finished, we can remove the disabled modules or
  # replace the modules list with the ones in the enabled list
  if(${NS3_ENABLED_MODULES} OR ${ns3rc_enabled_modules})
    # List of enabled modules passed by the command line overwrites list read
    # from ns3rc
    if(${NS3_ENABLED_MODULES})
      set(ns3rc_enabled_modules ${${NS3_ENABLED_MODULES}})
    endif()

    # Filter enabled modules
    filter_modules(ns3rc_enabled_modules libs_to_build "")
    filter_modules(ns3rc_enabled_modules contrib_libs_to_build "")

    # Use recursion to automatically determine dependencies required by the
    # manually enabled modules
    foreach(lib ${${contrib_libs_to_build}})
      resolve_dependencies(${lib} dependencies contrib_dependencies)
      list(APPEND ${contrib_libs_to_build} "${contrib_dependencies}")
      list(APPEND ${libs_to_build} "${dependencies}")
      unset(dependencies)
      unset(contrib_dependencies)
    endforeach()

    foreach(lib ${${libs_to_build}})
      resolve_dependencies(${lib} dependencies contrib_dependencies)
      list(APPEND ${libs_to_build} "${dependencies}")
      unset(dependencies)
      unset(contrib_dependencies)
    endforeach()
  endif()

  if(${NS3_DISABLED_MODULES} OR ${ns3rc_disabled_modules})
    # List of disabled modules passed by the command line overwrites list read
    # from ns3rc

    if(${NS3_DISABLED_MODULES})
      set(ns3rc_disabled_modules ${${NS3_DISABLED_MODULES}})
    endif()

    set(all_libs ${${libs_to_build}};${${contrib_libs_to_build}})

    # We then use the recursive dependency finding to get all dependencies of
    # all modules
    foreach(lib ${all_libs})
      resolve_dependencies(${lib} dependencies contrib_dependencies)
      set(${lib}_dependencies "${dependencies};${contrib_dependencies}")
      unset(dependencies)
      unset(contrib_dependencies)
    endforeach()

    # Now we can begin removing libraries that require disabled dependencies
    set(disabled_libs "${${ns3rc_disabled_modules}}")
    foreach(libo ${all_libs})
      foreach(lib ${all_libs})
        foreach(disabled_lib ${disabled_libs})
          if(${lib} STREQUAL ${disabled_lib})
            continue()
          endif()
          if(${disabled_lib} IN_LIST ${lib}_dependencies)
            list(APPEND disabled_libs ${lib})
            break() # proceed to the next lib in all_libs
          endif()
        endforeach()
      endforeach()
    endforeach()

    # Clean dependencies lists
    foreach(lib ${all_libs})
      unset(${lib}_dependencies)
    endforeach()

    # We can filter out disabled modules
    filter_modules(disabled_libs libs_to_build "NOT")
    filter_modules(disabled_libs contrib_libs_to_build "NOT")

    if(core IN_LIST ${libs_to_build})
      list(APPEND ${libs_to_build} test) # include test module
    endif()
  endif()

  # Older CMake versions require this workaround for empty lists
  if(NOT ${contrib_libs_to_build})
    set(${contrib_libs_to_build} "")
  endif()

  # Filter out any eventual duplicates
  list(REMOVE_DUPLICATES ${libs_to_build})
  list(REMOVE_DUPLICATES ${contrib_libs_to_build})

  # Export list with all enabled modules (used to separate ns libraries from
  # non-ns libraries in ns3_module_macros)
  set(ns3-all-enabled-modules "${${libs_to_build}};${${contrib_libs_to_build}}"
      CACHE INTERNAL "list with all enabled modules"
  )
endmacro()
