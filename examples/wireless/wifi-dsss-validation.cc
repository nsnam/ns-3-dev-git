/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

// This example is used to validate error rate models for DSSS rates.
//
// It outputs plots of the Frame Success Rate versus the Signal-to-noise ratio
// for the DSSS error rate models and for every DSSS mode.

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
    std::ofstream file("frame-success-rate-dsss.plt");

    const std::vector<std::string> modes{
        "DsssRate1Mbps",
        "DsssRate2Mbps",
        "DsssRate5_5Mbps",
        "DsssRate11Mbps",
    };

    CommandLine cmd(__FILE__);
    cmd.AddValue("FrameSize", "The frame size in bytes", frameSizeBytes);
    cmd.Parse(argc, argv);

    Gnuplot plot = Gnuplot("frame-success-rate-dsss.eps");

    Ptr<YansErrorRateModel> yans = CreateObject<YansErrorRateModel>();
    Ptr<NistErrorRateModel> nist = CreateObject<NistErrorRateModel>();
    Ptr<TableBasedErrorRateModel> table = CreateObject<TableBasedErrorRateModel>();
    WifiTxVector txVector;

    uint32_t frameSizeBits = frameSizeBytes * 8;

    for (const auto& mode : modes)
    {
        std::cout << mode << std::endl;
        Gnuplot2dDataset dataset(mode);
        txVector.SetMode(mode);

        WifiMode wifiMode(mode);

        for (double snrDb = -10.0; snrDb <= 20.0; snrDb += 0.1)
        {
            double snr = std::pow(10.0, snrDb / 10.0);

            double psYans = yans->GetChunkSuccessRate(wifiMode, txVector, snr, frameSizeBits);
            if (psYans < 0.0 || psYans > 1.0)
            {
                // error
                exit(1);
            }
            double psNist = nist->GetChunkSuccessRate(wifiMode, txVector, snr, frameSizeBits);
            if (psNist < 0.0 || psNist > 1.0)
            {
                std::cout << psNist << std::endl;
                // error
                exit(1);
            }
            if (psNist != psYans)
            {
                exit(1);
            }
            double psTable = table->GetChunkSuccessRate(wifiMode, txVector, snr, frameSizeBits);
            if (psTable < 0.0 || psTable > 1.0)
            {
                std::cout << psTable << std::endl;
                // error
                exit(1);
            }
            if (psTable != psYans)
            {
                exit(1);
            }
            dataset.Add(snrDb, psYans);
        }

        plot.AddDataset(dataset);
    }

    plot.SetTerminal("postscript eps color enh \"Times-BoldItalic\"");
    plot.SetLegend("SNR(dB)", "Frame Success Rate");
    plot.SetExtra("set xrange [-10:20]\n\
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
    plot.GenerateOutput(file);
    file.close();

    return 0;
}
