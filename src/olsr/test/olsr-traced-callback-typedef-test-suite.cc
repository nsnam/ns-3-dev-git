/*
 * Copyright (c) 2015 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "ns3/olsr-header.h"
#include "ns3/olsr-routing-protocol.h"
#include "ns3/traced-callback-typedef-test.h"

/**
 * @file
 * @ingroup olsr-test
 *
 * Verify that the TracedCallback typedefs declared by the olsr module are
 * invoked with the right type and number of arguments.
 */

namespace ns3
{

#ifndef DOXYGEN_SHOULD_SKIP_THIS
TRACED_CALLBACK_TYPENAME(olsr::RoutingProtocol::PacketTxRxTracedCallback);
TRACED_CALLBACK_TYPENAME(olsr::RoutingProtocol::TableChangeTracedCallback);
#endif // DOXYGEN_SHOULD_SKIP_THIS

/**
 * @ingroup olsr-test
 *
 * TracedCallback typedef Testcase for the olsr module.
 */
class OlsrTracedCallbackTypedefTestCase : public TracedCallbackTypedefTestCase
{
  public:
    OlsrTracedCallbackTypedefTestCase();

  private:
    void DoRun() override;
};

OlsrTracedCallbackTypedefTestCase::OlsrTracedCallbackTypedefTestCase()
    : TracedCallbackTypedefTestCase("Check olsr TracedCallback typedefs")
{
}

void
OlsrTracedCallbackTypedefTestCase::DoRun()
{
    TRACED_CALLBACK_CHECK(olsr::RoutingProtocol::PacketTxRxTracedCallback,
                          const olsr::PacketHeader&,
                          const olsr::MessageList&);

    TRACED_CALLBACK_CHECK(olsr::RoutingProtocol::TableChangeTracedCallback, uint32_t);
}

/**
 * @ingroup olsr-test
 *
 * @brief olsr TracedCallback typedef TestSuite
 */
class OlsrTracedCallbackTypedefTestSuite : public TestSuite
{
  public:
    OlsrTracedCallbackTypedefTestSuite();
};

OlsrTracedCallbackTypedefTestSuite::OlsrTracedCallbackTypedefTestSuite()
    : TestSuite("olsr-traced-callback-typedef", Type::UNIT)
{
    AddTestCase(new OlsrTracedCallbackTypedefTestCase, TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static OlsrTracedCallbackTypedefTestSuite g_olsrTracedCallbackTypedefTestSuite;

} // namespace ns3
