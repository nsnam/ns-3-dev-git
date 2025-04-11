/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sharan Naribole <sharan.naribole@gmail.com>
 */

#include "wifi-static-setup-helper.h"

#include "ns3/adhoc-wifi-mac.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/assert.h"
#include "ns3/boolean.h"
#include "ns3/emlsr-manager.h"
#include "ns3/ht-configuration.h"
#include "ns3/ht-frame-exchange-manager.h"
#include "ns3/log.h"
#include "ns3/mgt-action-headers.h"
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

void
WifiStaticSetupHelper::SetStaticBlockAck(Ptr<WifiNetDevice> apDev,
                                         const NetDeviceContainer& clientDevs,
                                         const std::set<tid_t>& tids,
                                         std::optional<Mac48Address> gcrGroupAddr)
{
    NS_LOG_FUNCTION_NOARGS();

    for (auto i = clientDevs.Begin(); i != clientDevs.End(); ++i)
    {
        auto clientDev = DynamicCast<WifiNetDevice>(*i);
        NS_ASSERT_MSG(clientDev, "WifiNetDevice expected");
        if (!clientDev->GetHtConfiguration())
        {
            // Block Ack requires HT support
            continue;
        }
        for (const auto tid : tids)
        {
            SetStaticBlockAck(apDev, clientDev, tid, gcrGroupAddr); // Downlink setup
            SetStaticBlockAck(clientDev, apDev, tid, gcrGroupAddr); // Uplink setup
        }
    }
}

void
WifiStaticSetupHelper::SetStaticBlockAck(Ptr<WifiNetDevice> originatorDev,
                                         Ptr<WifiNetDevice> recipientDev,
                                         tid_t tid,
                                         std::optional<Mac48Address> gcrGroupAddr)
{
    NS_LOG_FUNCTION_NOARGS();

    Simulator::ScheduleNow(&WifiStaticSetupHelper::SetStaticBlockAckPostInit,
                           originatorDev,
                           recipientDev,
                           tid,
                           gcrGroupAddr);
}

