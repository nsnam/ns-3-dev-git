/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-emlsr-link-switch-test.h"

#include "ns3/advanced-emlsr-manager.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/eht-configuration.h"
#include "ns3/eht-frame-exchange-manager.h"
#include "ns3/interference-helper.h"
#include "ns3/log.h"
#include "ns3/mgt-action-headers.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/pointer.h"
#include "ns3/qos-txop.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/string.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-spectrum-phy-interface.h"

#include <algorithm>
#include <functional>
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiEmlsrLinkSwitchTest");

EmlsrLinkSwitchTest::EmlsrLinkSwitchTest(const Params& params)
    : EmlsrOperationsTestBase(
          std::string("Check EMLSR link switching (switchAuxPhy=") +
          std::to_string(params.switchAuxPhy) + ", resetCamStateAndInterruptSwitch=" +
          std::to_string(params.resetCamStateAndInterruptSwitch) +
          ", auxPhyMaxChWidth=" + std::to_string(params.auxPhyMaxChWidth) + "MHz )"),
      m_switchAuxPhy(params.switchAuxPhy),
      m_resetCamStateAndInterruptSwitch(params.resetCamStateAndInterruptSwitch),
      m_auxPhyMaxChWidth(params.auxPhyMaxChWidth),
      m_countQoSframes(0),
      m_countIcfFrames(0),
      m_countRtsFrames(0),
      m_txPsdusPos(0)
{
    m_nEmlsrStations = 1;
    m_nNonEmlsrStations = 0;
    m_linksToEnableEmlsrOn = {0, 1, 2}; // enable EMLSR on all links right after association
    m_mainPhyId = 1;
    m_establishBaDl = {0};
    m_duration = Seconds(1.0);
    // when aux PHYs do not switch link, the main PHY switches back to its previous link after
    // a TXOP, hence the transition delay must exceed the channel switch delay (default: 250us)
    m_transitionDelay = {MicroSeconds(128)};
}

void
EmlsrLinkSwitchTest::Transmit(Ptr<WifiMac> mac,
                              uint8_t phyId,
                              WifiConstPsduMap psduMap,
                              WifiTxVector txVector,
                              double txPowerW)
{
    EmlsrOperationsTestBase::Transmit(mac, phyId, psduMap, txVector, txPowerW);
    auto linkId = m_txPsdus.back().linkId;

    auto psdu = psduMap.begin()->second;
    auto nodeId = mac->GetDevice()->GetNode()->GetId();

    switch (psdu->GetHeader(0).GetType())
    {
    case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
        NS_ASSERT_MSG(nodeId > 0, "APs do not send AssocReq frames");
        NS_TEST_EXPECT_MSG_EQ(+linkId, +m_mainPhyId, "AssocReq not sent by the main PHY");
        break;

    case WIFI_MAC_MGT_ACTION: {
        auto [category, action] = WifiActionHeader::Peek(psdu->GetPayload(0));

        if (nodeId == 1 && category == WifiActionHeader::PROTECTED_EHT &&
            action.protectedEhtAction ==
                WifiActionHeader::PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION)
        {
            // the EMLSR client is starting the transmission of the EML OMN frame;
            // temporarily block transmissions of QoS data frames from the AP MLD to the
            // non-AP MLD on all the links but the one used for ML setup, so that we know
            // that the first QoS data frame is sent on the link of the main PHY
            std::set<uint8_t> linksToBlock;
            for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
            {
                if (id != m_mainPhyId)
                {
                    linksToBlock.insert(id);
                }
            }
            m_apMac->GetMacQueueScheduler()->BlockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                         AC_BE,
                                                         {WIFI_QOSDATA_QUEUE},
                                                         m_staMacs[0]->GetAddress(),
                                                         m_apMac->GetAddress(),
                                                         {0},
                                                         linksToBlock);
        }
    }
    break;

    case WIFI_MAC_CTL_TRIGGER:
        CheckInitialControlFrame(psduMap, txVector, linkId);
        break;

    case WIFI_MAC_QOSDATA:
        CheckQosFrames(psduMap, txVector, linkId);
        break;

    case WIFI_MAC_CTL_RTS:
        CheckRtsFrame(psduMap, txVector, linkId);
        break;

    default:;
    }
}

void
EmlsrLinkSwitchTest::DoSetup()
{
    Config::SetDefault("ns3::DefaultEmlsrManager::SwitchAuxPhy", BooleanValue(m_switchAuxPhy));
    Config::SetDefault("ns3::EmlsrManager::ResetCamState",
                       BooleanValue(m_resetCamStateAndInterruptSwitch));
    Config::SetDefault("ns3::AdvancedEmlsrManager::InterruptSwitch",
                       BooleanValue(m_resetCamStateAndInterruptSwitch));
    Config::SetDefault("ns3::EmlsrManager::AuxPhyChannelWidth", UintegerValue(m_auxPhyMaxChWidth));
    Config::SetDefault("ns3::WifiPhy::ChannelSwitchDelay", TimeValue(MicroSeconds(45)));

    EmlsrOperationsTestBase::DoSetup();

    m_errorModel = CreateObject<ListErrorModel>();
    for (std::size_t linkId = 0; linkId < m_apMac->GetNLinks(); ++linkId)
    {
        m_apMac->GetWifiPhy(linkId)->SetPostReceptionErrorModel(m_errorModel);
    }

    // use channels of different widths
    for (auto mac : std::initializer_list<Ptr<WifiMac>>{m_apMac, m_staMacs[0]})
    {
        mac->GetWifiPhy(0)->SetOperatingChannel(
            WifiPhy::ChannelTuple{4, 40, WIFI_PHY_BAND_2_4GHZ, 1});
        mac->GetWifiPhy(1)->SetOperatingChannel(
            WifiPhy::ChannelTuple{58, 80, WIFI_PHY_BAND_5GHZ, 3});
        mac->GetWifiPhy(2)->SetOperatingChannel(
            WifiPhy::ChannelTuple{79, 160, WIFI_PHY_BAND_6GHZ, 7});
    }
}

void
EmlsrLinkSwitchTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    CheckResults();

    Simulator::Destroy();
}

void
EmlsrLinkSwitchTest::CheckQosFrames(const WifiConstPsduMap& psduMap,
                                    const WifiTxVector& txVector,
                                    uint8_t linkId)
{
    m_countQoSframes++;

    switch (m_countQoSframes)
    {
    case 1:
        // unblock transmissions on all links
        m_apMac->GetMacQueueScheduler()->UnblockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                       AC_BE,
                                                       {WIFI_QOSDATA_QUEUE},
                                                       m_staMacs[0]->GetAddress(),
                                                       m_apMac->GetAddress(),
                                                       {0},
                                                       {0, 1, 2});
        // block transmissions on the link used for ML setup
        m_apMac->GetMacQueueScheduler()->BlockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                     AC_BE,
                                                     {WIFI_QOSDATA_QUEUE},
                                                     m_staMacs[0]->GetAddress(),
                                                     m_apMac->GetAddress(),
                                                     {0},
                                                     {m_mainPhyId});
        // generate a new data packet, which will be sent on a link other than the one
        // used for ML setup, hence triggering a link switching on the EMLSR client
        m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(DOWNLINK, 0, 2, 1000));
        break;
    case 2:
        // block transmission on the link used to send this QoS data frame
        m_apMac->GetMacQueueScheduler()->BlockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                     AC_BE,
                                                     {WIFI_QOSDATA_QUEUE},
                                                     m_staMacs[0]->GetAddress(),
                                                     m_apMac->GetAddress(),
                                                     {0},
                                                     {linkId});
        // generate a new data packet, which will be sent on the link that has not been used
        // so far, hence triggering another link switching on the EMLSR client
        m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(DOWNLINK, 0, 2, 1000));
        break;
    case 3:
        // block transmission on the link used to send this QoS data frame
        m_apMac->GetMacQueueScheduler()->BlockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                     AC_BE,
                                                     {WIFI_QOSDATA_QUEUE},
                                                     m_staMacs[0]->GetAddress(),
                                                     m_apMac->GetAddress(),
                                                     {0},
                                                     {linkId});
        // unblock transmissions on the link used for ML setup
        m_apMac->GetMacQueueScheduler()->UnblockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                       AC_BE,
                                                       {WIFI_QOSDATA_QUEUE},
                                                       m_staMacs[0]->GetAddress(),
                                                       m_apMac->GetAddress(),
                                                       {0},
                                                       {m_mainPhyId});
        // generate a new data packet, which will be sent again on the link used for ML setup,
        // hence triggering yet another link switching on the EMLSR client
        m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(DOWNLINK, 0, 2, 1000));
        break;
    case 4:
        // block transmissions on all links at non-AP MLD side
        m_staMacs[0]->GetMacQueueScheduler()->BlockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                          AC_BE,
                                                          {WIFI_QOSDATA_QUEUE},
                                                          m_apMac->GetAddress(),
                                                          m_staMacs[0]->GetAddress(),
                                                          {0},
                                                          {0, 1, 2});
        // unblock transmissions on the link used for ML setup (non-AP MLD side)
        m_staMacs[0]->GetMacQueueScheduler()->UnblockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                            AC_BE,
                                                            {WIFI_QOSDATA_QUEUE},
                                                            m_apMac->GetAddress(),
                                                            m_staMacs[0]->GetAddress(),
                                                            {0},
                                                            {m_mainPhyId});
        // trigger establishment of BA agreement with AP as recipient
        m_staMacs[0]->GetDevice()->GetNode()->AddApplication(GetApplication(UPLINK, 0, 4, 1000));
        break;
    case 5:
        // unblock transmissions on all links at non-AP MLD side
        m_staMacs[0]->GetMacQueueScheduler()->UnblockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                            AC_BE,
                                                            {WIFI_QOSDATA_QUEUE},
                                                            m_apMac->GetAddress(),
                                                            m_staMacs[0]->GetAddress(),
                                                            {0},
                                                            {0, 1, 2});
        // block transmissions on the link used for ML setup (non-AP MLD side)
        m_staMacs[0]->GetMacQueueScheduler()->BlockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                          AC_BE,
                                                          {WIFI_QOSDATA_QUEUE},
                                                          m_apMac->GetAddress(),
                                                          m_staMacs[0]->GetAddress(),
                                                          {0},
                                                          {m_mainPhyId});
        // generate a new data packet, which will be sent on a link other than the one
        // used for ML setup, hence triggering a link switching on the EMLSR client
        m_staMacs[0]->GetDevice()->GetNode()->AddApplication(GetApplication(UPLINK, 0, 2, 1000));
        break;
    }
}

/**
 * AUX PHY switching enabled (X = channel switch delay)
 *
 *  |--------- aux PHY A ---------|------ main PHY ------|-------------- aux PHY B -------------
 *                           ┌───┐     ┌───┐
 *                           │ICF│     │QoS│
 * ──────────────────────────┴───┴┬───┬┴───┴┬──┬────────────────────────────────────────────────
 *  [link 0]                      │CTS│     │BA│
 *                                └───┘     └──┘
 *
 *
 *  |--------- main PHY ----------|------------------ aux PHY A ----------------|--- main PHY ---
 *     ┌───┐     ┌───┐                                                      ┌───┐     ┌───┐
 *     │ICF│     │QoS│                                                      │ICF│     │QoS│
 *  ───┴───┴┬───┬┴───┴┬──┬──────────────────────────────────────────────────┴───┴┬───┬┴───┴┬──┬──
 *  [link 1]│CTS│     │BA│                                                       │CTS│     │BA│
 *          └───┘     └──┘                                                       └───┘     └──┘
 *
 *
 *  |--------------------- aux PHY B --------------------|------ main PHY ------|-- aux PHY A ---
 *                                                   ┌───┐     ┌───┐
 *                                                   │ICF│     │QoS│
 *  ─────────────────────────────────────────────────┴───┴┬───┬┴───┴┬──┬─────────────────────────
 *  [link 2]                                              │CTS│     │BA│
 *                                                        └───┘     └──┘
 *
 * ... continued ...
 *
 *  |----------------------------------------- aux PHY B ---------------------------------------
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 *  [link 0]
 *
 *  |--------- main PHY ----------|X|X|------------------------ aux PHY A ----------------------
 *                 ┌───┐
 *                 │ACK│
 *  ──────────┬───┬┴───┴────────────────────────────────────────────────────────────────────────
 *  [link 1]  │QoS│
 *            └───┘
 *
 *  |-------- aux PHY A ----------|X|---------------------- main PHY ---------------------------
 *                                          ┌──┐
 *                                          │BA│
 *  ────────────────────────┬───X──────┬───┬┴──┴────────────────────────────────────────────────
 *  [link 2]                │RTS│      │QoS│
 *                          └───┘      └───┘
 ************************************************************************************************
 *
 * AUX PHY switching disabled (X = channel switch delay)
 *
 *  |------------------------------------------ aux PHY A ---------------------------------------
 *                                |-- main PHY --|X|
 *                            ┌───┐     ┌───┐
 *                            │ICF│     │QoS│
 *  ──────────────────────────┴───┴┬───┬┴───┴┬──┬────────────────────────────────────────────────
 *  [link 0]                       │CTS│     │BA│
 *                                 └───┘     └──┘
 *
 *                                                 |-main|
 *  |--------- main PHY ----------|                |-PHY-|                |------ main PHY ------
 *     ┌───┐     ┌───┐                                                      ┌───┐     ┌───┐
 *     │ICF│     │QoS│                                                      │ICF│     │QoS│
 *  ───┴───┴┬───┬┴───┴┬──┬──────────────────────────────────────────────────┴───┴┬───┬┴───┴┬──┬──
 *  [link 1]│CTS│     │BA│                                                       │CTS│     │BA│
 *          └───┘     └──┘                                                       └───┘     └──┘
 *
 *
 *  |------------------------------------------ aux PHY B ---------------------------------------
 *                                                       |-- main PHY --|X|
 *                                                   ┌───┐     ┌───┐
 *                                                   │ICF│     │QoS│
 *  ─────────────────────────────────────────────────┴───┴┬───┬┴───┴┬──┬─────────────────────────
 *  [link 2]                                              │CTS│     │BA│
 *                                                        └───┘     └──┘
 *
 * ... continued ...
 *
 *  |----------------------------------------- aux PHY A ---------------------------------------
 * ─────────────────────────────────────────────────────────────────────────────────────────────
 *  [link 0]
 *
 *  |-------- main PHY --------|      |--- main PHY ---|
 *                 ┌───┐
 *                 │ACK│
 *  ──────────┬───┬┴───┴────────────────────────────────────────────────────────────────────────
 *  [link 1]  │QoS│
 *            └───┘
 *
 *  |------------------------------------------ aux PHY B --------------------------------------
 *                              |X||X|                 |X|-------------- main PHY --------------
 *                                                   ┌───┐     ┌──┐
 *                                                   │CTS│     │BA│
 *  ────────────────────────┬───X───────────────┬───┬┴───┴┬───┬┴──┴─────────────────────────────
 *  [link 2]                │RTS│               │RTS│     │QoS│
 *                          └───┘               └───┘     └───┘
 *
 */

