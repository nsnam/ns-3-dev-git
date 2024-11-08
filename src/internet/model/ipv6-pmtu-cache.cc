/*
 * Copyright (c) 2013 Universita' di Firenze
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "ipv6-pmtu-cache.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv6PmtuCache");

NS_OBJECT_ENSURE_REGISTERED(Ipv6PmtuCache);

TypeId
Ipv6PmtuCache::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::Ipv6PmtuCache")
            .SetParent<Object>()
            .SetGroupName("Internet")
            .AddAttribute(
                "CacheExpiryTime",
                "Validity time for a Path MTU entry. Default is 10 minutes, minimum is 5 minutes.",
                TimeValue(Seconds(60 * 10)),
                MakeTimeAccessor(&Ipv6PmtuCache::m_validityTime),
                MakeTimeChecker(Seconds(60 * 5)));
    return tid;
}

Ipv6PmtuCache::Ipv6PmtuCache()
{
}

Ipv6PmtuCache::~Ipv6PmtuCache()
{
}

void
Ipv6PmtuCache::DoDispose()
{
    for (auto iter = m_pathMtuTimer.begin(); iter != m_pathMtuTimer.end(); iter++)
    {
        iter->second.Cancel();
    }
    m_pathMtuTimer.clear();
    m_pathMtu.clear();
}

uint32_t
Ipv6PmtuCache::GetPmtu(Ipv6Address dst)
{
    NS_LOG_FUNCTION(this << dst);

    if (m_pathMtu.find(dst) != m_pathMtu.end())
    {
        return m_pathMtu[dst];
    }
    return 0;
}

void
Ipv6PmtuCache::SetPmtu(Ipv6Address dst, uint32_t pmtu)
{
    NS_LOG_FUNCTION(this << dst << pmtu);

    m_pathMtu[dst] = pmtu;
    if (m_pathMtuTimer.find(dst) != m_pathMtuTimer.end())
    {
        m_pathMtuTimer[dst].Cancel();
    }
    EventId pMtuTimer;
    pMtuTimer = Simulator::Schedule(m_validityTime, &Ipv6PmtuCache::ClearPmtu, this, dst);
    m_pathMtuTimer[dst] = pMtuTimer;
}

Time
Ipv6PmtuCache::GetPmtuValidityTime() const
{
    NS_LOG_FUNCTION(this);
    return m_validityTime;
}

bool
Ipv6PmtuCache::SetPmtuValidityTime(Time validity)
{
    NS_LOG_FUNCTION(this << validity);

    if (validity > Seconds(60 * 5))
    {
        m_validityTime = validity;
        return true;
    }

    NS_LOG_LOGIC("rejecting a PMTU validity timer lesser than 5 minutes");
    return false;
}

void
Ipv6PmtuCache::ClearPmtu(Ipv6Address dst)
{
    NS_LOG_FUNCTION(this << dst);

    m_pathMtu.erase(dst);
    m_pathMtuTimer.erase(dst);
}

} // namespace ns3
