/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "ht-frame-exchange-manager.h"

#include "ht-configuration.h"

#include "ns3/abort.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/assert.h"
#include "ns3/ctrl-headers.h"
#include "ns3/gcr-manager.h"
#include "ns3/log.h"
#include "ns3/mgt-action-headers.h"
#include "ns3/recipient-block-ack-agreement.h"
#include "ns3/snr-tag.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/vht-configuration.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-utils.h"

#include <array>
#include <optional>

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT WIFI_FEM_NS_LOG_APPEND_CONTEXT

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("HtFrameExchangeManager");

NS_OBJECT_ENSURE_REGISTERED(HtFrameExchangeManager);

TypeId
HtFrameExchangeManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::HtFrameExchangeManager")
                            .SetParent<QosFrameExchangeManager>()
                            .AddConstructor<HtFrameExchangeManager>()
                            .SetGroupName("Wifi");
    return tid;
}

HtFrameExchangeManager::HtFrameExchangeManager()
{
    NS_LOG_FUNCTION(this);
    m_msduAggregator = CreateObject<MsduAggregator>();
    m_mpduAggregator = CreateObject<MpduAggregator>();
}

HtFrameExchangeManager::~HtFrameExchangeManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
HtFrameExchangeManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    if (m_flushGroupcastMpdusEvent.IsPending())
    {
        m_flushGroupcastMpdusEvent.Cancel();
    }
    m_pendingAddBaResp.clear();
    m_msduAggregator = nullptr;
    m_mpduAggregator = nullptr;
    m_psdu = nullptr;
    m_txParams.Clear();
    QosFrameExchangeManager::DoDispose();
}

void
HtFrameExchangeManager::SetWifiMac(const Ptr<WifiMac> mac)
{
    m_msduAggregator->SetWifiMac(mac);
    m_mpduAggregator->SetWifiMac(mac);
    QosFrameExchangeManager::SetWifiMac(mac);
}

Ptr<MsduAggregator>
HtFrameExchangeManager::GetMsduAggregator() const
{
    return m_msduAggregator;
}

Ptr<MpduAggregator>
HtFrameExchangeManager::GetMpduAggregator() const
{
    return m_mpduAggregator;
}

Ptr<BlockAckManager>
HtFrameExchangeManager::GetBaManager(uint8_t tid) const
{
    return m_mac->GetQosTxop(tid)->GetBaManager();
}

bool
HtFrameExchangeManager::NeedSetupBlockAck(Mac48Address recipient, uint8_t tid)
{
    Ptr<QosTxop> qosTxop = m_mac->GetQosTxop(tid);
    bool establish;

    // NOLINTBEGIN(bugprone-branch-clone)
    if (!m_mac->GetHtConfiguration() ||
        (!GetWifiRemoteStationManager()->GetHtSupported(recipient) &&
         !GetWifiRemoteStationManager()->GetStationHe6GhzCapabilities(recipient)))
    {
        // no Block Ack if this device or the recipient are not HT STAs and do not operate
        // in the 6 GHz band
        establish = false;
    }
    else if (auto agreement = qosTxop->GetBaManager()->GetAgreementAsOriginator(recipient, tid);
             agreement && !agreement->get().IsReset())
    {
        // Block Ack agreement already established
        establish = false;
    }
    // NOLINTEND(bugprone-branch-clone)
    else
    {
        WifiContainerQueueId queueId{WIFI_QOSDATA_QUEUE, WIFI_UNICAST, recipient, tid};
        uint32_t packets = qosTxop->GetWifiMacQueue()->GetNPackets(queueId);
        establish =
            (m_mac->Is6GhzBand(m_linkId) ||
             (qosTxop->GetBlockAckThreshold() > 0 && packets >= qosTxop->GetBlockAckThreshold()) ||
             (m_mpduAggregator->GetMaxAmpduSize(recipient, tid, WIFI_MOD_CLASS_HT) > 0 &&
              packets > 1) ||
             m_mac->GetVhtConfiguration());
    }

    NS_LOG_FUNCTION(this << recipient << +tid << establish);
    return establish;
}

std::optional<Mac48Address>
HtFrameExchangeManager::NeedSetupGcrBlockAck(const WifiMacHeader& header)
{
    NS_ASSERT(m_mac->GetTypeOfStation() == AP && m_apMac->UseGcr(header));
    const auto& groupAddress = header.GetAddr1();

    const auto tid = header.GetQosTid();
    auto qosTxop = m_mac->GetQosTxop(tid);
    const auto maxMpduSize =
        m_mpduAggregator->GetMaxAmpduSize(groupAddress, tid, WIFI_MOD_CLASS_HT);
    const auto isGcrBa = (m_apMac->GetGcrManager()->GetRetransmissionPolicy() ==
                          GroupAddressRetransmissionPolicy::GCR_BLOCK_ACK);
    WifiContainerQueueId queueId{WIFI_QOSDATA_QUEUE, WIFI_GROUPCAST, groupAddress, tid};

    for (const auto& recipients =
             m_apMac->GetGcrManager()->GetMemberStasForGroupAddress(groupAddress);
         const auto& nextRecipient : recipients)
    {
        if (auto agreement =
                qosTxop->GetBaManager()->GetAgreementAsOriginator(nextRecipient, tid, groupAddress);
            agreement && !agreement->get().IsReset())
        {
            continue;
        }

        const auto packets = qosTxop->GetWifiMacQueue()->GetNPackets(queueId);
        const auto establish =
            (isGcrBa ||
             (qosTxop->GetBlockAckThreshold() > 0 && packets >= qosTxop->GetBlockAckThreshold()) ||
             (maxMpduSize > 0 && packets > 1));
        NS_LOG_FUNCTION(this << groupAddress << +tid << establish);
        if (establish)
        {
            return nextRecipient;
        }
    }

    return std::nullopt;
}

bool
HtFrameExchangeManager::SendAddBaRequest(Mac48Address dest,
                                         uint8_t tid,
                                         uint16_t startingSeq,
                                         uint16_t timeout,
                                         bool immediateBAck,
                                         Time availableTime,
                                         std::optional<Mac48Address> gcrGroupAddr)
{
    NS_LOG_FUNCTION(this << dest << +tid << startingSeq << timeout << immediateBAck << availableTime
                         << gcrGroupAddr.has_value());
    NS_LOG_DEBUG("Send ADDBA request to " << dest);

    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_MGT_ACTION);
    // use the remote link address if dest is an MLD address
    auto addr1 = GetWifiRemoteStationManager()->GetAffiliatedStaAddress(dest);
    hdr.SetAddr1(addr1 ? *addr1 : dest);
    hdr.SetAddr2(m_self);
    hdr.SetAddr3(m_bssid);
    hdr.SetDsNotTo();
    hdr.SetDsNotFrom();

    WifiActionHeader actionHdr;
    WifiActionHeader::ActionValue action;
    action.blockAck = WifiActionHeader::BLOCK_ACK_ADDBA_REQUEST;
    actionHdr.SetAction(WifiActionHeader::BLOCK_ACK, action);

    Ptr<Packet> packet = Create<Packet>();
    // Setting ADDBARequest header
    MgtAddBaRequestHeader reqHdr;
    reqHdr.SetAmsduSupport(true);
    if (immediateBAck)
    {
        reqHdr.SetImmediateBlockAck();
    }
    else
    {
        reqHdr.SetDelayedBlockAck();
    }
    reqHdr.SetTid(tid);
    /* For now we don't use buffer size field in the ADDBA request frame. The recipient
     * will choose how many packets it can receive under block ack.
     */
    reqHdr.SetBufferSize(0);
    reqHdr.SetTimeout(timeout);
    // set the starting sequence number for the BA agreement
    reqHdr.SetStartingSequence(startingSeq);

    if (gcrGroupAddr)
    {
        reqHdr.SetGcrGroupAddress(*gcrGroupAddr);
    }

    GetBaManager(tid)->CreateOriginatorAgreement(reqHdr, dest);

    packet->AddHeader(reqHdr);
    packet->AddHeader(actionHdr);

    Ptr<WifiMpdu> mpdu = Create<WifiMpdu>(packet, hdr);

    // get the sequence number for the ADDBA Request management frame
    uint16_t sequence = m_txMiddle->GetNextSequenceNumberFor(&mpdu->GetHeader());
    mpdu->GetHeader().SetSequenceNumber(sequence);

    WifiTxParameters txParams;
    txParams.m_txVector =
        GetWifiRemoteStationManager()->GetDataTxVector(mpdu->GetHeader(), m_allowedWidth);
    if (!TryAddMpdu(mpdu, txParams, availableTime))
    {
        NS_LOG_DEBUG("Not enough time to send the ADDBA Request frame");
        return false;
    }

    // Wifi MAC queue scheduler is expected to prioritize management frames
    m_mac->GetQosTxop(tid)->GetWifiMacQueue()->Enqueue(mpdu);
    SendMpduWithProtection(mpdu, txParams);
    return true;
}

