/*
 * Copyright (c) 2026 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#ifndef IANA_LINK_TYPE_NUMBERS_H
#define IANA_LINK_TYPE_NUMBERS_H

#include <cstdint>

/**
 * @file
 * @brief Centralized definitions for packet capture (pcap) data link layer types.
 * @details This file contains the data link numbers as used by many packet analyzers.
 * (e.g., tcpdump, wireshark)
 * Only a small subset of data link types supported in ns-3 are included.
 * References:
 *  - https://www.tcpdump.org/linktypes.html
 *  - https://datatracker.ietf.org/doc/draft-ietf-opsawg-pcaplinktype/05/
 *  - https://www.iana.org/assignments/pcap/pcap.xhtml
 *
 */

namespace ns3
{
namespace iana
{
namespace linktype
{

constexpr uint16_t NULL_LINK = 0;          //!< BSD loopback encapsulation.
constexpr uint16_t ETHERNET = 1;           //!< IEEE 802.3 Ethernet (10Mb, 100Mb, 1000Mb, and up).
constexpr uint16_t PPP = 9;                //!< Point to point protococol [RFC1661]/[RFC1662].
constexpr uint16_t RAW = 101;              //!< Raw IP; begins with an IPv4 or IPv6 header.
constexpr uint16_t IEEE802_11 = 105;       //!< IEEE 802.11 wireless LAN.
constexpr uint16_t LINUX_SLL = 113;        //!< Linux "cooked" capture encapsulation.
constexpr uint16_t IEEE802_11_PRISM = 119; //!< Prism monitor mode.
constexpr uint16_t IEEE802_15_4_WITHFCS = 195; //!< IEEE 802.15.4 packets with FCS.
constexpr uint16_t IEEE802_15_4_NOFCS = 230;   //!< IEEE 802.15.4 packets without FCS.
constexpr uint16_t NETLINK = 253;              //!< Linux Netlink capture encapsulation [RFC3549].
constexpr uint16_t LORATAP = 270;              //!< LoRaTap pseudo-header.

} // namespace linktype
} // namespace iana
} // namespace ns3

#endif // IANA_LINK_TYPE_NUMBERS_H
