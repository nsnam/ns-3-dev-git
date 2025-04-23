/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-association-test.h"

#include "ns3/config.h"
#include "ns3/ht-frame-exchange-manager.h"
#include "ns3/mobility-helper.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/wifi-mac-queue.h"

#include <algorithm>
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiAssocTest");

NS_OBJECT_ENSURE_REGISTERED(WifiTestAssocManager);

/***********************************
 *       WifiTestAssocManager
 ***********************************/

TypeId
WifiTestAssocManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WifiTestAssocManager")
                            .SetParent<WifiAssocManager>()
                            .AddConstructor<WifiTestAssocManager>()
                            .SetGroupName("Wifi");
    return tid;
}

void
WifiTestAssocManager::NotifyChannelSwitched(linkId_t linkId)
{
}

bool
WifiTestAssocManager::Compare(const StaWifiMac::ApInfo& lhs, const StaWifiMac::ApInfo& rhs) const
{
    return true;
}

void
WifiTestAssocManager::SetBestAp(Mac48Address bssid,
                                const std::list<StaWifiMac::ApInfo::SetupLinksInfo>& setupLinks)
{
    m_bestAp = bssid;
    m_setupLinks = setupLinks;
}

bool
WifiTestAssocManager::CanBeInserted(const StaWifiMac::ApInfo& apInfo) const
{
    return true;
}

bool
WifiTestAssocManager::CanBeReturned(const StaWifiMac::ApInfo& apInfo) const
{
    return (apInfo.m_bssid == m_bestAp);
}

void
WifiTestAssocManager::DoStartScanning()
{
    Simulator::Schedule(GetScanParams().maxChannelTime, [=, this]() mutable {
        for (const auto& apInfo : GetSortedList())
        {
            if (apInfo.m_bssid == m_bestAp)
            {
                // this is the AP to return
                GetSetupLinks(apInfo) = m_setupLinks;
            }
        }
        ScanningTimeout();
    });
}

/***********************************
 *     WifiAssociationTestBase
 ***********************************/

WifiAssociationTestBase::WifiAssociationTestBase(const std::string& name,
                                                 const BaseParams& baseParams)
    : TestCase(name + ", " + toString(baseParams)),
      m_baseParams(baseParams)
{
}

void
WifiAssociationTestBase::DoSetup()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 30;

    NodeContainer nodes(3); // AP 1, AP 2, STA
    NetDeviceContainer devices;

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211be);

    Ssid ssid("wifi-assoc-test");
    WifiMacHelper mac;
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));

    auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    spectrumChannel->AddPropagationLossModel(CreateObject<FriisPropagationLossModel>());

    // lambda to create and install wifi netdevices
    auto install = [&](std::string channels, Ptr<Node> node) {
        auto strings = SplitString(channels, ";");
        SpectrumWifiPhyHelper phy{static_cast<uint8_t>(strings.size())};
        for (std::size_t id = 0; id < strings.size(); ++id)
        {
            phy.Set(id, "ChannelSettings", StringValue(strings[id]));
        }
        phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        phy.SetChannel(spectrumChannel);
        devices.Add(wifi.Install(phy, mac, node));
    };

    install(m_baseParams.ap1Channels, nodes.Get(0));
    install(m_baseParams.ap2Channels, nodes.Get(1));

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    mac.SetAssocManager(m_assocMgrTypeId);

    install(m_baseParams.staChannels, nodes.Get(2));

    // Assign fixed streams to random variables in use
    streamNumber += WifiHelper::AssignStreams(devices, streamNumber);

    MobilityHelper mobility;
    auto positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));   // AP 1
    positionAlloc->Add(Vector(20.0, 0.0, 0.0));  // AP 2
    positionAlloc->Add(Vector(10.0, 10.0, 0.0)); // non-AP STA
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    m_ap1Mac = DynamicCast<ApWifiMac>(DynamicCast<WifiNetDevice>(devices.Get(0))->GetMac());
    m_ap2Mac = DynamicCast<ApWifiMac>(DynamicCast<WifiNetDevice>(devices.Get(1))->GetMac());
    m_staMac = DynamicCast<StaWifiMac>(DynamicCast<WifiNetDevice>(devices.Get(2))->GetMac());

    // Trace PSDUs passed to the PHY on all devices
    for (uint32_t i = 0; i < devices.GetN(); ++i)
    {
        auto dev = DynamicCast<WifiNetDevice>(devices.Get(i));
        for (uint8_t phyId = 0; phyId < dev->GetNPhys(); ++phyId)
        {
            dev->GetPhy(phyId)->TraceConnectWithoutContext(
                "PhyTxPsduBegin",
                MakeCallback(&WifiAssociationTestBase::Transmit, this).Bind(dev->GetMac(), phyId));
        }
    }

    // install packet socket on all nodes
    PacketSocketHelper packetSocket;
    packetSocket.Install(nodes);

    // install a packet socket server on all nodes
    for (uint32_t i = 0; i < nodes.GetN(); ++i)
    {
        PacketSocketAddress srvAddr;
        auto device = DynamicCast<WifiNetDevice>(nodes.Get(i)->GetDevice(0));
        NS_TEST_ASSERT_MSG_NE(device, nullptr, "Expected a WifiNetDevice");
        srvAddr.SetSingleDevice(device->GetIfIndex());
        srvAddr.SetProtocol(1);

        auto server = CreateObject<PacketSocketServer>();
        server->SetLocal(srvAddr);
        nodes.Get(i)->AddApplication(server);
        server->SetStartTime(Seconds(0)); // now
        server->SetStopTime(m_duration);
    }

    // set DL and UL packet socket addresses for both APs
    using enum WifiDirection;
    for (auto apMac : {m_ap1Mac, m_ap2Mac})
    {
        PacketSocketAddress dlAddress;
        dlAddress.SetSingleDevice(apMac->GetDevice()->GetIfIndex());
        dlAddress.SetPhysicalAddress(m_staMac->GetDevice()->GetAddress());
        dlAddress.SetProtocol(1);
        (apMac == m_ap1Mac ? m_ap1Sockets : m_ap2Sockets)[DOWNLINK] = dlAddress;

        PacketSocketAddress ulAddress;
        ulAddress.SetSingleDevice(m_staMac->GetDevice()->GetIfIndex());
        ulAddress.SetPhysicalAddress(apMac->GetDevice()->GetAddress());
        ulAddress.SetProtocol(1);
        (apMac == m_ap1Mac ? m_ap1Sockets : m_ap2Sockets)[UPLINK] = ulAddress;
    }

    // connect (dis)association callbacks
    m_staMac->TraceConnectWithoutContext(
        "Assoc",
        MakeCallback(&WifiAssociationTestBase::AssocSeenBySta, this));
    m_staMac->TraceConnectWithoutContext(
        "DeAssoc",
        MakeCallback(&WifiAssociationTestBase::DisassocSeenBySta, this));

    m_ap1Mac->TraceConnectWithoutContext(
        "AssociatedSta",
        MakeCallback(&WifiAssociationTestBase::AssocSeenByAp, this).Bind(m_ap1Mac));
    m_ap1Mac->TraceConnectWithoutContext(
        "DeAssociatedSta",
        MakeCallback(&WifiAssociationTestBase::DisassocSeenByAp, this).Bind(m_ap1Mac));

    m_ap2Mac->TraceConnectWithoutContext(
        "AssociatedSta",
        MakeCallback(&WifiAssociationTestBase::AssocSeenByAp, this).Bind(m_ap2Mac));
    m_ap2Mac->TraceConnectWithoutContext(
        "DeAssociatedSta",
        MakeCallback(&WifiAssociationTestBase::DisassocSeenByAp, this).Bind(m_ap2Mac));
}

