/*
 * Copyright (c) 2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ssid.h"

namespace ns3
{

Ssid::Ssid()
{
    m_length = 0;
    for (uint8_t i = 0; i < 33; i++)
    {
        m_ssid[i] = 0;
    }
}

Ssid::Ssid(std::string s)
{
    NS_ASSERT(s.size() <= 32);
    const char* ssid = s.c_str();
    uint8_t len = 0;
    while (*ssid != 0 && len < 32)
    {
        m_ssid[len] = *ssid;
        ssid++;
        len++;
    }
    NS_ASSERT(len <= 32);
    m_length = len;
    while (len < 33)
    {
        m_ssid[len] = 0;
        len++;
    }
}

void
Ssid::Print(std::ostream& os) const
{
    os << "SSID=[" << PeekString() << "]";
}

bool
Ssid::IsEqual(const Ssid& o) const
{
    uint8_t i = 0;
    while (i < 32 && m_ssid[i] == o.m_ssid[i] && m_ssid[i] != 0)
    {
        i++;
    }
    return m_ssid[i] == o.m_ssid[i];
}

bool
Ssid::IsBroadcast() const
{
    return m_ssid[0] == 0;
}

char*
Ssid::PeekString() const
{
    // It is safe to return a pointer to the buffer because it is
    // guaranteed to be zero-terminated.
    return (char*)m_ssid;
}

WifiInformationElementId
Ssid::ElementId() const
{
    return IE_SSID;
}

uint16_t
Ssid::GetInformationFieldSize() const
{
    return m_length;
}

void
Ssid::SerializeInformationField(Buffer::Iterator start) const
{
    NS_ASSERT(m_length <= 32);
    start.Write(m_ssid, m_length);
}

uint16_t
Ssid::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    m_length = length;
    NS_ASSERT(m_length <= 32);
    start.Read(m_ssid, m_length);
    return length;
}

ATTRIBUTE_HELPER_CPP(Ssid);

std::istream&
operator>>(std::istream& is, Ssid& ssid)
{
    std::string str;
    is >> str;
    ssid = Ssid(str);
    return is;
}

} // namespace ns3
