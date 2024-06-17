/*
 * Copyright (c) 2010 The Boeing Company
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gary Pei <guangyu.pei@boeing.com>
 */

// This example is used to validate Nist, Yans and Table-based error rate models for OFDM rates.
//
// It outputs plots of the Frame Success Rate versus the Signal-to-noise ratio for
// Nist, Yans and Table-based error rate models and for every OFDM mode.

#include "ns3/command-line.h"
#include "ns3/gnuplot.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/table-based-error-rate-model.h"
#include "ns3/wifi-tx-vector.h"
#include "ns3/yans-error-rate-model.h"

#include <cmath>
#include <fstream>

using namespace ns3;

int
main(int argc, char* argv[])
{
    uint32_t frameSizeBytes = 1500;
    std::ofstream yansfile("yans-frame-success-rate-ofdm.plt");
    std::ofstream nistfile("nist-frame-success-rate-ofdm.plt");
    std::ofstream tablefile("table-frame-success-rate-ofdm.plt");

    const std::vector<std::string> modes{
        "OfdmRate6Mbps",
        "OfdmRate9Mbps",
        "OfdmRate12Mbps",
        "OfdmRate18Mbps",
        "OfdmRate24Mbps",
        "OfdmRate36Mbps",
        "OfdmRate48Mbps",
        "OfdmRate54Mbps",
    };

    CommandLine cmd(__FILE__);
    cmd.AddValue("FrameSize", "The frame size in bytes", frameSizeBytes);
    cmd.Parse(argc, argv);

    Gnuplot yansplot = Gnuplot("yans-frame-success-rate-ofdm.eps");
    Gnuplot nistplot = Gnuplot("nist-frame-success-rate-ofdm.eps");
    Gnuplot tableplot = Gnuplot("table-frame-success-rate-ofdm.eps");

    Ptr<YansErrorRateModel> yans = CreateObject<YansErrorRateModel>();
    Ptr<NistErrorRateModel> nist = CreateObject<NistErrorRateModel>();
    Ptr<TableBasedErrorRateModel> table = CreateObject<TableBasedErrorRateModel>();
    WifiTxVector txVector;

    uint32_t frameSizeBits = frameSizeBytes * 8;

    for (const auto& mode : modes)
    {
        std::cout << mode << std::endl;
        Gnuplot2dDataset yansdataset(mode);
        Gnuplot2dDataset nistdataset(mode);
        Gnuplot2dDataset tabledataset(mode);
        txVector.SetMode(mode);

        WifiMode wifiMode(mode);

        for (double snrDb = -5.0; snrDb <= 30.0; snrDb += 0.1)
        {
            double snr = std::pow(10.0, snrDb / 10.0);

            double ps = yans->GetChunkSuccessRate(wifiMode, txVector, snr, frameSizeBits);
            if (ps < 0.0 || ps > 1.0)
            {
                // error
                exit(1);
            }
            yansdataset.Add(snrDb, ps);

            ps = nist->GetChunkSuccessRate(wifiMode, txVector, snr, frameSizeBits);
            if (ps < 0.0 || ps > 1.0)
            {
                // error
                exit(1);
            }
            nistdataset.Add(snrDb, ps);

            ps = table->GetChunkSuccessRate(wifiMode, txVector, snr, frameSizeBits);
            if (ps < 0.0 || ps > 1.0)
            {
                // error
                exit(1);
            }
            tabledataset.Add(snrDb, ps);
        }

        yansplot.AddDataset(yansdataset);
        nistplot.AddDataset(nistdataset);
        tableplot.AddDataset(tabledataset);
    }

    yansplot.SetTerminal("postscript eps color enh \"Times-BoldItalic\"");
    yansplot.SetLegend("SNR(dB)", "Frame Success Rate");
    yansplot.SetExtra("set xrange [-5:30]\n\
set yrange [0:1.2]\n\
set style line 1 linewidth 5\n\
set style line 2 linewidth 5\n\
set style line 3 linewidth 5\n\
set style line 4 linewidth 5\n\
set style line 5 linewidth 5\n\
set style line 6 linewidth 5\n\
set style line 7 linewidth 5\n\
set style line 8 linewidth 5\n\
set style increment user");
    yansplot.GenerateOutput(yansfile);
    yansfile.close();

    nistplot.SetTerminal("postscript eps color enh \"Times-BoldItalic\"");
    nistplot.SetLegend("SNR(dB)", "Frame Success Rate");
    nistplot.SetExtra("set xrange [-5:30]\n\
set yrange [0:1.2]\n\
set style line 1 linewidth 5\n\
set style line 2 linewidth 5\n\
set style line 3 linewidth 5\n\
set style line 4 linewidth 5\n\
set style line 5 linewidth 5\n\
set style line 6 linewidth 5\n\
set style line 7 linewidth 5\n\
set style line 8 linewidth 5\n\
set style increment user");

    nistplot.GenerateOutput(nistfile);
    nistfile.close();

    tableplot.SetTerminal("postscript eps color enh \"Times-BoldItalic\"");
    tableplot.SetLegend("SNR(dB)", "Frame Success Rate");
    tableplot.SetExtra("set xrange [-5:30]\n\
set yrange [0:1.2]\n\
set style line 1 linewidth 5\n\
set style line 2 linewidth 5\n\
set style line 3 linewidth 5\n\
set style line 4 linewidth 5\n\
set style line 5 linewidth 5\n\
set style line 6 linewidth 5\n\
set style line 7 linewidth 5\n\
set style line 8 linewidth 5\n\
set style increment user");

    tableplot.GenerateOutput(tablefile);
    tablefile.close();

    return 0;
}