void
WifiAssociationTestBase::Transmit(Ptr<WifiMac> mac,
                                  uint8_t phyId,
                                  WifiConstPsduMap psduMap,
                                  WifiTxVector txVector,
                                  double txPowerW)
{
    auto linkId = mac->GetLinkForPhy(phyId);
    NS_TEST_ASSERT_MSG_EQ(linkId.has_value(), true, "No link found for PHY ID " << +phyId);

    for (const auto& [aid, psdu] : psduMap)
    {
        std::stringstream ss;
        ss << std::setprecision(10) << " Link ID " << +linkId.value() << " Phy ID " << +phyId
           << " #MPDUs " << psdu->GetNMpdus();
        for (auto it = psdu->begin(); it != psdu->end(); ++it)
        {
            ss << "\n" << **it;
        }
        NS_LOG_INFO(ss.str());
    }
    NS_LOG_INFO("TXVECTOR = " << txVector << "\n");

    const auto psdu = psduMap.cbegin()->second;
    const auto& hdr = psdu->GetHeader(0);

    if (!m_checkTxPsdus || m_framesToSkip.contains(hdr.GetType()))
    {
        return;
    }

    if (!m_events.empty())
    {
        // check that the expected frame is being transmitted
        NS_TEST_EXPECT_MSG_EQ(WifiMacHeader(m_events.front().hdrType).GetTypeString(),
                              hdr.GetTypeString(),
                              "Unexpected MAC header type for frame transmitted at time "
                                  << Simulator::Now().As(Time::US));
        // perform actions/checks, if any
        if (m_events.front().func)
        {
            m_events.front().func(psdu, txVector, linkId.value());
        }

        m_events.pop_front();
    }
}

Ptr<PacketSocketClient>
WifiAssociationTestBase::GetApplication(WifiDirection dir,
                                        std::size_t apId,
                                        std::size_t count,
                                        std::size_t pktSize,
                                        uint8_t priority) const
{
    auto client = CreateObject<PacketSocketClient>();
    client->SetAttribute("PacketSize", UintegerValue(pktSize));
    client->SetAttribute("MaxPackets", UintegerValue(count));
    client->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
    client->SetAttribute("Priority", UintegerValue(priority));
    client->SetRemote(apId == 1 ? m_ap1Sockets.at(dir) : m_ap2Sockets.at(dir));
    client->SetStartTime(Time{0}); // now
    client->SetStopTime(m_duration - Simulator::Now());

    return client;
}

/***********************************
 *    WifiDisassocAndReassocTest
 ***********************************/

WifiDisassocAndReassocTest::WifiDisassocAndReassocTest(const BaseParams& baseParams,
                                                       const Params& params)
    : WifiAssociationTestBase("Disassociation and reassociation test, " + toString(params),
                              baseParams),
      m_params(params),
      m_staErrorModel(CreateObject<ListErrorModel>()),
      m_ap1ErrorModel(CreateObject<ListErrorModel>())
{
    m_assocMgrTypeId = "ns3::WifiTestAssocManager";
    m_framesToSkip = {WIFI_MAC_MGT_BEACON, WIFI_MAC_DATA_NULL, WIFI_MAC_CTL_END};
    NS_TEST_ASSERT_MSG_EQ((m_params.trafficDir == WifiDirection::DOWNLINK ||
                           m_params.trafficDir == WifiDirection::UPLINK),
                          true,
                          "Unexpected traffic direction: " << m_params.trafficDir);
}

