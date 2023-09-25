/*
 * Copyright (c) 2009, 2010 MIRKO BANCHI
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */

#include "block-ack-manager.h"

#include "ctrl-headers.h"
#include "mac-rx-middle.h"
#include "mgt-action-headers.h"
#include "qos-utils.h"
#include "wifi-mac-queue.h"
#include "wifi-tx-vector.h"
#include "wifi-utils.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <optional>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BlockAckManager");

NS_OBJECT_ENSURE_REGISTERED(BlockAckManager);

TypeId
BlockAckManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::BlockAckManager")
            .SetParent<Object>()
            .SetGroupName("Wifi")
            .AddConstructor<BlockAckManager>()
            .AddTraceSource("AgreementState",
                            "The state of the ADDBA handshake",
                            MakeTraceSourceAccessor(&BlockAckManager::m_originatorAgreementState),
                            "ns3::BlockAckManager::AgreementStateTracedCallback");
    return tid;
}

BlockAckManager::BlockAckManager()
{
    NS_LOG_FUNCTION(this);
}

BlockAckManager::~BlockAckManager()
{
    NS_LOG_FUNCTION(this);
}

void
BlockAckManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_originatorAgreements.clear();
    m_queue = nullptr;
}

BlockAckManager::OriginatorAgreementsI
BlockAckManager::GetOriginatorBaAgreement(const Mac48Address& recipient,
                                          uint8_t tid,
                                          std::optional<Mac48Address> gcrGroupAddr)
{
    auto [first, last] = m_originatorAgreements.equal_range({recipient, tid});
    for (auto it = first; it != last; ++it)
    {
        [[maybe_unused]] const auto& [baAgreement, packetQueue] = it->second;
        if (baAgreement.GetGcrGroupAddress() == gcrGroupAddr)
        {
            return it;
        }
    }
    return m_originatorAgreements.end();
}

BlockAckManager::OriginatorAgreementsCI
BlockAckManager::GetOriginatorBaAgreement(const Mac48Address& recipient,
                                          uint8_t tid,
                                          std::optional<Mac48Address> gcrGroupAddr) const
{
    const auto [first, last] = m_originatorAgreements.equal_range({recipient, tid});
    for (auto it = first; it != last; ++it)
    {
        [[maybe_unused]] const auto& [baAgreement, packetQueue] = it->second;
        if (baAgreement.GetGcrGroupAddress() == gcrGroupAddr)
        {
            return it;
        }
    }
    return m_originatorAgreements.cend();
}

BlockAckManager::RecipientAgreementsI
BlockAckManager::GetRecipientBaAgreement(const Mac48Address& originator,
                                         uint8_t tid,
                                         std::optional<Mac48Address> gcrGroupAddr)
{
    auto [first, last] = m_recipientAgreements.equal_range({originator, tid});
    for (auto it = first; it != last; ++it)
    {
        if (it->second.GetGcrGroupAddress() == gcrGroupAddr)
        {
            return it;
        }
    }
    return m_recipientAgreements.end();
}

BlockAckManager::RecipientAgreementsCI
BlockAckManager::GetRecipientBaAgreement(const Mac48Address& originator,
                                         uint8_t tid,
                                         std::optional<Mac48Address> gcrGroupAddr) const
{
    const auto [first, last] = m_recipientAgreements.equal_range({originator, tid});
    for (auto it = first; it != last; ++it)
    {
        if (it->second.GetGcrGroupAddress() == gcrGroupAddr)
        {
            return it;
        }
    }
    return m_recipientAgreements.cend();
}

BlockAckManager::OriginatorAgreementOptConstRef
BlockAckManager::GetAgreementAsOriginator(const Mac48Address& recipient,
                                          uint8_t tid,
                                          std::optional<Mac48Address> gcrGroupAddr) const
{
    if (const auto it = GetOriginatorBaAgreement(recipient, tid, gcrGroupAddr);
        it != m_originatorAgreements.cend())
    {
        return std::cref(it->second.first);
    }
    return std::nullopt;
}

BlockAckManager::RecipientAgreementOptConstRef
BlockAckManager::GetAgreementAsRecipient(const Mac48Address& originator,
                                         uint8_t tid,
                                         std::optional<Mac48Address> gcrGroupAddr) const
{
    if (const auto it = GetRecipientBaAgreement(originator, tid, gcrGroupAddr);
        it != m_recipientAgreements.end())
    {
        return std::cref(it->second);
    }
    return std::nullopt;
}

void
BlockAckManager::CreateOriginatorAgreement(const MgtAddBaRequestHeader& reqHdr,
                                           const Mac48Address& recipient)
{
    NS_LOG_FUNCTION(this << reqHdr << recipient);
    const auto tid = reqHdr.GetTid();

    OriginatorBlockAckAgreement agreement(recipient, tid);
    agreement.SetStartingSequence(reqHdr.GetStartingSequence());
    /* For now we assume that originator doesn't use this field. Use of this field
       is mandatory only for recipient */
    agreement.SetBufferSize(reqHdr.GetBufferSize());
    agreement.SetTimeout(reqHdr.GetTimeout());
    agreement.SetAmsduSupport(reqHdr.IsAmsduSupported());
    agreement.SetHtSupported(true);
    if (reqHdr.IsImmediateBlockAck())
    {
        agreement.SetImmediateBlockAck();
    }
    else
    {
        agreement.SetDelayedBlockAck();
    }
    if (const auto gcrGroupAddr = reqHdr.GetGcrGroupAddress())
    {
        agreement.SetGcrGroupAddress(*gcrGroupAddr);
    }
    agreement.SetState(OriginatorBlockAckAgreement::PENDING);

    m_originatorAgreementState(Simulator::Now(),
                               recipient,
                               tid,
                               OriginatorBlockAckAgreement::PENDING);

    if (auto it = GetOriginatorBaAgreement(recipient, tid, reqHdr.GetGcrGroupAddress());
        it != m_originatorAgreements.end())
    {
        NS_ASSERT_MSG(it->second.first.IsReset(), "Existing agreement must be in RESET state");
        it->second = std::make_pair(std::move(agreement), PacketQueue{});
    }
    else
    {
        m_originatorAgreements.emplace(std::make_pair(recipient, tid),
                                       std::make_pair(std::move(agreement), PacketQueue{}));
    }

    const auto [first, last] = m_originatorAgreements.equal_range({recipient, tid});
    NS_ASSERT_MSG(std::count_if(first,
                                last,
                                [&reqHdr](const auto& elem) {
                                    return elem.second.first.GetGcrGroupAddress() ==
                                           reqHdr.GetGcrGroupAddress();
                                }) == 1,
                  "There exists more than one "
                      << (reqHdr.GetGcrGroupAddress().has_value() ? "GCR " : " ")
                      << "Block Ack agreement for recipient " << recipient << " and tid " << +tid);

    m_blockPackets(reqHdr.GetGcrGroupAddress().value_or(recipient), tid);
}

