/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-types.h"

namespace ns3
{

WifiChannelConfig::WifiChannelConfig(const std::list<TupleWithoutUnits>& tuples)
{
    std::for_each(tuples.cbegin(), tuples.cend(), [this](auto&& t) {
        segments.emplace_back(std::make_from_tuple<SegmentWithoutUnits>(t));
    });
}

} // namespace ns3
