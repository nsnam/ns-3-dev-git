/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Boh Jie Qi <jieqiboh5836@gmail.com>
 */

#include "sixlowpan-nd-test-utils.h"

#include "ns3/internet-stack-helper.h"
#include "ns3/ipv6-routing-helper.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/simple-net-device.h"
#include "ns3/sixlowpan-helper.h"
#include "ns3/sixlowpan-nd-protocol.h"
#include "ns3/test.h"

#include <fstream>
#include <limits>
#include <regex>
#include <string>

namespace ns3
{

/**
 * @ingroup sixlowpan-nd-reg-tests
 *
 * @brief Test successful registration of varying numbers of 6LNs with 1 6LBR
 */
class SixLowPanNdOneLNRegTest : public TestCase
{
  public:
    SixLowPanNdOneLNRegTest()
        : TestCase("Registration of 1 6LN with 1 6LBR")
    {
    }

    void DoRun() override
    {
        // 6LBR - node 0
        // LLaddr: fe80::200:ff:fe00:1
        // Link-layer address: 02:00:00:00:00:01
        // Gaddr: 2001::200:ff:fe00:1

        // 6LN - node 1
        // LLaddr: fe80::200:ff:fe00:2
        // Link-layer address: 02:00:00:00:00:02
        // Gaddr: 2001::200:ff:fe00:2 (Needs reg first)

        // Basic Exchange, then assert NC, 6LNC and RT contents of 6LN and 6LBR
        // Create nodes
        NodeContainer nodes;
        nodes.Create(2);
        Ptr<Node> lbrNode = nodes.Get(0);
        Ptr<Node> lnNode = nodes.Get(1);

        // Install SimpleNetDevice on nodes
        SimpleNetDeviceHelper simpleNetDeviceHelper;
        NetDeviceContainer simpleNetDevices = simpleNetDeviceHelper.Install(nodes);

        // Install Internet stack
        InternetStackHelper internetv6;
        internetv6.Install(nodes);

        // Install 6LoWPAN on top of SimpleNetDevice
        SixLowPanHelper sixlowpan;
        NetDeviceContainer devices = sixlowpan.Install(simpleNetDevices);

        // Configure 6LoWPAN ND
        // Node 0 = 6LBR
        sixlowpan.InstallSixLowPanNdBorderRouter(devices.Get(0), "2001::");
        sixlowpan.SetAdvertisedPrefix(devices.Get(0), Ipv6Prefix("2001::", 64));
        // Node 1 = 6LN
        sixlowpan.InstallSixLowPanNdNode(devices.Get(1));

        std::ostringstream ndiscStream;
        Ptr<OutputStreamWrapper> outputNdiscStream = Create<OutputStreamWrapper>(&ndiscStream);
        std::ostringstream routingTableStream;
        Ptr<OutputStreamWrapper> outputRoutingTableStream =
            Create<OutputStreamWrapper>(&routingTableStream);
        std::ostringstream bindingTableStream;
        Ptr<OutputStreamWrapper> outputBindingTableStream =
            Create<OutputStreamWrapper>(&bindingTableStream);

        Ipv6RoutingHelper::PrintNeighborCacheAllAt(Seconds(5), outputNdiscStream);
        Ipv6RoutingHelper::PrintRoutingTableAllAt(Seconds(5), outputRoutingTableStream);
        SixLowPanHelper::PrintBindingTableAllAt(Seconds(5), outputBindingTableStream);

        Ptr<SixLowPanNdProtocol> nd = lnNode->GetObject<SixLowPanNdProtocol>();
        nd->TraceConnectWithoutContext(
            "AddressRegistrationResult",
            MakeCallback(&SixLowPanNdOneLNRegTest::RegistrationResultSink, this));

        Simulator::Stop(Seconds(5.0));
        Simulator::Run();
        Simulator::Destroy();

        constexpr auto expectedNdiscStream =
            "NDISC Cache of node 0 at time +5s\n"
            "2001::200:ff:fe00:2 dev 2 lladdr 03-06-00:00:00:00:00:02 REACHABLE\n"
            "fe80::200:ff:fe00:2 dev 2 lladdr 03-06-00:00:00:00:00:02 REACHABLE\n"
            "NDISC Cache of node 1 at time +5s\n"
            "fe80::200:ff:fe00:1 dev 2 lladdr 03-06-00:00:00:00:00:01 REACHABLE\n";
        NS_TEST_EXPECT_MSG_EQ(ndiscStream.str(), expectedNdiscStream, "NdiscCache is incorrect.");

        constexpr auto expectedRoutingTableStream =
            "Node: 0, Time: +5s, Local time: +5s, Ipv6ListRouting table\n"
            "  Priority: 0 Protocol: ns3::Ipv6StaticRouting\n"
            "Node: 0, Time: +5s, Local time: +5s, Ipv6StaticRouting table\n"
            "Destination                    Next Hop                   Flag Met Ref Use If\n"
            "::1/128                        ::                         UH   0   -   -   0\n"
            "fe80::/64                      ::                         U    0   -   -   1\n"
            "2001::200:ff:fe00:2/128        fe80::200:ff:fe00:2        UH   0   -   -   1\n\n"
            "  Priority: -10 Protocol: ns3::Ipv6GlobalRouting\n"
            "Node: 0, Time: +5s, Local time: +5s, Ipv6GlobalRouting table\n"
            "Destination                    Next Hop                   Flag Met Ref Use Iface\n\n"
            "Node: 1, Time: +5s, Local time: +5s, Ipv6ListRouting table\n"
            "  Priority: 0 Protocol: ns3::Ipv6StaticRouting\n"
            "Node: 1, Time: +5s, Local time: +5s, Ipv6StaticRouting table\n"
            "Destination                    Next Hop                   Flag Met Ref Use If\n"
            "::1/128                        ::                         UH   0   -   -   0\n"
            "fe80::/64                      ::                         U    0   -   -   1\n"
            "::/0                           fe80::200:ff:fe00:1        UG   0   -   -   1\n\n"
            "  Priority: -10 Protocol: ns3::Ipv6GlobalRouting\n"
            "Node: 1, Time: +5s, Local time: +5s, Ipv6GlobalRouting table\n"
            "Destination                    Next Hop                   Flag Met Ref Use Iface\n\n";
        NS_TEST_EXPECT_MSG_EQ(routingTableStream.str(),
                              expectedRoutingTableStream,
                              "Routing table does not match expected.");

        NS_TEST_ASSERT_MSG_EQ(registeredAddress,
                              Ipv6Address("2001::200:ff:fe00:2"),
                              "Registered address does not match expected value.");

        // Validate binding table output for Node 0 and Node 1
        NS_TEST_ASSERT_MSG_EQ((bindingTableStream.str()),
                              GenerateBindingTableOutput(2, Seconds(5)),
                              "BindingTable does not match expected output.");
    }