void
BlockAckManager::DestroyOriginatorAgreement(const Mac48Address& recipient,
                                            uint8_t tid,
                                            std::optional<Mac48Address> gcrGroupAddr)
{
    NS_LOG_FUNCTION(this << recipient << tid << gcrGroupAddr.has_value());
    if (auto it = GetOriginatorBaAgreement(recipient, tid, gcrGroupAddr);
        it != m_originatorAgreements.end())
    {
        m_originatorAgreements.erase(it);
    }
}

void
BlockAckManager::UpdateOriginatorAgreement(const MgtAddBaResponseHeader& respHdr,
                                           const Mac48Address& recipient,
                                           uint16_t startingSeq)
{
    NS_LOG_FUNCTION(this << respHdr << recipient << startingSeq);
    const auto tid = respHdr.GetTid();
    if (auto it = GetOriginatorBaAgreement(recipient, tid, respHdr.GetGcrGroupAddress());
        it != m_originatorAgreements.end())
    {
        OriginatorBlockAckAgreement& agreement = it->second.first;
        agreement.SetBufferSize(respHdr.GetBufferSize());
        agreement.SetTimeout(respHdr.GetTimeout());
        agreement.SetAmsduSupport(respHdr.IsAmsduSupported());
        agreement.SetStartingSequence(startingSeq);
        agreement.InitTxWindow();
        if (respHdr.IsImmediateBlockAck())
        {
            agreement.SetImmediateBlockAck();
        }
        else
        {
            agreement.SetDelayedBlockAck();
        }
        if (const auto gcrGroupAddr = respHdr.GetGcrGroupAddress())
        {
            agreement.SetGcrGroupAddress(*gcrGroupAddr);
            m_gcrBlockAcks.emplace(*gcrGroupAddr, GcrBlockAcks{});
        }
        if (!it->second.first.IsEstablished())
        {
            m_originatorAgreementState(Simulator::Now(),
                                       recipient,
                                       tid,
                                       OriginatorBlockAckAgreement::ESTABLISHED);
        }
        agreement.SetState(OriginatorBlockAckAgreement::ESTABLISHED);
        if (agreement.GetTimeout() != 0)
        {
            Time timeout = MicroSeconds(1024 * agreement.GetTimeout());
            agreement.m_inactivityEvent = Simulator::Schedule(timeout,
                                                              &BlockAckManager::InactivityTimeout,
                                                              this,
                                                              recipient,
                                                              tid,
                                                              respHdr.GetGcrGroupAddress());
        }
    }
    if (!respHdr.GetGcrGroupAddress().has_value())
    {
        m_unblockPackets(recipient, tid);
    }
    else
    {
        for (const auto& agreement : m_originatorAgreements)
        {
            if (agreement.second.first.GetGcrGroupAddress() != respHdr.GetGcrGroupAddress())
            {
                continue;
            }
            if (!agreement.second.first.IsEstablished())
            {
                return;
            }
        }
        // established with all members so we can unblock
        m_unblockPackets(respHdr.GetGcrGroupAddress().value(), tid);
    }
}

void
BlockAckManager::CreateRecipientAgreement(const MgtAddBaResponseHeader& respHdr,
                                          const Mac48Address& originator,
                                          uint16_t startingSeq,
                                          Ptr<MacRxMiddle> rxMiddle)
{
    NS_LOG_FUNCTION(this << respHdr << originator << startingSeq << rxMiddle);
    const auto tid = respHdr.GetTid();

    RecipientBlockAckAgreement agreement(originator,
                                         respHdr.IsAmsduSupported(),
                                         tid,
                                         respHdr.GetBufferSize(),
                                         respHdr.GetTimeout(),
                                         startingSeq,
                                         true);

    agreement.SetMacRxMiddle(rxMiddle);
    if (respHdr.IsImmediateBlockAck())
    {
        agreement.SetImmediateBlockAck();
    }
    else
    {
        agreement.SetDelayedBlockAck();
    }
    if (const auto gcrGroupAddr = respHdr.GetGcrGroupAddress())
    {
        agreement.SetGcrGroupAddress(*gcrGroupAddr);
    }

    if (auto it = GetRecipientBaAgreement(originator, tid, respHdr.GetGcrGroupAddress());
        it != m_recipientAgreements.end())
    {
        it->second = std::move(agreement);
    }
    else
    {
        m_recipientAgreements.emplace(std::make_pair(originator, tid), std::move(agreement));
    }

    const auto [first, last] = m_recipientAgreements.equal_range({originator, tid});
    NS_ASSERT_MSG(
        std::count_if(first,
                      last,
                      [&respHdr](const auto& elem) {
                          return elem.second.GetGcrGroupAddress() == respHdr.GetGcrGroupAddress();
                      }) == 1,
        "There exists more than one " << (respHdr.GetGcrGroupAddress().has_value() ? "GCR " : " ")
                                      << "Block Ack agreement for originator " << originator
                                      << " and tid " << +tid);
}

