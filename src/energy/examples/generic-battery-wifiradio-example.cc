/*
 * Copyright (c) 2023 Tokushima University, Japan
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
 * Author: Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

/*
 *  Node 1 <-------------- distanceToRx ------------> Node2
 *  (SoC 89%)                                        (SoC 95%)
 *
 *  This example is based on the basic-energy-model-test created by He Wu.
 *  The objective is to demonstrate the use of a GenericBatteryModel with
 *  the WifiRadioEnergyModel. The WifiRadioEnergyModel was created to work
 *  specifically with the BasicEnergySource, therefore, the current example
 *  should be considered a prototype until WifiRadioEnergyModel can be
 *  revised and thoroughly tested with the GenericBatterySource.
 *
 *  In the example, 2 wifi nodes are created each with a GenericBatterySource
 *  (Li-Ion battery type) is created with 4 cells (2 series, 2 parallel).
 *  The simulation runs for 3600 secs. Tx, Rx and Idle consumption values
 *  have been exaggerated for demonstration purposes. At the end of the simulation,
 *  the State of Charge (Soc %) and remaining capacity in Jouls for each node is
 *  displayed.
 *
 */

#include <ns3/core-module.h>
#include <ns3/energy-module.h>
#include <ns3/internet-module.h>
#include <ns3/mobility-module.h>
#include <ns3/network-module.h>
#include <ns3/wifi-module.h>

#include <sstream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("GenericBatteryWifiRadioExample");

/**
 * Print a received packet
 *
 * \param from sender address
 * \return a string with the details of the packet: dst {IP, port}, time.
 */
inline std::string
PrintReceivedPacket(Address& from)
{
    InetSocketAddress iaddr = InetSocketAddress::ConvertFrom(from);

    std::ostringstream oss;
    oss << " Received one packet! Socket: " << iaddr.GetIpv4() << " port: " << iaddr.GetPort()
        << "\n";

    return oss.str();
}

/**
 * \param socket Pointer to socket.
 *
 * Packet receiving sink.
 */
void
ReceivePacket(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        if (packet->GetSize() > 0)
        {
            NS_LOG_DEBUG(PrintReceivedPacket(from));
        }
    }
}

/**
 * \param socket Pointer to socket.
 * \param pktSize Packet size.
 * \param n Pointer to node.
 * \param pktCount Number of packets to generate.
 * \param pktInterval Packet sending interval.
 *
 * Generate Traffic
 */
static void
GenerateTraffic(Ptr<Socket> socket,
                uint32_t pktSize,
                Ptr<Node> n,
                uint32_t pktCount,
                Time pktInterval)
{
    if (pktCount > 0)
    {
        socket->Send(Create<Packet>(pktSize));
        Simulator::Schedule(pktInterval,
                            &GenerateTraffic,
                            socket,
                            pktSize,
                            n,
                            pktCount - 1,
                            pktInterval);
    }
    else
    {
        socket->Close();
    }
}

/**
 * Trace function for remaining energy at node.
 *
 * \param oldValue Old value
 * \param remainingEnergy New value
 */
void
RemainingEnergy(double oldValue, double remainingEnergy)
{
    NS_LOG_DEBUG(" Remaining energy Node 1 = " << remainingEnergy << " J");
}

