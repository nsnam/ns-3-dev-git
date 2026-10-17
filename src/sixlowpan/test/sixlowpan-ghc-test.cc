/*
 * Copyright (c) 2026 SRM Institute of Science and Technology
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Usham Roy <ushamroy80@gmail.com>
 *
 * Tests for 6LoWPAN-GHC (RFC 7400) implementation.
 */

#include "ns3/boolean.h"
#include "ns3/enum.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"
#include "ns3/simulator.h"
#include "ns3/sixlowpan-ghc.h"
#include "ns3/sixlowpan-header.h"
#include "ns3/sixlowpan-net-device.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/test.h"
#include "ns3/udp-socket-factory.h"

#include <array>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

using namespace ns3;

// ============================================================================
//  Test 1: GHC engine (compress/decompress)
// ============================================================================

/**
 * @ingroup sixlowpan-tests
 * @brief GHC engine in-memory compress/decompress test.
 *
 * Tests that the GHC engine can compress data and decompress it
 * back to the original, verifying the LZ77 bytecode pipeline.
 */
class SixlowpanGhcEngineTest : public TestCase
{
  public:
    SixlowpanGhcEngineTest();
    void DoRun() override;
};

SixlowpanGhcEngineTest::SixlowpanGhcEngineTest()
    : TestCase("GHC engine compress then decompress (in-memory)")
{
}

