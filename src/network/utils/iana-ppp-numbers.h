/*
 * Copyright (c) 2026 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Samar Sodhi <smrsodhi@gmail.com>
 *          Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#ifndef IANA_PPP_NUMBERS_H
#define IANA_PPP_NUMBERS_H

#include <cstdint>

namespace ns3
{
namespace iana
{
/**
 *
 * Centralized definitions for PPP protocol numbers.
 * This enumeration contains the PPP protocol numbers as defined by IANA.
 * - Reference: https://www.iana.org/assignments/ppp-numbers/ppp-numbers.xhtml
 */
enum PppDllNumbers : uint16_t
{
    IPV4_DLL = 0x0021, //!< IPv4 PPP protocol number
    IPV6_DLL = 0x0057, //!< IPv6 PPP protocol number
};
} // namespace iana
} // namespace ns3

#endif // IANA_PPP_NUMBERS_H
