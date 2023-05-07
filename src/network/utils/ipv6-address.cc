/*
 * Copyright (c) 2007-2008 Louis Pasteur University
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Sebastien Vincent <vincent@clarinet.u-strasbg.fr>
 */

#include "ipv6-address.h"

#include "mac16-address.h"
#include "mac48-address.h"
#include "mac64-address.h"

#include "ns3/assert.h"
#include "ns3/log.h"

#include <iomanip>
#include <memory>

#ifdef __WIN32__
#include <WS2tcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv6Address");

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

    /**
     * \brief Get a hash key.
     * \param k the key
     * \param length the length of the key
     * \param level the previous hash, or an arbitrary value
     * \return hash
     * \note Adapted from Jens Jakobsen implementation (chillispot).
     */
    static uint32_t lookuphash(unsigned char* k, uint32_t length, uint32_t level)
    {
        NS_LOG_FUNCTION(k << length << level);
#define mix(a, b, c)                                                                               \
    ({                                                                                             \
        (a) -= (b);                                                                                \
        (a) -= (c);                                                                                \
        (a) ^= ((c) >> 13);                                                                        \
        (b) -= (c);                                                                                \
        (b) -= (a);                                                                                \
        (b) ^= ((a) << 8);                                                                         \
        (c) -= (a);                                                                                \
        (c) -= (b);                                                                                \
        (c) ^= ((b) >> 13);                                                                        \
        (a) -= (b);                                                                                \
        (a) -= (c);                                                                                \
        (a) ^= ((c) >> 12);                                                                        \
        (b) -= (c);                                                                                \
        (b) -= (a);                                                                                \
        (b) ^= ((a) << 16);                                                                        \
        (c) -= (a);                                                                                \
        (c) -= (b);                                                                                \
        (c) ^= ((b) >> 5);                                                                         \
        (a) -= (b);                                                                                \
        (a) -= (c);                                                                                \
        (a) ^= ((c) >> 3);                                                                         \
        (b) -= (c);                                                                                \
        (b) -= (a);                                                                                \
        (b) ^= ((a) << 10);                                                                        \
        (c) -= (a);                                                                                \
        (c) -= (b);                                                                                \
        (c) ^= ((b) >> 15);                                                                        \
    })

        typedef uint32_t ub4; /* unsigned 4-byte quantities */
        uint32_t a = 0;
        uint32_t b = 0;
        uint32_t c = 0;
        uint32_t len = 0;

        /* Set up the internal state */
        len = length;
        a = b = 0x9e3779b9; /* the golden ratio; an arbitrary value */
        c = level;          /* the previous hash value */

        /* handle most of the key */
        while (len >= 12)
        {
            a += (k[0] + ((ub4)k[1] << 8) + ((ub4)k[2] << 16) + ((ub4)k[3] << 24));
            b += (k[4] + ((ub4)k[5] << 8) + ((ub4)k[6] << 16) + ((ub4)k[7] << 24));
            c += (k[8] + ((ub4)k[9] << 8) + ((ub4)k[10] << 16) + ((ub4)k[11] << 24));
            mix(a, b, c);
            k += 12;
            len -= 12;
        }

        /* handle the last 11 bytes */
        c += length;
        switch (len) /* all the case statements fall through */
        {
        case 11:
            c += ((ub4)k[10] << 24);
        case 10:
            c += ((ub4)k[9] << 16);
        case 9:
            c += ((ub4)k[8] << 8); /* the first byte of c is reserved for the length */
        case 8:
            b += ((ub4)k[7] << 24);
        case 7:
            b += ((ub4)k[6] << 16);
        case 6:
            b += ((ub4)k[5] << 8);
        case 5:
            b += k[4];
        case 4:
            a += ((ub4)k[3] << 24);
        case 3:
            a += ((ub4)k[2] << 16);
        case 2:
            a += ((ub4)k[1] << 8);
        case 1:
            a += k[0];
            /* case 0: nothing left to add */
        }
        mix(a, b, c);

#undef mix

        /* report the result */
        return c;
    }

#ifdef __cplusplus
}
#endif

