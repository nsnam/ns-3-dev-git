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
WifiChannelConfig::FromString(const std::string& settings)
{
    WifiPhy::ChannelSettingsValue value;
    value.DeserializeFromString(settings, WifiPhy::GetChannelSegmentsChecker());
    return WifiChannelConfig(value.Get());
}

std::ostream&
operator<<(std::ostream& os, WifiChannelAccessStatus status)
{
    switch (status)
    {
    case WifiChannelAccessStatus::NOT_REQUESTED_NO_BACKOFF:
        return (os << "NOT_REQUESTED_NO_BACKOFF");
    case WifiChannelAccessStatus::NOT_REQUESTED_WITH_BACKOFF:
        return (os << "NOT_REQUESTED_WITH_BACKOFF");
    case WifiChannelAccessStatus::REQUESTED:
        return (os << "REQUESTED");
    case WifiChannelAccessStatus::GRANTED:
        return (os << "GRANTED");
    default:
        NS_ABORT_MSG("Invalid channel access status: " << static_cast<uint16_t>(status));
    }
    return os;
}

} // namespace ns3
