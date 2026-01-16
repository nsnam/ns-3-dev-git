/*
 * Copyright (c) 2024 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kavya Bhat <kavyabhat@gmail.com>
 *
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/data-rate.h"
#include "ns3/dhcp6-header.h"
#include "ns3/dhcp6-helper.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv6-address-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/ping-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/simple-net-device.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/trace-helper.h"

using namespace ns3;

/**
 * @ingroup dhcp6
 * @defgroup dhcp6-test DHCPv6 module tests
 */

/**
 * @ingroup dhcp6-test
 * @ingroup tests
 *
 * @brief DHCPv6 header tests
 */
class Dhcp6TestCase : public TestCase
{
  public:
    Dhcp6TestCase();
    ~Dhcp6TestCase() override;

    /**
     * Triggered by an address lease on a client.
     * @param context The test name.
     * @param newAddress The leased address.
     */
    void LeaseObtained(std::string context, const Ipv6Address& newAddress);

  private:
    void DoRun() override;
    Ipv6Address m_leasedAddress[2]; //!< Address given to the nodes
};

Dhcp6TestCase::Dhcp6TestCase()
    : TestCase("Dhcp6 test case ")
{
}

Dhcp6TestCase::~Dhcp6TestCase()
{
}

void
Dhcp6TestCase::LeaseObtained(std::string context, const Ipv6Address& newAddress)
{
    uint8_t numericalContext = std::stoi(context, nullptr, 10);

    if (numericalContext >= 0 && numericalContext < std::size(m_leasedAddress))
    {
        m_leasedAddress[numericalContext] = newAddress;
    }
}

void
Dhcp6TestCase::DoRun()
{
    NodeContainer nonRouterNodes;
    nonRouterNodes.Create(3);
    Ptr<Node> router = CreateObject<Node>();
    NodeContainer all(nonRouterNodes, router);

    SimpleNetDeviceHelper simpleNetDevice;
    simpleNetDevice.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    simpleNetDevice.SetDeviceAttribute("DataRate", DataRateValue(DataRate("5Mbps")));
    NetDeviceContainer devices = simpleNetDevice.Install(all); // all nodes

    InternetStackHelper internetv6;
    internetv6.Install(all);

    Ipv6AddressHelper ipv6;
    ipv6.SetBase(Ipv6Address("2001:cafe::"), Ipv6Prefix(64));
    NetDeviceContainer nonRouterDevices;
    nonRouterDevices.Add(devices.Get(0)); // The server node, S0.
    nonRouterDevices.Add(devices.Get(1)); // The first client node, N0.
    nonRouterDevices.Add(devices.Get(2)); // The second client node, N1.
    Ipv6InterfaceContainer i = ipv6.AssignWithoutAddress(nonRouterDevices);

    NetDeviceContainer routerDevice;
    routerDevice.Add(devices.Get(3)); // CSMA interface of the node R0.
    Ipv6InterfaceContainer r1 = ipv6.Assign(routerDevice);
    r1.SetForwarding(0, true);

    RadvdHelper radvdHelper;

    /* Set up unsolicited RAs */
    radvdHelper.AddAnnouncedPrefix(r1.GetInterfaceIndex(0), Ipv6Address("2001:cafe::1"), 64);
    radvdHelper.GetRadvdInterface(r1.GetInterfaceIndex(0))->SetManagedFlag(true);

    Dhcp6Helper dhcp6Helper;

    dhcp6Helper.SetServerAttribute("RenewTime", StringValue("10s"));
    dhcp6Helper.SetServerAttribute("RebindTime", StringValue("16s"));
    dhcp6Helper.SetServerAttribute("PreferredLifetime", StringValue("18s"));
    dhcp6Helper.SetServerAttribute("ValidLifetime", StringValue("20s"));

    // DHCPv6 clients
    NodeContainer nodes = NodeContainer(nonRouterNodes.Get(1), nonRouterNodes.Get(2));
    ApplicationContainer dhcpClients = dhcp6Helper.InstallDhcp6Client(nodes);
    dhcpClients.Start(Seconds(1.0));
    dhcpClients.Stop(Seconds(20.0));

    // DHCPv6 server
    NetDeviceContainer serverNetDevices;
    serverNetDevices.Add(nonRouterDevices.Get(0));
    ApplicationContainer dhcpServerApp = dhcp6Helper.InstallDhcp6Server(serverNetDevices);

    Ptr<Dhcp6Server> server = DynamicCast<Dhcp6Server>(dhcpServerApp.Get(0));
    server->AddSubnet(Ipv6Address("2001:cafe::"),
                      Ipv6Prefix(64),
                      Ipv6Address("2001:cafe::42:1"),
                      Ipv6Address("2001:cafe::42:ffff"));

    dhcpServerApp.Start(Seconds(0.0));
    dhcpServerApp.Stop(Seconds(20.0));

    ApplicationContainer radvdApps = radvdHelper.Install(router);
    radvdApps.Start(Seconds(1.0));
    radvdApps.Stop(Seconds(20.0));

    dhcpClients.Get(0)->TraceConnect("NewLease",
                                     "0",
                                     MakeCallback(&Dhcp6TestCase::LeaseObtained, this));
    dhcpClients.Get(1)->TraceConnect("NewLease",
                                     "1",
                                     MakeCallback(&Dhcp6TestCase::LeaseObtained, this));

    Simulator::Stop(Seconds(21.0));
    Simulator::Run();

    // Validate that the client nodes have three addresses on each interface -
    // link-local, autoconfigured, and leased from DHCPv6 server.
    Ptr<Ipv6> ipv6_client1 = nonRouterNodes.Get(1)->GetObject<Ipv6>();
    Ptr<Ipv6L3Protocol> l3_client1 = ipv6_client1->GetObject<Ipv6L3Protocol>();
    int32_t ifIndex_client1 = ipv6_client1->GetInterfaceForDevice(nonRouterDevices.Get(1));

    NS_TEST_ASSERT_MSG_EQ(l3_client1->GetNAddresses(ifIndex_client1),
                          3,
                          "Incorrect number of addresses.");

    Ptr<Ipv6> ipv6_client2 = nonRouterNodes.Get(2)->GetObject<Ipv6>();
    Ptr<Ipv6L3Protocol> l3_client2 = ipv6_client2->GetObject<Ipv6L3Protocol>();
    int32_t ifIndex_client2 = ipv6_client2->GetInterfaceForDevice(nonRouterDevices.Get(2));
    NS_TEST_ASSERT_MSG_EQ(l3_client2->GetNAddresses(ifIndex_client2),
                          3,
                          "Incorrect number of addresses.");

    Simulator::Destroy();
}

