/*
 * Copyright (c) 2022 Tokushima University, Japan
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
 * Authors: Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 *          Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

/*
   This program produces a gnuplot file that plots the theoretical and experimental packet error
   rate (PER) as a function of receive signal for the 802.15.4 model. As described by the standard,
   the PER is calculated with the transmission of frames with a PSDU of 20 bytes. This is equivalent
   to an MPDU = MAC header (11 bytes) + FCS (2 bytes) +  payload (MSDU 7 bytes). In the experimental
   test, 1000 frames are transmitted for each Rx signal ranging from -130 dBm to -100 dBm with
   increments of 0.01 dBm. The point before PER is < 1 % is the device receive sensitivity.
   Theoretical and experimental Rx sensitivity is printed at the end of the end and a plot is
   generated.

   Example usage:

   ./ns3 run "lr-wpan-per-plot --rxSensitivity=-92"

*/

#include <ns3/core-module.h>
#include <ns3/gnuplot.h>
#include <ns3/lr-wpan-module.h>
#include <ns3/mobility-module.h>
#include <ns3/network-module.h>
#include <ns3/propagation-module.h>
#include <ns3/spectrum-module.h>

using namespace ns3;

uint32_t g_packetsReceived = 0; //!< number of packets received

NS_LOG_COMPONENT_DEFINE("LrWpanErrorDistancePlot");

/**
 * Function called when a Data indication is invoked
 * \param params MCPS data indication parameters
 * \param p packet
 */
void
PacketReceivedCallback(McpsDataIndicationParams params, Ptr<Packet> p)
{
    g_packetsReceived++;
}

