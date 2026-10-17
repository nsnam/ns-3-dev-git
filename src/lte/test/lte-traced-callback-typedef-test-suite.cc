/*
 * Copyright (c) 2015 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "ns3/epc-ue-nas.h"
#include "ns3/lte-common.h"
#include "ns3/lte-enb-mac.h"
#include "ns3/lte-enb-phy.h"
#include "ns3/lte-enb-rrc.h"
#include "ns3/lte-pdcp.h"
#include "ns3/lte-rlc.h"
#include "ns3/lte-ue-phy.h"
#include "ns3/lte-ue-rrc.h"
#include "ns3/traced-callback-typedef-test.h"

/**
 * @file
 * @ingroup lte-test
 *
 * Verify that the TracedCallback typedefs declared by the lte module are
 * invoked with the right type and number of arguments.
 */

namespace ns3
{

#ifndef DOXYGEN_SHOULD_SKIP_THIS
TRACED_CALLBACK_TYPENAME(EpcUeNas::StateTracedCallback);
TRACED_CALLBACK_TYPENAME(LteEnbMac::DlSchedulingTracedCallback);
TRACED_CALLBACK_TYPENAME(LteEnbMac::UlSchedulingTracedCallback);
TRACED_CALLBACK_TYPENAME(LteEnbPhy::ReportInterferenceTracedCallback);
TRACED_CALLBACK_TYPENAME(LteEnbPhy::ReportUeSinrTracedCallback);
TRACED_CALLBACK_TYPENAME(LteEnbRrc::ConnectionHandoverTracedCallback);
TRACED_CALLBACK_TYPENAME(LteEnbRrc::HandoverStartTracedCallback);
TRACED_CALLBACK_TYPENAME(LteEnbRrc::NewUeContextTracedCallback);
TRACED_CALLBACK_TYPENAME(LteEnbRrc::ReceiveReportTracedCallback);
TRACED_CALLBACK_TYPENAME(LtePdcp::PduRxTracedCallback);
TRACED_CALLBACK_TYPENAME(LtePdcp::PduTxTracedCallback);
TRACED_CALLBACK_TYPENAME(LteUePhy::StateTracedCallback);
TRACED_CALLBACK_TYPENAME(LteUePhy::RsrpSinrTracedCallback);
TRACED_CALLBACK_TYPENAME(LteUeRrc::CellSelectionTracedCallback);
TRACED_CALLBACK_TYPENAME(LteUeRrc::StateTracedCallback);
TRACED_CALLBACK_TYPENAME(PhyReceptionStatParameters::TracedCallback);
TRACED_CALLBACK_TYPENAME(PhyTransmissionStatParameters::TracedCallback);
TRACED_CALLBACK_TYPENAME(UeManager::StateTracedCallback);
#endif // DOXYGEN_SHOULD_SKIP_THIS

/**
 * @ingroup lte-test
 *
 * TracedCallback typedef Testcase for the lte module.
 */
class LteTracedCallbackTypedefTestCase : public TracedCallbackTypedefTestCase
{
  public:
    LteTracedCallbackTypedefTestCase();