void
WifiDisassocAndReassocTest::DoSetup()
{
    Config::SetDefault("ns3::StaWifiMac::AssocType", EnumValue(m_params.assocType));

    WifiAssociationTestBase::DoSetup();

    auto assocManager = DynamicCast<WifiTestAssocManager>(m_staMac->GetAssocManager());
    NS_TEST_ASSERT_MSG_NE(assocManager, nullptr, "Expected a test association manager");

    // set the AP to associate with first

    std::list<StaWifiMac::ApInfo::SetupLinksInfo> setupLinks;
    // the association manager provides setup links only in case of ML setup
    if (m_staMac->GetAssocType() == WifiAssocType::ML_SETUP)
    {
        for (const auto& [staLinkId, apLinkId] : m_params.firstSetupLinks)
        {
            setupLinks.push_back(
                {staLinkId, apLinkId, m_ap1Mac->GetFrameExchangeManager(apLinkId)->GetAddress()});
        }
    }
    assocManager->SetBestAp(m_ap1Mac->GetFrameExchangeManager(m_params.firstApLinkId)->GetAddress(),
                            setupLinks);

    // install error models
    for (std::size_t phyId = 0; phyId < m_staMac->GetNLinks(); ++phyId)
    {
        m_staMac->GetDevice()->GetPhy(phyId)->SetPostReceptionErrorModel(m_staErrorModel);
    }
    for (std::size_t phyId = 0; phyId < m_ap1Mac->GetNLinks(); ++phyId)
    {
        m_ap1Mac->GetDevice()->GetPhy(phyId)->SetPostReceptionErrorModel(m_ap1ErrorModel);
    }
}

void
WifiDisassocAndReassocTest::AssocSeenBySta(Mac48Address bssid)
{
    NS_LOG_FUNCTION(this << bssid);

    if (m_staMac->GetAssocType() == WifiAssocType::ML_SETUP
            ? (bssid == m_ap1Mac->GetAddress())
            : m_ap1Mac->GetLinkIdByAddress(bssid).has_value())
    {
        ++m_assocWithAp1Count;
    }
    else if (m_staMac->GetAssocType() == WifiAssocType::ML_SETUP
                 ? (bssid == m_ap2Mac->GetAddress())
                 : m_ap2Mac->GetLinkIdByAddress(bssid).has_value())
    {
        ++m_assocWithAp2Count;
    }
    else
    {
        NS_TEST_ASSERT_MSG_EQ(true, false, "Unexpected BSSID " << bssid);
    }

    Simulator::Schedule(TimeStep(1),
                        &WifiDisassocAndReassocTest::CheckRemoteApInfo,
                        this,
                        (m_assocWithAp1Count + m_assocWithAp2Count == 1));
}

void
WifiDisassocAndReassocTest::AssocSeenByAp(Ptr<ApWifiMac> apMac,
                                          uint16_t aid,
                                          Mac48Address staAddress)
{
    NS_LOG_FUNCTION(this << apMac->GetAddress() << aid << staAddress);

    if (Simulator::Now() == m_assocTime)
    {
        return; // notification of other links that have been setup
    }

    const auto assocCount = m_assocSeenByAp1Count + m_assocSeenByAp2Count;
    if (assocCount == 0)
    {
        // first association
        InsertEvents();
    }
    Simulator::Schedule(TimeStep(1),
                        &WifiDisassocAndReassocTest::CheckRemoteStaInfo,
                        this,
                        (assocCount == 0));

    if (apMac == m_ap1Mac)
    {
        ++m_assocSeenByAp1Count;
    }
    else if (apMac == m_ap2Mac)
    {
        ++m_assocSeenByAp2Count;
    }
    else
    {
        NS_TEST_ASSERT_MSG_EQ(true, false, "Unexpected AP " << apMac->GetAddress());
    }

    m_assocTime = Simulator::Now();
    m_checkTxPsdus = true; // check transmitted frames
}

