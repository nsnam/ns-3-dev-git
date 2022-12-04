#!/usr/bin/env python3

import re
import subprocess

ccache_misses_line = subprocess.check_output(["ccache", "--print-stats"]).decode()
ccache_misses = int(re.findall("cache_miss(.*)", ccache_misses_line)[0])
print(ccache_misses)