void
EmlsrLinkSwitchTest::CheckInitialControlFrame(const WifiConstPsduMap& psduMap,
                                              const WifiTxVector& txVector,
                                              uint8_t linkId)
{
    if (++m_countIcfFrames == 1)
    {
        m_txPsdusPos = m_txPsdus.size() - 1;
    }

    // the first ICF is sent to protect ADDBA_REQ for DL BA agreement, then one ICF is sent before
    // each of the 4 DL QoS Data frames; finally, another ICF is sent before the ADDBA_RESP for UL
    // BA agreement. Hence, at any time the number of ICF sent is always greater than or equal to
    // the number of QoS data frames sent.
    NS_TEST_EXPECT_MSG_GT_OR_EQ(m_countIcfFrames, m_countQoSframes, "Unexpected number of ICFs");

    auto mainPhy = m_staMacs[0]->GetDevice()->GetPhy(m_mainPhyId);
    auto phyRecvIcf = m_staMacs[0]->GetWifiPhy(linkId); // PHY receiving the ICF

    auto currMainPhyLinkId = m_staMacs[0]->GetLinkForPhy(mainPhy);
    NS_TEST_ASSERT_MSG_EQ(currMainPhyLinkId.has_value(),
                          true,
                          "Didn't find the link on which the Main PHY is operating");
    NS_TEST_ASSERT_MSG_NE(phyRecvIcf,
                          nullptr,
                          "No PHY on the link where ICF " << m_countQoSframes << " was sent");

    if (phyRecvIcf != mainPhy)
    {
        NS_TEST_EXPECT_MSG_LT_OR_EQ(
            phyRecvIcf->GetChannelWidth(),
            m_auxPhyMaxChWidth,
            "Aux PHY that received ICF "
                << m_countQoSframes << " is operating on a channel whose width exceeds the limit");
    }

    // the first two ICFs (before ADDBA_REQ and before first DL QoS Data) and the ICF before the
    // ADDBA_RESP are received by the main PHY. If aux PHYs do not switch links, the ICF before
    // the last DL QoS Data is also received by the main PHY
    NS_TEST_EXPECT_MSG_EQ((phyRecvIcf == mainPhy),
                          (m_countIcfFrames == 1 || m_countIcfFrames == 2 ||
                           (!m_switchAuxPhy && m_countIcfFrames == 5) || m_countIcfFrames == 6),
                          "Expecting that the ICF was received by the main PHY");

    // if aux PHYs do not switch links, the main PHY is operating on its original link when
    // the transmission of an ICF starts
    NS_TEST_EXPECT_MSG_EQ(m_switchAuxPhy || currMainPhyLinkId == m_mainPhyId,
                          true,
                          "Main PHY is operating on an unexpected link ("
                              << +currMainPhyLinkId.value() << ", expected " << +m_mainPhyId
                              << ")");

    auto txDuration =
        WifiPhy::CalculateTxDuration(psduMap, txVector, m_apMac->GetWifiPhy(linkId)->GetPhyBand());

    // check that PHYs are operating on the expected link after the reception of the ICF
    Simulator::Schedule(txDuration + NanoSeconds(1), [=, this]() {
        // the main PHY must be operating on the link where ICF was sent
        NS_TEST_EXPECT_MSG_EQ(m_staMacs[0]->GetWifiPhy(linkId),
                              mainPhy,
                              "PHY operating on link where ICF was sent is not the main PHY");

        // the behavior of Aux PHYs depends on whether they switch channel or not
        if (m_switchAuxPhy)
        {
            if (mainPhy != phyRecvIcf)
            {
                NS_TEST_EXPECT_MSG_EQ(phyRecvIcf->IsStateSwitching(),
                                      true,
                                      "Aux PHY expected to switch channel");
            }
            Simulator::Schedule(phyRecvIcf->GetChannelSwitchDelay(), [=, this]() {
                NS_TEST_EXPECT_MSG_EQ(m_staMacs[0]->GetWifiPhy(*currMainPhyLinkId),
                                      phyRecvIcf,
                                      "The Aux PHY that received the ICF is expected to operate "
                                      "on the link where Main PHY was before switching channel");
            });
        }
        else
        {
            NS_TEST_EXPECT_MSG_EQ(phyRecvIcf->IsStateSwitching(),
                                  false,
                                  "Aux PHY is not expected to switch channel");
            NS_TEST_EXPECT_MSG_EQ(phyRecvIcf->GetPhyBand(),
                                  mainPhy->GetPhyBand(),
                                  "The Aux PHY that received the ICF is expected to operate "
                                  "on the same band as the Main PHY");
        }
    });
}

void
EmlsrLinkSwitchTest::CheckRtsFrame(const WifiConstPsduMap& psduMap,
                                   const WifiTxVector& txVector,
                                   uint8_t linkId)
{
    // corrupt the first RTS frame (sent by the EMLSR client)
    if (++m_countRtsFrames == 1)
    {
        auto psdu = psduMap.begin()->second;
        m_errorModel->SetList({psdu->GetPacket()->GetUid()});

        // check that when CTS timeout occurs, the main PHY is switching
        Simulator::Schedule(
            m_staMacs[0]->GetFrameExchangeManager(linkId)->GetWifiTxTimer().GetDelayLeft() -
                TimeStep(1),
            [=, this]() {
                // store the time to complete the current channel switch at CTS timeout
                auto mainPhy = m_staMacs[0]->GetDevice()->GetPhy(m_mainPhyId);
                auto toCurrSwitchEnd = mainPhy->GetDelayUntilIdle() + TimeStep(1);

                Simulator::Schedule(TimeStep(1), [=, this]() {
                    NS_TEST_EXPECT_MSG_EQ(mainPhy->IsStateSwitching(),
                                          true,
                                          "Main PHY expected to be in SWITCHING state instead of "
                                              << mainPhy->GetState()->GetState());

                    // If main PHY channel switch can be interrupted, the main PHY should be back
                    // operating on the preferred link after a channel switch delay. Otherwise, it
                    // will be operating on the preferred link, if SwitchAuxPhy is false, or on the
                    // link used to send the RTS, if SwitchAuxPhy is true, after the remaining
                    // channel switching time plus the channel switch delay.
                    auto newLinkId = (m_resetCamStateAndInterruptSwitch || !m_switchAuxPhy)
                                         ? m_mainPhyId
                                         : linkId;
                    auto delayLeft = m_resetCamStateAndInterruptSwitch
                                         ? Time{0}
                                         : toCurrSwitchEnd; // time to complete current switch
                    if (m_resetCamStateAndInterruptSwitch || !m_switchAuxPhy)
                    {
                        // add the time to perform another channel switch
                        delayLeft += mainPhy->GetChannelSwitchDelay();
                    }

                    auto totalSwitchDelay =
                        delayLeft + (mainPhy->GetChannelSwitchDelay() - toCurrSwitchEnd);

                    Simulator::Schedule(delayLeft - TimeStep(1), [=, this]() {
                        // check if the MSD timer was running on the link left by the main PHY
                        // before completing channel switch
                        bool msdWasRunning = m_staMacs[0]
                                                 ->GetEmlsrManager()
                                                 ->GetElapsedMediumSyncDelayTimer(m_mainPhyId)
                                                 .has_value();

                        Simulator::Schedule(TimeStep(2), [=, this]() {
                            auto id = m_staMacs[0]->GetLinkForPhy(mainPhy);
                            NS_TEST_EXPECT_MSG_EQ(id.has_value(),
                                                  true,
                                                  "Expected main PHY to operate on a link");
                            NS_TEST_EXPECT_MSG_EQ(*id,
                                                  newLinkId,
                                                  "Main PHY is operating on an unexpected link");
                            const auto startMsd = (totalSwitchDelay > MEDIUM_SYNC_THRESHOLD);
                            const auto msdIsRunning = msdWasRunning || startMsd;
                            CheckMsdTimerRunning(
                                m_staMacs[0],
                                m_mainPhyId,
                                msdIsRunning,
                                std::string("because total switch delay was ") +
                                    std::to_string(totalSwitchDelay.GetNanoSeconds()) + "ns");
                        });
                    });
                });
            });
    }
    // block transmissions on all other links at non-AP MLD side
    std::set<uint8_t> links{0, 1, 2};
    links.erase(linkId);
    m_staMacs[0]->GetMacQueueScheduler()->BlockQueues(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                                      AC_BE,
                                                      {WIFI_QOSDATA_QUEUE},
                                                      m_apMac->GetAddress(),
                                                      m_staMacs[0]->GetAddress(),
                                                      {0},
                                                      links);
}

void
EmlsrLinkSwitchTest::CheckResults()
{
    NS_TEST_ASSERT_MSG_NE(m_txPsdusPos, 0, "BA agreement establishment not completed");

    // Expected frame exchanges after ML setup and EML OMN exchange:
    //  1. (DL) ICF + CTS + ADDBA_REQ + ACK
    //  2. (UL) ADDBA_RESP + ACK
    //  3. (DL) ICF + CTS + DATA + BA
    //  4. (DL) ICF + CTS + DATA + BA
    //  5. (DL) ICF + CTS + DATA + BA
    //  6. (DL) ICF + CTS + DATA + BA
    //  7. (UL) ADDBA_REQ + ACK
    //  8. (DL) ICF + CTS + ADDBA_RESP + ACK
    //  9. (UL) DATA + BA
    // 10. (UL) RTS - CTS timeout
    // 11. (UL) (RTS + CTS + ) DATA + BA

    // frame exchange 11 is protected if SwitchAuxPhy is false or (SwitchAuxPhy is true and) the
    // main PHY switch can be interrupted
    bool fe11protected = !m_switchAuxPhy || m_resetCamStateAndInterruptSwitch;

    NS_TEST_EXPECT_MSG_EQ(m_countIcfFrames, 6, "Unexpected number of ICFs sent");

    // frame exchanges without RTS because the EMLSR client sent the initial frame through main PHY
    const std::size_t nFrameExchNoRts = fe11protected ? 3 : 4;

    const std::size_t nFrameExchWithRts = fe11protected ? 1 : 0;

    NS_TEST_ASSERT_MSG_GT_OR_EQ(m_txPsdus.size(),
                                m_txPsdusPos +
                                    m_countIcfFrames * 4 + /* frames in frame exchange with ICF */
                                    nFrameExchNoRts * 2 + /* frames in frame exchange without RTS */
                                    nFrameExchWithRts * 4 + /* frames in frame exchange with RTS */
                                    1,                      /* corrupted RTS */
                                "Insufficient number of TX PSDUs");

    // m_txPsdusPos points to the first ICF
    auto psduIt = std::next(m_txPsdus.cbegin(), m_txPsdusPos);

    // lambda to increase psduIt while skipping Beacon frames
    auto nextPsdu = [&]() {
        do
        {
            ++psduIt;
        } while (psduIt != m_txPsdus.cend() &&
                 psduIt->psduMap.at(SU_STA_ID)->GetHeader(0).IsBeacon());
    };

    const std::size_t nFrameExchanges =
        m_countIcfFrames + nFrameExchNoRts + nFrameExchWithRts + 1 /* corrupted RTS */;

    for (std::size_t i = 1; i <= nFrameExchanges; ++i)
    {
        if (i == 1 || (i >= 3 && i <= 6) || i == 8 || i == 10 || (i == 11 && fe11protected))
        {
            // frame exchanges with protection
            NS_TEST_EXPECT_MSG_EQ((psduIt->psduMap.size() == 1 &&
                                   (i < 9 ? psduIt->psduMap.at(SU_STA_ID)->GetHeader(0).IsTrigger()
                                          : psduIt->psduMap.at(SU_STA_ID)->GetHeader(0).IsRts())),
                                  true,
                                  "Expected a Trigger Frame (ICF)");
            nextPsdu();
            if (i == 10)
            {
                continue; // corrupted RTS
            }
            NS_TEST_EXPECT_MSG_EQ((psduIt->psduMap.size() == 1 &&
                                   psduIt->psduMap.at(SU_STA_ID)->GetHeader(0).IsCts()),
                                  true,
                                  "Expected a CTS");
            nextPsdu();
        }

        if (i == 1 || i == 2 || i == 7 || i == 8) // frame exchanges with ADDBA REQ/RESP frames
        {
            NS_TEST_EXPECT_MSG_EQ((psduIt->psduMap.size() == 1 &&
                                   psduIt->psduMap.at(SU_STA_ID)->GetHeader(0).IsMgt()),
                                  true,
                                  "Expected a management frame");
            nextPsdu();
            NS_TEST_EXPECT_MSG_EQ((psduIt->psduMap.size() == 1 &&
                                   psduIt->psduMap.at(SU_STA_ID)->GetHeader(0).IsAck()),
                                  true,
                                  "Expected a Normal Ack");
        }
        else
        {
            NS_TEST_EXPECT_MSG_EQ((psduIt->psduMap.size() == 1 &&
                                   psduIt->psduMap.at(SU_STA_ID)->GetHeader(0).IsQosData()),
                                  true,
                                  "Expected a QoS Data frame");
            nextPsdu();
            NS_TEST_EXPECT_MSG_EQ((psduIt->psduMap.size() == 1 &&
                                   psduIt->psduMap.at(SU_STA_ID)->GetHeader(0).IsBlockAck()),
                                  true,
                                  "Expected a BlockAck");
        }
        nextPsdu();
    }
}