void
BlockAckManager::DestroyRecipientAgreement(const Mac48Address& originator,
                                           uint8_t tid,
                                           std::optional<Mac48Address> gcrGroupAddr)
{
    NS_LOG_FUNCTION(this << originator << tid << gcrGroupAddr.has_value());
    if (auto it = GetRecipientBaAgreement(originator, tid, gcrGroupAddr);
        it != m_recipientAgreements.end())
    {
        // forward up the buffered MPDUs before destroying the agreement
        it->second.Flush();
        m_recipientAgreements.erase(it);
    }
}

void
BlockAckManager::StorePacket(Ptr<WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << *mpdu);
    DoStorePacket(mpdu, mpdu->GetHeader().GetAddr1());
}

void
BlockAckManager::StoreGcrPacket(Ptr<WifiMpdu> mpdu, const GcrManager::GcrMembers& members)
{
    NS_LOG_FUNCTION(this << *mpdu << members.size());
    for (const auto& member : members)
    {
        DoStorePacket(mpdu, member, mpdu->begin()->second.GetDestinationAddr());
    }
}

void
BlockAckManager::DoStorePacket(Ptr<WifiMpdu> mpdu,
                               const Mac48Address& recipient,
                               std::optional<Mac48Address> gcrGroupAddr)
{
    NS_LOG_FUNCTION(this << *mpdu << recipient << gcrGroupAddr.has_value());
    NS_ASSERT(mpdu->GetHeader().IsQosData());

    const auto tid = mpdu->GetHeader().GetQosTid();
    auto agreementIt = GetOriginatorBaAgreement(recipient, tid, gcrGroupAddr);
    NS_ASSERT(agreementIt != m_originatorAgreements.end());

    const auto mpduDist =
        agreementIt->second.first.GetDistance(mpdu->GetHeader().GetSequenceNumber());

    if (mpduDist >= SEQNO_SPACE_HALF_SIZE)
    {
        NS_LOG_DEBUG("Got an old packet. Do nothing");
        return;
    }

    // store the packet and keep the list sorted in increasing order of sequence number
    // with respect to the starting sequence number
    auto it = agreementIt->second.second.rbegin();
    while (it != agreementIt->second.second.rend())
    {
        if (mpdu->GetHeader().GetSequenceControl() == (*it)->GetHeader().GetSequenceControl())
        {
            NS_LOG_DEBUG("Packet already in the queue of the BA agreement");
            return;
        }

        const auto dist =
            agreementIt->second.first.GetDistance((*it)->GetHeader().GetSequenceNumber());

        if (mpduDist > dist || (mpduDist == dist && mpdu->GetHeader().GetFragmentNumber() >
                                                        (*it)->GetHeader().GetFragmentNumber()))
        {
            break;
        }

        it++;
    }
    agreementIt->second.second.insert(it.base(), mpdu);
    agreementIt->second.first.NotifyTransmittedMpdu(mpdu);
}

uint32_t
BlockAckManager::GetNBufferedPackets(const Mac48Address& recipient, uint8_t tid) const
{
    const auto it = GetOriginatorBaAgreement(recipient, tid);
    return (it != m_originatorAgreements.cend()) ? it->second.second.size() : 0;
}

void
BlockAckManager::SetBlockAckThreshold(uint8_t nPackets)
{
    NS_LOG_FUNCTION(this << nPackets);
    m_blockAckThreshold = nPackets;
}

BlockAckManager::PacketQueueI
BlockAckManager::HandleInFlightMpdu(uint8_t linkId,
                                    PacketQueueI mpduIt,
                                    MpduStatus status,
                                    const OriginatorAgreementsI& it,
                                    const Time& now)
{
    NS_LOG_FUNCTION(this << linkId << **mpduIt << static_cast<uint8_t>(status));

    if (!(*mpduIt)->IsQueued())
    {
        // MPDU is not in the EDCA queue (e.g., its lifetime expired and it was
        // removed by another method), remove from the queue of in flight MPDUs
        NS_LOG_DEBUG("MPDU is not stored in the EDCA queue, drop MPDU");
        return it->second.second.erase(mpduIt);
    }

    if (status == ACKNOWLEDGED)
    {
        // the MPDU has to be dequeued from the EDCA queue
        return it->second.second.erase(mpduIt);
    }

    const WifiMacHeader& hdr = (*mpduIt)->GetHeader();

    NS_ASSERT((hdr.GetAddr1() == it->first.first) || hdr.GetAddr1().IsGroup());
    NS_ASSERT(hdr.IsQosData() && hdr.GetQosTid() == it->first.second);

    if (it->second.first.GetDistance(hdr.GetSequenceNumber()) >= SEQNO_SPACE_HALF_SIZE)
    {
        NS_LOG_DEBUG("Old packet. Remove from the EDCA queue, too");
        NS_ASSERT(!m_droppedOldMpduCallback.IsNull());
        m_droppedOldMpduCallback(*mpduIt);
        m_queue->Remove(*mpduIt);
        return it->second.second.erase(mpduIt);
    }

    std::optional<PacketQueueI> prevIt;
    if (mpduIt != it->second.second.begin())
    {
        prevIt = std::prev(mpduIt);
    }

    if (m_queue->TtlExceeded(*mpduIt, now))
    {
        // WifiMacQueue::TtlExceeded() has removed the MPDU from the EDCA queue
        // and fired the Expired trace source, which called NotifyDiscardedMpdu,
        // which removed this MPDU (and possibly others) from the in flight queue as well
        NS_LOG_DEBUG("MSDU lifetime expired, drop MPDU");
        return (prevIt.has_value() ? std::next(prevIt.value()) : it->second.second.begin());
    }

    if (status == STAY_INFLIGHT)
    {
        // the MPDU has to stay in flight, do nothing
        return ++mpduIt;
    }

    NS_ASSERT(status == TO_RETRANSMIT);
    (*mpduIt)->GetHeader().SetRetry();
    (*mpduIt)->ResetInFlight(linkId); // no longer in flight; will be if retransmitted

    return it->second.second.erase(mpduIt);
}

