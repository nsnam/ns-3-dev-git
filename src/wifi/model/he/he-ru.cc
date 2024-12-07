/*
 * Copyright (c) 2018
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "he-ru.h"

#include "ns3/abort.h"
#include "ns3/assert.h"

#include <optional>
#include <tuple>

namespace ns3
{

const HeRu::SubcarrierGroups HeRu::m_heRuSubcarrierGroups = {
    // RUs in a 20 MHz HE PPDU (Table 28-6)
    {{MHz_u{20}, HeRu::RU_26_TONE},
     {/* 1 */ {{-121, -96}},
      /* 2 */ {{-95, -70}},
      /* 3 */ {{-68, -43}},
      /* 4 */ {{-42, -17}},
      /* 5 */ {{-16, -4}, {4, 16}},
      /* 6 */ {{17, 42}},
      /* 7 */ {{43, 68}},
      /* 8 */ {{70, 95}},
      /* 9 */ {{96, 121}}}},
    {{MHz_u{20}, HeRu::RU_52_TONE},
     {/* 1 */ {{-121, -70}},
      /* 2 */ {{-68, -17}},
      /* 3 */ {{17, 68}},
      /* 4 */ {{70, 121}}}},
    {{MHz_u{20}, HeRu::RU_106_TONE},
     {/* 1 */ {{-122, -17}},
      /* 2 */ {{17, 122}}}},
    {{MHz_u{20}, HeRu::RU_242_TONE}, {/* 1 */ {{-122, -2}, {2, 122}}}},
    // RUs in a 40 MHz HE PPDU (Table 28-7)
    {{MHz_u{40}, HeRu::RU_26_TONE},
     {/* 1 */ {{-243, -218}},
      /* 2 */ {{-217, -192}},
      /* 3 */ {{-189, -164}},
      /* 4 */ {{-163, -138}},
      /* 5 */ {{-136, -111}},
      /* 6 */ {{-109, -84}},
      /* 7 */ {{-83, -58}},
      /* 8 */ {{-55, -30}},
      /* 9 */ {{-29, -4}},
      /* 10 */ {{4, 29}},
      /* 11 */ {{30, 55}},
      /* 12 */ {{58, 83}},
      /* 13 */ {{84, 109}},
      /* 14 */ {{111, 136}},
      /* 15 */ {{138, 163}},
      /* 16 */ {{164, 189}},
      /* 17 */ {{192, 217}},
      /* 18 */ {{218, 243}}}},
    {{MHz_u{40}, HeRu::RU_52_TONE},
     {/* 1 */ {{-243, -192}},
      /* 2 */ {{-189, -138}},
      /* 3 */ {{-109, -58}},
      /* 4 */ {{-55, -4}},
      /* 5 */ {{4, 55}},
      /* 6 */ {{58, 109}},
      /* 7 */ {{138, 189}},
      /* 8 */ {{192, 243}}}},
    {{MHz_u{40}, HeRu::RU_106_TONE},
     {/* 1 */ {{-243, -138}},
      /* 2 */ {{-109, -4}},
      /* 3 */ {{4, 109}},
      /* 4 */ {{138, 243}}}},
    {{MHz_u{40}, HeRu::RU_242_TONE},
     {/* 1 */ {{-244, -3}},
      /* 2 */ {{3, 244}}}},
    {{MHz_u{40}, HeRu::RU_484_TONE}, {/* 1 */ {{-244, -3}, {3, 244}}}},
    // RUs in an 80 MHz HE PPDU (Table 28-8)
    {{MHz_u{80}, HeRu::RU_26_TONE},
     {/* 1 */ {{-499, -474}},
      /* 2 */ {{-473, -448}},
      /* 3 */ {{-445, -420}},
      /* 4 */ {{-419, -394}},
      /* 5 */ {{-392, -367}},
      /* 6 */ {{-365, -340}},
      /* 7 */ {{-339, -314}},
      /* 8 */ {{-311, -286}},
      /* 9 */ {{-285, -260}},
      /* 10 */ {{-257, -232}},
      /* 11 */ {{-231, -206}},
      /* 12 */ {{-203, -178}},
      /* 13 */ {{-177, -152}},
      /* 14 */ {{-150, -125}},
      /* 15 */ {{-123, -98}},
      /* 16 */ {{-97, -72}},
      /* 17 */ {{-69, -44}},
      /* 18 */ {{-43, -18}},
      /* 19 */ {{-16, -4}, {4, 16}},
      /* 20 */ {{18, 43}},
      /* 21 */ {{44, 69}},
      /* 22 */ {{72, 97}},
      /* 23 */ {{98, 123}},
      /* 24 */ {{125, 150}},
      /* 25 */ {{152, 177}},
      /* 26 */ {{178, 203}},
      /* 27 */ {{206, 231}},
      /* 28 */ {{232, 257}},
      /* 29 */ {{260, 285}},
      /* 30 */ {{286, 311}},
      /* 31 */ {{314, 339}},
      /* 32 */ {{340, 365}},
      /* 33 */ {{367, 392}},
      /* 34 */ {{394, 419}},
      /* 35 */ {{420, 445}},
      /* 36 */ {{448, 473}},
      /* 37 */ {{474, 499}}}},
    {{MHz_u{80}, HeRu::RU_52_TONE},
     {/* 1 */ {{-499, -448}},
      /* 2 */ {{-445, -394}},
      /* 3 */ {{-365, -314}},
      /* 4 */ {{-311, -260}},
      /* 5 */ {{-257, -206}},
      /* 6 */ {{-203, -152}},
      /* 7 */ {{-123, -72}},
      /* 8 */ {{-69, -18}},
      /* 9 */ {{18, 69}},
      /* 10 */ {{72, 123}},
      /* 11 */ {{152, 203}},
      /* 12 */ {{206, 257}},
      /* 13 */ {{260, 311}},
      /* 14 */ {{314, 365}},
      /* 15 */ {{394, 445}},
      /* 16 */ {{448, 499}}}},
    {{MHz_u{80}, HeRu::RU_106_TONE},
     {/* 1 */ {{-499, -394}},
      /* 2 */ {{-365, -260}},
      /* 3 */ {{-257, -152}},
      /* 4 */ {{-123, -18}},
      /* 5 */ {{18, 123}},
      /* 6 */ {{152, 257}},
      /* 7 */ {{260, 365}},
      /* 8 */ {{394, 499}}}},
    {{MHz_u{80}, HeRu::RU_242_TONE},
     {/* 1 */ {{-500, -259}},
      /* 2 */ {{-258, -17}},
      /* 3 */ {{17, 258}},
      /* 4 */ {{259, 500}}}},
    {{MHz_u{80}, HeRu::RU_484_TONE},
     {/* 1 */ {{-500, -17}},
      /* 2 */ {{17, 500}}}},
    {{MHz_u{80}, HeRu::RU_996_TONE}, {/* 1 */ {{-500, -3}, {3, 500}}}},
};

