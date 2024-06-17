/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kirill Andreev <andreev@iitp.ru>
 */
#include "flame-rtable.h"

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("FlameRtable");

namespace flame
{

NS_OBJECT_ENSURE_REGISTERED(FlameRtable);

TypeId
FlameRtable::GetTypeId()
{
    static TypeId tid = TypeId("ns3::flame::FlameRtable")
                            .SetParent<Object>()
                            .SetGroupName("Mesh")
                            .AddConstructor<FlameRtable>()
                            .AddAttribute("Lifetime",
                                          "The lifetime of the routing entry",
                                          TimeValue(Seconds(120)),
                                          MakeTimeAccessor(&FlameRtable::m_lifetime),
                                          MakeTimeChecker());
    return tid;
}

FlameRtable::FlameRtable()
    : m_lifetime(Seconds(120))
{
}

FlameRtable::~FlameRtable()
{
}

void
FlameRtable::DoDispose()
{
    m_routes.clear();
}

void
FlameRtable::AddPath(const Mac48Address destination,
                     const Mac48Address retransmitter,
                     const uint32_t interface,
                     const uint8_t cost,
                     const uint16_t seqnum)
{
    auto i = m_routes.find(destination);
    if (i == m_routes.end())
    {
        Route newroute;
        newroute.cost = cost;
        newroute.retransmitter = retransmitter;
        newroute.interface = interface;
        newroute.whenExpire = Simulator::Now() + m_lifetime;
        newroute.seqnum = seqnum;
        m_routes[destination] = newroute;
        return;
    }
    i->second.seqnum = seqnum;
    NS_ASSERT(i != m_routes.end());
    i->second.retransmitter = retransmitter;
    i->second.interface = interface;
    i->second.cost = cost;
    i->second.whenExpire = Simulator::Now() + m_lifetime;
}

FlameRtable::LookupResult
FlameRtable::Lookup(Mac48Address destination)
{
    auto i = m_routes.find(destination);
    if (i == m_routes.end())
    {
        return LookupResult();
    }
    if (i->second.whenExpire < Simulator::Now())
    {
        NS_LOG_DEBUG("Route has expired, sorry.");
        m_routes.erase(i);
        return LookupResult();
    }
    return LookupResult(i->second.retransmitter,
                        i->second.interface,
                        i->second.cost,
                        i->second.seqnum);
}

bool
FlameRtable::LookupResult::operator==(const FlameRtable::LookupResult& o) const
{
    return (retransmitter == o.retransmitter && ifIndex == o.ifIndex && cost == o.cost &&
            seqnum == o.seqnum);
}

bool
FlameRtable::LookupResult::IsValid() const
{
    return !(retransmitter == Mac48Address::GetBroadcast() && ifIndex == INTERFACE_ANY &&
             cost == MAX_COST && seqnum == 0);
}

} // namespace flame
} // namespace ns3
