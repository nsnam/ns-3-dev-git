/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 NITK Surathkal
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
 * Author: Harsh Patel <thadodaharsh10@gmail.com>
 *         Hrishikesh Hiraskar <hrishihiraskar@gmail.com>
 *         Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

// +----------------------+     +-----------------------+
// |      client host     |     |      server host      |
// +----------------------+     +-----------------------+
// |     ns-3 Node 0      |     |      ns-3 Node 1      |
// |  +----------------+  |     |   +----------------+  |
// |  |    ns-3 TCP    |  |     |   |    ns-3 TCP    |  |
// |  +----------------+  |     |   +----------------+  |
// |  |    ns-3 IPv4   |  |     |   |    ns-3 IPv4   |  |
// |  +----------------+  |     |   +----------------+  |
// |  |  FdNetDevice   |  |     |   |  FdNetDevice   |  |
// |  |      or        |  |     |   |      or        |  |
// |  | DpdkNetDevice  |  |     |   | DpdkNetDevice  |  |
// |  |    10.1.1.1    |  |     |   |    10.1.1.2    |  |
// |  +----------------+  |     |   +----------------+  |
// |  |   raw socket   |  |     |   |   raw socket   |  |
// |  |      or        |  |     |   |      or        |  |
// |  |      EAL       |  |     |   |      EAL       |  |
// |  +----------------+  |     |   +----------------+  |
// |    |    eth0    |    |     |     |    eth0    |    |
// |    |     or     |    |     |     |     or     |    |
// |    | 0000:00.1f |    |     |     | 0000:00.1f |    |
// +----+------------+----+     +-----+------------+----+
//
//         10.1.1.11                     10.1.1.12
//
//             |                            |
//             +----------------------------+
//
// This example is aimed at measuring the throughput of the FdNetDevice or
// DpdkNetDevice (if dpdkMode is true) when using the EmuFdNetDeviceHelper.
// This is achieved by saturating the channel with TCP traffic. Then the
// throughput can be obtained from the generated .pcap files.
//
// To run this example you will need two hosts (client & server).
// Steps to run the experiment:
//
// 1 - Connect the 2 computers with an Ethernet cable.
//
// 2 - Set the IP addresses on both Ethernet devices.
//
// client machine: $ sudo ip addr add dev eth0 10.1.1.11/24
// server machine: $ sudo ip addr add dev eth0 10.1.1.12/24
//
// 3 - The NetDevice will be one of FdNetDevice or DpdkNetDevice based on the
//     the value of dpdkMode parameter.
//
// 4 - In case of FdNetDevice, set both Ethernet devices to promiscuous mode.
//
// both machines: $ sudo ip link set eth0 promisc on
//
// 5 - In case of FdNetDevice, give root suid to the raw socket creator binary.
//     If the --enable-sudo option was used to configure ns-3 with waf, then the following
//     step will not be necessary.
//
// both hosts: $ sudo chown root.root build/src/fd-net-device/ns3-dev-raw-sock-creator
// both hosts: $ sudo chmod 4755 build/src/fd-net-device/ns3-dev-raw-sock-creator
//
// 6 - In case of DpdkNetDevice, use device address instead of device name
//
// For example: 0000:00:1f.6 (can be obtained by lspci)
//
// 7 - Run the server side:
//
// server host: $ ./waf --run="fd-emu-onoff --serverMode=1 --dpdkMode=true"
//
// 8 - Run the client side:
//
// client host: $ ./waf --run="fd-emu-onoff --dpdkMode=true"
//
// NOTE: You can also use iperf as client or server.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/fd-net-device-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("EmuFdNetDeviceSaturationExample");