EmlsrCcaBusyTest::EmlsrCcaBusyTest(MHz_u auxPhyMaxChWidth)
    : EmlsrOperationsTestBase(std::string("Check EMLSR link switching (auxPhyMaxChWidth=") +
                              std::to_string(auxPhyMaxChWidth) + "MHz )"),
      m_auxPhyMaxChWidth(auxPhyMaxChWidth),
      m_channelSwitchDelay(MicroSeconds(75)),
      m_currMainPhyLinkId(0),
      m_nextMainPhyLinkId(0)
{
    m_nEmlsrStations = 1;
    m_nNonEmlsrStations = 1;
    m_linksToEnableEmlsrOn = {0, 1, 2}; // enable EMLSR on all links right after association
    m_mainPhyId = 1;
    m_establishBaUl = {0};
    m_duration = Seconds(1.0);
    m_transitionDelay = {MicroSeconds(128)};
}

void
EmlsrCcaBusyTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    Simulator::Destroy();
}

void
EmlsrCcaBusyTest::DoSetup()
{
    Config::SetDefault("ns3::DefaultEmlsrManager::SwitchAuxPhy", BooleanValue(true));
    Config::SetDefault("ns3::EmlsrManager::AuxPhyChannelWidth", UintegerValue(m_auxPhyMaxChWidth));
    Config::SetDefault("ns3::EmlsrManager::AuxPhyMaxModClass", StringValue("EHT"));
    Config::SetDefault("ns3::WifiPhy::ChannelSwitchDelay", TimeValue(m_channelSwitchDelay));

    EmlsrOperationsTestBase::DoSetup();

    // use channels of different widths
    for (auto mac : std::initializer_list<Ptr<WifiMac>>{m_apMac, m_staMacs[0], m_staMacs[1]})
    {
        mac->GetWifiPhy(0)->SetOperatingChannel(
            WifiPhy::ChannelTuple{4, 40, WIFI_PHY_BAND_2_4GHZ, 0});
        mac->GetWifiPhy(1)->SetOperatingChannel(
            WifiPhy::ChannelTuple{58, 80, WIFI_PHY_BAND_5GHZ, 0});
        mac->GetWifiPhy(2)->SetOperatingChannel(
            WifiPhy::ChannelTuple{79, 160, WIFI_PHY_BAND_6GHZ, 0});
    }
}

void
EmlsrCcaBusyTest::TransmitPacketToAp(uint8_t linkId)
{
    m_staMacs[1]->GetDevice()->GetNode()->AddApplication(GetApplication(UPLINK, 1, 1, 2000));

    // force the transmission of the packet to happen now on the given link.
    // Multiple ScheduleNow calls are needed because Node::AddApplication() schedules a call to
    // Application::Initialize(), which schedules a call to Application::StartApplication(), which
    // schedules a call to PacketSocketClient::Send(), which finally generates the packet
    Simulator::ScheduleNow([=, this]() {
        Simulator::ScheduleNow([=, this]() {
            Simulator::ScheduleNow([=, this]() {
                m_staMacs[1]->GetFrameExchangeManager(linkId)->StartTransmission(
                    m_staMacs[1]->GetQosTxop(AC_BE),
                    m_staMacs[1]->GetWifiPhy(linkId)->GetChannelWidth());
            });
        });
    });

    // check that the other MLD started transmitting on the correct link
    Simulator::Schedule(TimeStep(1), [=, this]() {
        NS_TEST_EXPECT_MSG_EQ(m_staMacs[1]->GetWifiPhy(linkId)->IsStateTx(),
                              true,
                              "At time " << Simulator::Now().As(Time::NS)
                                         << ", other MLD did not start transmitting on link "
                                         << +linkId);
    });
}

void
EmlsrCcaBusyTest::StartTraffic()
{
    auto currMainPhyLinkId = m_staMacs[0]->GetLinkForPhy(m_mainPhyId);
    NS_TEST_ASSERT_MSG_EQ(currMainPhyLinkId.has_value(),
                          true,
                          "Main PHY is not operating on any link");
    m_currMainPhyLinkId = *currMainPhyLinkId;
    m_nextMainPhyLinkId = (m_currMainPhyLinkId + 1) % 2;

    // request the main PHY to switch to another link
    m_staMacs[0]->GetEmlsrManager()->SwitchMainPhy(
        m_nextMainPhyLinkId,
        false,
        EmlsrManager::DONT_REQUEST_ACCESS,
        EmlsrDlTxopIcfReceivedByAuxPhyTrace{}); // trace info not used

    // the other MLD transmits a packet to the AP
    TransmitPacketToAp(m_nextMainPhyLinkId);

    // schedule another packet transmission slightly (10 us) before the end of aux PHY switch
    Simulator::Schedule(m_channelSwitchDelay - MicroSeconds(10),
                        &EmlsrCcaBusyTest::TransmitPacketToAp,
                        this,
                        m_currMainPhyLinkId);

    // first checkpoint is after that the preamble of the PPDU has been received
    Simulator::Schedule(MicroSeconds(8), &EmlsrCcaBusyTest::CheckPoint1, this);
}

/**
 *                               ┌───────────────┐
 *  [link X]                     │  other to AP  │CP3
 * ──────────────────────────────┴───────────────┴──────────────────────────────────────────────
 *  |------ main PHY ------|                  |------------------- aux PHY ---------------------
 *                         .\_              _/
 *                         .  \_          _/
 *                         .    \_      _/
 *                         .      \_  _/
 *  [link Y]               . CP1    \/ CP2
 *                         .┌───────────────┐
 *                         .│  other to AP  │
 * ─────────────────────────┴───────────────┴────────────────────────────────────────────────────
 *  |------------ aux PHY ----------|---------------------- main PHY ----------------------------
 *
 */

void
EmlsrCcaBusyTest::CheckPoint1()
{
    // first checkpoint is after that the preamble of the first PPDU has been received
    auto mainPhy = m_staMacs[0]->GetDevice()->GetPhy(m_mainPhyId);

    // 1. Main PHY is switching
    NS_TEST_EXPECT_MSG_EQ(mainPhy->IsStateSwitching(), true, "Main PHY is not switching");

    auto auxPhy = m_staMacs[0]->GetWifiPhy(m_nextMainPhyLinkId);
    NS_TEST_EXPECT_MSG_NE(mainPhy, auxPhy, "Main PHY is operating on an unexpected link");

    // 2. Aux PHY is receiving the PHY header
    NS_TEST_EXPECT_MSG_EQ(auxPhy->GetInfoIfRxingPhyHeader().has_value(),
                          true,
                          "Aux PHY is not receiving a PHY header");

    // 3. Main PHY dropped the preamble because it is switching
    NS_TEST_EXPECT_MSG_EQ(mainPhy->GetInfoIfRxingPhyHeader().has_value(),
                          false,
                          "Main PHY is receiving a PHY header");

    // 4. Channel access manager on destination link (Y) has been notified of CCA busy, but not
    // until the end of transmission (main PHY dropped the preamble and notified CCA busy until
    // end of transmission but the channel access manager on link Y does not yet have a listener
    // attached to the main PHY; aux PHY notified CCA busy until the end of the PHY header field
    // being received)
    const auto caManager = m_staMacs[0]->GetChannelAccessManager(m_nextMainPhyLinkId);
    const auto endTxTime = m_staMacs[1]->GetChannelAccessManager(m_nextMainPhyLinkId)->m_lastTxEnd;
    NS_TEST_ASSERT_MSG_EQ(caManager->m_lastBusyEnd.contains(WIFI_CHANLIST_PRIMARY),
                          true,
                          "No CCA information for primary20 channel");
    NS_TEST_EXPECT_MSG_GT_OR_EQ(
        caManager->m_lastBusyEnd[WIFI_CHANLIST_PRIMARY],
        Simulator::Now(),
        "ChannelAccessManager on destination link not notified of CCA busy");
    NS_TEST_EXPECT_MSG_LT(
        caManager->m_lastBusyEnd[WIFI_CHANLIST_PRIMARY],
        endTxTime,
        "ChannelAccessManager on destination link notified of CCA busy until end of transmission");

    // second checkpoint is after that the main PHY completed the link switch
    Simulator::Schedule(mainPhy->GetDelayUntilIdle() + TimeStep(1),
                        &EmlsrCcaBusyTest::CheckPoint2,
                        this);
}

void
EmlsrCcaBusyTest::CheckPoint2()
{
    // second checkpoint is after that the main PHY completed the link switch. The channel access
    // manager on destination link (Y) is expected to be notified by the main PHY that medium is
    // busy until the end of the ongoing transmission
    const auto caManager = m_staMacs[0]->GetChannelAccessManager(m_nextMainPhyLinkId);
    const auto endTxTime = m_staMacs[1]->GetChannelAccessManager(m_nextMainPhyLinkId)->m_lastTxEnd;
    NS_TEST_ASSERT_MSG_EQ(caManager->m_lastBusyEnd.contains(WIFI_CHANLIST_PRIMARY),
                          true,
                          "No CCA information for primary20 channel");
    NS_TEST_EXPECT_MSG_GT_OR_EQ(
        caManager->m_lastBusyEnd[WIFI_CHANLIST_PRIMARY],
        Simulator::Now(),
        "ChannelAccessManager on destination link not notified of CCA busy");
    NS_TEST_EXPECT_MSG_GT_OR_EQ(caManager->m_lastBusyEnd[WIFI_CHANLIST_PRIMARY],
                                endTxTime,
                                "ChannelAccessManager on destination link not notified of CCA busy "
                                "until end of transmission");

    // third checkpoint is after that the aux PHY completed the link switch
    Simulator::Schedule(m_channelSwitchDelay, &EmlsrCcaBusyTest::CheckPoint3, this);
}

void
EmlsrCcaBusyTest::CheckPoint3()
{
    // third checkpoint is after that the aux PHY completed the link switch. The channel access
    // manager on source link (X) is expected to be notified by the aux PHY that medium is
    // busy until the end of the ongoing transmission (even if the aux PHY was not listening to
    // link X when transmission started, its interface on link X recorded the transmission)
    const auto caManager = m_staMacs[0]->GetChannelAccessManager(m_currMainPhyLinkId);
    const auto endTxTime = m_staMacs[1]->GetChannelAccessManager(m_currMainPhyLinkId)->m_lastTxEnd;
    NS_TEST_ASSERT_MSG_EQ(caManager->m_lastBusyEnd.contains(WIFI_CHANLIST_PRIMARY),
                          true,
                          "No CCA information for primary20 channel");
    NS_TEST_EXPECT_MSG_GT_OR_EQ(caManager->m_lastBusyEnd[WIFI_CHANLIST_PRIMARY],
                                Simulator::Now(),
                                "ChannelAccessManager on source link not notified of CCA busy");
    NS_TEST_EXPECT_MSG_GT_OR_EQ(caManager->m_lastBusyEnd[WIFI_CHANLIST_PRIMARY],
                                endTxTime,
                                "ChannelAccessManager on source link not notified of CCA busy "
                                "until end of transmission");
}

SingleLinkEmlsrTest::SingleLinkEmlsrTest(bool switchAuxPhy, bool auxPhyTxCapable)
    : EmlsrOperationsTestBase(
          "Check EMLSR single link operation (switchAuxPhy=" + std::to_string(switchAuxPhy) +
          ", auxPhyTxCapable=" + std::to_string(auxPhyTxCapable) + ")"),
      m_switchAuxPhy(switchAuxPhy),
      m_auxPhyTxCapable(auxPhyTxCapable)
{
    m_mainPhyId = 0;
    m_linksToEnableEmlsrOn = {m_mainPhyId};
    m_nPhysPerEmlsrDevice = 1;
    m_nEmlsrStations = 1;
    m_nNonEmlsrStations = 0;

    // channel switch delay will be also set to 64 us
    m_paddingDelay = {MicroSeconds(64)};
    m_transitionDelay = {MicroSeconds(64)};
    m_establishBaDl = {0};
    m_establishBaUl = {0};
    m_duration = Seconds(0.5);
}

void
SingleLinkEmlsrTest::DoSetup()
{
    Config::SetDefault("ns3::WifiPhy::ChannelSwitchDelay", TimeValue(MicroSeconds(64)));
    Config::SetDefault("ns3::DefaultEmlsrManager::SwitchAuxPhy", BooleanValue(m_switchAuxPhy));
    Config::SetDefault("ns3::EmlsrManager::AuxPhyTxCapable", BooleanValue(m_auxPhyTxCapable));

    EmlsrOperationsTestBase::DoSetup();
}

void
SingleLinkEmlsrTest::Transmit(Ptr<WifiMac> mac,
                              uint8_t phyId,
                              WifiConstPsduMap psduMap,
                              WifiTxVector txVector,
                              double txPowerW)
{
    EmlsrOperationsTestBase::Transmit(mac, phyId, psduMap, txVector, txPowerW);

    const auto psdu = psduMap.cbegin()->second;
    const auto& hdr = psdu->GetHeader(0);

    // nothing to do in case of Beacon and CF-End frames
    if (hdr.IsBeacon() || hdr.IsCfEnd())
    {
        return;
    }

    auto linkId = mac->GetLinkForPhy(phyId);
    NS_TEST_ASSERT_MSG_EQ(linkId.has_value(),
                          true,
                          "PHY " << +phyId << " is not operating on any link");
    NS_TEST_EXPECT_MSG_EQ(+linkId.value(), 0, "TX occurred on unexpected link " << +linkId.value());

    if (m_eventIt != m_events.cend())
    {
        // check that the expected frame is being transmitted
        NS_TEST_EXPECT_MSG_EQ(m_eventIt->hdrType,
                              hdr.GetType(),
                              "Unexpected MAC header type for frame #"
                                  << std::distance(m_events.cbegin(), m_eventIt));
        // perform actions/checks, if any
        if (m_eventIt->func)
        {
            m_eventIt->func(psdu, txVector);
        }

        ++m_eventIt;
    }
}

