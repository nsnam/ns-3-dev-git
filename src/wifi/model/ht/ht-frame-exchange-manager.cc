/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "ht-frame-exchange-manager.h"

#include "ht-configuration.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/ctrl-headers.h"
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

bool
HtFrameExchangeManager::SendAddBaRequest(Mac48Address dest,
                                         uint8_t tid,
                                         uint16_t startingSeq,
                                         uint16_t timeout,
                                         bool immediateBAck,
                                         Time availableTime)
{
    NS_LOG_FUNCTION(this << dest << +tid << startingSeq << timeout << immediateBAck
                         << availableTime);
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
HtFrameExchangeManager::SendAddBaResponse(const MgtAddBaRequestHeader* reqHdr,
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
    respHdr.SetAmsduSupport(reqHdr->IsAmsduSupported());

    if (reqHdr->IsImmediateBlockAck())
    {
        respHdr.SetImmediateBlockAck();
    }
    else
    {
        respHdr.SetDelayedBlockAck();
    }
    auto tid = reqHdr->GetTid();
    respHdr.SetTid(tid);

    auto bufferSize = std::min(m_mac->GetMpduBufferSize(), m_mac->GetMaxBaBufferSize(originator));
    respHdr.SetBufferSize(bufferSize);
    respHdr.SetTimeout(reqHdr->GetTimeout());

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
                                                reqHdr->GetStartingSequence(),
                                                m_rxMiddle);

    auto agreement = GetBaManager(tid)->GetAgreementAsRecipient(originator, tid);
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
                                false);
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
HtFrameExchangeManager::SendDelbaFrame(Mac48Address addr, uint8_t tid, bool byOriginator)
{
    NS_LOG_FUNCTION(this << addr << +tid << byOriginator);
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

    WifiActionHeader actionHdr;
    WifiActionHeader::ActionValue action;
    action.blockAck = WifiActionHeader::BLOCK_ACK_DELBA;
    actionHdr.SetAction(WifiActionHeader::BLOCK_ACK, action);

    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(delbaHdr);
    packet->AddHeader(actionHdr);

    m_mac->GetQosTxop(tid)->Queue(Create<WifiMpdu>(packet, hdr));
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
        // if the peeked MPDU has been already transmitted, use its sequence number
        // as the starting sequence number for the BA agreement, otherwise use the
        // next available sequence number
        uint16_t startingSeq =
            (hdr.IsRetry()
                 ? hdr.GetSequenceNumber()
                 : m_txMiddle->GetNextSeqNumberByTidAndAddress(hdr.GetQosTid(), hdr.GetAddr1()));
        return SendAddBaRequest(hdr.GetAddr1(),
                                hdr.GetQosTid(),
                                startingSeq,
                                edca->GetBlockAckInactivityTimeout(),
                                true,
                                availableTime);
    }

    // Use SendDataFrame if we can try aggregation
    if (hdr.IsQosData() && !hdr.GetAddr1().IsGroup() && !peekedItem->IsFragment() &&
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

            auto agreement = m_mac->GetBaAgreementEstablishedAsOriginator(recipient, tid);
            if (!agreement)
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
            if (queue->PeekByTidAndAddress(tid, recipient))
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

    if (selectedBar && selectedBar->GetHeader().GetAddr2() != m_self)
    {
        // the selected BAR has MLD addresses in Addr1/Addr2, replace them with link addresses
        // and move to the appropriate container queue
        NS_ASSERT(selectedBar->GetHeader().GetAddr2() == m_mac->GetAddress());
        DequeueMpdu(selectedBar);
        const auto currAddr1 = selectedBar->GetHeader().GetAddr1();
        auto addr1 =
            GetWifiRemoteStationManager()->GetAffiliatedStaAddress(currAddr1).value_or(currAddr1);
        selectedBar->GetHeader().SetAddr1(addr1);
        selectedBar->GetHeader().SetAddr2(m_self);
        queue->Enqueue(selectedBar);
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
        SendPsduWithProtection(Create<WifiPsdu>(mpdu, false), txParams);
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
        Time baTxDuration = m_phy->CalculateTxDuration(GetBlockAckSize(blockAcknowledgment->baType),
                                                       blockAcknowledgment->blockAckTxVector,
                                                       m_phy->GetPhyBand());
        blockAcknowledgment->acknowledgmentTime = m_phy->GetSifs() + baTxDuration;
    }
    else if (acknowledgment->method == WifiAcknowledgment::BAR_BLOCK_ACK)
    {
        auto barBlockAcknowledgment = static_cast<WifiBarBlockAck*>(acknowledgment);
        Time barTxDuration =
            m_phy->CalculateTxDuration(GetBlockAckRequestSize(barBlockAcknowledgment->barType),
                                       barBlockAcknowledgment->blockAckReqTxVector,
                                       m_phy->GetPhyBand());
        Time baTxDuration =
            m_phy->CalculateTxDuration(GetBlockAckSize(barBlockAcknowledgment->baType),
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
                    GetBaManager(tid)->DestroyOriginatorAgreement(address, tid);
                }
                else
                {
                    GetBaManager(tid)->DestroyRecipientAgreement(address, tid);
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
                                    addBa.GetTid());
            }
            else if (actionHdr.GetAction().blockAck == WifiActionHeader::BLOCK_ACK_ADDBA_RESPONSE)
            {
                // A recipient Block Ack agreement must exist
                MgtAddBaResponseHeader addBa;
                p->PeekHeader(addBa);
                auto tid = addBa.GetTid();
                NS_ASSERT_MSG(GetBaManager(tid)->GetAgreementAsRecipient(address, tid),
                              "Recipient BA agreement {" << address << ", " << +tid
                                                         << "} not found");
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

    if (m_edca && m_edca->GetTxopLimit(m_linkId).IsZero() && GetBar(m_edca->GetAccessCategory()))
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
            uint8_t tid = GetTid(mpdu->GetPacket(), mpdu->GetHeader());
            auto recipient = mpdu->GetHeader().GetAddr1();
            // if the recipient is an MLD, use its MLD address
            if (auto mldAddr = GetWifiRemoteStationManager()->GetMldAddress(recipient))
            {
                recipient = *mldAddr;
            }
            if (auto agreement = GetBaManager(tid)->GetAgreementAsOriginator(recipient, tid);
                agreement && agreement->get().IsPending())
            {
                NS_LOG_DEBUG("No ACK after ADDBA request");
                Ptr<QosTxop> qosTxop = m_mac->GetQosTxop(tid);
                qosTxop->NotifyOriginatorAgreementNoReply(recipient, tid);
                Simulator::Schedule(qosTxop->GetFailedAddBaTimeout(),
                                    &QosTxop::ResetBa,
                                    qosTxop,
                                    recipient,
                                    tid);
            }
        }
    }
    QosFrameExchangeManager::NotifyPacketDiscarded(mpdu);
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

    auto tids = psdu->GetTids();

    if (tids.empty() || // no QoS data frames included
        !m_mac->GetBaAgreementEstablishedAsOriginator(psdu->GetAddr1(), *tids.begin()))
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
            NS_ASSERT(m_mac->GetBaAgreementEstablishedAsOriginator(hdr.GetAddr1(), tid));

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

    if (m_edca->GetTxopLimit(m_linkId).IsZero())
    {
        NS_ASSERT(txParams.m_acknowledgment &&
                  txParams.m_acknowledgment->acknowledgmentTime.has_value());
        return *txParams.m_acknowledgment->acknowledgmentTime;
    }

    // under multiple protection settings, if the TXOP limit is not null, Duration/ID
    // is set to cover the remaining TXOP time (Sec. 9.2.5.2 of 802.11-2016).
    // The TXOP holder may exceed the TXOP limit in some situations (Sec. 10.22.2.8
    // of 802.11-2016)
    return std::max(m_edca->GetRemainingTxop(m_linkId) - txDuration, Seconds(0));
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
        SendPsdu();
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

    DoCtsTimeout(m_psdu);
    m_psdu = nullptr;
}

