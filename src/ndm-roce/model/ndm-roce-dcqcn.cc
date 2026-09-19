/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 */

#include "ns3/ndm-roce-dcqcn.h"

#include "ns3/simulator.h"

#include <algorithm>
#include <cmath>

namespace ns3
{

NdmRoceDcQcn::NdmRoceDcQcn() = default;

NdmRoceDcQcn::~NdmRoceDcQcn()
{
    Stop();
}

void
NdmRoceDcQcn::SetLinkRate(DataRate linkRate)
{
    m_rMax = linkRate;
    if (m_rate > linkRate)
    {
        m_rate = linkRate;
    }
    if (m_target > linkRate)
    {
        m_target = linkRate;
    }
}

void
NdmRoceDcQcn::SetRttEstimate(Time rtt)
{
    if (rtt > NanoSeconds(1))
    {
        m_rtt = rtt;
    }
}

void
NdmRoceDcQcn::OnCnp(Time now)
{
    // Congestion increases; rate target drops (rate itself drains over RP).
    m_cnpInSlot = true;
    m_alpha = std::min(1.0, m_alpha + m_alpha * kBeta);
    if (m_alpha >= 0.5)
    {
        DataRate dec = DataRate(static_cast<uint64_t>(
            std::max(m_rate.GetBitRate() * (1.0 - kBeta),
                     0.5 * m_rate.GetBitRate())));
        m_target = std::max(dec, m_rMin);
    }
}

void
NdmRoceDcQcn::Start(Time now)
{
    if (m_started)
    {
        return;
    }
    m_started = true;
    m_rate = std::min(m_rMax, m_rate);
    m_target = m_rate;
    m_slotEvent = Simulator::Schedule(m_slot, &NdmRoceDcQcn::DoSlot, this);
}

void
NdmRoceDcQcn::Stop()
{
    if (m_slotEvent.IsRunning())
    {
        Simulator::Cancel(m_slotEvent);
    }
    m_started = false;
}

uint64_t
NdmRoceDcQcn::GetWindowBytes() const
{
    // credits = rate * RTT, in bytes; floor at one MTU so the QP never stalls.
    const double bytes = static_cast<double>(m_rate.GetBitRate()) *
                         m_rtt.GetSeconds() / 8.0;
    return static_cast<uint64_t>(std::max(bytes, 2048.0));
}

void
NdmRoceDcQcn::DoSlot()
{
    if (!m_started)
    {
        return;
    }

    // 1) alpha dynamics.
    if (m_cnpInSlot)
    {
        // CNP already applied alpha increase in OnCnp.
    }
    else
    {
        const double frac = std::min(1.0,
                                     m_slot.GetSeconds() /
                                         (2.0 * m_rtt.GetSeconds()));
        m_alpha = std::max(0.0, m_alpha * (1.0 - frac));
    }
    m_cnpInSlot = false;

    // 2) rate target.
    if (m_alpha >= 1.0)
    {
        m_target = m_rMin;
    }
    else if (m_alpha >= 0.5)
    {
        DataRate dec = DataRate(static_cast<uint64_t>(
            std::max(m_rate.GetBitRate() * (1.0 - kBeta),
                     0.5 * m_rate.GetBitRate())));
        m_target = std::max(dec, m_rMin);
    }
    else
    {
        // multiplicative increase, floored at one packet per RTT
        const double pktBytes = 2048.0 + 16.0 + 8.0 + 48.0; // payload+bth+udp+ipv6
        const double onePerRtt = pktBytes / m_rtt.GetSeconds() * 8.0;
        const double add = std::max(m_rate.GetBitRate() / kGain, onePerRtt);
        const double t = std::min(static_cast<double>(m_rMax.GetBitRate()),
                                  m_rate.GetBitRate() + add);
        m_target = DataRate(static_cast<uint64_t>(t));
    }

    // 3) rate movement: up immediately, down over RP = RTT/4.
    if (m_rate > m_target)
    {
        const Time rp = m_rtt / 4;
        const double frac = std::min(1.0, m_slot.GetSeconds() / rp.GetSeconds());
        const double diff = (m_rate.GetBitRate() - m_target.GetBitRate()) * frac;
        m_rate = DataRate(std::max(m_target.GetBitRate(),
                                   static_cast<uint64_t>(m_rate.GetBitRate() - diff)));
    }
    else
    {
        m_rate = m_target;
    }

    m_slotEvent = Simulator::Schedule(m_slot, &NdmRoceDcQcn::DoSlot, this);
}

} // namespace ns3
