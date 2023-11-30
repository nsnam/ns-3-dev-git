/*
 * Copyright (c) 2021 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "eht-capabilities.h"

#include <bitset>
#include <cmath>

namespace ns3
{

uint16_t
EhtMacCapabilities::GetSize() const
{
    return 2;
}

void
EhtMacCapabilities::Serialize(Buffer::Iterator& start) const
{
    uint16_t val = epcsPriorityAccessSupported | (ehtOmControlSupport << 1) |
                   (triggeredTxopSharingMode1Support << 2) |
                   (triggeredTxopSharingMode2Support << 3) | (restrictedTwtSupport << 4) |
                   (scsTrafficDescriptionSupport << 5) | (maxMpduLength << 6) |
                   (maxAmpduLengthExponentExtension << 8);
    start.WriteHtolsbU16(val);
}

uint16_t
EhtMacCapabilities::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    uint16_t val = i.ReadLsbtohU16();
    epcsPriorityAccessSupported = val & 0x0001;
    ehtOmControlSupport = (val >> 1) & 0x0001;
    triggeredTxopSharingMode1Support = (val >> 2) & 0x0001;
    triggeredTxopSharingMode2Support = (val >> 3) & 0x0001;
    restrictedTwtSupport = (val >> 4) & 0x0001;
    scsTrafficDescriptionSupport = (val >> 5) & 0x0001;
    maxMpduLength = (val >> 6) & 0x0003;
    maxAmpduLengthExponentExtension = (val >> 8) & 0x0001;
    return 2;
}

uint16_t
EhtPhyCapabilities::GetSize() const
{
    return 9;
}

void
EhtPhyCapabilities::Serialize(Buffer::Iterator& start) const
{
    uint64_t val1 =
        (support320MhzIn6Ghz << 1) | (support242ToneRuInBwLargerThan20Mhz << 2) |
        (ndpWith4TimesEhtLtfAnd32usGi << 3) | (partialBandwidthUlMuMimo << 4) |
        (suBeamformer << 5) | (suBeamformee << 6) | (beamformeeSsBwNotLargerThan80Mhz << 7) |
        (beamformeeSs160Mhz << 10) | (beamformeeSs320Mhz << 13) |
        (nSoundingDimensionsBwNotLargerThan80Mhz << 16) | (nSoundingDimensions160Mhz << 19) |
        (nSoundingDimensions320Mhz << 22) | (ng16SuFeedback << 25) | (ng16MuFeedback << 26) |
        (codebooksizeSuFeedback << 27) | (codebooksizeMuFeedback << 28) |
        (triggeredSuBeamformingFeedback << 29) | (triggeredMuBeamformingPartialBwFeedback << 30) |
        (triggeredCqiFeedback << 31) | (static_cast<uint64_t>(partialBandwidthDlMuMimo) << 32) |
        (static_cast<uint64_t>(psrBasedSpatialReuseSupport) << 33) |
        (static_cast<uint64_t>(powerBoostFactorSupport) << 34) |
        (static_cast<uint64_t>(muPpdu4xEhtLtfAnd800nsGi) << 35) |
        (static_cast<uint64_t>(maxNc) << 36) |
        (static_cast<uint64_t>(nonTriggeredCqiFeedback) << 40) |
        (static_cast<uint64_t>(supportTx1024And4096QamForRuSmallerThan242Tones) << 41) |
        (static_cast<uint64_t>(supportRx1024And4096QamForRuSmallerThan242Tones) << 42) |
        (static_cast<uint64_t>(ppeThresholdsPresent) << 43) |
        (static_cast<uint64_t>(commonNominalPacketPadding) << 44) |
        (static_cast<uint64_t>(maxNumSupportedEhtLtfs) << 46) |
        (static_cast<uint64_t>(supportMcs15) << 51) |
        (static_cast<uint64_t>(supportEhtDupIn6GHz) << 55) |
        (static_cast<uint64_t>(support20MhzOperatingStaReceivingNdpWithWiderBw) << 56) |
        (static_cast<uint64_t>(nonOfdmaUlMuMimoBwNotLargerThan80Mhz) << 57) |
        (static_cast<uint64_t>(nonOfdmaUlMuMimo160Mhz) << 58) |
        (static_cast<uint64_t>(nonOfdmaUlMuMimo320Mhz) << 59) |
        (static_cast<uint64_t>(muBeamformerBwNotLargerThan80Mhz) << 60) |
        (static_cast<uint64_t>(muBeamformer160Mhz) << 61) |
        (static_cast<uint64_t>(muBeamformer320Mhz) << 62) |
        (static_cast<uint64_t>(tbSoundingFeedbackRateLimit) << 63);
    uint8_t val2 = rx1024QamInWiderBwDlOfdmaSupport | (rx4096QamInWiderBwDlOfdmaSupport << 1);
    start.WriteHtolsbU64(val1);
    start.WriteU8(val2);
}

uint16_t
EhtPhyCapabilities::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    uint64_t val1 = i.ReadLsbtohU64();
    uint8_t val2 = i.ReadU8();

    support320MhzIn6Ghz = (val1 >> 1) & 0x0001;
    support242ToneRuInBwLargerThan20Mhz = (val1 >> 2) & 0x0001;
    ndpWith4TimesEhtLtfAnd32usGi = (val1 >> 3) & 0x0001;
    partialBandwidthUlMuMimo = (val1 >> 4) & 0x0001;
    suBeamformer = (val1 >> 5) & 0x0001;
    suBeamformee = (val1 >> 6) & 0x0001;
    beamformeeSsBwNotLargerThan80Mhz = (val1 >> 7) & 0x0007;
    beamformeeSs160Mhz = (val1 >> 10) & 0x0007;
    beamformeeSs320Mhz = (val1 >> 13) & 0x0007;
    nSoundingDimensionsBwNotLargerThan80Mhz = (val1 >> 16) & 0x0007;
    nSoundingDimensions160Mhz = (val1 >> 19) & 0x0007;
    nSoundingDimensions320Mhz = (val1 >> 22) & 0x0007;
    ng16SuFeedback = (val1 >> 25) & 0x0001;
    ng16MuFeedback = (val1 >> 26) & 0x0001;
    codebooksizeSuFeedback = (val1 >> 27) & 0x0001;
    codebooksizeMuFeedback = (val1 >> 28) & 0x0001;
    triggeredSuBeamformingFeedback = (val1 >> 29) & 0x0001;
    triggeredMuBeamformingPartialBwFeedback = (val1 >> 30) & 0x0001;
    triggeredCqiFeedback = (val1 >> 31) & 0x0001;
    partialBandwidthDlMuMimo = (val1 >> 32) & 0x0001;
    psrBasedSpatialReuseSupport = (val1 >> 33) & 0x0001;
    powerBoostFactorSupport = (val1 >> 34) & 0x0001;
    muPpdu4xEhtLtfAnd800nsGi = (val1 >> 35) & 0x0001;
    maxNc = (val1 >> 36) & 0x000f;
    nonTriggeredCqiFeedback = (val1 >> 40) & 0x0001;
    supportTx1024And4096QamForRuSmallerThan242Tones = (val1 >> 41) & 0x0001;
    supportRx1024And4096QamForRuSmallerThan242Tones = (val1 >> 42) & 0x0001;
    ppeThresholdsPresent = (val1 >> 43) & 0x0001;
    commonNominalPacketPadding = (val1 >> 44) & 0x0003;
    maxNumSupportedEhtLtfs = (val1 >> 46) & 0x001f;
    supportMcs15 = (val1 >> 51) & 0x000f;
    supportEhtDupIn6GHz = (val1 >> 55) & 0x0001;
    support20MhzOperatingStaReceivingNdpWithWiderBw = (val1 >> 56) & 0x0001;
    nonOfdmaUlMuMimoBwNotLargerThan80Mhz = (val1 >> 57) & 0x0001;
    nonOfdmaUlMuMimo160Mhz = (val1 >> 58) & 0x0001;
    nonOfdmaUlMuMimo320Mhz = (val1 >> 59) & 0x0001;
    muBeamformerBwNotLargerThan80Mhz = (val1 >> 60) & 0x0001;
    muBeamformer160Mhz = (val1 >> 61) & 0x0001;
    muBeamformer320Mhz = (val1 >> 62) & 0x0001;
    tbSoundingFeedbackRateLimit = (val1 >> 63) & 0x0001;

    rx1024QamInWiderBwDlOfdmaSupport = val2 & 0x01;
    rx4096QamInWiderBwDlOfdmaSupport = (val2 >> 1) & 0x01;

    return 9;
}

uint16_t
EhtMcsAndNssSet::GetSize() const
{
    if (supportedEhtMcsAndNssSet.empty())
    {
        return 0;
    }
    uint16_t size = 0;
    for (const auto& ehtMcsAndNssSet : supportedEhtMcsAndNssSet)
    {
        size += ehtMcsAndNssSet.second.size();
    }
    return size;
}

void
EhtMcsAndNssSet::Serialize(Buffer::Iterator& start) const
{
    NS_ASSERT(!supportedEhtMcsAndNssSet.empty());
    for (const auto& ehtMcsAndNssSet : supportedEhtMcsAndNssSet)
    {
        for (const auto& byte : ehtMcsAndNssSet.second)
        {
            start.WriteU8(byte);
        }
    }
}

uint16_t
EhtMcsAndNssSet::Deserialize(Buffer::Iterator start,
                             bool is2_4Ghz,
                             uint8_t heSupportedChannelWidthSet,
                             bool support320MhzIn6Ghz)
{
    Buffer::Iterator i = start;
    uint16_t count = 0;
    supportedEhtMcsAndNssSet.clear();
    std::vector<uint8_t> bytes;
    if (is2_4Ghz)
    {
        if ((heSupportedChannelWidthSet & 0x01) == 0)
        {
            bytes.clear();
            for (std::size_t j = 0; j < 4; ++j)
            {
                uint8_t byte = i.ReadU8();
                bytes.push_back(byte);
                count++;
            }
            supportedEhtMcsAndNssSet[EHT_MCS_MAP_TYPE_20_MHZ_ONLY] = bytes;
        }
        else
        {
            bytes.clear();
            for (std::size_t j = 0; j < 3; ++j)
            {
                uint8_t byte = i.ReadU8();
                bytes.push_back(byte);
                count++;
            }
            supportedEhtMcsAndNssSet[EHT_MCS_MAP_TYPE_NOT_LARGER_THAN_80_MHZ] = bytes;
        }
    }
    else
    {
        if ((heSupportedChannelWidthSet & 0x0e) == 0)
        {
            bytes.clear();
            for (std::size_t j = 0; j < 4; ++j)
            {
                uint8_t byte = i.ReadU8();
                bytes.push_back(byte);
                count++;
            }
            supportedEhtMcsAndNssSet[EHT_MCS_MAP_TYPE_20_MHZ_ONLY] = bytes;
        }
        if ((heSupportedChannelWidthSet & 0x02) != 0)
        {
            bytes.clear();
            for (std::size_t j = 0; j < 3; ++j)
            {
                uint8_t byte = i.ReadU8();
                bytes.push_back(byte);
                count++;
            }
            supportedEhtMcsAndNssSet[EHT_MCS_MAP_TYPE_NOT_LARGER_THAN_80_MHZ] = bytes;
        }
        if ((heSupportedChannelWidthSet & 0x04) != 0)
        {
            bytes.clear();
            for (std::size_t j = 0; j < 3; ++j)
            {
                uint8_t byte = i.ReadU8();
                bytes.push_back(byte);
                count++;
            }
            supportedEhtMcsAndNssSet[EHT_MCS_MAP_TYPE_160_MHZ] = bytes;
        }
        if (support320MhzIn6Ghz)
        {
            bytes.clear();
            for (std::size_t j = 0; j < 3; ++j)
            {
                uint8_t byte = i.ReadU8();
                bytes.push_back(byte);
                count++;
            }
            supportedEhtMcsAndNssSet[EHT_MCS_MAP_TYPE_320_MHZ] = bytes;
        }
    }
    return count;
}

uint16_t
EhtPpeThresholds::GetSize() const
{
    const auto numBitsSet = std::bitset<5>(ruIndexBitmask).count();
    const uint64_t nBitsNoPadding = 4 + 5 + (6 * numBitsSet * (nssPe + 1));
    return std::ceil(static_cast<double>(nBitsNoPadding) / 8.0);
}

void
EhtPpeThresholds::Serialize(Buffer::Iterator& start) const
{
    uint64_t nBitsNoPadding = 0;
    uint8_t val = nssPe | ((ruIndexBitmask & 0x0f) << 4);
    start.WriteU8(val);
    nBitsNoPadding += 8;
    val = (ruIndexBitmask & 0x10) >> 4;
    nBitsNoPadding += 1;
    uint8_t bitsPerPpet = 3;
    for (const auto& info : ppeThresholdsInfo)
    {
        uint8_t offset = nBitsNoPadding % 8;
        uint8_t bitsLeft = (8 - offset);
        uint8_t bitMask = (0x01 << bitsLeft) - 0x01;
        val |= (info.ppetMax & bitMask) << offset;
        nBitsNoPadding += std::min(bitsLeft, bitsPerPpet);
        if (nBitsNoPadding % 8 == 0)
        {
            start.WriteU8(val);
            if (bitsLeft < 3)
            {
                const uint8_t remainingBits = (3 - bitsLeft);
                bitMask = (0x01 << remainingBits) - 0x01;
                val = (info.ppetMax >> bitsLeft) & bitMask;
                nBitsNoPadding += remainingBits;
            }
            else
            {
                val = 0;
            }
        }
        offset = nBitsNoPadding % 8;
        bitsLeft = (8 - offset);
        bitMask = (0x01 << bitsLeft) - 0x01;
        val |= (info.ppet8 & bitMask) << offset;
        nBitsNoPadding += std::min(bitsLeft, bitsPerPpet);
        if (nBitsNoPadding % 8 == 0)
        {
            start.WriteU8(val);
            if (bitsLeft < 3)
            {
                const uint8_t remainingBits = (3 - bitsLeft);
                bitMask = (0x01 << remainingBits) - 0x01;
                val = (info.ppet8 >> bitsLeft) & bitMask;
                nBitsNoPadding += remainingBits;
            }
            else
            {
                val = 0;
            }
        }
    }
    if (nBitsNoPadding % 8 > 0)
    {
        start.WriteU8(val);
    }
}

uint16_t
EhtPpeThresholds::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    uint64_t nBitsNoPadding = 0;
    uint8_t val = i.ReadU8();
    nssPe = val & 0x0f;
    ruIndexBitmask = ((val >> 4) & 0x0f);
    nBitsNoPadding += 8;
    val = i.ReadU8();
    ruIndexBitmask |= ((val & 0x01) << 4);
    nBitsNoPadding += 1;
    const auto numBitsSet = std::bitset<5>(ruIndexBitmask).count();
    const uint64_t bitsToDeserialize = (4 + 5 + (6 * numBitsSet * (nssPe + 1)));
    uint8_t bitsPerPpet = 3;
    while (nBitsNoPadding < bitsToDeserialize)
    {
        EhtPpeThresholdsInfo info;
        uint8_t offset = nBitsNoPadding % 8;
        uint8_t bitsLeft = (8 - offset);
        uint8_t bitMask = (1 << bitsLeft) - 1;
        info.ppetMax = ((val >> offset) & bitMask);
        nBitsNoPadding += std::min(bitsLeft, bitsPerPpet);
        if (nBitsNoPadding % 8 == 0)
        {
            val = i.ReadU8();
            if (bitsLeft < 3)
            {
                const uint8_t remainingBits = (3 - bitsLeft);
                bitMask = (1 << remainingBits) - 1;
                info.ppetMax |= ((val & bitMask) << bitsLeft);
                nBitsNoPadding += remainingBits;
            }
        }
        offset = nBitsNoPadding % 8;
        bitsLeft = (8 - offset);
        bitMask = (1 << bitsLeft) - 1;
        info.ppet8 = ((val >> offset) & bitMask);
        nBitsNoPadding += std::min(bitsLeft, bitsPerPpet);
        if (nBitsNoPadding % 8 == 0)
        {
            val = i.ReadU8();
            if (bitsLeft < 3)
            {
                const uint8_t remainingBits = (3 - bitsLeft);
                bitMask = (1 << remainingBits) - 1;
                info.ppet8 |= ((val & bitMask) << bitsLeft);
                nBitsNoPadding += remainingBits;
            }
        }
        ppeThresholdsInfo.push_back(info);
    }
    return std::ceil(static_cast<double>(bitsToDeserialize) / 8.0);
}

EhtCapabilities::EhtCapabilities()
    : m_macCapabilities{},
      m_phyCapabilities{},
      m_supportedEhtMcsAndNssSet{},
      m_ppeThresholds{},
      m_is2_4Ghz{false},
      m_heCapabilities{std::nullopt}
{
}

EhtCapabilities::EhtCapabilities(bool is2_4Ghz, const std::optional<HeCapabilities>& heCapabilities)
    : m_macCapabilities{},
      m_phyCapabilities{},
      m_supportedEhtMcsAndNssSet{},
      m_ppeThresholds{},
      m_is2_4Ghz{is2_4Ghz},
      m_heCapabilities{heCapabilities}
{
}

WifiInformationElementId
EhtCapabilities::ElementId() const
{
    return IE_EXTENSION;
}

WifiInformationElementId
EhtCapabilities::ElementIdExt() const
{
    return IE_EXT_EHT_CAPABILITIES;
}

void
EhtCapabilities::Print(std::ostream& os) const
{
    os << "EHT Capabilities="; // TODO
}

uint16_t
EhtCapabilities::GetInformationFieldSize() const
{
    uint16_t size = 1 + // ElementIdExt
                    m_macCapabilities.GetSize() + m_phyCapabilities.GetSize() +
                    m_supportedEhtMcsAndNssSet.GetSize();
    if (m_phyCapabilities.ppeThresholdsPresent)
    {
        size += m_ppeThresholds.GetSize();
    }
    return size;
}

void
EhtCapabilities::SetMaxMpduLength(uint16_t length)
{
    switch (length)
    {
    case 3895:
        m_macCapabilities.maxMpduLength = 0;
        break;
    case 7991:
        m_macCapabilities.maxMpduLength = 1;
        break;
    case 11454:
        m_macCapabilities.maxMpduLength = 2;
        break;
    default:
        NS_ABORT_MSG("Invalid MPDU Max Length value");
    }
}

uint16_t
EhtCapabilities::GetMaxMpduLength() const
{
    switch (m_macCapabilities.maxMpduLength)
    {
    case 0:
        return 3895;
    case 1:
        return 7991;
    case 2:
        return 11454;
    default:
        NS_ABORT_MSG("The value 3 is reserved");
    }
    return 0;
}

void
EhtCapabilities::SetMaxAmpduLength(uint32_t maxAmpduLength)
{
    for (uint8_t i = 0; i <= 1; i++)
    {
        if ((1UL << (23 + i)) - 1 == maxAmpduLength)
        {
            m_macCapabilities.maxAmpduLengthExponentExtension = i;
            return;
        }
    }
    NS_ABORT_MSG("Invalid A-MPDU Max Length value");
}

uint32_t
EhtCapabilities::GetMaxAmpduLength() const
{
    return std::min<uint32_t>((1UL << (23 + m_macCapabilities.maxAmpduLengthExponentExtension)) - 1,
                              15523200);
}

void
EhtCapabilities::SetSupportedRxEhtMcsAndNss(EhtMcsAndNssSet::EhtMcsMapType mapType,
                                            uint8_t upperMcs,
                                            uint8_t maxNss)
{
    NS_ASSERT_MSG(maxNss <= 8, "Invalid maximum NSS " << maxNss);
    std::size_t index = 0;
    switch (upperMcs)
    {
    case 7:
        NS_ASSERT(mapType == EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY);
        index = 0;
        break;
    case 9:
        index = (mapType == EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY) ? 1 : 0;
        break;
    case 11:
        index = (mapType == EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY) ? 2 : 1;
        break;
    case 13:
        index = (mapType == EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY) ? 3 : 2;
        break;
    default:
        NS_ASSERT_MSG(false, "Invalid upper MCS " << +upperMcs);
    }
    uint8_t nBytes = 0;
    switch (mapType)
    {
    case EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY:
        nBytes = 4;
        break;
    case EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_NOT_LARGER_THAN_80_MHZ:
    case EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_160_MHZ:
    case EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_320_MHZ:
        nBytes = 3;
        break;
    default:
        NS_ASSERT_MSG(false, "Invalid map type " << mapType);
    }
    auto it = m_supportedEhtMcsAndNssSet.supportedEhtMcsAndNssSet.find(mapType);
    if (it == m_supportedEhtMcsAndNssSet.supportedEhtMcsAndNssSet.end())
    {
        m_supportedEhtMcsAndNssSet.supportedEhtMcsAndNssSet[mapType].resize(nBytes);
        m_supportedEhtMcsAndNssSet.supportedEhtMcsAndNssSet[mapType][index] = (maxNss & 0x0f);
    }
    else
    {
        NS_ASSERT(it->second.size() == nBytes);
        it->second[index] |= (maxNss & 0x0f);
    }
}

void
EhtCapabilities::SetSupportedTxEhtMcsAndNss(EhtMcsAndNssSet::EhtMcsMapType mapType,
                                            uint8_t upperMcs,
                                            uint8_t maxNss)
{
    NS_ASSERT_MSG(maxNss <= 8, "Invalid maximum NSS " << maxNss);
    std::size_t index = 0;
    switch (upperMcs)
    {
    case 7:
        NS_ASSERT(mapType == EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY);
        index = 0;
        break;
    case 9:
        index = (mapType == EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY) ? 1 : 0;
        break;
    case 11:
        index = (mapType == EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY) ? 2 : 1;
        break;
    case 13:
        index = (mapType == EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY) ? 3 : 2;
        break;
    default:
        NS_ASSERT_MSG(false, "Invalid upper MCS " << upperMcs);
    }
    uint8_t nBytes = 0;
    switch (mapType)
    {
    case EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY:
        nBytes = 4;
        break;
    case EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_NOT_LARGER_THAN_80_MHZ:
    case EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_160_MHZ:
    case EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_320_MHZ:
        nBytes = 3;
        break;
    default:
        NS_ASSERT_MSG(false, "Invalid map type " << mapType);
    }
    auto it = m_supportedEhtMcsAndNssSet.supportedEhtMcsAndNssSet.find(mapType);
    if (it == m_supportedEhtMcsAndNssSet.supportedEhtMcsAndNssSet.end())
    {
        m_supportedEhtMcsAndNssSet.supportedEhtMcsAndNssSet[mapType].resize(nBytes);
        m_supportedEhtMcsAndNssSet.supportedEhtMcsAndNssSet[mapType][index] =
            ((maxNss & 0x0f) << 4);
    }
    else
    {
        NS_ASSERT(it->second.size() == nBytes);
        it->second[index] |= ((maxNss & 0x0f) << 4);
    }
}

uint8_t
EhtCapabilities::GetHighestSupportedRxMcs(EhtMcsAndNssSet::EhtMcsMapType mapType) const
{
    const auto it = m_supportedEhtMcsAndNssSet.supportedEhtMcsAndNssSet.find(mapType);
    if (it == m_supportedEhtMcsAndNssSet.supportedEhtMcsAndNssSet.cend())
    {
        return 0;
    }
    int8_t index = -1;
    for (int8_t i = (mapType == EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY) ? 3 : 2; i >= 0; i--)
    {
        if ((it->second[i] & 0x0f) > 0)
        {
            index = i;
            break;
        }
    }
    NS_ASSERT_MSG(index >= 0, "Supported EHT-MCS And NSS Set subfield is incorrect");
    switch (index)
    {
    case 0:
        return (mapType == EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY) ? 7 : 9;
    case 1:
        return (mapType == EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY) ? 9 : 11;
    case 2:
        return (mapType == EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY) ? 11 : 13;
    case 3:
        NS_ASSERT(mapType == EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY);
        return 13;
    default:
        NS_ASSERT(false);
    }
    return 0;
}

uint8_t
EhtCapabilities::GetHighestSupportedTxMcs(EhtMcsAndNssSet::EhtMcsMapType mapType) const
{
    const auto it = m_supportedEhtMcsAndNssSet.supportedEhtMcsAndNssSet.find(mapType);
    if (it == m_supportedEhtMcsAndNssSet.supportedEhtMcsAndNssSet.cend())
    {
        return 0;
    }
    int8_t index = -1;
    for (int8_t i = (mapType == EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY) ? 3 : 2; i >= 0; i--)
    {
        if ((it->second[i] & 0xf0) > 0)
        {
            index = i;
            break;
        }
    }
    NS_ASSERT_MSG(index >= 0, "Supported EHT-MCS And NSS Set subfield is incorrect");
    switch (index)
    {
    case 0:
        return (mapType == EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY) ? 7 : 9;
    case 1:
        return (mapType == EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY) ? 9 : 11;
    case 2:
        return (mapType == EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY) ? 11 : 13;
    case 3:
        NS_ASSERT(mapType == EhtMcsAndNssSet::EHT_MCS_MAP_TYPE_20_MHZ_ONLY);
        return 13;
    default:
        NS_ASSERT(false);
    }
    return 0;
}

void
EhtCapabilities::SetPpeThresholds(uint8_t nssPe,
                                  uint8_t ruIndexBitmask,
                                  const std::vector<std::pair<uint8_t, uint8_t>>& ppeThresholds)
{
    NS_ASSERT(ppeThresholds.size() == (std::bitset<5>(ruIndexBitmask).count() * (nssPe + 1)));
    m_phyCapabilities.ppeThresholdsPresent = 1;
    m_ppeThresholds.nssPe = nssPe;
    m_ppeThresholds.ruIndexBitmask = ruIndexBitmask;
    m_ppeThresholds.ppeThresholdsInfo.clear();
    for (const auto& [ppetMax, ppet8] : ppeThresholds)
    {
        m_ppeThresholds.ppeThresholdsInfo.push_back({ppetMax, ppet8});
    }
}

void
EhtCapabilities::SerializeInformationField(Buffer::Iterator start) const
{
    m_macCapabilities.Serialize(start);
    m_phyCapabilities.Serialize(start);
    m_supportedEhtMcsAndNssSet.Serialize(start);
    if (m_phyCapabilities.ppeThresholdsPresent)
    {
        m_ppeThresholds.Serialize(start);
    }
}

uint16_t
EhtCapabilities::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    uint16_t count = 0;
    Buffer::Iterator i = start;

    uint16_t nBytes = m_macCapabilities.Deserialize(i);
    i.Next(nBytes);
    count += nBytes;

    nBytes = m_phyCapabilities.Deserialize(i);
    i.Next(nBytes);
    count += nBytes;

    NS_ASSERT(m_heCapabilities.has_value());
    nBytes = m_supportedEhtMcsAndNssSet.Deserialize(i,
                                                    m_is2_4Ghz,
                                                    m_heCapabilities->GetChannelWidthSet(),
                                                    m_phyCapabilities.support320MhzIn6Ghz);
    i.Next(nBytes);
    count += nBytes;

    if (m_phyCapabilities.ppeThresholdsPresent)
    {
        count += m_ppeThresholds.Deserialize(i);
    }

    return count;
}

} // namespace ns3