// Table 27-26 IEEE802.11ax-2021
const HeRu::RuAllocationMap HeRu::m_heRuAllocations = {
    // clang-format off
    {0,
     {HeRu::RuSpec{HeRu::RU_26_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 4, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 6, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 7, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 8, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 9, true}}},
    {1,
     {HeRu::RuSpec{HeRu::RU_26_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 4, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 6, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 7, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 4, true}}},
    {2,
     {HeRu::RuSpec{HeRu::RU_26_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 4, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 8, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 9, true}}},
    {3,
     {HeRu::RuSpec{HeRu::RU_26_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 4, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 4, true}}},
    {4,
     {HeRu::RuSpec{HeRu::RU_26_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 6, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 7, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 8, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 9, true}}},
    {5,
     {HeRu::RuSpec{HeRu::RU_26_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 6, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 7, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 4, true}}},
    {6,
     {HeRu::RuSpec{HeRu::RU_26_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 8, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 9, true}}},
    {7,
     {HeRu::RuSpec{HeRu::RU_26_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 4, true}}},
    {8,
     {HeRu::RuSpec{HeRu::RU_52_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 4, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 6, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 7, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 8, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 9, true}}},
    {9,
     {HeRu::RuSpec{HeRu::RU_52_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 4, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 6, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 7, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 4, true}}},
    {10,
     {HeRu::RuSpec{HeRu::RU_52_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 4, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 8, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 9, true}}},
    {11,
     {HeRu::RuSpec{HeRu::RU_52_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 4, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 4, true}}},
    {12,
     {HeRu::RuSpec{HeRu::RU_52_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 6, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 7, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 8, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 9, true}}},
    {13,
     {HeRu::RuSpec{HeRu::RU_52_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 6, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 7, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 4, true}}},
    {14,
     {HeRu::RuSpec{HeRu::RU_52_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 8, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 9, true}}},
    {15,
     {HeRu::RuSpec{HeRu::RU_52_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 4, true}}},
    {16,
     {HeRu::RuSpec{HeRu::RU_52_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_106_TONE, 2, true}}},
    {24,
     {HeRu::RuSpec{HeRu::RU_106_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 4, true}}},
    {32,
     {HeRu::RuSpec{HeRu::RU_26_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 4, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_106_TONE, 2, true}}},
    {40,
     {HeRu::RuSpec{HeRu::RU_26_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_106_TONE, 2, true}}},
    {48,
     {HeRu::RuSpec{HeRu::RU_52_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 4, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_106_TONE, 2, true}}},
    {56,
     {HeRu::RuSpec{HeRu::RU_52_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_106_TONE, 2, true}}},
    {64,
     {HeRu::RuSpec{HeRu::RU_106_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 6, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 7, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 8, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 9, true}}},
    {72,
     {HeRu::RuSpec{HeRu::RU_106_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 6, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 7, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 4, true}}},
    {80,
     {HeRu::RuSpec{HeRu::RU_106_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 8, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 9, true}}},
    {88,
     {HeRu::RuSpec{HeRu::RU_106_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 4, true}}},
    {96,
     {HeRu::RuSpec{HeRu::RU_106_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_106_TONE, 2, true}}},
    {112,
     {HeRu::RuSpec{HeRu::RU_52_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 2, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 3, true},
      HeRu::RuSpec{HeRu::RU_52_TONE, 4, true}}},
    {128,
     {HeRu::RuSpec{HeRu::RU_106_TONE, 1, true},
      HeRu::RuSpec{HeRu::RU_26_TONE, 5, true},
      HeRu::RuSpec{HeRu::RU_106_TONE, 2, true}}},
    {192,
      {HeRu::RuSpec{HeRu::RU_242_TONE, 1, true}}},
    {200,
      {HeRu::RuSpec{HeRu::RU_484_TONE, 1, true}}},
    {208,
      {HeRu::RuSpec{HeRu::RU_996_TONE, 1, true}}},
    // clang-format on
};

