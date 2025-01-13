/*
 *  Copyright 2013. Lawrence Livermore National Security, LLC.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Steven Smith <smith84@llnl.gov>
 *
 */

/**
 * @file
 * @ingroup mpi
 * Implementation of class ns3::RemoteChannelBundle.
 */

#include "remote-channel-bundle.h"

#include "null-message-mpi-interface.h"
#include "null-message-simulator-impl.h"

#include "ns3/simulator.h"

namespace ns3
{

TypeId
RemoteChannelBundle::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RemoteChannelBundle")
                            .SetParent<Object>()
                            .SetGroupName("Mpi")
                            .AddConstructor<RemoteChannelBundle>();
    return tid;
}

RemoteChannelBundle::RemoteChannelBundle()
    : m_remoteSystemId(UINT32_MAX),
      m_guaranteeTime(0),
      m_delay(Time::Max())
{
}

RemoteChannelBundle::RemoteChannelBundle(const uint32_t remoteSystemId)
    : m_remoteSystemId(remoteSystemId),
      m_guaranteeTime(0),
      m_delay(Time::Max())
{
}

void
RemoteChannelBundle::AddChannel(Ptr<Channel> channel, Time delay)
{
    m_channels[channel->GetId()] = channel;
    m_delay = ns3::Min(m_delay, delay);
}

uint32_t
RemoteChannelBundle::GetSystemId() const
{
    return m_remoteSystemId;
}

Time
RemoteChannelBundle::GetGuaranteeTime() const
{
    return m_guaranteeTime;
}

void
RemoteChannelBundle::SetGuaranteeTime(Time time)
{
    NS_ASSERT(time >= Simulator::Now());

    m_guaranteeTime = time;
}

Time
RemoteChannelBundle::GetDelay() const
{
    return m_delay;
}

void
RemoteChannelBundle::SetEventId(EventId id)
{
    m_nullEventId = id;
}

EventId
RemoteChannelBundle::GetEventId() const
{
    return m_nullEventId;
}

std::size_t
RemoteChannelBundle::GetSize() const
{
    return m_channels.size();
}

void
RemoteChannelBundle::Send(Time time)
{
    NullMessageMpiInterface::SendNullMessage(time, this);
}

std::ostream&
operator<<(std::ostream& out, ns3::RemoteChannelBundle& bundle)
{
    out << "RemoteChannelBundle Rank = " << bundle.m_remoteSystemId
        << ", GuaranteeTime = " << bundle.m_guaranteeTime << ", Delay = " << bundle.m_delay
        << std::endl;

    for (const auto& element : bundle.m_channels)
    {
        out << "\t" << element.second << std::endl;
    }

    return out;
}

} // namespace ns3
