/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Pavel Boyko <boyko@iitp.ru>
 */

#include "ns3/flame-header.h"
#include "ns3/flame-rtable.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

using namespace ns3;
using namespace flame;

/**
 * @ingroup flame-test
 *
 * @brief Built-in self test for FlameHeader
 */
struct FlameHeaderTest : public TestCase
{
    FlameHeaderTest()
        : TestCase("FlameHeader roundtrip serialization")
    {
    }

    void DoRun() override;
};

void
FlameHeaderTest::DoRun()
{
    FlameHeader a;
    a.AddCost(123);
    a.SetSeqno(456);
    a.SetOrigDst(Mac48Address("11:22:33:44:55:66"));
    a.SetOrigSrc(Mac48Address("00:11:22:33:44:55"));
    a.SetProtocol(0x806);
    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(a);
    FlameHeader b;
    packet->RemoveHeader(b);
    NS_TEST_ASSERT_MSG_EQ(b, a, "FlameHeader roundtrip serialization works");
}

//-----------------------------------------------------------------------------

/**
 * @ingroup flame-test
 *
 * @brief Unit test for FlameRtable
 */
class FlameRtableTest : public TestCase
{
  public:
    FlameRtableTest();
    void DoRun() override;

  private:
    /// Test Add apth and lookup path;
    void TestLookup();

    /// Test add path and try to lookup after entry has expired
    void TestAddPath();
    /// Test add path and try to lookup after entry has expired
    void TestExpire();

  private:
    Mac48Address dst;       ///< destination address
    Mac48Address hop;       ///< hop address
    uint32_t iface;         ///< interface
    uint8_t cost;           ///< cost
    uint16_t seqnum;        ///< sequence number
    Ptr<FlameRtable> table; ///< table
};

/// Test instance
static FlameRtableTest g_FlameRtableTest;

FlameRtableTest::FlameRtableTest()
    : TestCase("FlameRtable"),
      dst("01:00:00:01:00:01"),
      hop("01:00:00:01:00:03"),
      iface(8010),
      cost(10),
      seqnum(1)
{
}

void
FlameRtableTest::TestLookup()
{
    FlameRtable::LookupResult correct(hop, iface, cost, seqnum);

    table->AddPath(dst, hop, iface, cost, seqnum);
    NS_TEST_EXPECT_MSG_EQ((table->Lookup(dst) == correct), true, "Routing table lookup works");
}

void
FlameRtableTest::TestAddPath()
{
    table->AddPath(dst, hop, iface, cost, seqnum);
}

void
FlameRtableTest::TestExpire()
{
    // this is assumed to be called when path records are already expired
    FlameRtable::LookupResult correct(hop, iface, cost, seqnum);
    NS_TEST_EXPECT_MSG_EQ(table->Lookup(dst).IsValid(),
                          false,
                          "Routing table records expirations works");
}

void
FlameRtableTest::DoRun()
{
    table = CreateObject<FlameRtable>();

    Simulator::Schedule(Seconds(0), &FlameRtableTest::TestLookup, this);
    Simulator::Schedule(Seconds(1), &FlameRtableTest::TestAddPath, this);
    Simulator::Schedule(Seconds(122), &FlameRtableTest::TestExpire, this);

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup flame-test
 *
 * @brief Flame Test Suite
 */
class FlameTestSuite : public TestSuite
{
  public:
    FlameTestSuite();
};

FlameTestSuite::FlameTestSuite()
    : TestSuite("devices-mesh-flame", Type::UNIT)
{
    AddTestCase(new FlameHeaderTest, TestCase::Duration::QUICK);
    AddTestCase(new FlameRtableTest, TestCase::Duration::QUICK);
}

static FlameTestSuite g_flameTestSuite; ///< the test suite
