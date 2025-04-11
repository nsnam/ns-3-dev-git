/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sharan Naribole <sharan.naribole@gmail.com>
 */

#include "wifi-static-setup-helper.h"

#include "ns3/ap-wifi-mac.h"
#include "ns3/assert.h"
#include "ns3/boolean.h"
#include "ns3/frame-exchange-manager.h"
#include "ns3/log.h"
#include "ns3/mgt-headers.h"
#include "ns3/net-device-container.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-remote-station-manager.h"
#include "ns3/wifi-utils.h"

NS_LOG_COMPONENT_DEFINE("WifiStaticSetupHelper");

namespace ns3
{

void
WifiStaticSetupHelper::SetStaticAssociation(Ptr<WifiNetDevice> bssDev,
                                            const NetDeviceContainer& clientDevs)
{
    NS_LOG_FUNCTION_NOARGS();

    for (auto i = clientDevs.Begin(); i != clientDevs.End(); ++i)
    {
        auto clientDev = DynamicCast<WifiNetDevice>(*i);
        NS_ASSERT_MSG(clientDev, "WifiNetDevice expected");
        SetStaticAssociation(bssDev, clientDev);
    }
}

void
WifiStaticSetupHelper::SetStaticAssociation(Ptr<WifiNetDevice> bssDev, Ptr<WifiNetDevice> clientDev)
{
    NS_LOG_FUNCTION_NOARGS();

    Simulator::ScheduleNow(
        [=] { WifiStaticSetupHelper::SetStaticAssocPostInit(bssDev, clientDev); });
}

void
WifiStaticSetupHelper::SetStaticAssocPostInit(Ptr<WifiNetDevice> bssDev,
                                              Ptr<WifiNetDevice> clientDev)
{
    NS_LOG_FUNCTION_NOARGS();

    auto clientMac = DynamicCast<StaWifiMac>(clientDev->GetMac());
    auto apMac = DynamicCast<ApWifiMac>(bssDev->GetMac());

    NS_ABORT_MSG_IF(!apMac || !clientMac, "Invalid static capabilities exchange case");
    SetStaticAssocPostInit(apMac, clientMac);
}

void
WifiStaticSetupHelper::SetStaticAssocPostInit(Ptr<ApWifiMac> apMac, Ptr<StaWifiMac> clientMac)
{
    NS_LOG_FUNCTION_NOARGS();

    NS_ASSERT_MSG(apMac, "Expected ApWifiMac");
    NS_ASSERT_MSG(clientMac, "Expected StaWifiMac");

    // disable scanning
    clientMac->SetAttribute("EnableScanning", BooleanValue(false));

    const auto linkIdMap = GetLinkIdMap(apMac, clientMac);
    const auto nClientLinks = clientMac->GetNLinks();
    const auto isMldAssoc = nClientLinks > 1;
    const auto apMldAddr = apMac->GetAddress();
    const auto clientMldAddr = clientMac->GetAddress();

    // Swap links at client MAC to match with AP (MLD)
    clientMac->SwapLinks(linkIdMap);
    const auto clientLinkIds = clientMac->GetLinkIds();
    const auto assocLinkId = *clientLinkIds.cbegin();
    std::optional<MultiLinkElement> apMle;
    if (isMldAssoc)
    {
        apMle = apMac->GetMultiLinkElement(assocLinkId, WIFI_MAC_MGT_BEACON);
    }
    std::shared_ptr<CommonInfoBasicMle> mleCommonInfo;
    for (const auto clientLinkId : clientLinkIds)
    {
        // AP Link ID matches with non-AP MLD Link ID after swap operation
        // using WifiMac::SwapLinks performed above
        const auto apLinkId = clientLinkId;
        auto& clientLink = clientMac->GetLink(clientLinkId);

        // Set BSSID
        auto apLinkAddr = apMac->GetFrameExchangeManager(apLinkId)->GetAddress();
        clientMac->SetBssid(apLinkAddr, clientLinkId);
        clientLink.bssid = apLinkAddr;
        if (!isMldAssoc)
        {
            continue;
        }
        NS_ASSERT_MSG(apMle.has_value(), "Expected Multi-link Element");
        if (!mleCommonInfo)
        {
            mleCommonInfo = std::make_shared<CommonInfoBasicMle>(apMle->GetCommonInfoBasic());
        }
        auto clientManager = clientMac->GetWifiRemoteStationManager(clientLinkId);
        clientManager->AddStationMleCommonInfo(apLinkAddr, mleCommonInfo);
    }

    // Association Request
    auto assocReq = GetAssocReq(clientMac, assocLinkId, isMldAssoc);
    auto clientLinkAddr = clientMac->GetFrameExchangeManager(assocLinkId)->GetAddress();
    WifiMacHeader assocReqHdr(WIFI_MAC_MGT_ASSOCIATION_REQUEST);
    assocReqHdr.SetAddr2(clientLinkAddr);
    auto assocSuccess = apMac->ReceiveAssocRequest(assocReq, clientLinkAddr, assocLinkId);
    apMac->ParseReportedStaInfo(assocReq, clientLinkAddr, assocLinkId);
    NS_ASSERT_MSG(assocSuccess,
                  "Static Association failed AP: " << apMldAddr << ", STA: " << clientMldAddr);

    // Association Response
    clientMac->SetState(StaWifiMac::WAIT_ASSOC_RESP);
    auto assocResp = GetAssocResp(clientLinkAddr, apMac, assocLinkId, isMldAssoc);
    auto assocRespMacHdr = GetAssocRespMacHdr(clientLinkAddr, apMac, assocLinkId);
    auto linkIdStaAddrMap = apMac->GetLinkIdStaAddrMap(assocResp, clientLinkAddr, assocLinkId);
    apMac->SetAid(assocResp, linkIdStaAddrMap);
    auto packet = Create<Packet>();
    packet->AddHeader(assocResp);
    auto mpdu = Create<WifiMpdu>(packet, assocRespMacHdr);
    clientMac->ReceiveAssocResp(mpdu, assocLinkId);

    // Record Association success in Remote STA manager
    for (auto clientLinkId : clientLinkIds)
    {
        const auto apLinkId = clientLinkId;
        clientLinkAddr = clientMac->GetFrameExchangeManager(clientLinkId)->GetAddress();
        apMac->GetWifiRemoteStationManager(apLinkId)->RecordGotAssocTxOk(clientLinkAddr);
        const auto aid = apMac->GetAssociationId(clientLinkAddr, apLinkId);
        if (const auto gcrMgr = apMac->GetGcrManager())
        {
            const auto extendedCapabilities =
                apMac->GetWifiRemoteStationManager(apLinkId)->GetStationExtendedCapabilities(
                    clientLinkAddr);
            gcrMgr->NotifyStaAssociated(clientLinkAddr,
                                        extendedCapabilities &&
                                            (extendedCapabilities->m_robustAvStreaming > 0));
        }
        apMac->m_assocLogger(aid, clientLinkAddr);
    }

    NS_LOG_DEBUG("Assoc success AP addr=" << apMldAddr << ", STA addr=" << clientMldAddr);

    if (isMldAssoc)
    {
        // Update TID-to-Link Mapping in MAC queues
        apMac->ApplyTidLinkMapping(clientMldAddr, WifiDirection::DOWNLINK);
        clientMac->ApplyTidLinkMapping(apMldAddr, WifiDirection::UPLINK);
    }
}

std::map<linkId_t, linkId_t>
WifiStaticSetupHelper::GetLinkIdMap(Ptr<WifiNetDevice> apDev, Ptr<WifiNetDevice> clientDev)
{
    auto apMac = DynamicCast<ApWifiMac>(apDev->GetMac());
    NS_ASSERT_MSG(apMac, "Expected ApWifiMac");
    auto clientMac = DynamicCast<StaWifiMac>(clientDev->GetMac());
    NS_ASSERT_MSG(clientMac, "Expected StaWifiMac");
    return GetLinkIdMap(apMac, clientMac);
}

std::map<linkId_t, linkId_t>
WifiStaticSetupHelper::GetLinkIdMap(Ptr<ApWifiMac> apMac, Ptr<StaWifiMac> clientMac)
{
    NS_ASSERT(apMac && clientMac);
    const auto nApLinks = apMac->GetNLinks();
    const auto nClientLinks = clientMac->GetNLinks();
    // Assumed all links of non-AP MLD need to be associated
    NS_ASSERT_MSG(
        nApLinks >= nClientLinks,
        "Expected AP MLD to have at least the same number of links than non-AP MLD, nApLinks="
            << nApLinks << ", nClientLinks=" << nClientLinks);

    std::map<linkId_t, linkId_t> linkIdMap{};
    auto apCandidateLinks = apMac->GetLinkIds();

    for (const auto clientLinkId : clientMac->GetLinkIds())
    {
        for (const auto apLinkId : apCandidateLinks)
        {
            const auto apPhy = apMac->GetWifiPhy(apLinkId);
            const auto clientPhy = clientMac->GetWifiPhy(clientLinkId);
            if (!apPhy || !clientPhy)
            {
                continue;
            }
            NS_LOG_DEBUG("AP channel: " << apPhy->GetOperatingChannel());
            NS_LOG_DEBUG("Client channel: " << clientPhy->GetOperatingChannel());
            const auto isMatch = (apPhy->GetOperatingChannel().GetPrimaryChannel(MHz_u{20}) ==
                                  clientPhy->GetOperatingChannel().GetPrimaryChannel(MHz_u{20}));
            NS_LOG_DEBUG("Client Link ID = " << +clientLinkId << ", AP Link ID Candidate="
                                             << +apLinkId << ", isMatch=" << isMatch);
            if (isMatch)
            {
                linkIdMap[clientLinkId] = apLinkId;
                break;
            }
        }

        NS_ASSERT_MSG(linkIdMap.contains(clientLinkId),
                      "No matching AP found for STA PHY setting link ID=" << +clientLinkId);
        apCandidateLinks.erase(linkIdMap.at(clientLinkId));
    }

    return linkIdMap;
}

WifiMacHeader
WifiStaticSetupHelper::GetAssocRespMacHdr(Mac48Address clientLinkAddr,
                                          Ptr<ApWifiMac> apMac,
                                          linkId_t apLinkId)
{
    WifiMacHeader hdr(WIFI_MAC_MGT_ASSOCIATION_RESPONSE);
    hdr.SetAddr1(clientLinkAddr);
    auto apLinkAddr = apMac->GetFrameExchangeManager(apLinkId)->GetAddress();
    hdr.SetAddr2(apLinkAddr);
    hdr.SetAddr3(apLinkAddr);
    return hdr;
}

MgtAssocResponseHeader
WifiStaticSetupHelper::GetAssocResp(Mac48Address clientLinkAddr,
                                    Ptr<ApWifiMac> apMac,
                                    linkId_t apLinkId,
                                    bool isMldAssoc)
{
    auto assocResp = apMac->GetAssocResp(clientLinkAddr, apLinkId);
    if (!isMldAssoc)
    {
        return assocResp;
    }
    auto mle =
        apMac->GetMultiLinkElement(apLinkId, WIFI_MAC_MGT_ASSOCIATION_RESPONSE, clientLinkAddr);
    assocResp.Get<MultiLinkElement>() = mle;
    return assocResp;
}

MgtAssocRequestHeader
WifiStaticSetupHelper::GetAssocReq(Ptr<StaWifiMac> clientMac, linkId_t linkId, bool isMldAssoc)
{
    auto frame = clientMac->GetAssociationRequest(false, linkId);
    auto assocReq = std::get<MgtAssocRequestHeader>(frame);
    if (!isMldAssoc)
    {
        return assocReq;
    }

    auto mle = clientMac->GetBasicMultiLinkElement(false, linkId);
    assocReq.Get<MultiLinkElement>() = mle;
    auto& link = clientMac->GetLink(linkId);
    auto bssid = *link.bssid;
    auto remoteManager = clientMac->GetWifiRemoteStationManager(linkId);
    const auto& mldCap = remoteManager->GetStationMldCapabilities(bssid);
    NS_ASSERT_MSG(mldCap, "Expected MLD Capabilities info for AP MLD");
    auto apNegSupport = mldCap->get().tidToLinkMappingSupport;
    if (apNegSupport > 0)
    {
        assocReq.Get<TidToLinkMapping>() = clientMac->GetTidToLinkMappingElements(
            static_cast<WifiTidToLinkMappingNegSupport>(apNegSupport));
    }

    return assocReq;
}

} // namespace ns3
