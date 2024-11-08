/*
 * Copyright (c) 2010 Hemanth Narra
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Hemanth Narra <hemanth@ittc.ku.com>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  https://resilinets.org/
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/dsdv-helper.h"
#include "ns3/dsdv-packet.h"
#include "ns3/dsdv-rtable.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/mesh-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/pcap-file.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"

using namespace ns3;

/**
 * @ingroup dsdv
 * @ingroup tests
 * @defgroup dsdv-test DSDV module tests
 */

/**
 * @ingroup dsdv-test
 *
 * @brief DSDV test case to verify the DSDV header
 *
 */
class DsdvHeaderTestCase : public TestCase
{
  public:
    DsdvHeaderTestCase();
    ~DsdvHeaderTestCase() override;
    void DoRun() override;
};

DsdvHeaderTestCase::DsdvHeaderTestCase()
    : TestCase("Verifying the DSDV header")
{
}

DsdvHeaderTestCase::~DsdvHeaderTestCase()
{
}

void
DsdvHeaderTestCase::DoRun()
{
    Ptr<Packet> packet = Create<Packet>();

    {
        dsdv::DsdvHeader hdr1;
        hdr1.SetDst(Ipv4Address("10.1.1.2"));
        hdr1.SetDstSeqno(2);
        hdr1.SetHopCount(2);
        packet->AddHeader(hdr1);
        dsdv::DsdvHeader hdr2;
        hdr2.SetDst(Ipv4Address("10.1.1.3"));
        hdr2.SetDstSeqno(4);
        hdr2.SetHopCount(1);
        packet->AddHeader(hdr2);
        NS_TEST_ASSERT_MSG_EQ(packet->GetSize(), 24, "001");
    }

    {
        dsdv::DsdvHeader hdr2;
        packet->RemoveHeader(hdr2);
        NS_TEST_ASSERT_MSG_EQ(hdr2.GetSerializedSize(), 12, "002");
        NS_TEST_ASSERT_MSG_EQ(hdr2.GetDst(), Ipv4Address("10.1.1.3"), "003");
        NS_TEST_ASSERT_MSG_EQ(hdr2.GetDstSeqno(), 4, "004");
        NS_TEST_ASSERT_MSG_EQ(hdr2.GetHopCount(), 1, "005");
        dsdv::DsdvHeader hdr1;
        packet->RemoveHeader(hdr1);
        NS_TEST_ASSERT_MSG_EQ(hdr1.GetSerializedSize(), 12, "006");
        NS_TEST_ASSERT_MSG_EQ(hdr1.GetDst(), Ipv4Address("10.1.1.2"), "008");
        NS_TEST_ASSERT_MSG_EQ(hdr1.GetDstSeqno(), 2, "009");
        NS_TEST_ASSERT_MSG_EQ(hdr1.GetHopCount(), 2, "010");
    }
}

/**
 * @ingroup dsdv-test
 *
 * @brief DSDV routing table tests (adding and looking up routes)
 */
class DsdvTableTestCase : public TestCase
{
  public:
    DsdvTableTestCase();
    ~DsdvTableTestCase() override;
    void DoRun() override;
};

DsdvTableTestCase::DsdvTableTestCase()
    : TestCase("Dsdv Routing Table test case")
{
}

DsdvTableTestCase::~DsdvTableTestCase()
{
}

