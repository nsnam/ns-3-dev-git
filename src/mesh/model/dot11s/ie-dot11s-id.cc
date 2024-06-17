/*
 * Copyright (c) 2008,2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kirill Andreev <andreev@iitp.ru>
 */

#include "ie-dot11s-id.h"

#include "ns3/assert.h"

namespace ns3
{
namespace dot11s
{

IeMeshId::IeMeshId()
{
    for (uint8_t i = 0; i < 32; i++)
    {
        m_meshId[i] = 0;
    }
}

IeMeshId::IeMeshId(std::string s)
{
    NS_ASSERT(s.size() < 32);
    const char* meshid = s.c_str();
    uint8_t len = 0;
    while (*meshid != 0 && len < 32)
    {
        m_meshId[len] = *meshid;
        meshid++;
        len++;
    }
    NS_ASSERT(len <= 32);
    while (len < 33)
    {
        m_meshId[len] = 0;
        len++;
    }
}

WifiInformationElementId
IeMeshId::ElementId() const
{
    return IE_MESH_ID;
}

bool
IeMeshId::IsEqual(const IeMeshId& o) const
{
    uint8_t i = 0;
    while (i < 32 && m_meshId[i] == o.m_meshId[i] && m_meshId[i] != 0)
    {
        i++;
    }
    return m_meshId[i] == o.m_meshId[i];
}

bool
IeMeshId::IsBroadcast() const
{
    return m_meshId[0] == 0;
}

char*
IeMeshId::PeekString() const
{
    return (char*)m_meshId;
}

uint16_t
IeMeshId::GetInformationFieldSize() const
{
    uint8_t size = 0;
    while (m_meshId[size] != 0 && size < 32)
    {
        size++;
    }
    NS_ASSERT(size <= 32);
    return size;
}

void
IeMeshId::SerializeInformationField(Buffer::Iterator i) const
{
    uint8_t size = 0;
    while (m_meshId[size] != 0 && size < 32)
    {
        i.WriteU8(m_meshId[size]);
        size++;
    }
}

uint16_t
IeMeshId::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    Buffer::Iterator i = start;
    NS_ASSERT(length <= 32);
    i.Read(m_meshId, length);
    m_meshId[length] = 0;
    return i.GetDistanceFrom(start);
}

void
IeMeshId::Print(std::ostream& os) const
{
    os << "MeshId=(meshId=" << PeekString() << ")";
}

bool
operator==(const IeMeshId& a, const IeMeshId& b)
{
    bool result(true);
    uint8_t size = 0;

    while (size < 32)
    {
        result = result && (a.m_meshId[size] == b.m_meshId[size]);
        if (a.m_meshId[size] == 0)
        {
            return result;
        }
        size++;
    }
    return result;
}

std::ostream&
operator<<(std::ostream& os, const IeMeshId& a)
{
    a.Print(os);
    return os;
}

std::istream&
operator>>(std::istream& is, IeMeshId& a)
{
    std::string str;
    is >> str;
    a = IeMeshId(str);
    return is;
}

} // namespace dot11s
} // namespace ns3