void
WifiDisassocAndReassocTest::CheckRemoteStaInfo(bool firstAssoc)
{
    // check that the remote station manager(s) of the AP store the correct status for the STA
    const auto when = std::string(" after ") + (firstAssoc ? "first" : "second") + " association";
    const auto apId = (firstAssoc ? m_firstApId : m_params.secondApId);
    const auto& setupLinks = (firstAssoc ? m_params.firstSetupLinks : m_params.secondSetupLinks);
    auto apMac = (apId == m_firstApId ? m_ap1Mac : m_ap2Mac);
    const auto isMlSetup =
        (m_staMac->GetAssocType() == WifiAssocType::ML_SETUP) && (apMac->GetNLinks() > 1);
    for (linkId_t linkId = 0; linkId < apMac->GetNLinks(); ++linkId)
    {
        auto stationManager = apMac->GetWifiRemoteStationManager(linkId);

        if (std::any_of(setupLinks.cbegin(), setupLinks.cend(), [=](auto&& linkIds) {
                return linkIds.second == linkId;
            }))
        {
            auto staLinkId = linkId;
            if (!isMlSetup)
            {
                // find the STA link ID corresponding to the link setup by the AP
                auto it = std::find_if(setupLinks.cbegin(),
                                       setupLinks.cend(),
                                       [linkId](auto&& pair) { return pair.second == linkId; });
                NS_TEST_ASSERT_MSG_EQ((it != setupLinks.cend()),
                                      true,
                                      "AP link ID (" << +linkId
                                                     << ") not found in given setup links");
                staLinkId = it->first;
            }
            // this link must have been setup
            NS_TEST_EXPECT_MSG_EQ(stationManager->IsAssociated(
                                      m_staMac->GetFrameExchangeManager(staLinkId)->GetAddress()),
                                  true,
                                  "Expected STA to be associated with AP " << apId << " on link "
                                                                           << +linkId << when);
            const auto staAddr = stationManager->GetAffiliatedStaAddress(m_staMac->GetAddress());
            NS_TEST_EXPECT_MSG_EQ(staAddr.has_value(),
                                  isMlSetup,
                                  "Unexpected value returned by GetAffiliatedStaAddress" << when);
            if (staAddr.has_value())
            {
                NS_TEST_EXPECT_MSG_EQ(staAddr.value(),
                                      m_staMac->GetFrameExchangeManager(linkId)->GetAddress(),
                                      "Unexpected address returned by GetAffiliatedStaAddress"
                                          << when);
            }
        }
        else
        {
            // this link must have not been setup, thus every STA link address should not be
            // reported as associated by the remote station manager of the AP operating on this
            // link. The exception is the case in which the STA performs legacy association, does
            // not send a Disassociation frame and re-associates with the same AP on a distinct
            // link. In such a case, the AP cannot know that the STA requesting association is the
            // same as the STA previously associated on another link
            for (const auto staLinkId : m_staMac->GetLinkIds())
            {
                const auto isAssociated = !isMlSetup && !m_params.sendDisassoc && !firstAssoc &&
                                          (m_params.secondApId == m_firstApId) &&
                                          (staLinkId == m_params.firstSetupLinks.front().first) &&
                                          (linkId == m_params.firstSetupLinks.front().second);

                NS_TEST_EXPECT_MSG_EQ(
                    stationManager->IsAssociated(
                        m_staMac->GetFrameExchangeManager(staLinkId)->GetAddress()),
                    isAssociated,
                    "Expected STA " << (isAssociated ? "" : "not ") << "to be associated with AP "
                                    << apId << " on link " << +linkId << when);
            }
            const auto staAddr = stationManager->GetAffiliatedStaAddress(m_staMac->GetAddress());
            NS_TEST_EXPECT_MSG_EQ(staAddr.has_value(),
                                  false,
                                  "Unexpected value returned by GetAffiliatedStaAddress for link "
                                      << +linkId << when);
        }
    }
}

void
WifiDisassocAndReassocTest::CheckRemoteApInfo(bool firstAssoc)
{
    const auto when = std::string(" after ") + (firstAssoc ? "first" : "second") + " association";
    const auto apId = (firstAssoc ? m_firstApId : m_params.secondApId);
    const auto setupLinks = m_staMac->GetSetupLinkIds();
    auto apMac = (apId == 1 ? m_ap1Mac : m_ap2Mac);
    const auto isMlSetup =
        (m_staMac->GetAssocType() == WifiAssocType::ML_SETUP) && (apMac->GetNLinks() > 1);

    const auto& givenSetupLinks =
        (firstAssoc ? m_params.firstSetupLinks : m_params.secondSetupLinks);
    std::set<linkId_t> expectedSetupLinks;
    for (const auto& [staLinkId, apLinkId] : givenSetupLinks)
    {
        expectedSetupLinks.insert(isMlSetup ? apLinkId : staLinkId);
    }

    for (const auto linkId : m_staMac->GetLinkIds())
    {
        NS_TEST_EXPECT_MSG_EQ(setupLinks.contains(linkId),
                              expectedSetupLinks.contains(linkId),
                              "Unexpected setup link (" << +linkId << ")" << when);
    }

    for (const auto staLinkId : setupLinks)
    {
        auto apLinkId = staLinkId;
        if (!isMlSetup)
        {
            // find the AP link ID corresponding to the link setup by the STA
            auto it = std::find_if(givenSetupLinks.cbegin(),
                                   givenSetupLinks.cend(),
                                   [staLinkId](auto&& pair) { return pair.first == staLinkId; });
            NS_TEST_ASSERT_MSG_EQ((it != givenSetupLinks.cend()),
                                  true,
                                  "STA link ID (" << +staLinkId
                                                  << ") not found in given setup links");
            apLinkId = it->second;
        }
        NS_TEST_EXPECT_MSG_EQ(m_staMac->GetBssid(staLinkId),
                              apMac->GetFrameExchangeManager(apLinkId)->GetAddress(),
                              "Incorrect BSSID on link " << +staLinkId << when);
    }
}

void
WifiDisassocAndReassocTest::StartRoaming()
{
    NS_LOG_FUNCTION(this << "\n");

    auto assocManager = DynamicCast<WifiTestAssocManager>(m_staMac->GetAssocManager());
    NS_TEST_ASSERT_MSG_NE(assocManager, nullptr, "Expected a test association manager");

    // set the AP to associate with next

    const auto apMac = (m_params.secondApId == 1 ? m_ap1Mac : m_ap2Mac);
    std::list<StaWifiMac::ApInfo::SetupLinksInfo> setupLinks;
    // the association manager provides setup links only in case of ML setup
    if (m_staMac->GetAssocType() == WifiAssocType::ML_SETUP)
    {
        for (const auto& [staLinkId, apLinkId] : m_params.secondSetupLinks)
        {
            setupLinks.push_back(
                {staLinkId, apLinkId, apMac->GetFrameExchangeManager(apLinkId)->GetAddress()});
        }
    }
    assocManager->SetBestAp(apMac->GetFrameExchangeManager(m_params.secondApLinkId)->GetAddress(),
                            setupLinks);

    // request the STA to disassociate
    m_staMac->ForceDisassociation(m_params.sendDisassoc);
}