void
DsdvTableTestCase::DoRun()
{
    dsdv::RoutingTable rtable;
    Ptr<NetDevice> dev;
    {
        dsdv::RoutingTableEntry rEntry1(
            /*dev=*/dev,
            /*dst=*/Ipv4Address("10.1.1.4"),
            /*seqNo=*/2,
            /*iface=*/Ipv4InterfaceAddress(Ipv4Address("10.1.1.1"), Ipv4Mask("255.255.255.0")),
            /*hops=*/2,
            /*nextHop=*/Ipv4Address("10.1.1.2"),
            /*lifetime=*/Seconds(10));
        NS_TEST_EXPECT_MSG_EQ(rtable.AddRoute(rEntry1), true, "add route");

        dsdv::RoutingTableEntry rEntry2(
            /*dev=*/dev,
            /*dst=*/Ipv4Address("10.1.1.2"),
            /*seqNo=*/4,
            /*iface=*/Ipv4InterfaceAddress(Ipv4Address("10.1.1.1"), Ipv4Mask("255.255.255.0")),
            /*hops=*/1,
            /*nextHop=*/Ipv4Address("10.1.1.2"),
            /*lifetime=*/Seconds(10));
        NS_TEST_EXPECT_MSG_EQ(rtable.AddRoute(rEntry2), true, "add route");

        dsdv::RoutingTableEntry rEntry3(
            /*dev=*/dev,
            /*dst=*/Ipv4Address("10.1.1.3"),
            /*seqNo=*/4,
            /*iface=*/Ipv4InterfaceAddress(Ipv4Address("10.1.1.1"), Ipv4Mask("255.255.255.0")),
            /*hops=*/1,
            /*nextHop=*/Ipv4Address("10.1.1.3"),
            /*lifetime=*/Seconds(10));
        NS_TEST_EXPECT_MSG_EQ(rtable.AddRoute(rEntry3), true, "add route");

        dsdv::RoutingTableEntry rEntry4(
            /*dev=*/dev,
            /*dst=*/Ipv4Address("10.1.1.255"),
            /*seqNo=*/0,
            /*iface=*/Ipv4InterfaceAddress(Ipv4Address("10.1.1.1"), Ipv4Mask("255.255.255.0")),
            /*hops=*/0,
            /*nextHop=*/Ipv4Address("10.1.1.255"),
            /*lifetime=*/Seconds(10));
        NS_TEST_EXPECT_MSG_EQ(rtable.AddRoute(rEntry4), true, "add route");
    }
    {
        dsdv::RoutingTableEntry rEntry;
        if (rtable.LookupRoute(Ipv4Address("10.1.1.4"), rEntry))
        {
            NS_TEST_ASSERT_MSG_EQ(rEntry.GetDestination(), Ipv4Address("10.1.1.4"), "100");
            NS_TEST_ASSERT_MSG_EQ(rEntry.GetSeqNo(), 2, "101");
            NS_TEST_ASSERT_MSG_EQ(rEntry.GetHop(), 2, "102");
        }
        if (rtable.LookupRoute(Ipv4Address("10.1.1.2"), rEntry))
        {
            NS_TEST_ASSERT_MSG_EQ(rEntry.GetDestination(), Ipv4Address("10.1.1.2"), "103");
            NS_TEST_ASSERT_MSG_EQ(rEntry.GetSeqNo(), 4, "104");
            NS_TEST_ASSERT_MSG_EQ(rEntry.GetHop(), 1, "105");
        }
        if (rtable.LookupRoute(Ipv4Address("10.1.1.3"), rEntry))
        {
            NS_TEST_ASSERT_MSG_EQ(rEntry.GetDestination(), Ipv4Address("10.1.1.3"), "106");
            NS_TEST_ASSERT_MSG_EQ(rEntry.GetSeqNo(), 4, "107");
            NS_TEST_ASSERT_MSG_EQ(rEntry.GetHop(), 1, "108");
        }
        if (rtable.LookupRoute(Ipv4Address("10.1.1.255"), rEntry))
        {
            NS_TEST_ASSERT_MSG_EQ(rEntry.GetDestination(), Ipv4Address("10.1.1.255"), "109");
        }
        NS_TEST_ASSERT_MSG_EQ(rEntry.GetInterface().GetLocal(), Ipv4Address("10.1.1.1"), "110");
        NS_TEST_ASSERT_MSG_EQ(rEntry.GetInterface().GetBroadcast(),
                              Ipv4Address("10.1.1.255"),
                              "111");
        NS_TEST_ASSERT_MSG_EQ(rtable.RoutingTableSize(), 4, "Rtable size incorrect");
    }
    Simulator::Destroy();
}

/**
 * @ingroup dsdv-test
 *
 * @brief DSDV test suite
 */
class DsdvTestSuite : public TestSuite
{
  public:
    DsdvTestSuite()
        : TestSuite("routing-dsdv", Type::UNIT)
    {
        AddTestCase(new DsdvHeaderTestCase(), TestCase::Duration::QUICK);
        AddTestCase(new DsdvTableTestCase(), TestCase::Duration::QUICK);
    }
} g_dsdvTestSuite; ///< the test suite
