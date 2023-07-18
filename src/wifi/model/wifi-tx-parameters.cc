/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-tx-parameters.h"

#include "mpdu-aggregator.h"
#include "msdu-aggregator.h"
#include "wifi-acknowledgment.h"
#include "wifi-mac-trailer.h"
#include "wifi-mpdu.h"
#include "wifi-protection.h"

#include "ns3/log.h"
#include "ns3/packet.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiTxParameters");

WifiTxParameters::WifiTxParameters()
{
}

WifiTxParameters::WifiTxParameters(const WifiTxParameters& txParams)
{
    m_txVector = txParams.m_txVector;
    m_protection = (txParams.m_protection ? txParams.m_protection->Copy() : nullptr);
    m_acknowledgment = (txParams.m_acknowledgment ? txParams.m_acknowledgment->Copy() : nullptr);
    m_txDuration = txParams.m_txDuration;
    m_info = txParams.m_info;
}

WifiTxParameters&
WifiTxParameters::operator=(const WifiTxParameters& txParams)
{
    // check for self-assignment
    if (&txParams == this)
    {
        return *this;
    }

    m_txVector = txParams.m_txVector;
    m_protection = (txParams.m_protection ? txParams.m_protection->Copy() : nullptr);
    m_acknowledgment = (txParams.m_acknowledgment ? txParams.m_acknowledgment->Copy() : nullptr);
    m_txDuration = txParams.m_txDuration;
    m_info = txParams.m_info;

    return *this;
}

void
WifiTxParameters::Clear()
{
    NS_LOG_FUNCTION(this);

    // Reset the current info
    m_info.clear();
    m_txVector = WifiTxVector();
    m_protection.reset(nullptr);
    m_acknowledgment.reset(nullptr);
    m_txDuration.reset();
}

const WifiTxParameters::PsduInfo*
WifiTxParameters::GetPsduInfo(Mac48Address receiver) const
{
    auto infoIt = m_info.find(receiver);

    if (infoIt == m_info.end())
    {
        return nullptr;
    }
    return &infoIt->second;
}

const WifiTxParameters::PsduInfoMap&
WifiTxParameters::GetPsduInfoMap() const
{
    return m_info;
}

void
WifiTxParameters::AddMpdu(Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << *mpdu);

    const WifiMacHeader& hdr = mpdu->GetHeader();

    auto infoIt = m_info.find(hdr.GetAddr1());

    if (infoIt == m_info.end())
    {
        // this is an MPDU starting a new PSDU
        std::map<uint8_t, std::set<uint16_t>> seqNumbers;
        if (hdr.IsQosData())
        {
            seqNumbers[hdr.GetQosTid()] = {hdr.GetSequenceNumber()};
        }

        // Insert the info about the given frame
        const auto [it, inserted] =
            m_info.emplace(hdr.GetAddr1(), PsduInfo{hdr, mpdu->GetPacketSize(), 0, seqNumbers});
        NS_ASSERT(inserted);

        // store information to undo the addition of this MPDU
        m_lastInfoIt = it;
        m_undoInfo = PsduInfo{WifiMacHeader{}, 0, 0, {}};
        return;
    }

    // a PSDU for the receiver of the given MPDU is already being built
    NS_ASSERT_MSG((hdr.IsQosData() && !hdr.HasData()) || infoIt->second.amsduSize > 0,
                  "An MPDU can only be aggregated to an existing (A-)MPDU");

    // store information to undo the addition of this MPDU
    m_lastInfoIt = infoIt;
    m_undoInfo =
        PsduInfo{infoIt->second.header, infoIt->second.amsduSize, infoIt->second.ampduSize, {}};
    if (hdr.IsQosData())
    {
        m_undoInfo.seqNumbers = {
            {hdr.GetQosTid(), {hdr.GetSequenceNumber()}}}; // seq number to remove
    }

    // The (A-)MSDU being built is included in an A-MPDU subframe
    infoIt->second.ampduSize = MpduAggregator::GetSizeIfAggregated(
        infoIt->second.header.GetSize() + infoIt->second.amsduSize + WIFI_MAC_FCS_LENGTH,
        infoIt->second.ampduSize);
    infoIt->second.header = hdr;
    infoIt->second.amsduSize = mpdu->GetPacketSize();

    if (hdr.IsQosData())
    {
        const auto [it, inserted] =
            infoIt->second.seqNumbers.emplace(hdr.GetQosTid(),
                                              std::set<uint16_t>{hdr.GetSequenceNumber()});

        if (!inserted)
        {
            // insertion did not happen because an entry with the same TID already exists
            it->second.insert(hdr.GetSequenceNumber());
        }
    }
}

void
WifiTxParameters::UndoAddMpdu()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_lastInfoIt.has_value());

    if (m_undoInfo.amsduSize == 0 && m_undoInfo.ampduSize == 0)
    {
        // the last MPDU was the first one being added for its receiver
        m_info.erase(*m_lastInfoIt);
        m_lastInfoIt.reset();
        return;
    }

    auto& lastInfo = (*m_lastInfoIt)->second;
    lastInfo.header = m_undoInfo.header;
    lastInfo.amsduSize = m_undoInfo.amsduSize;
    lastInfo.ampduSize = m_undoInfo.ampduSize;
    // if the MPDU to remove is not a QoS data frame or it is the first QoS data frame added for
    // a given receiver, no sequence number information is stored
    if (!m_undoInfo.seqNumbers.empty())
    {
        NS_ASSERT(m_undoInfo.seqNumbers.size() == 1);
        const auto tid = m_undoInfo.seqNumbers.cbegin()->first;
        auto& seqNoSet = m_undoInfo.seqNumbers.cbegin()->second;
        NS_ASSERT(seqNoSet.size() == 1);
        NS_ASSERT(lastInfo.seqNumbers.contains(tid));
        lastInfo.seqNumbers.at(tid).erase(*seqNoSet.cbegin());
    }
    m_lastInfoIt.reset();
}

