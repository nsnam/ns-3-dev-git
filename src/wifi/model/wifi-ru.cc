/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "wifi-ru.h"

#include <algorithm>

namespace ns3
{

WifiRu::RuSpecCompare::RuSpecCompare(MHz_u channelWidth, uint8_t p20Index)
    : m_channelWidth(channelWidth),
      m_p20Index(p20Index)
{
}

bool
WifiRu::RuSpecCompare::operator()(const RuSpec& lhs, const RuSpec& rhs) const
{
    const auto lhsIndex = WifiRu::GetPhyIndex(lhs, m_channelWidth, m_p20Index);
    const auto rhsIndex = WifiRu::GetPhyIndex(rhs, m_channelWidth, m_p20Index);
    const auto lhsStartTone =
        WifiRu::GetSubcarrierGroup(m_channelWidth,
                                   WifiRu::GetRuType(lhs),
                                   lhsIndex,
                                   WifiRu::IsHe(lhs) ? WIFI_MOD_CLASS_HE : WIFI_MOD_CLASS_EHT)
            .front()
            .first;
    const auto rhsStartTone =
        WifiRu::GetSubcarrierGroup(m_channelWidth,
                                   WifiRu::GetRuType(rhs),
                                   rhsIndex,
                                   WifiRu::IsHe(lhs) ? WIFI_MOD_CLASS_HE : WIFI_MOD_CLASS_EHT)
            .front()
            .first;
    return lhsStartTone < rhsStartTone;
}

RuType
WifiRu::GetRuType(RuSpec ruVariant)
{
    return std::visit([&](auto&& ru) { return ru.GetRuType(); }, ruVariant);
}

std::size_t
WifiRu::GetIndex(RuSpec ruVariant)
{
    return std::visit([&](auto&& ru) { return ru.GetIndex(); }, ruVariant);
}

std::size_t
WifiRu::GetPhyIndex(RuSpec ruVariant, MHz_u bw, uint8_t p20Index)
{
    return std::visit([&](auto&& ru) { return ru.GetPhyIndex(bw, p20Index); }, ruVariant);
}

RuType
WifiRu::GetMaxRuType(WifiModulationClass mc)
{
    switch (mc)
    {
    case WIFI_MOD_CLASS_HE:
        return RuType::RU_2x996_TONE;
    case WIFI_MOD_CLASS_EHT:
        return RuType::RU_4x996_TONE;
    default:
        NS_ABORT_MSG("Unknown modulation class: " << mc);
        return RuType::RU_TYPE_MAX;
    }
}

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

std::size_t
WifiRu::GetNRus(MHz_u bw, RuType ruType, WifiModulationClass mc)
{
    if (ruType > GetMaxRuType(mc))
    {
        return 0;
    }
    return (mc == WIFI_MOD_CLASS_HE) ? HeRu::GetNRus(bw, ruType) : EhtRu::GetNRus(bw, ruType);
}

SubcarrierGroup
WifiRu::GetSubcarrierGroup(MHz_u bw, RuType ruType, std::size_t phyIndex, WifiModulationClass mc)
{
    return (mc == WIFI_MOD_CLASS_HE) ? HeRu::GetSubcarrierGroup(bw, ruType, phyIndex)
                                     : EhtRu::GetSubcarrierGroup(bw, ruType, phyIndex);
}

uint16_t
WifiRu::GetEqualizedRuAllocation(RuType ruType, bool isOdd, bool hasUsers, WifiModulationClass mc)
{
    return (mc == WIFI_MOD_CLASS_HE) ? HeRu::GetEqualizedRuAllocation(ruType, isOdd, hasUsers)
                                     : EhtRu::GetEqualizedRuAllocation(ruType, isOdd, hasUsers);
}

std::vector<WifiRu::RuSpec>
WifiRu::GetRuSpecs(uint16_t ruAllocation, WifiModulationClass mc)
{
    std::vector<RuSpec> ruSpecs;
    if (mc == WIFI_MOD_CLASS_HE)
    {
        auto heRuSpecs = HeRu::GetRuSpecs(ruAllocation);
        std::copy(heRuSpecs.begin(), heRuSpecs.end(), std::back_inserter(ruSpecs));
    }
    else
    {
        auto ehtRuSpecs = EhtRu::GetRuSpecs(ruAllocation);
        std::copy(ehtRuSpecs.begin(), ehtRuSpecs.end(), std::back_inserter(ruSpecs));
    }
    return ruSpecs;
}

std::vector<WifiRu::RuSpec>
WifiRu::GetRusOfType(MHz_u bw, RuType ruType, WifiModulationClass mc)
{
    std::vector<RuSpec> rus;
    if (mc == WIFI_MOD_CLASS_HE)
    {
        auto heRuSpecs = HeRu::GetRusOfType(bw, ruType);
        std::copy(heRuSpecs.begin(), heRuSpecs.end(), std::back_inserter(rus));
    }
    else
    {
        auto ehtRuSpecs = EhtRu::GetRusOfType(bw, ruType);
        std::copy(ehtRuSpecs.begin(), ehtRuSpecs.end(), std::back_inserter(rus));
    }
    return rus;
}

std::vector<WifiRu::RuSpec>
WifiRu::GetCentral26TonesRus(MHz_u bw, RuType ruType, WifiModulationClass mc)
{
    std::vector<RuSpec> central26TonesRus;
    if (mc == WIFI_MOD_CLASS_HE)
    {
        auto heRuSpecs = HeRu::GetCentral26TonesRus(bw, ruType);
        std::copy(heRuSpecs.begin(), heRuSpecs.end(), std::back_inserter(central26TonesRus));
    }
    else
    {
        auto ehtRuSpecs = EhtRu::GetCentral26TonesRus(bw, ruType);
        std::copy(ehtRuSpecs.begin(), ehtRuSpecs.end(), std::back_inserter(central26TonesRus));
    }
    return central26TonesRus;
}

bool
WifiRu::DoesOverlap(MHz_u bw, RuSpec ru, const std::vector<RuSpec>& v)
{
    if (IsHe(ru))
    {
        std::vector<HeRu::RuSpec> heRus;
        std::transform(v.cbegin(), v.cend(), std::back_inserter(heRus), [](const auto& heRu) {
            return std::get<HeRu::RuSpec>(heRu);
        });
        return HeRu::DoesOverlap(bw, std::get<HeRu::RuSpec>(ru), heRus);
    }

    std::vector<EhtRu::RuSpec> ehtRus;
    std::transform(v.cbegin(), v.cend(), std::back_inserter(ehtRus), [](const auto& ehtRu) {
        return std::get<EhtRu::RuSpec>(ehtRu);
    });
    return EhtRu::DoesOverlap(bw, std::get<EhtRu::RuSpec>(ru), ehtRus);
}

WifiRu::RuSpec
WifiRu::FindOverlappingRu(MHz_u bw, RuSpec referenceRu, RuType searchedRuType)
{
    return IsHe(referenceRu) ? RuSpec{HeRu::FindOverlappingRu(bw,
                                                              std::get<HeRu::RuSpec>(referenceRu),
                                                              searchedRuType)}
                             : RuSpec{EhtRu::FindOverlappingRu(bw,
                                                               std::get<EhtRu::RuSpec>(referenceRu),
                                                               searchedRuType)};
}

RuType
WifiRu::GetEqualSizedRusForStations(MHz_u bandwidth,
                                    std::size_t& nStations,
                                    std::size_t& nCentral26TonesRus,
                                    WifiModulationClass mc)
{
    return (mc == WIFI_MOD_CLASS_HE)
               ? HeRu::GetEqualSizedRusForStations(bandwidth, nStations, nCentral26TonesRus)
               : EhtRu::GetEqualSizedRusForStations(bandwidth, nStations, nCentral26TonesRus);
}

bool
WifiRu::IsHe(RuSpec ru)
{
    return std::holds_alternative<HeRu::RuSpec>(ru);
}

bool
WifiRu::IsEht(RuSpec ru)
{
    return std::holds_alternative<EhtRu::RuSpec>(ru);
}

std::ostream&
operator<<(std::ostream& os, const WifiRu::RuSpec& ruVariant)
{
    std::visit([&os](auto&& arg) { os << arg; }, ruVariant);
    return os;
}

} // namespace ns3
