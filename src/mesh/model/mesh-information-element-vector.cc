/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Pavel Boyko <boyko@iitp.ru>
 */

#include "mesh-information-element-vector.h"

#include "ns3/hwmp-protocol.h"
#include "ns3/packet.h"

#include <algorithm>
// All information elements:
#include "ns3/ie-dot11s-beacon-timing.h"
#include "ns3/ie-dot11s-configuration.h"
#include "ns3/ie-dot11s-id.h"
#include "ns3/ie-dot11s-metric-report.h"
#include "ns3/ie-dot11s-peer-management.h"
#include "ns3/ie-dot11s-peering-protocol.h"
#include "ns3/ie-dot11s-perr.h"
#include "ns3/ie-dot11s-prep.h"
#include "ns3/ie-dot11s-preq.h"
#include "ns3/ie-dot11s-rann.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(MeshInformationElementVector);

MeshInformationElementVector::MeshInformationElementVector()
    : m_maxSize(1500)
{
}

MeshInformationElementVector::~MeshInformationElementVector()
{
    for (auto i = m_elements.begin(); i != m_elements.end(); i++)
    {
        *i = nullptr;
    }
    m_elements.clear();
}

TypeId
MeshInformationElementVector::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MeshInformationElementVector")
                            .SetParent<Header>()
                            .SetGroupName("Mesh")
                            .AddConstructor<MeshInformationElementVector>();
    return tid;
}

TypeId
MeshInformationElementVector::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
MeshInformationElementVector::GetSerializedSize() const
{
    return GetSize();
}

void
MeshInformationElementVector::Serialize(Buffer::Iterator start) const
{
    for (auto i = m_elements.begin(); i != m_elements.end(); i++)
    {
        start = (*i)->Serialize(start);
    }
}

uint32_t
MeshInformationElementVector::Deserialize(Buffer::Iterator start)
{
    NS_FATAL_ERROR("This variant should not be called on a variable-sized header");
    return 0;
}

uint32_t
MeshInformationElementVector::Deserialize(Buffer::Iterator start, Buffer::Iterator end)
{
    uint32_t size = start.GetDistanceFrom(end);
    uint32_t remaining = size;
    while (remaining > 0)
    {
        uint32_t deserialized = DeserializeSingleIe(start);
        start.Next(deserialized);
        NS_ASSERT(deserialized <= remaining);
        remaining -= deserialized;
    }
    NS_ASSERT_MSG(remaining == 0, "Error in deserialization");
    return size;
}

uint32_t
MeshInformationElementVector::DeserializeSingleIe(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    uint8_t id = i.ReadU8();
    uint8_t length = i.ReadU8();
    i.Prev(2);
    Ptr<WifiInformationElement> newElement;
    switch (id)
    {
    case IE_MESH_CONFIGURATION:
        newElement = Create<dot11s::IeConfiguration>();
        break;
    case IE_MESH_ID:
        newElement = Create<dot11s::IeMeshId>();
        break;
    case IE_MESH_LINK_METRIC_REPORT:
        newElement = Create<dot11s::IeLinkMetricReport>();
        break;
    case IE_MESH_PEERING_MANAGEMENT:
        newElement = Create<dot11s::IePeerManagement>();
        break;
    case IE_BEACON_TIMING:
        newElement = Create<dot11s::IeBeaconTiming>();
        break;
    case IE_RANN:
        newElement = Create<dot11s::IeRann>();
        break;
    case IE_PREQ:
        newElement = Create<dot11s::IePreq>();
        break;
    case IE_PREP:
        newElement = Create<dot11s::IePrep>();
        break;
    case IE_PERR:
        newElement = Create<dot11s::IePerr>();
        break;
    case IE11S_MESH_PEERING_PROTOCOL_VERSION:
        newElement = Create<dot11s::IePeeringProtocol>();
        break;
    default:
        NS_FATAL_ERROR("Information element " << +id << " is not implemented");
        return 0;
    }
    if (GetSize() + length > m_maxSize)
    {
        NS_FATAL_ERROR("Check max size for information element!");
    }
    i = newElement->Deserialize(i);
    m_elements.push_back(newElement);
    return i.GetDistanceFrom(start);
}

void
MeshInformationElementVector::Print(std::ostream& os) const
{
    for (auto i = m_elements.begin(); i != m_elements.end(); i++)
    {
        os << "(";
        (*i)->Print(os);
        os << ")";
    }
}

MeshInformationElementVector::Iterator
MeshInformationElementVector::Begin()
{
    return m_elements.begin();
}

MeshInformationElementVector::Iterator
MeshInformationElementVector::End()
{
    return m_elements.end();
}

bool
MeshInformationElementVector::AddInformationElement(Ptr<WifiInformationElement> element)
{
    if (element->GetSerializedSize() + GetSize() > m_maxSize)
    {
        return false;
    }
    m_elements.push_back(element);
    return true;
}

Ptr<WifiInformationElement>
MeshInformationElementVector::FindFirst(WifiInformationElementId id) const
{
    for (auto i = m_elements.begin(); i != m_elements.end(); i++)
    {
        if ((*i)->ElementId() == id)
        {
            return *i;
        }
    }
    return nullptr;
}

uint32_t
MeshInformationElementVector::GetSize() const
{
    uint32_t size = 0;
    for (auto i = m_elements.begin(); i != m_elements.end(); i++)
    {
        size += (*i)->GetSerializedSize();
    }
    return size;
}

bool
MeshInformationElementVector::operator==(const MeshInformationElementVector& a) const
{
    if (m_elements.size() != a.m_elements.size())
    {
        NS_ASSERT(false);
        return false;
    }
    // In principle we could bypass some of the faffing about (and speed
    // the comparison) by simply serialising each IE vector into a
    // buffer and memcmp'ing the two.
    //
    // I'm leaving it like this, however, so that there is the option of
    // having individual Information Elements implement slightly more
    // flexible equality operators.
    auto j = a.m_elements.begin();
    for (auto i = m_elements.begin(); i != m_elements.end(); i++, j++)
    {
        if (!(*(*i) == *(*j)))
        {
            return false;
        }
    }

    return true;
}

} // namespace ns3