void
SingleLinkEmlsrTest::DoRun()
{
    // lambda to check that AP MLD started the transition delay timer after the TX/RX of given frame
    auto checkTransDelay = [this](Ptr<const WifiPsdu> psdu,
                                  const WifiTxVector& txVector,
                                  bool testUnblockedForOtherReasons,
                                  const std::string& frameStr) {
        const auto txDuration = WifiPhy::CalculateTxDuration(psdu->GetSize(),
                                                             txVector,
                                                             m_apMac->GetWifiPhy(0)->GetPhyBand());
        Simulator::Schedule(txDuration + MicroSeconds(1), /* to account for propagation delay */
                            &SingleLinkEmlsrTest::CheckBlockedLink,
                            this,
                            m_apMac,
                            m_staMacs[0]->GetAddress(),
                            0,
                            WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY,
                            true,
                            "Checking that AP MLD blocked transmissions to single link EMLSR "
                            "client after " +
                                frameStr,
                            testUnblockedForOtherReasons);
    };

    // expected sequence of transmitted frames
    m_events.emplace_back(WIFI_MAC_MGT_ASSOCIATION_REQUEST);
    m_events.emplace_back(WIFI_MAC_CTL_ACK);
    m_events.emplace_back(WIFI_MAC_MGT_ASSOCIATION_RESPONSE);
    m_events.emplace_back(WIFI_MAC_CTL_ACK);

    // EML OMN sent by EMLSR client
    m_events.emplace_back(WIFI_MAC_MGT_ACTION,
                          [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) {
                              // check that the address of the EMLSR client is seen as an MLD
                              // address
                              NS_TEST_EXPECT_MSG_EQ(
                                  m_apMac->GetWifiRemoteStationManager(0)
                                      ->GetMldAddress(m_staMacs[0]->GetAddress())
                                      .has_value(),
                                  true,
                                  "Expected the EMLSR client address to be seen as an MLD address");
                          });
    m_events.emplace_back(WIFI_MAC_CTL_ACK);
    // EML OMN sent by AP MLD, protected by ICF
    m_events.emplace_back(WIFI_MAC_CTL_TRIGGER);
    m_events.emplace_back(WIFI_MAC_CTL_CTS);
    m_events.emplace_back(WIFI_MAC_MGT_ACTION);
    m_events.emplace_back(WIFI_MAC_CTL_ACK,
                          [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) {
                              // check that EMLSR mode has been enabled on link 0 of EMLSR client
                              NS_TEST_EXPECT_MSG_EQ(
                                  m_staMacs[0]->IsEmlsrLink(0),
                                  true,
                                  "Expected EMLSR mode to be enabled on link 0 of EMLSR client");
                          });

    // Establishment of BA agreement for downlink direction

    // ADDBA REQUEST sent by AP MLD (protected by ICF)
    m_events.emplace_back(WIFI_MAC_CTL_TRIGGER);
    m_events.emplace_back(WIFI_MAC_CTL_CTS);
    m_events.emplace_back(WIFI_MAC_MGT_ACTION);
    m_events.emplace_back(WIFI_MAC_CTL_ACK,
                          [=](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) {
                              // check that transition delay is started after reception of Ack
                              checkTransDelay(psdu, txVector, false, "DL ADDBA REQUEST");
                          });

    // ADDBA RESPONSE sent by EMLSR client (no RTS because it is sent by main PHY)
    m_events.emplace_back(WIFI_MAC_MGT_ACTION);
    m_events.emplace_back(WIFI_MAC_CTL_ACK,
                          [=](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) {
                              // check that transition delay is started after reception of Ack
                              checkTransDelay(psdu, txVector, true, "DL ADDBA RESPONSE");
                          });

    // Downlink QoS data frame that triggered BA agreement establishment
    m_events.emplace_back(WIFI_MAC_CTL_TRIGGER);
    m_events.emplace_back(WIFI_MAC_CTL_CTS);
    m_events.emplace_back(WIFI_MAC_QOSDATA);
    m_events.emplace_back(WIFI_MAC_CTL_BACKRESP,
                          [=](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) {
                              // check that transition delay is started after reception of BlockAck
                              checkTransDelay(psdu, txVector, true, "DL QoS Data");
                          });

    // Establishment of BA agreement for uplink direction

    // ADDBA REQUEST sent by EMLSR client (no RTS because it is sent by main PHY)
    m_events.emplace_back(WIFI_MAC_MGT_ACTION);
    m_events.emplace_back(WIFI_MAC_CTL_ACK,
                          [=](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) {
                              // check that transition delay is started after reception of Ack
                              checkTransDelay(psdu, txVector, false, "UL ADDBA REQUEST");
                          });
    // ADDBA RESPONSE sent by AP MLD (protected by ICF)
    m_events.emplace_back(WIFI_MAC_CTL_TRIGGER);
    m_events.emplace_back(WIFI_MAC_CTL_CTS);
    m_events.emplace_back(WIFI_MAC_MGT_ACTION);
    m_events.emplace_back(WIFI_MAC_CTL_ACK,
                          [=](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) {
                              // check that transition delay is started after reception of Ack
                              checkTransDelay(psdu, txVector, true, "UL ADDBA RESPONSE");
                          });

    // Uplink QoS data frame that triggered BA agreement establishment
    m_events.emplace_back(WIFI_MAC_QOSDATA);
    m_events.emplace_back(WIFI_MAC_CTL_BACKRESP,
                          [=](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) {
                              // check that transition delay is started after reception of BlockAck
                              checkTransDelay(psdu, txVector, true, "UL QoS Data");
                          });

    m_eventIt = m_events.cbegin();

    Simulator::Stop(m_duration);
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ((m_eventIt == m_events.cend()), true, "Not all events took place");

    Simulator::Destroy();
}

EmlsrIcfSentDuringMainPhySwitchTest::EmlsrIcfSentDuringMainPhySwitchTest()
    : EmlsrOperationsTestBase("Check ICF reception while main PHY is switching")
{
    m_mainPhyId = 0;
    m_linksToEnableEmlsrOn = {0, 1, 2};
    m_nEmlsrStations = 1;
    m_nNonEmlsrStations = 0;

    // channel switch delay will be also set to 64 us
    m_paddingDelay = {MicroSeconds(128)};
    m_transitionDelay = {MicroSeconds(64)};
    m_establishBaDl = {0, 3};
    m_establishBaUl = {0, 3};
    m_duration = Seconds(0.5);
}

void
EmlsrIcfSentDuringMainPhySwitchTest::DoSetup()
{
    // channel switch delay will be modified during test scenarios
    Config::SetDefault("ns3::WifiPhy::ChannelSwitchDelay", TimeValue(MicroSeconds(64)));
    Config::SetDefault("ns3::WifiPhy::NotifyMacHdrRxEnd", BooleanValue(true));
    Config::SetDefault("ns3::DefaultEmlsrManager::SwitchAuxPhy", BooleanValue(false));
    Config::SetDefault("ns3::EmlsrManager::AuxPhyTxCapable", BooleanValue(false));
    // AP MLD transmits both TID 0 and TID 3 on link 1
    Config::SetDefault("ns3::EhtConfiguration::TidToLinkMappingDl", StringValue("0,3 1"));
    // EMLSR client transmits TID 0 on link 1 and TID 3 on link 2
    Config::SetDefault("ns3::EhtConfiguration::TidToLinkMappingUl",
                       StringValue("0 1; 3 " + std::to_string(m_linkIdForTid3)));

    EmlsrOperationsTestBase::DoSetup();

    m_staMacs[0]->TraceConnectWithoutContext(
        "EmlsrLinkSwitch",
        MakeCallback(&EmlsrIcfSentDuringMainPhySwitchTest::EmlsrLinkSwitchCb, this));

    for (uint8_t i = 0; i < m_staMacs[0]->GetDevice()->GetNPhys(); ++i)
    {
        m_bands.at(i) = m_staMacs[0]->GetDevice()->GetPhy(i)->GetBand(MHz_u{20}, 0);
    }
}

void
EmlsrIcfSentDuringMainPhySwitchTest::GenerateNoiseOnAllLinks(Ptr<WifiMac> mac, Time duration)
{
    for (const auto linkId : mac->GetLinkIds())
    {
        auto phy = DynamicCast<SpectrumWifiPhy>(mac->GetWifiPhy(linkId));
        NS_TEST_ASSERT_MSG_NE(phy, nullptr, "No PHY on link " << +linkId);
        const auto txPower = phy->GetPower(1) + phy->GetTxGain();

        auto psd = Create<SpectrumValue>(phy->GetCurrentInterface()->GetRxSpectrumModel());
        *psd = txPower;

        auto spectrumSignalParams = Create<SpectrumSignalParameters>();
        spectrumSignalParams->duration = duration;
        spectrumSignalParams->txPhy = phy->GetCurrentInterface();
        spectrumSignalParams->txAntenna = phy->GetAntenna();
        spectrumSignalParams->psd = psd;

        phy->StartRx(spectrumSignalParams, phy->GetCurrentInterface());
    }
}

void
EmlsrIcfSentDuringMainPhySwitchTest::CheckInDeviceInterference(const std::string& frameTypeStr,
                                                               uint8_t linkId,
                                                               Time duration)
{
    for (const auto& phy : m_staMacs[0]->GetDevice()->GetPhys())
    {
        // ignore the PHY that is transmitting
        if (m_staMacs[0]->GetLinkForPhy(phy) == linkId)
        {
            continue;
        }

        PointerValue ptr;
        phy->GetAttribute("InterferenceHelper", ptr);
        auto interferenceHelper = ptr.Get<InterferenceHelper>();

        // we need to check that all the PHY interfaces recorded the in-device interference,
        // hence we consider a 20 MHz sub-band of the frequency channels of all the links
        for (uint8_t i = 0; i < m_staMacs[0]->GetNLinks(); ++i)
        {
            auto energyDuration =
                interferenceHelper->GetEnergyDuration(DbmToW(phy->GetCcaEdThreshold()),
                                                      m_bands.at(i));

            NS_TEST_EXPECT_MSG_EQ(
                energyDuration,
                duration,
                m_testStr << ", " << frameTypeStr << ": Unexpected energy duration for PHY "
                          << +phy->GetPhyId() << " in the band corresponding to link " << +i);
        }
    }
}

void
EmlsrIcfSentDuringMainPhySwitchTest::Transmit(Ptr<WifiMac> mac,
                                              uint8_t phyId,
                                              WifiConstPsduMap psduMap,
                                              WifiTxVector txVector,
                                              double txPowerW)
{
    EmlsrOperationsTestBase::Transmit(mac, phyId, psduMap, txVector, txPowerW);

    const auto psdu = psduMap.cbegin()->second;
    const auto& hdr = psdu->GetHeader(0);

    // nothing to do before setup is completed
    if (!m_setupDone)
    {
        return;
    }

    auto linkId = mac->GetLinkForPhy(phyId);
    NS_TEST_ASSERT_MSG_EQ(linkId.has_value(),
                          true,
                          "PHY " << +phyId << " is not operating on any link");

    if (!m_events.empty())
    {
        // check that the expected frame is being transmitted
        NS_TEST_EXPECT_MSG_EQ(m_events.front().hdrType,
                              hdr.GetType(),
                              "Unexpected MAC header type for frame #" << ++m_processedEvents);
        // perform actions/checks, if any
        if (m_events.front().func)
        {
            m_events.front().func(psdu, txVector, linkId.value());
        }

        m_events.pop_front();
    }
}

void
EmlsrIcfSentDuringMainPhySwitchTest::StartTraffic()
{
    m_setupDone = true;
    RunOne();
}

void
EmlsrIcfSentDuringMainPhySwitchTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(m_events.empty(), true, "Not all events took place");

    Simulator::Destroy();
}

void
EmlsrIcfSentDuringMainPhySwitchTest::EmlsrLinkSwitchCb(uint8_t linkId,
                                                       Ptr<WifiPhy> phy,
                                                       bool connected)
{
    if (!m_setupDone)
    {
        return;
    }

    if (!connected)
    {
        const auto mainPhy = m_staMacs[0]->GetDevice()->GetPhy(m_mainPhyId);
        NS_LOG_DEBUG("Main PHY leaving link " << +linkId << ", switch delay "
                                              << mainPhy->GetChannelSwitchDelay().As(Time::US)
                                              << "\n");
        m_switchFrom = MainPhySwitchInfo{Simulator::Now(), linkId};
        m_switchTo.reset();
    }
    else
    {
        NS_LOG_DEBUG((phy->GetPhyId() == m_mainPhyId ? "Main" : "Aux")
                     << " PHY connected to link " << +linkId << "\n");
        if (phy->GetPhyId() == m_mainPhyId)
        {
            m_switchTo = MainPhySwitchInfo{Simulator::Now(), linkId};
            m_switchFrom.reset();
        }
    }
}