void
HtFrameExchangeManager::SendPsdu()
{
    NS_LOG_FUNCTION(this);

    Time txDuration =
        m_phy->CalculateTxDuration(m_psdu->GetSize(), m_txParams.m_txVector, m_phy->GetPhyBand());

    NS_ASSERT(m_txParams.m_acknowledgment);

    if (m_txParams.m_acknowledgment->method == WifiAcknowledgment::NONE)
    {
        Simulator::Schedule(txDuration, &HtFrameExchangeManager::TransmissionSucceeded, this);

        std::set<uint8_t> tids = m_psdu->GetTids();
        NS_ASSERT_MSG(tids.size() <= 1, "Multi-TID A-MPDUs are not supported");

        if (tids.empty() || m_psdu->GetAckPolicyForTid(*tids.begin()) == WifiMacHeader::NO_ACK)
        {
            // No acknowledgment, hence dequeue the PSDU if it is stored in a queue
            DequeuePsdu(m_psdu);
        }
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
            m_phy->CalculatePhyPreambleAndHeaderDuration(blockAcknowledgment->blockAckTxVector);
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
        std::set<uint8_t> tids = m_psdu->GetTids();
        NS_ABORT_MSG_IF(tids.size() > 1,
                        "Acknowledgment method incompatible with a Multi-TID A-MPDU");
        uint8_t tid = *tids.begin();

        Ptr<QosTxop> edca = m_mac->GetQosTxop(tid);
        auto [reqHdr, hdr] = edca->PrepareBlockAckRequest(m_psdu->GetAddr1(), tid);
        GetBaManager(tid)->ScheduleBar(reqHdr, hdr);

        Simulator::Schedule(txDuration, &HtFrameExchangeManager::TransmissionSucceeded, this);
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
                    queueSizeForTid[tid] = edca->GetQosQueueSize(tid, hdr.GetAddr1());
                }

                hdr.SetQosEosp();
                hdr.SetQosQueueSize(queueSizeForTid[tid].value());
            }
        }
    }

    QosFrameExchangeManager::FinalizeMacHeader(psdu);
}

