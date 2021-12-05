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

function(generate_fakewaflock)
  set(fakewaflock_contents "")
  string(APPEND fakewaflock_contents "launch_dir = '${PROJECT_SOURCE_DIR}'\n")
  string(APPEND fakewaflock_contents "run_dir = '${PROJECT_SOURCE_DIR}'\n")
  string(APPEND fakewaflock_contents "top_dir = '${PROJECT_SOURCE_DIR}'\n")
  string(APPEND fakewaflock_contents "out_dir = '${CMAKE_OUTPUT_DIRECTORY}'\n")

  set(fakewaflock_filename)
  if(LINUX)
    set(fakewaflock_filename .lock-waf_linux_build)
  elseif(APPLE)
    set(fakewaflock_filename .lock-waf_darwin_build)
  else()
    message(FATAL_ERROR "Platform not supported")
  endif()

  file(WRITE ${PROJECT_SOURCE_DIR}/${fakewaflock_filename}
       ${fakewaflock_contents}
  )
endfunction(generate_fakewaflock)