void
WifiDisassocAndReassocTest::CheckAfterDisassoc(Ptr<WifiMac> local)
{
    NS_TEST_EXPECT_MSG_EQ(((local == m_staMac) || (local == m_ap1Mac)),
                          true,
                          "Unexpected local MAC");
    std::string context{(local == m_staMac ? "STA: " : "AP1: ")};

    const auto remote =
        (local == m_staMac ? StaticCast<WifiMac>(m_ap1Mac) : StaticCast<WifiMac>(m_staMac));
    const auto localIsSender =
        (local == m_staMac && m_params.trafficDir == WifiDirection::UPLINK) ||
        (local == m_ap1Mac && m_params.trafficDir == WifiDirection::DOWNLINK);

    Mac48Address addr;
    const auto wasMlSetup =
        (m_params.assocType == WifiAssocType::ML_SETUP) && (m_ap1Mac->GetNLinks() > 1);

    // check that queues have been blocked with DISASSOCIATION reason
    for (const auto& [staLinkId, apLinkId] : m_params.firstSetupLinks)
    {
        const auto localLinkId =
            (wasMlSetup ? apLinkId : (local == m_staMac ? staLinkId : apLinkId));
        const auto remoteLinkId =
            (wasMlSetup ? apLinkId : (local == m_staMac ? apLinkId : staLinkId));
        const auto linkAddr = remote->GetFrameExchangeManager(remoteLinkId)->GetAddress();
        addr = (wasMlSetup ? remote->GetAddress() : linkAddr);

        for (const auto& [aci, ac] : wifiAcList)
        {
            for (const auto& queueId : std::initializer_list<WifiContainerQueueId>{
                     {WIFI_QOSDATA_QUEUE, WifiRcvAddr::UNICAST, addr, ac.GetLowTid()},
                     {WIFI_QOSDATA_QUEUE, WifiRcvAddr::UNICAST, addr, ac.GetHighTid()},
                     {WIFI_CTL_QUEUE, WifiRcvAddr::UNICAST, addr, std::nullopt},
                     {WIFI_CTL_QUEUE, WifiRcvAddr::UNICAST, linkAddr, std::nullopt}})
            {
                if (auto mask =
                        local->GetMacQueueScheduler()->GetQueueLinkMask(aci, queueId, localLinkId))
                {
                    NS_TEST_EXPECT_MSG_EQ(mask->test(static_cast<std::size_t>(
                                              WifiQueueBlockedReason::DISASSOCIATION)),
                                          true,
                                          context << "Expected transmissions on link "
                                                  << +localLinkId << " to be blocked for queue "
                                                  << queueId);
                }
            }
        }

        if (localIsSender)
        {
            // check that QoS data frames have not been dropped
            const WifiContainerQueueId queueId{WIFI_QOSDATA_QUEUE, WifiRcvAddr::UNICAST, addr, 0};
            NS_TEST_EXPECT_MSG_EQ(local->GetTxopQueue(AC_BE)->GetNPackets(queueId),
                                  m_nPackets,
                                  context << "Unexpected number of QoS data frames queued");
        }
    }

    if (local == m_staMac)
    {
        NS_TEST_EXPECT_MSG_EQ(m_staMac->IsAssociated(),
                              false,
                              context << "Expected STA in unassociated state");
    }
    else if (m_params.sendDisassoc)
    {
        NS_TEST_EXPECT_MSG_EQ(m_ap1Mac->IsAssociated(addr).has_value(),
                              false,
                              context << "Expected STA in unassociated state");
    }

    if (localIsSender)
    {
        // check that control queues have been flushed if and only if ML setup was performed and STA
        // did not notify disassociation to the AP
        const auto acBe = local->GetTxopQueue(AC_BE);
        const auto remoteBaLinkAddr = remote->GetFrameExchangeManager(m_baLinkId)->GetAddress();
        NS_TEST_EXPECT_MSG_EQ(
            acBe->GetNPackets(
                {WIFI_CTL_QUEUE, WifiRcvAddr::UNICAST, remoteBaLinkAddr, std::nullopt}),
            (m_params.sendDisassoc || wasMlSetup ? 0 : 1),
            context << "Expected BAR to be kept only if legacy association was performed and STA "
                       "did not notify disassociation to the AP");
    }

    // A Block Ack agreement is still established with the remote device if and only if the STA has
    // not sent a Disassociation frame
    NS_TEST_EXPECT_MSG_EQ((localIsSender
                               ? local->GetBaAgreementEstablishedAsOriginator(addr, 0).has_value()
                               : local->GetBaAgreementEstablishedAsRecipient(addr, 0).has_value()),
                          !m_params.sendDisassoc,
                          context << "Unexpected Block Ack agreement state");
}

