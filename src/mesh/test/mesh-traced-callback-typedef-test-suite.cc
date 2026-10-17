/*
 * Copyright (c) 2015 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "ns3/peer-management-protocol.h"
#include "ns3/traced-callback-typedef-test.h"

/**
 * @file
 * @ingroup mesh-test
 *
 * Verify that the TracedCallback typedefs declared by the mesh module are
 * invoked with the right type and number of arguments.
 */

namespace ns3
{

#ifndef DOXYGEN_SHOULD_SKIP_THIS
TRACED_CALLBACK_TYPENAME(dot11s::PeerManagementProtocol::LinkOpenCloseTracedCallback);
#endif // DOXYGEN_SHOULD_SKIP_THIS

/**
 * @ingroup mesh-test
 *
 * TracedCallback typedef Testcase for the mesh module.
 */
class Dot11sTracedCallbackTypedefTestCase : public TracedCallbackTypedefTestCase
{
  public:
    Dot11sTracedCallbackTypedefTestCase();

  private:
    void DoRun() override;
};

Dot11sTracedCallbackTypedefTestCase::Dot11sTracedCallbackTypedefTestCase()
    : TracedCallbackTypedefTestCase("Check mesh TracedCallback typedefs")
{
}

void
Dot11sTracedCallbackTypedefTestCase::DoRun()
{
    TRACED_CALLBACK_CHECK(dot11s::PeerManagementProtocol::LinkOpenCloseTracedCallback,
                          Mac48Address,
                          Mac48Address);
}

/**
 * @ingroup mesh-test
 *
 * @brief mesh TracedCallback typedef TestSuite
 */
class Dot11sTracedCallbackTypedefTestSuite : public TestSuite
{
  public:
    Dot11sTracedCallbackTypedefTestSuite();
};

Dot11sTracedCallbackTypedefTestSuite::Dot11sTracedCallbackTypedefTestSuite()
    : TestSuite("mesh-traced-callback-typedef", Type::UNIT)
{
    AddTestCase(new Dot11sTracedCallbackTypedefTestCase, TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static Dot11sTracedCallbackTypedefTestSuite g_dot11sTracedCallbackTypedefTestSuite;

} // namespace ns3