Ipv6Address::Ipv6Address()
{
    NS_LOG_FUNCTION(this);
    memset(m_address, 0x00, 16);
    m_initialized = false;
}

Ipv6Address::Ipv6Address(const Ipv6Address& addr)
{
    // Do not add function logging here, to avoid stack overflow
    memcpy(m_address, addr.m_address, 16);
    m_initialized = true;
}

Ipv6Address::Ipv6Address(const Ipv6Address* addr)
{
    // Do not add function logging here, to avoid stack overflow
    memcpy(m_address, addr->m_address, 16);
    m_initialized = true;
}

Ipv6Address::Ipv6Address(const char* address)
{
    NS_LOG_FUNCTION(this << address);

    if (inet_pton(AF_INET6, address, m_address) <= 0)
    {
        memset(m_address, 0x00, 16);
        NS_LOG_LOGIC("Error, can not build an IPv6 address from an invalid string: " << address);
        m_initialized = false;
        return;
    }
    m_initialized = true;
}

Ipv6Address::Ipv6Address(uint8_t address[16])
{
    NS_LOG_FUNCTION(this << &address);
    /* 128 bit => 16 bytes */
    memcpy(m_address, address, 16);
    m_initialized = true;
}

Ipv6Address::~Ipv6Address()
{
    /* do nothing */
    NS_LOG_FUNCTION(this);
}

void
Ipv6Address::Set(const char* address)
{
    NS_LOG_FUNCTION(this << address);
    if (inet_pton(AF_INET6, address, m_address) <= 0)
    {
        memset(m_address, 0x00, 16);
        NS_LOG_LOGIC("Error, can not build an IPv6 address from an invalid string: " << address);
        m_initialized = false;
        return;
    }
    m_initialized = true;
}

void
Ipv6Address::Set(uint8_t address[16])
{
    /* 128 bit => 16 bytes */
    NS_LOG_FUNCTION(this << &address);
    memcpy(m_address, address, 16);
    m_initialized = true;
}

void
Ipv6Address::Serialize(uint8_t buf[16]) const
{
    NS_LOG_FUNCTION(this << &buf);
    memcpy(buf, m_address, 16);
}

Ipv6Address
Ipv6Address::Deserialize(const uint8_t buf[16])
{
    NS_LOG_FUNCTION(&buf);
    Ipv6Address ipv6((uint8_t*)buf);
    ipv6.m_initialized = true;
    return ipv6;
}

Ipv6Address
Ipv6Address::MakeIpv4MappedAddress(Ipv4Address addr)
{
    NS_LOG_FUNCTION(addr);
    uint8_t buf[16] = {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0xff,
        0xff,
        0x00,
        0x00,
        0x00,
        0x00,
    };
    addr.Serialize(&buf[12]);
    return (Ipv6Address(buf));
}

Ipv4Address
Ipv6Address::GetIpv4MappedAddress() const
{
    NS_LOG_FUNCTION(this);
    uint8_t buf[16];
    Ipv4Address v4Addr;

    Serialize(buf);
    v4Addr = Ipv4Address::Deserialize(&buf[12]);
    return (v4Addr);
}

Ipv6Address
Ipv6Address::MakeAutoconfiguredAddress(Address addr, Ipv6Address prefix)
{
    Ipv6Address ipv6Addr = Ipv6Address::GetAny();

    if (Mac64Address::IsMatchingType(addr))
    {
        ipv6Addr = Ipv6Address::MakeAutoconfiguredAddress(Mac64Address::ConvertFrom(addr), prefix);
    }
    else if (Mac48Address::IsMatchingType(addr))
    {
        ipv6Addr = Ipv6Address::MakeAutoconfiguredAddress(Mac48Address::ConvertFrom(addr), prefix);
    }
    else if (Mac16Address::IsMatchingType(addr))
    {
        ipv6Addr = Ipv6Address::MakeAutoconfiguredAddress(Mac16Address::ConvertFrom(addr), prefix);
    }
    else if (Mac8Address::IsMatchingType(addr))
    {
        ipv6Addr = Ipv6Address::MakeAutoconfiguredAddress(Mac8Address::ConvertFrom(addr), prefix);
    }

    if (ipv6Addr.IsAny())
    {
        NS_ABORT_MSG("Unknown address type");
    }
    return ipv6Addr;
}