  private:
    Ipv6Address registeredAddress; //!< Address that was successfully registered during the test

    /**
     * @brief Callback sink for address registration result trace events
     * @param address IPv6 address that was being registered
     * @param success Whether the registration was successful
     * @param status Registration status code
     */
    void RegistrationResultSink(Ipv6Address address, bool success, uint8_t status)
    {
        if (success)
        {
            registeredAddress = address;
        }
    }
};

/**
 * @ingroup sixlowpan-nd-reg-tests
 *
 * @brief Test successful registration of 5 6LNs with 1 6LBR
 */
class SixLowPanNdFiveLNRegTest : public TestCase
{
  public:
    SixLowPanNdFiveLNRegTest()
        : TestCase("Registration of 5 6LNs with 1 6LBR")
    {
    }

    void DoRun() override
    {
        Time duration = Time("50s");

        constexpr uint32_t numLns = 5;

        NodeContainer nodes;
        nodes.Create(1 + numLns); // 1 LBR + 5 LNs
        Ptr<Node> lbrNode = nodes.Get(0);

        SimpleNetDeviceHelper simpleNetDeviceHelper;

        NetDeviceContainer simpleNetDevices;
        simpleNetDevices = simpleNetDeviceHelper.Install(nodes);

        InternetStackHelper internetv6;
        internetv6.Install(nodes);

        SixLowPanHelper sixlowpan;
        NetDeviceContainer devices = sixlowpan.Install(simpleNetDevices);

        sixlowpan.InstallSixLowPanNdBorderRouter(devices.Get(0), "2001::");
        sixlowpan.SetAdvertisedPrefix(devices.Get(0), Ipv6Prefix("2001::", 64));

        for (uint32_t i = 1; i <= numLns; ++i)
        {
            sixlowpan.InstallSixLowPanNdNode(devices.Get(i));
        }

        std::ostringstream ndiscStream;
        Ptr<OutputStreamWrapper> outputNdiscStream = Create<OutputStreamWrapper>(&ndiscStream);
        std::ostringstream routingTableStream;
        Ptr<OutputStreamWrapper> outputRoutingTableStream =
            Create<OutputStreamWrapper>(&routingTableStream);
        std::ostringstream bindingTableStream;
        Ptr<OutputStreamWrapper> outputBindingTableStream =
            Create<OutputStreamWrapper>(&bindingTableStream);

        Ipv6RoutingHelper::PrintNeighborCacheAllAt(duration, outputNdiscStream);
        Ipv6RoutingHelper::PrintRoutingTableAllAt(duration, outputRoutingTableStream);
        SixLowPanHelper::PrintBindingTableAllAt(duration, outputBindingTableStream);

        Simulator::Stop(duration);
        Simulator::Run();
        Simulator::Destroy();

        NS_TEST_ASSERT_MSG_EQ(NormalizeNdiscCacheStates(ndiscStream.str()),
                              GenerateNdiscCacheOutput(numLns + 1, duration),
                              "NdiscCache does not match expected output.");

        NS_TEST_ASSERT_MSG_EQ(SortRoutingTableString(routingTableStream.str()),
                              GenerateRoutingTableOutput(numLns + 1, duration),
                              "RoutingTable does not match expected output.");

        NS_TEST_ASSERT_MSG_EQ((bindingTableStream.str()),
                              GenerateBindingTableOutput(numLns + 1, duration),
                              "BindingTable does not match expected output.");
    }
};

/**
 * @ingroup sixlowpan-nd-reg-tests
 *
 * @brief Test successful registration of 15 6LNs with 1 6LBR
 */
class SixLowPanNdFifteenLNRegTest : public TestCase
{
  public:
    SixLowPanNdFifteenLNRegTest()
        : TestCase("Registration of 15 6LNs with 1 6LBR")
    {
    }

