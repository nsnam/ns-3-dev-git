/*
 * Copyright (c) 2015 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "ns3/traced-callback-typedef-test.h"
#include "ns3/uan-mac-cw.h"
#include "ns3/uan-mac-rc.h"
#include "ns3/uan-mac.h"
#include "ns3/uan-net-device.h"
#include "ns3/uan-phy.h"

/**
 * @file
 * @ingroup uan-test
 *
 * Verify that the TracedCallback typedefs declared by the uan module are
 * invoked with the right type and number of arguments.
 */

namespace ns3
{

#ifndef DOXYGEN_SHOULD_SKIP_THIS
TRACED_CALLBACK_TYPENAME(UanMac::PacketModeTracedCallback);
TRACED_CALLBACK_TYPENAME(UanMacCw::QueueTracedCallback);
TRACED_CALLBACK_TYPENAME(UanMacRc::QueueTracedCallback);
TRACED_CALLBACK_TYPENAME(UanNetDevice::RxTxTracedCallback);
TRACED_CALLBACK_TYPENAME(UanPhy::TracedCallback);
#endif // DOXYGEN_SHOULD_SKIP_THIS

/**
 * @ingroup uan-test
 *
 * TracedCallback typedef Testcase for the uan module.
 */
class UanTracedCallbackTypedefTestCase : public TracedCallbackTypedefTestCase
{
  public:
    UanTracedCallbackTypedefTestCase();

  private:
    void DoRun() override;
};

UanTracedCallbackTypedefTestCase::UanTracedCallbackTypedefTestCase()
    : TracedCallbackTypedefTestCase("Check uan TracedCallback typedefs")
{
}

void
UanTracedCallbackTypedefTestCase::DoRun()
{
    TRACED_CALLBACK_CHECK(UanMac::PacketModeTracedCallback, Ptr<const Packet>, UanTxMode);

    TRACED_CALLBACK_CHECK(UanMacCw::QueueTracedCallback, Ptr<const Packet>, uint16_t);

    TRACED_CALLBACK_CHECK(UanMacRc::QueueTracedCallback, Ptr<const Packet>, uint32_t);

    TRACED_CALLBACK_CHECK(UanNetDevice::RxTxTracedCallback, Ptr<const Packet>, Mac8Address);

    TRACED_CALLBACK_CHECK(UanPhy::TracedCallback, Ptr<const Packet>, double, UanTxMode);
}

/**
 * @ingroup uan-test
 *
 * @brief uan TracedCallback typedef TestSuite
 */
class UanTracedCallbackTypedefTestSuite : public TestSuite
{
  public:
    UanTracedCallbackTypedefTestSuite();
};

UanTracedCallbackTypedefTestSuite::UanTracedCallbackTypedefTestSuite()
    : TestSuite("uan-traced-callback-typedef", Type::UNIT)
{
    AddTestCase(new UanTracedCallbackTypedefTestCase, TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static UanTracedCallbackTypedefTestSuite g_uanTracedCallbackTypedefTestSuite;

} // namespace ns3