int
main(int argc, char* argv[])
{
    LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_FUNC));
    LogComponentEnable("GenericBatteryWifiRadioExample", LOG_LEVEL_DEBUG);

    std::string phyMode("DsssRate1Mbps");
    double rss = -80;          // dBm
    uint32_t packetSize = 200; // bytes
    bool verbose = false;

    // simulation parameters
    uint32_t numPackets = 10000; // number of packets to send
    double interval = 1;         // seconds
    double startTime = 0.0;      // seconds
    double distanceToRx = 100.0; // meters

    CommandLine cmd(__FILE__);
    cmd.AddValue("phyMode", "Wifi Phy mode", phyMode);
    cmd.AddValue("rss", "Intended primary RSS (dBm)", rss);
    cmd.AddValue("packetSize", "size of application packet sent (Bytes)", packetSize);
    cmd.AddValue("numPackets", "Total number of packets to send", numPackets);
    cmd.AddValue("startTime", "Simulation start time (seconds)", startTime);
    cmd.AddValue("distanceToRx", "X-Axis distance between nodes (meters)", distanceToRx);
    cmd.AddValue("verbose", "Turn on all device log components", verbose);
    cmd.Parse(argc, argv);

    Time interPacketInterval = Seconds(interval);

    Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold",
                       StringValue("2200"));
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("2200"));
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));

    NodeContainer nodeContainer;
    nodeContainer.Create(2);

    WifiHelper wifi;
    if (verbose)
    {
        WifiHelper::EnableLogComponents();
    }
    wifi.SetStandard(WIFI_STANDARD_80211b);

    ////////////////////////
    // Wifi PHY and MAC   //
    ////////////////////////

    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");

    Ptr<YansWifiChannel> wifiChannelPtr = wifiChannel.Create();
    wifiPhy.SetChannel(wifiChannelPtr);

    WifiMacHelper wifiMac;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue(phyMode),
                                 "ControlMode",
                                 StringValue(phyMode));

    wifiMac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodeContainer);

    //////////////////
    //   Mobility   //
    //////////////////

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(2 * distanceToRx, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodeContainer);

    //////////////////////
    //  Energy Model    //
    //////////////////////

    // Use a preset PANASONIC Li-Ion batteries arranged in a cell pack (2 series, 2 parallel)
    GenericBatteryModelHelper batteryHelper;
    EnergySourceContainer energySourceContainer =
        batteryHelper.Install(nodeContainer, PANASONIC_CGR18650DA_LION);
    batteryHelper.SetCellPack(energySourceContainer, 2, 2);

    Ptr<GenericBatteryModel> battery0 =
        DynamicCast<GenericBatteryModel>(energySourceContainer.Get(0));
    Ptr<GenericBatteryModel> battery1 =
        DynamicCast<GenericBatteryModel>(energySourceContainer.Get(1));

    // Energy consumption quantities have been exaggerated for
    // demonstration purposes, real consumption values are much smaller.
    WifiRadioEnergyModelHelper radioEnergyHelper;
    radioEnergyHelper.Set("TxCurrentA", DoubleValue(4.66));
    radioEnergyHelper.Set("RxCurrentA", DoubleValue(0.466));
    radioEnergyHelper.Set("IdleCurrentA", DoubleValue(0.466));
    DeviceEnergyModelContainer deviceModels =
        radioEnergyHelper.Install(devices, energySourceContainer);

    /////////////////////
    // Internet stack  //
    /////////////////////

    InternetStackHelper internet;
    internet.Install(nodeContainer);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i = ipv4.Assign(devices);

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> recvSink = Socket::CreateSocket(nodeContainer.Get(1), tid); // node 1, receiver
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 80);
    recvSink->Bind(local);
    recvSink->SetRecvCallback(MakeCallback(&ReceivePacket));

    Ptr<Socket> source = Socket::CreateSocket(nodeContainer.Get(0), tid); // node 0, sender
    InetSocketAddress remote = InetSocketAddress(Ipv4Address::GetBroadcast(), 80);
    source->SetAllowBroadcast(true);
    source->Connect(remote);

    /////////////////////
    // Trace Sources   //
    /////////////////////

    battery1->TraceConnectWithoutContext("RemainingEnergy", MakeCallback(&RemainingEnergy));

    Ptr<DeviceEnergyModel> radioConsumptionModel =
        battery1->FindDeviceEnergyModels("ns3::WifiRadioEnergyModel").Get(0);

    /////////////////////
    // Traffic Setup   //
    /////////////////////
    Simulator::Schedule(Seconds(startTime),
                        &GenerateTraffic,
                        source,
                        packetSize,
                        nodeContainer.Get(0),
                        numPackets,
                        interPacketInterval);

    Simulator::Stop(Seconds(3600));
    Simulator::Run();

    NS_LOG_DEBUG(" *Remaining Capacity * "
                 << "| Node 0: " << battery0->GetRemainingEnergy() << " J "
                 << "| Node 1: " << battery1->GetRemainingEnergy() << " J");
    NS_LOG_DEBUG(" *SoC * "
                 << "| Node 0: " << battery0->GetStateOfCharge() << " % "
                 << "| Node 1: " << battery1->GetStateOfCharge() << " % ");

    Simulator::Destroy();

    return 0;
}