    void DoRun() override
    {
        Time duration = Time("300s");
        constexpr uint32_t numLns = 15;

        NodeContainer nodes;
        nodes.Create(1 + numLns); // 1 LBR + 15 LNs
        Ptr<Node> lbrNode = nodes.Get(0);

        // Use SimpleNetDevice instead of LrWpan
        SimpleNetDeviceHelper simpleNetDeviceHelper;
        NetDeviceContainer simpleNetDevices = simpleNetDeviceHelper.Install(nodes);

        InternetStackHelper internetv6;
        internetv6.Install(nodes);

        SixLowPanHelper sixlowpan;
        NetDeviceContainer devices = sixlowpan.Install(simpleNetDevices);

        // Configure 6LoWPAN ND
        sixlowpan.InstallSixLowPanNdBorderRouter(devices.Get(0), "2001::");
        sixlowpan.SetAdvertisedPrefix(devices.Get(0), Ipv6Prefix("2001::", 64));

        for (uint32_t i = 1; i <= numLns; ++i)
        {
            sixlowpan.InstallSixLowPanNdNode(devices.Get(i));
        }

        std::ostringstream ndiscStream;
        Ptr<OutputStreamWrapper> outputNdiscStream = Create<OutputStreamWrapper>(&ndiscStream);
        std::ostringstream routingTableStream;
        Ptr<OutputStreamWrapper> outputRoutingTableStream =
            Create<OutputStreamWrapper>(&routingTableStream);
        std::ostringstream bindingTableStream;
        Ptr<OutputStreamWrapper> outputBindingTableStream =
            Create<OutputStreamWrapper>(&bindingTableStream);

        Ipv6RoutingHelper::PrintNeighborCacheAllAt(duration, outputNdiscStream);
        Ipv6RoutingHelper::PrintRoutingTableAllAt(duration, outputRoutingTableStream);
        SixLowPanHelper::PrintBindingTableAllAt(duration, outputBindingTableStream);

        Simulator::Stop(duration);
        Simulator::Run();
        Simulator::Destroy();

        NS_TEST_ASSERT_MSG_EQ(NormalizeNdiscCacheStates(ndiscStream.str()),
                              GenerateNdiscCacheOutput(numLns + 1, duration),
                              "NdiscCache does not match expected output.");

        NS_TEST_ASSERT_MSG_EQ(SortRoutingTableString(routingTableStream.str()),
                              GenerateRoutingTableOutput(numLns + 1, duration),
                              "RoutingTable does not match expected output.");

        NS_TEST_ASSERT_MSG_EQ((bindingTableStream.str()),
                              GenerateBindingTableOutput(numLns + 1, duration),
                              "BindingTable does not match expected output.");
    }
};

/**
 * @ingroup sixlowpan-nd-reg-tests
 *
 * @brief Test successful registration of 20 6LNs with 1 6LBR
 */
class SixLowPanNdTwentyLNRegTest : public TestCase
{
  public:
    SixLowPanNdTwentyLNRegTest()
        : TestCase("Registration of 20 6LNs with 1 6LBR")
    {
    }

