/*
 * Copyright (c) 2021 DERONNE SOFTWARE ENGINEERING
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "eht-capabilities.h"

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

EhtCapabilities::EhtCapabilities()
    : m_macCapabilities{},
      m_phyCapabilities{},
      m_is2_4Ghz{false},
      m_heCapabilities{std::nullopt}
{
}

EhtCapabilities::EhtCapabilities(bool is2_4Ghz, const std::optional<HeCapabilities>& heCapabilities)
    : m_macCapabilities{},
      m_phyCapabilities{},
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

uint16_t
EhtCapabilities::GetInformationFieldSize() const
{
    uint16_t size = 1 + // ElementIdExt
                    m_macCapabilities.GetSize() + m_phyCapabilities.GetSize();
    // FIXME: currently only EHT MAC and PHY Capabilities Information
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
EhtCapabilities::SerializeInformationField(Buffer::Iterator start) const
{
    m_macCapabilities.Serialize(start);
    m_phyCapabilities.Serialize(start);
}

uint16_t
EhtCapabilities::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    uint16_t count = 0;
    Buffer::Iterator i = start;

    uint16_t nBytes = m_macCapabilities.Deserialize(i);
    i.Next(nBytes);
    count += nBytes;

    count += m_phyCapabilities.Deserialize(i);
    return count;
}

std::ostream&
operator<<(std::ostream& os, const EhtCapabilities& ehtCapabilities)
{
    // TODO
    return os;
}

} // namespace ns3
