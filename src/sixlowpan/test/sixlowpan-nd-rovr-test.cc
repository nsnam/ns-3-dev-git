/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Boh Jie Qi <jieqiboh5836@gmail.com>
 */

#include "ns3/internet-stack-helper.h"
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
 * @ingroup sixlowpan-nd-rovr-tests
 *
 * @brief Test successful registration of varying numbers of 6LNs with 1 6LBR
 */
class SixLowPanNdRovrTest : public TestCase
{
  public:
    SixLowPanNdRovrTest()
        : TestCase("2 6LNs with different ROVR attempt to register the same address")
    {
    }

    void DoRun() override
    {
        // Create nodes
        NodeContainer nodes;
        nodes.Create(2);
        Ptr<Node> lbrNode = nodes.Get(0);
        Ptr<Node> lnNode = nodes.Get(1);

        // Install SimpleNetDevice instead of LrWpan
        SimpleNetDeviceHelper simpleNetDeviceHelper;
        NetDeviceContainer simpleNetDevices = simpleNetDeviceHelper.Install(nodes);

        // Install IPv6 stack
        InternetStackHelper internetv6;
        internetv6.Install(nodes);

        // Install 6LoWPAN
        SixLowPanHelper sixlowpan;
        NetDeviceContainer devices = sixlowpan.Install(simpleNetDevices);

        // Install 6LoWPAN-ND
        sixlowpan.InstallSixLowPanNdBorderRouter(devices.Get(0), "2001::");
        sixlowpan.SetAdvertisedPrefix(devices.Get(0), Ipv6Prefix("2001::", 64));

        sixlowpan.InstallSixLowPanNdNode(devices.Get(1)); // 6LN

        Ptr<SixLowPanNdProtocol> lnNdProtocol = lnNode->GetObject<SixLowPanNdProtocol>();
        lnNdProtocol->TraceConnectWithoutContext(
            "NaRx",
            MakeCallback(&SixLowPanNdRovrTest::NaRxSink, this));

        // Derive link-local addresses from MAC addresses rather than hardcoding them.
        Address lnMac = simpleNetDevices.Get(1)->GetAddress();
        Address lbrMac = simpleNetDevices.Get(0)->GetAddress();
        Ipv6Address lnLLaddr = Ipv6Address::MakeAutoconfiguredLinkLocalAddress(lnMac);
        Ipv6Address lbrLLaddr = Ipv6Address::MakeAutoconfiguredLinkLocalAddress(lbrMac);

        // Use a zeroed ROVR — deliberately different from the node's actual ROVR (derived from its
        // EUI-64) to trigger a duplicate-address conflict on the 6LBR.
        std::vector<uint8_t> conflictingRovr(16, 0);

        // Registration lifetime of 5 minutes (300s), matching the minimum ARO value.
        constexpr uint16_t regLifetimeMinutes = 5;

        Simulator::Schedule(Seconds(5),
                            &SixLowPanNdProtocol::SendSixLowPanNsWithEaro,
                            lnNode->GetObject<SixLowPanNdProtocol>(),
                            lnLLaddr,  // address to register
                            lbrLLaddr, // destination (6LBR link-local)
                            lbrMac,    // destination MAC
                            regLifetimeMinutes,
                            conflictingRovr,
                            devices.Get(1));

        Simulator::Stop(Seconds(10));
        Simulator::Run();
        Simulator::Destroy();

        NS_TEST_ASSERT_MSG_EQ(!m_naPacketsReceived.empty(),
                              true,
                              "Should have received at least one NA packet");

        Ptr<Packet> lastNa = m_naPacketsReceived.back();

        Icmpv6NA naHdr;
        Icmpv6OptionLinkLayerAddress tlla(false);
        Icmpv6OptionSixLowPanExtendedAddressRegistration earo;
        bool hasEaro = false;

        bool isValid =
            SixLowPanNdProtocol::ParseAndValidateNaEaroPacket(lastNa, naHdr, tlla, earo, hasEaro);
        NS_TEST_ASSERT_MSG_EQ(isValid, true, "Packet should be valid NA with EARO");

        NS_TEST_ASSERT_MSG_EQ(earo.GetStatus(),
                              static_cast<uint8_t>(SixLowPanNdProtocol::DUPLICATE_ADDRESS),
                              "NA EARO status should be DUPLICATE_ADDRESS");
    }

  private:
    std::vector<Ptr<Packet>> m_naPacketsReceived; //!< Container for NA packets received during test

    /**
     * @brief Callback sink for NA packet reception trace events
     * @param pkt Received NA packet
     */
    void NaRxSink(Ptr<Packet> pkt)
    {
        m_naPacketsReceived.push_back(pkt);
    }
};

/**
 * @ingroup sixlowpan-nd-rovr-tests
 *
 * @brief 6LoWPAN-ND TestSuite
 */
class SixLowPanNdRovrTestSuite : public TestSuite
{
  public:
    SixLowPanNdRovrTestSuite()
        : TestSuite("sixlowpan-nd-rovr-test", Type::UNIT)
    {
        AddTestCase(new SixLowPanNdRovrTest(), TestCase::Duration::QUICK);
    }
};

static SixLowPanNdRovrTestSuite
    g_sixlowpanndrovrTestSuite; //!< Static variable for test initialization
} // namespace ns3