void
WifiDisassocAndReassocTest::InsertEvents()
{
    using enum WifiDirection;

    const auto isMlSetup =
        (m_staMac->GetAssocType() == WifiAssocType::ML_SETUP) && (m_ap1Mac->GetNLinks() > 1);

    // lambda to check that a BAR is queued before disassociation
    auto checkQueuedBarBeforeDisassoc = [=, this]() {
        auto staLinkId = m_baLinkId;
        if (!isMlSetup)
        {
            // find the STA link ID corresponding to the link setup by the AP
            auto it = std::find_if(m_params.firstSetupLinks.cbegin(),
                                   m_params.firstSetupLinks.cend(),
                                   [this](auto&& pair) { return pair.second == m_baLinkId; });
            NS_TEST_ASSERT_MSG_EQ((it != m_params.firstSetupLinks.cend()),
                                  true,
                                  "AP link ID (" << +m_baLinkId
                                                 << ") not found in given setup links");
            staLinkId = it->first;
        }
        auto mpdu = StaticCast<HtFrameExchangeManager>(m_staMac->GetFrameExchangeManager(staLinkId))
                        ->GetBar(AC_BE);
        NS_TEST_ASSERT_MSG_NE(mpdu, nullptr, "Expected a BAR before disassociation");
        NS_TEST_EXPECT_MSG_EQ(
            m_ap1Mac->GetLinkIdByAddress(mpdu->GetHeader().GetAddr1()).has_value(),
            true,
            "Expected BAR to be addressed to an AP link address");
    };

    if (m_staMac->GetAssocType() == WifiAssocType::LEGACY)
    {
        const auto staLinkId = m_params.firstSetupLinks.front().first;
        m_ap1Sockets[DOWNLINK].SetPhysicalAddress(
            m_staMac->GetFrameExchangeManager(staLinkId)->GetAddress());
    }

    const auto nLinks = m_staMac->GetSetupLinkIds().size();
    NS_TEST_ASSERT_MSG_GT_OR_EQ(nLinks, 1, "No link setup");
    auto client = (m_params.trafficDir == DOWNLINK ? m_ap1Mac->GetDevice() : m_staMac->GetDevice());

    // After association, Data Null frames are sent on all the links other than the one used for
    // setup to switch to active mode. Data Null frames have been excluded from the list of events
    for (std::size_t i = 0; i < nLinks - 1; ++i)
    {
        m_events.emplace_back(
            WIFI_MAC_CTL_ACK,
            [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
                // Generate more packets to test correct reception after last Ack
                if (i == nLinks - 2)
                {
                    client->GetNode()->AddApplication(
                        GetApplication(m_params.trafficDir, m_firstApId, m_nPackets, 1000));
                }
            });
    }

    if (nLinks == 1)
    {
        // no Data Null frame, generate packets directly
        client->GetNode()->AddApplication(
            GetApplication(m_params.trafficDir, m_firstApId, m_nPackets, 1000));
    }

    // BlockAck agreement establishment
    m_events.emplace_back(WIFI_MAC_MGT_ACTION);
    m_events.emplace_back(WIFI_MAC_CTL_ACK);
    m_events.emplace_back(WIFI_MAC_MGT_ACTION);
    m_events.emplace_back(WIFI_MAC_CTL_ACK);

    // First A-MPDU
    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetNMpdus(),
                                  m_nPackets,
                                  "Unexpected number of MPDUs in first A-MPDU");

            // in the uplink case, corrupt the first two MPDUs so that the STA does not receive it
            if (m_params.trafficDir == UPLINK)
            {
                m_ap1ErrorModel->SetList(
                    {psdu->GetPayload(0)->GetUid(), psdu->GetPayload(1)->GetUid()});
            }

            const auto txMac = (m_params.trafficDir == UPLINK ? StaticCast<WifiMac>(m_staMac)
                                                              : StaticCast<WifiMac>(m_ap1Mac));
            const auto rxMac = (m_params.trafficDir == DOWNLINK ? StaticCast<WifiMac>(m_staMac)
                                                                : StaticCast<WifiMac>(m_ap1Mac));
            NS_TEST_EXPECT_MSG_EQ(
                psdu->GetAddr2(),
                txMac->GetFrameExchangeManager(linkId)->GetAddress(),
                "Unexpected TA for first QoS data frames sent after first association");
            NS_TEST_EXPECT_MSG_EQ(rxMac->GetLinkIdByAddress(psdu->GetAddr1()).has_value(),
                                  true,
                                  "RA (" << psdu->GetAddr1()
                                         << ") does not belong to the expected device");
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_BACKRESP,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            const auto txMac = (m_params.trafficDir == UPLINK ? StaticCast<WifiMac>(m_ap1Mac)
                                                              : StaticCast<WifiMac>(m_staMac));
            NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                  txMac->GetFrameExchangeManager(linkId)->GetAddress(),
                                  "Unexpected TA for first BlockAck sent after first association");
            m_baLinkId = linkId;

            // in the uplink case, corrupt the BlockAck so that the STA does not receive it
            if (m_params.trafficDir == UPLINK)
            {
                m_staErrorModel->SetList({psdu->GetPayload(0)->GetUid()});
                // enable reception of the two MPDUs next time they are transmitted
                m_ap1ErrorModel->SetList({});
                const auto txDuration =
                    WifiPhy::CalculateTxDuration(psdu,
                                                 txVector,
                                                 txMac->GetWifiPhy(linkId)->GetPhyBand());

                // if a Disassociation frame is sent, request disassociation now. Disassociation
                // actually takes place when the Disassociation frame is acked, but we need to
                // enqueue the Disassociation frame before the BlockAckReq (enqueued at BlockAck
                // timeout) to send the Disassociation frame first and be able to test the
                // situation in which a BlockAckReq is queued when disassociation occurs
                if (m_params.sendDisassoc)
                {
                    Simulator::Schedule(txDuration,
                                        &WifiDisassocAndReassocTest::StartRoaming,
                                        this);
                }
                else
                {
                    // Disassociation takes place immediately when a Disassociation frame is not
                    // sent, thus wait until after BlockAck timeout (which occurs at the end of the
                    // expected BlockAck reception because the BlockAck is corrupted) to request
                    // disassociation
                    Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY, [=, this] {
                        checkQueuedBarBeforeDisassoc();
                        StartRoaming();
                        CheckAfterDisassoc(m_staMac);
                    });
                }
            }
        });

    if (m_params.sendDisassoc)
    {
        m_events.emplace_back(
            WIFI_MAC_MGT_DISASSOCIATION,
            [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
                NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                      m_staMac->GetFrameExchangeManager(linkId)->GetAddress(),
                                      "Unexpected TA for Disassociation frame");
                // the Disassociation frame is sent after BlockAck timeout, thus a BAR should be
                // already queued
                checkQueuedBarBeforeDisassoc();
            });

        m_events.emplace_back(
            WIFI_MAC_CTL_ACK,
            [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
                CheckAfterDisassoc(m_ap1Mac);
                const auto txDuration =
                    WifiPhy::CalculateTxDuration(psdu,
                                                 txVector,
                                                 m_ap1Mac->GetWifiPhy(linkId)->GetPhyBand());
                Simulator::Schedule(txDuration + MAX_PROPAGATION_DELAY,
                                    &WifiDisassocAndReassocTest::CheckAfterDisassoc,
                                    this,
                                    m_staMac);
            });
    }

    // Roaming
    m_events.emplace_back(WIFI_MAC_MGT_ASSOCIATION_REQUEST);
    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            // if the first AP has not received a Disassociation frame and receives an Association
            // frame, it realizes that the STA is re-associating, thus clears the STA state
            if (m_params.secondApId == m_firstApId && !m_params.sendDisassoc)
            {
                CheckAfterDisassoc(m_ap1Mac);
            }
        });

    // STA has not dropped the BlockAckReq frame if and only if STA has not sent a Disassociation
    // frame (otherwise all control frames are dropped), STA established legacy association and
    // reassociates with the same AP (and same link)
    const auto hasBlockAckReq =
        (!m_params.sendDisassoc && !isMlSetup && (m_params.secondApId == m_firstApId) &&
         (m_params.firstApLinkId == m_params.secondApLinkId));

    m_events.emplace_back(WIFI_MAC_MGT_ASSOCIATION_RESPONSE);
    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            // check that STA still has a queued BlockAckReq only in the case described above
            for (linkId_t i = 0; i < m_ap1Mac->GetNLinks(); ++i)
            {
                WifiContainerQueueId queueId{WIFI_CTL_QUEUE,
                                             WifiRcvAddr::UNICAST,
                                             m_ap1Mac->GetFrameExchangeManager(i)->GetAddress(),
                                             std::nullopt};
                NS_TEST_EXPECT_MSG_EQ(
                    (m_staMac->GetTxopQueue(AC_BE)->PeekByQueueId(queueId) == nullptr),
                    !hasBlockAckReq,
                    "Expected" << (hasBlockAckReq ? "" : " no") << " BlockAckReq in queue "
                               << queueId);
            }
        });

    // A new BlockAck agreement must be established if STA roams to a different AP or STA destroyed
    // previous agreement (which occurs when STA sends a Disassociation frame) or legacy association
    // is performed and STA setups a different link than before
    if (m_params.secondApId != m_firstApId || m_params.sendDisassoc ||
        (!isMlSetup && m_params.firstApLinkId != m_params.secondApLinkId))
    {
        m_events.emplace_back(WIFI_MAC_MGT_ACTION);
        m_events.emplace_back(WIFI_MAC_CTL_ACK);
        m_events.emplace_back(WIFI_MAC_MGT_ACTION);
        m_events.emplace_back(WIFI_MAC_CTL_ACK);
    }

    if (hasBlockAckReq)
    {
        m_events.emplace_back(WIFI_MAC_CTL_BACKREQ);
        m_events.emplace_back(WIFI_MAC_CTL_BACKRESP);
    }

    // First A-MPDU retransmitted
    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetNMpdus(),
                                  (hasBlockAckReq ? 2 : m_nPackets),
                                  "Unexpected number of MPDUs in first A-MPDU after roaming");

            const auto txMac = (m_params.trafficDir == UPLINK
                                    ? StaticCast<WifiMac>(m_staMac)
                                    : (m_params.secondApId == 1 ? StaticCast<WifiMac>(m_ap1Mac)
                                                                : StaticCast<WifiMac>(m_ap2Mac)));
            const auto rxMac = (m_params.trafficDir == DOWNLINK
                                    ? StaticCast<WifiMac>(m_staMac)
                                    : (m_params.secondApId == 1 ? StaticCast<WifiMac>(m_ap1Mac)
                                                                : StaticCast<WifiMac>(m_ap2Mac)));
            NS_TEST_EXPECT_MSG_EQ(
                psdu->GetAddr2(),
                txMac->GetFrameExchangeManager(linkId)->GetAddress(),
                "Unexpected TA for first QoS data frames sent after second association");
            NS_TEST_EXPECT_MSG_EQ(rxMac->GetLinkIdByAddress(psdu->GetAddr1()).has_value(),
                                  true,
                                  "RA (" << psdu->GetAddr1()
                                         << ") does not belong to the expected device");
        });

    m_events.emplace_back(WIFI_MAC_CTL_BACKRESP);
}