void
HtFrameExchangeManager::SendAddBaResponse(const MgtAddBaRequestHeader& reqHdr,
                                          Mac48Address originator)
{
    NS_LOG_FUNCTION(this << originator);
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_MGT_ACTION);
    hdr.SetAddr1(originator);
    hdr.SetAddr2(m_self);
    hdr.SetAddr3(m_bssid);
    hdr.SetDsNotFrom();
    hdr.SetDsNotTo();

    MgtAddBaResponseHeader respHdr;
    StatusCode code;
    code.SetSuccess();
    respHdr.SetStatusCode(code);
    // Here a control about queues type?
    respHdr.SetAmsduSupport(reqHdr.IsAmsduSupported());

    if (reqHdr.IsImmediateBlockAck())
    {
        respHdr.SetImmediateBlockAck();
    }
    else
    {
        respHdr.SetDelayedBlockAck();
    }
    auto tid = reqHdr.GetTid();
    respHdr.SetTid(tid);

    auto bufferSize = std::min(m_mac->GetMpduBufferSize(), m_mac->GetMaxBaBufferSize(originator));
    respHdr.SetBufferSize(bufferSize);
    respHdr.SetTimeout(reqHdr.GetTimeout());

    if (auto gcrGroupAddr = reqHdr.GetGcrGroupAddress())
    {
        respHdr.SetGcrGroupAddress(*gcrGroupAddr);
    }

    WifiActionHeader actionHdr;
    WifiActionHeader::ActionValue action;
    action.blockAck = WifiActionHeader::BLOCK_ACK_ADDBA_RESPONSE;
    actionHdr.SetAction(WifiActionHeader::BLOCK_ACK, action);

    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(respHdr);
    packet->AddHeader(actionHdr);

    // Get the MLD address of the originator, if an ML setup was performed
    if (auto originatorMld = GetWifiRemoteStationManager()->GetMldAddress(originator))
    {
        originator = *originatorMld;
    }
    GetBaManager(tid)->CreateRecipientAgreement(respHdr,
                                                originator,
                                                reqHdr.GetStartingSequence(),
                                                m_rxMiddle);

    auto agreement =
        GetBaManager(tid)->GetAgreementAsRecipient(originator, tid, reqHdr.GetGcrGroupAddress());
    NS_ASSERT(agreement);
    if (respHdr.GetTimeout() != 0)
    {
        Time timeout = MicroSeconds(1024 * agreement->get().GetTimeout());

        agreement->get().m_inactivityEvent =
            Simulator::Schedule(timeout,
                                &HtFrameExchangeManager::SendDelbaFrame,
                                this,
                                originator,
                                tid,
                                false,
                                reqHdr.GetGcrGroupAddress());
    }

    auto mpdu = Create<WifiMpdu>(packet, hdr);

    /*
     * It is possible (though, unlikely) that at this point there are other ADDBA_RESPONSE frame(s)
     * in the MAC queue. This may happen if the recipient receives an ADDBA_REQUEST frame, enqueues
     * an ADDBA_RESPONSE frame, but is not able to successfully transmit it before the timer to
     * wait for ADDBA_RESPONSE expires at the originator. The latter may then send another
     * ADDBA_REQUEST frame, which triggers the creation of another ADDBA_RESPONSE frame.
     * To avoid sending unnecessary ADDBA_RESPONSE frames, we keep track of the previously enqueued
     * ADDBA_RESPONSE frame (if any), dequeue it and replace it with the new ADDBA_RESPONSE frame.
     */

    // remove any pending ADDBA_RESPONSE frame
    AgreementKey key(originator, tid);
    if (auto it = m_pendingAddBaResp.find(key); it != m_pendingAddBaResp.end())
    {
        NS_ASSERT_MSG(it->second, "The pointer to the pending ADDBA_RESPONSE cannot be null");
        DequeueMpdu(it->second);
        m_pendingAddBaResp.erase(it);
    }
    // store the new ADDBA_RESPONSE frame
    m_pendingAddBaResp[key] = mpdu;

    // It is unclear which queue this frame should go into. For now we
    // bung it into the queue corresponding to the TID for which we are
    // establishing an agreement, and push it to the head.
    //  Wifi MAC queue scheduler is expected to prioritize management frames
    m_mac->GetQosTxop(tid)->Queue(mpdu);
}

void
HtFrameExchangeManager::SendDelbaFrame(Mac48Address addr,
                                       uint8_t tid,
                                       bool byOriginator,
                                       std::optional<Mac48Address> gcrGroupAddr)
{
    NS_LOG_FUNCTION(this << addr << +tid << byOriginator << gcrGroupAddr.has_value());
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_MGT_ACTION);
    // use the remote link address if addr is an MLD address
    hdr.SetAddr1(GetWifiRemoteStationManager()->GetAffiliatedStaAddress(addr).value_or(addr));
    hdr.SetAddr2(m_self);
    hdr.SetAddr3(m_bssid);
    hdr.SetDsNotTo();
    hdr.SetDsNotFrom();

    MgtDelBaHeader delbaHdr;
    delbaHdr.SetTid(tid);
    byOriginator ? delbaHdr.SetByOriginator() : delbaHdr.SetByRecipient();
    if (gcrGroupAddr.has_value())
    {
        delbaHdr.SetGcrGroupAddress(gcrGroupAddr.value());
    }

    WifiActionHeader actionHdr;
    WifiActionHeader::ActionValue action;
    action.blockAck = WifiActionHeader::BLOCK_ACK_DELBA;
    actionHdr.SetAction(WifiActionHeader::BLOCK_ACK, action);

    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(delbaHdr);
    packet->AddHeader(actionHdr);

    m_mac->GetQosTxop(tid)->Queue(Create<WifiMpdu>(packet, hdr));
}

uint16_t
HtFrameExchangeManager::GetBaAgreementStartingSequenceNumber(const WifiMacHeader& header)
{
    // if the peeked MPDU has been already transmitted, use its sequence number
    // as the starting sequence number for the BA agreement, otherwise use the
    // next available sequence number
    return header.IsRetry()
               ? header.GetSequenceNumber()
               : m_txMiddle->GetNextSeqNumberByTidAndAddress(header.GetQosTid(), header.GetAddr1());
}

bool
HtFrameExchangeManager::StartFrameExchange(Ptr<QosTxop> edca, Time availableTime, bool initialFrame)
{
    NS_LOG_FUNCTION(this << edca << availableTime << initialFrame);

    // First, check if there is a BAR to be transmitted
    if (auto mpdu = GetBar(edca->GetAccessCategory());
        mpdu && SendMpduFromBaManager(mpdu, availableTime, initialFrame))
    {
        return true;
    }

    Ptr<WifiMpdu> peekedItem = edca->PeekNextMpdu(m_linkId);

    // Even though channel access is requested when the queue is not empty, at
    // the time channel access is granted the lifetime of the packet might be
    // expired and the queue might be empty.
    if (!peekedItem)
    {
        NS_LOG_DEBUG("No frames available for transmission");
        return false;
    }

    const WifiMacHeader& hdr = peekedItem->GetHeader();
    // setup a Block Ack agreement if needed
    if (hdr.IsQosData() && !hdr.GetAddr1().IsGroup() &&
        NeedSetupBlockAck(hdr.GetAddr1(), hdr.GetQosTid()))
    {
        return SendAddBaRequest(hdr.GetAddr1(),
                                hdr.GetQosTid(),
                                GetBaAgreementStartingSequenceNumber(hdr),
                                edca->GetBlockAckInactivityTimeout(),
                                true,
                                availableTime);
    }
    else if (IsGcr(m_mac, hdr))
    {
        if (const auto addbaRecipient = NeedSetupGcrBlockAck(hdr))
        {
            return SendAddBaRequest(addbaRecipient.value(),
                                    hdr.GetQosTid(),
                                    GetBaAgreementStartingSequenceNumber(hdr),
                                    edca->GetBlockAckInactivityTimeout(),
                                    true,
                                    availableTime,
                                    hdr.GetAddr1());
        }
    }

    // Use SendDataFrame if we can try aggregation
    if (hdr.IsQosData() && !hdr.GetAddr1().IsBroadcast() && !peekedItem->IsFragment() &&
        !GetWifiRemoteStationManager()->NeedFragmentation(peekedItem =
                                                              CreateAliasIfNeeded(peekedItem)))
    {
        return SendDataFrame(peekedItem, availableTime, initialFrame);
    }

    // Use the QoS FEM to transmit the frame in all the other cases, i.e.:
    // - the frame is not a QoS data frame
    // - the frame is a broadcast QoS data frame
    // - the frame is a fragment
    // - the frame must be fragmented
    return QosFrameExchangeManager::StartFrameExchange(edca, availableTime, initialFrame);
}