Ipv6Address
Ipv6Address::MakeAutoconfiguredAddress(Address addr, Ipv6Prefix prefix)
{
    Ipv6Address ipv6PrefixAddr = Ipv6Address::GetOnes().CombinePrefix(prefix);
    return MakeAutoconfiguredAddress(addr, ipv6PrefixAddr);
}

Ipv6Address
Ipv6Address::MakeAutoconfiguredAddress(Mac16Address addr, Ipv6Address prefix)
{
    NS_LOG_FUNCTION(addr << prefix);
    Ipv6Address ret;
    uint8_t buf[2];
    uint8_t buf2[16];

    addr.CopyTo(buf);
    prefix.GetBytes(buf2);
    memset(buf2 + 8, 0, 8);

    memcpy(buf2 + 14, buf, 2);
    buf2[11] = 0xff;
    buf2[12] = 0xfe;

    ret.Set(buf2);
    return ret;
}

Ipv6Address
Ipv6Address::MakeAutoconfiguredAddress(Mac48Address addr, Ipv6Address prefix)
{
    NS_LOG_FUNCTION(addr << prefix);
    Ipv6Address ret;
    uint8_t buf[16];
    uint8_t buf2[16];

    addr.CopyTo(buf);
    prefix.GetBytes(buf2);

    memcpy(buf2 + 8, buf, 3);
    buf2[11] = 0xff;
    buf2[12] = 0xfe;
    memcpy(buf2 + 13, buf + 3, 3);
    buf2[8] ^= 0x02;

    ret.Set(buf2);
    return ret;
}

Ipv6Address
Ipv6Address::MakeAutoconfiguredAddress(Mac64Address addr, Ipv6Address prefix)
{
    NS_LOG_FUNCTION(addr << prefix);
    Ipv6Address ret;
    uint8_t buf[8];
    uint8_t buf2[16];

    addr.CopyTo(buf);
    prefix.GetBytes(buf2);

    memcpy(buf2 + 8, buf, 8);

    ret.Set(buf2);
    return ret;
}

Ipv6Address
Ipv6Address::MakeAutoconfiguredAddress(Mac8Address addr, Ipv6Address prefix)
{
    NS_LOG_FUNCTION(addr << prefix);
    Ipv6Address ret;
    uint8_t buf[2];
    uint8_t buf2[16];

    buf[0] = 0;
    addr.CopyTo(&buf[1]);
    prefix.GetBytes(buf2);
    memset(buf2 + 8, 0, 8);

    memcpy(buf2 + 14, buf, 2);
    buf2[11] = 0xff;
    buf2[12] = 0xfe;

    ret.Set(buf2);
    return ret;
}

Ipv6Address
Ipv6Address::MakeAutoconfiguredLinkLocalAddress(Address addr)
{
    Ipv6Address ipv6Addr = Ipv6Address::GetAny();

    if (Mac64Address::IsMatchingType(addr))
    {
        ipv6Addr = Ipv6Address::MakeAutoconfiguredLinkLocalAddress(Mac64Address::ConvertFrom(addr));
    }
    else if (Mac48Address::IsMatchingType(addr))
    {
        ipv6Addr = Ipv6Address::MakeAutoconfiguredLinkLocalAddress(Mac48Address::ConvertFrom(addr));
    }
    else if (Mac16Address::IsMatchingType(addr))
    {
        ipv6Addr = Ipv6Address::MakeAutoconfiguredLinkLocalAddress(Mac16Address::ConvertFrom(addr));
    }
    else if (Mac8Address::IsMatchingType(addr))
    {
        ipv6Addr = Ipv6Address::MakeAutoconfiguredLinkLocalAddress(Mac8Address::ConvertFrom(addr));
    }

    if (ipv6Addr.IsAny())
    {
        NS_ABORT_MSG("Unknown address type");
    }
    return ipv6Addr;
}

