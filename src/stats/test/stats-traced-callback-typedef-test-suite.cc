/*
 * Copyright (c) 2015 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "ns3/time-series-adaptor.h"
#include "ns3/traced-callback-typedef-test.h"

/**
 * @file
 * @ingroup stats-tests
 *
 * Verify that the TracedCallback typedefs declared by the stats module are
 * invoked with the right type and number of arguments.
 */

namespace ns3
{

#ifndef DOXYGEN_SHOULD_SKIP_THIS
TRACED_CALLBACK_TYPENAME(TimeSeriesAdaptor::OutputTracedCallback);
#endif // DOXYGEN_SHOULD_SKIP_THIS

/**
 * @ingroup stats-tests
 *
 * TracedCallback typedef Testcase for the stats module.
 */
class StatsTracedCallbackTypedefTestCase : public TracedCallbackTypedefTestCase
{
  public:
    StatsTracedCallbackTypedefTestCase();

  private:
    void DoRun() override;
};

StatsTracedCallbackTypedefTestCase::StatsTracedCallbackTypedefTestCase()
    : TracedCallbackTypedefTestCase("Check stats TracedCallback typedefs")
{
}

void
StatsTracedCallbackTypedefTestCase::DoRun()
{
    TRACED_CALLBACK_CHECK(TimeSeriesAdaptor::OutputTracedCallback, double, double);
}

/**
 * @ingroup stats-tests
 *
 * @brief stats TracedCallback typedef TestSuite
 */
class StatsTracedCallbackTypedefTestSuite : public TestSuite
{
  public:
    StatsTracedCallbackTypedefTestSuite();
};

StatsTracedCallbackTypedefTestSuite::StatsTracedCallbackTypedefTestSuite()
    : TestSuite("stats-traced-callback-typedef", Type::UNIT)
{
    AddTestCase(new StatsTracedCallbackTypedefTestCase, TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static StatsTracedCallbackTypedefTestSuite g_statsTracedCallbackTypedefTestSuite;

} // namespace ns3
