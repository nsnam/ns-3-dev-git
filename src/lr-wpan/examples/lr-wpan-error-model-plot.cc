/*
 * Copyright (c) 2011 The Boeing Company
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gary Pei <guangyu.pei@boeing.com>
 */
#include "ns3/abort.h"
#include "ns3/command-line.h"
#include "ns3/gnuplot.h"
#include "ns3/log.h"
#include "ns3/lr-wpan-error-model.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace ns3;
using namespace ns3::lrwpan;

NS_LOG_COMPONENT_DEFINE("LrWpanErrorModelPlot");

//
// Plot 802.15.4 BER curve (compare against Figure E.2 of IEEE-802.15.4-2006)
//
int
main(int argc, char* argv[])
{
    std::ofstream berfile("802.15.4-ber.plt");
    Ptr<LrWpanErrorModel> lrWpanError = CreateObject<LrWpanErrorModel>();

    double increment = 0.1;
    double minSnr = -10; // dB
    double maxSnr = 5;

    Gnuplot berplot = Gnuplot("802.15.4-ber.eps");
    Gnuplot2dDataset berdataset("802.15.4");

    for (double snr = minSnr; snr <= maxSnr; snr += increment)
    {
        double ber = 1.0 - lrWpanError->GetChunkSuccessRate(pow(10.0, snr / 10.0), 1);
        NS_LOG_DEBUG(snr << "(dB) " << ber << " (BER)");
        berdataset.Add(snr, ber);
    }

    berplot.AddDataset(berdataset);

    berplot.SetTerminal("postscript eps color enh \"Times-BoldItalic\"");
    berplot.SetLegend("SNR (dB)", "Bit Error Rate (BER)");
    berplot.SetExtra("set xrange [-10:15]\n\
set yrange [1e-9:1]\n\
set log y\n\
set grid\n\
set style line 1 linewidth 5\n\
set style line 2 linewidth 5\n\
set style line 3 linewidth 5\n\
set style line 4 linewidth 5\n\
set style line 5 linewidth 5\n\
set style line 6 linewidth 5\n\
set style line 7 linewidth 5\n\
set style line 8 linewidth 5\n\
set style increment user");
    berplot.GenerateOutput(berfile);
    berfile.close();

    return 0;
}