void
WifiStaticSetupHelper::SetStaticBlockAckPostInit(Ptr<WifiNetDevice> originatorDev,
                                                 Ptr<WifiNetDevice> recipientDev,
                                                 tid_t tid,
                                                 std::optional<Mac48Address> gcrGroupAddr)
{
    NS_LOG_FUNCTION_NOARGS();

    // Originator Device
    const auto originatorMac = originatorDev->GetMac();
    const auto originatorLinkId = *(originatorMac->GetLinkIds().begin());
    const auto originatorFem = DynamicCast<HtFrameExchangeManager>(
        originatorMac->GetFrameExchangeManager(originatorLinkId));
    NS_ASSERT_MSG(originatorFem, "Block ACK setup requires HT support");
    const auto originatorBaManager = originatorMac->GetQosTxop(tid)->GetBaManager();

    // Recipient Device
    const auto recipientMac = recipientDev->GetMac();
    const auto recipientLinkId = *(recipientMac->GetLinkIds().begin());
    const auto recipientFem =
        DynamicCast<HtFrameExchangeManager>(recipientMac->GetFrameExchangeManager(recipientLinkId));
    NS_ASSERT_MSG(recipientFem, "Block ACK setup requires HT support");
    const auto recipientBaManager = recipientMac->GetQosTxop(tid)->GetBaManager();

    const auto originatorAddr = GetBaOriginatorAddr(originatorMac, recipientMac);
    const auto recipientAddr = GetBaRecipientAddr(originatorMac, recipientMac);

    // Early return if BA Agreement already exists
    if (originatorMac->GetBaAgreementEstablishedAsOriginator(recipientAddr, tid, gcrGroupAddr))
    {
        return;
    }

    // ADDBA Request
    MgtAddBaRequestHeader reqHdr;
    reqHdr.SetAmsduSupport(true);
    reqHdr.SetImmediateBlockAck();
    reqHdr.SetTid(tid);
    reqHdr.SetBufferSize(originatorMac->GetMpduBufferSize());
    reqHdr.SetTimeout(0);
    reqHdr.SetStartingSequence(0);
    if (gcrGroupAddr)
    {
        reqHdr.SetGcrGroupAddress(gcrGroupAddr.value());
    }

    // ADDBA Response
    MgtAddBaResponseHeader respHdr;
    StatusCode code;
    code.SetSuccess();
    respHdr.SetStatusCode(code);
    respHdr.SetAmsduSupport(true);
    respHdr.SetImmediateBlockAck();
    respHdr.SetTid(tid);
    auto agrBufferSize = std::min(reqHdr.GetBufferSize(), recipientMac->GetMpduBufferSize());
    respHdr.SetBufferSize(agrBufferSize);
    respHdr.SetTimeout(0);
    if (auto gcrGroupAddr = reqHdr.GetGcrGroupAddress())
    {
        respHdr.SetGcrGroupAddress(gcrGroupAddr.value());
    }

    originatorBaManager->CreateOriginatorAgreement(reqHdr, recipientAddr);
    recipientBaManager->CreateRecipientAgreement(respHdr,
                                                 originatorAddr,
                                                 reqHdr.GetStartingSequence(),
                                                 recipientMac->m_rxMiddle);
    auto recipientAgr [[maybe_unused]] =
        recipientBaManager->GetAgreementAsRecipient(originatorAddr,
                                                    tid,
                                                    reqHdr.GetGcrGroupAddress());
    NS_ASSERT_MSG(recipientAgr.has_value(),
                  "No agreement as recipient found for originator " << originatorAddr << ", TID "
                                                                    << +tid);
    originatorBaManager->UpdateOriginatorAgreement(respHdr,
                                                   recipientAddr,
                                                   reqHdr.GetStartingSequence());
    auto originatorAgr [[maybe_unused]] =
        originatorBaManager->GetAgreementAsOriginator(recipientAddr,
                                                      tid,
                                                      reqHdr.GetGcrGroupAddress());
    NS_ASSERT_MSG(originatorAgr.has_value(),
                  "No agreement as originator found for recipient " << recipientAddr << ", TID "
                                                                    << +tid);
}

Mac48Address
WifiStaticSetupHelper::GetBaOriginatorAddr(Ptr<WifiMac> originatorMac, Ptr<WifiMac> recipientMac)
{
    // Originator is AdhocWifiMac type
    // FIXME Restricted to single link operation, as AdHocWifiMac does not support multi-link yet
    if (const auto origAdhoc = DynamicCast<AdhocWifiMac>(originatorMac))
    {
        return origAdhoc->GetAddress();
    }

    // Recipient is AdhocWifiMac type
    // Return MAC address of link communicating with recipient
    if (const auto recAdhoc = DynamicCast<AdhocWifiMac>(recipientMac))
    {
        const auto origSta = DynamicCast<StaWifiMac>(originatorMac);
        NS_ASSERT_MSG(origSta, "Expected originator StaWifiMac type");
        return origSta->GetLocalAddress(recAdhoc->GetAddress());
    }

    // Infra WLAN case
    auto isOriginatorClient{true};
    auto staMac = DynamicCast<StaWifiMac>(originatorMac);
    if (!staMac)
    {
        staMac = DynamicCast<StaWifiMac>(recipientMac);
        isOriginatorClient = false;
    }
    NS_ASSERT_MSG(staMac, "Expected one of the MACs to be StaWifiMac type");

    const auto setupLinks = staMac->GetSetupLinkIds();
    const auto nSetupLinks = setupLinks.size();
    if (nSetupLinks != 1)
    { // Handle cases other than single link association
        return originatorMac->GetAddress();
    }

    // Handle case where one device is MLD and other is single link device
    // Link MAC address to be used for Block ACK agreement
    // Required for one device to be StaWifiMac type
    const auto linkId = *(setupLinks.cbegin());
    if (isOriginatorClient)
    {
        const auto fem = originatorMac->GetFrameExchangeManager(linkId);
        return fem->GetAddress();
    }
    else
    {
        const auto fem = recipientMac->GetFrameExchangeManager(linkId);
        return fem->GetBssid();
    }
}

