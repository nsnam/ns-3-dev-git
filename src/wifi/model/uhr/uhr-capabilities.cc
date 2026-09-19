/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "uhr-capabilities.h"

namespace ns3
{

void
UhrCapabilities::UhrMacCapabilities::SetOmpUpdateTimeout(const Time& timeout)
{
    // IEEE 802.11bn/D2.0 Table 9-bb3 (Encoding of the UHR Operating Mode And Parameters Update
    // Timeout field): 0, or 2^(6+n) us with n=1..11 (i.e. 128 us .. 128 TUs)
    const auto timeoutUs = timeout.GetMicroSeconds();

    if (timeoutUs == 0)
    {
        uhrOpModeAndParamsUpdateTimeout = 0;
        return;
    }

    for (uint8_t i = 1; i <= 11; ++i)
    {
        if ((1 << (i + 6)) == timeoutUs)
        {
            uhrOpModeAndParamsUpdateTimeout = i;
            return;
        }
    }
    NS_ABORT_MSG("OMP timeout value not allowed (" << timeout.As(Time::US) << ")");
}

Time
UhrCapabilities::UhrMacCapabilities::GetOmpUpdateTimeout() const
{
    // IEEE 802.11bn/D2.0 Table 9-bb3 (Encoding of the UHR Operating Mode And Parameters Update
    // Timeout field)
    NS_ABORT_MSG_IF(uhrOpModeAndParamsUpdateTimeout > 11,
                    "Reserved UHR Operating Mode And Parameters Update Timeout value ("
                        << +uhrOpModeAndParamsUpdateTimeout << ")");
    if (uhrOpModeAndParamsUpdateTimeout == 0)
    {
        return MicroSeconds(0);
    }
    return MicroSeconds(1 << (6 + uhrOpModeAndParamsUpdateTimeout));
}

uint16_t
UhrCapabilities::UhrMacCapabilities::GetSize() const
{
    return WIFI_UHR_MAC_CAPABILITIES_SIZE_B;
}

void
UhrCapabilities::UhrMacCapabilities::Serialize(Buffer::Iterator& start) const
{
    uint64_t val =
        dpsSupport | (dpsAssistingSupport << 1) | (dpsApStaticHcmSupport << 2) |
        (npcaSupport << 4) | (enhancedBsrSupport << 5) | (additionalMappedTidSupport << 6) |
        (eotspSupport << 7) | (static_cast<uint64_t>(dsoSupport) << 8) |
        (static_cast<uint64_t>(pEdcaSupport) << 9) | (static_cast<uint64_t>(dbeSupport) << 10) |
        (static_cast<uint64_t>(ulLliSupport) << 11) | (static_cast<uint64_t>(p2pLliSupport) << 12) |
        (static_cast<uint64_t>(puoSupport) << 13) | (static_cast<uint64_t>(apPuoSupport) << 14) |
        (static_cast<uint64_t>(duoSupport) << 15) |
        (static_cast<uint64_t>(omCtrlUlMuDataDisableRxSupport) << 16) |
        (static_cast<uint64_t>(aomSupport) << 17) |
        (static_cast<uint64_t>(ifcsLocationSupport) << 18) |
        (static_cast<uint64_t>(uhrTrsSupport) << 19) | (static_cast<uint64_t>(txspgSupport) << 20) |
        (static_cast<uint64_t>(txspgTxopReturnSupport) << 21) |
        (static_cast<uint64_t>(uhrOpModeAndParamsUpdateTimeout) << 22) |
        (static_cast<uint64_t>(paramUpdateAdvNotificationInterval) << 26) |
        (static_cast<uint64_t>(updateIndicationTimIntervals) << 31) |
        (static_cast<uint64_t>(boundedEss) << 36) | (static_cast<uint64_t>(btmAssurance) << 37) |
        (static_cast<uint64_t>(coBfSupport) << 38) | (static_cast<uint64_t>(coSrSupport) << 39) |
        (static_cast<uint64_t>(mapcEnhancedMeasurementSupport) << 40) |
        (static_cast<uint64_t>(hgRtwtSupport) << 41) |
        (static_cast<uint64_t>(extendedChannelUsageSupport) << 42);
    start.WriteU32(val & 0xffffffff);
    start.WriteU16((val >> 32) & 0xffff);
}

uint16_t
UhrCapabilities::UhrMacCapabilities::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    uint64_t val = i.ReadU32();
    val |= static_cast<uint64_t>(i.ReadU16()) << 32;
    dpsSupport = val & 0x01;
    dpsAssistingSupport = (val >> 1) & 0x01;
    dpsApStaticHcmSupport = (val >> 2) & 0x01;
    npcaSupport = (val >> 4) & 0x01;
    enhancedBsrSupport = (val >> 5) & 0x01;
    additionalMappedTidSupport = (val >> 6) & 0x01;
    eotspSupport = (val >> 7) & 0x01;
    dsoSupport = (val >> 8) & 0x01;
    pEdcaSupport = (val >> 9) & 0x01;
    dbeSupport = (val >> 10) & 0x01;
    ulLliSupport = (val >> 11) & 0x01;
    p2pLliSupport = (val >> 12) & 0x01;
    puoSupport = (val >> 13) & 0x01;
    apPuoSupport = (val >> 14) & 0x01;
    duoSupport = (val >> 15) & 0x01;
    omCtrlUlMuDataDisableRxSupport = (val >> 16) & 0x01;
    aomSupport = (val >> 17) & 0x01;
    ifcsLocationSupport = (val >> 18) & 0x01;
    uhrTrsSupport = (val >> 19) & 0x01;
    txspgSupport = (val >> 20) & 0x01;
    txspgTxopReturnSupport = (val >> 21) & 0x01;
    uhrOpModeAndParamsUpdateTimeout = (val >> 22) & 0x0f;
    paramUpdateAdvNotificationInterval = (val >> 26) & 0x1f;
    updateIndicationTimIntervals = (val >> 31) & 0x1f;
    boundedEss = (val >> 36) & 0x01;
    btmAssurance = (val >> 37) & 0x01;
    coBfSupport = (val >> 38) & 0x01;
    coSrSupport = (val >> 39) & 0x01;
    mapcEnhancedMeasurementSupport = (val >> 40) & 0x01;
    hgRtwtSupport = (val >> 41) & 0x01;
    extendedChannelUsageSupport = (val >> 42) & 0x01;
    return i.GetDistanceFrom(start);
}