void
EmlsrIcfSentDuringMainPhySwitchTest::RunOne()
{
    const auto useMacHdrInfo = ((m_testIndex & 0b001) != 0);
    const auto interruptSwitch = ((m_testIndex & 0b010) != 0);
    const auto switchToOtherLink = ((m_testIndex & 0b100) != 0);

    const auto keepMainPhyAfterDlTxop = useMacHdrInfo;

    m_staMacs[0]->GetEmlsrManager()->SetAttribute("UseNotifiedMacHdr", BooleanValue(useMacHdrInfo));
    auto advEmlsrMgr = DynamicCast<AdvancedEmlsrManager>(m_staMacs[0]->GetEmlsrManager());
    NS_TEST_ASSERT_MSG_NE(advEmlsrMgr, nullptr, "Advanced EMLSR Manager required");
    advEmlsrMgr->SetAttribute("InterruptSwitch", BooleanValue(interruptSwitch));
    advEmlsrMgr->SetAttribute("KeepMainPhyAfterDlTxop", BooleanValue(keepMainPhyAfterDlTxop));

    m_testStr = "SwitchToOtherLink=" + std::to_string(switchToOtherLink) +
                ", InterruptSwitch=" + std::to_string(interruptSwitch) +
                ", UseMacHdrInfo=" + std::to_string(useMacHdrInfo) +
                ", KeepMainPhyAfterDlTxop=" + std::to_string(keepMainPhyAfterDlTxop) +
                ", ChannelSwitchDurationIdx=" + std::to_string(m_csdIndex);
    NS_LOG_INFO("Starting test: " << m_testStr << "\n");

    // generate noise on all the links of the AP MLD and the EMLSR client, so as to align the EDCA
    // backoff boundaries
    Simulator::Schedule(MilliSeconds(3), [=, this]() {
        GenerateNoiseOnAllLinks(m_apMac, MicroSeconds(500));
        GenerateNoiseOnAllLinks(m_staMacs[0], MicroSeconds(500));
    });

    // wait some more time to ensure that backoffs count down to zero and then generate a packet
    // at the AP MLD and a packet at the EMLSR client. AP MLD and EMLSR client are expected to get
    // access at the same time because backoff counter is zero and EDCA boundaries are aligned
    Simulator::Schedule(MilliSeconds(5), [=, this]() {
        uint8_t prio = (switchToOtherLink ? 3 : 0);
        m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(DOWNLINK, 0, 1, 500, prio));
        m_staMacs[0]->GetDevice()->GetNode()->AddApplication(
            GetApplication(UPLINK, 0, 1, 500, prio));
    });

    m_switchFrom.reset();
    m_switchTo.reset();

    m_events.emplace_back(
        WIFI_MAC_CTL_TRIGGER,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            const auto phyHdrDuration = WifiPhy::CalculatePhyPreambleAndHeaderDuration(txVector);
            const auto txDuration =
                WifiPhy::CalculateTxDuration(psdu,
                                             txVector,
                                             m_apMac->GetWifiPhy(linkId)->GetPhyBand());
            auto mainPhy = m_staMacs[0]->GetDevice()->GetPhy(m_mainPhyId);

            // compute channel switch delay based on the scenario to test
            Time channelSwitchDelay{0};
            const auto margin = MicroSeconds(2);

            switch (m_csdIndex)
            {
            case DURING_PREAMBLE_DETECTION:
                channelSwitchDelay = MicroSeconds(1);
                break;
            case BEFORE_PHY_HDR_END:
                channelSwitchDelay = phyHdrDuration - margin;
                break;
            case BEFORE_MAC_HDR_END:
                channelSwitchDelay = phyHdrDuration + margin;
                break;
            case BEFORE_MAC_PAYLOAD_END:
                channelSwitchDelay = txDuration - m_paddingDelay.at(0) - margin;
                break;
            case BEFORE_PADDING_END:
                channelSwitchDelay = txDuration - m_paddingDelay.at(0) + margin;
                break;
            default:;
            }

            NS_TEST_ASSERT_MSG_EQ(channelSwitchDelay.IsStrictlyPositive(),
                                  true,
                                  m_testStr << ": Channel switch delay is not strictly positive ("
                                            << channelSwitchDelay.As(Time::US) << ")");
            NS_TEST_ASSERT_MSG_LT(channelSwitchDelay,
                                  m_paddingDelay.at(0),
                                  m_testStr
                                      << ": Channel switch delay is greater than padding delay");
            // set channel switch delay
            mainPhy->SetAttribute("ChannelSwitchDelay", TimeValue(channelSwitchDelay));

            const auto startTx = Simulator::Now();

            // check that main PHY has started switching
            Simulator::ScheduleNow([=, this]() {
                NS_TEST_ASSERT_MSG_EQ(m_switchFrom.has_value(),
                                      true,
                                      m_testStr << ": Main PHY did not start switching");
                NS_TEST_EXPECT_MSG_EQ(+m_switchFrom->linkId,
                                      +m_mainPhyId,
                                      m_testStr << ": Main PHY did not left the preferred link");
                NS_TEST_EXPECT_MSG_EQ(m_switchFrom->time,
                                      startTx,
                                      m_testStr
                                          << ": Main PHY did not start switching at ICF TX start");
            });

            // check what happens after channel switch is completed
            Simulator::Schedule(channelSwitchDelay + TimeStep(1), [=, this]() {
                // sanity check that the channel switch delay was computed correctly
                auto auxPhy = m_staMacs[0]->GetWifiPhy(linkId);
                auto fem = m_staMacs[0]->GetFrameExchangeManager(linkId);
                switch (m_csdIndex)
                {
                case BEFORE_PADDING_END:
                    NS_TEST_EXPECT_MSG_GT(
                        Simulator::Now(),
                        startTx + txDuration - m_paddingDelay.at(0),
                        m_testStr << ": Channel switch terminated before padding start");
                    [[fallthrough]];
                case BEFORE_MAC_PAYLOAD_END:
                    if (useMacHdrInfo)
                    {
                        NS_TEST_EXPECT_MSG_EQ(fem->GetReceivedMacHdr().has_value(),
                                              true,
                                              m_testStr << ": Channel switch terminated before "
                                                           "MAC header info is received");
                    }
                    [[fallthrough]];
                case BEFORE_MAC_HDR_END:
                    NS_TEST_EXPECT_MSG_EQ(fem->GetOngoingRxInfo().has_value(),
                                          true,
                                          m_testStr << ": Channel switch terminated before "
                                                       "receiving RXSTART indication");
                    break;
                case BEFORE_PHY_HDR_END:
                    NS_TEST_EXPECT_MSG_EQ(auxPhy->GetInfoIfRxingPhyHeader().has_value(),
                                          true,
                                          m_testStr << ": Expected to be receiving the PHY header");
                    break;
                case DURING_PREAMBLE_DETECTION:
                    NS_TEST_EXPECT_MSG_EQ(auxPhy->GetTimeToPreambleDetectionEnd().has_value(),
                                          true,
                                          m_testStr
                                              << ": Expected to be in preamble detection period");
                default:
                    NS_ABORT_MSG("Unexpected channel switch duration index");
                }

                // if the main PHY switched to the same link on which the ICF is being received,
                // connecting the main PHY to the link is postponed until the end of the ICF, hence
                // the main PHY is not operating on any link at this time;
                // if the main PHY switched to another link, it was connected to that link but
                // the UL TXOP did not start because, at the end of the NAV and CCA busy in the last
                // PIFS check, it was detected that a frame which could be an ICF was being received
                // on another link)
                NS_TEST_EXPECT_MSG_EQ(m_staMacs[0]->GetLinkForPhy(m_mainPhyId).has_value(),
                                      switchToOtherLink,
                                      m_testStr
                                          << ": Main PHY not expected to be connected to a link");

                if (switchToOtherLink)
                {
                    NS_TEST_EXPECT_MSG_EQ(
                        +m_staMacs[0]->GetLinkForPhy(m_mainPhyId).value(),
                        +m_linkIdForTid3,
                        m_testStr << ": Main PHY did not left the link on which TID 3 is mapped");
                }
            });

            // check what happens when the ICF ends
            Simulator::Schedule(txDuration + TimeStep(1), [=, this]() {
                // if the main PHY switched to the same link on which the ICF has been received,
                // it has now been connected to that link; if the main PHY switched to another
                // link and there was not enough time for the main PHY to start switching to the
                // link on which the ICF has been received at the start of the padding, the ICF
                // has been dropped and the main PHY stayed on the preferred link

                const auto id = m_staMacs[0]->GetLinkForPhy(m_mainPhyId);
                NS_TEST_EXPECT_MSG_EQ(id.has_value(),
                                      true,
                                      m_testStr << ": Main PHY expected to be connected to a link");
                NS_TEST_EXPECT_MSG_EQ(+id.value(),
                                      +linkId,
                                      m_testStr << ": Main PHY connected to an unexpected link");

                NS_TEST_ASSERT_MSG_EQ(m_switchTo.has_value(),
                                      true,
                                      m_testStr << ": Main PHY was not connected to a link");
                NS_TEST_EXPECT_MSG_EQ(+m_switchTo->linkId,
                                      +linkId,
                                      m_testStr
                                          << ": Main PHY was not connected to the expected link");
                NS_TEST_EXPECT_MSG_EQ(m_switchTo->time,
                                      Simulator::Now() - TimeStep(1),
                                      m_testStr << ": Main PHY was not connected at ICF TX end");
            });
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_CTS,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            const auto id = m_staMacs[0]->GetLinkForPhy(m_mainPhyId);
            NS_TEST_EXPECT_MSG_EQ(id.has_value(),
                                  true,
                                  m_testStr << ": Main PHY expected to be connected to a link");
            NS_TEST_EXPECT_MSG_EQ(+id.value(),
                                  +linkId,
                                  m_testStr
                                      << ": Main PHY expected to be connected to same link as ICF");
            Simulator::ScheduleNow([=, this]() {
                NS_TEST_EXPECT_MSG_EQ(m_staMacs[0]->GetDevice()->GetPhy(m_mainPhyId)->IsStateTx(),
                                      true,
                                      m_testStr << ": Main PHY expected to be transmitting");
            });

            const auto txDuration =
                WifiPhy::CalculateTxDuration(psdu,
                                             txVector,
                                             m_staMacs[0]->GetWifiPhy(linkId)->GetPhyBand());

            Simulator::ScheduleNow([=, this]() {
                CheckInDeviceInterference(m_testStr + ", CTS", linkId, txDuration);
            });
        });

    m_events.emplace_back(WIFI_MAC_QOSDATA);
    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            const auto txDuration =
                WifiPhy::CalculateTxDuration(psdu,
                                             txVector,
                                             m_staMacs[0]->GetWifiPhy(linkId)->GetPhyBand());

            Simulator::ScheduleNow([=, this]() {
                CheckInDeviceInterference(m_testStr + ", ACK", linkId, txDuration);
            });
            // check the KeepMainPhyAfterDlTxop attribute
            Simulator::Schedule(txDuration + TimeStep(1), [=, this]() {
                auto mainPhy = m_staMacs[0]->GetDevice()->GetPhy(m_mainPhyId);
                auto shouldSwitch = (!keepMainPhyAfterDlTxop || switchToOtherLink);
                NS_TEST_EXPECT_MSG_EQ(mainPhy->IsStateSwitching(),
                                      shouldSwitch,
                                      m_testStr << ": Main PHY should "
                                                << (shouldSwitch ? "" : "not")
                                                << " be switching back after DL TXOP end");
            });
        });

    // Uplink TXOP
    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            const auto txDuration =
                WifiPhy::CalculateTxDuration(psdu,
                                             txVector,
                                             m_staMacs[0]->GetWifiPhy(linkId)->GetPhyBand());

            Simulator::ScheduleNow([=, this]() {
                CheckInDeviceInterference(m_testStr + ", QoS Data", linkId, txDuration);
            });
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            const auto txDuration =
                WifiPhy::CalculateTxDuration(psdu,
                                             txVector,
                                             m_staMacs[0]->GetWifiPhy(linkId)->GetPhyBand());
            // check that main PHY switches back after UL TXOP
            Simulator::Schedule(txDuration + TimeStep(1), [=, this]() {
                auto mainPhy = m_staMacs[0]->GetDevice()->GetPhy(m_mainPhyId);
                NS_TEST_EXPECT_MSG_EQ(
                    mainPhy->IsStateSwitching(),
                    true,
                    m_testStr << ": Main PHY should be switching back after UL TXOP end");
            });
            // Continue with the next test scenario
            Simulator::Schedule(MilliSeconds(2), [=, this]() {
                NS_TEST_EXPECT_MSG_EQ(m_events.empty(), true, "Not all events took place");
                m_csdIndex = static_cast<ChannelSwitchEnd>(static_cast<uint8_t>(m_csdIndex) + 1);
                if (m_csdIndex == CSD_COUNT)
                {
                    ++m_testIndex;
                    m_csdIndex = BEFORE_PHY_HDR_END;
                }

                if (m_testIndex < 8)
                {
                    RunOne();
                }
            });
        });
}

EmlsrSwitchMainPhyBackTest::EmlsrSwitchMainPhyBackTest()
    : EmlsrOperationsTestBase("Check handling of the switch main PHY back timer")
{
    m_mainPhyId = 2;
    m_linksToEnableEmlsrOn = {0, 1, 2};
    m_nEmlsrStations = 1;
    m_nNonEmlsrStations = 0;

    // channel switch delay will be also set to 64 us
    m_paddingDelay = {MicroSeconds(64)};
    m_transitionDelay = {MicroSeconds(64)};
    m_establishBaDl = {0};
    m_establishBaUl = {0, 4};
    m_duration = Seconds(0.5);
}

void
EmlsrSwitchMainPhyBackTest::DoSetup()
{
    Config::SetDefault("ns3::WifiPhy::ChannelSwitchDelay", TimeValue(MicroSeconds(64)));
    Config::SetDefault("ns3::WifiPhy::NotifyMacHdrRxEnd", BooleanValue(true));
    Config::SetDefault("ns3::DefaultEmlsrManager::SwitchAuxPhy", BooleanValue(false));
    Config::SetDefault("ns3::EmlsrManager::AuxPhyTxCapable", BooleanValue(false));
    // Use only link 1 for DL and UL traffic
    std::string mapping =
        "0 " + std::to_string(m_linkIdForTid0) + "; 4 " + std::to_string(m_linkIdForTid4);
    Config::SetDefault("ns3::EhtConfiguration::TidToLinkMappingDl", StringValue(mapping));
    Config::SetDefault("ns3::EhtConfiguration::TidToLinkMappingUl", StringValue(mapping));
    Config::SetDefault("ns3::EmlsrManager::AuxPhyMaxModClass", StringValue("HT"));
    Config::SetDefault("ns3::AdvancedEmlsrManager::UseAuxPhyCca", BooleanValue(true));

    EmlsrOperationsTestBase::DoSetup();

    WifiMacHeader hdr(WIFI_MAC_QOSDATA);
    hdr.SetAddr1(Mac48Address::GetBroadcast());
    hdr.SetAddr2(m_apMac->GetFrameExchangeManager(m_linkIdForTid0)->GetAddress());
    hdr.SetAddr3(m_apMac->GetAddress());
    hdr.SetDsFrom();
    hdr.SetDsNotTo();
    hdr.SetQosTid(0);

    m_bcastFrame = Create<WifiMpdu>(Create<Packet>(1000), hdr);
}