HeRu::RuSpecCompare::RuSpecCompare(MHz_u channelWidth, uint8_t p20Index)
    : m_channelWidth(channelWidth),
      m_p20Index(p20Index)
{
}

bool
HeRu::RuSpecCompare::operator()(const HeRu::RuSpec& lhs, const HeRu::RuSpec& rhs) const
{
    const auto lhsIndex = lhs.GetPhyIndex(m_channelWidth, m_p20Index);
    const auto rhsIndex = rhs.GetPhyIndex(m_channelWidth, m_p20Index);
    const auto lhsStartTone =
        HeRu::GetSubcarrierGroup(m_channelWidth, lhs.GetRuType(), lhsIndex).front().first;
    const auto rhsStartTone =
        HeRu::GetSubcarrierGroup(m_channelWidth, rhs.GetRuType(), rhsIndex).front().first;
    return lhsStartTone < rhsStartTone;
}

std::vector<HeRu::RuSpec>
HeRu::GetRuSpecs(uint8_t ruAllocation)
{
    std::optional<std::size_t> idx;
    if (((ruAllocation >= 0) && (ruAllocation <= 15)) || (ruAllocation == 112))
    {
        idx = ruAllocation;
    }
    else if (((ruAllocation >= 16) && (ruAllocation <= 95)) ||
             ((ruAllocation >= 192) && (ruAllocation <= 215)))
    {
        idx = ruAllocation & 0xF8;
    }
    else if ((ruAllocation >= 96) && (ruAllocation <= 111))
    {
        idx = ruAllocation & 0xF0;
    }
    else if ((ruAllocation >= 113) && (ruAllocation <= 115))
    {
        // Do not set idx to return an undefined RU allocation
    }
    else if ((ruAllocation >= 128) && (ruAllocation <= 191))
    {
        idx = ruAllocation & 0xC0;
    }
    else
    {
        NS_FATAL_ERROR("Reserved RU allocation " << +ruAllocation);
    }
    return idx.has_value() ? m_heRuAllocations.at(idx.value()) : std::vector<HeRu::RuSpec>{};
}