uint16_t
UhrCapabilities::UhrPhyCapabilities::GetSize() const
{
    return WIFI_UHR_PHY_CAPABILITIES_SIZE_B;
}

void
UhrCapabilities::UhrPhyCapabilities::Serialize(Buffer::Iterator& start) const
{
    uint32_t val = maxNssRxForSoundingNdpLe80Mhz | (maxNssTotalRxForDlMuMimoLe80Mhz << 1) |
                   (maxNssRxForSoundingNdp160Mhz << 2) | (maxNssTotalRxForDlMuMimo160Mhz << 3) |
                   (maxNssRxForSoundingNdp320Mhz << 4) | (maxNssTotalRxForDlMuMimo320Mhz << 5) |
                   (elrTxSupport << 6) | (elrRxSupport << 7) | (partialBwDlMuMimoSupport << 8) |
                   (partialBwUlMuMimoSupport << 9) | (mcs15Support << 10) |
                   (ldpc2xTxSupport << 11) | (ldpc2xRxSupport << 12) |
                   (ueqmTxSupportAndMaxNssTx << 13) | (ueqmRxSupportAndMaxNssRx << 15) |
                   (coBfJointSoundingSupport << 18) | (imTxSupport << 19) | (imRxSupport << 20) |
                   (coSrMode1Support << 21) | (coSrMode2Support << 22) |
                   (druDbw20InPbw20MhzSupport << 23) | (druDbw40InPbw40MhzSupport << 24) |
                   (druDbw80InPbw80MhzSupport << 25) | (druDbw80InPbw160MhzSupport << 26) |
                   (druDbw80InPbw320MhzSupport << 27) | (druDbw20InPbwGe80MhzSupport << 28) |
                   (druDbw40InPbwGe80MhzSupport << 29) | (druDbw60InPbwGe80MhzSupport << 30) |
                   (druRruHybridModeSupport << 31);
    start.WriteU32(val);
    start.WriteU8(0); // B32-B39 Reserved
}