Ptr<WifiMpdu>
HtFrameExchangeManager::GetBar(AcIndex ac,
                               std::optional<uint8_t> optTid,
                               std::optional<Mac48Address> optAddress)
{
    NS_LOG_FUNCTION(this << +ac << optTid.has_value() << optAddress.has_value());
    NS_ASSERT_MSG(optTid.has_value() == optAddress.has_value(),
                  "Either both or none of TID and address must be provided");

    // remove all expired MPDUs from the MAC queue, so that
    // BlockAckRequest frames (if needed) are scheduled
    auto queue = m_mac->GetTxopQueue(ac);
    queue->WipeAllExpiredMpdus();

    Ptr<WifiMpdu> bar;
    Ptr<WifiMpdu> prevBar;
    Ptr<WifiMpdu> selectedBar;

    // we could iterate over all the scheduler's queues and ignore those that do not contain
    // control frames, but it's more efficient to peek frames until we get frames that are
    // not control frames, given that control frames have the highest priority
    while ((bar = queue->PeekFirstAvailable(m_linkId, prevBar)) && bar && bar->GetHeader().IsCtl())
    {
        if (bar->GetHeader().IsBlockAckReq())
        {
            CtrlBAckRequestHeader reqHdr;
            bar->GetPacket()->PeekHeader(reqHdr);
            auto tid = reqHdr.GetTidInfo();
            Mac48Address recipient = bar->GetHeader().GetAddr1();
            auto recipientMld = m_mac->GetMldAddress(recipient);

            // the scheduler should not return a BlockAckReq that cannot be sent on this link:
            // either the TA address is the address of this link or it is the MLD address and
            // the RA field is the MLD address of a device we can communicate with on this link
            NS_ASSERT_MSG(bar->GetHeader().GetAddr2() == m_self ||
                              (bar->GetHeader().GetAddr2() == m_mac->GetAddress() && recipientMld &&
                               GetWifiRemoteStationManager()->GetAffiliatedStaAddress(recipient)),
                          "Cannot use link " << +m_linkId << " to send BAR: " << *bar);

            if (optAddress &&
                (GetWifiRemoteStationManager()->GetMldAddress(*optAddress).value_or(*optAddress) !=
                     GetWifiRemoteStationManager()->GetMldAddress(recipient).value_or(recipient) ||
                 optTid != tid))
            {
                NS_LOG_DEBUG("BAR " << *bar
                                    << " cannot be returned because it is not addressed"
                                       " to the given station for the given TID");
                prevBar = bar;
                continue;
            }

            auto agreement = m_mac->GetBaAgreementEstablishedAsOriginator(
                recipient,
                tid,
                reqHdr.IsGcr() ? std::optional{reqHdr.GetGcrGroupAddress()} : std::nullopt);
            if (const auto isGcrBa =
                    reqHdr.IsGcr() && (m_apMac->GetGcrManager()->GetRetransmissionPolicy() ==
                                       GroupAddressRetransmissionPolicy::GCR_BLOCK_ACK);
                agreement && reqHdr.IsGcr() && !isGcrBa)
            {
                NS_LOG_DEBUG("Skip GCR BAR if GCR-BA retransmission policy is not selected");
                queue->Remove(bar);
                continue;
            }
            else if (!agreement)
            {
                NS_LOG_DEBUG("BA agreement with " << recipient << " for TID=" << +tid
                                                  << " was torn down");
                queue->Remove(bar);
                continue;
            }
            // update BAR if the starting sequence number changed
            if (auto seqNo = agreement->get().GetStartingSequence();
                reqHdr.GetStartingSequence() != seqNo)
            {
                reqHdr.SetStartingSequence(seqNo);
                Ptr<Packet> packet = Create<Packet>();
                packet->AddHeader(reqHdr);
                auto updatedBar = Create<WifiMpdu>(packet, bar->GetHeader(), bar->GetTimestamp());
                queue->Replace(bar, updatedBar);
                bar = updatedBar;
            }
            // bar is the BlockAckReq to send
            selectedBar = bar;

            // if the selected BAR is intended to be sent on this specific link and the recipient
            // is an MLD, remove the BAR (if any) for this BA agreement that can be sent on any
            // link (because a BAR that can be sent on any link to a recipient is no longer
            // needed after sending a BAR to that recipient on this link)
            if (bar->GetHeader().GetAddr2() == m_self && recipientMld)
            {
                WifiContainerQueueId queueId{WIFI_CTL_QUEUE,
                                             WIFI_UNICAST,
                                             *recipientMld,
                                             std::nullopt};
                Ptr<WifiMpdu> otherBar;
                while ((otherBar = queue->PeekByQueueId(queueId, otherBar)))
                {
                    if (otherBar->GetHeader().IsBlockAckReq())
                    {
                        CtrlBAckRequestHeader otherReqHdr;
                        otherBar->GetPacket()->PeekHeader(otherReqHdr);
                        if (otherReqHdr.GetTidInfo() == tid)
                        {
                            queue->Remove(otherBar);
                            break;
                        }
                    }
                }
            }
            break;
        }
        if (bar->GetHeader().IsTrigger() && !optAddress && !selectedBar)
        {
            return bar;
        }
        // not a BAR nor a Trigger Frame, continue
        prevBar = bar;
    }

    if (!selectedBar)
    {
        // check if we can send a BAR to a recipient to which a BAR can only be sent if data queued
        auto baManager = m_mac->GetQosTxop(ac)->GetBaManager();
        for (const auto& [recipient, tid] : baManager->GetSendBarIfDataQueuedList())
        {
            WifiContainerQueueId queueId(
                WIFI_QOSDATA_QUEUE,
                WIFI_UNICAST,
                GetWifiRemoteStationManager()->GetMldAddress(recipient).value_or(recipient),
                tid);
            // check if data is queued and can be transmitted on this link
            if (queue->PeekByTidAndAddress(tid, recipient) &&
                !m_mac->GetTxBlockedOnLink(QosUtilsMapTidToAc(tid), queueId, m_linkId))
            {
                auto [reqHdr, hdr] = m_mac->GetQosTxop(ac)->PrepareBlockAckRequest(recipient, tid);
                auto pkt = Create<Packet>();
                pkt->AddHeader(reqHdr);
                selectedBar = Create<WifiMpdu>(pkt, hdr);
                baManager->RemoveFromSendBarIfDataQueuedList(recipient, tid);
                queue->Enqueue(selectedBar);
                break;
            }
        }
    }

    if (selectedBar)
    {
        if (const auto currAddr1 = selectedBar->GetHeader().GetAddr1();
            currAddr1 == m_mac->GetMldAddress(currAddr1))
        {
            // the selected BAR has MLD addresses in Addr1/Addr2, replace them with link addresses
            // and move to the appropriate container queue
            DequeueMpdu(selectedBar);
            const auto addr1 =
                GetWifiRemoteStationManager()->GetAffiliatedStaAddress(currAddr1).value_or(
                    currAddr1);
            selectedBar->GetHeader().SetAddr1(addr1);
            selectedBar->GetHeader().SetAddr2(m_self);
            queue->Enqueue(selectedBar);
        }
    }

    return selectedBar;
}

bool
HtFrameExchangeManager::SendMpduFromBaManager(Ptr<WifiMpdu> mpdu,
                                              Time availableTime,
                                              bool initialFrame)
{
    NS_LOG_FUNCTION(this << *mpdu << availableTime << initialFrame);

    // First, check if there is a BAR to be transmitted
    if (!mpdu->GetHeader().IsBlockAckReq())
    {
        NS_LOG_DEBUG("Block Ack Manager returned no frame to send");
        return false;
    }

    // Prepare the TX parameters. Note that the default ack manager expects the
    // data TxVector in the m_txVector field to compute the BlockAck TxVector.
    // The m_txVector field of the TX parameters is set to the BlockAckReq TxVector
    // a few lines below.
    WifiTxParameters txParams;
    txParams.m_txVector =
        GetWifiRemoteStationManager()->GetDataTxVector(mpdu->GetHeader(), m_allowedWidth);

    if (!TryAddMpdu(mpdu, txParams, availableTime))
    {
        NS_LOG_DEBUG("Not enough time to send the BAR frame returned by the Block Ack Manager");
        return false;
    }

    NS_ABORT_IF(txParams.m_acknowledgment->method != WifiAcknowledgment::BLOCK_ACK);

    // the BlockAckReq frame is sent using the same TXVECTOR as the BlockAck frame
    auto blockAcknowledgment = static_cast<WifiBlockAck*>(txParams.m_acknowledgment.get());
    txParams.m_txVector = blockAcknowledgment->blockAckTxVector;

    // we can transmit the BlockAckReq frame
    SendPsduWithProtection(GetWifiPsdu(mpdu, txParams.m_txVector), txParams);
    return true;
}

bool
HtFrameExchangeManager::SendDataFrame(Ptr<WifiMpdu> peekedItem,
                                      Time availableTime,
                                      bool initialFrame)
{
    NS_ASSERT(peekedItem && peekedItem->GetHeader().IsQosData() &&
              !peekedItem->GetHeader().GetAddr1().IsBroadcast() && !peekedItem->IsFragment());
    NS_LOG_FUNCTION(this << *peekedItem << availableTime << initialFrame);

    Ptr<QosTxop> edca = m_mac->GetQosTxop(peekedItem->GetHeader().GetQosTid());
    WifiTxParameters txParams;
    txParams.m_txVector =
        GetWifiRemoteStationManager()->GetDataTxVector(peekedItem->GetHeader(), m_allowedWidth);
    Ptr<WifiMpdu> mpdu =
        edca->GetNextMpdu(m_linkId, peekedItem, txParams, availableTime, initialFrame);

    if (!mpdu)
    {
        NS_LOG_DEBUG("Not enough time to transmit a frame");
        return false;
    }

    // try A-MPDU aggregation
    std::vector<Ptr<WifiMpdu>> mpduList =
        m_mpduAggregator->GetNextAmpdu(mpdu, txParams, availableTime);
    NS_ASSERT(txParams.m_acknowledgment);

    if (mpduList.size() > 1)
    {
        // A-MPDU aggregation succeeded
        SendPsduWithProtection(Create<WifiPsdu>(std::move(mpduList)), txParams);
    }
    else if (txParams.m_acknowledgment->method == WifiAcknowledgment::BAR_BLOCK_ACK)
    {
        // a QoS data frame using the Block Ack policy can be followed by a BlockAckReq
        // frame and a BlockAck frame. Such a sequence is handled by the HT FEM
        SendPsduWithProtection(GetWifiPsdu(mpdu, txParams.m_txVector), txParams);
    }
    else
    {
        // transmission can be handled by the base FEM
        SendMpduWithProtection(mpdu, txParams);
    }

    return true;
}

void
HtFrameExchangeManager::CalculateAcknowledgmentTime(WifiAcknowledgment* acknowledgment) const
{
    NS_LOG_FUNCTION(this << acknowledgment);
    NS_ASSERT(acknowledgment);

    if (acknowledgment->method == WifiAcknowledgment::BLOCK_ACK)
    {
        auto blockAcknowledgment = static_cast<WifiBlockAck*>(acknowledgment);
        auto baTxDuration =
            WifiPhy::CalculateTxDuration(GetBlockAckSize(blockAcknowledgment->baType),
                                         blockAcknowledgment->blockAckTxVector,
                                         m_phy->GetPhyBand());
        blockAcknowledgment->acknowledgmentTime = m_phy->GetSifs() + baTxDuration;
    }
    else if (acknowledgment->method == WifiAcknowledgment::BAR_BLOCK_ACK)
    {
        auto barBlockAcknowledgment = static_cast<WifiBarBlockAck*>(acknowledgment);
        auto barTxDuration =
            WifiPhy::CalculateTxDuration(GetBlockAckRequestSize(barBlockAcknowledgment->barType),
                                         barBlockAcknowledgment->blockAckReqTxVector,
                                         m_phy->GetPhyBand());
        auto baTxDuration =
            WifiPhy::CalculateTxDuration(GetBlockAckSize(barBlockAcknowledgment->baType),
                                         barBlockAcknowledgment->blockAckTxVector,
                                         m_phy->GetPhyBand());
        barBlockAcknowledgment->acknowledgmentTime =
            2 * m_phy->GetSifs() + barTxDuration + baTxDuration;
    }
    else
    {
        QosFrameExchangeManager::CalculateAcknowledgmentTime(acknowledgment);
    }
}

void
HtFrameExchangeManager::ForwardMpduDown(Ptr<WifiMpdu> mpdu, WifiTxVector& txVector)
{
    ForwardPsduDown(GetWifiPsdu(mpdu, txVector), txVector);
}

Ptr<WifiPsdu>
HtFrameExchangeManager::GetWifiPsdu(Ptr<WifiMpdu> mpdu, const WifiTxVector& txVector) const
{
    return Create<WifiPsdu>(mpdu, false);
}

