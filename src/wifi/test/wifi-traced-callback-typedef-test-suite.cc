/*
 * Copyright (c) 2015 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "ns3/traced-callback-typedef-test.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-phy-state-helper.h"
#include "ns3/wifi-remote-station-manager.h"

/**
 * @file
 * @ingroup wifi-test
 *
 * Verify that the TracedCallback typedefs declared by the wifi module are
 * invoked with the right type and number of arguments.
 */

namespace ns3
{

#ifndef DOXYGEN_SHOULD_SKIP_THIS
TRACED_CALLBACK_TYPENAME(WifiMacHeader::TracedCallback);
TRACED_CALLBACK_TYPENAME(WifiPhyStateHelper::RxEndErrorTracedCallback);
TRACED_CALLBACK_TYPENAME(WifiPhyStateHelper::RxOkTracedCallback);
TRACED_CALLBACK_TYPENAME(WifiPhyStateHelper::StateTracedCallback);
TRACED_CALLBACK_TYPENAME(WifiPhyStateHelper::TxTracedCallback);
TRACED_CALLBACK_TYPENAME(WifiRemoteStationManager::PowerChangeTracedCallback);
TRACED_CALLBACK_TYPENAME(WifiRemoteStationManager::RateChangeTracedCallback);
#endif // DOXYGEN_SHOULD_SKIP_THIS

/**
 * @ingroup wifi-test
 *
 * TracedCallback typedef Testcase for the wifi module.
 */
class WifiTracedCallbackTypedefTestCase : public TracedCallbackTypedefTestCase
{
  public:
    WifiTracedCallbackTypedefTestCase();

  private:
    void DoRun() override;
};

WifiTracedCallbackTypedefTestCase::WifiTracedCallbackTypedefTestCase()
    : TracedCallbackTypedefTestCase("Check wifi TracedCallback typedefs")
{
}

void
WifiTracedCallbackTypedefTestCase::DoRun()
{
    TRACED_CALLBACK_CHECK(WifiMacHeader::TracedCallback, const WifiMacHeader&);

    TRACED_CALLBACK_CHECK(WifiPhyStateHelper::RxEndErrorTracedCallback, Ptr<const Packet>, double);

    TRACED_CALLBACK_CHECK(WifiPhyStateHelper::RxOkTracedCallback,
                          Ptr<const Packet>,
                          double,
                          WifiMode,
                          WifiPreamble);

    TRACED_CALLBACK_CHECK(WifiPhyStateHelper::StateTracedCallback, Time, Time, WifiPhyState);

    TRACED_CALLBACK_CHECK(WifiPhyStateHelper::TxTracedCallback,
                          Ptr<const Packet>,
                          WifiMode,
                          WifiPreamble,
                          uint8_t);

    TRACED_CALLBACK_CHECK(WifiRemoteStationManager::PowerChangeTracedCallback,
                          double,
                          double,
                          Mac48Address);

    TRACED_CALLBACK_CHECK(WifiRemoteStationManager::RateChangeTracedCallback,
                          DataRate,
                          DataRate,
                          Mac48Address);
}

/**
 * @ingroup wifi-test
 *
 * @brief wifi TracedCallback typedef TestSuite
 */
class WifiTracedCallbackTypedefTestSuite : public TestSuite
{
  public:
    WifiTracedCallbackTypedefTestSuite();
};

WifiTracedCallbackTypedefTestSuite::WifiTracedCallbackTypedefTestSuite()
    : TestSuite("wifi-traced-callback-typedef", Type::UNIT)
{
    AddTestCase(new WifiTracedCallbackTypedefTestCase, TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static WifiTracedCallbackTypedefTestSuite g_wifiTracedCallbackTypedefTestSuite;

} // namespace ns3
