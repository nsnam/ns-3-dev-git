/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-types.h"

#include "wifi-phy.h"

namespace ns3
{

WifiChannelConfig::WifiChannelConfig(const std::list<TupleWithoutUnits>& tuples)
{
    std::for_each(tuples.cbegin(), tuples.cend(), [this](auto&& t) {
        segments.emplace_back(std::make_from_tuple<SegmentWithoutUnits>(t));
    });
}

WifiChannelConfig
WifiChannelConfig::FromString(const std::string& settings, WifiStandard standard)
{
    WifiPhy::ChannelSettingsValue value;
    value.DeserializeFromString(settings, WifiPhy::GetChannelSegmentsChecker());
    auto channelCfg = WifiChannelConfig(value.Get());
    if (standard != WIFI_STANDARD_UNSPECIFIED)
    {
        WifiPhy::SetUnspecifiedChannelParams(channelCfg, standard);
    }
    return channelCfg;
}

} // namespace ns3