bool
WifiTxParameters::LastAddedIsFirstMpdu(Mac48Address receiver) const
{
    auto infoIt = m_info.find(receiver);
    NS_ASSERT_MSG(infoIt != m_info.cend(), "No frame added for receiver " << receiver);
    NS_ASSERT_MSG(m_lastInfoIt == infoIt, "Last MPDU not addressed to " << receiver);
    return (m_undoInfo.amsduSize == 0 && m_undoInfo.ampduSize == 0);
}

uint32_t
WifiTxParameters::GetSizeIfAddMpdu(Ptr<const WifiMpdu> mpdu) const
{
    NS_LOG_FUNCTION(this << *mpdu);

    auto infoIt = m_info.find(mpdu->GetHeader().GetAddr1());

    if (infoIt == m_info.end())
    {
        // this is an MPDU starting a new PSDU
        if (m_txVector.GetModulationClass() >= WIFI_MOD_CLASS_VHT)
        {
            // All MPDUs are sent with the A-MPDU structure
            return MpduAggregator::GetSizeIfAggregated(mpdu->GetSize(), 0);
        }
        return mpdu->GetSize();
    }

    // aggregate the (A-)MSDU being built to the existing A-MPDU (if any)
    uint32_t ampduSize = MpduAggregator::GetSizeIfAggregated(
        infoIt->second.header.GetSize() + infoIt->second.amsduSize + WIFI_MAC_FCS_LENGTH,
        infoIt->second.ampduSize);
    // aggregate the new MPDU to the A-MPDU
    return MpduAggregator::GetSizeIfAggregated(mpdu->GetSize(), ampduSize);
}

void
WifiTxParameters::AggregateMsdu(Ptr<const WifiMpdu> msdu)
{
    NS_LOG_FUNCTION(this << *msdu);

    auto infoIt = m_info.find(msdu->GetHeader().GetAddr1());
    NS_ASSERT_MSG(infoIt != m_info.end(),
                  "There must be already an MPDU addressed to the same receiver");

    // store information to undo the addition of this MSDU
    m_lastInfoIt = infoIt;
    m_undoInfo =
        PsduInfo{infoIt->second.header, infoIt->second.amsduSize, infoIt->second.ampduSize, {}};

    infoIt->second.amsduSize = GetSizeIfAggregateMsdu(msdu);
    infoIt->second.header.SetQosAmsdu();
}

uint32_t
WifiTxParameters::GetSizeIfAggregateMsdu(Ptr<const WifiMpdu> msdu) const
{
    NS_LOG_FUNCTION(this << *msdu);

    NS_ASSERT_MSG(msdu->GetHeader().IsQosData(),
                  "Can only aggregate a QoS data frame to an A-MSDU");

    auto infoIt = m_info.find(msdu->GetHeader().GetAddr1());
    NS_ASSERT_MSG(infoIt != m_info.end(),
                  "There must be already an MPDU addressed to the same receiver");

    NS_ASSERT_MSG(infoIt->second.amsduSize > 0,
                  "The amsduSize should be set to the size of the previous MSDU(s)");
    NS_ASSERT_MSG(infoIt->second.header.IsQosData(),
                  "The MPDU being built for this receiver must be a QoS data frame");
    NS_ASSERT_MSG(infoIt->second.header.GetQosTid() == msdu->GetHeader().GetQosTid(),
                  "The MPDU being built must belong to the same TID as the MSDU to aggregate");
    NS_ASSERT_MSG(infoIt->second.seqNumbers.contains(msdu->GetHeader().GetQosTid()),
                  "At least one MPDU with the same TID must have been added previously");

    // all checks passed
    uint32_t currAmsduSize = infoIt->second.amsduSize;

    if (!infoIt->second.header.IsQosAmsdu())
    {
        // consider the A-MSDU subframe for the first MSDU
        currAmsduSize = MsduAggregator::GetSizeIfAggregated(currAmsduSize, 0);
    }

    return MsduAggregator::GetSizeIfAggregated(msdu->GetPacket()->GetSize(), currAmsduSize);
}

uint32_t
WifiTxParameters::GetSize(Mac48Address receiver) const
{
    auto infoIt = m_info.find(receiver);

    if (infoIt == m_info.end())
    {
        return 0;
    }

    uint32_t newMpduSize =
        infoIt->second.header.GetSize() + infoIt->second.amsduSize + WIFI_MAC_FCS_LENGTH;

    if (infoIt->second.ampduSize > 0 || m_txVector.GetModulationClass() >= WIFI_MOD_CLASS_VHT)
    {
        return MpduAggregator::GetSizeIfAggregated(newMpduSize, infoIt->second.ampduSize);
    }

    return newMpduSize;
}

void
WifiTxParameters::Print(std::ostream& os) const
{
    os << "TXVECTOR=" << m_txVector;
    if (m_protection)
    {
        os << ", Protection=" << m_protection.get();
    }
    if (m_acknowledgment)
    {
        os << ", Acknowledgment=" << m_acknowledgment.get();
    }
    os << ", PSDUs:";
    for (const auto& info : m_info)
    {
        os << " [To=" << info.second.header.GetAddr1() << ", A-MSDU size=" << info.second.amsduSize
           << ", A-MPDU size=" << info.second.ampduSize << "]";
    }
}

std::ostream&
operator<<(std::ostream& os, const WifiTxParameters* txParams)
{
    txParams->Print(os);
    return os;
}

} // namespace ns3