void
HtFrameExchangeManager::NotifyReceivedNormalAck(Ptr<WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << *mpdu);

    if (mpdu->GetHeader().IsQosData())
    {
        uint8_t tid = mpdu->GetHeader().GetQosTid();
        Ptr<QosTxop> edca = m_mac->GetQosTxop(tid);

        if (m_mac->GetBaAgreementEstablishedAsOriginator(mpdu->GetHeader().GetAddr1(), tid))
        {
            // notify the BA manager that the MPDU was acknowledged
            edca->GetBaManager()->NotifyGotAck(m_linkId, mpdu);
            // the BA manager fires the AckedMpdu trace source, so nothing else must be done
            return;
        }
    }
    else if (mpdu->GetHeader().IsAction())
    {
        auto addr1 = mpdu->GetHeader().GetAddr1();
        auto address = GetWifiRemoteStationManager()->GetMldAddress(addr1).value_or(addr1);
        WifiActionHeader actionHdr;
        Ptr<Packet> p = mpdu->GetPacket()->Copy();
        p->RemoveHeader(actionHdr);
        if (actionHdr.GetCategory() == WifiActionHeader::BLOCK_ACK)
        {
            if (actionHdr.GetAction().blockAck == WifiActionHeader::BLOCK_ACK_DELBA)
            {
                MgtDelBaHeader delBa;
                p->PeekHeader(delBa);
                auto tid = delBa.GetTid();
                if (delBa.IsByOriginator())
                {
                    GetBaManager(tid)->DestroyOriginatorAgreement(address,
                                                                  tid,
                                                                  delBa.GetGcrGroupAddress());
                }
                else
                {
                    GetBaManager(tid)->DestroyRecipientAgreement(address,
                                                                 tid,
                                                                 delBa.GetGcrGroupAddress());
                }
            }
            else if (actionHdr.GetAction().blockAck == WifiActionHeader::BLOCK_ACK_ADDBA_REQUEST)
            {
                // Setup ADDBA response timeout
                MgtAddBaRequestHeader addBa;
                p->PeekHeader(addBa);
                Ptr<QosTxop> edca = m_mac->GetQosTxop(addBa.GetTid());
                Simulator::Schedule(edca->GetAddBaResponseTimeout(),
                                    &QosTxop::AddBaResponseTimeout,
                                    edca,
                                    address,
                                    addBa.GetTid(),
                                    addBa.GetGcrGroupAddress());
            }
            else if (actionHdr.GetAction().blockAck == WifiActionHeader::BLOCK_ACK_ADDBA_RESPONSE)
            {
                // A recipient Block Ack agreement must exist
                MgtAddBaResponseHeader addBa;
                p->PeekHeader(addBa);
                auto tid = addBa.GetTid();
                NS_ASSERT_MSG(
                    GetBaManager(tid)->GetAgreementAsRecipient(address,
                                                               tid,
                                                               addBa.GetGcrGroupAddress()),
                    "Recipient BA agreement {" << address << ", " << +tid << "} not found");
                m_pendingAddBaResp.erase({address, tid});
            }
        }
    }
    QosFrameExchangeManager::NotifyReceivedNormalAck(mpdu);
}

void
HtFrameExchangeManager::TransmissionSucceeded()
{
    NS_LOG_DEBUG(this);

    if (m_edca && m_edca->GetTxopLimit(m_linkId).IsZero() && GetBar(m_edca->GetAccessCategory()) &&
        (m_txNav > Simulator::Now() + m_phy->GetSifs()))
    {
        // A TXOP limit of 0 indicates that the TXOP holder may transmit or cause to
        // be transmitted (as responses) the following within the current TXOP:
        // f) Any number of BlockAckReq frames
        // (Sec. 10.22.2.8 of 802.11-2016)
        NS_LOG_DEBUG("Schedule a transmission from Block Ack Manager in a SIFS");
        bool (HtFrameExchangeManager::*fp)(Ptr<QosTxop>, Time) =
            &HtFrameExchangeManager::StartTransmission;

        // TXOP limit is null, hence the txopDuration parameter is unused
        Simulator::Schedule(m_phy->GetSifs(), fp, this, m_edca, Seconds(0));

        if (m_protectedIfResponded)
        {
            m_protectedStas.merge(m_sentFrameTo);
        }
        m_sentFrameTo.clear();
    }
    else
    {
        QosFrameExchangeManager::TransmissionSucceeded();
    }
}

void
HtFrameExchangeManager::NotifyPacketDiscarded(Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << *mpdu);

    if (mpdu->GetHeader().IsQosData())
    {
        GetBaManager(mpdu->GetHeader().GetQosTid())->NotifyDiscardedMpdu(mpdu);
    }
    else if (mpdu->GetHeader().IsAction())
    {
        WifiActionHeader actionHdr;
        mpdu->GetPacket()->PeekHeader(actionHdr);
        if (actionHdr.GetCategory() == WifiActionHeader::BLOCK_ACK &&
            actionHdr.GetAction().blockAck == WifiActionHeader::BLOCK_ACK_ADDBA_REQUEST)
        {
            const auto tid = GetTid(mpdu->GetPacket(), mpdu->GetHeader());
            auto recipient = mpdu->GetHeader().GetAddr1();
            // if the recipient is an MLD, use its MLD address
            if (auto mldAddr = GetWifiRemoteStationManager()->GetMldAddress(recipient))
            {
                recipient = *mldAddr;
            }
            auto p = mpdu->GetPacket()->Copy();
            p->RemoveHeader(actionHdr);
            MgtAddBaRequestHeader addBa;
            p->PeekHeader(addBa);
            if (auto agreement =
                    GetBaManager(tid)->GetAgreementAsOriginator(recipient,
                                                                tid,
                                                                addBa.GetGcrGroupAddress());
                agreement && agreement->get().IsPending())
            {
                NS_LOG_DEBUG("No ACK after ADDBA request");
                Ptr<QosTxop> qosTxop = m_mac->GetQosTxop(tid);
                qosTxop->NotifyOriginatorAgreementNoReply(recipient,
                                                          tid,
                                                          addBa.GetGcrGroupAddress());
                Simulator::Schedule(qosTxop->GetFailedAddBaTimeout(),
                                    &QosTxop::ResetBa,
                                    qosTxop,
                                    recipient,
                                    tid,
                                    addBa.GetGcrGroupAddress());
            }
        }
    }
    // the MPDU may have been dropped (and dequeued) by the above call to the NotifyDiscardedMpdu
    // method of the BlockAckManager with reason WIFI_MAC_DROP_QOS_OLD_PACKET; in such a case, we
    // must not fire the dropped callback again (with reason WIFI_MAC_DROP_REACHED_RETRY_LIMIT)
    if (mpdu->IsQueued())
    {
        QosFrameExchangeManager::NotifyPacketDiscarded(mpdu);
    }
}

void
HtFrameExchangeManager::RetransmitMpduAfterMissedAck(Ptr<WifiMpdu> mpdu) const
{
    NS_LOG_FUNCTION(this << *mpdu);

    if (mpdu->GetHeader().IsQosData())
    {
        uint8_t tid = mpdu->GetHeader().GetQosTid();
        Ptr<QosTxop> edca = m_mac->GetQosTxop(tid);

        if (m_mac->GetBaAgreementEstablishedAsOriginator(mpdu->GetHeader().GetAddr1(), tid))
        {
            // notify the BA manager that the MPDU was not acknowledged
            edca->GetBaManager()->NotifyMissedAck(m_linkId, mpdu);
            return;
        }
    }
    QosFrameExchangeManager::RetransmitMpduAfterMissedAck(mpdu);
}

void
HtFrameExchangeManager::ReleaseSequenceNumbers(Ptr<const WifiPsdu> psdu) const
{
    NS_LOG_FUNCTION(this << *psdu);

    const auto tids = psdu->GetTids();
    const auto isGcr = IsGcr(m_mac, psdu->GetHeader(0));
    auto agreementEstablished =
        !tids.empty() /* no QoS data frame included */ &&
        (isGcr ? GetBaManager(*tids.begin())
                     ->IsGcrAgreementEstablished(
                         psdu->GetHeader(0).GetAddr1(),
                         *tids.begin(),
                         m_apMac->GetGcrManager()->GetMemberStasForGroupAddress(
                             psdu->GetHeader(0).GetAddr1()))
               : m_mac->GetBaAgreementEstablishedAsOriginator(psdu->GetAddr1(), *tids.begin())
                     .has_value());

    if (!agreementEstablished)
    {
        QosFrameExchangeManager::ReleaseSequenceNumbers(psdu);
        return;
    }

    // iterate over MPDUs in reverse order (to process them in decreasing order of sequence number)
    auto mpduIt = psdu->end();

    do
    {
        std::advance(mpduIt, -1);

        const WifiMacHeader& hdr = (*mpduIt)->GetOriginal()->GetHeader();
        if (hdr.IsQosData())
        {
            uint8_t tid = hdr.GetQosTid();
            agreementEstablished =
                isGcr ? GetBaManager(tid)->IsGcrAgreementEstablished(
                            psdu->GetHeader(0).GetAddr1(),
                            tid,
                            m_apMac->GetGcrManager()->GetMemberStasForGroupAddress(
                                psdu->GetHeader(0).GetAddr1()))
                      : m_mac->GetBaAgreementEstablishedAsOriginator(psdu->GetAddr1(), tid)
                            .has_value();
            NS_ASSERT(agreementEstablished);

            if (!hdr.IsRetry() && !(*mpduIt)->IsInFlight())
            {
                // The MPDU has never been transmitted, so we can make its sequence
                // number available again if it is the highest sequence number
                // assigned by the MAC TX middle
                uint16_t currentNextSeq = m_txMiddle->PeekNextSequenceNumberFor(&hdr);

                if ((hdr.GetSequenceNumber() + 1) % SEQNO_SPACE_SIZE == currentNextSeq)
                {
                    (*mpduIt)->UnassignSeqNo();
                    m_txMiddle->SetSequenceNumberFor(&hdr);

                    NS_LOG_DEBUG("Released " << hdr.GetSequenceNumber()
                                             << ", next sequence "
                                                "number for dest="
                                             << hdr.GetAddr1() << ",tid=" << +tid << " is "
                                             << m_txMiddle->PeekNextSequenceNumberFor(&hdr));
                }
            }
        }
    } while (mpduIt != psdu->begin());
}

Time
HtFrameExchangeManager::GetPsduDurationId(Time txDuration, const WifiTxParameters& txParams) const
{
    NS_LOG_FUNCTION(this << txDuration << &txParams);

    NS_ASSERT(m_edca);
    NS_ASSERT(txParams.m_acknowledgment &&
              txParams.m_acknowledgment->acknowledgmentTime.has_value());

    const auto singleDurationId = *txParams.m_acknowledgment->acknowledgmentTime;

    if (m_edca->GetTxopLimit(m_linkId).IsZero())
    {
        return singleDurationId;
    }

    // under multiple protection settings, if the TXOP limit is not null, Duration/ID
    // is set to cover the remaining TXOP time (Sec. 9.2.5.2 of 802.11-2016).
    // The TXOP holder may exceed the TXOP limit in some situations (Sec. 10.22.2.8
    // of 802.11-2016)
    auto duration = std::max(m_edca->GetRemainingTxop(m_linkId) - txDuration, Seconds(0));

    if (m_protectSingleExchange)
    {
        duration = std::min(duration, singleDurationId + m_singleExchangeProtectionSurplus);
    }

    return duration;
}