uint8_t
HeRu::GetEqualizedRuAllocation(RuType ruType, bool isOdd)
{
    switch (ruType)
    {
    case HeRu::RU_26_TONE:
        return 0;
    case HeRu::RU_52_TONE:
        return isOdd ? 15 : 112;
    case HeRu::RU_106_TONE:
        return isOdd ? 128 : 96;
    case HeRu::RU_242_TONE:
        return 192;
    case HeRu::RU_484_TONE:
        return 200;
    default:
        return 208;
    }
}

HeRu::RuSpec::RuSpec()
    : m_index(0) // indicates undefined RU
{
}

HeRu::RuSpec::RuSpec(RuType ruType, std::size_t index, bool primary80MHz)
    : m_ruType(ruType),
      m_index(index),
      m_primary80MHz(primary80MHz)
{
    NS_ABORT_MSG_IF(index == 0, "Index cannot be zero");
}

HeRu::RuType
HeRu::RuSpec::GetRuType() const
{
    NS_ABORT_MSG_IF(m_index == 0, "Undefined RU");
    return m_ruType;
}

std::size_t
HeRu::RuSpec::GetIndex() const
{
    NS_ABORT_MSG_IF(m_index == 0, "Undefined RU");
    return m_index;
}

bool
HeRu::RuSpec::GetPrimary80MHz() const
{
    NS_ABORT_MSG_IF(m_index == 0, "Undefined RU");
    return m_primary80MHz;
}

std::size_t
HeRu::RuSpec::GetPhyIndex(MHz_u bw, uint8_t p20Index) const
{
    bool primary80IsLower80 = (p20Index < bw / MHz_u{40});

    if (bw < MHz_u{160} || m_ruType == HeRu::RU_2x996_TONE ||
        (primary80IsLower80 && m_primary80MHz) || (!primary80IsLower80 && !m_primary80MHz))
    {
        return m_index;
    }
    else
    {
        return m_index + GetNRus(bw, m_ruType) / 2;
    }
}

std::size_t
HeRu::GetNRus(MHz_u bw, RuType ruType)
{
    if (bw == MHz_u{160} && ruType == RU_2x996_TONE)
    {
        return 1;
    }

    // if the bandwidth is 160MHz, search for the number of RUs available
    // in 80MHz and double the result.
    auto it = m_heRuSubcarrierGroups.find({(bw == MHz_u{160} ? MHz_u{80} : bw), ruType});

    if (it == m_heRuSubcarrierGroups.end())
    {
        return 0;
    }

    return (bw == MHz_u{160} ? 2 : 1) * it->second.size();
}