void
EmlsrSwitchMainPhyBackTest::Transmit(Ptr<WifiMac> mac,
                                     uint8_t phyId,
                                     WifiConstPsduMap psduMap,
                                     WifiTxVector txVector,
                                     double txPowerW)
{
    EmlsrOperationsTestBase::Transmit(mac, phyId, psduMap, txVector, txPowerW);

    const auto psdu = psduMap.cbegin()->second;
    const auto& hdr = psdu->GetHeader(0);

    // nothing to do before setup is completed
    if (!m_setupDone)
    {
        return;
    }

    auto linkId = mac->GetLinkForPhy(phyId);
    NS_TEST_ASSERT_MSG_EQ(linkId.has_value(),
                          true,
                          "PHY " << +phyId << " is not operating on any link");

    if (!m_events.empty())
    {
        // check that the expected frame is being transmitted
        NS_TEST_EXPECT_MSG_EQ(m_events.front().hdrType,
                              hdr.GetType(),
                              "Unexpected MAC header type for frame #" << ++m_processedEvents);
        // perform actions/checks, if any
        if (m_events.front().func)
        {
            m_events.front().func(psdu, txVector, linkId.value());
        }

        m_events.pop_front();
    }
}

void
EmlsrSwitchMainPhyBackTest::StartTraffic()
{
    m_setupDone = true;
    RunOne();
}

void
EmlsrSwitchMainPhyBackTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(m_events.empty(), true, "Not all events took place");

    Simulator::Destroy();
}

void
EmlsrSwitchMainPhyBackTest::MainPhySwitchInfoCallback(std::size_t index,
                                                      const EmlsrMainPhySwitchTrace& info)
{
    EmlsrOperationsTestBase::MainPhySwitchInfoCallback(index, info);

    if (!m_setupDone)
    {
        return;
    }

    if (!m_dlPktDone && info.GetName() == "UlTxopAuxPhyNotTxCapable")
    {
        NS_LOG_INFO("Main PHY starts switching\n");
        const auto delay =
            static_cast<TestScenario>(m_testIndex) <= RXSTART_WHILE_SWITCH_INTERRUPT
                ? Time{0}
                : MicroSeconds(30); // greater than duration of PHY header of non-HT PPDU
        Simulator::Schedule(delay,
                            [=, this]() { m_apMac->GetQosTxop(AC_BE)->Queue(m_bcastFrame); });
        return;
    }

    // lambda to generate a frame with TID 4 and to handle the corresponding frames
    auto genTid4Frame = [=, this]() {
        m_dlPktDone = true;

        // in 5 microseconds, while still switching, generate a packet with TID 4, which causes a
        // channel access request on link 0; if switching can be interrupted, the main PHY starts
        // switching to link 0 as soon as channel access is gained on link 0
        Simulator::Schedule(MicroSeconds(5), [=, this]() {
            m_staMacs[0]->GetDevice()->GetNode()->AddApplication(
                GetApplication(UPLINK, 0, 1, 500, 4));
            // channel access can be obtained within a slot due to slot alignment
            Simulator::Schedule(
                m_apMac->GetWifiPhy(m_linkIdForTid4)->GetSlot() + TimeStep(1),
                [=, this]() {
                    auto advEmlsrMgr =
                        DynamicCast<AdvancedEmlsrManager>(m_staMacs[0]->GetEmlsrManager());

                    NS_TEST_EXPECT_MSG_EQ(advEmlsrMgr->m_mainPhySwitchInfo.disconnected,
                                          true,
                                          "Expected the main PHY to be switching");
                    NS_TEST_EXPECT_MSG_EQ(
                        +advEmlsrMgr->m_mainPhySwitchInfo.to,
                        +(advEmlsrMgr->m_interruptSwitching ? m_linkIdForTid4 : m_mainPhyId),
                        "Test index " << +m_testIndex << ": Main PHY is switching to wrong link");
                });
        });
        InsertEventsForQosTid4();
    };

    if (m_expectedMainPhySwitchBackTime == Simulator::Now() &&
        info.GetName() == "TxopNotGainedOnAuxPhyLink")
    {
        NS_LOG_INFO("Main PHY switches back\n");

        const auto& traceInfo = static_cast<const EmlsrSwitchMainPhyBackTrace&>(info);

        switch (static_cast<TestScenario>(m_testIndex))
        {
        case RXSTART_WHILE_SWITCH_NO_INTERRUPT:
        case RXSTART_WHILE_SWITCH_INTERRUPT:
            NS_TEST_EXPECT_MSG_EQ((traceInfo.elapsed.IsStrictlyPositive() &&
                                   traceInfo.elapsed < m_switchMainPhyBackDelay),
                                  true,
                                  "Unexpected value for the elapsed field");
            NS_TEST_EXPECT_MSG_EQ(traceInfo.earlySwitchReason.has_value(),
                                  true,
                                  "earlySwitchReason should hold a value");
            NS_TEST_EXPECT_MSG_EQ(traceInfo.earlySwitchReason.value(),
                                  WifiExpectedAccessReason::RX_END,
                                  "Unexpected earlySwitchReason value");
            NS_TEST_EXPECT_MSG_EQ(traceInfo.isSwitching, true, "Unexpected value for isSwitching");
            break;

        case RXSTART_AFTER_SWITCH_HT_PPDU:
            NS_TEST_EXPECT_MSG_EQ((traceInfo.elapsed.IsStrictlyPositive() &&
                                   traceInfo.elapsed < m_switchMainPhyBackDelay),
                                  true,
                                  "Unexpected value for the elapsed field");
            NS_TEST_EXPECT_MSG_EQ(traceInfo.earlySwitchReason.has_value(),
                                  true,
                                  "earlySwitchReason should hold a value");
            NS_TEST_EXPECT_MSG_EQ(traceInfo.earlySwitchReason.value(),
                                  WifiExpectedAccessReason::BUSY_END,
                                  "Unexpected earlySwitchReason value");
            NS_TEST_EXPECT_MSG_EQ(traceInfo.isSwitching, false, "Unexpected value for isSwitching");
            break;

        case NON_HT_PPDU_USE_MAC_HDR:
            NS_TEST_EXPECT_MSG_EQ((traceInfo.elapsed.IsStrictlyPositive() &&
                                   traceInfo.elapsed < m_switchMainPhyBackDelay),
                                  true,
                                  "Unexpected value for the elapsed field");
            NS_TEST_EXPECT_MSG_EQ(traceInfo.earlySwitchReason.has_value(),
                                  true,
                                  "earlySwitchReason should hold a value");
            NS_TEST_EXPECT_MSG_EQ(traceInfo.earlySwitchReason.value(),
                                  WifiExpectedAccessReason::RX_END,
                                  "Unexpected earlySwitchReason value");
            NS_TEST_EXPECT_MSG_EQ(traceInfo.isSwitching, false, "Unexpected value for isSwitching");
            break;

        case LONG_SWITCH_BACK_DELAY_DONT_USE_MAC_HDR:
            NS_TEST_EXPECT_MSG_EQ((traceInfo.elapsed.IsStrictlyPositive() &&
                                   traceInfo.elapsed >= m_switchMainPhyBackDelay),
                                  true,
                                  "Unexpected value for the elapsed field");
            NS_TEST_EXPECT_MSG_EQ(traceInfo.earlySwitchReason.has_value(),
                                  true,
                                  "earlySwitchReason should hold a value");
            NS_TEST_EXPECT_MSG_EQ(traceInfo.earlySwitchReason.value(),
                                  WifiExpectedAccessReason::BACKOFF_END,
                                  "Unexpected earlySwitchReason value");
            NS_TEST_EXPECT_MSG_EQ(traceInfo.isSwitching, false, "Unexpected value for isSwitching");
            break;

        case LONG_SWITCH_BACK_DELAY_USE_MAC_HDR:
            NS_TEST_EXPECT_MSG_EQ((traceInfo.elapsed.IsStrictlyPositive() &&
                                   traceInfo.elapsed < m_switchMainPhyBackDelay),
                                  true,
                                  "Unexpected value for the elapsed field");
            NS_TEST_EXPECT_MSG_EQ(traceInfo.earlySwitchReason.has_value(),
                                  true,
                                  "earlySwitchReason should hold a value");
            NS_TEST_EXPECT_MSG_EQ(traceInfo.earlySwitchReason.value(),
                                  WifiExpectedAccessReason::BACKOFF_END,
                                  "Unexpected earlySwitchReason value");
            NS_TEST_EXPECT_MSG_EQ(traceInfo.isSwitching, false, "Unexpected value for isSwitching");
            break;

        default:
            NS_TEST_ASSERT_MSG_EQ(true, false, "Unexpected scenario: " << +m_testIndex);
        }

        genTid4Frame();
    }

    if (m_expectedMainPhySwitchBackTime == Simulator::Now() && info.GetName() == "TxopEnded")
    {
        NS_LOG_INFO("Main PHY switches back\n");

        NS_TEST_EXPECT_MSG_EQ(+m_testIndex,
                              +static_cast<uint8_t>(NON_HT_PPDU_DONT_USE_MAC_HDR),
                              "Unexpected TxopEnded reason for switching main PHY back");

        genTid4Frame();
    }
}

void
EmlsrSwitchMainPhyBackTest::InsertEventsForQosTid4()
{
    const auto testIndex = static_cast<TestScenario>(m_testIndex);
    std::list<Events> events;

    events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId,
                                  +m_linkIdForTid4,
                                  "Unicast frame with TID 4 transmitted on wrong link");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr1(),
                                  m_apMac->GetFrameExchangeManager(linkId)->GetAddress(),
                                  "Unexpected RA for the unicast frame with TID 4");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_staMacs[0]->GetFrameExchangeManager(linkId)->GetAddress(),
                                  "Unexpected TA for the unicast frame with TID 4");
            NS_TEST_EXPECT_MSG_EQ(+(*psdu->GetTids().cbegin()),
                                  4,
                                  "Expected a unicast frame with TID 4");
            // if switching can be interrupted, the frame with TID 4 is transmitted as soon as
            // the main PHY completes the switching to link 0
            if (auto advEmlsrMgr =
                    DynamicCast<AdvancedEmlsrManager>(m_staMacs[0]->GetEmlsrManager());
                advEmlsrMgr->m_interruptSwitching)
            {
                auto mainPhy = m_staMacs[0]->GetDevice()->GetPhy(m_mainPhyId);
                NS_TEST_EXPECT_MSG_EQ(advEmlsrMgr->m_mainPhySwitchInfo.start +
                                          mainPhy->GetChannelSwitchDelay(),
                                      Simulator::Now(),
                                      "Expected TX to start at main PHY switch end");
            }
        });

    events.emplace_back(WIFI_MAC_CTL_ACK);

    events.emplace_back(
        WIFI_MAC_CTL_END,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            if (testIndex == NON_HT_PPDU_DONT_USE_MAC_HDR)
            {
                Simulator::Schedule(MilliSeconds(2), [=, this]() {
                    // check that trace infos have been received
                    NS_TEST_EXPECT_MSG_EQ(m_dlPktDone,
                                          true,
                                          "Did not receive the expected trace infos");
                    NS_TEST_EXPECT_MSG_EQ(m_events.empty(), true, "Not all events took place");

                    if (++m_testIndex < static_cast<uint8_t>(COUNT))
                    {
                        RunOne();
                    }
                });
            }
        });

    // In the NON_HT_PPDU_DONT_USE_MAC_HDR scenario, the main PHY does not switch back to the
    // preferred link after the transmission of the broadcast frame, so the QoS data frame with
    // TID 0 is transmitted (on link 1) before the QoS data frame with TID 4 (on link 0)
    const auto pos =
        (testIndex == NON_HT_PPDU_DONT_USE_MAC_HDR ? m_events.cend() : m_events.cbegin());
    m_events.splice(pos, events);
}