  private:
    void DoRun() override;
};

LteTracedCallbackTypedefTestCase::LteTracedCallbackTypedefTestCase()
    : TracedCallbackTypedefTestCase("Check lte TracedCallback typedefs")
{
    m_dupes.insert("LteRlc::NotifyTxTracedCallback");
    m_dupes.insert("LteRlc::ReceiveTracedCallback");
    m_dupes.insert("LteUeRrc::ImsiCidRntiTracedCallback");
    m_dupes.insert("LteUeRrc::MibSibHandoverTracedCallback");
}

void
LteTracedCallbackTypedefTestCase::DoRun()
{
    TRACED_CALLBACK_CHECK(EpcUeNas::StateTracedCallback, EpcUeNas::State, EpcUeNas::State);

    TRACED_CALLBACK_CHECK(LteEnbMac::DlSchedulingTracedCallback,
                          uint32_t,
                          uint32_t,
                          uint16_t,
                          uint8_t,
                          uint16_t,
                          uint8_t,
                          uint16_t,
                          uint8_t);

    TRACED_CALLBACK_CHECK(LteEnbMac::UlSchedulingTracedCallback,
                          uint32_t,
                          uint32_t,
                          uint16_t,
                          uint8_t,
                          uint16_t);

    TRACED_CALLBACK_CHECK(LteEnbPhy::ReportUeSinrTracedCallback,
                          uint16_t,
                          uint16_t,
                          double,
                          uint8_t);

    TRACED_CALLBACK_CHECK(LteEnbPhy::ReportInterferenceTracedCallback,
                          uint16_t,
                          Ptr<SpectrumValue>);

    TRACED_CALLBACK_CHECK(LteEnbRrc::ConnectionHandoverTracedCallback,
                          uint64_t,
                          uint16_t,
                          uint16_t);

    TRACED_CALLBACK_CHECK(LteEnbRrc::HandoverStartTracedCallback,
                          uint64_t,
                          uint16_t,
                          uint16_t,
                          uint16_t);

    TRACED_CALLBACK_CHECK(LteEnbRrc::NewUeContextTracedCallback, uint16_t, uint16_t);

    TRACED_CALLBACK_CHECK(LteEnbRrc::ReceiveReportTracedCallback,
                          uint64_t,
                          uint16_t,
                          uint16_t,
                          LteRrcSap::MeasurementReport);

    TRACED_CALLBACK_CHECK(LtePdcp::PduRxTracedCallback, uint16_t, uint8_t, uint32_t, uint64_t);

    TRACED_CALLBACK_CHECK(LtePdcp::PduTxTracedCallback, uint16_t, uint8_t, uint32_t);

    TRACED_CALLBACK_DUPE(LteRlc::NotifyTxTracedCallback, LtePdcp::PduTxTracedCallback);

    TRACED_CALLBACK_DUPE(LteRlc::ReceiveTracedCallback, LtePdcp::PduRxTracedCallback);

    TRACED_CALLBACK_CHECK(LteUePhy::RsrpSinrTracedCallback,
                          uint16_t,
                          uint16_t,
                          double,
                          double,
                          uint8_t);

    TRACED_CALLBACK_CHECK(LteUePhy::StateTracedCallback,
                          uint16_t,
                          uint16_t,
                          LteUePhy::State,
                          LteUePhy::State);

    TRACED_CALLBACK_CHECK(LteUeRrc::CellSelectionTracedCallback, uint64_t, uint16_t);

    TRACED_CALLBACK_DUPE(LteUeRrc::ImsiCidRntiTracedCallback,
                         LteEnbRrc::ConnectionHandoverTracedCallback);

    TRACED_CALLBACK_DUPE(LteUeRrc::MibSibHandoverTracedCallback,
                         LteEnbRrc::HandoverStartTracedCallback);

    TRACED_CALLBACK_CHECK(LteUeRrc::StateTracedCallback,
                          uint64_t,
                          uint16_t,
                          uint16_t,
                          LteUeRrc::State,
                          LteUeRrc::State);

    TRACED_CALLBACK_CHECK(PhyReceptionStatParameters::TracedCallback, PhyReceptionStatParameters);

    TRACED_CALLBACK_CHECK(PhyTransmissionStatParameters::TracedCallback,
                          PhyTransmissionStatParameters);

    TRACED_CALLBACK_CHECK(UeManager::StateTracedCallback,
                          uint64_t,
                          uint16_t,
                          uint16_t,
                          UeManager::State,
                          UeManager::State);
}

/**
 * @ingroup lte-test
 *
 * @brief lte TracedCallback typedef TestSuite
 */
class LteTracedCallbackTypedefTestSuite : public TestSuite
{
  public:
    LteTracedCallbackTypedefTestSuite();
};

LteTracedCallbackTypedefTestSuite::LteTracedCallbackTypedefTestSuite()
    : TestSuite("lte-traced-callback-typedef", Type::UNIT)
{
    AddTestCase(new LteTracedCallbackTypedefTestCase, TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static LteTracedCallbackTypedefTestSuite g_lteTracedCallbackTypedefTestSuite;

} // namespace ns3
