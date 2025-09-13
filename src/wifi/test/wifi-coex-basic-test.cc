/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-coex-basic-test.h"

#include "ns3/boolean.h"
#include "ns3/coex-helper.h"
#include "ns3/config.h"
#include "ns3/frame-exchange-manager.h"
#include "ns3/log.h"
#include "ns3/qos-txop.h"
#include "ns3/test.h"
#include "ns3/wifi-net-device.h"

#include <algorithm>
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiCoexBasicTest");

namespace
{
/**
 * @param scenario the WifiCoexDlTxopTest scenario to test
 * @returns a string identifying the scenario to test
 */
std::string
GetScenarioStr(WifiCoexDlTxopTest::Scenario scenario)
{
    using enum WifiCoexDlTxopTest::Scenario;
    switch (scenario)
    {
    case BEFORE_PPDU_START:
        return "BEFORE_PPDU_START";
    case DURING_PREAMBLE_DETECTION:
        return "DURING_PREAMBLE_DETECTION";
    case DURING_PHY_HEADER:
        return "DURING_PHY_HEADER";
    case DURING_FIRST_MPDU:
        return "DURING_FIRST_MPDU";
    case DURING_SECOND_MPDU:
        return "DURING_SECOND_MPDU";
    case BEFORE_BLOCKACK:
        return "BEFORE_BLOCKACK";
    case DURING_BLOCKACK:
        return "DURING_BLOCKACK";
    default:
        NS_ABORT_MSG("Unexpected value " << static_cast<uint16_t>(scenario));
        return "";
    }
}

/**
 * @param scenario the WifiCoexUlTxopTest scenario to test
 * @returns a string identifying the scenario to test
 */
std::string
GetScenarioStr(WifiCoexUlTxopTest::Scenario scenario)
{
    using enum WifiCoexUlTxopTest::Scenario;
    switch (scenario)
    {
    case DEFER_TXOP_START:
        return "DEFER_TXOP_START";
    case DEFER_ONE_PACKET:
        return "DEFER_ONE_PACKET";
    case TXOP_BEFORE_COEX:
        return "TXOP_BEFORE_COEX";
    case INTRA_TXOP_COEX:
        return "INTRA_TXOP_COEX";
    default:
        NS_ABORT_MSG("Unexpected value " << static_cast<uint16_t>(scenario));
        return "";
    }
}

} // namespace

WifiCoexDlTxopTest::WifiCoexDlTxopTest(Scenario scenario)
    : WifiCoexTestBase("DL TXOP test, scenario " + GetScenarioStr(scenario)),
      m_scenario(scenario)
{
}

void
WifiCoexDlTxopTest::DoSetup()
{
    // retransmit the A-MPDU in case of missed BlockAck to perform the same checks both in case
    // the coex event overlaps with the A-MPDU and in case the coex event overlaps with the BlockAck
    Config::SetDefault("ns3::QosTxop::UseExplicitBarAfterMissedBlockAck", BooleanValue(false));
    Config::SetDefault("ns3::WifiPhy::NotifyMacHdrRxEnd", BooleanValue(true));

    WifiCoexTestBase::DoSetup();

    const auto phy = m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID);
    const auto slot = phy->GetSlot();
    const auto aifs =
        phy->GetSifs() + m_apMac->GetQosTxop(AC_BE)->GetAifsn(SINGLE_LINK_OP_ID) * slot;
    // DL packets are transmitted at a slot boundary; last busy time is zero because no management
    // frame is sent due to static setup
    m_dlPktTxTime = aifs + 550 * slot; // approx 5 ms in the 5 GHz or 6 GHz band

    // the time of the first coex event notification equals the value of the InterEventTime
    // attribute of the mock event generator when the latter is constructed
    if (m_scenario == Scenario::BEFORE_PPDU_START)
    {
        m_mockInterEventTime = m_dlPktTxTime - slot;
        m_mockEventStartDelay = Time{0};
        m_mockEventDuration = m_dlPktTxTime - m_mockInterEventTime + m_coexEventPpduOverlap;
    }
    else
    {
        m_mockInterEventTime = m_dlPktTxTime + m_coexEventPpduOffset;
        m_mockEventStartDelay = Time{0}; // modified when PPDU TX starts
        m_mockEventDuration =
            (m_scenario != Scenario::BEFORE_BLOCKACK ? m_coexEventPpduOverlap : phy->GetSifs());
    }

    SetMockEventGen(m_mockInterEventTime, m_mockEventStartDelay, m_mockEventDuration);

    // schedule generation of DL packets
    Simulator::Schedule(m_dlPktTxTime,
                        &Node::AddApplication,
                        m_apMac->GetDevice()->GetNode(),
                        GetApplication(WifiDirection::DOWNLINK, m_nPackets, 750));

    // generate a single coex event
    m_duration = 2 * m_mockInterEventTime - MicroSeconds(1);
}