void
EmlsrSwitchMainPhyBackTest::RunOne()
{
    const auto testIndex = static_cast<TestScenario>(m_testIndex);

    const auto bcastTxVector =
        m_apMac->GetWifiRemoteStationManager(m_linkIdForTid0)
            ->GetGroupcastTxVector(m_bcastFrame->GetHeader(),
                                   m_apMac->GetWifiPhy(m_linkIdForTid0)->GetChannelWidth());
    const auto bcastTxDuration =
        WifiPhy::CalculateTxDuration(m_bcastFrame->GetSize(),
                                     bcastTxVector,
                                     m_apMac->GetWifiPhy(m_linkIdForTid0)->GetPhyBand());

    const auto mode = (testIndex >= NON_HT_PPDU_DONT_USE_MAC_HDR ? OfdmPhy::GetOfdmRate6Mbps()
                                                                 : HtPhy::GetHtMcs0());

    m_switchMainPhyBackDelay = bcastTxDuration;
    if (testIndex != LONG_SWITCH_BACK_DELAY_DONT_USE_MAC_HDR &&
        testIndex != LONG_SWITCH_BACK_DELAY_USE_MAC_HDR)
    {
        // make switch main PHY back delay at least two channel switch delays shorter than the
        // PPDU TX duration
        m_switchMainPhyBackDelay -= MicroSeconds(250);
    }

    const auto interruptSwitch =
        (testIndex == RXSTART_WHILE_SWITCH_INTERRUPT || testIndex == NON_HT_PPDU_DONT_USE_MAC_HDR ||
         testIndex == LONG_SWITCH_BACK_DELAY_USE_MAC_HDR);
    const auto useMacHeader =
        (testIndex == NON_HT_PPDU_USE_MAC_HDR || testIndex == LONG_SWITCH_BACK_DELAY_USE_MAC_HDR);

    m_apMac->GetWifiRemoteStationManager(m_linkIdForTid0)
        ->SetAttribute("NonUnicastMode", WifiModeValue(mode));
    m_staMacs[0]->GetEmlsrManager()->SetAttribute("SwitchMainPhyBackDelay",
                                                  TimeValue(m_switchMainPhyBackDelay));
    m_staMacs[0]->GetEmlsrManager()->SetAttribute("InterruptSwitch", BooleanValue(interruptSwitch));
    m_staMacs[0]->GetEmlsrManager()->SetAttribute("UseNotifiedMacHdr", BooleanValue(useMacHeader));
    m_staMacs[0]->GetEmlsrManager()->SetAttribute("CheckAccessOnMainPhyLink", BooleanValue(false));
    // no in-device interference, just to avoid starting MSD timer causing RTS-CTS exchanges
    m_staMacs[0]->GetEmlsrManager()->SetAttribute("InDeviceInterference", BooleanValue(false));

    NS_LOG_INFO("Starting test #" << +m_testIndex << "\n");
    m_dlPktDone = false;

    // wait some more time to ensure that backoffs count down to zero and then generate a packet
    // at the EMLSR client. When notified of the main PHY switch, we decide when the AP MLD has to
    // transmit a broadcast frame
    Simulator::Schedule(MilliSeconds(5), [=, this]() {
        m_staMacs[0]->GetDevice()->GetNode()->AddApplication(GetApplication(UPLINK, 0, 1, 500));
    });

    auto mainPhy = m_staMacs[0]->GetDevice()->GetPhy(m_mainPhyId);
    auto advEmlsrMgr = DynamicCast<AdvancedEmlsrManager>(m_staMacs[0]->GetEmlsrManager());

    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            const auto phyHdrDuration = WifiPhy::CalculatePhyPreambleAndHeaderDuration(txVector);
            const auto txDuration =
                WifiPhy::CalculateTxDuration(psdu,
                                             txVector,
                                             m_apMac->GetWifiPhy(linkId)->GetPhyBand());
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr1(),
                                  Mac48Address::GetBroadcast(),
                                  "Expected a broadcast frame");
            NS_TEST_EXPECT_MSG_EQ(+linkId,
                                  +m_linkIdForTid0,
                                  "Broadcast frame transmitted on wrong link");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_apMac->GetFrameExchangeManager(linkId)->GetAddress(),
                                  "Unexpected TA for the broadcast frame");
            NS_TEST_EXPECT_MSG_EQ(txVector.GetMode(), mode, "Unexpected WifiMode");

            switch (testIndex)
            {
            case RXSTART_WHILE_SWITCH_NO_INTERRUPT:
                // main PHY is switching before the end of PHY header reception and
                // the switch main PHY back timer is running
                Simulator::Schedule(phyHdrDuration - TimeStep(1), [=, this]() {
                    NS_TEST_EXPECT_MSG_EQ(
                        mainPhy->GetState()->GetLastTime({WifiPhyState::SWITCHING}),
                        Simulator::Now(),
                        "Main PHY is not switching at the end of PHY header reception");
                    NS_TEST_EXPECT_MSG_EQ(+advEmlsrMgr->m_mainPhySwitchInfo.to,
                                          +m_linkIdForTid0,
                                          "Main PHY is switching to a wrong link");
                    NS_TEST_EXPECT_MSG_EQ(advEmlsrMgr->m_switchMainPhyBackEvent.IsPending(),
                                          true,
                                          "Main PHY switch back timer should be running");
                });
                // main PHY is still switching right after the end of PHY header reception, but
                // the switch main PHY back timer has been stopped
                Simulator::Schedule(phyHdrDuration + TimeStep(2), [=, this]() {
                    NS_TEST_EXPECT_MSG_EQ(
                        mainPhy->GetState()->GetLastTime({WifiPhyState::SWITCHING}),
                        Simulator::Now(),
                        "Main PHY is not switching at the end of PHY header reception");
                    NS_TEST_EXPECT_MSG_EQ(+advEmlsrMgr->m_mainPhySwitchInfo.to,
                                          +m_linkIdForTid0,
                                          "Main PHY is switching to a wrong link");
                    NS_TEST_EXPECT_MSG_EQ(advEmlsrMgr->m_switchMainPhyBackEvent.IsPending(),
                                          false,
                                          "Main PHY switch back timer should have been stopped");
                });
                // main PHY is expected to switch back when the ongoing switch terminates
                m_expectedMainPhySwitchBackTime = Simulator::Now() + mainPhy->GetDelayUntilIdle();
                break;

            case RXSTART_WHILE_SWITCH_INTERRUPT:
                // main PHY is switching before the end of PHY header reception and
                // the switch main PHY back timer is running
                Simulator::Schedule(phyHdrDuration - TimeStep(1), [=, this]() {
                    NS_TEST_EXPECT_MSG_EQ(
                        mainPhy->GetState()->GetLastTime({WifiPhyState::SWITCHING}),
                        Simulator::Now(),
                        "Main PHY is not switching at the end of PHY header reception");
                    NS_TEST_EXPECT_MSG_EQ(+advEmlsrMgr->m_mainPhySwitchInfo.to,
                                          +m_linkIdForTid0,
                                          "Main PHY is switching to a wrong link");
                    NS_TEST_EXPECT_MSG_EQ(advEmlsrMgr->m_switchMainPhyBackEvent.IsPending(),
                                          true,
                                          "Main PHY switch back timer should be running");
                });
                // main PHY is switching back right after the end of PHY header reception, but
                // the switch main PHY back timer has been stopped
                Simulator::Schedule(phyHdrDuration + TimeStep(2), [=, this]() {
                    NS_TEST_EXPECT_MSG_EQ(
                        mainPhy->GetState()->GetLastTime({WifiPhyState::SWITCHING}),
                        Simulator::Now(),
                        "Main PHY is not switching at the end of PHY header reception");
                    NS_TEST_EXPECT_MSG_EQ(+advEmlsrMgr->m_mainPhySwitchInfo.to,
                                          +m_mainPhyId,
                                          "Main PHY is switching to a wrong link");
                    NS_TEST_EXPECT_MSG_EQ(advEmlsrMgr->m_switchMainPhyBackEvent.IsPending(),
                                          false,
                                          "Main PHY switch back timer should have been stopped");
                });
                // main PHY is expected to switch back when the reception of PHY header ends
                m_expectedMainPhySwitchBackTime = Simulator::Now() + phyHdrDuration + TimeStep(1);
                break;

            case RXSTART_AFTER_SWITCH_HT_PPDU:
                // main PHY is switching back at the end of PHY header reception and
                // the switch main PHY back timer has been stopped
                Simulator::Schedule(phyHdrDuration, [=, this]() {
                    NS_TEST_EXPECT_MSG_EQ(
                        mainPhy->GetState()->GetLastTime({WifiPhyState::SWITCHING}),
                        Simulator::Now(),
                        "Main PHY is not switching at the end of PHY header reception");
                    NS_TEST_EXPECT_MSG_EQ(+advEmlsrMgr->m_mainPhySwitchInfo.to,
                                          +m_mainPhyId,
                                          "Main PHY is switching to a wrong link");
                    NS_TEST_EXPECT_MSG_EQ(advEmlsrMgr->m_switchMainPhyBackEvent.IsPending(),
                                          false,
                                          "Main PHY switch back timer should have been stopped");
                });
                // main PHY is expected to switch back when the reception of PHY header ends
                m_expectedMainPhySwitchBackTime =
                    Simulator::Now() + mainPhy->GetDelayUntilIdle() + TimeStep(1);
                break;

            case NON_HT_PPDU_DONT_USE_MAC_HDR:
                // when the main PHY completes the channel switch, it is not connected to the aux
                // PHY link and the switch main PHY back timer is running
                Simulator::Schedule(mainPhy->GetDelayUntilIdle() + TimeStep(1), [=, this]() {
                    NS_TEST_EXPECT_MSG_EQ(advEmlsrMgr->m_mainPhySwitchInfo.disconnected,
                                          true,
                                          "Main PHY should be waiting to be connected to a link");
                    NS_TEST_EXPECT_MSG_EQ(+advEmlsrMgr->m_mainPhySwitchInfo.to,
                                          +m_linkIdForTid0,
                                          "Main PHY is waiting to be connected to a wrong link");
                    NS_TEST_EXPECT_MSG_EQ(advEmlsrMgr->m_switchMainPhyBackEvent.IsPending(),
                                          true,
                                          "Main PHY switch back timer should be running");
                    // when PIFS check is performed at the end of the main PHY switch, the medium
                    // is found busy and a backoff value is generated; make sure that this value is
                    // at most 2 to ensure the conditions expected by this scenario
                    if (auto beTxop = m_staMacs[0]->GetQosTxop(AC_BE);
                        beTxop->GetBackoffSlots(m_linkIdForTid0) > 2)
                    {
                        beTxop->StartBackoffNow(2, m_linkIdForTid0);
                        m_staMacs[0]
                            ->GetChannelAccessManager(m_linkIdForTid0)
                            ->NotifyAckTimeoutResetNow(); // force restart access timeout
                    }
                });
                // once the PPDU is received, the main PHY is connected to the aux PHY and the
                // switch main PHY back timer is still running
                Simulator::Schedule(txDuration + TimeStep(1), [=, this]() {
                    NS_TEST_EXPECT_MSG_EQ(
                        mainPhy->GetState()->IsStateSwitching(),
                        false,
                        "Main PHY should not be switching at the end of PPDU reception");
                    NS_TEST_EXPECT_MSG_EQ(advEmlsrMgr->m_mainPhySwitchInfo.disconnected,
                                          false,
                                          "Main PHY should have been connected to a link");
                    NS_TEST_ASSERT_MSG_EQ(m_staMacs[0]->GetLinkForPhy(m_mainPhyId).has_value(),
                                          true,
                                          "Main PHY should have been connected to a link");
                    NS_TEST_EXPECT_MSG_EQ(+m_staMacs[0]->GetLinkForPhy(m_mainPhyId).value(),
                                          +m_linkIdForTid0,
                                          "Main PHY is connected to a wrong link");
                    NS_TEST_EXPECT_MSG_EQ(advEmlsrMgr->m_switchMainPhyBackEvent.IsPending(),
                                          true,
                                          "Main PHY switch back timer should be running");
                });
                break;

            case NON_HT_PPDU_USE_MAC_HDR:
            case LONG_SWITCH_BACK_DELAY_USE_MAC_HDR:
                // when the main PHY completes the channel switch, it is not connected to the aux
                // PHY link and the switch main PHY back timer is running. The aux PHY is in RX
                // state and has MAC header info available
                Simulator::Schedule(mainPhy->GetDelayUntilIdle() + TimeStep(1), [=, this]() {
                    NS_TEST_EXPECT_MSG_EQ(advEmlsrMgr->m_mainPhySwitchInfo.disconnected,
                                          true,
                                          "Main PHY should be waiting to be connected to a link");
                    NS_TEST_EXPECT_MSG_EQ(+advEmlsrMgr->m_mainPhySwitchInfo.to,
                                          +m_linkIdForTid0,
                                          "Main PHY is waiting to be connected to a wrong link");
                    NS_TEST_EXPECT_MSG_EQ(advEmlsrMgr->m_switchMainPhyBackEvent.IsPending(),
                                          true,
                                          "Main PHY switch back timer should be running");
                    const auto auxPhy = m_staMacs[0]->GetDevice()->GetPhy(m_linkIdForTid0);
                    NS_TEST_EXPECT_MSG_EQ(auxPhy->IsStateRx(),
                                          true,
                                          "Aux PHY should be in RX state");
                    auto remTime = auxPhy->GetTimeToMacHdrEnd(SU_STA_ID);
                    NS_TEST_ASSERT_MSG_EQ(remTime.has_value(),
                                          true,
                                          "No MAC header info available");
                    if (testIndex == LONG_SWITCH_BACK_DELAY_USE_MAC_HDR)
                    {
                        // when PIFS check is performed at the end of the main PHY switch, the
                        // medium is found busy and a backoff value is generated; make sure that
                        // this value is at least 7 to ensure that the backoff timer is still
                        // running when the switch main PHY back timer is expected to expire
                        if (auto beTxop = m_staMacs[0]->GetQosTxop(AC_BE);
                            beTxop->GetBackoffSlots(m_linkIdForTid0) <= 6)
                        {
                            beTxop->StartBackoffNow(7, m_linkIdForTid0);
                        }
                    }
                    // main PHY is expected to switch back when the MAC header is received
                    m_expectedMainPhySwitchBackTime = Simulator::Now() + remTime.value();
                    // once the MAC header is received, the main PHY switches back and the
                    // switch main PHY back timer is stopped
                    Simulator::Schedule(remTime.value() + TimeStep(1), [=, this]() {
                        NS_TEST_EXPECT_MSG_EQ(
                            mainPhy->GetState()->IsStateSwitching(),
                            true,
                            "Main PHY should be switching after receiving the MAC header");
                        NS_TEST_EXPECT_MSG_EQ(+advEmlsrMgr->m_mainPhySwitchInfo.to,
                                              +m_mainPhyId,
                                              "Main PHY should be switching to the preferred link");
                        NS_TEST_EXPECT_MSG_EQ(advEmlsrMgr->m_switchMainPhyBackEvent.IsPending(),
                                              false,
                                              "Main PHY switch back timer should not be running");
                    });
                });
                break;

            case LONG_SWITCH_BACK_DELAY_DONT_USE_MAC_HDR:
                // when the main PHY completes the channel switch, it is not connected to the aux
                // PHY link and the switch main PHY back timer is running
                Simulator::Schedule(mainPhy->GetDelayUntilIdle() + TimeStep(1), [=, this]() {
                    NS_TEST_EXPECT_MSG_EQ(advEmlsrMgr->m_mainPhySwitchInfo.disconnected,
                                          true,
                                          "Main PHY should be waiting to be connected to a link");
                    NS_TEST_EXPECT_MSG_EQ(+advEmlsrMgr->m_mainPhySwitchInfo.to,
                                          +m_linkIdForTid0,
                                          "Main PHY is waiting to be connected to a wrong link");
                    NS_TEST_EXPECT_MSG_EQ(advEmlsrMgr->m_switchMainPhyBackEvent.IsPending(),
                                          true,
                                          "Main PHY switch back timer should be running");
                    // when PIFS check is performed at the end of the main PHY switch, the medium
                    // is found busy and a backoff value is generated; make sure that this value is
                    // at least 7 to ensure that the backoff timer is still running when the switch
                    // main PHY back timer is expected to expire
                    if (auto beTxop = m_staMacs[0]->GetQosTxop(AC_BE);
                        beTxop->GetBackoffSlots(m_linkIdForTid0) <= 6)
                    {
                        beTxop->StartBackoffNow(7, m_linkIdForTid0);
                    }
                });
                // once the PPDU is received, the switch main PHY back timer is stopped and the
                // main PHY switches back to the preferred link
                Simulator::Schedule(txDuration + TimeStep(2), [=, this]() {
                    NS_TEST_EXPECT_MSG_EQ(
                        mainPhy->GetState()->IsStateSwitching(),
                        true,
                        "Main PHY should be switching at the end of PPDU reception");
                    NS_TEST_EXPECT_MSG_EQ(+advEmlsrMgr->m_mainPhySwitchInfo.to,
                                          +m_mainPhyId,
                                          "Main PHY should be switching back to preferred link");
                    NS_TEST_EXPECT_MSG_EQ(advEmlsrMgr->m_switchMainPhyBackEvent.IsPending(),
                                          false,
                                          "Main PHY switch back timer should be not running");
                });
                // main PHY is expected to switch back when the reception of PPDU ends
                m_expectedMainPhySwitchBackTime = Simulator::Now() + txDuration + TimeStep(1);
                break;

            default:
                NS_TEST_ASSERT_MSG_EQ(true, false, "Unexpected scenario: " << +m_testIndex);
            }
        });

    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId,
                                  +m_linkIdForTid0,
                                  "Unicast frame transmitted on wrong link");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr1(),
                                  m_apMac->GetFrameExchangeManager(linkId)->GetAddress(),
                                  "Unexpected RA for the unicast frame");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_staMacs[0]->GetFrameExchangeManager(linkId)->GetAddress(),
                                  "Unexpected TA for the unicast frame");

            if (testIndex == NON_HT_PPDU_DONT_USE_MAC_HDR)
            {
                Simulator::Schedule(TimeStep(1), [=, this]() {
                    // UL TXOP started, main PHY switch back time was cancelled
                    NS_TEST_EXPECT_MSG_EQ(advEmlsrMgr->m_switchMainPhyBackEvent.IsPending(),
                                          false,
                                          "Main PHY switch back timer should not be running");
                });
            }
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            const auto txDuration =
                WifiPhy::CalculateTxDuration(psdu,
                                             txVector,
                                             m_apMac->GetWifiPhy(linkId)->GetPhyBand());

            if (testIndex == NON_HT_PPDU_DONT_USE_MAC_HDR)
            {
                // main PHY is expected to switch back when the UL TXOP ends
                m_expectedMainPhySwitchBackTime = Simulator::Now() + txDuration;
            }
            else
            {
                Simulator::Schedule(MilliSeconds(2), [=, this]() {
                    // check that trace infos have been received
                    NS_TEST_EXPECT_MSG_EQ(m_dlPktDone,
                                          true,
                                          "Did not receive the expected trace infos");
                    NS_TEST_EXPECT_MSG_EQ(m_events.empty(), true, "Not all events took place");

                    if (++m_testIndex < static_cast<uint8_t>(COUNT))
                    {
                        RunOne();
                    }
                });
            }
        });
}

