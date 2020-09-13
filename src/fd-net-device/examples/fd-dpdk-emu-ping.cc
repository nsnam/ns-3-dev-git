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

// Allow ns-3 to ping a real host somewhere, using DPDK emulation mode
//
//   +----------------------+
//   |          host        |
//   +----------------------+
//   |    ns-3 simulation   |
//   +----------------------+
//   |      ns-3 Node       |
//   |  +----------------+  |
//   |  |    ns-3 TCP    |  |
//   |  +----------------+  |
//   |  |    ns-3 IPv4   |  |
//   |  +----------------+  |
//   |  |  FdNetDevice   |  |
//   |  |       or       |  |
//   |  | DpdkNetDevice  |  |
//   |--+----------------+--+
//   |  |   'eth0' or    |  |
//   |  | '0000:00.1f.6' |  |
//   |  +----------------+  |
//   |          |           |
//   +----------|-----------+
//              |
//              |         +---------+
//              .---------| GW host |--- (Internet) -----
//                        +---------+
//
/// To use this example:
//  1) You need to decide on a physical device on your real system, and either
//     overwrite the hard-configured device address below (0000:00.1f.6) or pass
//     this device addresss in as a command-line argument.  This address can be
//     obtained by doing `lspci`.
//  2) (Optional) You can add your device Mac address to macClient.  This is
//     required if mac spoofing is not allowed in network.
//  3) You will need to assign an IP address to the ns-3 simulation node that
//     is consistent with the subnet that is active on the host device's link.
//     That is, you will have to assign an IP address to the ns-3 node as if
//     it were on your real subnet.  Search for "Ipv4Address localIp" and
//     replace the string "1.2.3.4" with a valid IP address.
//  4) You will need to configure a default route in the ns-3 node to tell it
//     how to get off of your subnet. One thing you could do is a
//     'netstat -rn' command and find the IP address of the default gateway
//     on your host.  Search for "Ipv4Address gateway" and replace the string
//     "1.2.3.4" string with the gateway IP address.
//

#include "ns3/abort.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/fd-net-device-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DpdkEmulationPingExample");

static void
PingRtt (std::string context, Time rtt)
{
  NS_LOG_UNCOND ("Received Response with RTT = " << rtt);
}