int main(int argc, char *argv[])
{
    uint16_t sinkPort = 8000;
    uint32_t packetSize = 190; // bytes
    std::string dataRate("950Mb/s");
    bool serverMode = false;

    std::string deviceName("eno1");
    std::string client("10.0.1.11");
    std::string server("10.0.1.22");
    std::string netmask("255.255.255.0");
    std::string macClient("00:00:00:00:00:01");
    std::string macServer("00:00:00:00:00:02");

    std::string transportProt = "Udp";
    std::string socketType;

    bool dpdkMode = true;
    bool ping = false;
    int dpdkTimeout = 2000;

    double samplingPeriod = 0.5; // s

    CommandLine cmd;
    cmd.AddValue("deviceName", "Device name", deviceName);
    cmd.AddValue("client", "Local IP address (dotted decimal only please)", client);
    cmd.AddValue("server", "Remote IP address (dotted decimal only please)", server);
    cmd.AddValue("localmask", "Local mask address (dotted decimal only please)", netmask);
    cmd.AddValue("serverMode", "1:true, 0:false, default client", serverMode);
    cmd.AddValue("macClient", "Mac Address for Client. Default : 00:00:00:00:00:01", macClient);
    cmd.AddValue("macServer", "Mac Address for Server. Default : 00:00:00:00:00:02", macServer);
    cmd.AddValue("dataRate", "Data rate defaults to 1000Mb/s", dataRate);
    cmd.AddValue("transportPort", "Transport protocol to use: Tcp, Udp", transportProt);
    cmd.AddValue("dpdkMode", "Enable the netmap emulation mode", dpdkMode);
    cmd.AddValue("dpdkTimeout", "Tx Timeout to use in dpdkMode. (in microseconds)", dpdkTimeout);
    cmd.AddValue("ping", "Enable server ping client side", ping);
    cmd.AddValue("packetSize", "Size of the packet (without header) in bytes.", packetSize);
    cmd.Parse(argc, argv);

    Ipv4Address remoteIp;
    Ipv4Address localIp;
    Mac48AddressValue localMac;

    if (serverMode)
    {
        remoteIp = Ipv4Address(client.c_str());
        localIp = Ipv4Address(server.c_str());
        localMac = Mac48AddressValue(macServer.c_str());
    }
    else
    {
        remoteIp = Ipv4Address(server.c_str());
        localIp = Ipv4Address(client.c_str());
        localMac = Mac48AddressValue(macClient.c_str());
    }

    if (transportProt.compare("Tcp") == 0)
    {
        socketType = "ns3::TcpSocketFactory";
    }
    else
    {
        socketType = "ns3::UdpSocketFactory";
    }

    Ipv4Mask localMask(netmask.c_str());

    GlobalValue::Bind("SimulatorImplementationType", StringValue("ns3::RealtimeSimulatorImpl"));

    GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));

    Config::SetDefault("ns3::TcpSocketBase::Sack", BooleanValue(true));

    NS_LOG_INFO("Create Node");
    Ptr<Node> node = CreateObject<Node>();

    NS_LOG_INFO("Create Device");
    EmuFdNetDeviceHelper emu;

    // set the dpdk emulation mode
    if (dpdkMode)
    {
        // set the dpdk emulation mode
        char **ealArgv = new char*[20];
        // arg[0] is program name (optional)
        ealArgv[0] = new char[20];
        strcpy (ealArgv[0], "");
        // logical core usage
        ealArgv[1] = new char[20];
        strcpy (ealArgv[1], "-l");
        // Use core 0 and 1
        ealArgv[2] = new char[20];
        strcpy (ealArgv[2], "0,1");
        // Load library
        ealArgv[3] = new char[20];
        strcpy (ealArgv[3], "-d");
        // Use e1000 driver library
        ealArgv[4] = new char[20];
        strcpy (ealArgv[4], "librte_pmd_e1000.so");
        // Load library
        ealArgv[5] = new char[20];
        strcpy (ealArgv[5], "-d");
        // Use mempool ring library
        ealArgv[6] = new char[50];
        strcpy (ealArgv[6], "librte_mempool_ring.so");
        emu.SetDpdkMode(7, ealArgv);
    }

    emu.SetDeviceName(deviceName);
    NetDeviceContainer devices = emu.Install(node);
    Ptr<NetDevice> device = devices.Get(0);
    device->SetAttribute("Address", localMac);
    if (dpdkMode)
      {
        device->SetAttribute("TxTimeout", UintegerValue (dpdkTimeout) );
      }

    NS_LOG_INFO("Add Internet Stack");
    InternetStackHelper internetStackHelper;
    internetStackHelper.SetIpv4StackInstall(true);
    internetStackHelper.Install(node);

    NS_LOG_INFO("Create IPv4 Interface");
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    uint32_t interface = ipv4->AddInterface(device);
    Ipv4InterfaceAddress address = Ipv4InterfaceAddress(localIp, localMask);
    ipv4->AddAddress(interface, address);
    ipv4->SetMetric(interface, 1);
    ipv4->SetUp(interface);

    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(packetSize));

    if (serverMode)
    {
        Address sinkLocalAddress(InetSocketAddress(localIp, sinkPort));
        PacketSinkHelper sinkHelper(socketType, sinkLocalAddress);
        ApplicationContainer sinkApp = sinkHelper.Install(node);
        sinkApp.Start(Seconds(1));
        sinkApp.Stop(Seconds(30.0));

        emu.EnablePcap("fd-server", device);
    }
    else
    {
        // add traffic generator
        AddressValue remoteAddress(InetSocketAddress(remoteIp, sinkPort));
        OnOffHelper onoff(socketType, Address());
        onoff.SetAttribute("Remote", remoteAddress);
        onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        onoff.SetAttribute("DataRate", DataRateValue(dataRate));
        onoff.SetAttribute("PacketSize", UintegerValue(packetSize));

        ApplicationContainer clientApps = onoff.Install(node);
        clientApps.Start(Seconds(6.0));
        clientApps.Stop(Seconds(106.0));

        if (ping)
        {
            printf("Adding ping app\n");
            // add ping application
            Ptr<V4Ping> app = CreateObject<V4Ping>();
            app->SetAttribute("Remote", Ipv4AddressValue(remoteIp));
            app->SetAttribute("Verbose", BooleanValue(true));
            app->SetAttribute("Interval", TimeValue(Seconds(samplingPeriod)));
            node->AddApplication(app);
            app->SetStartTime(Seconds(6.0));
            app->SetStopTime(Seconds(106.0));
        }

        emu.EnablePcap("fd-client", device);
    }

    Simulator::Stop(Seconds(120));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