EmlsrCheckNavAndCcaLastPifsTest::EmlsrCheckNavAndCcaLastPifsTest()
    : EmlsrOperationsTestBase("Verify operations during the NAV and CCA check in the last PIFS")
{
    m_mainPhyId = 1;
    m_linksToEnableEmlsrOn = {0, 1, 2};
    m_nEmlsrStations = 1;
    m_nNonEmlsrStations = 0;

    m_paddingDelay = {MicroSeconds(64)};
    m_transitionDelay = {MicroSeconds(64)};
    m_establishBaDl = {};
    m_establishBaUl = {0};
    m_duration = Seconds(0.5);
}

void
EmlsrCheckNavAndCcaLastPifsTest::DoSetup()
{
    Config::SetDefault("ns3::EmlsrManager::AuxPhyTxCapable", BooleanValue(false));
    Config::SetDefault("ns3::DefaultEmlsrManager::SwitchAuxPhy", BooleanValue(false));
    Config::SetDefault("ns3::ChannelAccessManager::NSlotsLeft", UintegerValue(m_nSlotsLeft));
    // Use only one link for DL and UL traffic
    std::string mapping = "0 " + std::to_string(m_linkIdForTid0);
    Config::SetDefault("ns3::EhtConfiguration::TidToLinkMappingDl", StringValue(mapping));
    Config::SetDefault("ns3::EhtConfiguration::TidToLinkMappingUl", StringValue(mapping));
    Config::SetDefault("ns3::AdvancedEmlsrManager::UseAuxPhyCca", BooleanValue(true));
    Config::SetDefault("ns3::EmlsrManager::AuxPhyChannelWidth", UintegerValue(20));

    // use 40 MHz channels
    m_channelsStr = {"{3, 40, BAND_2_4GHZ, 0}"s,
                     "{38, 40, BAND_5GHZ, 0}"s,
                     "{3, 40, BAND_6GHZ, 0}"s};

    EmlsrOperationsTestBase::DoSetup();
}

void
EmlsrCheckNavAndCcaLastPifsTest::Transmit(Ptr<WifiMac> mac,
                                          uint8_t phyId,
                                          WifiConstPsduMap psduMap,
                                          WifiTxVector txVector,
                                          double txPowerW)
{
    EmlsrOperationsTestBase::Transmit(mac, phyId, psduMap, txVector, txPowerW);

    const auto psdu = psduMap.cbegin()->second;
    const auto& hdr = psdu->GetHeader(0);

    // nothing to do before setup is completed
    if (!m_setupDone)
    {
        return;
    }

    auto linkId = mac->GetLinkForPhy(phyId);
    NS_TEST_ASSERT_MSG_EQ(linkId.has_value(),
                          true,
                          "PHY " << +phyId << " is not operating on any link");

    if (!m_events.empty())
    {
        // check that the expected frame is being transmitted
        NS_TEST_EXPECT_MSG_EQ(m_events.front().hdrType,
                              hdr.GetType(),
                              "Unexpected MAC header type for frame #" << ++m_processedEvents);
        // perform actions/checks, if any
        if (m_events.front().func)
        {
            m_events.front().func(psdu, txVector, linkId.value());
        }

        m_events.pop_front();
    }
}

void
EmlsrCheckNavAndCcaLastPifsTest::StartTraffic()
{
    Simulator::Schedule(MilliSeconds(5), [=, this]() {
        m_setupDone = true;
        RunOne();
    });
}

void
EmlsrCheckNavAndCcaLastPifsTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(m_events.empty(), true, "Not all events took place");

    Simulator::Destroy();
}

void
EmlsrCheckNavAndCcaLastPifsTest::RunOne()
{
    const auto useAuxPhyCca = ((m_testIndex & 0x1) == 1);
    const auto scenario = static_cast<TestScenario>(m_testIndex >> 1);

    // aux PHY operating on the link on which TID 0 is mapped
    const auto auxPhy = m_staMacs[0]->GetDevice()->GetPhy(m_linkIdForTid0);
    const auto mainPhy = m_staMacs[0]->GetDevice()->GetPhy(m_mainPhyId);
    const auto slot = auxPhy->GetSlot();
    const auto pifs = auxPhy->GetSifs() + slot;
    const auto timeToBackoffEnd = slot * m_nSlotsLeft;
    NS_TEST_ASSERT_MSG_GT(timeToBackoffEnd, pifs + slot, "m_nSlotsLeft too small for this test");

    const auto switchDelay =
        (scenario == BACKOFF_END_BEFORE_SWITCH_END
             ? timeToBackoffEnd + slot
             : (scenario == LESS_THAN_PIFS_UNTIL_BACKOFF_END ? timeToBackoffEnd - pifs + slot
                                                             : timeToBackoffEnd - pifs - slot));

    m_staMacs[0]->GetEmlsrManager()->SetAttribute("UseAuxPhyCca", BooleanValue(useAuxPhyCca));
    mainPhy->SetAttribute("ChannelSwitchDelay", TimeValue(switchDelay));

    NS_LOG_INFO("Starting test #" << m_testIndex << "\n");

    // the AP sends a broadcast frame on the link on which TID 0 is mapped
    WifiMacHeader hdr(WIFI_MAC_QOSDATA);
    hdr.SetAddr1(Mac48Address::GetBroadcast());
    hdr.SetAddr2(m_apMac->GetFrameExchangeManager(m_linkIdForTid0)->GetAddress());
    hdr.SetAddr3(m_apMac->GetAddress());
    hdr.SetDsFrom();
    hdr.SetDsNotTo();
    hdr.SetQosTid(0);

    m_apMac->GetQosTxop(AC_BE)->Queue(Create<WifiMpdu>(Create<Packet>(1000), hdr));

    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr1(),
                                  Mac48Address::GetBroadcast(),
                                  "Expected a broadcast frame");
            NS_TEST_EXPECT_MSG_EQ(+linkId,
                                  +m_linkIdForTid0,
                                  "Broadcast frame transmitted on wrong link");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_apMac->GetFrameExchangeManager(linkId)->GetAddress(),
                                  "Unexpected TA for the broadcast frame");
            const auto txDuration =
                WifiPhy::CalculateTxDuration(psdu,
                                             txVector,
                                             m_apMac->GetWifiPhy(linkId)->GetPhyBand());
            const auto emlsrBeEdca = m_staMacs[0]->GetQosTxop(AC_BE);

            // generate a packet at the EMLSR client while the medium on the link on which TID 0
            // is mapped is still busy, so that a backoff value is generated. The backoff counter
            // is configured to be equal to the m_nSlotsLeft value
            Simulator::Schedule(txDuration - TimeStep(1), [=, this]() {
                emlsrBeEdca->StartBackoffNow(m_nSlotsLeft, m_linkIdForTid0);
                m_staMacs[0]->GetDevice()->GetNode()->AddApplication(
                    GetApplication(UPLINK, 0, 1, 500));
            });

            // given that the backoff counter equals m_nSlotsLeft, we expect that, an AIFS after the
            // end of the broadcast frame transmission, the NSlotsLeftAlert trace is fired and the
            // main PHY starts switching to the link on which TID 0 is mapped
            const auto aifs = auxPhy->GetSifs() + emlsrBeEdca->GetAifsn(m_linkIdForTid0) * slot;
            Simulator::Schedule(txDuration + aifs + TimeStep(1), [=, this]() {
                NS_TEST_EXPECT_MSG_EQ(mainPhy->IsStateSwitching(),
                                      true,
                                      "Expected the main PHY to be switching");
                NS_TEST_EXPECT_MSG_EQ(mainPhy->GetDelayUntilIdle(),
                                      switchDelay - TimeStep(1),
                                      "Unexpected end of main PHY channel switch");

                const auto now = Simulator::Now();
                switch (scenario)
                {
                case BACKOFF_END_BEFORE_SWITCH_END:
                    if (!useAuxPhyCca)
                    {
                        m_expectedTxStart = now + mainPhy->GetDelayUntilIdle() + pifs;
                        m_expectedWidth = m_mainPhyWidth;
                    }
                    else
                    {
                        m_expectedTxStart = now + mainPhy->GetDelayUntilIdle();
                        m_expectedWidth = m_auxPhyWidth;
                    }
                    break;
                case LESS_THAN_PIFS_UNTIL_BACKOFF_END:
                    if (!useAuxPhyCca)
                    {
                        m_expectedTxStart = now + mainPhy->GetDelayUntilIdle() + pifs;
                        m_expectedWidth = m_mainPhyWidth;
                    }
                    else
                    {
                        m_expectedTxStart = m_staMacs[0]
                                                ->GetChannelAccessManager(m_linkIdForTid0)
                                                ->GetBackoffEndFor(emlsrBeEdca);
                        m_expectedWidth = m_auxPhyWidth;
                    }
                    break;
                case MORE_THAN_PIFS_UNTIL_BACKOFF_END:
                    m_expectedTxStart = m_staMacs[0]
                                            ->GetChannelAccessManager(m_linkIdForTid0)
                                            ->GetBackoffEndFor(emlsrBeEdca);
                    m_expectedWidth = m_mainPhyWidth;
                    break;
                default:
                    NS_ABORT_MSG("Unexpected scenario " << +static_cast<uint8_t>(scenario));
                }
            });
        });

    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(+linkId,
                                  +m_linkIdForTid0,
                                  "Unicast frame transmitted on wrong link");
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  m_staMacs[0]->GetFrameExchangeManager(linkId)->GetAddress(),
                                  "Unexpected TA for the unicast frame");
            NS_TEST_EXPECT_MSG_EQ(m_expectedTxStart,
                                  Simulator::Now(),
                                  "Unexpected start TX time for unicast frame");
            NS_TEST_EXPECT_MSG_EQ(m_expectedWidth,
                                  txVector.GetChannelWidth(),
                                  "Unexpected channel width for the unicast frame");
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            Simulator::Schedule(MilliSeconds(2), [=, this]() {
                NS_TEST_EXPECT_MSG_EQ(m_events.empty(), true, "Not all events took place");

                if (++m_testIndex < static_cast<uint8_t>(COUNT) * 2)
                {
                    RunOne();
                }
            });
        });
}

WifiEmlsrLinkSwitchTestSuite::WifiEmlsrLinkSwitchTestSuite()
    : TestSuite("wifi-emlsr-link-switch", Type::UNIT)
{
    for (bool switchAuxPhy : {true, false})
    {
        for (bool resetCamStateAndInterruptSwitch : {true, false})
        {
            for (auto auxPhyMaxChWidth : {MHz_u{20}, MHz_u{40}, MHz_u{80}, MHz_u{160}})
            {
                AddTestCase(new EmlsrLinkSwitchTest(
                                {switchAuxPhy, resetCamStateAndInterruptSwitch, auxPhyMaxChWidth}),
                            TestCase::Duration::QUICK);
            }
        }
    }

    AddTestCase(new EmlsrCheckNavAndCcaLastPifsTest(), TestCase::Duration::QUICK);
    AddTestCase(new EmlsrIcfSentDuringMainPhySwitchTest(), TestCase::Duration::QUICK);
    AddTestCase(new EmlsrSwitchMainPhyBackTest(), TestCase::Duration::QUICK);

    AddTestCase(new EmlsrCcaBusyTest(MHz_u{20}), TestCase::Duration::QUICK);
    AddTestCase(new EmlsrCcaBusyTest(MHz_u{80}), TestCase::Duration::QUICK);

    for (const auto switchAuxPhy : {true, false})
    {
        for (const auto auxPhyTxCapable : {true, false})
        {
            AddTestCase(new SingleLinkEmlsrTest(switchAuxPhy, auxPhyTxCapable),
                        TestCase::Duration::QUICK);
        }
    }
}

static WifiEmlsrLinkSwitchTestSuite g_wifiEmlsrLinkSwitchTestSuite; ///< the test suite