void
HtFrameExchangeManager::DequeuePsdu(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_DEBUG(this << psdu);

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

    bool resetCw;
    MissedBlockAck(psdu, txVector, resetCw);

    NS_ASSERT(m_edca);

    if (resetCw)
    {
        m_edca->ResetCw(m_linkId);
    }
    else
    {
        m_edca->UpdateFailedCw(m_linkId);
    }

    m_psdu = nullptr;
    TransmissionFailed();
}

void
HtFrameExchangeManager::MissedBlockAck(Ptr<WifiPsdu> psdu,
                                       const WifiTxVector& txVector,
                                       bool& resetCw)
{
    NS_LOG_FUNCTION(this << psdu << txVector << resetCw);

    auto recipient = psdu->GetAddr1();
    auto recipientMld = GetWifiRemoteStationManager()->GetMldAddress(recipient).value_or(recipient);
    bool isBar;
    uint8_t tid;

    if (psdu->GetNMpdus() == 1 && psdu->GetHeader(0).IsBlockAckReq())
    {
        isBar = true;
        CtrlBAckRequestHeader baReqHdr;
        psdu->GetPayload(0)->PeekHeader(baReqHdr);
        tid = baReqHdr.GetTidInfo();
    }
    else
    {
        isBar = false;
        std::set<uint8_t> tids = psdu->GetTids();
        NS_ABORT_MSG_IF(tids.size() > 1, "Multi-TID A-MPDUs not handled here");
        NS_ASSERT(!tids.empty());
        tid = *tids.begin();
    }

    Ptr<QosTxop> edca = m_mac->GetQosTxop(tid);

    if (edca->UseExplicitBarAfterMissedBlockAck() || isBar)
    {
        // we have to send a BlockAckReq, if needed
        if (GetBaManager(tid)->NeedBarRetransmission(tid, recipientMld))
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
            resetCw = false;
        }
        else
        {
            NS_LOG_DEBUG("Missed Block Ack, do not transmit a BlockAckReq");
            // if a BA agreement exists, we can get here if there is no outstanding
            // MPDU whose lifetime has not expired yet.
            GetWifiRemoteStationManager()->ReportFinalDataFailed(*psdu->begin());
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
            resetCw = true;
        }
    }
    else
    {
        // we have to retransmit the data frames, if needed
        GetWifiRemoteStationManager()
            ->ReportAmpduTxStatus(recipient, 0, psdu->GetNMpdus(), 0, 0, txVector);
        if (!GetWifiRemoteStationManager()->NeedRetransmission(*psdu->begin()))
        {
            NS_LOG_DEBUG("Missed Block Ack, do not retransmit the data frames");
            GetWifiRemoteStationManager()->ReportFinalDataFailed(*psdu->begin());
            for (const auto& mpdu : *PeekPointer(psdu))
            {
                NotifyPacketDiscarded(mpdu);
                DequeueMpdu(mpdu);
            }
            resetCw = true;
        }
        else
        {
            NS_LOG_DEBUG("Missed Block Ack, retransmit data frames");
            GetBaManager(tid)->NotifyMissedBlockAck(m_linkId, recipientMld, tid);
            resetCw = false;
        }
    }
}

