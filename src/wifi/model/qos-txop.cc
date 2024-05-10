/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 *          Stefano Avallone <stavalli@unina.it>
 */

#include "qos-txop.h"

#include "channel-access-manager.h"
#include "ctrl-headers.h"
#include "mac-tx-middle.h"
#include "mgt-action-headers.h"
#include "mpdu-aggregator.h"
#include "msdu-aggregator.h"
#include "wifi-mac-queue-scheduler.h"
#include "wifi-mac-queue.h"
#include "wifi-mac-trailer.h"
#include "wifi-phy.h"
#include "wifi-psdu.h"
#include "wifi-tx-parameters.h"

#include "ns3/ht-configuration.h"
#include "ns3/ht-frame-exchange-manager.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT WIFI_TXOP_NS_LOG_APPEND_CONTEXT

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("QosTxop");

NS_OBJECT_ENSURE_REGISTERED(QosTxop);

TypeId
QosTxop::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::QosTxop")
            .SetParent<ns3::Txop>()
            .SetGroupName("Wifi")
            .AddConstructor<QosTxop>()
            .AddAttribute("UseExplicitBarAfterMissedBlockAck",
                          "Specify whether explicit BlockAckRequest should be sent upon missed "
                          "BlockAck Response.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&QosTxop::m_useExplicitBarAfterMissedBlockAck),
                          MakeBooleanChecker())
            .AddAttribute("AddBaResponseTimeout",
                          "The timeout to wait for ADDBA response after the Ack to "
                          "ADDBA request is received.",
                          TimeValue(MilliSeconds(5)),
                          MakeTimeAccessor(&QosTxop::SetAddBaResponseTimeout,
                                           &QosTxop::GetAddBaResponseTimeout),
                          MakeTimeChecker())
            .AddAttribute(
                "FailedAddBaTimeout",
                "The timeout after a failed BA agreement. During this "
                "timeout, the originator resumes sending packets using normal "
                "MPDU. After that, BA agreement is reset and the originator "
                "will retry BA negotiation.",
                TimeValue(MilliSeconds(200)),
                MakeTimeAccessor(&QosTxop::SetFailedAddBaTimeout, &QosTxop::GetFailedAddBaTimeout),
                MakeTimeChecker())
            .AddAttribute("BlockAckManager",
                          "The BlockAckManager object.",
                          PointerValue(),
                          MakePointerAccessor(&QosTxop::m_baManager),
                          MakePointerChecker<BlockAckManager>())
            .AddAttribute("NMaxInflights",
                          "The maximum number of links (in the range 1-15) on which an MPDU can be "
                          "simultaneously in-flight.",
                          UintegerValue(1),
                          MakeUintegerAccessor(&QosTxop::m_nMaxInflights),
                          MakeUintegerChecker<uint8_t>(1, 15))
            .AddTraceSource("TxopTrace",
                            "Trace source for TXOP start and duration times",
                            MakeTraceSourceAccessor(&QosTxop::m_txopTrace),
                            "ns3::QosTxop::TxopTracedCallback");
    return tid;
}

QosTxop::QosTxop()
{
    NS_LOG_FUNCTION(this);
    m_baManager = CreateObject<BlockAckManager>();
}

