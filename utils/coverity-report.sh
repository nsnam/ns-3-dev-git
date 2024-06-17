#!/bin/bash


# Copyright (c) 2013 Lawrence Livermore National Laboratory
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>

#
#  Do a coverity build and submit report
#

me=`basename $0`

# echo commands and output to a log file

logf=coverity/coverity-build.log
echo | tee $logf

function say ()
{
    echo "$me:" $* | tee -a $logf
}
blank ()
{
    echo | tee -a $logf
}
function doo ()
{
    say "$ "$*
    $* 2>&1 | tee -a $logf
}




say $(date)
blank

doo ./ns3 clean
blank

doo ./ns3 configure $NS3CONFIG
blank

cov=coverity/cov-int
doo cov-build --dir $cov ./ns3 build
blank

tarf=coverity/ns-3.tgz
doo tar cvzf $tarf -C coverity cov-int
blank

useremail=$(hg showconfig ui.username | \
    egrep -o "\b[a-zA-Z0-9.-]+@[a-zA-Z0-9.-]+\.[a-zA-Z0-9.-]+\b")

repoversion="$(basename $(dirname $PWD))@$(hg id -i)"

# curl complains if this contains white space
description="Coverity-mods"

doo curl \
     --form file=@$tarf \
     --form project=ns-3 \
     --form password=4jk2BVX9 \
     --form email="$useremail" \
     --form version="$repoversion" \
     --form description="$description" \
     http://scan5.coverity.com/cgi-bin/upload.py
blank

say $(date)
blank