std::vector<HeRu::RuSpec>
HeRu::GetRusOfType(MHz_u bw, HeRu::RuType ruType)
{
    if (ruType == HeRu::RU_2x996_TONE)
    {
        NS_ASSERT(bw >= MHz_u{160});
        return {{ruType, 1, true}};
    }

    std::vector<HeRu::RuSpec> ret;
    std::vector<bool> primary80MHzSet{true};

    if (bw == MHz_u{160})
    {
        primary80MHzSet.push_back(false);
        bw = MHz_u{80};
    }

    for (auto primary80MHz : primary80MHzSet)
    {
        for (std::size_t ruIndex = 1;
             ruIndex <= HeRu::m_heRuSubcarrierGroups.at({bw, ruType}).size();
             ruIndex++)
        {
            ret.emplace_back(ruType, ruIndex, primary80MHz);
        }
    }
    return ret;
}

std::vector<HeRu::RuSpec>
HeRu::GetCentral26TonesRus(MHz_u bw, HeRu::RuType ruType)
{
    std::vector<std::size_t> indices;

    if (ruType == HeRu::RU_52_TONE || ruType == HeRu::RU_106_TONE)
    {
        if (bw == MHz_u{20})
        {
            indices.push_back(5);
        }
        else if (bw == MHz_u{40})
        {
            indices.insert(indices.end(), {5, 14});
        }
        else if (bw >= MHz_u{80})
        {
            indices.insert(indices.end(), {5, 14, 19, 24, 33});
        }
    }
    else if (ruType == HeRu::RU_242_TONE || ruType == HeRu::RU_484_TONE)
    {
        if (bw >= MHz_u{80})
        {
            indices.push_back(19);
        }
    }

    std::vector<HeRu::RuSpec> ret;
    std::vector<bool> primary80MHzSet{true};

    if (bw == MHz_u{160})
    {
        primary80MHzSet.push_back(false);
    }

    for (auto primary80MHz : primary80MHzSet)
    {
        for (const auto& index : indices)
        {
            ret.emplace_back(HeRu::RU_26_TONE, index, primary80MHz);
        }
    }
    return ret;
}

HeRu::SubcarrierGroup
HeRu::GetSubcarrierGroup(MHz_u bw, RuType ruType, std::size_t phyIndex)
{
    if (ruType == HeRu::RU_2x996_TONE) // handle special case of RU covering 160 MHz channel
    {
        NS_ABORT_MSG_IF(bw != MHz_u{160}, "2x996 tone RU can only be used on 160 MHz band");
        return {{-1012, -3}, {3, 1012}};
    }

    // Determine the shift to apply to tone indices for 160 MHz channel (i.e. -1012 to 1012), since
    // m_heRuSubcarrierGroups contains indices for lower 80 MHz subchannel (i.e. from -500 to 500).
    // The phyIndex is used to that aim.
    std::size_t indexInLower80MHz = phyIndex;
    std::size_t numRus = GetNRus(bw, ruType);
    int16_t shift = (bw == MHz_u{160}) ? -512 : 0;
    if (bw == MHz_u{160} && phyIndex > (numRus / 2))
    {
        // The provided index is that of the upper 80 MHz subchannel
        indexInLower80MHz = phyIndex - (numRus / 2);
        shift = 512;
    }

    auto it = m_heRuSubcarrierGroups.find({(bw == MHz_u{160} ? MHz_u{80} : bw), ruType});

    NS_ABORT_MSG_IF(it == m_heRuSubcarrierGroups.end(), "RU not found");
    NS_ABORT_MSG_IF(indexInLower80MHz > it->second.size(), "RU index not available");

    SubcarrierGroup group = it->second.at(indexInLower80MHz - 1);
    if (bw == MHz_u{160})
    {
        for (auto& range : group)
        {
            range.first += shift;
            range.second += shift;
        }
    }
    return group;
}

