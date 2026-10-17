/*
 * Copyright (c) 2015 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "ns3/mac48-address.h"
#include "ns3/packet-burst.h"
#include "ns3/packet.h"
#include "ns3/traced-callback-typedef-test.h"

/**
 * @file
 * @ingroup network-test
 *
 * Verify that the TracedCallback typedefs declared by the network module are
 * invoked with the right type and number of arguments.
 */

namespace ns3
{

#ifndef DOXYGEN_SHOULD_SKIP_THIS
TRACED_CALLBACK_TYPENAME(Mac48Address::TracedCallback);
TRACED_CALLBACK_TYPENAME(Packet::AddressTracedCallback);
TRACED_CALLBACK_TYPENAME(Packet::Mac48AddressTracedCallback);
TRACED_CALLBACK_TYPENAME(Packet::SinrTracedCallback);
TRACED_CALLBACK_TYPENAME(Packet::SizeTracedCallback);
TRACED_CALLBACK_TYPENAME(Packet::TracedCallback);
TRACED_CALLBACK_TYPENAME(PacketBurst::TracedCallback);
#endif // DOXYGEN_SHOULD_SKIP_THIS

/**
 * @ingroup network-test
 *
 * TracedCallback typedef Testcase for the network module.
 */
class NetworkTracedCallbackTypedefTestCase : public TracedCallbackTypedefTestCase
{
  public:
    NetworkTracedCallbackTypedefTestCase();

  private:
    void DoRun() override;
};

NetworkTracedCallbackTypedefTestCase::NetworkTracedCallbackTypedefTestCase()
    : TracedCallbackTypedefTestCase("Check network TracedCallback typedefs")
{
}

void
NetworkTracedCallbackTypedefTestCase::DoRun()
{
    TRACED_CALLBACK_CHECK(Mac48Address::TracedCallback, Mac48Address);

    TRACED_CALLBACK_CHECK(Packet::AddressTracedCallback, Ptr<const Packet>, const Address&);

    TRACED_CALLBACK_CHECK(Packet::Mac48AddressTracedCallback, Ptr<const Packet>, Mac48Address);

    TRACED_CALLBACK_CHECK(Packet::SinrTracedCallback, Ptr<const Packet>, double);

    TRACED_CALLBACK_CHECK(Packet::SizeTracedCallback, uint32_t, uint32_t);

    TRACED_CALLBACK_CHECK(Packet::TracedCallback, Ptr<const Packet>);

    TRACED_CALLBACK_CHECK(PacketBurst::TracedCallback, Ptr<const PacketBurst>);
}

/**
 * @ingroup network-test
 *
 * @brief network TracedCallback typedef TestSuite
 */
class NetworkTracedCallbackTypedefTestSuite : public TestSuite
{
  public:
    NetworkTracedCallbackTypedefTestSuite();
};

NetworkTracedCallbackTypedefTestSuite::NetworkTracedCallbackTypedefTestSuite()
    : TestSuite("network-traced-callback-typedef", Type::UNIT)
{
    AddTestCase(new NetworkTracedCallbackTypedefTestCase, TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static NetworkTracedCallbackTypedefTestSuite g_networkTracedCallbackTypedefTestSuite;

} // namespace ns3
