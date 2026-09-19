/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 */

#include "ns3/ndm-roce-qpair.h"

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <cstring>

#define NS_LOG_COMPONENT_NAME "NdmRoceQPair"

namespace ns3
{

NdmRoceQPair::NdmRoceQPair(uint32_t localQpn, uint32_t peerQpn)
    : m_localQpn(localQpn),
      m_peerQpn(peerQpn)
{
}

NdmRoceQPair::~NdmRoceQPair()
{
    if (m_rtoEvent.IsRunning())
    {
        Simulator::Cancel(m_rtoEvent);
    }
    m_cc.Stop();
}

void
NdmRoceQPair::SetMtu(uint32_t mtuPayloadBytes)
{
    m_mtu = mtuPayloadBytes;
}

void
NdmRoceQPair::SetLinkRate(DataRate rate)
{
    m_cc.SetLinkRate(rate);
}

void
NdmRoceQPair::SetInitialRtt(Time rtt)
{
    m_rttEst = rtt;
    m_cc.SetRttEstimate(rtt);
}

uint32_t
NdmRoceQPair::PostWrite(const uint8_t* data, uint32_t nbytes, uint32_t imm)
{
    const uint32_t first = m_totalPsn;
    PendingMsg msg;
    if (data != nullptr && nbytes > 0)
    {
        msg.data.assign(data, data + nbytes);
    }
    msg.imm = imm;
    msg.psnStart = first;
    msg.psns = nbytes == 0 ? 1 : (nbytes + m_mtu - 1) / m_mtu;
    m_msgs.push_back(std::move(msg));
    m_totalPsn += m_msgs.back().psns;
    m_bytesPosted += nbytes;
    if (m_started)
    {
        Pump();
    }
    return first;
}

void
NdmRoceQPair::Start(Time now)
{
    if (m_started)
    {
        return;
    }
    m_started = true;
    m_cc.Start(now);
    Pump();
}

uint32_t
NdmRoceQPair::FragmentSize(uint32_t psn) const
{
    for (const auto& m : m_msgs)
    {
        if (m.psnStart <= psn && psn < m.psnStart + m.psns)
        {
            const uint32_t i = psn - m.psnStart;
            const uint64_t off = static_cast<uint64_t>(i) * m_mtu;
            return static_cast<uint32_t>(
                std::min<uint64_t>(m_mtu, m.data.size() - off));
        }
    }
    NS_ABORT_MSG("NdmRoceQPair: FragmentSize: PSN not in any message");
}

void
NdmRoceQPair::Pump()
{
    const uint64_t window = m_cc.GetWindowBytes();

    while (m_sndNxt < m_totalPsn)
    {
        const uint32_t nextSize = FragmentSize(m_sndNxt);
        if (m_flightBytes + nextSize > window)
        {
            break; // CC window exhausted; the next CC slot reopens it
        }
        SendPsn(m_sndNxt, false, Simulator::Now());
        m_sndNxt++;
    }

    // RTO on the oldest unacked (armed only for first transmissions).
    if (!m_rtoEvent.IsRunning() &&
        !m_unackedSend.empty() && m_unackedFresh.front())
    {
        m_rtoEvent =
            Simulator::Schedule(m_rttEst * 4, &NdmRoceQPair::OnRto, this);
    }
}

void
NdmRoceQPair::OnRto()
{
    if (!m_started)
    {
        return;
    }
    if (!m_unackedSend.empty() && m_unackedFresh.front())
    {
        NS_LOG_WARN("NdmRoceQPair " << m_localQpn << ": RTO on PSN " << m_sndUna);
        m_unackedFresh.front() = false;
        RetransmitFrom(m_sndUna, Simulator::Now());
    }
}

void
NdmRoceQPair::SendPsn(uint32_t psn, bool isRetransmit, Time now)
{
    Ptr<Packet> pkt = BuildPacket(psn);
    m_sendTx(pkt, m_peerQpn);
    m_packetsSent++;
    if (isRetransmit)
    {
        m_retransmits++;
        return;
    }
    m_unackedSend.push_back(now);
    m_unackedFresh.push_back(true);
    m_flightBytes += FragmentSize(psn);
}

Ptr<Packet>
NdmRoceQPair::BuildPacket(uint32_t psn)
{
    NS_ASSERT_MSG(psn < m_totalPsn, "NdmRoceQPair: BuildPacket past end");
    for (const auto& m : m_msgs)
    {
        if (m.psnStart <= psn && psn < m.psnStart + m.psns)
        {
            const uint32_t i = psn - m.psnStart;
            const uint64_t off = static_cast<uint64_t>(i) * m_mtu;
            const uint32_t len = static_cast<uint32_t>(
                std::min<uint64_t>(m_mtu, m.data.size() - off));
            Ptr<Packet> pkt = len > 0 ? Create<Packet>(m.data.data() + off, len)
                                : Create<Packet>();
            const bool last = (i + 1 == m.psns);
            NdmRoceBth bth(last ? NDM_ROCE_WRITE_WITH_IMM : NDM_ROCE_WRITE,
                           psn, m_peerQpn);
            bth.SetFlags(NDM_ROCE_BTH_ACK_REQ);
            if (last)
            {
                bth.m_imm = m.imm;
            }
            pkt->AddHeader(bth);
            return pkt;
        }
    }
    NS_ABORT_MSG("NdmRoceQPair: BuildPacket: PSN not in any message");
}

void
NdmRoceQPair::RetransmitFrom(uint32_t psn, Time now)
{
    for (uint32_t p = psn; p < m_sndNxt; ++p)
    {
        SendPsn(p, true, now);
    }
    Pump();
}

void
NdmRoceQPair::HandleAck(uint32_t psn, bool ecnEcho, Time now)
{
    (void)ecnEcho; // ECN reaction is via CNP (rate-based); echo is logged only
    if (!m_started)
    {
        return;
    }
    if (psn < m_sndUna)
    {
        return; // duplicate/stale ACK — idempotent no-op
    }
    if (psn >= m_totalPsn)
    {
        psn = m_totalPsn - 1;
    }
    const uint32_t ackedCount = psn - m_sndUna + 1;

    // RTT sample from the oldest unacked (the ACK covers it).
    if (!m_unackedSend.empty())
    {
        const Time sample = now - m_unackedSend.front();
        if (sample > NanoSeconds(0))
        {
            m_lastRttSample = sample;
            m_rttEst = m_rttEst * 9 / 10 + sample / 10;
            m_cc.SetRttEstimate(m_rttEst);
        }
    }

    // Free credits and count acknowledged bytes.
    for (uint32_t p = m_sndUna; p <= psn; ++p)
    {
        m_flightBytes -= FragmentSize(p);
        m_bytesAcked += FragmentSize(p);
    }

    m_unackedSend.erase(m_unackedSend.begin(), m_unackedSend.begin() + ackedCount);
    m_unackedFresh.erase(m_unackedFresh.begin(), m_unackedFresh.begin() + ackedCount);
    m_sndUna = psn + 1;

    // Drop fully-ACKed messages (data no longer needed).
    while (!m_msgs.empty() &&
           m_sndUna >= m_msgs.front().psnStart + m_msgs.front().psns)
    {
        m_msgs.erase(m_msgs.begin());
    }

    // Restart RTO for the new oldest unacked.
    if (m_rtoEvent.IsRunning())
    {
        Simulator::Cancel(m_rtoEvent);
    }

    if (m_msgs.empty() && m_totalPsn > 0 && m_sndUna >= m_totalPsn && !m_finished)
    {
        m_finished = true;
        m_cc.Stop();
        if (!m_notifyComplete.IsNull())
        {
            m_notifyComplete();
        }
        return;
    }
    Pump();
}

void
NdmRoceQPair::HandleNack(uint32_t psn, Time now)
{
    if (!m_started || psn < m_sndUna)
    {
        return; // stale NACK
    }
    RetransmitFrom(std::max(psn, m_sndUna), now);
}

void
NdmRoceQPair::HandleCnp(Time now)
{
    m_cc.OnCnp(now);
}

} // namespace ns3