Ipv6Address
Ipv6Address::MakeAutoconfiguredLinkLocalAddress(Mac16Address addr)
{
    NS_LOG_FUNCTION(addr);
    Ipv6Address ret;
    uint8_t buf[2];
    uint8_t buf2[16];

    addr.CopyTo(buf);

    memset(buf2, 0x00, sizeof(buf2));
    buf2[0] = 0xfe;
    buf2[1] = 0x80;
    memcpy(buf2 + 14, buf, 2);
    buf2[11] = 0xff;
    buf2[12] = 0xfe;

    ret.Set(buf2);
    return ret;
}

Ipv6Address
Ipv6Address::MakeAutoconfiguredLinkLocalAddress(Mac48Address addr)
{
    NS_LOG_FUNCTION(addr);
    Ipv6Address ret;
    uint8_t buf[16];
    uint8_t buf2[16];

    addr.CopyTo(buf);

    memset(buf2, 0x00, sizeof(buf2));
    buf2[0] = 0xfe;
    buf2[1] = 0x80;
    memcpy(buf2 + 8, buf, 3);
    buf2[11] = 0xff;
    buf2[12] = 0xfe;
    memcpy(buf2 + 13, buf + 3, 3);
    buf2[8] ^= 0x02;

    ret.Set(buf2);
    return ret;
}

Ipv6Address
Ipv6Address::MakeAutoconfiguredLinkLocalAddress(Mac64Address addr)
{
    NS_LOG_FUNCTION(addr);
    Ipv6Address ret;
    uint8_t buf[8];
    uint8_t buf2[16];

    addr.CopyTo(buf);

    memset(buf2, 0x00, sizeof(buf2));
    buf2[0] = 0xfe;
    buf2[1] = 0x80;
    memcpy(buf2 + 8, buf, 8);

    ret.Set(buf2);
    return ret;
}

Ipv6Address
Ipv6Address::MakeAutoconfiguredLinkLocalAddress(Mac8Address addr)
{
    NS_LOG_FUNCTION(addr);
    Ipv6Address ret;
    uint8_t buf[2];
    uint8_t buf2[16];

    buf[0] = 0;
    addr.CopyTo(&buf[1]);

    memset(buf2, 0x00, sizeof(buf2));
    buf2[0] = 0xfe;
    buf2[1] = 0x80;
    memcpy(buf2 + 14, buf, 2);
    buf2[11] = 0xff;
    buf2[12] = 0xfe;

    ret.Set(buf2);
    return ret;
}

Ipv6Address
Ipv6Address::MakeSolicitedAddress(Ipv6Address addr)
{
    NS_LOG_FUNCTION(addr);
    uint8_t buf[16];
    uint8_t buf2[16];
    Ipv6Address ret;

    addr.Serialize(buf2);

    memset(buf, 0x00, sizeof(buf));
    buf[0] = 0xff;
    buf[1] = 0x02;
    buf[11] = 0x01;
    buf[12] = 0xff;
    buf[13] = buf2[13];
    buf[14] = buf2[14];
    buf[15] = buf2[15];

    ret.Set(buf);
    return ret;
}

void
Ipv6Address::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);

    char str[INET6_ADDRSTRLEN];

    if (inet_ntop(AF_INET6, m_address, str, INET6_ADDRSTRLEN))
    {
        os << str;
    }
}

bool
Ipv6Address::IsLocalhost() const
{
    NS_LOG_FUNCTION(this);
    static Ipv6Address localhost("::1");
    return (*this == localhost);
}

bool
Ipv6Address::IsMulticast() const
{
    NS_LOG_FUNCTION(this);
    return m_address[0] == 0xff;
}

bool
Ipv6Address::IsLinkLocalMulticast() const
{
    NS_LOG_FUNCTION(this);
    return m_address[0] == 0xff && m_address[1] == 0x02;
}

bool
Ipv6Address::IsIpv4MappedAddress() const
{
    NS_LOG_FUNCTION(this);
    static uint8_t v4MappedPrefix[12] =
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff};
    if (memcmp(m_address, v4MappedPrefix, sizeof(v4MappedPrefix)) == 0)
    {
        return (true);
    }
    return (false);
}