void
HtFrameExchangeManager::SendPsduWithProtection(Ptr<WifiPsdu> psdu, WifiTxParameters& txParams)
{
    NS_LOG_FUNCTION(this << psdu << &txParams);

    m_psdu = psdu;
    m_txParams = std::move(txParams);

#ifdef NS3_BUILD_PROFILE_DEBUG
    // If protection is required, the MPDUs must be stored in some queue because
    // they are not put back in a queue if the RTS/CTS exchange fails
    if (m_txParams.m_protection->method != WifiProtection::NONE)
    {
        for (const auto& mpdu : *PeekPointer(m_psdu))
        {
            NS_ASSERT(mpdu->GetHeader().IsCtl() || mpdu->IsQueued());
        }
    }
#endif

    // Make sure that the acknowledgment time has been computed, so that SendRts()
    // and SendCtsToSelf() can reuse this value.
    NS_ASSERT(m_txParams.m_acknowledgment);

    if (!m_txParams.m_acknowledgment->acknowledgmentTime.has_value())
    {
        CalculateAcknowledgmentTime(m_txParams.m_acknowledgment.get());
    }

    // Set QoS Ack policy
    WifiAckManager::SetQosAckPolicy(m_psdu, m_txParams.m_acknowledgment.get());

    for (const auto& mpdu : *PeekPointer(m_psdu))
    {
        if (mpdu->IsQueued())
        {
            mpdu->SetInFlight(m_linkId);
        }
    }

    StartProtection(m_txParams);
}

void
HtFrameExchangeManager::ProtectionCompleted()
{
    NS_LOG_FUNCTION(this);
    if (m_psdu)
    {
        m_protectedStas.merge(m_sentRtsTo);
        m_sentRtsTo.clear();
        if (m_txParams.m_protection->method == WifiProtection::NONE)
        {
            SendPsdu();
        }
        else
        {
            Simulator::Schedule(m_phy->GetSifs(), &HtFrameExchangeManager::SendPsdu, this);
        }
        return;
    }
    QosFrameExchangeManager::ProtectionCompleted();
}

void
HtFrameExchangeManager::CtsTimeout(Ptr<WifiMpdu> rts, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << *rts << txVector);

    if (!m_psdu)
    {
        // A CTS Timeout occurred when protecting a single MPDU is handled by the
        // parent classes
        QosFrameExchangeManager::CtsTimeout(rts, txVector);
        return;
    }

    DoCtsTimeout(WifiPsduMap{{SU_STA_ID, m_psdu}});
    m_psdu = nullptr;
}

void
HtFrameExchangeManager::SendPsdu()
{
    NS_LOG_FUNCTION(this);

    Time txDuration =
        WifiPhy::CalculateTxDuration(m_psdu->GetSize(), m_txParams.m_txVector, m_phy->GetPhyBand());

    NS_ASSERT(m_txParams.m_acknowledgment);

    if (m_txParams.m_acknowledgment->method == WifiAcknowledgment::NONE)
    {
        std::set<uint8_t> tids = m_psdu->GetTids();
        NS_ASSERT_MSG(tids.size() <= 1, "Multi-TID A-MPDUs are not supported");

        if (m_mac->GetTypeOfStation() == AP && m_apMac->UseGcr(m_psdu->GetHeader(0)))
        {
            if (m_apMac->GetGcrManager()->KeepGroupcastQueued(*m_psdu->begin()))
            {
                // keep the groupcast frame in the queue for future retransmission
                Simulator::Schedule(txDuration + m_phy->GetSifs(), [=, this, psdu = m_psdu]() {
                    NS_LOG_DEBUG("Prepare groupcast PSDU for retry");
                    for (const auto& mpdu : *PeekPointer(psdu))
                    {
                        mpdu->ResetInFlight(m_linkId);
                        // restore addr1 to the group address instead of the concealment address
                        if (m_apMac->GetGcrManager()->UseConcealment(mpdu->GetHeader()))
                        {
                            mpdu->GetHeader().SetAddr1(mpdu->begin()->second.GetDestinationAddr());
                        }
                        mpdu->GetHeader().SetRetry();
                    }
                });
            }
            else
            {
                if (m_apMac->GetGcrManager()->GetRetransmissionPolicy() ==
                    GroupAddressRetransmissionPolicy::GCR_UNSOLICITED_RETRY)
                {
                    for (const auto& mpdu : *PeekPointer(m_psdu))
                    {
                        NotifyLastGcrUrTx(mpdu);
                    }
                }
                DequeuePsdu(m_psdu);
            }
        }
        else if (tids.empty() || m_psdu->GetAckPolicyForTid(*tids.begin()) == WifiMacHeader::NO_ACK)
        {
            // No acknowledgment, hence dequeue the PSDU if it is stored in a queue
            DequeuePsdu(m_psdu);
        }

        Simulator::Schedule(txDuration, [=, this]() {
            TransmissionSucceeded();
            m_psdu = nullptr;
        });
    }
    else if (m_txParams.m_acknowledgment->method == WifiAcknowledgment::BLOCK_ACK)
    {
        m_psdu->SetDuration(GetPsduDurationId(txDuration, m_txParams));

        // the timeout duration is "aSIFSTime + aSlotTime + aRxPHYStartDelay, starting
        // at the PHY-TXEND.confirm primitive" (section 10.3.2.9 or 10.22.2.2 of 802.11-2016).
        // aRxPHYStartDelay equals the time to transmit the PHY header.
        auto blockAcknowledgment = static_cast<WifiBlockAck*>(m_txParams.m_acknowledgment.get());

        Time timeout =
            txDuration + m_phy->GetSifs() + m_phy->GetSlot() +
            WifiPhy::CalculatePhyPreambleAndHeaderDuration(blockAcknowledgment->blockAckTxVector);
        NS_ASSERT(!m_txTimer.IsRunning());
        m_txTimer.Set(WifiTxTimer::WAIT_BLOCK_ACK,
                      timeout,
                      {m_psdu->GetAddr1()},
                      &HtFrameExchangeManager::BlockAckTimeout,
                      this,
                      m_psdu,
                      m_txParams.m_txVector);
        m_channelAccessManager->NotifyAckTimeoutStartNow(timeout);
    }
    else if (m_txParams.m_acknowledgment->method == WifiAcknowledgment::BAR_BLOCK_ACK)
    {
        m_psdu->SetDuration(GetPsduDurationId(txDuration, m_txParams));

        // schedule the transmission of a BAR in a SIFS
        const auto tids = m_psdu->GetTids();
        NS_ABORT_MSG_IF(tids.size() > 1,
                        "Acknowledgment method incompatible with a Multi-TID A-MPDU");
        const auto tid = *tids.begin();

        auto edca = m_mac->GetQosTxop(tid);
        const auto isGcr = IsGcr(m_mac, m_psdu->GetHeader(0));
        const auto& recipients =
            isGcr ? m_apMac->GetGcrManager()->GetMemberStasForGroupAddress(m_psdu->GetAddr1())
                  : GcrManager::GcrMembers{m_psdu->GetAddr1()};
        std::optional<Mac48Address> gcrGroupAddress{isGcr ? std::optional{m_psdu->GetAddr1()}
                                                          : std::nullopt};
        for (const auto& recipient : recipients)
        {
            auto [reqHdr, hdr] = edca->PrepareBlockAckRequest(recipient, tid, gcrGroupAddress);
            GetBaManager(tid)->ScheduleBar(reqHdr, hdr);
        }

        if (isGcr)
        {
            Simulator::Schedule(txDuration + m_phy->GetSifs(), [=, this, psdu = m_psdu]() {
                NS_LOG_DEBUG("Restore group address of PSDU");
                for (const auto& mpdu : *PeekPointer(psdu))
                {
                    // restore addr1 to the group address instead of the concealment address
                    if (m_apMac->GetGcrManager()->UseConcealment(mpdu->GetHeader()))
                    {
                        mpdu->GetHeader().SetAddr1(mpdu->begin()->second.GetDestinationAddr());
                    }
                }
            });
        }

        Simulator::Schedule(txDuration, [=, this]() {
            TransmissionSucceeded();
            m_psdu = nullptr;
        });
    }
    else
    {
        NS_ABORT_MSG("Unable to handle the selected acknowledgment method ("
                     << m_txParams.m_acknowledgment.get() << ")");
    }

    // transmit the PSDU
    if (m_psdu->GetNMpdus() > 1)
    {
        ForwardPsduDown(m_psdu, m_txParams.m_txVector);
    }
    else
    {
        ForwardMpduDown(*m_psdu->begin(), m_txParams.m_txVector);
    }

    if (m_txTimer.IsRunning())
    {
        NS_ASSERT(m_sentFrameTo.empty());
        m_sentFrameTo = {m_psdu->GetAddr1()};
    }

    if (m_txParams.m_acknowledgment->method == WifiAcknowledgment::NONE)
    {
        // we are done in case the A-MPDU does not require acknowledgment
        m_psdu = nullptr;
    }
}

void
HtFrameExchangeManager::NotifyTxToEdca(Ptr<const WifiPsdu> psdu) const
{
    NS_LOG_FUNCTION(this << psdu);

    for (const auto& mpdu : *PeekPointer(psdu))
    {
        auto& hdr = mpdu->GetHeader();

        if (hdr.IsQosData() && hdr.HasData())
        {
            auto tid = hdr.GetQosTid();
            m_mac->GetQosTxop(tid)->CompleteMpduTx(mpdu);
        }
    }
}

void
HtFrameExchangeManager::FinalizeMacHeader(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << psdu);

    // use an array to avoid computing the queue size for every MPDU in the PSDU
    std::array<std::optional<uint8_t>, 8> queueSizeForTid;

    for (const auto& mpdu : *PeekPointer(psdu))
    {
        WifiMacHeader& hdr = mpdu->GetHeader();

        if (hdr.IsQosData())
        {
            uint8_t tid = hdr.GetQosTid();
            auto edca = m_mac->GetQosTxop(tid);

            if (m_mac->GetTypeOfStation() == STA && (m_setQosQueueSize || hdr.IsQosEosp()))
            {
                // set the Queue Size subfield of the QoS Control field
                if (!queueSizeForTid[tid].has_value())
                {
                    queueSizeForTid[tid] =
                        edca->GetQosQueueSize(tid, mpdu->GetOriginal()->GetHeader().GetAddr1());
                }

                hdr.SetQosEosp();
                hdr.SetQosQueueSize(queueSizeForTid[tid].value());
            }

            if (m_mac->GetTypeOfStation() == AP && m_apMac->UseGcr(hdr) &&
                m_apMac->GetGcrManager()->UseConcealment(mpdu->GetHeader()))
            {
                const auto& gcrConcealmentAddress =
                    m_apMac->GetGcrManager()->GetGcrConcealmentAddress();
                hdr.SetAddr1(gcrConcealmentAddress);
            }
        }
    }

    QosFrameExchangeManager::FinalizeMacHeader(psdu);
}

