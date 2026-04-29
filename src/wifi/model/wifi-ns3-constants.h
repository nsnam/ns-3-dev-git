/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sharan Naribole <sharan.naribole@gmail.com>
 */

#ifndef WIFI_NS3_CONSTANTS_H
#define WIFI_NS3_CONSTANTS_H

#include "ns3/nstime.h"

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

/**
 *  @brief Default Beacon interval
 *
 *  Default beacon interval.
 *  Note: inlining Time can cause issues on MSVC due to how DLLs are
 *  loaded and runtime precision tracking. We temporarily call a function
 *  that returns the interval declared in the wifi-ns3-constants.cc file,
 *  until a permanent solution is found.
 *
 *  @return the default beacon interval
 */
Time DEFAULT_BEACON_INTERVAL();

} // namespace ns3

#endif /* WIFI_NS3_CONSTANTS_H */