Ipv6Address
Ipv6Address::CombinePrefix(const Ipv6Prefix& prefix) const
{
    NS_LOG_FUNCTION(this << prefix);
    Ipv6Address ipv6;
    uint8_t addr[16];
    uint8_t pref[16];
    unsigned int i = 0;

    memcpy(addr, m_address, 16);
    ((Ipv6Prefix)prefix).GetBytes(pref);

    /* a little bit ugly... */
    for (i = 0; i < 16; i++)
    {
        addr[i] = addr[i] & pref[i];
    }
    ipv6.Set(addr);
    return ipv6;
}

bool
Ipv6Address::IsSolicitedMulticast() const
{
    NS_LOG_FUNCTION(this);

    static Ipv6Address documentation("ff02::1:ff00:0");
    return CombinePrefix(Ipv6Prefix(104)) == documentation;
}

bool
Ipv6Address::IsAllNodesMulticast() const
{
    NS_LOG_FUNCTION(this);
    static Ipv6Address allNodesI("ff01::1");
    static Ipv6Address allNodesL("ff02::1");
    static Ipv6Address allNodesR("ff03::1");
    return (*this == allNodesI || *this == allNodesL || *this == allNodesR);
}

bool
Ipv6Address::IsAllRoutersMulticast() const
{
    NS_LOG_FUNCTION(this);
    static Ipv6Address allroutersI("ff01::2");
    static Ipv6Address allroutersL("ff02::2");
    static Ipv6Address allroutersR("ff03::2");
    static Ipv6Address allroutersS("ff05::2");
    return (*this == allroutersI || *this == allroutersL || *this == allroutersR ||
            *this == allroutersS);
}

bool
Ipv6Address::IsAny() const
{
    NS_LOG_FUNCTION(this);
    static Ipv6Address any("::");
    return (*this == any);
}

bool
Ipv6Address::IsDocumentation() const
{
    NS_LOG_FUNCTION(this);
    static Ipv6Address documentation("2001:db8::0");
    return CombinePrefix(Ipv6Prefix(32)) == documentation;
}

bool
Ipv6Address::HasPrefix(const Ipv6Prefix& prefix) const
{
    NS_LOG_FUNCTION(this << prefix);

    Ipv6Address masked = CombinePrefix(prefix);
    Ipv6Address reference = Ipv6Address::GetOnes().CombinePrefix(prefix);

    return (masked == reference);
}

bool
Ipv6Address::IsMatchingType(const Address& address)
{
    NS_LOG_FUNCTION(address);
    return address.CheckCompatible(GetType(), 16);
}

Ipv6Address::operator Address() const
{
    return ConvertTo();
}

Address
Ipv6Address::ConvertTo() const
{
    NS_LOG_FUNCTION(this);
    uint8_t buf[16];
    Serialize(buf);
    return Address(GetType(), buf, 16);
}

Ipv6Address
Ipv6Address::ConvertFrom(const Address& address)
{
    NS_LOG_FUNCTION(address);
    NS_ASSERT(address.CheckCompatible(GetType(), 16));
    uint8_t buf[16];
    address.CopyTo(buf);
    return Deserialize(buf);
}

uint8_t
Ipv6Address::GetType()
{
    NS_LOG_FUNCTION_NOARGS();
    static uint8_t type = Address::Register();
    return type;
}

Ipv6Address
Ipv6Address::GetAllNodesMulticast()
{
    NS_LOG_FUNCTION_NOARGS();
    static Ipv6Address nmc("ff02::1");
    return nmc;
}

Ipv6Address
Ipv6Address::GetAllRoutersMulticast()
{
    NS_LOG_FUNCTION_NOARGS();
    static Ipv6Address rmc("ff02::2");
    return rmc;
}

Ipv6Address
Ipv6Address::GetAllHostsMulticast()
{
    NS_LOG_FUNCTION_NOARGS();
    static Ipv6Address hmc("ff02::3");
    return hmc;
}

Ipv6Address
Ipv6Address::GetLoopback()
{
    NS_LOG_FUNCTION_NOARGS();
    static Ipv6Address loopback("::1");
    return loopback;
}

Ipv6Address
Ipv6Address::GetZero()
{
    NS_LOG_FUNCTION_NOARGS();
    static Ipv6Address zero("::");
    return zero;
}

