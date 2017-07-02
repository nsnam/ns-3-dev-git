/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 NITK Surathkal
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
 * Authors: Ankit Deepak <adadeepak8@gmail.com>
 *          Deepti Rajagopal <deeptir96@gmail.com>
 *
 */

#include "ns3/data-rate.h"
#include "ns3/simple-net-device.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/dhcp-client.h"
#include "ns3/dhcp-server.h"
#include "ns3/dhcp-helper.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * \ingroup dhcp
 * \defgroup dhcp-test DHCP module tests
 */


/**
 * \ingroup dhcp-test
 * \ingroup tests
 *
 * \brief DHCP basic tests
 */
class DhcpTestCase : public TestCase
{
public:
  DhcpTestCase ();
  virtual ~DhcpTestCase ();
  /**
   * Triggered by an address lease on a client.
   * \param newAddress The leased address.
   */
  void LeaseObtained (const Ipv4Address& newAddress);
private:
  virtual void DoRun (void);
  Ipv4Address m_leasedAddress; //!< Address given to the node
};

DhcpTestCase::DhcpTestCase ()
  : TestCase ("Dhcp test case ")
{
}

DhcpTestCase::~DhcpTestCase ()
{
}

void
DhcpTestCase::LeaseObtained (const Ipv4Address& newAddress)
{
  m_leasedAddress = newAddress;
}

void
DhcpTestCase::DoRun (void)
{
  /*Set up devices*/
  NodeContainer nodes;
  NodeContainer routers;
  nodes.Create (1);
  routers.Create (1);

  NodeContainer net (nodes, routers);

  SimpleNetDeviceHelper simpleNetDevice;
  simpleNetDevice.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  simpleNetDevice.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("5Mbps")));
  NetDeviceContainer devNet = simpleNetDevice.Install (net);

  InternetStackHelper tcpip;
  tcpip.Install (nodes);
  tcpip.Install (routers);

  Ptr<Ipv4> ipv4MN = net.Get (0)->GetObject<Ipv4> ();
  uint32_t ifIndex = ipv4MN->AddInterface (devNet.Get (0));
  ipv4MN->AddAddress (ifIndex, Ipv4InterfaceAddress (Ipv4Address ("0.0.0.0"), Ipv4Mask ("/32")));
  ipv4MN->SetForwarding (ifIndex, true);
  ipv4MN->SetUp (ifIndex);

  // Setup IPv4 addresses and forwarding
  Ptr<Ipv4> ipv4Router = net.Get (1)->GetObject<Ipv4> ();
  ifIndex = ipv4Router->AddInterface (devNet.Get (1));
  ipv4Router->AddAddress (ifIndex, Ipv4InterfaceAddress (Ipv4Address ("172.30.0.1"), Ipv4Mask ("/24")));
  ipv4Router->SetForwarding (ifIndex, true);
  ipv4Router->SetUp (ifIndex);

  DhcpServerHelper dhcpServerHelper (Ipv4Address ("172.30.0.0"), Ipv4Mask ("/24"),
                                     Ipv4Address ("172.30.0.10"), Ipv4Address ("172.30.0.100"));
  ApplicationContainer dhcpServerApps = dhcpServerHelper.Install (routers.Get (0));
  dhcpServerApps.Start (Seconds (1.0));
  dhcpServerApps.Stop (Seconds (20.0));

  DhcpClientHelper dhcpClient;
  NetDeviceContainer dhcpClientNetDevs;
  dhcpClientNetDevs.Add (devNet.Get (0));

  ApplicationContainer dhcpClients = dhcpClient.Install (dhcpClientNetDevs);
  dhcpClients.Start (Seconds (1.0));
  dhcpClients.Stop (Seconds (20.0));


  dhcpClients.Get(0)->TraceConnectWithoutContext ("NewLease", MakeCallback(&DhcpTestCase::LeaseObtained, this));

  Simulator::Stop (Seconds (21.0));

  Simulator::Run ();

  NS_TEST_ASSERT_MSG_EQ (m_leasedAddress, Ipv4Address ("172.30.0.10"),
                         m_leasedAddress << " instead of " << "172.30.0.10");

  Simulator::Destroy ();
}

/**
 * \ingroup dhcp-test
 * \ingroup tests
 *
 * \brief DHCP TestSuite
 */
class DhcpTestSuite : public TestSuite
{
public:
  DhcpTestSuite ();
};

DhcpTestSuite::DhcpTestSuite ()
  : TestSuite ("dhcp", UNIT)
{
  AddTestCase (new DhcpTestCase, TestCase::QUICK);
}

static DhcpTestSuite dhcpTestSuite; //!< Static variable for test initialization

