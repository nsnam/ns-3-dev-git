#!/bin/sh

# Helper to run waf from any directory. Inspired by:
# https://www.nsnam.org/docs/tutorial/html/getting-started.html#working-directory

# Source : https://github.com/NotSpecial/ns-3-dev

CWD="$PWD"
cd /ns3 >/dev/null
./waf --cwd="$CWD" "$@"
cd - >/dev/null