Ipv6Address
Ipv6Address::GetAny()
{
    NS_LOG_FUNCTION_NOARGS();
    static Ipv6Address any("::");
    return any;
}

Ipv6Address
Ipv6Address::GetOnes()
{
    NS_LOG_FUNCTION_NOARGS();
    static Ipv6Address ones("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
    return ones;
}

void
Ipv6Address::GetBytes(uint8_t buf[16]) const
{
    NS_LOG_FUNCTION(this << &buf);
    memcpy(buf, m_address, 16);
}

bool
Ipv6Address::IsLinkLocal() const
{
    NS_LOG_FUNCTION(this);
    static Ipv6Address linkLocal("fe80::0");
    return CombinePrefix(Ipv6Prefix(64)) == linkLocal;
}

bool
Ipv6Address::IsInitialized() const
{
    NS_LOG_FUNCTION(this);
    return (m_initialized);
}

std::ostream&
operator<<(std::ostream& os, const Ipv6Address& address)
{
    address.Print(os);
    return os;
}

std::istream&
operator>>(std::istream& is, Ipv6Address& address)
{
    std::string str;
    is >> str;
    address = Ipv6Address(str.c_str());
    return is;
}

Ipv6Prefix::Ipv6Prefix()
{
    NS_LOG_FUNCTION(this);
    memset(m_prefix, 0x00, 16);
    m_prefixLength = 64;
}

Ipv6Prefix::Ipv6Prefix(const char* prefix)
{
    NS_LOG_FUNCTION(this << prefix);
    if (inet_pton(AF_INET6, prefix, m_prefix) <= 0)
    {
        NS_ABORT_MSG("Error, can not build an IPv6 prefix from an invalid string: " << prefix);
    }
    m_prefixLength = GetMinimumPrefixLength();
}

Ipv6Prefix::Ipv6Prefix(uint8_t prefix[16])
{
    NS_LOG_FUNCTION(this << &prefix);
    memcpy(m_prefix, prefix, 16);
    m_prefixLength = GetMinimumPrefixLength();
}

Ipv6Prefix::Ipv6Prefix(const char* prefix, uint8_t prefixLength)
{
    NS_LOG_FUNCTION(this << prefix);
    if (inet_pton(AF_INET6, prefix, m_prefix) <= 0)
    {
        NS_ABORT_MSG("Error, can not build an IPv6 prefix from an invalid string: " << prefix);
    }
    uint8_t autoLength = GetMinimumPrefixLength();
    NS_ASSERT_MSG(autoLength <= prefixLength,
                  "Ipv6Prefix: address and prefix are not compatible: " << Ipv6Address(prefix)
                                                                        << "/" << +prefixLength);

    m_prefixLength = prefixLength;
}

Ipv6Prefix::Ipv6Prefix(uint8_t prefix[16], uint8_t prefixLength)
{
    NS_LOG_FUNCTION(this << &prefix);
    memcpy(m_prefix, prefix, 16);

    uint8_t autoLength = GetMinimumPrefixLength();
    NS_ASSERT_MSG(autoLength <= prefixLength,
                  "Ipv6Prefix: address and prefix are not compatible: " << Ipv6Address(prefix)
                                                                        << "/" << +prefixLength);

    m_prefixLength = prefixLength;
}

Ipv6Prefix::Ipv6Prefix(uint8_t prefix)
{
    NS_LOG_FUNCTION(this << static_cast<uint32_t>(prefix));
    unsigned int nb = 0;
    unsigned int mod = 0;
    unsigned int i = 0;

    memset(m_prefix, 0x00, 16);
    m_prefixLength = prefix;

    NS_ASSERT(prefix <= 128);

    nb = prefix / 8;
    mod = prefix % 8;

    // protect memset with 'nb > 0' check to suppress
    // __warn_memset_zero_len compiler errors in some gcc>4.5.x
    if (nb > 0)
    {
        memset(m_prefix, 0xff, nb);
    }
    if (mod)
    {
        m_prefix[nb] = 0xff << (8 - mod);
    }

    if (nb < 16)
    {
        nb++;
        for (i = nb; i < 16; i++)
        {
            m_prefix[i] = 0x00;
        }
    }
}

Ipv6Prefix::Ipv6Prefix(const Ipv6Prefix& prefix)
{
    memcpy(m_prefix, prefix.m_prefix, 16);
    m_prefixLength = prefix.m_prefixLength;
}

Ipv6Prefix::Ipv6Prefix(const Ipv6Prefix* prefix)
{
    memcpy(m_prefix, prefix->m_prefix, 16);
    m_prefixLength = prefix->m_prefixLength;
}

Ipv6Prefix::~Ipv6Prefix()
{
    /* do nothing */
    NS_LOG_FUNCTION(this);
}

bool
Ipv6Prefix::IsMatch(Ipv6Address a, Ipv6Address b) const
{
    NS_LOG_FUNCTION(this << a << b);
    uint8_t addrA[16];
    uint8_t addrB[16];
    unsigned int i = 0;

    a.GetBytes(addrA);
    b.GetBytes(addrB);

    /* a little bit ugly... */
    for (i = 0; i < 16; i++)
    {
        if ((addrA[i] & m_prefix[i]) != (addrB[i] & m_prefix[i]))
        {
            return false;
        }
    }
    return true;
}

void
Ipv6Prefix::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);

    os << "/" << (unsigned int)GetPrefixLength();
}

