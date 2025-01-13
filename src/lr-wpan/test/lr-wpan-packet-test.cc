/*
 * Copyright (c) 2011 The Boeing Company
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Tom Henderson <thomas.r.henderson@boeing.com>
 */
#include "ns3/log.h"
#include "ns3/lr-wpan-mac-header.h"
#include "ns3/lr-wpan-mac-trailer.h"
#include "ns3/mac16-address.h"
#include "ns3/mac64-address.h"
#include "ns3/packet.h"
#include "ns3/test.h"

#include <vector>

using namespace ns3;
using namespace ns3::lrwpan;

NS_LOG_COMPONENT_DEFINE("lr-wpan-packet-test");

/**
 * @ingroup lr-wpan-test
 * @ingroup tests
 *
 * @brief LrWpan header and trailer Test
 */
class LrWpanPacketTestCase : public TestCase
{
  public:
    LrWpanPacketTestCase();
    ~LrWpanPacketTestCase() override;

  private:
    void DoRun() override;
};

LrWpanPacketTestCase::LrWpanPacketTestCase()
    : TestCase("Test the 802.15.4 MAC header and trailer classes")
{
}

LrWpanPacketTestCase::~LrWpanPacketTestCase()
{
}

void
LrWpanPacketTestCase::DoRun()
{
    LrWpanMacHeader macHdr(LrWpanMacHeader::LRWPAN_MAC_BEACON, 0); // sequence number set to 0
    macHdr.SetSrcAddrMode(LrWpanMacHeader::SHORTADDR);             // short addr
    macHdr.SetDstAddrMode(LrWpanMacHeader::NOADDR);
    macHdr.SetSecDisable();
    macHdr.SetNoPanIdComp();
    // ... other setters

    uint16_t srcPanId = 100;
    Mac16Address srcWpanAddr("00:11");
    macHdr.SetSrcAddrFields(srcPanId, srcWpanAddr);

    LrWpanMacTrailer macTrailer;

    Ptr<Packet> p = Create<Packet>(20); // 20 bytes of dummy data
    NS_TEST_ASSERT_MSG_EQ(p->GetSize(), 20, "Packet created with unexpected size");
    p->AddHeader(macHdr);
    std::cout << " <--Mac Header added " << std::endl;

    NS_TEST_ASSERT_MSG_EQ(p->GetSize(), 27, "Packet wrong size after macHdr addition");
    p->AddTrailer(macTrailer);
    NS_TEST_ASSERT_MSG_EQ(p->GetSize(), 29, "Packet wrong size after macTrailer addition");

    // Test serialization and deserialization
    uint32_t size = p->GetSerializedSize();
    std::vector<uint8_t> buffer(size);
    p->Serialize(buffer.data(), size);
    Ptr<Packet> p2 = Create<Packet>(buffer.data(), size, true);

    p2->Print(std::cout);
    std::cout << " <--Packet P2 " << std::endl;

    NS_TEST_ASSERT_MSG_EQ(p2->GetSize(), 29, "Packet wrong size after deserialization");

    LrWpanMacHeader receivedMacHdr;
    p2->RemoveHeader(receivedMacHdr);

    receivedMacHdr.Print(std::cout);
    std::cout << " <--P2 Mac Header " << std::endl;

    NS_TEST_ASSERT_MSG_EQ(p2->GetSize(), 22, "Packet wrong size after removing machdr");

    LrWpanMacTrailer receivedMacTrailer;
    p2->RemoveTrailer(receivedMacTrailer);
    NS_TEST_ASSERT_MSG_EQ(p2->GetSize(),
                          20,
                          "Packet wrong size after removing headers and trailers");
    // Compare macHdr with receivedMacHdr, macTrailer with receivedMacTrailer,...
}

/**
 * @ingroup lr-wpan-test
 * @ingroup tests
 *
 * @brief LrWpan header and trailer TestSuite
 */
class LrWpanPacketTestSuite : public TestSuite
{
  public:
    LrWpanPacketTestSuite();
};

LrWpanPacketTestSuite::LrWpanPacketTestSuite()
    : TestSuite("lr-wpan-packet", Type::UNIT)
{
    AddTestCase(new LrWpanPacketTestCase, TestCase::Duration::QUICK);
}

static LrWpanPacketTestSuite g_lrWpanPacketTestSuite; //!< Static variable for test initialization
