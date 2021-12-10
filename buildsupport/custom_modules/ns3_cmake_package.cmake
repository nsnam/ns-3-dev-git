# Copyright (c) 2017-2021 Universidade de Brasília
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

function(ns3_cmake_package)
  # Only create configuration to export if there is an module configured to be
  # built
  set(enabled_modules "${ns3-libs};${ns3-contrib-libs}")
  if(enabled_modules STREQUAL ";")
    message(
      STATUS
        "No modules were configured, so we cannot create installation artifacts"
    )
    return()
  endif()

  install(
    EXPORT ns3ExportTargets
    NAMESPACE ns3::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ns3
    FILE ns3Targets.cmake
  )

  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    "buildsupport/Config.cmake.in" "ns3Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ns3
    PATH_VARS CMAKE_INSTALL_LIBDIR
  )

  # CMake does not support '-' separated versions in config packages, so replace
  # them with dots
  string(REPLACE "-" "." ns3_version "${NS3_VER}")
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/ns3ConfigVersion.cmake VERSION ${ns3_version}
    COMPATIBILITY ExactVersion
  )

  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ns3Config.cmake"
                "${CMAKE_CURRENT_BINARY_DIR}/ns3ConfigVersion.cmake"
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ns3
  )
endfunction()

# You will need administrative privileges to run this
add_custom_target(
  uninstall
  COMMAND
    rm `ls ${CMAKE_INSTALL_FULL_LIBDIR}/libns3*` && rm -R
    ${CMAKE_INSTALL_FULL_LIBDIR}/cmake/ns3 && rm -R
    ${CMAKE_INSTALL_FULL_INCLUDEDIR}/ns3
)