void
WifiCoexDlTxopTest::InsertEvents()
{
    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetNMpdus(),
                                  m_nPackets,
                                  "Unexpected number of MPDUs in first A-MPDU");

            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_apMac->GetFrameExchangeManager(linkId)->GetAddress(),
                                  "Unexpected TA for first QoS data frame");

            NS_TEST_EXPECT_MSG_EQ(Simulator::Now(),
                                  m_dlPktTxTime,
                                  "Unexpected TX time for first QoS data frame");

            const auto phy = m_staMac->GetWifiPhy();
            const auto preambleDetect = WifiPhy::GetPreambleDetectionDuration();
            const auto phyHeader = WifiPhy::CalculatePhyPreambleAndHeaderDuration(txVector);
            const auto txDuration = WifiPhy::CalculateTxDuration(psdu, txVector, phy->GetPhyBand());
            const auto oneMpdu = (txDuration - phyHeader) / m_nPackets;

            // compute and set a proper coex event start delay for scenarios other than
            // BEFORE_PPDU_START
            if (m_scenario != Scenario::BEFORE_PPDU_START)
            {
                switch (m_scenario)
                {
                case Scenario::DURING_PREAMBLE_DETECTION:
                    m_mockEventStartDelay = Time{0};
                    break;
                case Scenario::DURING_PHY_HEADER:
                    m_mockEventStartDelay = preambleDetect;
                    break;
                case Scenario::DURING_FIRST_MPDU:
                    m_mockEventStartDelay = phyHeader;
                    break;
                case Scenario::DURING_SECOND_MPDU:
                    m_mockEventStartDelay = phyHeader + oneMpdu;
                    break;
                case Scenario::BEFORE_BLOCKACK:
                    m_mockEventStartDelay = txDuration;
                    break;
                case Scenario::DURING_BLOCKACK:
                    m_mockEventStartDelay = txDuration + phy->GetSifs();
                    break;
                default:
                    break;
                }
                SetMockEventGen(m_mockInterEventTime, m_mockEventStartDelay, m_mockEventDuration);
            }

            // The STA PHY is already OFF only in the BEFORE_PPDU_START scenario
            NS_TEST_EXPECT_MSG_EQ(phy->IsStateOff(),
                                  (m_scenario == Scenario::BEFORE_PPDU_START),
                                  "Unexpected OFF state at the PPDU start");

            const auto coexEventStartDelay =
                Max(m_mockInterEventTime + m_mockEventStartDelay - m_dlPktTxTime, Time{0});
            // check that PHY is OFF right after the coex event start
            Simulator::Schedule(coexEventStartDelay + TimeStep(1), [=, this] {
                NS_TEST_EXPECT_MSG_EQ(phy->IsStateOff(),
                                      true,
                                      "Expected PHY to be OFF after coex event start");
            });
            // check that PHY is in CCA_BUSY state right after the coex event end, except in the
            // BEFORE_BLOCKACK and DURING_BLOCKACK scenarios
            Simulator::Schedule(coexEventStartDelay + m_mockEventDuration + TimeStep(1), [=, this] {
                NS_TEST_EXPECT_MSG_EQ(phy->IsStateOff(),
                                      false,
                                      "Expected PHY not to be OFF after coex event end");
                NS_TEST_EXPECT_MSG_EQ(phy->IsStateCcaBusy(),
                                      (m_scenario != Scenario::BEFORE_BLOCKACK &&
                                       m_scenario != Scenario::DURING_BLOCKACK),
                                      "Unexpected CCA_BUSY state after coex event end");
            });
        });

    // QoS data frame is not received successfully and is retransmitted later on after BlockAck
    // timeout.
    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetNMpdus(),
                                  m_nPackets,
                                  "Unexpected number of MPDUs in first A-MPDU");

            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_apMac->GetFrameExchangeManager(linkId)->GetAddress(),
                                  "Unexpected TA for first QoS data frame");

            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsRetry(),
                                  true,
                                  "First MPDU should have the Retry flag set");

            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(1).IsRetry(),
                                  true,
                                  "Second MPDU should have the Retry flag set");

            using enum Scenario;

            NS_TEST_EXPECT_MSG_EQ(
                m_rxPkts[1],
                ((m_scenario == BEFORE_BLOCKACK || m_scenario == DURING_BLOCKACK) ? 2
                 : m_scenario == DURING_SECOND_MPDU                               ? 1
                                                                                  : 0),
                "Unexpected number of received packets after first transmission attempt");
        });

    // STA now acknowledges the receipt of the QoS data frame
    m_events.emplace_back(
        WIFI_MAC_CTL_BACKRESP,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(
                m_rxPkts[1],
                2,
                "Unexpected number of received packets after second transmission attempt");
        });
}