void
QosTxop::CreateQueue(AcIndex aci)
{
    NS_LOG_FUNCTION(this << aci);
    Txop::CreateQueue(aci);
    m_ac = aci;
    m_baManager->SetQueue(m_queue);
    m_baManager->SetBlockDestinationCallback(
        Callback<void, Mac48Address, uint8_t>([this](Mac48Address recipient, uint8_t tid) {
            m_mac->GetMacQueueScheduler()->BlockQueues(WifiQueueBlockedReason::WAITING_ADDBA_RESP,
                                                       m_ac,
                                                       {WIFI_QOSDATA_QUEUE},
                                                       recipient,
                                                       m_mac->GetLocalAddress(recipient),
                                                       {tid});
        }));
    m_baManager->SetUnblockDestinationCallback(
        Callback<void, Mac48Address, uint8_t>([this](Mac48Address recipient, uint8_t tid) {
            // save the status of AC queues before unblocking the transmissions to the recipient
            std::map<uint8_t, bool> hasFramesToTransmit;
            for (const auto& [id, link] : GetLinks())
            {
                hasFramesToTransmit[id] = HasFramesToTransmit(id);
            }

            m_mac->GetMacQueueScheduler()->UnblockQueues(WifiQueueBlockedReason::WAITING_ADDBA_RESP,
                                                         m_ac,
                                                         {WIFI_QOSDATA_QUEUE},
                                                         recipient,
                                                         m_mac->GetLocalAddress(recipient),
                                                         {tid});

            // start access (if needed) on all the links
            for (const auto& [id, link] : GetLinks())
            {
                StartAccessAfterEvent(id, hasFramesToTransmit.at(id), CHECK_MEDIUM_BUSY);
            }
        }));
    m_queue->TraceConnectWithoutContext(
        "Expired",
        MakeCallback(&BlockAckManager::NotifyDiscardedMpdu, m_baManager));
}

QosTxop::~QosTxop()
{
    NS_LOG_FUNCTION(this);
}

void
QosTxop::DoDispose()
{
    NS_LOG_FUNCTION(this);
    if (m_baManager)
    {
        m_baManager->Dispose();
    }
    m_baManager = nullptr;
    Txop::DoDispose();
}

std::unique_ptr<Txop::LinkEntity>
QosTxop::CreateLinkEntity() const
{
    return std::make_unique<QosLinkEntity>();
}

QosTxop::QosLinkEntity&
QosTxop::GetLink(uint8_t linkId) const
{
    return static_cast<QosLinkEntity&>(Txop::GetLink(linkId));
}

uint8_t
QosTxop::GetQosQueueSize(uint8_t tid, Mac48Address receiver) const
{
    WifiContainerQueueId queueId{WIFI_QOSDATA_QUEUE, WIFI_UNICAST, receiver, tid};
    uint32_t bufferSize = m_queue->GetNBytes(queueId);
    // A queue size value of 254 is used for all sizes greater than 64 768 octets.
    uint8_t queueSize = static_cast<uint8_t>(std::ceil(std::min(bufferSize, 64769U) / 256.0));
    NS_LOG_DEBUG("Buffer size=" << bufferSize << " Queue Size=" << +queueSize);
    return queueSize;
}

void
QosTxop::SetDroppedMpduCallback(DroppedMpdu callback)
{
    NS_LOG_FUNCTION(this << &callback);
    Txop::SetDroppedMpduCallback(callback);
    m_baManager->SetDroppedOldMpduCallback(callback.Bind(WIFI_MAC_DROP_QOS_OLD_PACKET));
}

void
QosTxop::SetMuCwMin(uint16_t cwMin, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << cwMin << +linkId);
    GetLink(linkId).muCwMin = cwMin;
}

void
QosTxop::SetMuCwMax(uint16_t cwMax, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << cwMax << +linkId);
    GetLink(linkId).muCwMax = cwMax;
}

void
QosTxop::SetMuAifsn(uint8_t aifsn, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +aifsn << +linkId);
    GetLink(linkId).muAifsn = aifsn;
}

void
QosTxop::SetMuEdcaTimer(Time timer, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << timer << +linkId);
    GetLink(linkId).muEdcaTimer = timer;
}

void
QosTxop::StartMuEdcaTimerNow(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +linkId);
    auto& link = GetLink(linkId);
    link.muEdcaTimerStartTime = Simulator::Now();
    if (EdcaDisabled(linkId))
    {
        NS_LOG_DEBUG("Disable EDCA for " << link.muEdcaTimer.As(Time::MS));
        m_mac->GetChannelAccessManager(linkId)->DisableEdcaFor(this, link.muEdcaTimer);
    }
}

bool
QosTxop::MuEdcaTimerRunning(uint8_t linkId) const
{
    auto& link = GetLink(linkId);
    return (link.muEdcaTimerStartTime.IsStrictlyPositive() &&
            link.muEdcaTimer.IsStrictlyPositive() &&
            link.muEdcaTimerStartTime + link.muEdcaTimer > Simulator::Now());
}