void
HtFrameExchangeManager::DequeuePsdu(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << *psdu);
    for (const auto& mpdu : *PeekPointer(psdu))
    {
        DequeueMpdu(mpdu);
    }
}

void
HtFrameExchangeManager::ForwardPsduDown(Ptr<const WifiPsdu> psdu, WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psdu << txVector);

    NS_LOG_DEBUG("Transmitting a PSDU: " << *psdu << " TXVECTOR: " << txVector);
    FinalizeMacHeader(psdu);
    NotifyTxToEdca(psdu);
    m_allowedWidth = std::min(m_allowedWidth, txVector.GetChannelWidth());

    if (psdu->IsAggregate())
    {
        txVector.SetAggregation(true);
    }

    const auto txDuration = WifiPhy::CalculateTxDuration(psdu, txVector, m_phy->GetPhyBand());
    SetTxNav(*psdu->begin(), txDuration);

    m_phy->Send(psdu, txVector);
}

bool
HtFrameExchangeManager::IsWithinLimitsIfAddMpdu(Ptr<const WifiMpdu> mpdu,
                                                const WifiTxParameters& txParams,
                                                Time ppduDurationLimit) const
{
    NS_ASSERT(mpdu);
    NS_LOG_FUNCTION(this << *mpdu << &txParams << ppduDurationLimit);

    Mac48Address receiver = mpdu->GetHeader().GetAddr1();
    uint32_t ampduSize = txParams.GetSize(receiver);

    if (!txParams.LastAddedIsFirstMpdu(receiver))
    {
        // we are attempting to perform A-MPDU aggregation, hence we have to check
        // that we meet the limit on the max A-MPDU size
        uint8_t tid;
        const WifiTxParameters::PsduInfo* info;

        if (mpdu->GetHeader().IsQosData())
        {
            tid = mpdu->GetHeader().GetQosTid();
        }
        else if ((info = txParams.GetPsduInfo(receiver)) && !info->seqNumbers.empty())
        {
            tid = info->seqNumbers.begin()->first;
        }
        else
        {
            NS_ABORT_MSG("Cannot aggregate a non-QoS data frame to an A-MPDU that does"
                         " not contain any QoS data frame");
        }

        WifiModulationClass modulation = txParams.m_txVector.GetModulationClass();

        if (!IsWithinAmpduSizeLimit(ampduSize, receiver, tid, modulation))
        {
            return false;
        }
    }

    return IsWithinSizeAndTimeLimits(ampduSize, receiver, txParams, ppduDurationLimit);
}

bool
HtFrameExchangeManager::IsWithinAmpduSizeLimit(uint32_t ampduSize,
                                               Mac48Address receiver,
                                               uint8_t tid,
                                               WifiModulationClass modulation) const
{
    NS_LOG_FUNCTION(this << ampduSize << receiver << +tid << modulation);

    uint32_t maxAmpduSize = m_mpduAggregator->GetMaxAmpduSize(receiver, tid, modulation);

    if (maxAmpduSize == 0)
    {
        NS_LOG_DEBUG("A-MPDU aggregation disabled");
        return false;
    }

    if (ampduSize > maxAmpduSize)
    {
        NS_LOG_DEBUG("the frame does not meet the constraint on max A-MPDU size (" << maxAmpduSize
                                                                                   << ")");
        return false;
    }
    return true;
}

bool
HtFrameExchangeManager::TryAggregateMsdu(Ptr<const WifiMpdu> msdu,
                                         WifiTxParameters& txParams,
                                         Time availableTime) const
{
    NS_ASSERT(msdu && msdu->GetHeader().IsQosData());
    NS_LOG_FUNCTION(this << *msdu << &txParams << availableTime);

    // tentatively aggregate the given MPDU
    auto prevTxDuration = txParams.m_txDuration;
    txParams.AggregateMsdu(msdu);
    UpdateTxDuration(msdu->GetHeader().GetAddr1(), txParams);

    // check if aggregating the given MSDU requires a different protection method
    NS_ASSERT(txParams.m_protection);
    auto protectionTime = txParams.m_protection->protectionTime;

    std::unique_ptr<WifiProtection> protection;
    protection = GetProtectionManager()->TryAggregateMsdu(msdu, txParams);
    bool protectionSwapped = false;

    if (protection)
    {
        // the protection method has changed, calculate the new protection time
        CalculateProtectionTime(protection.get());
        protectionTime = protection->protectionTime;
        // swap unique pointers, so that the txParams that is passed to the next
        // call to IsWithinLimitsIfAggregateMsdu is the most updated one
        txParams.m_protection.swap(protection);
        protectionSwapped = true;
    }
    NS_ASSERT(protectionTime.has_value());

    // check if aggregating the given MSDU requires a different acknowledgment method
    NS_ASSERT(txParams.m_acknowledgment);
    auto acknowledgmentTime = txParams.m_acknowledgment->acknowledgmentTime;

    std::unique_ptr<WifiAcknowledgment> acknowledgment;
    acknowledgment = GetAckManager()->TryAggregateMsdu(msdu, txParams);
    bool acknowledgmentSwapped = false;

    if (acknowledgment)
    {
        // the acknowledgment method has changed, calculate the new acknowledgment time
        CalculateAcknowledgmentTime(acknowledgment.get());
        acknowledgmentTime = acknowledgment->acknowledgmentTime;
        // swap unique pointers, so that the txParams that is passed to the next
        // call to IsWithinLimitsIfAggregateMsdu is the most updated one
        txParams.m_acknowledgment.swap(acknowledgment);
        acknowledgmentSwapped = true;
    }
    NS_ASSERT(acknowledgmentTime.has_value());

    Time ppduDurationLimit = Time::Min();
    if (availableTime != Time::Min())
    {
        ppduDurationLimit = availableTime - *protectionTime - *acknowledgmentTime;
    }

    if (!IsWithinLimitsIfAggregateMsdu(msdu, txParams, ppduDurationLimit))
    {
        // adding MPDU failed, undo the addition of the MPDU and restore protection and
        // acknowledgment methods if they were swapped
        txParams.UndoAddMpdu();
        txParams.m_txDuration = prevTxDuration;
        if (protectionSwapped)
        {
            txParams.m_protection.swap(protection);
        }
        if (acknowledgmentSwapped)
        {
            txParams.m_acknowledgment.swap(acknowledgment);
        }
        return false;
    }

    return true;
}

bool
HtFrameExchangeManager::IsWithinLimitsIfAggregateMsdu(Ptr<const WifiMpdu> msdu,
                                                      const WifiTxParameters& txParams,
                                                      Time ppduDurationLimit) const
{
    NS_ASSERT(msdu && msdu->GetHeader().IsQosData());
    NS_LOG_FUNCTION(this << *msdu << &txParams << ppduDurationLimit);

    auto receiver = msdu->GetHeader().GetAddr1();
    auto tid = msdu->GetHeader().GetQosTid();
    auto modulation = txParams.m_txVector.GetModulationClass();
    auto psduInfo = txParams.GetPsduInfo(receiver);
    NS_ASSERT_MSG(psduInfo, "No PSDU info for receiver " << receiver);

    // Check that the limit on A-MSDU size is met
    uint16_t maxAmsduSize = m_msduAggregator->GetMaxAmsduSize(receiver, tid, modulation);

    if (maxAmsduSize == 0)
    {
        NS_LOG_DEBUG("A-MSDU aggregation disabled");
        return false;
    }

    if (psduInfo->amsduSize > maxAmsduSize)
    {
        NS_LOG_DEBUG("No other MSDU can be aggregated: maximum A-MSDU size (" << maxAmsduSize
                                                                              << ") reached ");
        return false;
    }

    const WifiTxParameters::PsduInfo* info = txParams.GetPsduInfo(msdu->GetHeader().GetAddr1());
    NS_ASSERT(info);
    auto ampduSize = txParams.GetSize(receiver);

    if (info->ampduSize > 0)
    {
        // the A-MSDU being built is aggregated to other MPDUs in an A-MPDU.
        // Check that the limit on A-MPDU size is met.
        if (!IsWithinAmpduSizeLimit(ampduSize, receiver, tid, modulation))
        {
            return false;
        }
    }

    return IsWithinSizeAndTimeLimits(ampduSize, receiver, txParams, ppduDurationLimit);
}

void
HtFrameExchangeManager::BlockAckTimeout(Ptr<WifiPsdu> psdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << *psdu << txVector);

    GetWifiRemoteStationManager()->ReportDataFailed(*psdu->begin());

    MissedBlockAck(psdu, txVector);

    m_psdu = nullptr;
    TransmissionFailed();
}

