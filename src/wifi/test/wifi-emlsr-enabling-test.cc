/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-emlsr-enabling-test.h"

#include "ns3/advanced-emlsr-manager.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/ctrl-headers.h"
#include "ns3/log.h"
#include "ns3/mgt-action-headers.h"
#include "ns3/qos-txop.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"

#include <algorithm>
#include <functional>
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiEmlsrEnablingTest");

EmlOperatingModeNotificationTest::EmlOperatingModeNotificationTest()
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of the EML Operating Mode Notification frame")
{
}

void
EmlOperatingModeNotificationTest::DoRun()
{
    MgtEmlOmn frame;

    // Both EMLSR Mode and EMLMR Mode subfields set to 0 (no link bitmap);
    TestHeaderSerialization(frame);

    frame.m_emlControl.emlsrMode = 1;
    frame.SetLinkIdInBitmap(0);
    frame.SetLinkIdInBitmap(5);
    frame.SetLinkIdInBitmap(15);

    // Adding Link Bitmap
    TestHeaderSerialization(frame);

    NS_TEST_EXPECT_MSG_EQ((frame.GetLinkBitmap() == std::list<uint8_t>{0, 5, 15}),
                          true,
                          "Unexpected link bitmap");

    auto padding = MicroSeconds(64);
    auto transition = MicroSeconds(128);

    frame.m_emlControl.emlsrParamUpdateCtrl = 1;
    frame.m_emlsrParamUpdate = MgtEmlOmn::EmlsrParamUpdate{};
    frame.m_emlsrParamUpdate->paddingDelay = CommonInfoBasicMle::EncodeEmlsrPaddingDelay(padding);
    frame.m_emlsrParamUpdate->transitionDelay =
        CommonInfoBasicMle::EncodeEmlsrTransitionDelay(transition);

    // Adding the EMLSR Parameter Update field
    TestHeaderSerialization(frame);

    NS_TEST_EXPECT_MSG_EQ(
        CommonInfoBasicMle::DecodeEmlsrPaddingDelay(frame.m_emlsrParamUpdate->paddingDelay),
        padding,
        "Unexpected EMLSR Padding Delay");
    NS_TEST_EXPECT_MSG_EQ(
        CommonInfoBasicMle::DecodeEmlsrTransitionDelay(frame.m_emlsrParamUpdate->transitionDelay),
        transition,
        "Unexpected EMLSR Transition Delay");
}

EmlOmnExchangeTest::EmlOmnExchangeTest(const std::set<uint8_t>& linksToEnableEmlsrOn,
                                       Time transitionTimeout)
    : EmlsrOperationsTestBase("Check EML Notification exchange"),
      m_checkEmlsrLinksCount(0),
      m_emlNotificationDroppedCount(0)
{
    m_linksToEnableEmlsrOn = linksToEnableEmlsrOn;
    m_nEmlsrStations = 1;
    m_nNonEmlsrStations = 0;
    m_transitionTimeout = transitionTimeout;
    m_duration = Seconds(0.5);
}