bool
QosTxop::EdcaDisabled(uint8_t linkId) const
{
    return (MuEdcaTimerRunning(linkId) && GetLink(linkId).muAifsn == 0);
}

uint32_t
QosTxop::GetMinCw(uint8_t linkId) const
{
    if (!MuEdcaTimerRunning(linkId))
    {
        return GetLink(linkId).cwMin;
    }
    NS_ASSERT(!EdcaDisabled(linkId));
    return GetLink(linkId).muCwMin;
}

uint32_t
QosTxop::GetMaxCw(uint8_t linkId) const
{
    if (!MuEdcaTimerRunning(linkId))
    {
        return GetLink(linkId).cwMax;
    }
    NS_ASSERT(!EdcaDisabled(linkId));
    return GetLink(linkId).muCwMax;
}

uint8_t
QosTxop::GetAifsn(uint8_t linkId) const
{
    if (!MuEdcaTimerRunning(linkId))
    {
        return GetLink(linkId).aifsn;
    }
    return GetLink(linkId).muAifsn;
}

Ptr<BlockAckManager>
QosTxop::GetBaManager()
{
    return m_baManager;
}

uint16_t
QosTxop::GetBaBufferSize(Mac48Address address, uint8_t tid) const
{
    return m_baManager->GetRecipientBufferSize(address, tid);
}

uint16_t
QosTxop::GetBaStartingSequence(Mac48Address address, uint8_t tid) const
{
    return m_baManager->GetOriginatorStartingSequence(address, tid);
}

std::pair<CtrlBAckRequestHeader, WifiMacHeader>
QosTxop::PrepareBlockAckRequest(Mac48Address recipient, uint8_t tid) const
{
    NS_LOG_FUNCTION(this << recipient << +tid);
    NS_ASSERT(QosUtilsMapTidToAc(tid) == m_ac);

    auto recipientMld = m_mac->GetMldAddress(recipient);

    CtrlBAckRequestHeader reqHdr =
        m_baManager->GetBlockAckReqHeader(recipientMld.value_or(recipient), tid);

    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_CTL_BACKREQ);
    hdr.SetAddr1(recipient);
    hdr.SetAddr2(m_mac->GetLocalAddress(recipient));
    hdr.SetDsNotTo();
    hdr.SetDsNotFrom();
    hdr.SetNoRetry();
    hdr.SetNoMoreFragments();

    return {reqHdr, hdr};
}

bool
QosTxop::UseExplicitBarAfterMissedBlockAck() const
{
    return m_useExplicitBarAfterMissedBlockAck;
}

bool
QosTxop::HasFramesToTransmit(uint8_t linkId)
{
    // remove MSDUs with expired lifetime starting from the head of the queue
    m_queue->WipeAllExpiredMpdus();
    auto hasFramesToTransmit = static_cast<bool>(m_queue->PeekFirstAvailable(linkId));

    // Print the number of packets that are actually in the queue (which might not be
    // eligible for transmission for some reason, e.g., TID not mapped to the link, etc.)
    NS_LOG_DEBUG(m_ac << " on link " << +linkId << (hasFramesToTransmit ? " has" : " has not")
                      << " frames to transmit with " << m_queue->GetNPackets()
                      << " packets in the queue");
    return hasFramesToTransmit;
}

uint16_t
QosTxop::GetNextSequenceNumberFor(const WifiMacHeader* hdr)
{
    return m_txMiddle->GetNextSequenceNumberFor(hdr);
}

uint16_t
QosTxop::PeekNextSequenceNumberFor(const WifiMacHeader* hdr)
{
    return m_txMiddle->PeekNextSequenceNumberFor(hdr);
}

bool
QosTxop::IsQosOldPacket(Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << *mpdu);

    if (!mpdu->GetHeader().IsQosData())
    {
        return false;
    }

    Mac48Address recipient = mpdu->GetHeader().GetAddr1();
    uint8_t tid = mpdu->GetHeader().GetQosTid();

    if (!m_mac->GetBaAgreementEstablishedAsOriginator(recipient, tid))
    {
        return false;
    }

    return QosUtilsIsOldPacket(GetBaStartingSequence(recipient, tid),
                               mpdu->GetHeader().GetSequenceNumber());
}

