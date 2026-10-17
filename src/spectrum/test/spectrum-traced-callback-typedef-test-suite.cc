/*
 * Copyright (c) 2015 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "ns3/spectrum-channel.h"
#include "ns3/spectrum-value.h"
#include "ns3/traced-callback-typedef-test.h"

/**
 * @file
 * @ingroup spectrum-tests
 *
 * Verify that the TracedCallback typedefs declared by the spectrum module are
 * invoked with the right type and number of arguments.
 */

namespace ns3
{

#ifndef DOXYGEN_SHOULD_SKIP_THIS
TRACED_CALLBACK_TYPENAME(SpectrumChannel::LossTracedCallback);
TRACED_CALLBACK_TYPENAME(SpectrumValue::TracedCallback);
#endif // DOXYGEN_SHOULD_SKIP_THIS

/**
 * @ingroup spectrum-tests
 *
 * TracedCallback typedef Testcase for the spectrum module.
 */
class SpectrumTracedCallbackTypedefTestCase : public TracedCallbackTypedefTestCase
{
  public:
    SpectrumTracedCallbackTypedefTestCase();

  private:
    void DoRun() override;
};

SpectrumTracedCallbackTypedefTestCase::SpectrumTracedCallbackTypedefTestCase()
    : TracedCallbackTypedefTestCase("Check spectrum TracedCallback typedefs")
{
}

void
SpectrumTracedCallbackTypedefTestCase::DoRun()
{
    TRACED_CALLBACK_CHECK(SpectrumChannel::LossTracedCallback,
                          Ptr<const SpectrumPhy>,
                          Ptr<const SpectrumPhy>,
                          double);

    TRACED_CALLBACK_CHECK(SpectrumValue::TracedCallback, Ptr<SpectrumValue>);
}

/**
 * @ingroup spectrum-tests
 *
 * @brief spectrum TracedCallback typedef TestSuite
 */
class SpectrumTracedCallbackTypedefTestSuite : public TestSuite
{
  public:
    SpectrumTracedCallbackTypedefTestSuite();
};

SpectrumTracedCallbackTypedefTestSuite::SpectrumTracedCallbackTypedefTestSuite()
    : TestSuite("spectrum-traced-callback-typedef", Type::UNIT)
{
    AddTestCase(new SpectrumTracedCallbackTypedefTestCase, TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static SpectrumTracedCallbackTypedefTestSuite g_spectrumTracedCallbackTypedefTestSuite;

} // namespace ns3