uint16_t
UhrCapabilities::UhrPhyCapabilities::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    uint32_t val = i.ReadU32();
    i.ReadU8(); // B32-B39 Reserved
    maxNssRxForSoundingNdpLe80Mhz = val & 0x01;
    maxNssTotalRxForDlMuMimoLe80Mhz = (val >> 1) & 0x01;
    maxNssRxForSoundingNdp160Mhz = (val >> 2) & 0x01;
    maxNssTotalRxForDlMuMimo160Mhz = (val >> 3) & 0x01;
    maxNssRxForSoundingNdp320Mhz = (val >> 4) & 0x01;
    maxNssTotalRxForDlMuMimo320Mhz = (val >> 5) & 0x01;
    elrTxSupport = (val >> 6) & 0x01;
    elrRxSupport = (val >> 7) & 0x01;
    partialBwDlMuMimoSupport = (val >> 8) & 0x01;
    partialBwUlMuMimoSupport = (val >> 9) & 0x01;
    mcs15Support = (val >> 10) & 0x01;
    ldpc2xTxSupport = (val >> 11) & 0x01;
    ldpc2xRxSupport = (val >> 12) & 0x01;
    ueqmTxSupportAndMaxNssTx = (val >> 13) & 0x03;
    ueqmRxSupportAndMaxNssRx = (val >> 15) & 0x03;
    coBfJointSoundingSupport = (val >> 18) & 0x01;
    imTxSupport = (val >> 19) & 0x01;
    imRxSupport = (val >> 20) & 0x01;
    coSrMode1Support = (val >> 21) & 0x01;
    coSrMode2Support = (val >> 22) & 0x01;
    druDbw20InPbw20MhzSupport = (val >> 23) & 0x01;
    druDbw40InPbw40MhzSupport = (val >> 24) & 0x01;
    druDbw80InPbw80MhzSupport = (val >> 25) & 0x01;
    druDbw80InPbw160MhzSupport = (val >> 26) & 0x01;
    druDbw80InPbw320MhzSupport = (val >> 27) & 0x01;
    druDbw20InPbwGe80MhzSupport = (val >> 28) & 0x01;
    druDbw40InPbwGe80MhzSupport = (val >> 29) & 0x01;
    druDbw60InPbwGe80MhzSupport = (val >> 30) & 0x01;
    druRruHybridModeSupport = (val >> 31) & 0x01;
    return i.GetDistanceFrom(start);
}

uint16_t
UhrCapabilities::DbeBandwidthCapabilities::GetSize() const
{
    return WIFI_UHR_DBE_BANDWIDTH_CAPABILITIES_SIZE_B;
}

void
UhrCapabilities::DbeBandwidthCapabilities::Serialize(Buffer::Iterator& start) const
{
    uint32_t val = ehtMcsMap[0] | (ehtMcsMap[1] << 8) | (ehtMcsMap[2] << 16) |
                   (nSoundingDimensions << 24) | (nonOfdmaUlMuMimo << 27) | (muBeamformer << 28) |
                   (beamformeeSs << 29);
    start.WriteU32(val);
}

uint16_t
UhrCapabilities::DbeBandwidthCapabilities::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    uint32_t val = i.ReadU32();
    ehtMcsMap[0] = val & 0xff;
    ehtMcsMap[1] = (val >> 8) & 0xff;
    ehtMcsMap[2] = (val >> 16) & 0xff;
    nSoundingDimensions = (val >> 24) & 0x07;
    nonOfdmaUlMuMimo = (val >> 27) & 0x01;
    muBeamformer = (val >> 28) & 0x01;
    beamformeeSs = (val >> 29) & 0x07;
    return i.GetDistanceFrom(start);
}