void
WifiCoexDlTxopTest::DoRun()
{
    InsertEvents();

    Simulator::Stop(m_duration);
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(m_events.empty(), true, "Not all events took place");

    Simulator::Destroy();
}

void
WifiCoexDlTxopTest::CoexEventCallback(const coex::Event& coexEvent)
{
    WifiCoexTestBase::CoexEventCallback(coexEvent);

    const auto phy = m_staMac->GetWifiPhy();
    const auto fem = m_staMac->GetFrameExchangeManager();
    const auto now = Simulator::Now();
    const auto delay = coexEvent.start - now;
    using enum Scenario;

    // check that the PHY is doing the right thing before and after the coex event start
    switch (m_scenario)
    {
    case BEFORE_PPDU_START:
        // coex event starts now
        NS_TEST_EXPECT_MSG_EQ(
            phy->IsStateIdle(),
            true,
            "Unexpected PHY state before coex event start: " << phy->GetState()->GetState());
        break;
    case DURING_PREAMBLE_DETECTION:
        // coex event starts now
        NS_TEST_EXPECT_MSG_EQ(phy->GetTimeToPreambleDetectionEnd().has_value(),
                              true,
                              "Expected the PHY to be detecting a preamble");
        Simulator::Schedule(delay + TimeStep(1), [=, this] {
            NS_TEST_EXPECT_MSG_EQ(phy->GetTimeToPreambleDetectionEnd().has_value(),
                                  false,
                                  "Expected PHY detection preamble events to be cancelled");
        });
        break;
    case DURING_PHY_HEADER:
        Simulator::Schedule(delay - TimeStep(1), [=, this] {
            NS_TEST_EXPECT_MSG_EQ(phy->GetInfoIfRxingPhyHeader().has_value(),
                                  true,
                                  "Expected the PHY to be receiving a PHY header");
        });
        Simulator::Schedule(delay + TimeStep(1), [=, this] {
            NS_TEST_EXPECT_MSG_EQ(phy->GetInfoIfRxingPhyHeader().has_value(),
                                  false,
                                  "Expected the reception of PHY header to be cancelled");
        });
        break;
    case DURING_FIRST_MPDU:
    case DURING_SECOND_MPDU:
        Simulator::Schedule(delay - TimeStep(1), [=, this] {
            NS_TEST_EXPECT_MSG_EQ(phy->GetTimeToMacHdrEnd(SU_STA_ID).has_value() ||
                                      fem->GetReceivedMacHdr().has_value(),
                                  true,
                                  "Expected the PHY to be receiving an MPDU");
        });
        Simulator::Schedule(delay + TimeStep(1), [=, this] {
            NS_TEST_EXPECT_MSG_EQ(phy->GetTimeToMacHdrEnd(SU_STA_ID).has_value() ||
                                      fem->GetReceivedMacHdr().has_value(),
                                  false,
                                  "Expected the reception of the MPDU to be cancelled");
        });
        break;
    default:
        break;
    }
}

