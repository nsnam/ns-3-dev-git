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

macro(ns3_cmake_package)
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

  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/ns3ConfigVersion.cmake VERSION ${NS3_VER}
    COMPATIBILITY ExactVersion
  )

  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ns3Config.cmake"
                "${CMAKE_CURRENT_BINARY_DIR}/ns3ConfigVersion.cmake"
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ns3
  )

  install(DIRECTORY ${CMAKE_HEADER_OUTPUT_DIRECTORY}
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  # Hack to get the install_manifest.txt file before finishing the installation,
  # which we can then install along with ns3 to make uninstallation trivial
  set(sep "/")
  install(
    CODE "string(REPLACE \";\" \"\\n\" MY_CMAKE_INSTALL_MANIFEST_CONTENT \"\$\{CMAKE_INSTALL_MANIFEST_FILES\}\")\n\
            string(REPLACE \"/\" \"${sep}\" MY_CMAKE_INSTALL_MANIFEST_CONTENT_FINAL \"\$\{MY_CMAKE_INSTALL_MANIFEST_CONTENT\}\")\n\
                file(WRITE manifest.txt \"\$\{MY_CMAKE_INSTALL_MANIFEST_CONTENT_FINAL\}\")"
  )
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/manifest.txt
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ns3
  )
endmacro()

# You will need administrative privileges to run this
add_custom_target(
  uninstall
  COMMAND
    rm -R `cat ${CMAKE_INSTALL_FULL_LIBDIR}/cmake/ns3/manifest.txt` && rm -R
    ${CMAKE_INSTALL_FULL_LIBDIR}/cmake/ns3 && rm -R
    ${CMAKE_INSTALL_FULL_INCLUDEDIR}/ns3
)
