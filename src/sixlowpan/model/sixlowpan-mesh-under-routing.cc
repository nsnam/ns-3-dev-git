/*
 * Copyright (c) 2026 SRM Institute of Science and Technology, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Usham Roy <ushamroy80@gmail.com>
 */

#include "sixlowpan-mesh-under-routing.h"

#include "ns3/log.h"
#include "ns3/uinteger.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SixLowPanMeshUnderRouting");
NS_OBJECT_ENSURE_REGISTERED(SixLowPanMeshUnderRouting);

TypeId
SixLowPanMeshUnderRouting::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SixLowPanMeshUnderRouting")
            .SetParent<Object>()
            .SetGroupName("SixLowPan")
            .AddAttribute("MeshCacheLength",
                          "Per-originator cache size used for duplicate detection.",
                          UintegerValue(10),
                          MakeUintegerAccessor(&SixLowPanMeshUnderRouting::m_meshCacheLength),
                          MakeUintegerChecker<uint16_t>());
    return tid;
}

SixLowPanMeshUnderRouting::SixLowPanMeshUnderRouting()
{
    NS_LOG_FUNCTION(this);
}

SixLowPanMeshUnderRouting::~SixLowPanMeshUnderRouting()
{
    NS_LOG_FUNCTION(this);
}

void
SixLowPanMeshUnderRouting::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_seenPkts.clear();
    Object::DoDispose();
}

bool
SixLowPanMeshUnderRouting::IsDuplicate(const Address& originator, uint8_t seqNo) const
{
    NS_LOG_FUNCTION(this << originator << +seqNo);
    auto it = m_seenPkts.find(originator);
    if (it == m_seenPkts.end())
    {
        return false;
    }
    return std::find(it->second.begin(), it->second.end(), seqNo) != it->second.end();
}

void
SixLowPanMeshUnderRouting::RecordPacket(const Address& originator, uint8_t seqNo)
{
    NS_LOG_FUNCTION(this << originator << +seqNo);
    auto& queue = m_seenPkts[originator];
    queue.push_back(seqNo);
    if (queue.size() > m_meshCacheLength)
    {
        queue.pop_front();
    }
}

void
SixLowPanMeshUnderRouting::OnDuplicateReceived(const Address& originator, uint8_t seqNo)
{
    NS_LOG_FUNCTION(this << originator << +seqNo);
    // Default no-op. Subclasses may override to react to duplicates.
}

} // namespace ns3