Mac48Address
WifiStaticSetupHelper::GetBaRecipientAddr(Ptr<WifiMac> originatorMac, Ptr<WifiMac> recipientMac)
{
    return GetBaOriginatorAddr(recipientMac, originatorMac);
}

void
WifiStaticSetupHelper::SetStaticEmlsr(Ptr<WifiNetDevice> apDev, Ptr<WifiNetDevice> clientDev)
{
    NS_LOG_FUNCTION_NOARGS();

    Simulator::ScheduleNow(&WifiStaticSetupHelper::SetStaticEmlsrPostInit, apDev, clientDev);
}

void
WifiStaticSetupHelper::SetStaticEmlsrPostInit(Ptr<WifiNetDevice> apDev,
                                              Ptr<WifiNetDevice> clientDev)
{
    NS_LOG_FUNCTION_NOARGS();

    auto clientMac = DynamicCast<StaWifiMac>(clientDev->GetMac());
    NS_ASSERT_MSG(clientMac, "Expected StaWifiMac");
    NS_ASSERT_MSG(clientMac->IsAssociated(), "Expected Association complete");
    auto setupLinks = clientMac->GetSetupLinkIds();

    if (const auto isMldAssoc = (setupLinks.size() > 1); !isMldAssoc)
    {
        NS_LOG_DEBUG("Multi-link setup not performed, skipping EMLSR static setup");
        return;
    }
    if (!clientDev->IsEmlsrActivated())
    {
        NS_LOG_DEBUG("Non-AP MLD does not support EMLSR, not performing EMLSR static setup");
        return;
    }

    auto emlsrManager = clientMac->GetEmlsrManager();
    NS_ASSERT_MSG(emlsrManager, "EMLSR Manager not set");
    emlsrManager->ComputeOperatingChannels();
    auto emlOmnReq = emlsrManager->GetEmlOmn();
    auto emlsrLinkId = emlsrManager->GetLinkToSendEmlOmn();
    emlsrManager->ChangeEmlsrMode();
    auto clientLinkAddr = clientMac->GetFrameExchangeManager(emlsrLinkId)->GetAddress();
    auto apMac = DynamicCast<ApWifiMac>(apDev->GetMac());
    NS_ASSERT_MSG(apMac, "Expected ApWifiMac");
    apMac->ReceiveEmlOmn(emlOmnReq, clientLinkAddr, emlsrLinkId);
    apMac->EmlOmnExchangeCompleted(emlOmnReq, clientLinkAddr, emlsrLinkId);
}

void
WifiStaticSetupHelper::SetStaticEmlsr(Ptr<WifiNetDevice> apDev,
                                      const NetDeviceContainer& clientDevs)
{
    NS_LOG_FUNCTION_NOARGS();

    auto apMac = DynamicCast<ApWifiMac>(apDev->GetMac());
    NS_ASSERT_MSG(apMac, "Expected ApWifiMac");
    // Check if AP supports EMLSR
    if ((!apMac->GetEhtSupported()) || (apMac->GetNLinks() == 1))
    {
        NS_LOG_DEBUG("AP does not support MLD, not performing EMLSR static setup");
        return;
    }

    if (!apDev->IsEmlsrActivated())
    {
        NS_LOG_DEBUG("AP MLD does not support EMLSR, not performing EMLSR static setup");
        return;
    }

    for (auto i = clientDevs.Begin(); i != clientDevs.End(); ++i)
    {
        auto clientDev = DynamicCast<WifiNetDevice>(*i);
        NS_ASSERT_MSG(clientDev, "WifiNetDevice expected");
        SetStaticEmlsr(apDev, clientDev);
    }
}

} // namespace ns3