    void DoRun() override
    {
        Time duration = Time("300s");
        constexpr uint32_t numLns = 20;

        NodeContainer nodes;
        nodes.Create(1 + numLns); // 1 LBR + 20 LNs
        Ptr<Node> lbrNode = nodes.Get(0);

        // Use SimpleNetDevice instead of LrWpan
        SimpleNetDeviceHelper simpleNetDeviceHelper;
        NetDeviceContainer simpleNetDevices = simpleNetDeviceHelper.Install(nodes);

        InternetStackHelper internetv6;
        internetv6.Install(nodes);

        SixLowPanHelper sixlowpan;
        NetDeviceContainer devices = sixlowpan.Install(simpleNetDevices);

        // Configure 6LoWPAN ND
        sixlowpan.InstallSixLowPanNdBorderRouter(devices.Get(0), "2001::");
        sixlowpan.SetAdvertisedPrefix(devices.Get(0), Ipv6Prefix("2001::", 64));

        for (uint32_t i = 1; i <= numLns; ++i)
        {
            sixlowpan.InstallSixLowPanNdNode(devices.Get(i));
        }

        std::ostringstream ndiscStream;
        Ptr<OutputStreamWrapper> outputNdiscStream = Create<OutputStreamWrapper>(&ndiscStream);
        std::ostringstream routingTableStream;
        Ptr<OutputStreamWrapper> outputRoutingTableStream =
            Create<OutputStreamWrapper>(&routingTableStream);
        std::ostringstream bindingTableStream;
        Ptr<OutputStreamWrapper> outputBindingTableStream =
            Create<OutputStreamWrapper>(&bindingTableStream);

        Ipv6RoutingHelper::PrintNeighborCacheAllAt(duration, outputNdiscStream);
        Ipv6RoutingHelper::PrintRoutingTableAllAt(duration, outputRoutingTableStream);
        SixLowPanHelper::PrintBindingTableAllAt(duration, outputBindingTableStream);

        Simulator::Stop(duration);
        Simulator::Run();
        Simulator::Destroy();

        NS_TEST_ASSERT_MSG_EQ(NormalizeNdiscCacheStates(ndiscStream.str()),
                              GenerateNdiscCacheOutput(numLns + 1, duration),
                              "NdiscCache does not match expected output.");

        NS_TEST_ASSERT_MSG_EQ(SortRoutingTableString(routingTableStream.str()),
                              GenerateRoutingTableOutput(numLns + 1, duration),
                              "RoutingTable does not match expected output.");

        NS_TEST_ASSERT_MSG_EQ((bindingTableStream.str()),
                              GenerateBindingTableOutput(numLns + 1, duration),
                              "BindingTable does not match expected output.");
    }
};

/**
 * @ingroup sixlowpan-nd-reg-tests
 *
 * @brief Test 6LN multicast RS timeout behavior when no RA is received
 */
class SixLowPanNdMulticastRsTimeoutTest : public TestCase
{
  public:
    SixLowPanNdMulticastRsTimeoutTest()
        : TestCase("6LN sends multicast RS but receives no RA, test timeout behavior")
    {
    }

