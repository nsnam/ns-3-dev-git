/*
 * Copyright (c) 2007 INRIA
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
