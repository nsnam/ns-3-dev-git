# Copyright (c) 2010 The Boeing Company
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gary Pei <guangyu.pei@boeing.com>

set term postscript eps color enh "Times-BoldItalic"
set output '80211b.ieee.pkt.eps'
set xlabel "RSS (dBm)"
set ylabel "Packet Received"
set yrange [0:200]
set xrange [-102:-83]
plot "80211b.txt" using 1:11 title '1M IEEE', \
"80211b.txt" using 1:12 title '2M IEEE', \
"80211b.txt" using 1:13 title '5.5M IEEE', \
"80211b.txt" using 1:14 title '11M IEEE'
set term postscript eps color enh "Times-BoldItalic"
set output '80211b.ns3.pkt.eps'
set xlabel "RSS (dBm)"
set ylabel "Packet Received"
set yrange [0:200]
set xrange [-102:-83]
plot "80211b.txt" using 1:19 title '1M DBPSK', \
"80211b.txt" using 1:20 title '2M DQPSK', \
"80211b.txt" using 1:21 title '5.5M CCK16', \
"80211b.txt" using 1:22 title '11M CCK256'
set term postscript eps color enh "Times-BoldItalic"
set output '80211b.ieee.sir.eps'
set xlabel "SIR"
set ylabel "BER"
set yrange [10e-9:1]
set xrange [-2:10]
set logscale y
plot "80211b.txt" using 2:7 title '1M IEEE', \
"80211b.txt" using 2:8 title '2M IEEE', \
"80211b.txt" using 2:9 title '5.5M IEEE', \
"80211b.txt" using 2:10 title '11M IEEE'
