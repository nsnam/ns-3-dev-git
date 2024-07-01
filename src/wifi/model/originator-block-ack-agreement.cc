/*
 * Copyright (c) 2009, 2010 MIRKO BANCHI
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mirko Banchi <mk.banchi@gmail.com>
 *          Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "originator-block-ack-agreement.h"

#include "wifi-mpdu.h"
#include "wifi-utils.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OriginatorBlockAckAgreement");

OriginatorBlockAckAgreement::OriginatorBlockAckAgreement(Mac48Address recipient, uint8_t tid)
    : BlockAckAgreement(recipient, tid),
      m_state(PENDING)
{
}

OriginatorBlockAckAgreement::~OriginatorBlockAckAgreement()
{
}

void
OriginatorBlockAckAgreement::SetState(State state)
{
    m_state = state;
}

bool
OriginatorBlockAckAgreement::IsPending() const
{
    return m_state == PENDING;
}

bool
OriginatorBlockAckAgreement::IsEstablished() const
{
    return m_state == ESTABLISHED;
}

bool
OriginatorBlockAckAgreement::IsRejected() const
{
    return m_state == REJECTED;
}

bool
OriginatorBlockAckAgreement::IsNoReply() const
{
    return m_state == NO_REPLY;
}

bool
OriginatorBlockAckAgreement::IsReset() const
{
    return m_state == RESET;
}

uint16_t
OriginatorBlockAckAgreement::GetStartingSequence() const
{
    if (m_txWindow.GetWinSize() == 0)
    {
        // the TX window has not been initialized yet
        return m_startingSeq;
    }
    return m_txWindow.GetWinStart();
}

std::size_t
OriginatorBlockAckAgreement::GetDistance(uint16_t seqNumber) const
{
    return BlockAckAgreement::GetDistance(seqNumber, m_txWindow.GetWinStart());
}

void
OriginatorBlockAckAgreement::InitTxWindow()
{
    m_txWindow.Init(m_startingSeq, m_bufferSize);
}

bool
OriginatorBlockAckAgreement::AllAckedMpdusInTxWindow(const std::set<uint16_t>& seqNumbers) const
{
    std::set<std::size_t> distances;
    for (const auto seqN : seqNumbers)
    {
        distances.insert(GetDistance(seqN));
    }

    for (std::size_t i = 0; i < m_txWindow.GetWinSize(); ++i)
    {
        if (distances.contains(i))
        {
            continue; // this is one of the positions to ignore
        }
        if (!m_txWindow.At(i))
        {
            return false; // this position is available or contains an unacknowledged MPDU
        }
    }
    NS_LOG_INFO("TX window is blocked");
    return true;
}

void
OriginatorBlockAckAgreement::AdvanceTxWindow()
{
    while (m_txWindow.At(0))
    {
        m_txWindow.Advance(1); // reset the current head -- ensures loop termination
    }
}

void
OriginatorBlockAckAgreement::NotifyTransmittedMpdu(Ptr<const WifiMpdu> mpdu)
{
    uint16_t mpduSeqNumber = mpdu->GetHeader().GetSequenceNumber();
    uint16_t distance = GetDistance(mpduSeqNumber);

    if (distance >= SEQNO_SPACE_HALF_SIZE)
    {
        NS_LOG_DEBUG("Transmitted an old MPDU, do nothing.");
        return;
    }

    // advance the transmit window if an MPDU beyond the current transmit window
    // is transmitted (see Section 10.24.7.7 of 802.11-2016)
    if (distance >= m_txWindow.GetWinSize())
    {
        std::size_t count = distance - m_txWindow.GetWinSize() + 1;
        m_txWindow.Advance(count);
        // transmit window may advance further
        AdvanceTxWindow();
        NS_LOG_DEBUG(
            "Transmitted MPDU beyond current transmit window. New starting sequence number: "
            << m_txWindow.GetWinStart());
    }
}

void
OriginatorBlockAckAgreement::NotifyAckedMpdu(Ptr<const WifiMpdu> mpdu)
{
    uint16_t mpduSeqNumber = mpdu->GetHeader().GetSequenceNumber();
    uint16_t distance = GetDistance(mpduSeqNumber);

    if (distance >= SEQNO_SPACE_HALF_SIZE)
    {
        NS_LOG_DEBUG("Acked an old MPDU, do nothing.");
        return;
    }

    // when an MPDU is transmitted, the transmit window is updated such that the
    // transmitted MPDU is in the window, hence we cannot be notified of the
    // acknowledgment of an MPDU which is beyond the transmit window
    m_txWindow.At(distance) = true;

    // the starting sequence number can be advanced to the sequence number of
    // the nearest unacknowledged MPDU
    AdvanceTxWindow();
    NS_LOG_DEBUG("Starting sequence number: " << m_txWindow.GetWinStart());
}

void
OriginatorBlockAckAgreement::NotifyDiscardedMpdu(Ptr<const WifiMpdu> mpdu)
{
    uint16_t mpduSeqNumber = mpdu->GetHeader().GetSequenceNumber();
    uint16_t distance = GetDistance(mpduSeqNumber);

    if (distance >= SEQNO_SPACE_HALF_SIZE)
    {
        NS_LOG_DEBUG("Discarded an old MPDU, do nothing.");
        return;
    }

    m_txWindow.Advance(distance + 1);
    // transmit window may advance further
    AdvanceTxWindow();
    NS_LOG_DEBUG("Discarded MPDU within current transmit window. New starting sequence number: "
                 << m_txWindow.GetWinStart());
}

} // namespace ns3