Ptr<WifiMpdu>
QosTxop::PeekNextMpdu(uint8_t linkId, uint8_t tid, Mac48Address recipient, Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << +linkId << +tid << recipient << mpdu);

    // lambda to peek the next frame
    auto peek = [this, &linkId, &tid, &recipient, &mpdu]() -> Ptr<WifiMpdu> {
        if (tid == 8 && recipient.IsBroadcast()) // undefined TID and recipient
        {
            return m_queue->PeekFirstAvailable(linkId, mpdu);
        }
        WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WIFI_UNICAST, recipient, tid);
        if (auto mask = m_mac->GetMacQueueScheduler()->GetQueueLinkMask(m_ac, queueId, linkId);
            mask && mask->none())
        {
            return m_queue->PeekByQueueId(queueId, mpdu);
        }
        return nullptr;
    };

    auto item = peek();
    // remove old packets (must be retransmissions or in flight, otherwise they did
    // not get a sequence number assigned)
    while (item && !item->IsFragment())
    {
        if (item->GetHeader().IsCtl())
        {
            NS_LOG_DEBUG("Skipping control frame: " << *item);
            mpdu = item;
            item = peek();
            continue;
        }

        if (item->HasSeqNoAssigned() && IsQosOldPacket(item))
        {
            NS_LOG_DEBUG("Removing an old packet from EDCA queue: " << *item);
            if (!m_droppedMpduCallback.IsNull())
            {
                m_droppedMpduCallback(WIFI_MAC_DROP_QOS_OLD_PACKET, item);
            }
            mpdu = item;
            item = peek();
            m_queue->Remove(mpdu);
            continue;
        }

        if (auto linkIds = item->GetInFlightLinkIds(); !linkIds.empty()) // MPDU is in-flight
        {
            // if the MPDU is not already in-flight on the link for which we are requesting an
            // MPDU and the number of links on which the MPDU is in-flight is less than the
            // maximum number, then we can transmit this MPDU
            if (!linkIds.contains(linkId) && (linkIds.size() < m_nMaxInflights))
            {
                break;
            }

            // if no BA agreement, we cannot have multiple MPDUs in-flight
            if (item->GetHeader().IsQosData() &&
                !m_mac->GetBaAgreementEstablishedAsOriginator(item->GetHeader().GetAddr1(),
                                                              item->GetHeader().GetQosTid()))
            {
                NS_LOG_DEBUG("No BA agreement and an MPDU is already in-flight");
                return nullptr;
            }

            NS_LOG_DEBUG("Skipping in flight MPDU: " << *item);
            mpdu = item;
            item = peek();
            continue;
        }

        if (item->GetHeader().HasData() &&
            !m_mac->CanForwardPacketsTo(item->GetHeader().GetAddr1()))
        {
            NS_LOG_DEBUG("Skipping frame that cannot be forwarded: " << *item);
            mpdu = item;
            item = peek();
            continue;
        }
        break;
    }

    if (!item)
    {
        return nullptr;
    }

    WifiMacHeader& hdr = item->GetHeader();

    // peek the next sequence number and check if it is within the transmit window
    // in case of QoS data frame
    uint16_t sequence = item->HasSeqNoAssigned() ? hdr.GetSequenceNumber()
                                                 : m_txMiddle->PeekNextSequenceNumberFor(&hdr);
    if (hdr.IsQosData())
    {
        Mac48Address recipient = hdr.GetAddr1();
        uint8_t tid = hdr.GetQosTid();

        if (m_mac->GetBaAgreementEstablishedAsOriginator(recipient, tid) &&
            !IsInWindow(sequence,
                        GetBaStartingSequence(recipient, tid),
                        GetBaBufferSize(recipient, tid)))
        {
            NS_LOG_DEBUG("Packet beyond the end of the current transmit window");
            return nullptr;
        }
    }

    // Assign a sequence number if this is not a fragment nor it already has one assigned
    if (!item->IsFragment() && !item->HasSeqNoAssigned())
    {
        hdr.SetSequenceNumber(sequence);
    }
    NS_LOG_DEBUG("Packet peeked from EDCA queue: " << *item);
    return item;
}