int
main (int argc, char *argv[])
{
  NS_LOG_INFO ("Dpdk Emulation Ping Example");

  std::string deviceName ("0000:00:1f.6");
  std::string macClient ("78:0c:b8:d8:e1:95");
  // ping a real host connected back-to-back through the ethernet interfaces
  std::string remote ("192.168.43.2");

  double samplingPeriod = 0.5; // s

  CommandLine cmd;
  cmd.AddValue ("deviceName", "Device address to use (Eg: 0000:00:1f.6). Use `lspci` to find this.", deviceName);
  cmd.AddValue ("remote", "Remote IP address (dotted decimal only please)", remote);
  cmd.AddValue ("macClient", "Mac address of client (Optional).", macClient);
  cmd.Parse (argc, argv);

  Ipv4Address remoteIp (remote.c_str ());
  Ipv4Address localIp ("192.168.43.1");
  NS_ABORT_MSG_IF (localIp == "1.2.3.4", "You must change the local IP address before running this example");

  Ipv4Mask localMask ("255.255.255.0");

  //
  // Since we are using a real piece of hardware we need to use the realtime
  // simulator.
  //
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));

  //
  // Since we are going to be talking to real-world machines, we need to enable
  // calculation of checksums in our protocols.
  //
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  //
  // In such a simple topology, the use of the helper API can be a hindrance
  // so we drop down into the low level API and do it manually.
  //
  // First we need a single node.
  //
  NS_LOG_INFO ("Create Node");
  Ptr<Node> node = CreateObject<Node> ();

  //
  // Create an emu device, allocate a MAC address and point the device to the
  // Linux device name.  The device needs a transmit queueing discipline so
  // create a droptail queue and give it to the device.  Finally, "install"
  // the device into the node.
  //
  // Do understand that the ns-3 allocated MAC address will be sent out over
  // your network since the emu net device will spoof it.  By default, this
  // address will have an Organizationally Unique Identifier (OUI) of zero.
  // The Internet Assigned Number Authority IANA
  //
  //  http://www.iana.org/assignments/ethernet-numbers
  //
  // reports that this OUI is unassigned, and so should not conflict with
  // real hardware on your net.  It may raise all kinds of red flags in a
  // real environment to have packets from a device with an obviously bogus
  // OUI flying around.  Be aware.
  //
  NS_LOG_INFO ("Create Device");
  EmuFdNetDeviceHelper emu;

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
  emu.SetDpdkMode (7, ealArgv);
  emu.SetDeviceName (deviceName);
  NetDeviceContainer devices = emu.Install (node);
  Ptr<NetDevice> device = devices.Get (0);
  device->SetAttribute ("Address", Mac48AddressValue (macClient.c_str ()));

  //Ptr<Queue> queue = CreateObject<DropTailQueue> ();
  //device->SetQueue (queue);
  //node->AddDevice (device);

  //
  // Add a default internet stack to the node.  This gets us the ns-3 versions
  // of ARP, IPv4, ICMP, UDP and TCP.
  //
  NS_LOG_INFO ("Add Internet Stack");
  InternetStackHelper internetStackHelper;
  internetStackHelper.Install (node);

  NS_LOG_INFO ("Create IPv4 Interface");
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  uint32_t interface = ipv4->AddInterface (device);
  Ipv4InterfaceAddress address = Ipv4InterfaceAddress (localIp, localMask);
  ipv4->AddAddress (interface, address);
  ipv4->SetMetric (interface, 1);
  ipv4->SetUp (interface);

  //
  // When the ping application sends its ICMP packet, it will happily send it
  // down the ns-3 protocol stack.  We set the IP address of the destination
  // to the address corresponding to example.com above.  This address is off
  // our local network so we have got to provide some kind of default route
  // to ns-3 to be able to get that ICMP packet forwarded off of our network.
  //
  // You have got to provide an IP address of a real host that you can send
  // real packets to and have them forwarded off of your local network.  One
  // thing you could do is a 'netstat -rn' command and find the IP address of
  // the default gateway on your host and add it below, replacing the
  // "1.2.3.4" string.
  //
  Ipv4Address gateway ("192.168.43.2");
  NS_ABORT_MSG_IF (gateway == "1.2.3.4", "You must change the gateway IP address before running this example");

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> staticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4);
  staticRouting->SetDefaultRoute (gateway, interface);

  //
  // Create the ping application.  This application knows how to send
  // ICMP echo requests.  Setting up the packet sink manually is a bit
  // of a hassle and since there is no law that says we cannot mix the
  // helper API with the low level API, let's just use the helper.
  //
  NS_LOG_INFO ("Create V4Ping Appliation");
  Ptr<V4Ping> app = CreateObject<V4Ping> ();
  app->SetAttribute ("Remote", Ipv4AddressValue (remoteIp));
  app->SetAttribute ("Verbose", BooleanValue (true) );
  app->SetAttribute ("Interval", TimeValue (Seconds (samplingPeriod)));
  node->AddApplication (app);
  app->SetStartTime (Seconds (1.0));
  app->SetStopTime (Seconds (21.0));

  //
  // Give the application a name.  This makes life much easier when constructing
  // config paths.
  //
  Names::Add ("app", app);

  //
  // Hook a trace to print something when the response comes back.
  //
  Config::Connect ("/Names/app/Rtt", MakeCallback (&PingRtt));

  //
  // Enable a promiscuous pcap trace to see what is coming and going on our device.
  //
  emu.EnablePcap ("dpdk-emu-ping", device, true);

  //
  // Now, do the actual emulation.
  //
  NS_LOG_INFO ("Run Emulation.");
  Simulator::Stop (Seconds (22.0));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