WifiCoexUlTxopTest::WifiCoexUlTxopTest(Scenario scenario, const Time& txopLimit)
    : WifiCoexTestBase("UL TXOP test, scenario " + GetScenarioStr(scenario) + ", TXOP limit " +
                       std::to_string(txopLimit.GetMicroSeconds()) + "us"),
      m_scenario(scenario),
      m_txopLimit(txopLimit)
{
    m_duration = Seconds(1);
}

void
WifiCoexUlTxopTest::DoSetup()
{
    WifiCoexTestBase::DoSetup();

    // set TXOP limit after static setup
    Simulator::ScheduleNow([this] {
        m_staMac->GetQosTxop(AC_BE)->SetAttribute(
            "TxopLimits",
            AttributeContainerValue<TimeValue>(std::list{m_txopLimit}));
    });

    // compute an estimate of the time required to transmit a packet
    auto rsm = m_staMac->GetWifiRemoteStationManager(SINGLE_LINK_OP_ID);
    WifiModeValue dataMode;
    NS_TEST_ASSERT_MSG_EQ(rsm->GetAttributeFailSafe("DataMode", dataMode),
                          true,
                          "Unable to retrieve data mode from constant RSM");
    const auto width = m_staMac->GetWifiPhy()->GetChannelWidth();
    const auto txTime = Seconds(m_pktSize * 8. / dataMode.Get().GetDataRate(width));

    std::cout << "Approximate time " << txTime.As(Time::MS) << "\n\n";

    m_mockInterEventTime = MicroSeconds(500); // time of the first coex event notification
    m_mockEventDuration = MicroSeconds(100);  // arbitrary small value

    switch (m_scenario)
    {
    case Scenario::DEFER_TXOP_START:
        m_mockEventStartDelay = txTime;
        break;
    case Scenario::DEFER_ONE_PACKET:
        m_mockEventStartDelay = 2 * txTime;
        break;
    case Scenario::TXOP_BEFORE_COEX:
        m_mockEventStartDelay = 3 * txTime;
        break;
    default:
        break;
    }

    // In the INTRA_COEX_TXOP scenario, the mock coex event generator is installed after the
    // transmission of the first packet
    if (m_scenario != Scenario::INTRA_TXOP_COEX)
    {
        SetMockEventGen(m_mockInterEventTime, m_mockEventStartDelay, m_mockEventDuration);

        // change the inter event time to twice the simulation duration, so that only one coex event
        // is generated
        SetMockEventGen(2 * m_duration, m_mockEventStartDelay, m_mockEventDuration);
    }

    // generate packets right after the first coex event is notified
    m_pktGenTime = m_mockInterEventTime + TimeStep(1);

    std::size_t nPackets = (m_scenario == Scenario::INTRA_TXOP_COEX ? 1 : 2);

    Simulator::Schedule(m_pktGenTime,
                        &Node::AddApplication,
                        m_staMac->GetDevice()->GetNode(),
                        GetApplication(WifiDirection::UPLINK, nPackets, m_pktSize));
}

