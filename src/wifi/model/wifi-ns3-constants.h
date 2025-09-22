/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sharan Naribole <sharan.naribole@gmail.com>
 */

#ifndef WIFI_DEFAULTS_H
#define WIFI_DEFAULTS_H

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

/// Default channel switch delay expressed in microseconds
static constexpr uint64_t DEFAULT_CHANNEL_SWITCH_DELAY_USEC{250};

/// Default channel switch delay
extern const Time DEFAULT_CHANNEL_SWITCH_DELAY;

} // namespace ns3

#endif /* WIFI_DEFAULTS_H */