uint16_t
UhrCapabilities::DbeCapabilityParameters::GetSize() const
{
    uint16_t size = WIFI_UHR_DBE_CAPABILITY_PARAMETERS_HEADER_SIZE_B;
    if (dbeCapabilities160MhzPresent)
    {
        NS_ASSERT(dbeCapabilities160Mhz.has_value());
        size += dbeCapabilities160Mhz->GetSize();
    }
    if (dbeCapabilities320MhzPresent)
    {
        NS_ASSERT(dbeCapabilities320Mhz.has_value());
        size += dbeCapabilities320Mhz->GetSize();
    }
    return size;
}

void
UhrCapabilities::DbeCapabilityParameters::Serialize(Buffer::Iterator& start) const
{
    uint16_t val = maxSupportedDbeBandwidth | (dbeCapabilities160MhzPresent << 3) |
                   (dbeCapabilities320MhzPresent << 4) | (maxDbeBandwidthSwitchTimePeriod << 8);
    start.WriteU16(val);
    start.WriteU8(dbeModeChangeInterval);
    if (dbeCapabilities160MhzPresent)
    {
        NS_ASSERT(dbeCapabilities160Mhz.has_value());
        dbeCapabilities160Mhz->Serialize(start);
    }
    if (dbeCapabilities320MhzPresent)
    {
        NS_ASSERT(dbeCapabilities320Mhz.has_value());
        dbeCapabilities320Mhz->Serialize(start);
    }
}

uint16_t
UhrCapabilities::DbeCapabilityParameters::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    uint16_t val = i.ReadU16();
    maxSupportedDbeBandwidth = val & 0x07;
    dbeCapabilities160MhzPresent = (val >> 3) & 0x01;
    dbeCapabilities320MhzPresent = (val >> 4) & 0x01;
    maxDbeBandwidthSwitchTimePeriod = (val >> 8) & 0xff;
    dbeModeChangeInterval = i.ReadU8();
    dbeCapabilities160Mhz.reset();
    dbeCapabilities320Mhz.reset();
    if (dbeCapabilities160MhzPresent)
    {
        dbeCapabilities160Mhz = DbeBandwidthCapabilities();
        i.Next(dbeCapabilities160Mhz->Deserialize(i));
    }
    if (dbeCapabilities320MhzPresent)
    {
        dbeCapabilities320Mhz = DbeBandwidthCapabilities();
        i.Next(dbeCapabilities320Mhz->Deserialize(i));
    }
    return i.GetDistanceFrom(start);
}

WifiInformationElementId
UhrCapabilities::ElementId() const
{
    return IE_EXTENSION;
}

WifiInformationElementId
UhrCapabilities::ElementIdExt() const
{
    return IE_EXT_UHR_CAPABILITIES;
}

void
UhrCapabilities::Print(std::ostream& os) const
{
    os << "UHR Capabilities=[DSO Support=" << static_cast<bool>(m_macCapabilities.dsoSupport)
       << ", OMP Update Timeout=" << m_macCapabilities.GetOmpUpdateTimeout() << "]";
}

uint16_t
UhrCapabilities::GetInformationFieldSize() const
{
    uint16_t size = 1 + // ElementIdExt
                    m_macCapabilities.GetSize() + m_phyCapabilities.GetSize();
    if (m_dbeCapabilityParams)
    {
        size += m_dbeCapabilityParams->GetSize();
    }
    return size;
}

void
UhrCapabilities::SerializeInformationField(Buffer::Iterator start) const
{
    m_macCapabilities.Serialize(start);
    m_phyCapabilities.Serialize(start);
    if (m_dbeCapabilityParams)
    {
        NS_ASSERT(m_macCapabilities.dbeSupport);
        m_dbeCapabilityParams->Serialize(start);
    }
}

uint16_t
UhrCapabilities::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    uint16_t count = 0;
    Buffer::Iterator i = start;

    uint16_t nBytes = m_macCapabilities.Deserialize(i);
    i.Next(nBytes);
    count += nBytes;

    nBytes = m_phyCapabilities.Deserialize(i);
    i.Next(nBytes);
    count += nBytes;

    m_dbeCapabilityParams.reset();
    if (count < length)
    {
        m_dbeCapabilityParams = DbeCapabilityParameters();
        nBytes = m_dbeCapabilityParams->Deserialize(i);
        count += nBytes;
    }

    NS_ASSERT(count == length);
    return count;
}

} // namespace ns3