void
WifiCoexUlTxopTest::InsertEvents()
{
    switch (m_scenario)
    {
    case Scenario::DEFER_TXOP_START:
        InsertDeferTxopStartEvents();
        break;
    case Scenario::DEFER_ONE_PACKET:
        InsertDeferOnePacketEvents();
        break;
    case Scenario::TXOP_BEFORE_COEX:
        InsertTxopBeforeCoexEvents();
        break;
    case Scenario::INTRA_TXOP_COEX:
        InsertIntraCoexTxopEvents();
        break;
    default:
        NS_ABORT_MSG("Inserting no event for scenario " << m_scenario);
        break;
    }
}

void
WifiCoexUlTxopTest::InsertDeferTxopStartEvents()
{
    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetNMpdus(), 2, "Unexpected number of MPDUs in the A-MPDU");

            NS_TEST_EXPECT_MSG_GT_OR_EQ(Simulator::Now(),
                                        m_mockInterEventTime + m_mockEventStartDelay +
                                            m_mockEventDuration,
                                        "A-MPDU transmitted before the end of the coex event");
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_BACKRESP,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(
                m_rxPkts[0],
                2,
                "Unexpected number of received packets when BlockAck is transmitted");
        });
}

void
WifiCoexUlTxopTest::InsertDeferOnePacketEvents()
{
    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetNMpdus(),
                                  1,
                                  "Unexpected number of MPDUs in the first data frame");

            NS_TEST_EXPECT_MSG_LT(Simulator::Now(),
                                  m_mockInterEventTime + m_mockEventStartDelay,
                                  "First MPDU transmitted after the start of the coex event");
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(
                m_rxPkts[0],
                1,
                "Unexpected number of received packets when the first BlockAck is transmitted");
        });

    if (m_txopLimit.IsStrictlyPositive())
    {
        m_events.emplace_back(WIFI_MAC_CTL_END);
    }

    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetNMpdus(),
                                  1,
                                  "Unexpected number of MPDUs in the second data frame");

            NS_TEST_EXPECT_MSG_GT_OR_EQ(Simulator::Now(),
                                        m_mockInterEventTime + m_mockEventStartDelay +
                                            m_mockEventDuration,
                                        "Second MPDU transmitted before the end of the coex event");
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(
                m_rxPkts[0],
                2,
                "Unexpected number of received packets when the second BlockAck is transmitted");
        });
}

void
WifiCoexUlTxopTest::InsertTxopBeforeCoexEvents()
{
    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetNMpdus(), 2, "Unexpected number of MPDUs in the A-MPDU");

            NS_TEST_EXPECT_MSG_LT(Simulator::Now(),
                                  m_mockInterEventTime + m_mockEventStartDelay,
                                  "A-MPDU transmitted after the start of the coex event");
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_BACKRESP,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(
                m_rxPkts[0],
                2,
                "Unexpected number of received packets when BlockAck is transmitted");
        });
}