void
BlockAckManager::NotifyGotAck(uint8_t linkId, Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << linkId << *mpdu);
    NS_ASSERT(mpdu->GetHeader().IsQosData());

    const auto recipient = mpdu->GetOriginal()->GetHeader().GetAddr1();
    const auto tid = mpdu->GetHeader().GetQosTid();

    auto it = GetOriginatorBaAgreement(recipient, tid);
    NS_ASSERT(it != m_originatorAgreements.end());
    NS_ASSERT(it->second.first.IsEstablished());

    it->second.first.NotifyAckedMpdu(mpdu);
    if (!m_txOkCallback.IsNull())
    {
        m_txOkCallback(mpdu);
    }

    // remove the acknowledged frame from the queue of outstanding packets
    for (auto queueIt = it->second.second.begin(); queueIt != it->second.second.end(); ++queueIt)
    {
        if ((*queueIt)->GetHeader().GetSequenceNumber() == mpdu->GetHeader().GetSequenceNumber())
        {
            m_queue->DequeueIfQueued({*queueIt});
            HandleInFlightMpdu(linkId, queueIt, ACKNOWLEDGED, it, Simulator::Now());
            break;
        }
    }
}

void
BlockAckManager::NotifyLastGcrUrTx(Ptr<const WifiMpdu> mpdu,
                                   const GcrManager::GcrMembers& recipients)
{
    NS_LOG_FUNCTION(this << *mpdu << recipients.size());
    NS_ASSERT(mpdu->GetHeader().IsQosData());
    const auto tid = mpdu->GetHeader().GetQosTid();
    const auto gcrGroupAddr = mpdu->GetHeader().GetAddr1();
    for (const auto& recipient : recipients)
    {
        auto it = GetOriginatorBaAgreement(recipient, tid, gcrGroupAddr);
        NS_ASSERT(it != m_originatorAgreements.end());
        NS_ASSERT(it->second.first.IsEstablished());
        it->second.first.NotifyAckedMpdu(mpdu);
    }
}

void
BlockAckManager::NotifyMissedAck(uint8_t linkId, Ptr<WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << linkId << *mpdu);
    NS_ASSERT(mpdu->GetHeader().IsQosData());

    const auto recipient = mpdu->GetOriginal()->GetHeader().GetAddr1();
    const auto tid = mpdu->GetHeader().GetQosTid();

    auto it = GetOriginatorBaAgreement(recipient, tid);
    NS_ASSERT(it != m_originatorAgreements.end());
    NS_ASSERT(it->second.first.IsEstablished());

    // remove the frame from the queue of outstanding packets (it will be re-inserted
    // if retransmitted)
    for (auto queueIt = it->second.second.begin(); queueIt != it->second.second.end(); ++queueIt)
    {
        if ((*queueIt)->GetHeader().GetSequenceNumber() == mpdu->GetHeader().GetSequenceNumber())
        {
            HandleInFlightMpdu(linkId, queueIt, TO_RETRANSMIT, it, Simulator::Now());
            break;
        }
    }
}

std::pair<uint16_t, uint16_t>
BlockAckManager::NotifyGotBlockAck(uint8_t linkId,
                                   const CtrlBAckResponseHeader& blockAck,
                                   const Mac48Address& recipient,
                                   const std::set<uint8_t>& tids,
                                   size_t index)
{
    NS_LOG_FUNCTION(this << linkId << blockAck << recipient << index);

    NS_ABORT_MSG_IF(blockAck.IsBasic(), "Basic Block Ack is not supported");
    NS_ABORT_MSG_IF(blockAck.IsMultiTid(), "Multi-TID Block Ack is not supported");

    uint8_t tid = blockAck.GetTidInfo(index);
    // If this is a Multi-STA Block Ack with All-ack context (TID equal to 14),
    // use the TID passed by the caller.
    if (tid == 14)
    {
        NS_ASSERT(blockAck.GetAckType(index) && tids.size() == 1);
        tid = *tids.begin();
    }

    auto it = GetOriginatorBaAgreement(recipient, tid);
    if (it == m_originatorAgreements.end() || !it->second.first.IsEstablished())
    {
        return {0, 0};
    }

    uint16_t nSuccessfulMpdus = 0;
    uint16_t nFailedMpdus = 0;

    if (it->second.first.m_inactivityEvent.IsPending())
    {
        /* Upon reception of a BlockAck frame, the inactivity timer at the
            originator must be reset.
            For more details see section 11.5.3 in IEEE802.11e standard */
        it->second.first.m_inactivityEvent.Cancel();
        Time timeout = MicroSeconds(1024 * it->second.first.GetTimeout());
        it->second.first.m_inactivityEvent =
            Simulator::Schedule(timeout,
                                &BlockAckManager::InactivityTimeout,
                                this,
                                recipient,
                                tid,
                                std::nullopt);
    }

    NS_ASSERT(blockAck.IsCompressed() || blockAck.IsExtendedCompressed() || blockAck.IsMultiSta());
    Time now = Simulator::Now();
    std::list<Ptr<const WifiMpdu>> acked;

    for (auto queueIt = it->second.second.begin(); queueIt != it->second.second.end();)
    {
        uint16_t currentSeq = (*queueIt)->GetHeader().GetSequenceNumber();
        NS_LOG_DEBUG("Current seq=" << currentSeq);
        if (blockAck.IsPacketReceived(currentSeq, index))
        {
            it->second.first.NotifyAckedMpdu(*queueIt);
            nSuccessfulMpdus++;
            if (!m_txOkCallback.IsNull())
            {
                m_txOkCallback(*queueIt);
            }
            acked.emplace_back(*queueIt);
            queueIt = HandleInFlightMpdu(linkId, queueIt, ACKNOWLEDGED, it, now);
        }
        else
        {
            ++queueIt;
        }
    }

    // Dequeue all acknowledged MPDUs at once
    m_queue->DequeueIfQueued(acked);

    // Remaining outstanding MPDUs have not been acknowledged
    for (auto queueIt = it->second.second.begin(); queueIt != it->second.second.end();)
    {
        // transmission actually failed if the MPDU is inflight only on the same link on
        // which we received the BlockAck frame
        auto linkIds = (*queueIt)->GetInFlightLinkIds();

        if (linkIds.size() == 1 && *linkIds.begin() == linkId)
        {
            nFailedMpdus++;
            if (!m_txFailedCallback.IsNull())
            {
                m_txFailedCallback(*queueIt);
            }
            queueIt = HandleInFlightMpdu(linkId, queueIt, TO_RETRANSMIT, it, now);
            continue;
        }

        queueIt = HandleInFlightMpdu(linkId, queueIt, STAY_INFLIGHT, it, now);
    }

    return {nSuccessfulMpdus, nFailedMpdus};
}