bool
HeRu::DoesOverlap(MHz_u bw, RuSpec ru, const std::vector<RuSpec>& v)
{
    // A 2x996-tone RU spans 160 MHz, hence it overlaps with any other RU
    if (bw == MHz_u{160} && ru.GetRuType() == RU_2x996_TONE && !v.empty())
    {
        return true;
    }

    // This function may be called by the MAC layer, hence the PHY index may have
    // not been set yet. Hence, we pass the "MAC" index to GetSubcarrierGroup instead
    // of the PHY index. This is fine because we compare the primary 80 MHz bands of
    // the two RUs below.
    SubcarrierGroup rangesRu = GetSubcarrierGroup(bw, ru.GetRuType(), ru.GetIndex());
    for (auto& p : v)
    {
        if (ru.GetPrimary80MHz() != p.GetPrimary80MHz())
        {
            // the two RUs are located in distinct 80MHz bands
            continue;
        }
        for (const auto& rangeRu : rangesRu)
        {
            SubcarrierGroup rangesP = GetSubcarrierGroup(bw, p.GetRuType(), p.GetIndex());
            for (auto& rangeP : rangesP)
            {
                if (rangeP.second >= rangeRu.first && rangeRu.second >= rangeP.first)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

bool
HeRu::DoesOverlap(MHz_u bw, RuSpec ru, const SubcarrierGroup& toneRanges, uint8_t p20Index)
{
    for (const auto& range : toneRanges)
    {
        if (bw == MHz_u{160} && ru.GetRuType() == RU_2x996_TONE)
        {
            return true;
        }

        SubcarrierGroup rangesRu =
            GetSubcarrierGroup(bw, ru.GetRuType(), ru.GetPhyIndex(bw, p20Index));
        for (auto& r : rangesRu)
        {
            if (range.second >= r.first && r.second >= range.first)
            {
                return true;
            }
        }
    }
    return false;
}

HeRu::RuSpec
HeRu::FindOverlappingRu(MHz_u bw, RuSpec referenceRu, RuType searchedRuType)
{
    std::size_t numRus = HeRu::GetNRus(bw, searchedRuType);

    std::size_t numRusPer80Mhz;
    std::vector<bool> primary80MhzFlags;
    if (bw == MHz_u{160})
    {
        primary80MhzFlags.push_back(true);
        primary80MhzFlags.push_back(false);
        numRusPer80Mhz = (searchedRuType == HeRu::RU_2x996_TONE ? 1 : numRus / 2);
    }
    else
    {
        primary80MhzFlags.push_back(referenceRu.GetPrimary80MHz());
        numRusPer80Mhz = numRus;
    }

    for (const auto primary80MHz : primary80MhzFlags)
    {
        std::size_t index = 1;
        for (std::size_t indexPer80Mhz = 1; indexPer80Mhz <= numRusPer80Mhz;
             ++indexPer80Mhz, ++index)
        {
            RuSpec searchedRu(searchedRuType, index, primary80MHz);
            if (DoesOverlap(bw, referenceRu, {searchedRu}))
            {
                return searchedRu;
            }
        }
    }
    NS_ABORT_MSG("The searched RU type " << searchedRuType << " was not found for bw=" << bw
                                         << " and referenceRu=" << referenceRu);
    return HeRu::RuSpec();
}

std::ostream&
operator<<(std::ostream& os, const HeRu::RuType& ruType)
{
    switch (ruType)
    {
    case HeRu::RU_26_TONE:
        os << "26-tones";
        break;
    case HeRu::RU_52_TONE:
        os << "52-tones";
        break;
    case HeRu::RU_106_TONE:
        os << "106-tones";
        break;
    case HeRu::RU_242_TONE:
        os << "242-tones";
        break;
    case HeRu::RU_484_TONE:
        os << "484-tones";
        break;
    case HeRu::RU_996_TONE:
        os << "996-tones";
        break;
    case HeRu::RU_2x996_TONE:
        os << "2x996-tones";
        break;
    default:
        NS_FATAL_ERROR("Unknown RU type");
    }
    return os;
}

std::ostream&
operator<<(std::ostream& os, const HeRu::RuSpec& ru)
{
    os << "RU{" << ru.GetRuType() << "/" << ru.GetIndex() << "/"
       << (ru.GetPrimary80MHz() ? "primary80MHz" : "secondary80MHz");
    os << "}";
    return os;
}

MHz_u
HeRu::GetBandwidth(RuType ruType)
{
    switch (ruType)
    {
    case RU_26_TONE:
        return MHz_u{2};
    case RU_52_TONE:
        return MHz_u{4};
    case RU_106_TONE:
        return MHz_u{8};
    case RU_242_TONE:
        return MHz_u{20};
    case RU_484_TONE:
        return MHz_u{40};
    case RU_996_TONE:
        return MHz_u{80};
    case RU_2x996_TONE:
        return MHz_u{160};
    default:
        NS_ABORT_MSG("RU type " << ruType << " not found");
        return MHz_u{0};
    }
}

HeRu::RuType
HeRu::GetRuType(MHz_u bandwidth)
{
    switch (static_cast<uint16_t>(bandwidth))
    {
    case 2:
        return RU_26_TONE;
    case 4:
        return RU_52_TONE;
    case 8:
        return RU_106_TONE;
    case 20:
        return RU_242_TONE;
    case 40:
        return RU_484_TONE;
    case 80:
        return RU_996_TONE;
    case 160:
        return RU_2x996_TONE;
    default:
        NS_ABORT_MSG(bandwidth << " MHz bandwidth not found");
        return RU_242_TONE;
    }
}

HeRu::RuType
HeRu::GetEqualSizedRusForStations(MHz_u bandwidth,
                                  std::size_t& nStations,
                                  std::size_t& nCentral26TonesRus)
{
    RuType ruType;
    uint8_t nRusAssigned = 0;

    // iterate over all the available RU types
    for (auto& ru : m_heRuSubcarrierGroups)
    {
        if (ru.first.first == bandwidth && ru.second.size() <= nStations)
        {
            ruType = ru.first.second;
            nRusAssigned = ru.second.size();
            break;
        }
        else if (bandwidth == MHz_u{160} && ru.first.first == MHz_u{80} &&
                 (2 * ru.second.size() <= nStations))
        {
            ruType = ru.first.second;
            nRusAssigned = 2 * ru.second.size();
            break;
        }
    }
    if (nRusAssigned == 0)
    {
        NS_ABORT_IF(bandwidth != MHz_u{160} || nStations != 1);
        nRusAssigned = 1;
        ruType = RU_2x996_TONE;
    }

    nStations = nRusAssigned;

    switch (ruType)
    {
    case RU_52_TONE:
    case RU_106_TONE:
        if (bandwidth == MHz_u{20})
        {
            nCentral26TonesRus = 1;
        }
        else if (bandwidth == MHz_u{40})
        {
            nCentral26TonesRus = 2;
        }
        else
        {
            nCentral26TonesRus = 5;
        }
        break;
    case RU_242_TONE:
    case RU_484_TONE:
        nCentral26TonesRus = (bandwidth >= MHz_u{80} ? 1 : 0);
        break;
    default:
        nCentral26TonesRus = 0;
    }

    if (bandwidth == MHz_u{160})
    {
        nCentral26TonesRus *= 2;
    }

    return ruType;
}

bool
HeRu::RuSpec::operator==(const RuSpec& other) const
{
    // we do not compare the RU PHY indices because they may be uninitialized for
    // one of the compared RUs. This event should not cause the comparison to evaluate
    // to false
    return m_ruType == other.m_ruType && m_index == other.m_index &&
           m_primary80MHz == other.m_primary80MHz;
}

bool
HeRu::RuSpec::operator!=(const RuSpec& other) const
{
    return !(*this == other);
}

bool
HeRu::RuSpec::operator<(const RuSpec& other) const
{
    // we do not compare the RU PHY indices because they may be uninitialized for
    // one of the compared RUs. This event should not cause the comparison to evaluate
    // to false
    return std::tie(m_ruType, m_index, m_primary80MHz) <
           std::tie(other.m_ruType, other.m_index, other.m_primary80MHz);
}

} // namespace ns3