/**
 * @ingroup dhcp6-test
 * @ingroup tests
 *
 * @brief DHCPv6 DUID tests
 */
class Dhcp6DuidTestCase : public TestCase
{
  public:
    Dhcp6DuidTestCase();
    ~Dhcp6DuidTestCase() override;

  private:
    void DoRun() override;
};

Dhcp6DuidTestCase::Dhcp6DuidTestCase()
    : TestCase("Dhcp6 DUID test case ")
{
}

Dhcp6DuidTestCase::~Dhcp6DuidTestCase()
{
}

void
Dhcp6DuidTestCase::DoRun()
{
    NodeContainer nodes;
    nodes.Create(2);

    SimpleNetDeviceHelper simpleNetDevice;
    simpleNetDevice.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    simpleNetDevice.SetDeviceAttribute("DataRate", DataRateValue(DataRate("5Mbps")));
    NetDeviceContainer devices = simpleNetDevice.Install(nodes);

    InternetStackHelper internetv6;
    internetv6.Install(nodes);

    Ipv6AddressHelper ipv6;
    ipv6.SetBase(Ipv6Address("2001:cafe::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer i = ipv6.AssignWithoutAddress(devices);

    Dhcp6Helper dhcp6Helper;
    ApplicationContainer dhcpClients = dhcp6Helper.InstallDhcp6Client(nodes);

    // DUID-LL
    {
        Duid duidTest;
        duidTest.InitializeLL(nodes.Get(1));
        std::ostringstream buffer;
        buffer << duidTest;
        NS_TEST_ASSERT_MSG_EQ(duidTest.GetSerializedSize(), 10, "Unexpected DUID-LL length.");
        std::string reference{"DUID-LL HWtype: 1 Identifier: 0x000000000002"};
        NS_TEST_ASSERT_MSG_EQ(buffer.str(), reference, "Unexpected DUID-LL.");
    }

    // DUID-LLT
    {
        Duid duidTest;
        duidTest.InitializeLLT(nodes.Get(1));
        std::ostringstream buffer;
        buffer << duidTest;
        NS_TEST_ASSERT_MSG_EQ(duidTest.GetSerializedSize(), 14, "Unexpected DUID-LLT length.");
        std::string reference{"DUID-LLT HWtype: 1 Time: 0 Identifier: 0x000000000002"};
        NS_TEST_ASSERT_MSG_EQ(buffer.str(), reference, "Unexpected DUID-LLT.");
    }

    // DUID-EN
    {
        Duid duidTest;
        std::vector<uint8_t> identifier(32);
        std::iota(identifier.begin(), identifier.end(), 0);

        duidTest.InitializeEN(0xf00dcafe, identifier);
        std::ostringstream buffer;
        buffer << duidTest;
        NS_TEST_ASSERT_MSG_EQ(duidTest.GetSerializedSize(), 38, "Unexpected DUID-EN length.");
        std::string reference{"DUID-EN EnNumber: 0xf00dcafe Identifier: "
                              "0x000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"};
        NS_TEST_ASSERT_MSG_EQ(buffer.str(), reference, "Unexpected DUID-EN.");
    }

    // DUID-UUID
    {
        Duid duidTest;
        std::vector<uint8_t> identifier(16);
        std::iota(identifier.begin(), identifier.end(), 0);

        duidTest.InitializeUUID(identifier);
        std::ostringstream buffer;
        buffer << duidTest;
        NS_TEST_ASSERT_MSG_EQ(duidTest.GetSerializedSize(), 18, "Unexpected DUID-UUID length.");
        std::string reference{"DUID-UUID UUID: 0x000102030405060708090a0b0c0d0e0f"};
        NS_TEST_ASSERT_MSG_EQ(buffer.str(), reference, "Unexpected DUID-UUID.");
    }
}

/**
 * @ingroup dhcp6-test
 * @ingroup tests
 *
 * @brief DHCPv6 TestSuite
 */
class Dhcp6TestSuite : public TestSuite
{
  public:
    Dhcp6TestSuite();
};

Dhcp6TestSuite::Dhcp6TestSuite()
    : TestSuite("dhcp6", Type::UNIT)
{
    AddTestCase(new Dhcp6TestCase, TestCase::Duration::QUICK);
    AddTestCase(new Dhcp6DuidTestCase, TestCase::Duration::QUICK);
}

static Dhcp6TestSuite dhcp6TestSuite; //!< Static variable for test initialization
