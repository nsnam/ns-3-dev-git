/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sharan Naribole <sharan.naribole@gmail.com>
 */

#ifndef WIFI_NS3_CONSTANTS_H
#define WIFI_NS3_CONSTANTS_H

/**
 * @file
 * @ingroup wifi
 * Declaration of default values used across wifi module
 */

namespace ns3
{

/// UL MU Data Disable flag at non-AP STA
static constexpr bool DEFAULT_WIFI_UL_MU_DATA_DISABLE{false};

/// UL MU Data Disable Rx support at AP
static constexpr bool DEFAULT_WIFI_UL_MU_DATA_DISABLE_RX{true};

} // namespace ns3

#endif /* WIFI_NS3_CONSTANTS_H */