void
SixlowpanGhcEngineTest::DoRun()
{
    Ipv6Address srcAddr("2001:1::1");
    Ipv6Address dstAddr("2001:1::2");

    // Subtest 1: Data with lots of zeros (should compress well via zero-insert)
    {
        auto input = std::to_array<uint8_t>({0x3a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                             0x80, 0x00, 0x12, 0x34, 0x00, 0x01, 0x00, 0x01,
                                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
        const auto inputLen = static_cast<uint32_t>(input.size());

        std::array<uint8_t, 256> compressed{};
        const auto compBufLen = static_cast<uint32_t>(compressed.size());
        uint32_t compLen = SixLowPanGhcEngine::Compress(srcAddr,
                                                        dstAddr,
                                                        input.data(),
                                                        inputLen,
                                                        compressed.data(),
                                                        compBufLen,
                                                        false);

        NS_TEST_ASSERT_MSG_GT(compLen, 0u, "Subtest 1: compressor produced no output");
        NS_TEST_ASSERT_MSG_EQ(compLen, 12u, "Subtest 1: exact compressed size");
        {
            NS_TEST_ASSERT_MSG_LT(compLen, inputLen, "Compressed size should be less than input");

            std::array<uint8_t, 1280> decompressed{};
            const auto decompBufLen = static_cast<uint32_t>(decompressed.size());
            uint32_t decompLen = SixLowPanGhcEngine::Decompress(srcAddr,
                                                                dstAddr,
                                                                compressed.data(),
                                                                compLen,
                                                                decompressed.data(),
                                                                decompBufLen,
                                                                false);

            NS_TEST_ASSERT_MSG_EQ(decompLen, inputLen, "Decompressed length must match original");
            NS_TEST_ASSERT_MSG_EQ(std::memcmp(input.data(), decompressed.data(), inputLen),
                                  0,
                                  "Decompressed data must match original");
        }
    }

    // Subtest 2: Data that matches the static dictionary
    {
        // Include bytes from static dictionary: 0x16, 0xfe, 0xfd, 0x17...
        auto input = std::to_array<uint8_t>({0x16,
                                             0xfe,
                                             0xfd,
                                             0x17,
                                             0xfe,
                                             0xfd,
                                             0x00,
                                             0x01,
                                             0x00,
                                             0x00,
                                             0x00,
                                             0x00,
                                             0x00,
                                             0x01,
                                             0x00,
                                             0x00});
        const auto inputLen = static_cast<uint32_t>(input.size());

        std::array<uint8_t, 256> compressed{};
        const auto compBufLen = static_cast<uint32_t>(compressed.size());
        uint32_t compLen = SixLowPanGhcEngine::Compress(srcAddr,
                                                        dstAddr,
                                                        input.data(),
                                                        inputLen,
                                                        compressed.data(),
                                                        compBufLen,
                                                        false);

        NS_TEST_ASSERT_MSG_GT(compLen, 0u, "Subtest 2: compressor produced no output");
        NS_TEST_ASSERT_MSG_EQ(compLen, 2u, "Subtest 2: exact compressed size");
        {
            std::array<uint8_t, 1280> decompressed{};
            const auto decompBufLen = static_cast<uint32_t>(decompressed.size());
            uint32_t decompLen = SixLowPanGhcEngine::Decompress(srcAddr,
                                                                dstAddr,
                                                                compressed.data(),
                                                                compLen,
                                                                decompressed.data(),
                                                                decompBufLen,
                                                                false);

            NS_TEST_ASSERT_MSG_EQ(decompLen,
                                  inputLen,
                                  "Dictionary match: decompressed length must match");
            NS_TEST_ASSERT_MSG_EQ(std::memcmp(input.data(), decompressed.data(), inputLen),
                                  0,
                                  "Dictionary match: decompressed data must match");
        }
    }

    // Subtest 3: Data with address-derived dictionary match
    {
        // Use bytes that match the source IPv6 address (first 16 bytes of dictionary)
        std::array<uint8_t, 16> srcBuf{};
        srcAddr.GetBytes(srcBuf.data());
        const auto srcBufLen = static_cast<uint32_t>(srcBuf.size());

        std::array<uint8_t, 256> compressed{};
        const auto compBufLen = static_cast<uint32_t>(compressed.size());
        uint32_t compLen = SixLowPanGhcEngine::Compress(srcAddr,
                                                        dstAddr,
                                                        srcBuf.data(),
                                                        srcBufLen,
                                                        compressed.data(),
                                                        compBufLen,
                                                        false);

        NS_TEST_ASSERT_MSG_GT(compLen, 0u, "Subtest 3: compressor produced no output");
        NS_TEST_ASSERT_MSG_EQ(compLen, 2u, "Subtest 3: exact compressed size");
        {
            NS_TEST_ASSERT_MSG_LT(compLen, srcBufLen, "Address data should compress significantly");

            std::array<uint8_t, 1280> decompressed{};
            const auto decompBufLen = static_cast<uint32_t>(decompressed.size());
            uint32_t decompLen = SixLowPanGhcEngine::Decompress(srcAddr,
                                                                dstAddr,
                                                                compressed.data(),
                                                                compLen,
                                                                decompressed.data(),
                                                                decompBufLen,
                                                                false);

            NS_TEST_ASSERT_MSG_EQ(decompLen,
                                  srcBufLen,
                                  "Address match: decompressed length must match");
            NS_TEST_ASSERT_MSG_EQ(std::memcmp(srcBuf.data(), decompressed.data(), srcBufLen),
                                  0,
                                  "Address match: decompressed data must match");
        }
    }

    // Subtest 4: Stop code handling for extension headers (compressible input)
    {
        auto input = std::to_array<uint8_t>({0x80,
                                             0x00,
                                             0x00,
                                             0x00,
                                             0x00,
                                             0x00,
                                             0x00,
                                             0x00,
                                             0x00,
                                             0x00,
                                             0x00,
                                             0x00,
                                             0x00,
                                             0x00,
                                             0x00,
                                             0x00});
        const auto inputLen = static_cast<uint32_t>(input.size());

        std::array<uint8_t, 256> compressed{};
        const auto compBufLen = static_cast<uint32_t>(compressed.size());
        uint32_t compLen = SixLowPanGhcEngine::Compress(srcAddr,
                                                        dstAddr,
                                                        input.data(),
                                                        inputLen,
                                                        compressed.data(),
                                                        compBufLen,
                                                        true);

        NS_TEST_ASSERT_MSG_GT(compLen, 0u, "Subtest 4: compressor produced no output");
        NS_TEST_ASSERT_MSG_EQ(compLen, 4u, "Subtest 4: exact compressed size");
        {
            std::array<uint8_t, 1280> decompressed{};
            const auto decompBufLen = static_cast<uint32_t>(decompressed.size());
            uint32_t decompLen = SixLowPanGhcEngine::Decompress(srcAddr,
                                                                dstAddr,
                                                                compressed.data(),
                                                                compLen,
                                                                decompressed.data(),
                                                                decompBufLen,
                                                                true);

            NS_TEST_ASSERT_MSG_EQ(decompLen, inputLen, "Stop code: decompressed length must match");
            NS_TEST_ASSERT_MSG_EQ(std::memcmp(input.data(), decompressed.data(), inputLen),
                                  0,
                                  "Stop code: decompressed data must match");
        }
    }

    // Subtest 5: Payload larger than the IPv6 minimum MTU (1280). The engine
    // sizes its working buffers from the caller's lengths, so payloads from
    // larger-MTU links (e.g. Ethernet) must round-trip too.
    {
        constexpr uint32_t largeLen = 2000;
        std::vector<uint8_t> input(largeLen);
        for (uint32_t i = 0; i < largeLen; i++)
        {
            // Repetitive pattern with zero runs: compressible via backrefs
            input[i] = (i % 8 < 4) ? static_cast<uint8_t>(i % 4) : 0;
        }

        std::vector<uint8_t> compressed(largeLen);
        uint32_t compLen = SixLowPanGhcEngine::Compress(srcAddr,
                                                        dstAddr,
                                                        input.data(),
                                                        largeLen,
                                                        compressed.data(),
                                                        largeLen,
                                                        false);

        NS_TEST_ASSERT_MSG_GT(compLen, 0u, "Subtest 5: compressor produced no output");
        NS_TEST_ASSERT_MSG_LT(compLen, largeLen, "Subtest 5: no compression benefit");

        std::vector<uint8_t> decompressed(largeLen);
        uint32_t decompLen = SixLowPanGhcEngine::Decompress(srcAddr,
                                                            dstAddr,
                                                            compressed.data(),
                                                            compLen,
                                                            decompressed.data(),
                                                            largeLen,
                                                            false);

        NS_TEST_ASSERT_MSG_EQ(decompLen, largeLen, "Subtest 5: decompressed length mismatch");
        NS_TEST_ASSERT_MSG_EQ(std::memcmp(input.data(), decompressed.data(), largeLen),
                              0,
                              "Subtest 5: decompressed data must match original");
    }

    // Subtest 6: Bytecode classification
    {
        NS_TEST_ASSERT_MSG_EQ(static_cast<int>(SixLowPanGhcEngine::ClassifyBytecode(0x00)),
                              static_cast<int>(GhcBytecodeType::LITERAL),
                              "0x00 should be LITERAL");
        NS_TEST_ASSERT_MSG_EQ(static_cast<int>(SixLowPanGhcEngine::ClassifyBytecode(0x7F)),
                              static_cast<int>(GhcBytecodeType::LITERAL),
                              "0x7F should be LITERAL");
        NS_TEST_ASSERT_MSG_EQ(static_cast<int>(SixLowPanGhcEngine::ClassifyBytecode(0x80)),
                              static_cast<int>(GhcBytecodeType::ZERO_INSERT),
                              "0x80 should be ZERO_INSERT");
        NS_TEST_ASSERT_MSG_EQ(static_cast<int>(SixLowPanGhcEngine::ClassifyBytecode(0x8F)),
                              static_cast<int>(GhcBytecodeType::ZERO_INSERT),
                              "0x8F should be ZERO_INSERT");
        NS_TEST_ASSERT_MSG_EQ(static_cast<int>(SixLowPanGhcEngine::ClassifyBytecode(0x90)),
                              static_cast<int>(GhcBytecodeType::STOP_CODE),
                              "0x90 should be STOP_CODE");
        NS_TEST_ASSERT_MSG_EQ(static_cast<int>(SixLowPanGhcEngine::ClassifyBytecode(0xA0)),
                              static_cast<int>(GhcBytecodeType::EXTENDED_ARGS),
                              "0xA0 should be EXTENDED_ARGS");
        NS_TEST_ASSERT_MSG_EQ(static_cast<int>(SixLowPanGhcEngine::ClassifyBytecode(0xBF)),
                              static_cast<int>(GhcBytecodeType::EXTENDED_ARGS),
                              "0xBF should be EXTENDED_ARGS");
        NS_TEST_ASSERT_MSG_EQ(static_cast<int>(SixLowPanGhcEngine::ClassifyBytecode(0xC0)),
                              static_cast<int>(GhcBytecodeType::BACKREF),
                              "0xC0 should be BACKREF");
        NS_TEST_ASSERT_MSG_EQ(static_cast<int>(SixLowPanGhcEngine::ClassifyBytecode(0xFF)),
                              static_cast<int>(GhcBytecodeType::BACKREF),
                              "0xFF should be BACKREF");
    }
}

// ============================================================================
//  Test 2: GHC NHC header (serialize/deserialize)
// ============================================================================

/**
 * @ingroup sixlowpan-tests
 * @brief GHC NHC header serialize/deserialize test.
 */
class SixlowpanGhcHeaderTest : public TestCase
{
  public:
    SixlowpanGhcHeaderTest();
    void DoRun() override;
};

SixlowpanGhcHeaderTest::SixlowpanGhcHeaderTest()
    : TestCase("GHC NHC header serialize then deserialize (in-memory)")
{
}

void
SixlowpanGhcHeaderTest::DoRun()
{
    // Test GHC Extension Header
    {
        SixLowPanGhcExtension original;
        original.SetEid(SixLowPanGhcExtension::EID_HOPBYHOP_OPTIONS_H);
        original.SetNh(true);
        // 3 literal bytes + stop code
        auto blob = std::to_array<uint8_t>({0x01, 0x02, 0x03, 0x90});
        original.SetBlob(blob.data(), static_cast<uint32_t>(blob.size()));

        // Serialize
        Buffer buf;
        buf.AddAtStart(original.GetSerializedSize());
        original.Serialize(buf.Begin());

        // Deserialize
        SixLowPanGhcExtension decoded;
        decoded.Deserialize(buf.Begin());

        NS_TEST_ASSERT_MSG_EQ(decoded.GetEid(),
                              SixLowPanGhcExtension::EID_HOPBYHOP_OPTIONS_H,
                              "EID must match");
        NS_TEST_ASSERT_MSG_EQ(decoded.GetNh(), true, "NH flag must match");
        NS_TEST_ASSERT_MSG_EQ(decoded.GetBlobLength(), 4u, "Blob length must match");

        std::array<uint8_t, 256> decodedBlob{};
        decoded.CopyBlob(decodedBlob.data(), static_cast<uint32_t>(decodedBlob.size()));
        NS_TEST_ASSERT_MSG_EQ(std::memcmp(blob.data(), decodedBlob.data(), blob.size()),
                              0,
                              "Blob data must match");

        // Check dispatch type
        NS_TEST_ASSERT_MSG_EQ(decoded.GetNhcDispatchType(),
                              SixLowPanDispatch::LOWPAN_GHC_EXT,
                              "Dispatch type must be LOWPAN_GHC_EXT");
    }

    // Test GHC UDP Header
    {
        SixLowPanGhcUdp original;
        original.SetPorts(SixLowPanGhcUdp::PORTS_INLINE);
        original.SetSrcPort(1234);
        original.SetDstPort(5678);
        original.SetC(false);
        original.SetChecksum(0xABCD);

        Buffer buf;
        buf.AddAtStart(original.GetSerializedSize());
        original.Serialize(buf.Begin());

        SixLowPanGhcUdp decoded;
        decoded.Deserialize(buf.Begin());

        NS_TEST_ASSERT_MSG_EQ(decoded.GetSrcPort(), 1234u, "Src port must match");
        NS_TEST_ASSERT_MSG_EQ(decoded.GetDstPort(), 5678u, "Dst port must match");
        NS_TEST_ASSERT_MSG_EQ(decoded.GetC(), false, "C flag must match");
        NS_TEST_ASSERT_MSG_EQ(decoded.GetChecksum(), 0xABCDu, "Checksum must match");
        NS_TEST_ASSERT_MSG_EQ(decoded.GetNhcDispatchType(),
                              SixLowPanDispatch::LOWPAN_GHC_UDP,
                              "Dispatch type must be LOWPAN_GHC_UDP");
    }

    // Test GHC ICMPv6 Header
    {
        SixLowPanGhcIcmpv6 original;
        auto blob = std::to_array<uint8_t>({0x05, 0x80, 0x00, 0x12, 0x34, 0x84});
        original.SetBlob(blob.data(), static_cast<uint32_t>(blob.size()));

        Buffer buf;
        buf.AddAtStart(original.GetSerializedSize());
        original.Serialize(buf.Begin());

        SixLowPanGhcIcmpv6 decoded;
        decoded.Deserialize(buf.Begin(), buf.End());

        NS_TEST_ASSERT_MSG_EQ(decoded.GetBlobLength(), 6u, "Blob length must match");
        NS_TEST_ASSERT_MSG_EQ(decoded.GetNhcDispatchType(),
                              SixLowPanDispatch::LOWPAN_GHC_ICMPV6,
                              "Dispatch type must be LOWPAN_GHC_ICMPV6");

        std::array<uint8_t, 256> decodedBlob{};
        decoded.CopyBlob(decodedBlob.data(), static_cast<uint32_t>(decodedBlob.size()));
        NS_TEST_ASSERT_MSG_EQ(std::memcmp(blob.data(), decodedBlob.data(), blob.size()),
                              0,
                              "Blob data must match");
    }

    // Test 6CIO Option Header
    {
        SixLowPan6Cio original;
        original.SetGhcCapable(true);

        Buffer buf;
        buf.AddAtStart(original.GetSerializedSize());
        original.Serialize(buf.Begin());

        SixLowPan6Cio decoded;
        decoded.Deserialize(buf.Begin());

        NS_TEST_ASSERT_MSG_EQ(decoded.GetGhcCapable(), true, "GHC capable flag must match");
    }

    // Test dispatch type recognition
    {
        NS_TEST_ASSERT_MSG_EQ(SixLowPanDispatch::GetNhcDispatchType(0xB0),
                              SixLowPanDispatch::LOWPAN_GHC_EXT,
                              "0xB0 must be GHC_EXT");
        NS_TEST_ASSERT_MSG_EQ(SixLowPanDispatch::GetNhcDispatchType(0xBF),
                              SixLowPanDispatch::LOWPAN_GHC_EXT,
                              "0xBF must be GHC_EXT");
        NS_TEST_ASSERT_MSG_EQ(SixLowPanDispatch::GetNhcDispatchType(0xD0),
                              SixLowPanDispatch::LOWPAN_GHC_UDP,
                              "0xD0 must be GHC_UDP");
        NS_TEST_ASSERT_MSG_EQ(SixLowPanDispatch::GetNhcDispatchType(0xD7),
                              SixLowPanDispatch::LOWPAN_GHC_UDP,
                              "0xD7 must be GHC_UDP");
        NS_TEST_ASSERT_MSG_EQ(SixLowPanDispatch::GetNhcDispatchType(0xDF),
                              SixLowPanDispatch::LOWPAN_GHC_ICMPV6,
                              "0xDF must be GHC_ICMPV6");
        // Ensure standard NHC is still recognized
        NS_TEST_ASSERT_MSG_EQ(SixLowPanDispatch::GetNhcDispatchType(0xE0),
                              SixLowPanDispatch::LOWPAN_NHC,
                              "0xE0 must still be NHC");
        NS_TEST_ASSERT_MSG_EQ(SixLowPanDispatch::GetNhcDispatchType(0xF0),
                              SixLowPanDispatch::LOWPAN_UDPNHC,
                              "0xF0 must still be UDPNHC");
    }
}

// ============================================================================
//  Test 3: End-to-End GHC UDP Compression Test
// ============================================================================

/**
 * @ingroup sixlowpan-tests
 * @brief 6LoWPAN GHC end-to-end UDP test.
 *
 * Sends a UDP packet between two nodes with GHC enabled and verifies
 * the packet arrives intact after compression + decompression.
 */
class SixlowpanGhcUdpImplTest : public TestCase
{
    Ptr<Packet> m_receivedPacket; ///< The received packet.

    /**
     * @brief Schedule-friendly wrapper that performs the actual send.
     * @param [in] socket The sending socket.
     * @param [in] to Destination address in text form.
     */
    void DoSendData(Ptr<Socket> socket, std::string to);

    /**
     * @brief Schedule data to be sent through the socket.
     * @param [in] socket The sending socket.
     * @param [in] to Destination address in text form.
     */
    void SendData(Ptr<Socket> socket, std::string to);

  public:
    void DoRun() override;
    SixlowpanGhcUdpImplTest();

    /**
     * @brief Receive data callback.
     * @param [in] socket The receiving socket.
     * @param [in] packet The received packet.
     * @param [in] from The sender address.
     */
    void ReceivePacket(Ptr<Socket> socket, Ptr<Packet> packet, const Address& from);

    /**
     * @brief Receive data from the socket's RX buffer.
     * @param [in] socket The receiving socket.
     */
    void ReceivePkt(Ptr<Socket> socket);
};

SixlowpanGhcUdpImplTest::SixlowpanGhcUdpImplTest()
    : TestCase("Sixlowpan GHC UDP end-to-end")
{
}

void
SixlowpanGhcUdpImplTest::ReceivePacket(Ptr<Socket> socket, Ptr<Packet> packet, const Address& from)
{
    m_receivedPacket = packet;
}

void
SixlowpanGhcUdpImplTest::ReceivePkt(Ptr<Socket> socket)
{
    uint32_t availableData [[maybe_unused]] = socket->GetRxAvailable();
    m_receivedPacket = socket->Recv(std::numeric_limits<uint32_t>::max(), 0);
    NS_ASSERT(availableData == m_receivedPacket->GetSize());
}

void
SixlowpanGhcUdpImplTest::DoSendData(Ptr<Socket> socket, std::string to)
{
    Address realTo = Inet6SocketAddress(Ipv6Address(to.c_str()), 1234);
    const char* msg = "GHC RFC 7400 compression test - Generic Header Compression for "
                      "6LoWPAN IPv6 over Low-Power Wireless Personal Area Networks.";
    std::array<uint8_t, 128> buffer{};
    std::memcpy(buffer.data(), msg, std::strlen(msg));

    Ptr<Packet> packet = Create<Packet>(buffer.data(), static_cast<uint32_t>(buffer.size()));
    NS_TEST_EXPECT_MSG_EQ(socket->SendTo(packet, 0, realTo),
                          static_cast<int>(buffer.size()),
                          "Send should succeed");
}

void
SixlowpanGhcUdpImplTest::SendData(Ptr<Socket> socket, std::string to)
{
    m_receivedPacket = Create<Packet>();
    Simulator::ScheduleWithContext(socket->GetNode()->GetId(),
                                   Seconds(0),
                                   &SixlowpanGhcUdpImplTest::DoSendData,
                                   this,
                                   socket,
                                   to);
    Simulator::Run();
}

void
SixlowpanGhcUdpImplTest::DoRun()
{
    InternetStackHelper internet;
    internet.SetIpv4StackInstall(false);

    // Receiver Node
    Ptr<Node> rxNode = CreateObject<Node>();
    internet.Install(rxNode);
    Ptr<SimpleNetDevice> rxDev;
    {
        rxDev = CreateObject<SimpleNetDevice>();
        rxDev->SetAddress(Mac48Address::ConvertFrom(Mac48Address::Allocate()));
        rxNode->AddDevice(rxDev);

        Ptr<SixLowPanNetDevice> rxSix = CreateObject<SixLowPanNetDevice>();
        rxSix->SetAttribute("CompressionType", EnumValue(SixLowPanNetDevice::GHC));
        rxNode->AddDevice(rxSix);
        rxSix->SetNetDevice(rxDev);

        Ptr<Ipv6> ipv6 = rxNode->GetObject<Ipv6>();
        ipv6->AddInterface(rxDev);
        uint32_t netdev_idx = ipv6->AddInterface(rxSix);
        Ipv6InterfaceAddress ipv6Addr =
            Ipv6InterfaceAddress(Ipv6Address("2001:0100::1"), Ipv6Prefix(64));
        ipv6->AddAddress(netdev_idx, ipv6Addr);
        ipv6->SetUp(netdev_idx);
    }

    // Sender Node
    Ptr<Node> txNode = CreateObject<Node>();
    internet.Install(txNode);
    Ptr<SimpleNetDevice> txDev;
    {
        txDev = CreateObject<SimpleNetDevice>();
        txDev->SetAddress(Mac48Address::ConvertFrom(Mac48Address::Allocate()));
        txNode->AddDevice(txDev);

        Ptr<SixLowPanNetDevice> txSix = CreateObject<SixLowPanNetDevice>();
        txSix->SetAttribute("CompressionType", EnumValue(SixLowPanNetDevice::GHC));
        txNode->AddDevice(txSix);
        txSix->SetNetDevice(txDev);

        Ptr<Ipv6> ipv6 = txNode->GetObject<Ipv6>();
        ipv6->AddInterface(txDev);
        uint32_t netdev_idx = ipv6->AddInterface(txSix);
        Ipv6InterfaceAddress ipv6Addr =
            Ipv6InterfaceAddress(Ipv6Address("2001:0100::2"), Ipv6Prefix(64));
        ipv6->AddAddress(netdev_idx, ipv6Addr);
        ipv6->SetUp(netdev_idx);
    }

    // Link the two nodes
    Ptr<SimpleChannel> channel1 = CreateObject<SimpleChannel>();
    rxDev->SetChannel(channel1);
    txDev->SetChannel(channel1);

    // Create the UDP sockets
    Ptr<SocketFactory> rxSocketFactory = rxNode->GetObject<UdpSocketFactory>();
    Ptr<Socket> rxSocket = rxSocketFactory->CreateSocket();
    NS_TEST_EXPECT_MSG_EQ(rxSocket->Bind(Inet6SocketAddress(Ipv6Address("2001:0100::1"), 1234)),
                          0,
                          "trivial");
    rxSocket->SetRecvCallback(MakeCallback(&SixlowpanGhcUdpImplTest::ReceivePkt, this));

    Ptr<SocketFactory> txSocketFactory = txNode->GetObject<UdpSocketFactory>();
    Ptr<Socket> txSocket = txSocketFactory->CreateSocket();
    txSocket->SetAllowBroadcast(true);

    // Unicast test
    SendData(txSocket, "2001:0100::1");
    NS_TEST_EXPECT_MSG_EQ(m_receivedPacket->GetSize(), 128, "Received packet size must match");

    std::array<uint8_t, 128> rxBuffer{};
    std::array<uint8_t, 128> txBuffer{};
    const char* msg = "GHC RFC 7400 compression test - Generic Header Compression for "
                      "6LoWPAN IPv6 over Low-Power Wireless Personal Area Networks.";
    std::memcpy(txBuffer.data(), msg, std::strlen(msg));
    m_receivedPacket->CopyData(rxBuffer.data(), static_cast<uint32_t>(rxBuffer.size()));
    NS_TEST_EXPECT_MSG_EQ((rxBuffer == txBuffer), true, "Received data must match sent data");

    m_receivedPacket->RemoveAllByteTags();
    Simulator::Destroy();
}

// ============================================================================
//  Test 4: RFC 7400 Appendix A Test Vectors
// ============================================================================

/**
 * @ingroup sixlowpan-tests
 * @brief RFC 7400 Appendix A test vectors (Figures 8 - 17).
 *
 * For every one of the ten worked examples in RFC 7400 Appendix A the test
 * carries three reference byte arrays: the source/destination IPv6 addresses
 * (extracted from the example's IP header, seeding the GHC dictionary), the
 * original uncompressed payload (RefUncomp), and the RFC-provided compressed
 * form (RefComp).
 *
 * For each vector, the test runs two steps in the order suggested by the
 * RFC 7400 authors:
 *
 *   1. Compress(RefUncomp) -> MyComp. Because RFC 7400 LZ77 does not mandate
 *      a unique encoding (different compressors may select different but
 *      equally valid matches), MyComp is not required to match RefComp
 *      byte-for-byte. The test only asserts that MyComp is non-empty and
 *      fits in the output buffer.
 *
 *   2. Decompress(MyComp) -> MyUncomp. Decompression must recover
 *      RefUncomp byte-for-byte: this is the primary correctness check for
 *      codec consistency and is strict.
 *
 * A third independent check decompresses RefComp (the RFC-provided stream)
 * and verifies that it recovers RefUncomp; this validates interoperability
 * of the decompressor with third-party (and typically optimal) encodings.
 */
class SixlowpanGhcAppendixATest : public TestCase
{
  public:
    SixlowpanGhcAppendixATest();
    void DoRun() override;

  private:
    /**
     * @brief One RFC 7400 Appendix A reference vector.
     *
     * Uses std::vector<uint8_t> for the byte arrays so that the compressed
     * and uncompressed sizes travel with the data (per RFC 7400 guidance:
     * lexicographic std::vector comparison makes size + content checks
     * implicit).
     */
    struct Vector
    {
        /// Figure number / short description.
        std::string label;
        /// Source IPv6 address (text form).
        std::string src;
        /// Destination IPv6 address (text form).
        std::string dst;
        /// Original (uncompressed) payload.
        std::vector<uint8_t> payload;
        /// RFC-provided compressed bytes.
        std::vector<uint8_t> compressed;
    };

    /**
     * @brief Run one reference vector through Compress and Decompress.
     * @param [in] v The reference vector.
     */
    void RunVector(const Vector& v);
};

SixlowpanGhcAppendixATest::SixlowpanGhcAppendixATest()
    : TestCase("GHC RFC 7400 Appendix A test vectors (Figures 8-17)")
{
}

void
SixlowpanGhcAppendixATest::RunVector(const Vector& v)
{
    Ipv6Address srcAddr(v.src.c_str());
    Ipv6Address dstAddr(v.dst.c_str());
    const std::string& tag = v.label;

    // --- Step 1: Compress RefUncomp into MyComp ------------------------------
    // Per RFC 7400, LZ77 is not a unique encoding: different compressors can
    // produce different but equally valid compressed streams. MyComp is
    // therefore not required to match RefComp byte-for-byte; we only assert
    // that it is non-empty and fits in the output buffer.
    std::array<uint8_t, 512> myCompBuf{};
    const auto myPayloadLen = static_cast<uint32_t>(v.payload.size());
    const auto myCompBufLen = static_cast<uint32_t>(myCompBuf.size());
    uint32_t myCompLen = SixLowPanGhcEngine::Compress(srcAddr,
                                                      dstAddr,
                                                      v.payload.data(),
                                                      myPayloadLen,
                                                      myCompBuf.data(),
                                                      myCompBufLen);
    NS_TEST_EXPECT_MSG_GT(myCompLen, 0u, tag + ": compressor produced empty output");
    NS_TEST_EXPECT_MSG_LT(myCompLen, myCompBufLen, tag + ": compressed fits in buffer");

    std::vector<uint8_t> myComp(myCompBuf.begin(), myCompBuf.begin() + myCompLen);

    // --- Step 2: Decompress MyComp into MyUncomp; must equal RefUncomp ------
    // This compress/decompress recovery is the primary correctness check. The
    // codec must recover the original payload byte-for-byte regardless of
    // whether MyComp matches RefComp exactly.
    std::array<uint8_t, 512> myUncompBuf{};
    const auto myUncompBufLen = static_cast<uint32_t>(myUncompBuf.size());
    uint32_t myUncompLen = SixLowPanGhcEngine::Decompress(srcAddr,
                                                          dstAddr,
                                                          myComp.data(),
                                                          myCompLen,
                                                          myUncompBuf.data(),
                                                          myUncompBufLen);
    std::vector<uint8_t> myUncomp(myUncompBuf.begin(), myUncompBuf.begin() + myUncompLen);
    NS_TEST_EXPECT_MSG_EQ((myUncomp == v.payload),
                          true,
                          tag + ": decompression must recover original payload");

    // --- Step 3: Decompress RefComp; must equal RefUncomp --------------------
    // Independent interop check: our decompressor must handle the RFC-
    // provided (optimal) encoding produced by a third-party compressor.
    std::array<uint8_t, 512> refUncompBuf{};
    const auto refCompLen = static_cast<uint32_t>(v.compressed.size());
    const auto refUncompBufLen = static_cast<uint32_t>(refUncompBuf.size());
    uint32_t refUncompLen = SixLowPanGhcEngine::Decompress(srcAddr,
                                                           dstAddr,
                                                           v.compressed.data(),
                                                           refCompLen,
                                                           refUncompBuf.data(),
                                                           refUncompBufLen);
    std::vector<uint8_t> refUncomp(refUncompBuf.begin(), refUncompBuf.begin() + refUncompLen);
    NS_TEST_EXPECT_MSG_EQ((refUncomp == v.payload),
                          true,
                          tag + ": decompressed RFC reference must equal payload");
}

void
SixlowpanGhcAppendixATest::DoRun()
{
    // clang-format off
    const std::vector<Vector> vectors = {
        // ---- Figure 8: A Simple RPL Example ----------------------------------
        {"Fig 8 (RPL simple)",
         "fe80::21c:daff:fe00:2024",
         "ff02::1a",
         {0x9b, 0x00, 0x6b, 0xde, 0x00, 0x00, 0x00, 0x00},
         {0x04, 0x9b, 0x00, 0x6b, 0xde, 0x82}},

        // ---- Figure 9: A Longer RPL Example ----------------------------------
        {"Fig 9 (RPL longer)",
         "fe80::21c:daff:fe00:3023",
         "ff02::1a",
         {0x9b, 0x01, 0x7a, 0x5f, 0x00, 0xf0, 0x01, 0x00, 0x88, 0x00, 0x00, 0x00,
          0x20, 0x02, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
          0xfe, 0x00, 0xfa, 0xce, 0x04, 0x0e, 0x00, 0x14, 0x09, 0xff, 0x00, 0x00,
          0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x1e, 0x80, 0x20,
          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
          0x20, 0x02, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
          0xfe, 0x00, 0xfa, 0xce, 0x03, 0x0e, 0x40, 0x00, 0xff, 0xff, 0xff, 0xff,
          0x20, 0x02, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00},
         {0x06, 0x9b, 0x01, 0x7a, 0x5f, 0x00, 0xf0, 0xc7, 0x01, 0x88, 0x81, 0x04,
          0x20, 0x02, 0x0d, 0xb8, 0x85, 0xa7, 0xc9, 0x08, 0xfa, 0xce, 0x04, 0x0e,
          0x00, 0x14, 0x09, 0xff, 0xa4, 0xda, 0x83, 0x06, 0x08, 0x1e, 0x80, 0x20,
          0xff, 0xff, 0xc0, 0xd0, 0x82, 0xb4, 0xf0, 0x03, 0x03, 0x0e, 0x40, 0xc7,
          0xa3, 0xc9, 0xa2, 0xf0}},

        // ---- Figure 10: A RPL DAO Message ------------------------------------
        {"Fig 10 (RPL DAO)",
         "2002:db8::ff:fe00:3344",
         "2002:db8::ff:fe00:1122",
         {0x9b, 0x02, 0x58, 0x7d, 0x01, 0x80, 0x00, 0xf1, 0x05, 0x12, 0x00, 0x80,
          0x20, 0x02, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
          0xfe, 0x00, 0x33, 0x44, 0x06, 0x14, 0x00, 0x80, 0xf1, 0x00, 0xfe, 0x80,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xfe, 0x00,
          0x11, 0x22},
         {0x0c, 0x9b, 0x02, 0x58, 0x7d, 0x01, 0x80, 0x00, 0xf1, 0x05, 0x12, 0x00,
          0x80, 0xb5, 0xf4, 0x08, 0x06, 0x14, 0x00, 0x80, 0xf1, 0x00, 0xfe, 0x80,
          0x87, 0xa7, 0xdd}},

        // ---- Figure 11: An ND Neighbor Solicitation --------------------------
        {"Fig 11 (ND NS)",
         "2002:db8::ff:fe00:3bd3",
         "fe80::21c:daff:fe00:3023",
         {0x87, 0x00, 0xa7, 0x68, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x80, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x02, 0x1c, 0xda, 0xff, 0xfe, 0x00, 0x30, 0x23,
          0x01, 0x01, 0x3b, 0xd3, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x02, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x06, 0x00, 0x1c, 0xda, 0xff, 0xfe, 0x00, 0x20, 0x24},
         {0x04, 0x87, 0x00, 0xa7, 0x68, 0x82, 0xb3, 0xf0, 0x04, 0x01, 0x01, 0x3b,
          0xd3, 0x82, 0x02, 0x1f, 0x02, 0x83, 0x02, 0x06, 0x00, 0xa2, 0xdb, 0x02,
          0x20, 0x24}},

        // ---- Figure 12: An ND Neighbor Advertisement -------------------------
        {"Fig 12 (ND NA)",
         "fe80::21c:daff:fe00:3023",
         "2002:db8::ff:fe00:3bd3",
         {0x88, 0x00, 0x26, 0x6c, 0xc0, 0x00, 0x00, 0x00, 0xfe, 0x80, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x02, 0x1c, 0xda, 0xff, 0xfe, 0x00, 0x30, 0x23,
          0x02, 0x01, 0xfa, 0xce, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x02, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x06, 0x00, 0x1c, 0xda, 0xff, 0xfe, 0x00, 0x20, 0x24},
         {0x05, 0x88, 0x00, 0x26, 0x6c, 0xc0, 0x81, 0xb5, 0xf0, 0x04, 0x02, 0x01,
          0xfa, 0xce, 0x82, 0x02, 0x1f, 0x02, 0x83, 0x02, 0x06, 0x00, 0xa2, 0xdb,
          0x02, 0x20, 0x24}},

        // ---- Figure 13: An ND Router Solicitation ----------------------------
        {"Fig 13 (ND RS)",
         "fe80::aede:4800:0:1",
         "ff02::2",
         {0x85, 0x00, 0x90, 0x65, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0xac, 0xde,
          0x48, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
         {0x04, 0x85, 0x00, 0x90, 0x65, 0xde, 0x02, 0x02, 0xac, 0xa5, 0xeb, 0x84}},

        // ---- Figure 14: An ND Router Advertisement ---------------------------
        {"Fig 14 (ND RA)",
         "fe80::1034:ff:fe00:1122",
         "fe80::aede:4800:0:1",
         {0x86, 0x00, 0x55, 0xc9, 0x40, 0x00, 0x0f, 0xa0, 0x1c, 0x5a, 0x38, 0x17,
          0x00, 0x00, 0x07, 0xd0, 0x01, 0x01, 0x11, 0x22, 0x00, 0x00, 0x00, 0x00,
          0x03, 0x04, 0x40, 0x40, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
          0x00, 0x00, 0x00, 0x00, 0x20, 0x02, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x02, 0x40, 0x10,
          0x00, 0x00, 0x03, 0xe8, 0x20, 0x02, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
          0x21, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x20, 0x02, 0x0d, 0xb8,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xfe, 0x00, 0x11, 0x22},
         {0x0c, 0x86, 0x00, 0x55, 0xc9, 0x40, 0x00, 0x0f, 0xa0, 0x1c, 0x5a, 0x38,
          0x17, 0x80, 0x06, 0x07, 0xd0, 0x01, 0x01, 0x11, 0x22, 0x82, 0x06, 0x03,
          0x04, 0x40, 0x40, 0xff, 0xff, 0xc0, 0xd0, 0x82, 0x04, 0x20, 0x02, 0x0d,
          0xb8, 0x8a, 0x04, 0x20, 0x02, 0x40, 0x10, 0xa4, 0xcb, 0x01, 0xe8, 0xa2,
          0xf0, 0x02, 0x21, 0x03, 0xa9, 0xe6, 0xb3, 0xcd, 0xaf, 0xdb}},

        // ---- Figure 15: A DTLS Application Data Packet -----------------------
        {"Fig 15 (DTLS AppData)",
         "::",
         "::",
         {0x17, 0xfe, 0xfd, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
          0x1d, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x09, 0xb2, 0x0e,
          0x82, 0xc1, 0x6e, 0xb6, 0x96, 0xc5, 0x1f, 0x36, 0x8d, 0x17, 0x61, 0xe2,
          0xb5, 0xd4, 0x22, 0xd4, 0xed, 0x2b},
         {0xb0, 0xd1, 0x01, 0x1d, 0xf2, 0x15, 0x09, 0xb2, 0x0e, 0x82, 0xc1, 0x6e,
          0xb6, 0x96, 0xc5, 0x1f, 0x36, 0x8d, 0x17, 0x61, 0xe2, 0xb5, 0xd4, 0x22,
          0xd4, 0xed, 0x2b}},

        // ---- Figure 16: Another DTLS Application Data Packet -----------------
        {"Fig 16 (DTLS AppData 2)",
         "::",
         "::",
         {0x17, 0xfe, 0xfd, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00,
          0x16, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0xae, 0xa0, 0x15,
          0x56, 0x67, 0x92, 0x4d, 0xff, 0x8a, 0x24, 0xe4, 0xcb, 0x35, 0xb9},
         {0xb0, 0xc3, 0x03, 0x05, 0x00, 0x16, 0xf2, 0x0e, 0xae, 0xa0, 0x15, 0x56,
          0x67, 0x92, 0x4d, 0xff, 0x8a, 0x24, 0xe4, 0xcb, 0x35, 0xb9}},

        // ---- Figure 17: A DTLS Handshake Packet (Client Hello) ---------------
        {"Fig 17 (DTLS ClientHello)",
         "::",
         "::",
         {0x16, 0xfe, 0xfd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x36, 0x01, 0x00, 0x00, 0x2a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x2a, 0xfe, 0xfd, 0x51, 0x52, 0xed, 0x79, 0xa4, 0x20, 0xc9, 0x62, 0x56,
          0x11, 0x47, 0xc9, 0x39, 0xee, 0x6c, 0xc0, 0xa4, 0xfe, 0xc6, 0x89, 0x2f,
          0x32, 0x26, 0x9a, 0x16, 0x4e, 0x31, 0x7e, 0x9f, 0x20, 0x92, 0x92, 0x00,
          0x00, 0x00, 0x02, 0xc0, 0xa8, 0x01, 0x00},
         {0xa1, 0xcd, 0x87, 0x01, 0x36, 0xa1, 0xcd, 0x01, 0x2a, 0x85, 0x23, 0x2a,
          0xfe, 0xfd, 0x51, 0x52, 0xed, 0x79, 0xa4, 0x20, 0xc9, 0x62, 0x56, 0x11,
          0x47, 0xc9, 0x39, 0xee, 0x6c, 0xc0, 0xa4, 0xfe, 0xc6, 0x89, 0x2f, 0x32,
          0x26, 0x9a, 0x16, 0x4e, 0x31, 0x7e, 0x9f, 0x20, 0x92, 0x92, 0x81, 0x05,
          0x02, 0xc0, 0xa8, 0x01, 0x00}},
    };
    // clang-format on

    for (const auto& v : vectors)
    {
        RunVector(v);
    }
}

// ============================================================================
//  Test Suite
// ============================================================================

/**
 * @ingroup sixlowpan-tests
 * @brief 6LoWPAN GHC (RFC 7400) Test Suite.
 */
class SixlowpanGhcTestSuite : public TestSuite
{
  public:
    SixlowpanGhcTestSuite();
};

SixlowpanGhcTestSuite::SixlowpanGhcTestSuite()
    : TestSuite("sixlowpan-ghc", Type::UNIT)
{
    AddTestCase(new SixlowpanGhcEngineTest(), TestCase::Duration::QUICK);
    AddTestCase(new SixlowpanGhcHeaderTest(), TestCase::Duration::QUICK);
    AddTestCase(new SixlowpanGhcUdpImplTest(), TestCase::Duration::QUICK);
    AddTestCase(new SixlowpanGhcAppendixATest(), TestCase::Duration::QUICK);
}

static SixlowpanGhcTestSuite g_sixlowpanGhcTestSuite;
