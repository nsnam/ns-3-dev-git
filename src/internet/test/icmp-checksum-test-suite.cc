/*
 * Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

#include "ns3/iana-internet-protocol-numbers.h"
#include "ns3/icmpv4.h"
#include "ns3/icmpv6-header.h"
#include "ns3/icmpv6-l4-protocol.h"
#include "ns3/ipv6-address.h"
#include "ns3/packet.h"
#include "ns3/test.h"

#include <vector>

using namespace ns3;

/**
 * @ingroup internet-test
 *
 * @brief Verify that Icmpv4Header checksum verification accepts a correctly
 * formed ICMP message and rejects a corrupted one (@RFC{792}).
 */
class Icmpv4ChecksumTestCase : public TestCase
{
  public:
    Icmpv4ChecksumTestCase()
        : TestCase("ICMPv4 checksum verification")
    {
    }

  private:
    void DoRun() override;

    /**
     * @brief Deserialize the ICMP header of a packet and verify its checksum.
     * @param packet the packet holding an ICMP message
     * @return true if the checksum is valid, false otherwise
     */
    bool ChecksumOk(Ptr<const Packet> packet) const;
};

bool
Icmpv4ChecksumTestCase::ChecksumOk(Ptr<const Packet> packet) const
{
    Icmpv4Header icmp;
    icmp.EnableChecksum();
    packet->PeekHeader(icmp);
    return icmp.IsChecksumOk();
}

void
Icmpv4ChecksumTestCase::DoRun()
{
    // Build a valid ICMP Echo with a non-trivial payload and a checksum
    // computed exactly as on the transmission path.
    uint8_t payload[16];
    for (std::size_t i = 0; i < sizeof(payload); ++i)
    {
        payload[i] = i;
    }
    auto packet = Create<Packet>(payload, sizeof(payload));

    Icmpv4Echo echo;
    echo.SetIdentifier(1234);
    echo.SetSequenceNumber(1);
    packet->AddHeader(echo);

    Icmpv4Header icmp;
    icmp.SetType(Icmpv4Header::ICMPV4_ECHO);
    icmp.SetCode(0);
    icmp.EnableChecksum();
    packet->AddHeader(icmp);

    NS_TEST_ASSERT_MSG_EQ(ChecksumOk(packet), true, "Valid ICMPv4 checksum was rejected");

    // Corrupt a single byte of the serialized message: the checksum must now
    // fail to verify.
    uint32_t size = packet->GetSize();
    std::vector<uint8_t> bytes(size);
    packet->CopyData(bytes.data(), size);
    bytes[size - 1] ^= 0xFF;
    NS_TEST_ASSERT_MSG_EQ(ChecksumOk(Create<Packet>(bytes.data(), size)),
                          false,
                          "Corrupted ICMPv4 packet was accepted as valid");

    // Corrupting the checksum field itself must also be detected.
    bytes[size - 1] ^= 0xFF; // restore payload
    bytes[2] ^= 0xFF;        // checksum field is at offset 2 of the ICMP header
    NS_TEST_ASSERT_MSG_EQ(ChecksumOk(Create<Packet>(bytes.data(), size)),
                          false,
                          "ICMPv4 packet with tampered checksum field was accepted");
}

/**
 * @ingroup internet-test
 *
 * @brief Verify that Icmpv6Header checksum verification accepts a correctly
 * formed ICMPv6 message and rejects a corrupted one, as @RFC{4443} Section 2.3
 * requires (@issueid{1036}).
 */
class Icmpv6ChecksumTestCase : public TestCase
{
  public:
    Icmpv6ChecksumTestCase()
        : TestCase("ICMPv6 checksum verification")
    {
    }

  private:
    void DoRun() override;

    /**
     * @brief Deserialize the ICMPv6 header of a packet and verify its checksum.
     * @param packet the packet holding an ICMPv6 message
     * @param src the IPv6 source address of the pseudo header
     * @param dst the IPv6 destination address of the pseudo header
     * @return true if the checksum is valid, false otherwise
     */
    bool ChecksumOk(Ptr<const Packet> packet, Ipv6Address src, Ipv6Address dst) const;
};

bool
Icmpv6ChecksumTestCase::ChecksumOk(Ptr<const Packet> packet, Ipv6Address src, Ipv6Address dst) const
{
    Icmpv6Header icmp;
    icmp.InitializeChecksum(src, dst);
    packet->PeekHeader(icmp);
    return icmp.IsChecksumOk();
}

void
Icmpv6ChecksumTestCase::DoRun()
{
    Ipv6Address src("2001:db8::1");
    Ipv6Address dst("2001:db8::2");

    // Build a valid ICMPv6 Echo Request with a non-trivial payload and a
    // checksum computed exactly as on the transmission path.
    uint8_t payload[16];
    for (std::size_t i = 0; i < sizeof(payload); ++i)
    {
        payload[i] = i;
    }
    auto packet = Create<Packet>(payload, sizeof(payload));

    Icmpv6Echo echo(true);
    echo.SetId(1234);
    echo.SetSeq(1);
    echo.CalculatePseudoHeaderChecksum(src, dst, packet->GetSize() + echo.GetSerializedSize());
    packet->AddHeader(echo);

    NS_TEST_ASSERT_MSG_EQ(ChecksumOk(packet, src, dst), true, "Valid ICMPv6 checksum was rejected");

    // Corrupt a single byte of the serialized message: the checksum must now
    // fail to verify.
    uint32_t size = packet->GetSize();
    std::vector<uint8_t> bytes(size);
    packet->CopyData(bytes.data(), size);
    bytes[size - 1] ^= 0xFF;
    NS_TEST_ASSERT_MSG_EQ(ChecksumOk(Create<Packet>(bytes.data(), size), src, dst),
                          false,
                          "Corrupted ICMPv6 packet was accepted as valid");

    // Corrupting the checksum field itself must also be detected.
    bytes[size - 1] ^= 0xFF; // restore payload
    bytes[2] ^= 0xFF;        // checksum field is at offset 2 of the ICMPv6 header
    NS_TEST_ASSERT_MSG_EQ(ChecksumOk(Create<Packet>(bytes.data(), size), src, dst),
                          false,
                          "ICMPv6 packet with tampered checksum field was accepted");
}

/**
 * @ingroup internet-test
 *
 * @brief ICMP and ICMPv6 checksum test suite.
 */
class IcmpChecksumTestSuite : public TestSuite
{
  public:
    IcmpChecksumTestSuite()
        : TestSuite("icmp-checksum", Type::UNIT)
    {
        AddTestCase(new Icmpv4ChecksumTestCase, TestCase::Duration::QUICK);
        AddTestCase(new Icmpv6ChecksumTestCase, TestCase::Duration::QUICK);
    }
};

static IcmpChecksumTestSuite g_icmpChecksumTestSuite; //!< Static variable for test
                                                      //!< initialization
