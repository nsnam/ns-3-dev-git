/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "simple-channel.h"

#include "simple-net-device.h"

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SimpleChannel");

NS_OBJECT_ENSURE_REGISTERED(SimpleChannel);

TypeId
SimpleChannel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SimpleChannel")
                            .SetParent<Channel>()
                            .SetGroupName("Network")
                            .AddConstructor<SimpleChannel>()
                            .AddAttribute("Delay",
                                          "Transmission delay through the channel",
                                          TimeValue(Seconds(0)),
                                          MakeTimeAccessor(&SimpleChannel::m_delay),
                                          MakeTimeChecker());
    return tid;
}

SimpleChannel::SimpleChannel()
{
    NS_LOG_FUNCTION(this);
}

void
SimpleChannel::Send(Ptr<Packet> p,
                    uint16_t protocol,
                    Mac48Address to,
                    Mac48Address from,
                    Ptr<SimpleNetDevice> sender)
{
    NS_LOG_FUNCTION(this << p << protocol << to << from << sender);
    for (auto i = m_devices.begin(); i != m_devices.end(); ++i)
    {
        Ptr<SimpleNetDevice> tmp = *i;
        if (tmp == sender)
        {
            continue;
        }
        if (m_blackListedDevices.find(tmp) != m_blackListedDevices.end())
        {
            if (find(m_blackListedDevices[tmp].begin(), m_blackListedDevices[tmp].end(), sender) !=
                m_blackListedDevices[tmp].end())
            {
                continue;
            }
        }
        Simulator::ScheduleWithContext(tmp->GetNode()->GetId(),
                                       m_delay,
                                       &SimpleNetDevice::Receive,
                                       tmp,
                                       p->Copy(),
                                       protocol,
                                       to,
                                       from);
    }
}

void
SimpleChannel::Add(Ptr<SimpleNetDevice> device)
{
    NS_LOG_FUNCTION(this << device);
    m_devices.push_back(device);
}

std::size_t
SimpleChannel::GetNDevices() const
{
    NS_LOG_FUNCTION(this);
    return m_devices.size();
}

Ptr<NetDevice>
SimpleChannel::GetDevice(std::size_t i) const
{
    NS_LOG_FUNCTION(this << i);
    return m_devices[i];
}

void
SimpleChannel::BlackList(Ptr<SimpleNetDevice> from, Ptr<SimpleNetDevice> to)
{
    if (m_blackListedDevices.find(to) != m_blackListedDevices.end())
    {
        if (find(m_blackListedDevices[to].begin(), m_blackListedDevices[to].end(), from) ==
            m_blackListedDevices[to].end())
        {
            m_blackListedDevices[to].push_back(from);
        }
    }
    else
    {
        m_blackListedDevices[to].push_back(from);
    }
}

void
SimpleChannel::UnBlackList(Ptr<SimpleNetDevice> from, Ptr<SimpleNetDevice> to)
{
    if (m_blackListedDevices.find(to) != m_blackListedDevices.end())
    {
        auto iter = find(m_blackListedDevices[to].begin(), m_blackListedDevices[to].end(), from);
        if (iter != m_blackListedDevices[to].end())
        {
            m_blackListedDevices[to].erase(iter);
        }
    }
}

} // namespace ns3
