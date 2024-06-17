#!/bin/bash
#
# Copyright (c) 2012 University of Washington
#
# SPDX-License-Identifier: GPL-2.0-only
#

#
# This script calls test.py to get a list of all tests and examples.
# It then runs all of the C++ examples with full logging turned on,
# i.e. NS_LOG="*", to see if that causes any problems with the
# example.
#

cd ..
`./test.py -l >& /tmp/test.out`

while read line
do
  # search for examples, strip down $line as necessary
  if [[ "$line" == example* ]]
    then
      name=${line#example      }
      NS_LOG="*" ./ns3 --run "$name" >& /dev/null
      status="$?"
      echo "program $name status $status"
  fi
done < "/tmp/test.out"

rm -rf /tmp/test.out
cd utils