Ipv6Prefix
Ipv6Prefix::GetLoopback()
{
    NS_LOG_FUNCTION_NOARGS();
    static Ipv6Prefix prefix((uint8_t)128);
    return prefix;
}

Ipv6Prefix
Ipv6Prefix::GetOnes()
{
    NS_LOG_FUNCTION_NOARGS();
    static Ipv6Prefix ones((uint8_t)128);
    return ones;
}

Ipv6Prefix
Ipv6Prefix::GetZero()
{
    NS_LOG_FUNCTION_NOARGS();
    static Ipv6Prefix prefix((uint8_t)0);
    return prefix;
}

void
Ipv6Prefix::GetBytes(uint8_t buf[16]) const
{
    NS_LOG_FUNCTION(this << &buf);
    memcpy(buf, m_prefix, 16);
}

Ipv6Address
Ipv6Prefix::ConvertToIpv6Address() const
{
    uint8_t prefixBytes[16];
    memcpy(prefixBytes, m_prefix, 16);

    Ipv6Address convertedPrefix = Ipv6Address(prefixBytes);
    return convertedPrefix;
}

uint8_t
Ipv6Prefix::GetPrefixLength() const
{
    NS_LOG_FUNCTION(this);
    return m_prefixLength;
}

void
Ipv6Prefix::SetPrefixLength(uint8_t prefixLength)
{
    NS_LOG_FUNCTION(this);
    m_prefixLength = prefixLength;
}

uint8_t
Ipv6Prefix::GetMinimumPrefixLength() const
{
    NS_LOG_FUNCTION(this);

    uint8_t prefixLength = 0;
    bool stop = false;

    for (int8_t i = 15; i >= 0 && !stop; i--)
    {
        uint8_t mask = m_prefix[i];

        for (uint8_t j = 0; j < 8 && !stop; j++)
        {
            if ((mask & 1) == 0)
            {
                mask = mask >> 1;
                prefixLength++;
            }
            else
            {
                stop = true;
            }
        }
    }

    return 128 - prefixLength;
}

std::ostream&
operator<<(std::ostream& os, const Ipv6Prefix& prefix)
{
    prefix.Print(os);
    return os;
}

std::istream&
operator>>(std::istream& is, Ipv6Prefix& prefix)
{
    std::string str;
    is >> str;
    prefix = Ipv6Prefix(str.c_str());
    return is;
}

size_t
Ipv6AddressHash::operator()(const Ipv6Address& x) const
{
    uint8_t buf[16];

    x.GetBytes(buf);

    return lookuphash(buf, sizeof(buf), 0);
}

ATTRIBUTE_HELPER_CPP(Ipv6Address);
ATTRIBUTE_HELPER_CPP(Ipv6Prefix);

} /* namespace ns3 */