void
HtFrameExchangeManager::SendBlockAck(const RecipientBlockAckAgreement& agreement,
                                     Time durationId,
                                     WifiTxVector& blockAckTxVector,
                                     double rxSnr)
{
    NS_LOG_FUNCTION(this << durationId << blockAckTxVector << rxSnr);

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
    blockAck.SetTidInfo(agreement.GetTid());
    agreement.FillBlockAckBitmap(&blockAck);

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
                        m_phy->CalculateTxDuration(psdu, blockAckTxVector, m_phy->GetPhyBand());
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
            Simulator::Schedule(m_phy->GetSifs(),
                                &HtFrameExchangeManager::ProtectionCompleted,
                                this);
        }
        else if (hdr.IsBlockAck() && m_txTimer.IsRunning() &&
                 m_txTimer.GetReason() == WifiTxTimer::WAIT_BLOCK_ACK && hdr.GetAddr1() == m_self)
        {
            Mac48Address sender = hdr.GetAddr2();
            NS_LOG_DEBUG("Received BlockAck from=" << sender);

            SnrTag tag;
            mpdu->GetPacket()->PeekPacketTag(tag);

            // notify the Block Ack Manager
            CtrlBAckResponseHeader blockAck;
            mpdu->GetPacket()->PeekHeader(blockAck);
            uint8_t tid = blockAck.GetTidInfo();
            std::pair<uint16_t, uint16_t> ret =
                GetBaManager(tid)->NotifyGotBlockAck(m_linkId,
                                                     blockAck,
                                                     m_mac->GetMldAddress(sender).value_or(sender),
                                                     {tid});
            GetWifiRemoteStationManager()->ReportAmpduTxStatus(sender,
                                                               ret.first,
                                                               ret.second,
                                                               rxSnr,
                                                               tag.Get(),
                                                               m_txParams.m_txVector);

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

            Mac48Address sender = hdr.GetAddr2();
            NS_LOG_DEBUG("Received BlockAckReq from=" << sender);

            CtrlBAckRequestHeader blockAckReq;
            mpdu->GetPacket()->PeekHeader(blockAckReq);
            NS_ABORT_MSG_IF(blockAckReq.IsMultiTid(), "Multi-TID BlockAckReq not supported");
            uint8_t tid = blockAckReq.GetTidInfo();

            auto agreement = m_mac->GetBaAgreementEstablishedAsRecipient(sender, tid);

            if (!agreement)
            {
                NS_LOG_DEBUG("There's not a valid agreement for this BlockAckReq");
                return;
            }

            GetBaManager(tid)->NotifyGotBlockAckRequest(
                m_mac->GetMldAddress(sender).value_or(sender),
                tid,
                blockAckReq.GetStartingSequence());

            NS_LOG_DEBUG("Schedule Block Ack");
            Simulator::Schedule(
                m_phy->GetSifs(),
                &HtFrameExchangeManager::SendBlockAck,
                this,
                *agreement,
                hdr.GetDuration(),
                GetWifiRemoteStationManager()->GetBlockAckTxVector(sender, txVector),
                rxSnr);
        }
        else
        {
            // the received control frame cannot be handled here
            QosFrameExchangeManager::ReceiveMpdu(mpdu, rxSignalInfo, txVector, inAmpdu);
        }
        return;
    }

    if (hdr.IsQosData() && hdr.HasData() && hdr.GetAddr1() == m_self)
    {
        uint8_t tid = hdr.GetQosTid();

        if (m_mac->GetBaAgreementEstablishedAsRecipient(hdr.GetAddr2(), tid))
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

    QosFrameExchangeManager::ReceiveMpdu(mpdu, rxSignalInfo, txVector, inAmpdu);
}

void
HtFrameExchangeManager::EndReceiveAmpdu(Ptr<const WifiPsdu> psdu,
                                        const RxSignalInfo& rxSignalInfo,
                                        const WifiTxVector& txVector,
                                        const std::vector<bool>& perMpduStatus)
{
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
                rxSignalInfo.snr);
        }
    }
}

} // namespace ns3