void
WifiDisassocAndReassocTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(m_events.empty(), true, "Not all events took place");

    Simulator::Destroy();

    std::size_t expectedAssocWithAp1Count{0};
    std::size_t expectedAssocWithAp2Count{0};
    std::size_t expectedAssocSeenByAp1Count{0};
    std::size_t expectedAssocSeenByAp2Count{0};

    for (const auto id : {m_firstApId, m_params.secondApId})
    {
        switch (id)
        {
        case 1:
            ++expectedAssocWithAp1Count;
            ++expectedAssocSeenByAp1Count;
            break;
        case 2:
            ++expectedAssocWithAp2Count;
            ++expectedAssocSeenByAp2Count;
            break;
        default:
            NS_TEST_ASSERT_MSG_EQ(true, false, "Unexpected AP ID " << id);
        }
    }

    NS_TEST_EXPECT_MSG_EQ(m_assocWithAp1Count,
                          expectedAssocWithAp1Count,
                          "Unexpected number of associations with AP 1 (seen by STA)");
    NS_TEST_EXPECT_MSG_EQ(m_assocWithAp2Count,
                          expectedAssocWithAp2Count,
                          "Unexpected number of associations with AP 2 (seen by STA)");
    NS_TEST_EXPECT_MSG_EQ(m_assocSeenByAp1Count,
                          expectedAssocSeenByAp1Count,
                          "Unexpected number of associations with AP 1 (seen by AP 1)");
    NS_TEST_EXPECT_MSG_EQ(m_assocSeenByAp2Count,
                          expectedAssocSeenByAp2Count,
                          "Unexpected number of associations with AP 2 (seen by AP 2)");
}

