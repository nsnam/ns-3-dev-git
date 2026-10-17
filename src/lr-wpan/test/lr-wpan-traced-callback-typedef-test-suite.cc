/*
 * Copyright (c) 2015 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "ns3/lr-wpan-mac.h"
#include "ns3/lr-wpan-phy.h"
#include "ns3/traced-callback-typedef-test.h"

/**
 * @file
 * @ingroup lr-wpan-test
 *
 * Verify that the TracedCallback typedefs declared by the lr-wpan module are
 * invoked with the right type and number of arguments.
 */

namespace ns3
{

#ifndef DOXYGEN_SHOULD_SKIP_THIS
TRACED_CALLBACK_TYPENAME(lrwpan::LrWpanMac::SentTracedCallback);
TRACED_CALLBACK_TYPENAME(lrwpan::LrWpanMac::StateTracedCallback);
TRACED_CALLBACK_TYPENAME(lrwpan::LrWpanPhy::StateTracedCallback);
#endif // DOXYGEN_SHOULD_SKIP_THIS

/**
 * @ingroup lr-wpan-test
 *
 * TracedCallback typedef Testcase for the lr-wpan module.
 */
class LrWpanTracedCallbackTypedefTestCase : public TracedCallbackTypedefTestCase
{
  public:
    LrWpanTracedCallbackTypedefTestCase();

  private:
    void DoRun() override;
};

LrWpanTracedCallbackTypedefTestCase::LrWpanTracedCallbackTypedefTestCase()
    : TracedCallbackTypedefTestCase("Check lr-wpan TracedCallback typedefs")
{
}

void
LrWpanTracedCallbackTypedefTestCase::DoRun()
{
    TRACED_CALLBACK_CHECK(lrwpan::LrWpanMac::SentTracedCallback,
                          Ptr<const Packet>,
                          uint8_t,
                          uint8_t);

    TRACED_CALLBACK_CHECK(lrwpan::LrWpanMac::StateTracedCallback,
                          lrwpan::MacState,
                          lrwpan::MacState);

    TRACED_CALLBACK_CHECK(lrwpan::LrWpanPhy::StateTracedCallback,
                          Time,
                          lrwpan::PhyEnumeration,
                          lrwpan::PhyEnumeration);
}

/**
 * @ingroup lr-wpan-test
 *
 * @brief lr-wpan TracedCallback typedef TestSuite
 */
class LrWpanTracedCallbackTypedefTestSuite : public TestSuite
{
  public:
    LrWpanTracedCallbackTypedefTestSuite();
};

LrWpanTracedCallbackTypedefTestSuite::LrWpanTracedCallbackTypedefTestSuite()
    : TestSuite("lr-wpan-traced-callback-typedef", Type::UNIT)
{
    AddTestCase(new LrWpanTracedCallbackTypedefTestCase, TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static LrWpanTracedCallbackTypedefTestSuite g_lrWpanTracedCallbackTypedefTestSuite;

} // namespace ns3
