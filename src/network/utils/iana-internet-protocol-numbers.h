/*
 * Copyright (c) 2026 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 *
 */

/*
 * Centralized definitions for internet protocol numbers.
 * This enumeration contains the IEEE 802 protocol numbers as defined by IANA.
 * This represents only a portion of the complete list, protocols should
 * be added as needed.
 *
 * In the Internet Protocol version 4 (IPv4) [RFC791] there is a field
 * called "Protocol" to identify the next level protocol.  This is an 8
 * bit field.  In Internet Protocol version 6 (IPv6) [RFC8200], this field
 * is called the "Next Header" field.
 * References:
 * - https://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml

 */

#ifndef IANA_INTERNET_PROTOCOL_NUMBERS_H
#define IANA_INTERNET_PROTOCOL_NUMBERS_H

#include <cstdint>

namespace ns3
{
namespace iana
{
namespace internetprotocolnumbers
{
constexpr uint8_t HOPOPT = 0;          //!< IPv6 Hop-by-Hop Option [RFC8200]
constexpr uint8_t ICMP = 1;            //!< Internet Control Message for IPV4 [RFC792]
constexpr uint8_t IGMP = 2;            //!< Internet Group Management [RFC1112]
constexpr uint8_t TCP = 6;             //!< Transmission Control [RFC9293]
constexpr uint8_t UDP = 17;            //!< User Datagram [RFC768]
constexpr uint8_t IPV6_FRAG = 44;      //!< IPV6 Fragment Header
constexpr uint8_t DSR = 48;            //!< Dynamic Source Routing Protocol [RFC4728]
constexpr uint8_t ICMPV6 = 58;         //!< ICMP for IPv6 [RFC8200]
constexpr uint8_t NO_NEXT_HEADER = 59; //!< No Next Header for IPV4 and IPv6 [RFC8200]
constexpr uint8_t MANET = 138;         //!< MANET Protocols    [RFC5498]
} // namespace internetprotocolnumbers
} // namespace iana
} // namespace ns3

#endif // IANA_INTERNET_PROTOCOL_NUMBERS_H