std::optional<std::pair<uint16_t, uint16_t>>
BlockAckManager::NotifyGotGcrBlockAck(uint8_t linkId,
                                      const CtrlBAckResponseHeader& blockAck,
                                      const Mac48Address& recipient,
                                      const GcrManager::GcrMembers& members)
{
    NS_LOG_FUNCTION(this << linkId << blockAck << recipient);
    NS_ABORT_MSG_IF(!blockAck.IsGcr(), "GCR Block Ack is expected");
    NS_ABORT_MSG_IF(members.count(recipient) == 0,
                    "Received GCR Block Ack response from unexpected recipient");

    const auto tid = blockAck.GetTidInfo();
    auto it = GetOriginatorBaAgreement(recipient, tid, blockAck.GetGcrGroupAddress());
    if (it == m_originatorAgreements.end() || !it->second.first.IsEstablished())
    {
        return {};
    }

    NS_ASSERT_MSG(it->second.first.GetGcrGroupAddress().has_value() &&
                      it->second.first.GetGcrGroupAddress().value() ==
                          blockAck.GetGcrGroupAddress(),
                  "No GCR agreement for group address " << blockAck.GetGcrGroupAddress());
    if (it->second.first.m_inactivityEvent.IsPending())
    {
        /* Upon reception of a BlockAck frame, the inactivity timer at the
            originator must be reset.
            For more details see section 11.5.3 in IEEE802.11e standard */
        it->second.first.m_inactivityEvent.Cancel();
        Time timeout = MicroSeconds(1024 * it->second.first.GetTimeout());
        it->second.first.m_inactivityEvent =
            Simulator::Schedule(timeout,
                                &BlockAckManager::InactivityTimeout,
                                this,
                                recipient,
                                tid,
                                blockAck.GetGcrGroupAddress());
    }

    auto itGcrBlockAcks = m_gcrBlockAcks.find(blockAck.GetGcrGroupAddress());
    NS_ASSERT(itGcrBlockAcks != m_gcrBlockAcks.end());
    NS_ASSERT(itGcrBlockAcks->second.count(recipient) == 0);
    itGcrBlockAcks->second[recipient] = blockAck;

    if (itGcrBlockAcks->second.size() < members.size())
    {
        // we need to collect feedback from all members
        NS_LOG_DEBUG("Expecting more GCR Block ACK(s)");
        return {};
    }

    std::vector<bool> acked;
    for (auto queueIt = it->second.second.begin(); queueIt != it->second.second.end(); ++queueIt)
    {
        auto currentSeq = (*queueIt)->GetHeader().GetSequenceNumber();
        NS_LOG_DEBUG("Current seq=" << currentSeq);
        auto received = true;
        for ([[maybe_unused]] const auto& [recipient, gcrBlockAcks] : itGcrBlockAcks->second)
        {
            received &= gcrBlockAcks.IsPacketReceived(currentSeq, 0);
        }
        acked.emplace_back(received);
    }

    uint16_t nSuccessfulMpdus = 0;
    uint16_t nFailedMpdus = 0;
    const auto now = Simulator::Now();
    std::list<Ptr<const WifiMpdu>> ackedMpdus;
    auto countAndNotify = true;
    for (const auto& member : members)
    {
        std::size_t index = 0;
        it = GetOriginatorBaAgreement(member, tid, blockAck.GetGcrGroupAddress());
        NS_ASSERT(acked.size() == it->second.second.size());
        for (auto queueIt = it->second.second.begin(); queueIt != it->second.second.end();)
        {
            if (acked.at(index++))
            {
                it->second.first.NotifyAckedMpdu(*queueIt);
                if (countAndNotify)
                {
                    nSuccessfulMpdus++;
                    if (!m_txOkCallback.IsNull())
                    {
                        m_txOkCallback(*queueIt);
                    }
                    ackedMpdus.emplace_back(*queueIt);
                }
                queueIt = HandleInFlightMpdu(linkId, queueIt, ACKNOWLEDGED, it, now);
            }
            else
            {
                ++queueIt;
            }
        }
        countAndNotify = false;
    }

    // Dequeue all acknowledged MPDUs at once
    m_queue->DequeueIfQueued(ackedMpdus);

    // Remaining outstanding MPDUs have not been acknowledged
    countAndNotify = true;
    for (const auto& member : members)
    {
        it = GetOriginatorBaAgreement(member, tid, blockAck.GetGcrGroupAddress());
        for (auto queueIt = it->second.second.begin(); queueIt != it->second.second.end();)
        {
            // transmission actually failed if the MPDU is inflight only on the same link on
            // which we received the BlockAck frame
            auto linkIds = (*queueIt)->GetInFlightLinkIds();

            if (linkIds.size() == 1 && *linkIds.begin() == linkId)
            {
                if (countAndNotify)
                {
                    nFailedMpdus++;
                    if (!m_txFailedCallback.IsNull())
                    {
                        m_txFailedCallback(*queueIt);
                    }
                }
                queueIt = HandleInFlightMpdu(linkId, queueIt, TO_RETRANSMIT, it, now);
                continue;
            }

            queueIt = HandleInFlightMpdu(linkId, queueIt, STAY_INFLIGHT, it, now);
        }
        countAndNotify = false;
    }

    itGcrBlockAcks->second.clear();
    return std::make_pair(nSuccessfulMpdus, nFailedMpdus);
}