Ptr<WifiMpdu>
QosTxop::GetNextMpdu(uint8_t linkId,
                     Ptr<WifiMpdu> peekedItem,
                     WifiTxParameters& txParams,
                     Time availableTime,
                     bool initialFrame)
{
    NS_ASSERT(peekedItem);
    NS_LOG_FUNCTION(this << +linkId << *peekedItem << &txParams << availableTime << initialFrame);

    Mac48Address recipient = peekedItem->GetHeader().GetAddr1();

    // The TXOP limit can be exceeded by the TXOP holder if it does not transmit more
    // than one Data or Management frame in the TXOP and the frame is not in an A-MPDU
    // consisting of more than one MPDU (Sec. 10.22.2.8 of 802.11-2016)
    Time actualAvailableTime =
        (initialFrame && txParams.GetSize(recipient) == 0 ? Time::Min() : availableTime);

    auto qosFem = StaticCast<QosFrameExchangeManager>(m_mac->GetFrameExchangeManager(linkId));
    if (!qosFem->TryAddMpdu(peekedItem, txParams, actualAvailableTime))
    {
        return nullptr;
    }

    NS_ASSERT(peekedItem->IsQueued());
    Ptr<WifiMpdu> mpdu;

    // If it is a non-broadcast QoS Data frame and it is not a retransmission nor a fragment,
    // attempt A-MSDU aggregation
    if (peekedItem->GetHeader().IsQosData())
    {
        uint8_t tid = peekedItem->GetHeader().GetQosTid();

        // we should not be asked to dequeue an MPDU that is beyond the transmit window.
        // Note that PeekNextMpdu() temporarily assigns the next available sequence number
        // to the peeked frame
        NS_ASSERT(!m_mac->GetBaAgreementEstablishedAsOriginator(recipient, tid) ||
                  IsInWindow(
                      peekedItem->GetHeader().GetSequenceNumber(),
                      GetBaStartingSequence(peekedItem->GetOriginal()->GetHeader().GetAddr1(), tid),
                      GetBaBufferSize(peekedItem->GetOriginal()->GetHeader().GetAddr1(), tid)));

        // try A-MSDU aggregation if the MPDU does not contain an A-MSDU and does not already
        // have a sequence number assigned (may be a retransmission)
        if (m_mac->GetHtConfiguration() && !recipient.IsBroadcast() &&
            !peekedItem->GetHeader().IsQosAmsdu() && !peekedItem->HasSeqNoAssigned() &&
            !peekedItem->IsFragment())
        {
            auto htFem = StaticCast<HtFrameExchangeManager>(qosFem);
            mpdu = htFem->GetMsduAggregator()->GetNextAmsdu(peekedItem, txParams, availableTime);
        }

        if (mpdu)
        {
            NS_LOG_DEBUG("Prepared an MPDU containing an A-MSDU");
        }
        // else aggregation was not attempted or failed
    }

    if (!mpdu)
    {
        mpdu = peekedItem;
    }

    // Assign a sequence number if this is not a fragment nor a retransmission
    AssignSequenceNumber(mpdu);
    NS_LOG_DEBUG("Got MPDU from EDCA queue: " << *mpdu);

    return mpdu;
}

void
QosTxop::AssignSequenceNumber(Ptr<WifiMpdu> mpdu) const
{
    NS_LOG_FUNCTION(this << *mpdu);

    if (!mpdu->IsFragment() && !mpdu->HasSeqNoAssigned())
    {
        // in case of 11be MLDs, sequence numbers refer to the MLD address
        auto origMpdu = m_queue->GetOriginal(mpdu);
        uint16_t sequence = m_txMiddle->GetNextSequenceNumberFor(&origMpdu->GetHeader());
        mpdu->AssignSeqNo(sequence);
    }
}

void
QosTxop::NotifyChannelAccessed(uint8_t linkId, Time txopDuration)
{
    NS_LOG_FUNCTION(this << +linkId << txopDuration);

    NS_ASSERT(txopDuration != Time::Min());
    GetLink(linkId).startTxop = Simulator::Now();
    GetLink(linkId).txopDuration = txopDuration;
    Txop::NotifyChannelAccessed(linkId);
}

