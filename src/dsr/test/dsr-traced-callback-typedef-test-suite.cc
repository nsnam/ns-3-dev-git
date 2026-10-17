/*
 * Copyright (c) 2015 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "ns3/dsr-option-header.h"
#include "ns3/traced-callback-typedef-test.h"

/**
 * @file
 * @ingroup dsr-test
 *
 * Verify that the TracedCallback typedefs declared by the dsr module are
 * invoked with the right type and number of arguments.
 */

namespace ns3
{

#ifndef DOXYGEN_SHOULD_SKIP_THIS
TRACED_CALLBACK_TYPENAME(dsr::DsrOptionSRHeader::TracedCallback);
#endif // DOXYGEN_SHOULD_SKIP_THIS

/**
 * @ingroup dsr-test
 *
 * TracedCallback typedef Testcase for the dsr module.
 */
class DsrTracedCallbackTypedefTestCase : public TracedCallbackTypedefTestCase
{
  public:
    DsrTracedCallbackTypedefTestCase();

  private:
    void DoRun() override;
};

DsrTracedCallbackTypedefTestCase::DsrTracedCallbackTypedefTestCase()
    : TracedCallbackTypedefTestCase("Check dsr TracedCallback typedefs")
{
}

void
DsrTracedCallbackTypedefTestCase::DoRun()
{
    TRACED_CALLBACK_CHECK(dsr::DsrOptionSRHeader::TracedCallback, const dsr::DsrOptionSRHeader&);
}

/**
 * @ingroup dsr-test
 *
 * @brief dsr TracedCallback typedef TestSuite
 */
class DsrTracedCallbackTypedefTestSuite : public TestSuite
{
  public:
    DsrTracedCallbackTypedefTestSuite();
};

DsrTracedCallbackTypedefTestSuite::DsrTracedCallbackTypedefTestSuite()
    : TestSuite("dsr-traced-callback-typedef", Type::UNIT)
{
    AddTestCase(new DsrTracedCallbackTypedefTestCase, TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static DsrTracedCallbackTypedefTestSuite g_dsrTracedCallbackTypedefTestSuite;

} // namespace ns3
