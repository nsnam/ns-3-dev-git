/*
 * Copyright (c) 2005,2006,2007 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/athstats-helper.h"
#include "ns3/boolean.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-socket-address.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

using namespace ns3;

/// True for verbose output.
static bool g_verbose = true;

/**
 * MAC-level TX trace.
 *
 * \param context The context.
 * \param p The packet.
 */
void
DevTxTrace(std::string context, Ptr<const Packet> p)
{
    if (g_verbose)
    {
        std::cout << " TX p: " << *p << std::endl;
    }
}

/**
 * MAC-level RX trace.
 *
 * \param context The context.
 * \param p The packet.
 */
void
DevRxTrace(std::string context, Ptr<const Packet> p)
{
    if (g_verbose)
    {
        std::cout << " RX p: " << *p << std::endl;
    }
}

/**
 * PHY-level RX OK trace
 *
 * \param context The context.
 * \param packet The packet.
 * \param snr The SNR.
 * \param mode The wifi mode.
 * \param preamble The preamble.
 */
void
PhyRxOkTrace(std::string context,
             Ptr<const Packet> packet,
             double snr,
             WifiMode mode,
             WifiPreamble preamble)
{
    if (g_verbose)
    {
        std::cout << "PHYRXOK mode=" << mode << " snr=" << snr << " " << *packet << std::endl;
    }
}

/**
 * PHY-level RX error trace
 *
 * \param context The context.
 * \param packet The packet.
 * \param snr The SNR.
 */
void
PhyRxErrorTrace(std::string context, Ptr<const Packet> packet, double snr)
{
    if (g_verbose)
    {
        std::cout << "PHYRXERROR snr=" << snr << " " << *packet << std::endl;
    }
}

/**
 * PHY-level TX trace.
 *
 * \param context The context.
 * \param packet The packet.
 * \param mode The wifi mode.
 * \param preamble The preamble.
 * \param txPower The TX power.
 */
void
PhyTxTrace(std::string context,
           Ptr<const Packet> packet,
           WifiMode mode,
           WifiPreamble preamble,
           uint8_t txPower)
{
    if (g_verbose)
    {
        std::cout << "PHYTX mode=" << mode << " " << *packet << std::endl;
    }
}

/**
 * PHY state trace.
 *
 * \param context The context.
 * \param start Start time of the state.
 * \param duration Duration of the state.
 * \param state The state.
 */
void
PhyStateTrace(std::string context, Time start, Time duration, WifiPhyState state)
{
    if (g_verbose)
    {
        std::cout << " state=" << state << " start=" << start << " duration=" << duration
                  << std::endl;
    }
}

/**
 * Move a node position by 5m on the x axis every second, up to 210m.
 *
 * \param node The node.
 */
static void
AdvancePosition(Ptr<Node> node)
{
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    Vector pos = mobility->GetPosition();
    pos.x += 5.0;
    if (pos.x >= 210.0)
    {
        return;
    }
    mobility->SetPosition(pos);

    Simulator::Schedule(Seconds(1.0), &AdvancePosition, node);
}

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "Print trace information if true", g_verbose);
    cmd.Parse(argc, argv);

    Packet::EnablePrinting();

    WifiHelper wifi;
    MobilityHelper mobility;
    NodeContainer stas;
    NodeContainer ap;
    NetDeviceContainer staDevs;
    PacketSocketHelper packetSocket;

    stas.Create(2);
    ap.Create(1);

    // give packet socket powers to nodes.
    packetSocket.Install(stas);
    packetSocket.Install(ap);

    WifiMacHelper wifiMac;
    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiPhy.SetChannel(wifiChannel.Create());
    Ssid ssid = Ssid("wifi-default");
    // setup stas.
    wifiMac.SetType("ns3::StaWifiMac",
                    "ActiveProbing",
                    BooleanValue(true),
                    "Ssid",
                    SsidValue(ssid));
    staDevs = wifi.Install(wifiPhy, wifiMac, stas);
    // setup ap.
    wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    wifi.Install(wifiPhy, wifiMac, ap);

    // mobility.
    mobility.Install(stas);
    mobility.Install(ap);

    Simulator::Schedule(Seconds(1.0), &AdvancePosition, ap.Get(0));

    PacketSocketAddress socket;
    socket.SetSingleDevice(staDevs.Get(0)->GetIfIndex());
    socket.SetPhysicalAddress(staDevs.Get(1)->GetAddress());
    socket.SetProtocol(1);

    OnOffHelper onoff("ns3::PacketSocketFactory", Address(socket));
    onoff.SetConstantRate(DataRate("500kb/s"));

    ApplicationContainer apps = onoff.Install(stas.Get(0));
    apps.Start(Seconds(0.5));
    apps.Stop(Seconds(43.0));

    Simulator::Stop(Seconds(44.0));

    Config::Connect("/NodeList/*/DeviceList/*/Mac/MacTx", MakeCallback(&DevTxTrace));
    Config::Connect("/NodeList/*/DeviceList/*/Mac/MacRx", MakeCallback(&DevRxTrace));
    Config::Connect("/NodeList/*/DeviceList/*/Phy/State/RxOk", MakeCallback(&PhyRxOkTrace));
    Config::Connect("/NodeList/*/DeviceList/*/Phy/State/RxError", MakeCallback(&PhyRxErrorTrace));
    Config::Connect("/NodeList/*/DeviceList/*/Phy/State/Tx", MakeCallback(&PhyTxTrace));
    Config::Connect("/NodeList/*/DeviceList/*/Phy/State/State", MakeCallback(&PhyStateTrace));

    AthstatsHelper athstats;
    athstats.EnableAthstats("athstats-sta", stas);
    athstats.EnableAthstats("athstats-ap", ap);

    Simulator::Run();

    Simulator::Destroy();

    return 0;
}