std::optional<Time>
QosTxop::GetTxopStartTime(uint8_t linkId) const
{
    auto& link = GetLink(linkId);
    NS_LOG_FUNCTION(this << link.startTxop.has_value());
    return link.startTxop;
}

void
QosTxop::NotifyChannelReleased(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +linkId);
    auto& link = GetLink(linkId);

    if (link.startTxop)
    {
        NS_LOG_DEBUG("Terminating TXOP. Duration = " << Simulator::Now() - *link.startTxop);
        m_txopTrace(*link.startTxop, Simulator::Now() - *link.startTxop, linkId);
    }

    // generate a new backoff value if either the TXOP duration is not null (i.e., some frames
    // were transmitted) or no frame was transmitted but the queue actually contains frame to
    // transmit and the user indicated that a backoff value should be generated in this situation.
    // This behavior reflects the following specs text (Sec. 35.3.16.4 of 802.11be D4.0):
    // An AP or non-AP STA affiliated with an MLD that has gained the right to initiate the
    // transmission of a frame as described in 10.23.2.4 (Obtaining an EDCA TXOP) for an AC but
    // does not transmit any frame corresponding to that AC for the reasons stated above may:
    // - invoke a backoff for the EDCAF associated with that AC as allowed per h) of 10.23.2.2
    //   (EDCA backoff procedure).
    auto hasTransmitted = link.startTxop.has_value() && Simulator::Now() > *link.startTxop;

    m_queue->WipeAllExpiredMpdus();
    if ((hasTransmitted) ||
        (!m_queue->IsEmpty() && m_mac->GetChannelAccessManager(linkId)->GetGenerateBackoffOnNoTx()))
    {
        GenerateBackoff(linkId);
        if (!m_queue->IsEmpty())
        {
            Simulator::ScheduleNow(&QosTxop::RequestAccess, this, linkId);
        }
    }
    link.startTxop.reset();
    GetLink(linkId).access = NOT_REQUESTED;
}

Time
QosTxop::GetRemainingTxop(uint8_t linkId) const
{
    auto& link = GetLink(linkId);
    NS_ASSERT(link.startTxop.has_value());

    Time remainingTxop = link.txopDuration;
    remainingTxop -= (Simulator::Now() - *link.startTxop);
    if (remainingTxop.IsStrictlyNegative())
    {
        remainingTxop = Seconds(0);
    }
    NS_LOG_FUNCTION(this << remainingTxop);
    return remainingTxop;
}

void
QosTxop::GotAddBaResponse(const MgtAddBaResponseHeader& respHdr, Mac48Address recipient)
{
    NS_LOG_FUNCTION(this << respHdr << recipient);
    uint8_t tid = respHdr.GetTid();

    if (respHdr.GetStatusCode().IsSuccess())
    {
        NS_LOG_DEBUG("block ack agreement established with " << recipient << " tid " << +tid);
        // A (destination, TID) pair is "blocked" (i.e., no more packets are sent)
        // when an Add BA Request is sent to the destination. However, when the
        // Add BA Request timer expires, the (destination, TID) pair is "unblocked"
        // and packets to the destination are sent again (under normal ack policy).
        // Thus, there may be a packet needing to be retransmitted when the
        // Add BA Response is received. In this case, the starting sequence number
        // shall be set equal to the sequence number of such packet.
        uint16_t startingSeq = m_txMiddle->GetNextSeqNumberByTidAndAddress(tid, recipient);
        auto peekedItem = m_queue->PeekByTidAndAddress(tid, recipient);
        if (peekedItem && peekedItem->GetHeader().IsRetry())
        {
            startingSeq = peekedItem->GetHeader().GetSequenceNumber();
        }
        m_baManager->UpdateOriginatorAgreement(respHdr, recipient, startingSeq);
    }
    else
    {
        NS_LOG_DEBUG("discard ADDBA response" << recipient);
        m_baManager->NotifyOriginatorAgreementRejected(recipient, tid);
    }
}