void
HtFrameExchangeManager::MissedBlockAck(Ptr<WifiPsdu> psdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psdu << txVector);

    auto recipient = psdu->GetAddr1();
    auto recipientMld = GetWifiRemoteStationManager()->GetMldAddress(recipient).value_or(recipient);
    bool isBar;
    uint8_t tid;
    std::optional<Mac48Address> gcrGroupAddress;

    if (psdu->GetNMpdus() == 1 && psdu->GetHeader(0).IsBlockAckReq())
    {
        isBar = true;
        CtrlBAckRequestHeader baReqHdr;
        psdu->GetPayload(0)->PeekHeader(baReqHdr);
        tid = baReqHdr.GetTidInfo();
        if (baReqHdr.IsGcr())
        {
            gcrGroupAddress = baReqHdr.GetGcrGroupAddress();
        }
    }
    else
    {
        isBar = false;
        std::set<uint8_t> tids = psdu->GetTids();
        NS_ABORT_MSG_IF(tids.size() > 1, "Multi-TID A-MPDUs not handled here");
        NS_ASSERT(!tids.empty());
        tid = *tids.begin();

        GetWifiRemoteStationManager()
            ->ReportAmpduTxStatus(recipient, 0, psdu->GetNMpdus(), 0, 0, txVector);

        if (auto droppedMpdu = DropMpduIfRetryLimitReached(psdu))
        {
            // notify remote station manager if at least an MPDU was dropped
            GetWifiRemoteStationManager()->ReportFinalDataFailed(droppedMpdu);
        }
    }

    Ptr<QosTxop> edca = m_mac->GetQosTxop(tid);

    if (edca->UseExplicitBarAfterMissedBlockAck() || isBar)
    {
        // we have to send a BlockAckReq, if needed
        const auto retransmitBar =
            gcrGroupAddress.has_value()
                ? GetBaManager(tid)->NeedGcrBarRetransmission(gcrGroupAddress.value(),
                                                              recipientMld,
                                                              tid)
                : GetBaManager(tid)->NeedBarRetransmission(tid, recipientMld);
        if (retransmitBar)
        {
            NS_LOG_DEBUG("Missed Block Ack, transmit a BlockAckReq");
            /**
             * The BlockAckReq must be sent on the same link as the data frames to avoid issues.
             * As an example, assume that an A-MPDU is sent on link 0, the BlockAck timer
             * expires and the BlockAckReq is sent on another link (e.g., on link 1). When the
             * originator processes the BlockAck response, it will not interpret a '0' in the
             * bitmap corresponding to the transmitted MPDUs as a negative acknowledgment,
             * because the BlockAck is received on a different link than the one on which the
             * MPDUs are (still) inflight. Hence, such MPDUs stay inflight and are not
             * retransmitted.
             */
            if (isBar)
            {
                psdu->GetHeader(0).SetRetry();
            }
            else
            {
                // missed block ack after data frame with Implicit BAR Ack policy
                auto [reqHdr, hdr] = edca->PrepareBlockAckRequest(recipient, tid);
                GetBaManager(tid)->ScheduleBar(reqHdr, hdr);
            }
        }
        else
        {
            NS_LOG_DEBUG("Missed Block Ack, do not transmit a BlockAckReq");
            // if a BA agreement exists, we can get here if there is no outstanding
            // MPDU whose lifetime has not expired yet.
            if (isBar)
            {
                DequeuePsdu(psdu);
            }
            if (m_mac->GetBaAgreementEstablishedAsOriginator(recipient, tid))
            {
                // schedule a BlockAckRequest to be sent only if there are data frames queued
                // for this recipient
                GetBaManager(tid)->AddToSendBarIfDataQueuedList(recipientMld, tid);
            }
        }
    }
    else
    {
        // we have to retransmit the data frames, if needed
        GetBaManager(tid)->NotifyMissedBlockAck(m_linkId, recipientMld, tid);
    }
}

void
HtFrameExchangeManager::SendBlockAck(const RecipientBlockAckAgreement& agreement,
                                     Time durationId,
                                     WifiTxVector& blockAckTxVector,
                                     double rxSnr,
                                     std::optional<Mac48Address> gcrGroupAddr)
{
    NS_LOG_FUNCTION(this << durationId << blockAckTxVector << rxSnr << gcrGroupAddr.has_value());

    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_CTL_BACKRESP);
    auto addr1 = agreement.GetPeer();
    if (auto originator = GetWifiRemoteStationManager()->GetAffiliatedStaAddress(addr1))
    {
        addr1 = *originator;
    }
    hdr.SetAddr1(addr1);
    hdr.SetAddr2(m_self);
    hdr.SetDsNotFrom();
    hdr.SetDsNotTo();

    CtrlBAckResponseHeader blockAck;
    blockAck.SetType(agreement.GetBlockAckType());
    if (gcrGroupAddr.has_value())
    {
        blockAck.SetGcrGroupAddress(gcrGroupAddr.value());
    }
    blockAck.SetTidInfo(agreement.GetTid());
    agreement.FillBlockAckBitmap(blockAck);

    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(blockAck);
    Ptr<WifiPsdu> psdu = GetWifiPsdu(Create<WifiMpdu>(packet, hdr), blockAckTxVector);

    // 802.11-2016, Section 9.2.5.7: In a BlockAck frame transmitted in response
    // to a BlockAckReq frame or transmitted in response to a frame containing an
    // implicit block ack request, the Duration/ID field is set to the value obtained
    // from the Duration/ ID field of the frame that elicited the response minus the
    // time, in microseconds between the end of the PPDU carrying the frame that
    // elicited the response and the end of the PPDU carrying the BlockAck frame.
    Time baDurationId = durationId - m_phy->GetSifs() -
                        WifiPhy::CalculateTxDuration(psdu, blockAckTxVector, m_phy->GetPhyBand());
    // The TXOP holder may exceed the TXOP limit in some situations (Sec. 10.22.2.8 of 802.11-2016)
    if (baDurationId.IsStrictlyNegative())
    {
        baDurationId = Seconds(0);
    }
    psdu->GetHeader(0).SetDuration(baDurationId);

    SnrTag tag;
    tag.Set(rxSnr);
    psdu->GetPayload(0)->AddPacketTag(tag);

    ForwardPsduDown(psdu, blockAckTxVector);
}

void
HtFrameExchangeManager::ReceiveMpdu(Ptr<const WifiMpdu> mpdu,
                                    RxSignalInfo rxSignalInfo,
                                    const WifiTxVector& txVector,
                                    bool inAmpdu)
{
    NS_LOG_FUNCTION(this << *mpdu << rxSignalInfo << txVector << inAmpdu);

    // The received MPDU is either broadcast or addressed to this station
    NS_ASSERT(mpdu->GetHeader().GetAddr1().IsGroup() || mpdu->GetHeader().GetAddr1() == m_self);

    double rxSnr = rxSignalInfo.snr;
    const WifiMacHeader& hdr = mpdu->GetHeader();

    if (hdr.IsCtl())
    {
        if (hdr.IsCts() && m_txTimer.IsRunning() &&
            m_txTimer.GetReason() == WifiTxTimer::WAIT_CTS && m_psdu)
        {
            NS_ABORT_MSG_IF(inAmpdu, "Received CTS as part of an A-MPDU");
            NS_ASSERT(hdr.GetAddr1() == m_self);

            Mac48Address sender = m_psdu->GetAddr1();
            NS_LOG_DEBUG("Received CTS from=" << sender);

            SnrTag tag;
            mpdu->GetPacket()->PeekPacketTag(tag);
            GetWifiRemoteStationManager()->ReportRxOk(sender, rxSignalInfo, txVector);
            GetWifiRemoteStationManager()->ReportRtsOk(m_psdu->GetHeader(0),
                                                       rxSnr,
                                                       txVector.GetMode(),
                                                       tag.Get());

            m_txTimer.Cancel();
            m_channelAccessManager->NotifyCtsTimeoutResetNow();
            ProtectionCompleted();
        }
        else if (hdr.IsBlockAck() && m_txTimer.IsRunning() &&
                 m_txTimer.GetReason() == WifiTxTimer::WAIT_BLOCK_ACK && hdr.GetAddr1() == m_self)
        {
            Mac48Address sender = hdr.GetAddr2();
            NS_LOG_DEBUG("Received BlockAck from=" << sender);
            m_txTimer.GotResponseFrom(sender);

            SnrTag tag;
            mpdu->GetPacket()->PeekPacketTag(tag);

            // notify the Block Ack Manager
            CtrlBAckResponseHeader blockAck;
            mpdu->GetPacket()->PeekHeader(blockAck);
            uint8_t tid = blockAck.GetTidInfo();
            if (blockAck.IsGcr())
            {
                const auto& gcrMembers = m_apMac->GetGcrManager()->GetMemberStasForGroupAddress(
                    blockAck.GetGcrGroupAddress());
                const auto ret = GetBaManager(tid)->NotifyGotGcrBlockAck(
                    m_linkId,
                    blockAck,
                    m_mac->GetMldAddress(sender).value_or(sender),
                    gcrMembers);

                if (ret.has_value())
                {
                    for (const auto& sender : gcrMembers)
                    {
                        GetWifiRemoteStationManager()->ReportAmpduTxStatus(sender,
                                                                           ret->first,
                                                                           ret->second,
                                                                           rxSnr,
                                                                           tag.Get(),
                                                                           m_txParams.m_txVector);
                    }
                }
            }
            else
            {
                const auto [nSuccessful, nFailed] = GetBaManager(tid)->NotifyGotBlockAck(
                    m_linkId,
                    blockAck,
                    m_mac->GetMldAddress(sender).value_or(sender),
                    {tid});

                GetWifiRemoteStationManager()->ReportAmpduTxStatus(sender,
                                                                   nSuccessful,
                                                                   nFailed,
                                                                   rxSnr,
                                                                   tag.Get(),
                                                                   m_txParams.m_txVector);
            }

            // cancel the timer
            m_txTimer.Cancel();
            m_channelAccessManager->NotifyAckTimeoutResetNow();

            // Reset the CW
            m_edca->ResetCw(m_linkId);

            // if this BlockAck was sent in response to a BlockAckReq, dequeue the blockAckReq
            if (m_psdu && m_psdu->GetNMpdus() == 1 && m_psdu->GetHeader(0).IsBlockAckReq())
            {
                DequeuePsdu(m_psdu);
            }
            m_psdu = nullptr;
            TransmissionSucceeded();
        }
        else if (hdr.IsBlockAckReq())
        {
            NS_ASSERT(hdr.GetAddr1() == m_self);
            NS_ABORT_MSG_IF(inAmpdu, "BlockAckReq in A-MPDU is not supported");

            auto sender = hdr.GetAddr2();
            NS_LOG_DEBUG("Received BlockAckReq from=" << sender);

            CtrlBAckRequestHeader blockAckReq;
            mpdu->GetPacket()->PeekHeader(blockAckReq);
            NS_ABORT_MSG_IF(blockAckReq.IsMultiTid(), "Multi-TID BlockAckReq not supported");
            const auto tid = blockAckReq.GetTidInfo();

            auto agreement = m_mac->GetBaAgreementEstablishedAsRecipient(
                sender,
                tid,
                blockAckReq.IsGcr() ? std::optional{blockAckReq.GetGcrGroupAddress()}
                                    : std::nullopt);
            if (!agreement)
            {
                NS_LOG_DEBUG("There's not a valid agreement for this BlockAckReq");
                return;
            }

            GetBaManager(tid)->NotifyGotBlockAckRequest(
                m_mac->GetMldAddress(sender).value_or(sender),
                tid,
                blockAckReq.GetStartingSequence(),
                blockAckReq.IsGcr() ? std::optional{blockAckReq.GetGcrGroupAddress()}
                                    : std::nullopt);

            NS_LOG_DEBUG("Schedule Block Ack");
            Simulator::Schedule(
                m_phy->GetSifs(),
                &HtFrameExchangeManager::SendBlockAck,
                this,
                *agreement,
                hdr.GetDuration(),
                GetWifiRemoteStationManager()->GetBlockAckTxVector(sender, txVector),
                rxSnr,
                blockAckReq.IsGcr() ? std::optional{blockAckReq.GetGcrGroupAddress()}
                                    : std::nullopt);
        }
        else
        {
            // the received control frame cannot be handled here
            QosFrameExchangeManager::ReceiveMpdu(mpdu, rxSignalInfo, txVector, inAmpdu);
        }
        return;
    }

    if (const auto isGroup = IsGroupcast(hdr.GetAddr1());
        hdr.IsQosData() && hdr.HasData() &&
        ((hdr.GetAddr1() == m_self) || (isGroup && (inAmpdu || !mpdu->GetHeader().IsQosNoAck()))))
    {
        const auto tid = hdr.GetQosTid();

        auto agreement = m_mac->GetBaAgreementEstablishedAsRecipient(
            hdr.GetAddr2(),
            tid,
            isGroup ? std::optional{hdr.IsQosAmsdu() ? mpdu->begin()->second.GetDestinationAddr()
                                                     : hdr.GetAddr1()}
                    : std::nullopt);
        if (agreement)
        {
            // a Block Ack agreement has been established
            NS_LOG_DEBUG("Received from=" << hdr.GetAddr2() << " (" << *mpdu << ")");

            GetBaManager(tid)->NotifyGotMpdu(mpdu);

            if (!inAmpdu && hdr.GetQosAckPolicy() == WifiMacHeader::NORMAL_ACK)
            {
                NS_LOG_DEBUG("Schedule Normal Ack");
                Simulator::Schedule(m_phy->GetSifs(),
                                    &HtFrameExchangeManager::SendNormalAck,
                                    this,
                                    hdr,
                                    txVector,
                                    rxSnr);
            }
            return;
        }
        // We let the QosFrameExchangeManager handle QoS data frame not belonging
        // to a Block Ack agreement
    }

    if (hdr.IsMgt() && hdr.IsAction())
    {
        ReceiveMgtAction(mpdu, txVector);
    }

    if (IsGroupcast(hdr.GetAddr1()) && hdr.IsQosData() && hdr.IsQosAmsdu() &&
        !m_mac->GetRobustAVStreamingSupported())
    {
        return;
    }

    QosFrameExchangeManager::ReceiveMpdu(mpdu, rxSignalInfo, txVector, inAmpdu);
}