WifiAssociationTestSuite::WifiAssociationTestSuite()
    : TestSuite("wifi-assoc-test", Type::UNIT)
{
    for (const auto trafficDir : {WifiDirection::UPLINK})
    {
        for (const auto sendDisassoc : {true, false})
        {
            using ParamsList = std::list<WifiDisassocAndReassocTest::Params>;

            // Single-link devices
            // clang-format off
            for (const WifiAssociationTestBase::BaseParams baseParams =
                     {.ap1Channels = "{0,40,BAND_5GHZ,0}",
                      .ap2Channels = "{0,40,BAND_5GHZ,0}",
                      .staChannels = "{0,40,BAND_5GHZ,0}"};
                 const auto& params :
                 ParamsList{//  legacy setup, re-associating to the same AP (distinct link)
                            {.assocType = WifiAssocType::LEGACY,
                             .firstApLinkId = 0,
                             .firstSetupLinks = {{0, 0}},
                             .secondApId = 1,
                             .secondApLinkId = 0,
                             .secondSetupLinks = {{0, 0}},
                             .trafficDir = trafficDir,
                             .sendDisassoc = sendDisassoc},
                            //  legacy setup, roaming to a different AP
                            {.assocType = WifiAssocType::LEGACY,
                             .firstApLinkId = 0,
                             .firstSetupLinks = {{0, 0}},
                             .secondApId = 2,
                             .secondApLinkId = 0,
                             .secondSetupLinks = {{0, 0}},
                             .trafficDir = trafficDir,
                             .sendDisassoc = sendDisassoc}})
            // clang-format on
            {
                AddTestCase(new WifiDisassocAndReassocTest(baseParams, params),
                            TestCase::Duration::QUICK);
            }

            // Multi-link devices using either ML setup or legacy association
            // clang-format off
            for (const WifiAssociationTestBase::BaseParams baseParams =
                     {.ap1Channels = "{0,40,BAND_5GHZ,0};{0,40,BAND_6GHZ,0};{0,40,BAND_2_4GHZ,0}",
                      .ap2Channels = "{0,40,BAND_6GHZ,0};{0,40,BAND_2_4GHZ,0}",
                      .staChannels = "{0,40,BAND_2_4GHZ,0};{0,40,BAND_5GHZ,0};{0,40,BAND_6GHZ,0}"};
                 const auto& params :
                 ParamsList{// ML setup, re-associating to the same AP (distinct links)
                            {.assocType = WifiAssocType::ML_SETUP,
                             .firstApLinkId = 0,
                             .firstSetupLinks = {{1, 0}, {0, 2}}, // STA link 1->0, 0->2
                             .secondApId = 1,
                             .secondApLinkId = 1,
                             .secondSetupLinks = {{1, 1}},
                             .trafficDir = trafficDir,
                             .sendDisassoc = sendDisassoc},
                            // ML setup, roaming to a different AP
                            {.assocType = WifiAssocType::ML_SETUP,
                             .firstApLinkId = 0,
                             .firstSetupLinks = {{1, 0}, {0, 2}}, // STA link 1->0, 0->2
                             .secondApId = 2,
                             .secondApLinkId = 0,
                             .secondSetupLinks = {{1, 0}},
                             .trafficDir = trafficDir,
                             .sendDisassoc = sendDisassoc},
                            //  legacy setup, re-associating to the same AP (distinct link)
                            {.assocType = WifiAssocType::LEGACY,
                             .firstApLinkId = 0,
                             .firstSetupLinks = {{1, 0}},
                             .secondApId = 1,
                             .secondApLinkId = 1,
                             .secondSetupLinks = {{2, 1}},
                             .trafficDir = trafficDir,
                             .sendDisassoc = sendDisassoc},
                            //  legacy setup, roaming to a different AP
                            {.assocType = WifiAssocType::LEGACY,
                             .firstApLinkId = 0,
                             .firstSetupLinks = {{1, 0}},
                             .secondApId = 2,
                             .secondApLinkId = 1,
                             .secondSetupLinks = {{0, 1}},
                             .trafficDir = trafficDir,
                             .sendDisassoc = sendDisassoc}})
            // clang-format on
            {
                AddTestCase(new WifiDisassocAndReassocTest(baseParams, params),
                            TestCase::Duration::QUICK);
            }
        }
    }
}

static WifiAssociationTestSuite g_wifiAssociationTestSuite; ///< the test suite