void
EmlOmnExchangeTest::DoSetup()
{
    EmlsrOperationsTestBase::DoSetup();

    m_errorModel = CreateObject<ListErrorModel>();
    for (std::size_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
    {
        m_apMac->GetWifiPhy(linkId)->SetPostReceptionErrorModel(m_errorModel);
    }

    m_staMacs[0]->TraceConnectWithoutContext("AckedMpdu",
                                             MakeCallback(&EmlOmnExchangeTest::TxOk, this));
    m_staMacs[0]->TraceConnectWithoutContext("DroppedMpdu",
                                             MakeCallback(&EmlOmnExchangeTest::TxDropped, this));
}

void
EmlOmnExchangeTest::Transmit(Ptr<WifiMac> mac,
                             uint8_t phyId,
                             WifiConstPsduMap psduMap,
                             WifiTxVector txVector,
                             double txPowerW)
{
    EmlsrOperationsTestBase::Transmit(mac, phyId, psduMap, txVector, txPowerW);
    auto linkId = m_txPsdus.back().linkId;

    auto psdu = psduMap.begin()->second;

    switch (psdu->GetHeader(0).GetType())
    {
    case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
        NS_TEST_EXPECT_MSG_EQ(+linkId, +m_mainPhyId, "AssocReq not sent by the main PHY");
        CheckEmlCapabilitiesInAssocReq(*psdu->begin(), txVector, linkId);
        break;

    case WIFI_MAC_MGT_ASSOCIATION_RESPONSE:
        CheckEmlCapabilitiesInAssocResp(*psdu->begin(), txVector, linkId);
        break;

    case WIFI_MAC_MGT_ACTION:
        if (auto [category, action] = WifiActionHeader::Peek(psdu->GetPayload(0));
            category == WifiActionHeader::PROTECTED_EHT &&
            action.protectedEhtAction ==
                WifiActionHeader::PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION)
        {
            CheckEmlNotification(psdu, txVector, linkId);

            if (m_emlNotificationDroppedCount == 0 &&
                m_staMacs[0]->GetLinkIdByAddress(psdu->GetAddr2()) == linkId)
            {
                // transmitted by non-AP MLD, we need to corrupt it
                m_uidList.push_front(psdu->GetPacket()->GetUid());
                m_errorModel->SetList(m_uidList);
            }
            break;
        }

    default:;
    }
}

void
EmlOmnExchangeTest::CheckEmlCapabilitiesInAssocReq(Ptr<const WifiMpdu> mpdu,
                                                   const WifiTxVector& txVector,
                                                   uint8_t linkId)
{
    MgtAssocRequestHeader frame;
    mpdu->GetPacket()->PeekHeader(frame);

    const auto& mle = frame.Get<MultiLinkElement>();
    NS_TEST_ASSERT_MSG_EQ(mle.has_value(), true, "Multi-Link Element must be present in AssocReq");

    NS_TEST_ASSERT_MSG_EQ(mle->HasEmlCapabilities(),
                          true,
                          "Multi-Link Element in AssocReq must have EML Capabilities");
    NS_TEST_ASSERT_MSG_EQ(mle->IsEmlsrSupported(),
                          true,
                          "EML Support subfield of EML Capabilities in AssocReq must be set to 1");
    NS_TEST_ASSERT_MSG_EQ(mle->GetEmlsrPaddingDelay(),
                          m_paddingDelay.at(0),
                          "Unexpected Padding Delay in EML Capabilities included in AssocReq");
    NS_TEST_ASSERT_MSG_EQ(mle->GetEmlsrTransitionDelay(),
                          m_transitionDelay.at(0),
                          "Unexpected Transition Delay in EML Capabilities included in AssocReq");
}

void
EmlOmnExchangeTest::CheckEmlCapabilitiesInAssocResp(Ptr<const WifiMpdu> mpdu,
                                                    const WifiTxVector& txVector,
                                                    uint8_t linkId)
{
    bool sentToEmlsrClient =
        (m_staMacs[0]->GetLinkIdByAddress(mpdu->GetHeader().GetAddr1()) == linkId);

    if (!sentToEmlsrClient)
    {
        // nothing to check
        return;
    }

    MgtAssocResponseHeader frame;
    mpdu->GetPacket()->PeekHeader(frame);

    const auto& mle = frame.Get<MultiLinkElement>();
    NS_TEST_ASSERT_MSG_EQ(mle.has_value(), true, "Multi-Link Element must be present in AssocResp");

    NS_TEST_ASSERT_MSG_EQ(mle->HasEmlCapabilities(),
                          true,
                          "Multi-Link Element in AssocResp must have EML Capabilities");
    NS_TEST_ASSERT_MSG_EQ(mle->IsEmlsrSupported(),
                          true,
                          "EML Support subfield of EML Capabilities in AssocResp must be set to 1");
    NS_TEST_ASSERT_MSG_EQ(
        mle->GetTransitionTimeout(),
        m_transitionTimeout,
        "Unexpected Transition Timeout in EML Capabilities included in AssocResp");
}

void
EmlOmnExchangeTest::CheckEmlNotification(Ptr<const WifiPsdu> psdu,
                                         const WifiTxVector& txVector,
                                         uint8_t linkId)
{
    MgtEmlOmn frame;
    auto mpdu = *psdu->begin();
    auto pkt = mpdu->GetPacket()->Copy();
    WifiActionHeader::Remove(pkt);
    pkt->RemoveHeader(frame);
    NS_LOG_DEBUG(frame);

    bool sentbyNonApMld = m_staMacs[0]->GetLinkIdByAddress(mpdu->GetHeader().GetAddr2()) == linkId;

    NS_TEST_EXPECT_MSG_EQ(+frame.m_emlControl.emlsrMode,
                          1,
                          "EMLSR Mode subfield should be set to 1 (frame sent by non-AP MLD: "
                              << std::boolalpha << sentbyNonApMld << ")");

    NS_TEST_EXPECT_MSG_EQ(+frame.m_emlControl.emlmrMode,
                          0,
                          "EMLMR Mode subfield should be set to 0 (frame sent by non-AP MLD: "
                              << std::boolalpha << sentbyNonApMld << ")");

    NS_TEST_ASSERT_MSG_EQ(frame.m_emlControl.linkBitmap.has_value(),
                          true,
                          "Link Bitmap subfield should be present (frame sent by non-AP MLD: "
                              << std::boolalpha << sentbyNonApMld << ")");

    auto setupLinks = m_staMacs[0]->GetSetupLinkIds();
    std::list<uint8_t> expectedEmlsrLinks;
    std::set_intersection(setupLinks.begin(),
                          setupLinks.end(),
                          m_linksToEnableEmlsrOn.begin(),
                          m_linksToEnableEmlsrOn.end(),
                          std::back_inserter(expectedEmlsrLinks));

    NS_TEST_EXPECT_MSG_EQ((expectedEmlsrLinks == frame.GetLinkBitmap()),
                          true,
                          "Unexpected Link Bitmap subfield (frame sent by non-AP MLD: "
                              << std::boolalpha << sentbyNonApMld << ")");

    if (!sentbyNonApMld)
    {
        // the frame has been sent by the AP MLD
        NS_TEST_ASSERT_MSG_EQ(
            +frame.m_emlControl.emlsrParamUpdateCtrl,
            0,
            "EMLSR Parameter Update Control should be set to 0 in frames sent by the AP MLD");

        // as soon as the non-AP MLD receives this frame, it sets the EMLSR links
        auto delay = WifiPhy::CalculateTxDuration(psdu,
                                                  txVector,
                                                  m_staMacs[0]->GetWifiPhy(linkId)->GetPhyBand()) +
                     MicroSeconds(1); // to account for propagation delay
        Simulator::Schedule(delay, &EmlOmnExchangeTest::CheckEmlsrLinks, this);
    }

    NS_TEST_EXPECT_MSG_EQ(+m_mainPhyId,
                          +linkId,
                          "EML Notification received on unexpected link (frame sent by non-AP MLD: "
                              << std::boolalpha << sentbyNonApMld << ")");
}

void
EmlOmnExchangeTest::TxOk(Ptr<const WifiMpdu> mpdu)
{
    const auto& hdr = mpdu->GetHeader();

    if (hdr.IsMgt() && hdr.IsAction())
    {
        if (auto [category, action] = WifiActionHeader::Peek(mpdu->GetPacket());
            category == WifiActionHeader::PROTECTED_EHT &&
            action.protectedEhtAction ==
                WifiActionHeader::PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION)
        {
            // the EML Operating Mode Notification frame that the non-AP MLD sent has been
            // acknowledged; after the transition timeout, the EMLSR links have been set
            Simulator::Schedule(m_transitionTimeout + NanoSeconds(1),
                                &EmlOmnExchangeTest::CheckEmlsrLinks,
                                this);
        }
    }
}

void
EmlOmnExchangeTest::TxDropped(WifiMacDropReason reason, Ptr<const WifiMpdu> mpdu)
{
    const auto& hdr = mpdu->GetHeader();

    if (hdr.IsMgt() && hdr.IsAction())
    {
        if (auto [category, action] = WifiActionHeader::Peek(mpdu->GetPacket());
            category == WifiActionHeader::PROTECTED_EHT &&
            action.protectedEhtAction ==
                WifiActionHeader::PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION)
        {
            // the EML Operating Mode Notification frame has been dropped. Don't
            // corrupt it anymore
            m_emlNotificationDroppedCount++;
        }
    }
}

void
EmlOmnExchangeTest::CheckEmlsrLinks()
{
    m_checkEmlsrLinksCount++;

    auto setupLinks = m_staMacs[0]->GetSetupLinkIds();
    std::set<uint8_t> expectedEmlsrLinks;
    std::set_intersection(setupLinks.begin(),
                          setupLinks.end(),
                          m_linksToEnableEmlsrOn.begin(),
                          m_linksToEnableEmlsrOn.end(),
                          std::inserter(expectedEmlsrLinks, expectedEmlsrLinks.end()));

    NS_TEST_EXPECT_MSG_EQ((expectedEmlsrLinks == m_staMacs[0]->GetEmlsrManager()->GetEmlsrLinks()),
                          true,
                          "Unexpected set of EMLSR links)");
}

void
EmlOmnExchangeTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(m_checkEmlsrLinksCount,
                          2,
                          "Unexpected number of times CheckEmlsrLinks() is called");
    NS_TEST_EXPECT_MSG_EQ(
        m_emlNotificationDroppedCount,
        1,
        "Unexpected number of times the EML Notification frame is dropped due to max retry limit");

    Simulator::Destroy();
}

WifiEmlsrEnablingTestSuite::WifiEmlsrEnablingTestSuite()
    : TestSuite("wifi-emlsr-enabling", Type::UNIT)
{
    AddTestCase(new EmlOperatingModeNotificationTest(), TestCase::Duration::QUICK);
    AddTestCase(new EmlOmnExchangeTest({1, 2}, MicroSeconds(0)), TestCase::Duration::QUICK);
    AddTestCase(new EmlOmnExchangeTest({1, 2}, MicroSeconds(2048)), TestCase::Duration::QUICK);
    AddTestCase(new EmlOmnExchangeTest({0, 1, 2, 3}, MicroSeconds(0)), TestCase::Duration::QUICK);
    AddTestCase(new EmlOmnExchangeTest({0, 1, 2, 3}, MicroSeconds(2048)),
                TestCase::Duration::QUICK);
}

static WifiEmlsrEnablingTestSuite g_wifiEmlsrEnablingTestSuite; ///< the test suite
