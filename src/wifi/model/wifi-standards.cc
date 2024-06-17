/*
 * Copyright (c) 2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "wifi-standards.h"

namespace ns3
{

const std::map<WifiStandard, std::list<WifiPhyBand>> wifiStandards = {
    {WIFI_STANDARD_80211a, {WIFI_PHY_BAND_5GHZ}},
    {WIFI_STANDARD_80211b, {WIFI_PHY_BAND_2_4GHZ}},
    {WIFI_STANDARD_80211g, {WIFI_PHY_BAND_2_4GHZ}},
    {WIFI_STANDARD_80211p, {WIFI_PHY_BAND_5GHZ}},
    {WIFI_STANDARD_80211n, {WIFI_PHY_BAND_2_4GHZ, WIFI_PHY_BAND_5GHZ}},
    {WIFI_STANDARD_80211ac, {WIFI_PHY_BAND_5GHZ}},
    {WIFI_STANDARD_80211ad, {WIFI_PHY_BAND_60GHZ}},
    {WIFI_STANDARD_80211ax, {WIFI_PHY_BAND_2_4GHZ, WIFI_PHY_BAND_5GHZ, WIFI_PHY_BAND_6GHZ}},
    {WIFI_STANDARD_80211be, {WIFI_PHY_BAND_2_4GHZ, WIFI_PHY_BAND_5GHZ, WIFI_PHY_BAND_6GHZ}},
};

} // namespace ns3
