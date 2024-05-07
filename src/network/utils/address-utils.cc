/*
 * Copyright (c) 2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "address-utils.h"

#include "inet-socket-address.h"
#include "inet6-socket-address.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("AddressUtils");

void
WriteTo(Buffer::Iterator& i, Ipv4Address ad)
{
    NS_LOG_FUNCTION(&i << &ad);
    i.WriteHtonU32(ad.Get());
}

void
WriteTo(Buffer::Iterator& i, Ipv6Address ad)
{
    NS_LOG_FUNCTION(&i << &ad);
    uint8_t buf[16];
    ad.GetBytes(buf);
    i.Write(buf, 16);
}

void
WriteTo(Buffer::Iterator& i, const Address& ad)
{
    NS_LOG_FUNCTION(&i << &ad);
    uint8_t mac[Address::MAX_SIZE];
    ad.CopyTo(mac);
    i.Write(mac, ad.GetLength());
}

void
WriteTo(Buffer::Iterator& i, Mac64Address ad)
{
    NS_LOG_FUNCTION(&i << &ad);
    uint8_t mac[8];
    ad.CopyTo(mac);
    i.Write(mac, 8);
}

void
WriteTo(Buffer::Iterator& i, Mac48Address ad)
{
    NS_LOG_FUNCTION(&i << &ad);
    uint8_t mac[6];
    ad.CopyTo(mac);
    i.Write(mac, 6);
}

void
WriteTo(Buffer::Iterator& i, Mac16Address ad)
{
    NS_LOG_FUNCTION(&i << &ad);
    uint8_t mac[2];
    ad.CopyTo(mac);
    i.Write(mac + 1, 1);
    i.Write(mac, 1);
}

void
ReadFrom(Buffer::Iterator& i, Ipv4Address& ad)
{
    NS_LOG_FUNCTION(&i << &ad);
    ad.Set(i.ReadNtohU32());
}

void
ReadFrom(Buffer::Iterator& i, Ipv6Address& ad)
{
    NS_LOG_FUNCTION(&i << &ad);
    uint8_t ipv6[16];
    i.Read(ipv6, 16);
    ad.Set(ipv6);
}

void
ReadFrom(Buffer::Iterator& i, Address& ad, uint32_t len)
{
    NS_LOG_FUNCTION(&i << &ad << len);
    uint8_t mac[Address::MAX_SIZE];
    i.Read(mac, len);
    ad.CopyFrom(mac, len);
}

void
ReadFrom(Buffer::Iterator& i, Mac64Address& ad)
{
    NS_LOG_FUNCTION(&i << &ad);
    uint8_t mac[8];
    i.Read(mac, 8);
    ad.CopyFrom(mac);
}

void
ReadFrom(Buffer::Iterator& i, Mac48Address& ad)
{
    NS_LOG_FUNCTION(&i << &ad);
    uint8_t mac[6];
    i.Read(mac, 6);
    ad.CopyFrom(mac);
}

void
ReadFrom(Buffer::Iterator& i, Mac16Address& ad)
{
    NS_LOG_FUNCTION(&i << &ad);
    uint8_t mac[2];
    i.Read(mac + 1, 1);
    i.Read(mac, 1);
    ad.CopyFrom(mac);
}

namespace addressUtils
{

bool
IsMulticast(const Address& ad)
{
    NS_LOG_FUNCTION(&ad);
    if (InetSocketAddress::IsMatchingType(ad))
    {
        InetSocketAddress inetAddr = InetSocketAddress::ConvertFrom(ad);
        Ipv4Address ipv4 = inetAddr.GetIpv4();
        return ipv4.IsMulticast();
    }
    else if (Ipv4Address::IsMatchingType(ad))
    {
        Ipv4Address ipv4 = Ipv4Address::ConvertFrom(ad);
        return ipv4.IsMulticast();
    }
    else if (Inet6SocketAddress::IsMatchingType(ad))
    {
        Inet6SocketAddress inetAddr = Inet6SocketAddress::ConvertFrom(ad);
        Ipv6Address ipv6 = inetAddr.GetIpv6();
        return ipv6.IsMulticast();
    }
    else if (Ipv6Address::IsMatchingType(ad))
    {
        Ipv6Address ipv6 = Ipv6Address::ConvertFrom(ad);
        return ipv6.IsMulticast();
    }

    return false;
}

Address
ConvertToSocketAddress(const Address& address, uint16_t port)
{
    NS_LOG_FUNCTION(address << port);
    Address convertedAddress;
    if (Ipv4Address::IsMatchingType(address))
    {
        convertedAddress = InetSocketAddress(Ipv4Address::ConvertFrom(address), port);
        NS_LOG_DEBUG("Address converted to " << convertedAddress);
    }
    else if (Ipv6Address::IsMatchingType(address))
    {
        convertedAddress = Inet6SocketAddress(Ipv6Address::ConvertFrom(address), port);
        NS_LOG_DEBUG("Address converted to " << convertedAddress);
    }
    else
    {
        NS_FATAL_ERROR("This function should be called for an IPv4 or an IPv6 address");
    }
    return convertedAddress;
}

} // namespace addressUtils

} // namespace ns3
