/*
 * Copyright (c) 2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "wifi-mac-trailer.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(WifiMacTrailer);

WifiMacTrailer::WifiMacTrailer()
{
}

WifiMacTrailer::~WifiMacTrailer()
{
}

TypeId
WifiMacTrailer::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WifiMacTrailer")
                            .SetParent<Trailer>()
                            .SetGroupName("Wifi")
                            .AddConstructor<WifiMacTrailer>();
    return tid;
}

TypeId
WifiMacTrailer::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
WifiMacTrailer::Print(std::ostream& os) const
{
}

uint32_t
WifiMacTrailer::GetSerializedSize() const
{
    return WIFI_MAC_FCS_LENGTH;
}

void
WifiMacTrailer::Serialize(Buffer::Iterator start) const
{
    start.Prev(WIFI_MAC_FCS_LENGTH);
    start.WriteU32(0);
}

uint32_t
WifiMacTrailer::Deserialize(Buffer::Iterator start)
{
    return WIFI_MAC_FCS_LENGTH;
}

} // namespace ns3
