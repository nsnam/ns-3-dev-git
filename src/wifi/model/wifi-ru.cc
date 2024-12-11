/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "wifi-ru.h"

namespace ns3
{

MHz_u
WifiRu::GetBandwidth(RuType ruType)
{
    switch (ruType)
    {
    case RuType::RU_26_TONE:
        return MHz_u{2};
    case RuType::RU_52_TONE:
        return MHz_u{4};
    case RuType::RU_106_TONE:
        return MHz_u{8};
    case RuType::RU_242_TONE:
        return MHz_u{20};
    case RuType::RU_484_TONE:
        return MHz_u{40};
    case RuType::RU_996_TONE:
        return MHz_u{80};
    case RuType::RU_2x996_TONE:
        return MHz_u{160};
    case RuType::RU_4x996_TONE:
        return MHz_u{320};
    default:
        NS_ABORT_MSG("RU type " << ruType << " not found");
        return MHz_u{0};
    }
}

RuType
WifiRu::GetRuType(MHz_u bandwidth)
{
    switch (static_cast<uint16_t>(bandwidth))
    {
    case 2:
        return RuType::RU_26_TONE;
    case 4:
        return RuType::RU_52_TONE;
    case 8:
        return RuType::RU_106_TONE;
    case 20:
        return RuType::RU_242_TONE;
    case 40:
        return RuType::RU_484_TONE;
    case 80:
        return RuType::RU_996_TONE;
    case 160:
        return RuType::RU_2x996_TONE;
    case 320:
        return RuType::RU_4x996_TONE;
    default:
        NS_ABORT_MSG(bandwidth << " bandwidth not found");
        return RuType::RU_TYPE_MAX;
    }
}

} // namespace ns3
