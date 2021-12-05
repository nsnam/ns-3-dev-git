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

function(generate_buildstatus)
  # Build build-status.py file consumed by test.py
  set(buildstatus_contents "#! /usr/bin/env python3\n\n")
  string(APPEND buildstatus_contents "ns3_runnable_programs = [")

  foreach(executable ${ns3-execs})
    string(APPEND buildstatus_contents "'${executable}', ")
  endforeach()
  string(APPEND buildstatus_contents "]\n\n")

  string(APPEND buildstatus_contents "ns3_runnable_scripts = [") # missing
                                                                 # support
  string(APPEND buildstatus_contents "]\n\n")

  file(WRITE ${CMAKE_OUTPUT_DIRECTORY}/build-status.py
       "${buildstatus_contents}"
  )
endfunction(generate_buildstatus)