void
HtFrameExchangeManager::ReceiveMgtAction(Ptr<const WifiMpdu> mpdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << *mpdu << txVector);

    NS_ASSERT(mpdu->GetHeader().IsAction());
    const auto from = mpdu->GetOriginal()->GetHeader().GetAddr2();

    WifiActionHeader actionHdr;
    auto packet = mpdu->GetPacket()->Copy();
    packet->RemoveHeader(actionHdr);

    // compute the time to transmit the Ack
    const auto ackTxVector =
        GetWifiRemoteStationManager()->GetAckTxVector(mpdu->GetHeader().GetAddr2(), txVector);
    const auto ackTxTime =
        WifiPhy::CalculateTxDuration(GetAckSize(), ackTxVector, m_phy->GetPhyBand());

    switch (actionHdr.GetCategory())
    {
    case WifiActionHeader::BLOCK_ACK:

        switch (actionHdr.GetAction().blockAck)
        {
        case WifiActionHeader::BLOCK_ACK_ADDBA_REQUEST: {
            MgtAddBaRequestHeader reqHdr;
            packet->RemoveHeader(reqHdr);

            // We've received an ADDBA Request. Our policy here is to automatically accept it,
            // so we get the ADDBA Response on its way as soon as we finish transmitting the Ack,
            // to avoid to concurrently send Ack and ADDBA Response in case of multi-link devices
            Simulator::Schedule(m_phy->GetSifs() + ackTxTime,
                                &HtFrameExchangeManager::SendAddBaResponse,
                                this,
                                reqHdr,
                                from);
            // This frame is now completely dealt with, so we're done.
            return;
        }
        case WifiActionHeader::BLOCK_ACK_ADDBA_RESPONSE: {
            MgtAddBaResponseHeader respHdr;
            packet->RemoveHeader(respHdr);

            // We've received an ADDBA Response. Wait until we finish transmitting the Ack before
            // unblocking transmissions to the recipient, otherwise for multi-link devices the Ack
            // may be sent concurrently with a data frame containing an A-MPDU
            Simulator::Schedule(m_phy->GetSifs() + ackTxTime, [=, this]() {
                const auto recipient =
                    GetWifiRemoteStationManager()->GetMldAddress(from).value_or(from);
                m_mac->GetQosTxop(respHdr.GetTid())->GotAddBaResponse(respHdr, recipient);
                GetBaManager(respHdr.GetTid())
                    ->SetBlockAckInactivityCallback(
                        MakeCallback(&HtFrameExchangeManager::SendDelbaFrame, this));
            });
            // This frame is now completely dealt with, so we're done.
            return;
        }
        case WifiActionHeader::BLOCK_ACK_DELBA: {
            MgtDelBaHeader delBaHdr;
            packet->RemoveHeader(delBaHdr);
            auto recipient = GetWifiRemoteStationManager()->GetMldAddress(from).value_or(from);

            if (delBaHdr.IsByOriginator())
            {
                // This DELBA frame was sent by the originator, so
                // this means that an ingoing established
                // agreement exists in BlockAckManager and we need to
                // destroy it.
                GetBaManager(delBaHdr.GetTid())
                    ->DestroyRecipientAgreement(recipient,
                                                delBaHdr.GetTid(),
                                                delBaHdr.GetGcrGroupAddress());
            }
            else
            {
                // We must have been the originator. We need to
                // tell the correct queue that the agreement has
                // been torn down
                m_mac->GetQosTxop(delBaHdr.GetTid())->GotDelBaFrame(&delBaHdr, recipient);
            }
            // This frame is now completely dealt with, so we're done.
            return;
        }
        default:
            NS_FATAL_ERROR("Unsupported Action field in Block Ack Action frame");
        }
    default:
        // Other action frames are not processed here
        ;
    }
}

void
HtFrameExchangeManager::EndReceiveAmpdu(Ptr<const WifiPsdu> psdu,
                                        const RxSignalInfo& rxSignalInfo,
                                        const WifiTxVector& txVector,
                                        const std::vector<bool>& perMpduStatus)
{
    NS_LOG_FUNCTION(
        this << *psdu << rxSignalInfo << txVector << perMpduStatus.size()
             << std::all_of(perMpduStatus.begin(), perMpduStatus.end(), [](bool v) { return v; }));

    std::set<uint8_t> tids = psdu->GetTids();

    // Multi-TID A-MPDUs are not supported yet
    if (tids.size() == 1)
    {
        uint8_t tid = *tids.begin();
        WifiMacHeader::QosAckPolicy ackPolicy = psdu->GetAckPolicyForTid(tid);
        NS_ASSERT(psdu->GetNMpdus() > 1);

        if (ackPolicy == WifiMacHeader::NORMAL_ACK)
        {
            // Normal Ack or Implicit Block Ack Request
            NS_LOG_DEBUG("Schedule Block Ack");
            auto agreement = m_mac->GetBaAgreementEstablishedAsRecipient(psdu->GetAddr2(), tid);
            NS_ASSERT(agreement);

            Simulator::Schedule(
                m_phy->GetSifs(),
                &HtFrameExchangeManager::SendBlockAck,
                this,
                *agreement,
                psdu->GetDuration(),
                GetWifiRemoteStationManager()->GetBlockAckTxVector(psdu->GetAddr2(), txVector),
                rxSignalInfo.snr,
                std::nullopt);
        }
        else if (psdu->GetAddr1().IsGroup() && (ackPolicy == WifiMacHeader::NO_ACK))
        {
            // groupcast A-MPDU received
            m_flushGroupcastMpdusEvent.Cancel();

            /*
             * There might be pending MPDUs from a previous groupcast transmission
             * that have not been forwarded up yet (e.g. all transmission attempts
             * of a given MPDU have failed). For groupcast transmissions using GCR-UR service,
             * transmitter keeps advancing its window since there is no feedback from the
             * recipients. In order to forward up previously received groupcast MPDUs and avoid
             * following MPDUs not to be forwarded up, we flush the recipient window. The sequence
             * number to use can easily be deduced since sequence number of groupcast MPDUs are
             * consecutive.
             */
            const auto startSeq = psdu->GetHeader(0).GetSequenceNumber();
            const auto groupAddress = psdu->GetHeader(0).IsQosAmsdu()
                                          ? (*psdu->begin())->begin()->second.GetDestinationAddr()
                                          : psdu->GetAddr1();
            FlushGroupcastMpdus(groupAddress, psdu->GetAddr2(), tid, startSeq);

            /*
             * In case all MPDUs of all following transmissions are corrupted or
             * if no following groupcast transmission happens, some groupcast MPDUs
             * of the currently received A-MPDU would never be forwarded up. To prevent this,
             * we schedule a flush of the recipient window once the MSDU lifetime limit elapsed.
             */
            const auto stopSeq = (startSeq + perMpduStatus.size()) % 4096;
            const auto maxDelay = m_mac->GetQosTxop(tid)->GetWifiMacQueue()->GetMaxDelay();
            m_flushGroupcastMpdusEvent =
                Simulator::Schedule(maxDelay,
                                    &HtFrameExchangeManager::FlushGroupcastMpdus,
                                    this,
                                    groupAddress,
                                    psdu->GetAddr2(),
                                    tid,
                                    stopSeq);
        }
    }
}

void
HtFrameExchangeManager::FlushGroupcastMpdus(const Mac48Address& groupAddress,
                                            const Mac48Address& originator,
                                            uint8_t tid,
                                            uint16_t seq)
{
    NS_LOG_FUNCTION(this << groupAddress << originator << tid << seq);
    // We can flush the recipient window by indicating the reception of an implicit GCR BAR
    GetBaManager(tid)->NotifyGotBlockAckRequest(originator, tid, seq, groupAddress);
}

void
HtFrameExchangeManager::NotifyLastGcrUrTx(Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << mpdu);
    const auto tid = mpdu->GetHeader().GetQosTid();
    const auto groupAddress = mpdu->GetHeader().GetAddr1();
    if (!GetBaManager(tid)->IsGcrAgreementEstablished(
            groupAddress,
            tid,
            m_apMac->GetGcrManager()->GetMemberStasForGroupAddress(groupAddress)))
    {
        return;
    }
    GetBaManager(tid)->NotifyLastGcrUrTx(
        mpdu,
        m_apMac->GetGcrManager()->GetMemberStasForGroupAddress(groupAddress));
}

} // namespace ns3
