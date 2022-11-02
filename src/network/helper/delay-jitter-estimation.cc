/*
 * Copyright (c) 2007 INRIA
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
 */

#include "delay-jitter-estimation.h"

#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/timestamp-tag.h"

namespace ns3
{

DelayJitterEstimation::DelayJitterEstimation()
{
}

void
DelayJitterEstimation::PrepareTx(Ptr<const Packet> packet)
{
    TimestampTag tag(Simulator::Now());
    packet->AddByteTag(tag);
}

void
DelayJitterEstimation::RecordRx(Ptr<const Packet> packet)
{
    TimestampTag tag;

    if (!packet->FindFirstMatchingByteTag(tag))
    {
        return;
    }

    // Variable names from
    // RFC 1889 Appendix A.8 ,p. 71,
    // RFC 3550 Appendix A.8, p. 94
    Time r_ts = tag.GetTimestamp();
    Time arrival = Simulator::Now();
    Time transit = arrival - r_ts;
    Time delta = transit - m_transit;
    m_transit = transit;

    // floating jitter version
    // m_jitter += (Abs (delta) - m_jitter) / 16;

    // int variant
    m_jitter += Abs(delta) - ((m_jitter + TimeStep(8)) / 16);
}

Time
DelayJitterEstimation::GetLastDelay() const
{
    return m_transit;
}

uint64_t
DelayJitterEstimation::GetLastJitter() const
{
    // floating jitter version
    // return m_jitter.GetTimeStep();

    // int variant
    return (m_jitter / 16).GetTimeStep();
}

} // namespace ns3
