#!/usr/bin/perl
###
# *
# * Copyright (c) 2010 Adrian Sai-wah Tam
# *
# * SPDX-License-Identifier: GPL-2.0-only
# *
# * This perl script is used within plot.gp, a gnuplot file also
# * in this directory, for parsing the Ns3TcpLossTest logging output.
# * It can also be used, stand-alone, to tidy up the logging output
# * from Ns3TcpStateTestCases, if logging is enabled in these tests.
###

  while(<>) {
    s|ns3::PppHeader \(Point-to-Point Protocol: IP \(0x0021\)\) ||;
    s|/TxQueue||;
    s|/TxQ/|Q|;
    s|NodeList/|N|;
    s|/DeviceList/|D|;
    s|/MacRx||;
    s|/Enqueue||;
    s|/Dequeue||;
    s|/\$ns3::QbbNetDevice||;
    s|/\$ns3::PointToPointNetDevice||;
    s| /|\t|;
    s| ns3::|\t|g;
    s|tos 0x0 ||;
    s|protocol 6 ||;
    s|offset 0 ||;
    s|flags \[none\] ||;
    s|length:|len|;
    s|Header||g;
    s|/PhyRxDrop||;
    print;
 };