    void DoRun() override
    {
        NodeContainer nodes;
        nodes.Create(1);
        Ptr<Node> lnNode = nodes.Get(0);

        // Use SimpleNetDevice instead of LrWpan
        SimpleNetDeviceHelper simpleNetDeviceHelper;
        NetDeviceContainer simpleNetDevices = simpleNetDeviceHelper.Install(nodes);

        // Install Internet stack
        InternetStackHelper internetv6;
        internetv6.Install(nodes);

        // Install 6LoWPAN device on top of SimpleNetDevice
        SixLowPanHelper sixlowpan;
        NetDeviceContainer sixDevices = sixlowpan.Install(simpleNetDevices);

        // Install ND only as a node (no BR) - this means no RA responses
        sixlowpan.InstallSixLowPanNdNode(sixDevices.Get(0));

        // Set up trace to capture multicast RS events
        Ptr<SixLowPanNdProtocol> nd = lnNode->GetObject<SixLowPanNdProtocol>();
        NS_ASSERT_MSG(nd, "Failed to get SixLowPanNdProtocol");
        nd->TraceConnectWithoutContext(
            "MulticastRS",
            MakeCallback(&SixLowPanNdMulticastRsTimeoutTest::MulticastRsSink, this));

        // Simulation time should be long enough to see RS timeout
        // Keep same duration to maintain test behavior
        Simulator::Stop(Seconds(210));
        Simulator::Run();
        Simulator::Destroy();

        // Verify we got the expected number of multicast RS events
        NS_TEST_ASSERT_MSG_EQ(m_multicastRsEvents.size(),
                              7,
                              "Expected 7 multicast RS events, but got " +
                                  std::to_string(m_multicastRsEvents.size()));
    }

  private:
    std::vector<Ipv6Address>
        m_multicastRsEvents; //!< Container for multicast RS events captured during test

    /**
     * @brief Callback sink for multicast RS trace events
     * @param src Source address of the multicast RS
     */
    void MulticastRsSink(Ipv6Address src)
    {
        m_multicastRsEvents.push_back(src);
    }
};

/**
 * @ingroup sixlowpan-nd-reg-tests
 *
 * @brief 6LoWPAN-ND TestSuite
 */
class SixLowPanNdRegTestSuite : public TestSuite
{
  public:
    SixLowPanNdRegTestSuite()
        : TestSuite("sixlowpan-nd-reg-test", Type::UNIT)
    {
        AddTestCase(new SixLowPanNdOneLNRegTest(), TestCase::Duration::QUICK);
        AddTestCase(new SixLowPanNdFiveLNRegTest(), TestCase::Duration::QUICK);
        AddTestCase(new SixLowPanNdFifteenLNRegTest(), TestCase::Duration::QUICK);
        AddTestCase(new SixLowPanNdTwentyLNRegTest(), TestCase::Duration::QUICK);
        AddTestCase(new SixLowPanNdMulticastRsTimeoutTest(), TestCase::Duration::QUICK);
    }
};

static SixLowPanNdRegTestSuite
    g_sixlowpanndregTestSuite; //!< Static variable for test initialization
} // namespace ns3
