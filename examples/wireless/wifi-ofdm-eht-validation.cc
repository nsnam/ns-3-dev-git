/*
 * Copyright (c) 2021 DERONNE SOFTWARE ENGINEERING
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

// This example is used to validate NIST and YANS error rate models for EHT rates.
//
// It outputs plots of the Frame Success Rate versus the Signal-to-noise ratio for
// Nist, Yans and Table-based error rate models and for every HT MCS value.

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
    uint32_t FrameSize = 1500; // bytes
    std::ofstream yansfile("yans-frame-success-rate-be.plt");
    std::ofstream nistfile("nist-frame-success-rate-be.plt");
    std::ofstream tablefile("table-frame-success-rate-be.plt");

    const std::vector<std::string> modes{
        "EhtMcs0",
        "EhtMcs1",
        "EhtMcs2",
        "EhtMcs3",
        "EhtMcs4",
        "EhtMcs5",
        "EhtMcs6",
        "EhtMcs7",
        "EhtMcs8",
        "EhtMcs9",
        "EhtMcs10",
        "EhtMcs11",
        "EhtMcs12",
        "EhtMcs13",
    };

    CommandLine cmd(__FILE__);
    cmd.AddValue("FrameSize", "The frame size", FrameSize);
    cmd.Parse(argc, argv);

    Gnuplot yansplot = Gnuplot("yans-frame-success-rate-be.eps");
    Gnuplot nistplot = Gnuplot("nist-frame-success-rate-be.eps");
    Gnuplot tableplot = Gnuplot("table-frame-success-rate-be.eps");

    Ptr<YansErrorRateModel> yans = CreateObject<YansErrorRateModel>();
    Ptr<NistErrorRateModel> nist = CreateObject<NistErrorRateModel>();
    Ptr<TableBasedErrorRateModel> table = CreateObject<TableBasedErrorRateModel>();
    WifiTxVector txVector;

    for (uint32_t i = 0; i < modes.size(); i++)
    {
        std::cout << modes[i] << std::endl;
        Gnuplot2dDataset yansdataset(modes[i]);
        Gnuplot2dDataset nistdataset(modes[i]);
        Gnuplot2dDataset tabledataset(modes[i]);
        txVector.SetMode(modes[i]);

        for (double snr = -5.0; snr <= 55.0; snr += 0.1)
        {
            double ps = yans->GetChunkSuccessRate(WifiMode(modes[i]),
                                                  txVector,
                                                  std::pow(10.0, snr / 10.0),
                                                  FrameSize * 8);
            if (ps < 0.0 || ps > 1.0)
            {
                // error
                exit(1);
            }
            yansdataset.Add(snr, ps);

            ps = nist->GetChunkSuccessRate(WifiMode(modes[i]),
                                           txVector,
                                           std::pow(10.0, snr / 10.0),
                                           FrameSize * 8);
            if (ps < 0.0 || ps > 1.0)
            {
                // error
                exit(1);
            }
            nistdataset.Add(snr, ps);

            ps = table->GetChunkSuccessRate(WifiMode(modes[i]),
                                            txVector,
                                            std::pow(10.0, snr / 10.0),
                                            FrameSize * 8);
            if (ps < 0.0 || ps > 1.0)
            {
                // error
                exit(1);
            }
            tabledataset.Add(snr, ps);
        }

        yansplot.AddDataset(yansdataset);
        nistplot.AddDataset(nistdataset);
        tableplot.AddDataset(tabledataset);
    }

    std::stringstream plotExtra;
    plotExtra << "set xrange [-5:55]\n\
set yrange [0:1]\n";

    const std::vector<std::string> colors{
        "green",
        "blue",
        "red",
        "black",
        "orange",
        "purple",
        "yellow",
        "pink",
        "grey",
        "magenta",
        "brown",
        "turquoise",
        "olive",
        "beige",
    };

    NS_ASSERT_MSG(colors.size() == modes.size(), "Colors and modes vectors have different sizes");

    for (std::size_t i = 0; i < modes.size(); i++)
    {
        plotExtra << "set style line " << (i + 1) << " linewidth 5 linecolor rgb \"" << colors[i]
                  << "\" \n";
    }
    plotExtra << "set style increment user";

    yansplot.SetTerminal("postscript eps color enh \"Times-BoldItalic\"");
    yansplot.SetLegend("SNR(dB)", "Frame Success Rate");
    yansplot.SetExtra(plotExtra.str());
    yansplot.GenerateOutput(yansfile);
    yansfile.close();

    nistplot.SetTerminal("postscript eps color enh \"Times-BoldItalic\"");
    nistplot.SetLegend("SNR(dB)", "Frame Success Rate");
    nistplot.SetExtra(plotExtra.str());
    nistplot.GenerateOutput(nistfile);
    nistfile.close();

    tableplot.SetTerminal("postscript eps color enh \"Times-BoldItalic\"");
    tableplot.SetLegend("SNR(dB)", "Frame Success Rate");
    tableplot.SetExtra(plotExtra.str());
    tableplot.GenerateOutput(tablefile);
    tablefile.close();

    return 0;
}
