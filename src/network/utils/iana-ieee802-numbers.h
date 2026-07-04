/*
 * Copyright (c) 2026 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Samar Sodhi <smrsodhi@gmail.com>
 *          Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

/*
 * Centralized definitions for IEEE 802 protocol numbers.
 * This enumeration contains the IEEE 802 protocol numbers as defined by IANA.
 * References:
 * - https://www.iana.org/assignments/ieee-802-numbers/ieee-802-numbers.xhtml
 * - IPV4 RFC894
 * - ARP RFC1700
 * - IPV6 RFC2464
 * - LoWPAN RFC7973
 */

#ifndef IANA_IEEE_802_NUMBERS_H
#define IANA_IEEE_802_NUMBERS_H

#include <cstdint>

namespace ns3
{
namespace iana
{
namespace ieee802numbers
{
constexpr uint16_t IPV4 = 0x0800;   //!< IPv4 EtherType [RFC894]
constexpr uint16_t ARP = 0x0806;    //!< ARP EtherType [RFC1700]
constexpr uint16_t IPV6 = 0x86DD;   //!< IPv6 EtherType [RFC2464]
constexpr uint16_t LoWPAN = 0xA0ED; //!< LoWPAN EtherType (e.g., 6LowPan) [RFC7973]
} // namespace ieee802numbers
} // namespace iana
} // namespace ns3

#endif // IANA_IEEE_802_NUMBERS_H