void
BlockAckManager::NotifyMissedBlockAck(uint8_t linkId, const Mac48Address& recipient, uint8_t tid)
{
    NS_LOG_FUNCTION(this << linkId << recipient << tid);

    auto it = GetOriginatorBaAgreement(recipient, tid);
    if (it == m_originatorAgreements.end() || !it->second.first.IsEstablished())
    {
        return;
    }

    const auto now = Simulator::Now();

    // remove all packets from the queue of outstanding packets (they will be
    // re-inserted if retransmitted)
    for (auto mpduIt = it->second.second.begin(); mpduIt != it->second.second.end();)
    {
        // MPDUs that were transmitted on another link shall stay inflight
        auto linkIds = (*mpduIt)->GetInFlightLinkIds();
        if (!linkIds.contains(linkId))
        {
            mpduIt = HandleInFlightMpdu(linkId, mpduIt, STAY_INFLIGHT, it, now);
            continue;
        }
        mpduIt = HandleInFlightMpdu(linkId, mpduIt, TO_RETRANSMIT, it, now);
    }
}

void
BlockAckManager::NotifyDiscardedMpdu(Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << *mpdu);
    if (!mpdu->GetHeader().IsQosData())
    {
        NS_LOG_DEBUG("Not a QoS Data frame");
        return;
    }

    if (!mpdu->GetHeader().IsRetry() && !mpdu->IsInFlight())
    {
        NS_LOG_DEBUG("This frame has never been transmitted");
        return;
    }

    const auto recipient = mpdu->GetOriginal()->GetHeader().GetAddr1();
    const auto tid = mpdu->GetHeader().GetQosTid();
    if (!recipient.IsGroup())
    {
        auto it = GetOriginatorBaAgreement(recipient, tid);
        HandleDiscardedMpdu(mpdu, it);
    }
    else
    {
        const auto groupAddress = mpdu->GetOriginal()->GetHeader().GetAddr1();
        for (auto it = m_originatorAgreements.begin(); it != m_originatorAgreements.end(); ++it)
        {
            if (it->first.second != tid || !it->second.first.GetGcrGroupAddress().has_value() ||
                it->second.first.GetGcrGroupAddress().value() != groupAddress)
            {
                continue;
            }
            HandleDiscardedMpdu(mpdu, it);
        }
    }
}

void
BlockAckManager::HandleDiscardedMpdu(Ptr<const WifiMpdu> mpdu, OriginatorAgreementsI iter)
{
    if (iter == m_originatorAgreements.end() || !iter->second.first.IsEstablished())
    {
        NS_LOG_DEBUG("No established Block Ack agreement");
        return;
    }

    auto& [baAgreement, packetQueue] = iter->second;
    if (const auto currStartingSeq = baAgreement.GetStartingSequence();
        QosUtilsIsOldPacket(currStartingSeq, mpdu->GetHeader().GetSequenceNumber()))
    {
        NS_LOG_DEBUG("Discarded an old frame");
        return;
    }

    // actually advance the transmit window
    baAgreement.NotifyDiscardedMpdu(mpdu);

    // remove old MPDUs from the EDCA queue and from the in flight queue
    // (including the given MPDU which became old after advancing the transmit window)
    for (auto mpduIt = iter->second.second.begin(); mpduIt != iter->second.second.end();)
    {
        if (baAgreement.GetDistance((*mpduIt)->GetHeader().GetSequenceNumber()) >=
            SEQNO_SPACE_HALF_SIZE)
        {
            NS_LOG_DEBUG("Dropping old MPDU: " << **mpduIt);
            m_queue->DequeueIfQueued({*mpduIt});
            if (!m_droppedOldMpduCallback.IsNull())
            {
                m_droppedOldMpduCallback(*mpduIt);
            }
            mpduIt = packetQueue.erase(mpduIt);
        }
        else
        {
            break; // MPDUs are in increasing order of sequence number in the in flight queue
        }
    }

    // schedule a BlockAckRequest
    const auto [recipient, tid] = iter->first;
    NS_LOG_DEBUG("Schedule a Block Ack Request for agreement (" << recipient << ", " << +tid
                                                                << ")");

    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_CTL_BACKREQ);
    hdr.SetAddr1(recipient);
    hdr.SetAddr2(mpdu->GetOriginal()->GetHeader().GetAddr2());
    hdr.SetDsNotTo();
    hdr.SetDsNotFrom();
    hdr.SetNoRetry();
    hdr.SetNoMoreFragments();

    ScheduleBar(GetBlockAckReqHeader(recipient, tid, baAgreement.GetGcrGroupAddress()), hdr);
}

void
BlockAckManager::NotifyGotBlockAckRequest(const Mac48Address& originator,
                                          uint8_t tid,
                                          uint16_t startingSeq,
                                          std::optional<Mac48Address> gcrGroupAddr)
{
    NS_LOG_FUNCTION(this << originator << tid << startingSeq << gcrGroupAddr.has_value());
    if (auto it = GetRecipientBaAgreement(originator, tid, gcrGroupAddr);
        it != m_recipientAgreements.end())
    {
        it->second.NotifyReceivedBar(startingSeq);
    }
}

