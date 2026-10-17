/*
 * Copyright (c) 2015 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/traced-callback-typedef-test.h"

/**
 * @file
 * @ingroup internet-test
 *
 * Verify that the TracedCallback typedefs declared by the internet module are
 * invoked with the right type and number of arguments.
 */

namespace ns3
{

#ifndef DOXYGEN_SHOULD_SKIP_THIS
TRACED_CALLBACK_TYPENAME(Ipv4L3Protocol::DropTracedCallback);
TRACED_CALLBACK_TYPENAME(Ipv4L3Protocol::SentTracedCallback);
TRACED_CALLBACK_TYPENAME(Ipv4L3Protocol::TxRxTracedCallback);
TRACED_CALLBACK_TYPENAME(Ipv6L3Protocol::DropTracedCallback);
TRACED_CALLBACK_TYPENAME(Ipv6L3Protocol::SentTracedCallback);
TRACED_CALLBACK_TYPENAME(Ipv6L3Protocol::TxRxTracedCallback);
#endif // DOXYGEN_SHOULD_SKIP_THIS

/**
 * @ingroup internet-test
 *
 * TracedCallback typedef Testcase for the internet module.
 */
class InternetTracedCallbackTypedefTestCase : public TracedCallbackTypedefTestCase
{
  public:
    InternetTracedCallbackTypedefTestCase();

  private:
    void DoRun() override;
};

InternetTracedCallbackTypedefTestCase::InternetTracedCallbackTypedefTestCase()
    : TracedCallbackTypedefTestCase("Check internet TracedCallback typedefs")
{
}

void
InternetTracedCallbackTypedefTestCase::DoRun()
{
    TRACED_CALLBACK_CHECK(Ipv4L3Protocol::DropTracedCallback,
                          const Ipv4Header&,
                          Ptr<const Packet>,
                          Ipv4L3Protocol::DropReason,
                          Ptr<Ipv4>,
                          uint32_t);

    TRACED_CALLBACK_CHECK(Ipv4L3Protocol::SentTracedCallback,
                          const Ipv4Header&,
                          Ptr<const Packet>,
                          uint32_t);

    TRACED_CALLBACK_CHECK(Ipv4L3Protocol::TxRxTracedCallback,
                          Ptr<const Packet>,
                          Ptr<Ipv4>,
                          uint32_t);

    TRACED_CALLBACK_CHECK(Ipv6L3Protocol::DropTracedCallback,
                          const Ipv6Header&,
                          Ptr<const Packet>,
                          Ipv6L3Protocol::DropReason,
                          Ptr<Ipv6>,
                          uint32_t);

    TRACED_CALLBACK_CHECK(Ipv6L3Protocol::SentTracedCallback,
                          const Ipv6Header&,
                          Ptr<const Packet>,
                          uint32_t);

    TRACED_CALLBACK_CHECK(Ipv6L3Protocol::TxRxTracedCallback,
                          Ptr<const Packet>,
                          Ptr<Ipv6>,
                          uint32_t);
}

/**
 * @ingroup internet-test
 *
 * @brief internet TracedCallback typedef TestSuite
 */
class InternetTracedCallbackTypedefTestSuite : public TestSuite
{
  public:
    InternetTracedCallbackTypedefTestSuite();
};

InternetTracedCallbackTypedefTestSuite::InternetTracedCallbackTypedefTestSuite()
    : TestSuite("internet-traced-callback-typedef", Type::UNIT)
{
    AddTestCase(new InternetTracedCallbackTypedefTestCase, TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static InternetTracedCallbackTypedefTestSuite g_internetTracedCallbackTypedefTestSuite;

} // namespace ns3
