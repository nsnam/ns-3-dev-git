/*
 * Copyright (c) 2015 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "ns3/mobility-model.h"
#include "ns3/traced-callback-typedef-test.h"

/**
 * @file
 * @ingroup mobility-test
 *
 * Verify that the TracedCallback typedefs declared by the mobility module are
 * invoked with the right type and number of arguments.
 */

namespace ns3
{

#ifndef DOXYGEN_SHOULD_SKIP_THIS
TRACED_CALLBACK_TYPENAME(MobilityModel::TracedCallback);
#endif // DOXYGEN_SHOULD_SKIP_THIS

/**
 * @ingroup mobility-test
 *
 * TracedCallback typedef Testcase for the mobility module.
 */
class MobilityTracedCallbackTypedefTestCase : public TracedCallbackTypedefTestCase
{
  public:
    MobilityTracedCallbackTypedefTestCase();

  private:
    void DoRun() override;
};

MobilityTracedCallbackTypedefTestCase::MobilityTracedCallbackTypedefTestCase()
    : TracedCallbackTypedefTestCase("Check mobility TracedCallback typedefs")
{
}

void
MobilityTracedCallbackTypedefTestCase::DoRun()
{
    TRACED_CALLBACK_CHECK(MobilityModel::TracedCallback, Ptr<const MobilityModel>);
}

/**
 * @ingroup mobility-test
 *
 * @brief mobility TracedCallback typedef TestSuite
 */
class MobilityTracedCallbackTypedefTestSuite : public TestSuite
{
  public:
    MobilityTracedCallbackTypedefTestSuite();
};

MobilityTracedCallbackTypedefTestSuite::MobilityTracedCallbackTypedefTestSuite()
    : TestSuite("mobility-traced-callback-typedef", Type::UNIT)
{
    AddTestCase(new MobilityTracedCallbackTypedefTestCase, TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static MobilityTracedCallbackTypedefTestSuite g_mobilityTracedCallbackTypedefTestSuite;

} // namespace ns3