void
BlockAckManager::NotifyGotMpdu(Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << *mpdu);
    NS_ASSERT(mpdu->GetHeader().IsQosData());
    const auto originator = mpdu->GetOriginal()->GetHeader().GetAddr2();
    const auto tid = mpdu->GetHeader().GetQosTid();
    std::optional<Mac48Address> groupAddress;
    if (const auto addr1 = mpdu->GetOriginal()->GetHeader().GetAddr1(); addr1.IsGroup())
    {
        groupAddress =
            mpdu->GetHeader().IsQosAmsdu() ? mpdu->begin()->second.GetDestinationAddr() : addr1;
    }
    if (auto it = GetRecipientBaAgreement(originator, tid, groupAddress);
        it != m_recipientAgreements.end())
    {
        it->second.NotifyReceivedMpdu(mpdu);
    }
}

CtrlBAckRequestHeader
BlockAckManager::GetBlockAckReqHeader(const Mac48Address& recipient,
                                      uint8_t tid,
                                      std::optional<Mac48Address> gcrGroupAddr) const
{
    auto it = GetOriginatorBaAgreement(recipient, tid, gcrGroupAddr);
    NS_ASSERT(it != m_originatorAgreements.end());
    CtrlBAckRequestHeader reqHdr;
    if (gcrGroupAddr.has_value())
    {
        reqHdr.SetType(BlockAckReqType::GCR);
        reqHdr.SetGcrGroupAddress(gcrGroupAddr.value());
    }
    else
    {
        reqHdr.SetType((*it).second.first.GetBlockAckReqType());
    }
    reqHdr.SetTidInfo(tid);
    reqHdr.SetStartingSequence((*it).second.first.GetStartingSequence());
    return reqHdr;
}

void
BlockAckManager::ScheduleBar(const CtrlBAckRequestHeader& reqHdr, const WifiMacHeader& hdr)
{
    NS_LOG_FUNCTION(this << reqHdr << hdr);

    uint8_t tid = reqHdr.GetTidInfo();

    WifiContainerQueueId queueId(WIFI_CTL_QUEUE, WIFI_UNICAST, hdr.GetAddr1(), std::nullopt);
    auto pkt = Create<Packet>();
    pkt->AddHeader(reqHdr);
    Ptr<WifiMpdu> item = nullptr;

    // if a BAR for the given agreement is present, replace it with the new one
    while ((item = m_queue->PeekByQueueId(queueId, item)))
    {
        if (item->GetHeader().IsBlockAckReq() && item->GetHeader().GetAddr1() == hdr.GetAddr1())
        {
            CtrlBAckRequestHeader otherHdr;
            item->GetPacket()->PeekHeader(otherHdr);
            if (otherHdr.GetTidInfo() == tid)
            {
                auto bar = Create<WifiMpdu>(pkt, hdr, item->GetTimestamp());
                // replace item with bar
                m_queue->Replace(item, bar);
                return;
            }
        }
    }

    m_queue->Enqueue(Create<WifiMpdu>(pkt, hdr));
}

const std::list<BlockAckManager::AgreementKey>&
BlockAckManager::GetSendBarIfDataQueuedList() const
{
    return m_sendBarIfDataQueued;
}

void
BlockAckManager::AddToSendBarIfDataQueuedList(const Mac48Address& recipient, uint8_t tid)
{
    NS_LOG_FUNCTION(this << recipient << tid);
    // do nothing if the given pair is already in the list
    if (std::find(m_sendBarIfDataQueued.begin(),
                  m_sendBarIfDataQueued.end(),
                  BlockAckManager::AgreementKey{recipient, tid}) == m_sendBarIfDataQueued.end())
    {
        m_sendBarIfDataQueued.emplace_back(recipient, tid);
    }
}

void
BlockAckManager::RemoveFromSendBarIfDataQueuedList(const Mac48Address& recipient, uint8_t tid)
{
    NS_LOG_FUNCTION(this << recipient << tid);
    m_sendBarIfDataQueued.remove({recipient, tid});
}

void
BlockAckManager::InactivityTimeout(const Mac48Address& recipient,
                                   uint8_t tid,
                                   std::optional<Mac48Address> gcrGroupAddr)
{
    NS_LOG_FUNCTION(this << recipient << tid << gcrGroupAddr.has_value());
    m_blockAckInactivityTimeout(recipient, tid, true, gcrGroupAddr);
}

void
BlockAckManager::NotifyOriginatorAgreementRejected(const Mac48Address& recipient,
                                                   uint8_t tid,
                                                   std::optional<Mac48Address> gcrGroupAddr)
{
    NS_LOG_FUNCTION(this << recipient << tid << gcrGroupAddr.has_value());
    auto it = GetOriginatorBaAgreement(recipient, tid, gcrGroupAddr);
    NS_ASSERT(it != m_originatorAgreements.end());
    if (!it->second.first.IsRejected())
    {
        m_originatorAgreementState(Simulator::Now(),
                                   recipient,
                                   tid,
                                   OriginatorBlockAckAgreement::REJECTED);
    }
    it->second.first.SetState(OriginatorBlockAckAgreement::REJECTED);
    if (!gcrGroupAddr.has_value())
    {
        m_unblockPackets(recipient, tid);
    }
}

void
BlockAckManager::NotifyOriginatorAgreementNoReply(const Mac48Address& recipient,
                                                  uint8_t tid,
                                                  std::optional<Mac48Address> gcrGroupAddr)
{
    NS_LOG_FUNCTION(this << recipient << tid << gcrGroupAddr.has_value());
    auto it = GetOriginatorBaAgreement(recipient, tid, gcrGroupAddr);
    NS_ASSERT(it != m_originatorAgreements.end());
    if (!it->second.first.IsNoReply())
    {
        m_originatorAgreementState(Simulator::Now(),
                                   recipient,
                                   tid,
                                   OriginatorBlockAckAgreement::NO_REPLY);
    }
    it->second.first.SetState(OriginatorBlockAckAgreement::NO_REPLY);
    if (!gcrGroupAddr.has_value())
    {
        m_unblockPackets(recipient, tid);
    }
}

