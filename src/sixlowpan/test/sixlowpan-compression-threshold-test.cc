/*
 * Copyright (c) 2026 SRM Institute of Science and Technology
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Usham Roy <ushamroy80@gmail.com>
 */

#include "mock-net-device.h"

#include "ns3/ipv6-header.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/sixlowpan-header.h"
#include "ns3/sixlowpan-net-device.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"

using namespace ns3;

/**
 * @ingroup sixlowpan-tests
 *
 * @brief SixLowPanNetDevice compression threshold test.
 *
 * Packets whose compressed form is smaller than the underlying device
 * padding threshold must be sent with the uncompressed IPv6 dispatch:
 * link-layer padding is indistinguishable from data on such links, and it
 * would corrupt the reconstruction of the elided IPv6 Payload Length
 * (issue #1361).
 */
class SixLowPanCompressionThresholdTest : public TestCase
{
    uint8_t m_dispatch{0}; //!< First byte of the last transmitted frame

    /**
     * Callback for frames handed to the underlying mock device.
     *
     * @param device The mock device.
     * @param packet The transmitted packet.
     * @param protocol The protocol number.
     * @param source The source address.
     * @param destination The destination address.
     * @param packetType The packet type.
     * @return true
     */
    bool ReceiveFromMockDevice(Ptr<NetDevice> device,
                               Ptr<const Packet> packet,
                               uint16_t protocol,
                               const Address& source,
                               const Address& destination,
                               NetDevice::PacketType packetType);

    /**
     * Send one packet over a SixLowPanNetDevice.
     *
     * @param paddingThreshold Padding threshold reported by the mock device.
     * @param compressionThreshold Value for the CompressionThreshold attribute.
     * @param payloadSize IPv6 payload size in bytes.
     * @return The first dispatch byte of the transmitted frame.
     */
    uint8_t SendOnePacket(uint16_t paddingThreshold,
                          uint32_t compressionThreshold,
                          uint32_t payloadSize);

  public:
    SixLowPanCompressionThresholdTest();
    void DoRun() override;
};

SixLowPanCompressionThresholdTest::SixLowPanCompressionThresholdTest()
    : TestCase("SixLowPanNetDevice compression threshold from device padding threshold")
{
}

bool
SixLowPanCompressionThresholdTest::ReceiveFromMockDevice(Ptr<NetDevice> device,
                                                         Ptr<const Packet> packet,
                                                         uint16_t protocol,
                                                         const Address& source,
                                                         const Address& destination,
                                                         NetDevice::PacketType packetType)
{
    packet->CopyData(&m_dispatch, sizeof(m_dispatch));
    return true;
}

uint8_t
SixLowPanCompressionThresholdTest::SendOnePacket(uint16_t paddingThreshold,
                                                 uint32_t compressionThreshold,
                                                 uint32_t payloadSize)
{
    auto node = CreateObject<Node>();

    auto mock = CreateObject<MockNetDevice>();
    node->AddDevice(mock);
    mock->SetNode(node);
    mock->SetAddress(Mac48Address("00:00:00:00:00:01"));
    mock->SetPaddingThreshold(paddingThreshold);
    mock->SetSendCallback(
        MakeCallback(&SixLowPanCompressionThresholdTest::ReceiveFromMockDevice, this));

    auto six = CreateObject<SixLowPanNetDevice>();
    six->SetAttribute("CompressionThreshold", UintegerValue(compressionThreshold));
    node->AddDevice(six);
    six->SetNetDevice(mock);

    // With inline (incompressible) addresses the IPHC-compressed frame is
    // about 35 bytes plus the payload.
    Ptr<Packet> pkt = Create<Packet>(payloadSize);
    Ipv6Header ipHdr;
    ipHdr.SetSource(Ipv6Address("2001:1::1"));
    ipHdr.SetDestination(Ipv6Address("2001:1::2"));
    ipHdr.SetHopLimit(64);
    ipHdr.SetPayloadLength(payloadSize);
    ipHdr.SetNextHeader(0xff);
    pkt->AddHeader(ipHdr);

    // Send() forwards the frame synchronously to the mock device, whose
    // send callback (ReceiveFromMockDevice) stores the first byte of the
    // frame in m_dispatch, overwriting the zero (not a 6LoWPAN dispatch)
    // set here.
    m_dispatch = 0;
    six->Send(pkt, Mac48Address("00:00:00:00:00:02"), 0);

    Simulator::Destroy();
    return m_dispatch;
}

void
SixLowPanCompressionThresholdTest::DoRun()
{
    constexpr uint16_t noPadding = 0;
    constexpr uint16_t ethernetPadding = 46;
    constexpr uint32_t defaultThreshold = 0;
    constexpr uint32_t ethernetThreshold = 46;
    // The resulting IPv6 packets (40-byte header + payload) are 44 and 60
    // bytes respectively.
    constexpr uint32_t smallPayload = 4;
    constexpr uint32_t largePayload = 20;

    // No padding threshold: the packet is IPHC-compressed.
    uint8_t dispatch = SendOnePacket(noPadding, defaultThreshold, smallPayload);
    NS_TEST_EXPECT_MSG_EQ(SixLowPanDispatch::GetDispatchType(dispatch),
                          SixLowPanDispatch::LOWPAN_IPHC,
                          "Expected IPHC dispatch on a non-padding device");

    // Device padding threshold above the compressed size: the packet must
    // go out uncompressed even though the CompressionThreshold attribute
    // is set to the zero default.
    dispatch = SendOnePacket(ethernetPadding, defaultThreshold, smallPayload);
    NS_TEST_EXPECT_MSG_EQ(SixLowPanDispatch::GetDispatchType(dispatch),
                          SixLowPanDispatch::LOWPAN_IPv6,
                          "Expected uncompressed IPv6 dispatch on a padding device");

    // Converse of the previous case: setting the CompressionThreshold
    // attribute alone must force the uncompressed dispatch.
    dispatch = SendOnePacket(noPadding, ethernetThreshold, smallPayload);
    NS_TEST_EXPECT_MSG_EQ(SixLowPanDispatch::GetDispatchType(dispatch),
                          SixLowPanDispatch::LOWPAN_IPv6,
                          "Expected uncompressed IPv6 dispatch with the attribute set");

    // A payload large enough to exceed the threshold once compressed must
    // still be IPHC-compressed.
    dispatch = SendOnePacket(noPadding, ethernetThreshold, largePayload);
    NS_TEST_EXPECT_MSG_EQ(SixLowPanDispatch::GetDispatchType(dispatch),
                          SixLowPanDispatch::LOWPAN_IPHC,
                          "Expected IPHC dispatch for a packet above the threshold");
}

/**
 * @ingroup sixlowpan-tests
 *
 * @brief SixLowPanNetDevice compression threshold TestSuite.
 */
class SixLowPanCompressionThresholdTestSuite : public TestSuite
{
  public:
    SixLowPanCompressionThresholdTestSuite();
};

SixLowPanCompressionThresholdTestSuite::SixLowPanCompressionThresholdTestSuite()
    : TestSuite("sixlowpan-compression-threshold", Type::UNIT)
{
    AddTestCase(new SixLowPanCompressionThresholdTest(), TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static SixLowPanCompressionThresholdTestSuite g_sixLowPanCompressionThresholdTestSuite;