int
main(int argc, char* argv[])
{
    LogComponentEnableAll(LogLevel(LOG_PREFIX_FUNC | LOG_PREFIX_NODE | LOG_PREFIX_TIME));

    std::ostringstream os;
    std::ofstream perfile("802.15.4-per-vs-rxSignal.plt");

    double minRxSignal = -111; // dBm
    double maxRxSignal = -82;  // dBm
    double increment = 0.01;
    int maxPackets = 1000;
    int packetSize = 7; // bytes (MPDU payload)
    double txPower = 0; // dBm
    uint32_t channelNumber = 11;
    double rxSensitivity = -106.58; // dBm

    CommandLine cmd(__FILE__);

    cmd.AddValue("txPower", "transmit power (dBm)", txPower);
    cmd.AddValue("packetSize", "packet (MSDU) size (bytes)", packetSize);
    cmd.AddValue("channelNumber", "channel number", channelNumber);
    cmd.AddValue("rxSensitivity", "the rx sensitivity (dBm)", rxSensitivity);
    cmd.Parse(argc, argv);

    Gnuplot perplot = Gnuplot("802.15.4-per-vs-rxSignal.eps");
    Gnuplot2dDataset perdatasetExperimental("Experimental");
    Gnuplot2dDataset perdatasetTheoretical("Theoretical");

    Ptr<Node> n0 = CreateObject<Node>();
    Ptr<Node> n1 = CreateObject<Node>();
    Ptr<LrWpanNetDevice> dev0 = CreateObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev1 = CreateObject<LrWpanNetDevice>();

    dev0->SetAddress(Mac16Address("00:01"));
    dev1->SetAddress(Mac16Address("00:02"));
    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<FixedRssLossModel> propModel = CreateObject<FixedRssLossModel>();

    channel->AddPropagationLossModel(propModel);
    dev0->SetChannel(channel);
    dev1->SetChannel(channel);
    n0->AddDevice(dev0);
    n1->AddDevice(dev1);
    Ptr<ConstantPositionMobilityModel> mob0 = CreateObject<ConstantPositionMobilityModel>();
    dev0->GetPhy()->SetMobility(mob0);
    Ptr<ConstantPositionMobilityModel> mob1 = CreateObject<ConstantPositionMobilityModel>();
    dev1->GetPhy()->SetMobility(mob1);
    mob0->SetPosition(Vector(0, 0, 0));
    mob1->SetPosition(Vector(0, 0, 0));

    LrWpanSpectrumValueHelper svh;
    Ptr<SpectrumValue> psd = svh.CreateTxPowerSpectralDensity(txPower, channelNumber);
    dev0->GetPhy()->SetTxPowerSpectralDensity(psd);

    // Set Rx sensitivity of the receiving device
    dev1->GetPhy()->SetRxSensitivity(rxSensitivity);

    McpsDataIndicationCallback cb0;
    cb0 = MakeCallback(&PacketReceivedCallback);
    dev1->GetMac()->SetMcpsDataIndicationCallback(cb0);

    //////////////////////////////////
    // Experimental  PER v.s Signal //
    //////////////////////////////////

    double per = 1;
    double sensitivityExp = 0;
    bool sensThreshold = true;

    for (double j = minRxSignal; j < maxRxSignal; j += increment)
    {
        propModel->SetRss(j);
        if (sensThreshold)
        {
            sensitivityExp = j;
        }

        for (int i = 0; i < maxPackets; i++)
        {
            McpsDataRequestParams params;
            params.m_srcAddrMode = SHORT_ADDR;
            params.m_dstAddrMode = SHORT_ADDR;
            params.m_dstPanId = 0;
            params.m_dstAddr = Mac16Address("00:02");
            params.m_msduHandle = 0;
            params.m_txOptions = 0;
            Ptr<Packet> p;
            p = Create<Packet>(packetSize);
            Simulator::Schedule(Seconds(i), &LrWpanMac::McpsDataRequest, dev0->GetMac(), params, p);
        }

        Simulator::Run();

        per = (static_cast<double>(maxPackets - g_packetsReceived) / maxPackets) * 100;

        std::cout << "Experimental Test || Signal: " << j << " dBm | Received " << g_packetsReceived
                  << " pkts"
                  << "/" << maxPackets << " | PER " << per << " %\n";

        if (per <= 1 && sensThreshold)
        {
            sensThreshold = false;
        }

        perdatasetExperimental.Add(j, per);
        g_packetsReceived = 0;
    }

    /////////////////////////////////
    // Theoretical PER v.s. Signal //
    /////////////////////////////////

    Ptr<LrWpanErrorModel> lrWpanError = CreateObject<LrWpanErrorModel>();
    LrWpanSpectrumValueHelper psdHelper;

    // Calculate the noise that accounts for both thermal noise (floor noise) and
    // imperfections on the chip or lost before reaching the demodulator.
    // In O-QPSK 250kbps, the point where PER is <= 1% without
    // additional noise is -106.58 dBm (Noise Factor = 1)
    double maxRxSensitivityW = (pow(10.0, -106.58 / 10.0) / 1000.0);
    long double noiseFactor = (pow(10.0, rxSensitivity / 10.0) / 1000.0) / maxRxSensitivityW;

    psdHelper.SetNoiseFactor(noiseFactor);
    Ptr<const SpectrumValue> noisePsd = psdHelper.CreateNoisePowerSpectralDensity(11);
    double noise = LrWpanSpectrumValueHelper::TotalAvgPower(noisePsd, 11);

    double signal = 0;
    double sensitivityTheo = 0;
    double perTheoretical = 0;
    double snr = 0;
    sensThreshold = true;

    for (double j = minRxSignal; j < maxRxSignal; j += increment)
    {
        if (sensThreshold)
        {
            sensitivityTheo = j;
        }

        signal = pow(10.0, j / 10.0) / 1000.0; // signal in Watts
        snr = signal / noise;

        // According to the standard, Packet error rate should be obtained
        // using a PSDU of 20 bytes using
        // the equation PER = 1 - (1 - BER)^nbits
        perTheoretical = (1.0 - lrWpanError->GetChunkSuccessRate(snr, (packetSize + 13) * 8)) * 100;
        std::cout << "Theoretical Test || Signal: " << j << " dBm | SNR: " << snr << "| PER "
                  << perTheoretical << " % \n";

        if (perTheoretical <= 1 && sensThreshold)
        {
            sensThreshold = false;
        }

        perdatasetTheoretical.Add(j, perTheoretical);
    }

    std::cout << "_____________________________________________________________________________\n";
    std::cout << "Experimental Test || Receiving with a current sensitivity of " << sensitivityExp
              << " dBm\n";
    std::cout << "Theoretical Test  || Receiving with a current sensitivity of " << sensitivityTheo
              << " dBm\n";
    std::cout << "Gnu plot generated.";

    os << "Pkt Payload (MSDU) size = " << packetSize << " bytes | "
       << "Tx power = " << txPower << " dBm | "
       << "Rx Sensitivity (Theo) = " << sensitivityTheo << " dBm";

    perplot.AddDataset(perdatasetExperimental);
    perplot.AddDataset(perdatasetTheoretical);

    perplot.SetTitle(os.str());
    perplot.SetTerminal("postscript eps color enh \"Times-BoldItalic\"");
    perplot.SetLegend("Rx signal (dBm)", "Packet Error Rate (%)");
    perplot.SetExtra("set xrange [-110:-82]\n\
                      set logscale y\n\
                      set yrange [0.000000000001:120]\n\
                      set xtics 2\n\
                      set grid\n\
                      set style line 1 linewidth 5\n\
                      set style line 2 linewidth 3\n\
                      set style increment user\n\
                      set arrow from -110,1 to -82,1 nohead lc 'web-blue' front");
    perplot.GenerateOutput(perfile);
    perfile.close();

    Simulator::Destroy();
    return 0;
}