void
QosTxop::GotDelBaFrame(const MgtDelBaHeader* delBaHdr, Mac48Address recipient)
{
    NS_LOG_FUNCTION(this << delBaHdr << recipient);
    NS_LOG_DEBUG("received DELBA frame from=" << recipient);
    m_baManager->DestroyOriginatorAgreement(recipient, delBaHdr->GetTid());
}

void
QosTxop::NotifyOriginatorAgreementNoReply(const Mac48Address& recipient, uint8_t tid)
{
    NS_LOG_FUNCTION(this << recipient << tid);
    m_baManager->NotifyOriginatorAgreementNoReply(recipient, tid);
}

void
QosTxop::CompleteMpduTx(Ptr<WifiMpdu> mpdu)
{
    NS_ASSERT(mpdu->GetHeader().IsQosData());
    // If there is an established BA agreement, store the packet in the queue of outstanding packets
    if (m_mac->GetBaAgreementEstablishedAsOriginator(mpdu->GetHeader().GetAddr1(),
                                                     mpdu->GetHeader().GetQosTid()))
    {
        NS_ASSERT(mpdu->IsQueued());
        NS_ASSERT(m_queue->GetAc() == mpdu->GetQueueAc());
        m_baManager->StorePacket(m_queue->GetOriginal(mpdu));
    }
}

void
QosTxop::SetBlockAckThreshold(uint8_t threshold)
{
    NS_LOG_FUNCTION(this << +threshold);
    m_blockAckThreshold = threshold;
    m_baManager->SetBlockAckThreshold(threshold);
}

void
QosTxop::SetBlockAckInactivityTimeout(uint16_t timeout)
{
    NS_LOG_FUNCTION(this << timeout);
    m_blockAckInactivityTimeout = timeout;
}

uint8_t
QosTxop::GetBlockAckThreshold() const
{
    NS_LOG_FUNCTION(this);
    return m_blockAckThreshold;
}

uint16_t
QosTxop::GetBlockAckInactivityTimeout() const
{
    return m_blockAckInactivityTimeout;
}

void
QosTxop::AddBaResponseTimeout(Mac48Address recipient, uint8_t tid)
{
    NS_LOG_FUNCTION(this << recipient << +tid);
    // If agreement is still pending, ADDBA response is not received
    if (auto agreement = m_baManager->GetAgreementAsOriginator(recipient, tid);
        agreement && agreement->get().IsPending())
    {
        NotifyOriginatorAgreementNoReply(recipient, tid);
        Simulator::Schedule(m_failedAddBaTimeout, &QosTxop::ResetBa, this, recipient, tid);
    }
}

void
QosTxop::ResetBa(Mac48Address recipient, uint8_t tid)
{
    NS_LOG_FUNCTION(this << recipient << +tid);
    // This function is scheduled when waiting for an ADDBA response. However,
    // before this function is called, a DELBA request may arrive, which causes
    // the agreement to be deleted. Hence, check if an agreement exists before
    // notifying that the agreement has to be reset.
    if (auto agreement = m_baManager->GetAgreementAsOriginator(recipient, tid);
        agreement && !agreement->get().IsEstablished())
    {
        m_baManager->NotifyOriginatorAgreementReset(recipient, tid);
    }
}

void
QosTxop::SetAddBaResponseTimeout(Time addBaResponseTimeout)
{
    NS_LOG_FUNCTION(this << addBaResponseTimeout);
    m_addBaResponseTimeout = addBaResponseTimeout;
}

Time
QosTxop::GetAddBaResponseTimeout() const
{
    return m_addBaResponseTimeout;
}

void
QosTxop::SetFailedAddBaTimeout(Time failedAddBaTimeout)
{
    NS_LOG_FUNCTION(this << failedAddBaTimeout);
    m_failedAddBaTimeout = failedAddBaTimeout;
}

Time
QosTxop::GetFailedAddBaTimeout() const
{
    return m_failedAddBaTimeout;
}

bool
QosTxop::IsQosTxop() const
{
    return true;
}

AcIndex
QosTxop::GetAccessCategory() const
{
    return m_ac;
}

} // namespace ns3