void
WifiCoexUlTxopTest::InsertIntraCoexTxopEvents()
{
    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetNMpdus(),
                                  1,
                                  "Unexpected number of MPDUs in the first data frame");

            // enqueue another packet
            m_staMac->GetDevice()->GetNode()->AddApplication(
                GetApplication(WifiDirection::UPLINK, 1, m_pktSize));

            // install the mock coex event generator
            const auto phy = m_staMac->GetWifiPhy();
            const auto txDuration = WifiPhy::CalculateTxDuration(psdu, txVector, phy->GetPhyBand());
            const auto ackTxVector =
                m_apMac->GetWifiRemoteStationManager()->GetAckTxVector(m_staMac->GetAddress(),
                                                                       txVector);
            const auto ackTxDuration =
                WifiPhy::CalculateTxDuration(GetAckSize(), ackTxVector, phy->GetPhyBand());
            // the coex event is notified half of a SIFS after the end of the Ack transmission
            m_mockInterEventTime = txDuration + 1.5 * phy->GetSifs() + ackTxDuration;
            m_mockEventStartDelay = phy->GetSifs() + phy->GetSlot();

            SetMockEventGen(m_mockInterEventTime, m_mockEventStartDelay, m_mockEventDuration);

            // change the inter event time to twice the simulation duration, so that only one coex
            // event is generated
            SetMockEventGen(2 * m_duration, m_mockEventStartDelay, m_mockEventDuration);
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(
                m_rxPkts[0],
                1,
                "Unexpected number of received packets when the first BlockAck is transmitted");
        });

    // Note that, even in the case of non-zero TXOP limit, there is not enough time to transmit
    // a CF-End frame to terminate the first TXOP

    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetNMpdus(),
                                  1,
                                  "Unexpected number of MPDUs in the second data frame");

            NS_TEST_EXPECT_MSG_GT_OR_EQ(Simulator::Now(),
                                        m_mockInterEventTime + m_mockEventStartDelay +
                                            m_mockEventDuration,
                                        "Second MPDU transmitted before the end of the coex event");
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(
                m_rxPkts[0],
                2,
                "Unexpected number of received packets when the second BlockAck is transmitted");
        });
}

void
WifiCoexUlTxopTest::DoRun()
{
    InsertEvents();

    Simulator::Stop(m_duration);
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(m_events.empty(), true, "Not all events took place");

    Simulator::Destroy();
}

void
WifiCoexUlTxopTest::CoexEventCallback(const coex::Event& coexEvent)
{
    WifiCoexTestBase::CoexEventCallback(coexEvent);

    const auto phy = m_staMac->GetWifiPhy();
    const auto fem = m_staMac->GetFrameExchangeManager();
    const auto now = Simulator::Now();
    const auto delay = coexEvent.start - now;
    using enum Scenario;

    std::size_t expRxPktsAfterCoex{0};

    switch (m_scenario)
    {
    case DEFER_TXOP_START:
        break;
    case DEFER_ONE_PACKET:
    case INTRA_TXOP_COEX:
        expRxPktsAfterCoex = 1;
        break;
    case TXOP_BEFORE_COEX:
        expRxPktsAfterCoex = 2;
        break;
    default:
        break;
    }

    Simulator::Schedule(delay, [=, this] {
        NS_TEST_EXPECT_MSG_EQ(m_rxPkts[0],
                              expRxPktsAfterCoex,
                              "Unexpected number of received packet at the end of the coex event");
    });
}

WifiCoexBasicTestSuite::WifiCoexBasicTestSuite()
    : TestSuite("wifi-coex-basic-test", Type::UNIT)
{
    using enum WifiCoexDlTxopTest::Scenario;
    for (const WifiCoexDlTxopTest::Scenario scenario : {BEFORE_PPDU_START,
                                                        DURING_PREAMBLE_DETECTION,
                                                        DURING_PHY_HEADER,
                                                        DURING_FIRST_MPDU,
                                                        DURING_SECOND_MPDU,
                                                        BEFORE_BLOCKACK,
                                                        DURING_BLOCKACK})
    {
        AddTestCase(new WifiCoexDlTxopTest(scenario), TestCase::Duration::QUICK);
    }

    using enum WifiCoexUlTxopTest::Scenario;
    for (const Time& txopLimit : {Time{0}, MicroSeconds(3200)})
    {
        for (const WifiCoexUlTxopTest::Scenario scenario :
             {DEFER_TXOP_START, DEFER_ONE_PACKET, TXOP_BEFORE_COEX, INTRA_TXOP_COEX})
        {
            AddTestCase(new WifiCoexUlTxopTest(scenario, txopLimit), TestCase::Duration::QUICK);
        }
    }
}

static WifiCoexBasicTestSuite g_wifiCoexBasicTestSuite; ///< the test suite
