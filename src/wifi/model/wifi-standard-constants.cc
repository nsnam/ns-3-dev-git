/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "wifi-standard-constants.h"

namespace ns3
{

const Time WIFI_TU = MicroSeconds(1024);

const Time EMLSR_RX_PHY_START_DELAY = MicroSeconds(20);

const Time OFDM_SIFS_TIME_20MHZ = MicroSeconds(16);

const Time OFDM_SLOT_TIME_20MHZ = MicroSeconds(9);

const Time OFDM_SIFS_TIME_10MHZ = MicroSeconds(32);

const Time OFDM_SLOT_TIME_10MHZ = MicroSeconds(13);

const Time OFDM_SIFS_TIME_5MHZ = MicroSeconds(64);

const Time OFDM_SLOT_TIME_5MHZ = MicroSeconds(21);

const Time DSSS_SIFS_TIME = MicroSeconds(10);

const Time DSSS_SLOT_TIME = MicroSeconds(20);

const Time MAX_PROPAGATION_DELAY = MicroSeconds(1);

const Time MEDIUM_SYNC_THRESHOLD = MicroSeconds(72);

} // namespace ns3