void
BlockAckManager::NotifyOriginatorAgreementReset(const Mac48Address& recipient,
                                                uint8_t tid,
                                                std::optional<Mac48Address> gcrGroupAddr)
{
    NS_LOG_FUNCTION(this << recipient << tid << gcrGroupAddr.has_value());
    auto it = GetOriginatorBaAgreement(recipient, tid, gcrGroupAddr);
    NS_ASSERT(it != m_originatorAgreements.end());
    if (!it->second.first.IsReset())
    {
        m_originatorAgreementState(Simulator::Now(),
                                   recipient,
                                   tid,
                                   OriginatorBlockAckAgreement::RESET);
    }
    it->second.first.SetState(OriginatorBlockAckAgreement::RESET);
    if (gcrGroupAddr.has_value())
    {
        m_unblockPackets(gcrGroupAddr.value(), tid);
    }
}

void
BlockAckManager::SetQueue(const Ptr<WifiMacQueue> queue)
{
    NS_LOG_FUNCTION(this << queue);
    m_queue = queue;
}

bool
BlockAckManager::NeedBarRetransmission(uint8_t tid, const Mac48Address& recipient)
{
    auto it = GetOriginatorBaAgreement(recipient, tid);
    if (it == m_originatorAgreements.end() || !it->second.first.IsEstablished())
    {
        // If the inactivity timer has expired, QosTxop::SendDelbaFrame has been called and
        // has destroyed the agreement, hence we get here and correctly return false
        return false;
    }

    const auto now = Simulator::Now();

    // A BAR needs to be retransmitted if there is at least a non-expired in flight MPDU
    for (auto mpduIt = it->second.second.begin(); mpduIt != it->second.second.end();)
    {
        // remove MPDU if old or with expired lifetime
        mpduIt = HandleInFlightMpdu(SINGLE_LINK_OP_ID, mpduIt, STAY_INFLIGHT, it, now);

        if (mpduIt != it->second.second.begin())
        {
            // the MPDU has not been removed
            return true;
        }
    }

    return false;
}

bool
BlockAckManager::NeedGcrBarRetransmission(const Mac48Address& gcrGroupAddress,
                                          const Mac48Address& recipient,
                                          uint8_t tid) const
{
    const auto it = GetOriginatorBaAgreement(recipient, tid, gcrGroupAddress);
    return (it != m_originatorAgreements.cend() && it->second.first.IsEstablished());
}

void
BlockAckManager::SetBlockAckInactivityCallback(
    Callback<void, Mac48Address, uint8_t, bool, std::optional<Mac48Address>> callback)
{
    NS_LOG_FUNCTION(this << &callback);
    m_blockAckInactivityTimeout = callback;
}

void
BlockAckManager::SetBlockDestinationCallback(Callback<void, Mac48Address, uint8_t> callback)
{
    NS_LOG_FUNCTION(this << &callback);
    m_blockPackets = callback;
}

void
BlockAckManager::SetUnblockDestinationCallback(Callback<void, Mac48Address, uint8_t> callback)
{
    NS_LOG_FUNCTION(this << &callback);
    m_unblockPackets = callback;
}

void
BlockAckManager::SetTxOkCallback(TxOk callback)
{
    m_txOkCallback = callback;
}

void
BlockAckManager::SetTxFailedCallback(TxFailed callback)
{
    m_txFailedCallback = callback;
}

void
BlockAckManager::SetDroppedOldMpduCallback(DroppedOldMpdu callback)
{
    m_droppedOldMpduCallback = callback;
}

uint16_t
BlockAckManager::GetRecipientBufferSize(const Mac48Address& recipient, uint8_t tid) const
{
    const auto it = GetOriginatorBaAgreement(recipient, tid);
    return (it != m_originatorAgreements.cend()) ? it->second.first.GetBufferSize() : 0;
}

uint16_t
BlockAckManager::GetOriginatorStartingSequence(const Mac48Address& recipient, uint8_t tid) const
{
    const auto it = GetOriginatorBaAgreement(recipient, tid);
    return (it != m_originatorAgreements.cend()) ? it->second.first.GetStartingSequence() : 0;
}

uint16_t
BlockAckManager::GetGcrStartingSequence(const Mac48Address& groupAddress, uint8_t tid) const
{
    uint16_t seqNum = 0;
    for (const auto& [key, pair] : m_originatorAgreements)
    {
        if (key.second != tid)
        {
            continue;
        }
        if (pair.first.GetGcrGroupAddress() == groupAddress)
        {
            seqNum = pair.first.GetStartingSequence();
            break;
        }
    }
    return seqNum;
}

uint16_t
BlockAckManager::GetGcrBufferSize(const Mac48Address& groupAddress, uint8_t tid) const
{
    /* The AP shall maintain a set of the most recently received values of the
       Buffer Size subfield from the Block Ack Parameter Set field in the ADDBA
       Response frame received from each member of a specific group address.
       The minimum of that set of values is defined to be the GCR buffer size
       for that group address. */
    uint16_t gcrBufferSize = std::numeric_limits<uint16_t>::max();
    for (const auto& [key, pair] : m_originatorAgreements)
    {
        if ((key.second == tid) && (pair.first.GetGcrGroupAddress() == groupAddress))
        {
            gcrBufferSize = std::min(pair.first.GetBufferSize(), gcrBufferSize);
        }
    }
    return gcrBufferSize;
}

bool
BlockAckManager::IsGcrAgreementEstablished(const Mac48Address& gcrGroupAddress,
                                           uint8_t tid,
                                           const GcrManager::GcrMembers& members) const
{
    NS_ASSERT(!members.empty());
    for (const auto& member : members)
    {
        if (const auto agreement = GetAgreementAsOriginator(member, tid, gcrGroupAddress);
            !agreement || !agreement->get().IsEstablished())
        {
            return false;
        }
    }
    return true;
}

} // namespace ns3
