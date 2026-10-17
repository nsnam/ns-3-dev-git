/*
 * Copyright (c) 2015 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "ns3/sixlowpan-net-device.h"
#include "ns3/traced-callback-typedef-test.h"

/**
 * @file
 * @ingroup sixlowpan-tests
 *
 * Verify that the TracedCallback typedefs declared by the sixlowpan module are
 * invoked with the right type and number of arguments.
 */

namespace ns3
{

#ifndef DOXYGEN_SHOULD_SKIP_THIS
TRACED_CALLBACK_TYPENAME(SixLowPanNetDevice::DropTracedCallback);
TRACED_CALLBACK_TYPENAME(SixLowPanNetDevice::RxTxTracedCallback);
#endif // DOXYGEN_SHOULD_SKIP_THIS

/**
 * @ingroup sixlowpan-tests
 *
 * TracedCallback typedef Testcase for the sixlowpan module.
 */
class SixLowPanTracedCallbackTypedefTestCase : public TracedCallbackTypedefTestCase
{
  public:
    SixLowPanTracedCallbackTypedefTestCase();

  private:
    void DoRun() override;
};

SixLowPanTracedCallbackTypedefTestCase::SixLowPanTracedCallbackTypedefTestCase()
    : TracedCallbackTypedefTestCase("Check sixlowpan TracedCallback typedefs")
{
}

void
SixLowPanTracedCallbackTypedefTestCase::DoRun()
{
    TRACED_CALLBACK_CHECK(SixLowPanNetDevice::DropTracedCallback,
                          SixLowPanNetDevice::DropReason,
                          Ptr<const Packet>,
                          Ptr<SixLowPanNetDevice>,
                          uint32_t);

    TRACED_CALLBACK_CHECK(SixLowPanNetDevice::RxTxTracedCallback,
                          Ptr<const Packet>,
                          Ptr<SixLowPanNetDevice>,
                          uint32_t);
}

/**
 * @ingroup sixlowpan-tests
 *
 * @brief sixlowpan TracedCallback typedef TestSuite
 */
class SixLowPanTracedCallbackTypedefTestSuite : public TestSuite
{
  public:
    SixLowPanTracedCallbackTypedefTestSuite();
};

SixLowPanTracedCallbackTypedefTestSuite::SixLowPanTracedCallbackTypedefTestSuite()
    : TestSuite("sixlowpan-traced-callback-typedef", Type::UNIT)
{
    AddTestCase(new SixLowPanTracedCallbackTypedefTestCase, TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static SixLowPanTracedCallbackTypedefTestSuite g_sixLowPanTracedCallbackTypedefTestSuite;

} // namespace ns3
