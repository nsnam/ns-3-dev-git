/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "uhr-operation.h"

#include "ns3/assert.h"

#include <algorithm>

namespace
{
/**
 * set the max Tx/Rx NSS for input MCS index range
 * @param vec vector of max NSS per MCS
 * @param maxNss max NSS for input MCS range
 * @param mcsStart MCS index start
 * @param mcsEnd MCS index end
 */
void
SetMaxNss(std::vector<uint8_t>& vec, uint8_t maxNss, uint8_t mcsStart, uint8_t mcsEnd)
{
    NS_ASSERT(mcsStart <= mcsEnd);
    NS_ASSERT((mcsStart >= 0) && (mcsEnd <= ns3::WIFI_UHR_MAX_MCS_INDEX));
    NS_ASSERT((maxNss >= 1) && (maxNss <= ns3::WIFI_UHR_MAX_NSS_CONFIGURABLE));
    for (auto index = mcsStart; index <= mcsEnd; index++)
    {
        vec[index] = maxNss;
    }
}

/**
 * Get the max Tx/Rx NSS for input MCS index range
 * @param vec vector of max NSS per MCS
 * @param mcsStart MCS index start
 * @param mcsEnd MCS index end
 * @return max Rx NSS
 */
uint32_t
GetMaxNss(const std::vector<uint8_t>& vec, uint8_t mcsStart, uint8_t mcsEnd)
{
    NS_ASSERT(mcsStart <= mcsEnd);
    NS_ASSERT((mcsStart >= 0) && (mcsEnd <= ns3::WIFI_UHR_MAX_MCS_INDEX));
    auto minMaxNss = ns3::WIFI_UHR_MAX_NSS_CONFIGURABLE;
    for (auto index = mcsStart; index <= mcsEnd; index++)
    {
        if (vec[index] < minMaxNss)
        {
            minMaxNss = vec[index];
        }
    }
    return minMaxNss;
}
} // namespace

namespace ns3
{

void
UhrOperation::Print(std::ostream& os) const
{
    os << "UHR Operation=[]"; // TODO
}

void
UhrOperation::UhrOpParams::Serialize(Buffer::Iterator& start) const
{
    uint16_t val = (dpsEnabled ? 1 : 0) | ((npcaEnabled ? 1 : 0) << 1) |
                   ((dbeEnabled ? 1 : 0) << 2) | ((pEdcaEnabled ? 1 : 0) << 3) |
                   ((duoOpParamsPresent ? 1 : 0) << 8);
    start.WriteU16(val);
}

uint16_t
UhrOperation::UhrOpParams::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    auto params = start.ReadU16();
    dpsEnabled = (params & 0x01) == 1;
    npcaEnabled = ((params >> 1) & 0x01) == 1;
    dbeEnabled = ((params >> 2) & 0x01) == 1;
    pEdcaEnabled = ((params >> 3) & 0x01) == 1;
    duoOpParamsPresent = ((params >> 8) & 0x01) == 1;
    return start.GetDistanceFrom(i);
}

uint16_t
UhrOperation::UhrOpParams::GetSerializedSize() const
{
    return WIFI_UHR_OP_PARAMS_SIZE_B;
}

void
UhrOperation::UhrBasicMcsNssSet::Serialize(Buffer::Iterator& start) const
{
    uint32_t val = GetMaxNss(maxRxNss, 0, 7) | (GetMaxNss(maxTxNss, 0, 7) << 4) |
                   (GetMaxNss(maxRxNss, 8, 9) << 8) | (GetMaxNss(maxTxNss, 8, 9) << 12) |
                   (GetMaxNss(maxRxNss, 10, 11) << 16) | (GetMaxNss(maxTxNss, 10, 11) << 20) |
                   (GetMaxNss(maxRxNss, 12, 13) << 24) | (GetMaxNss(maxTxNss, 12, 13) << 28);
    start.WriteU32(val);
}

uint16_t
UhrOperation::UhrBasicMcsNssSet::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    auto subfield = start.ReadU32();
    auto rxNssMcs0_7 = subfield & 0xf; // Max Rx NSS MCS 0-7
    SetMaxNss(maxRxNss, rxNssMcs0_7, 0, 7);
    auto txNssMcs0_7 = (subfield >> 4) & 0xf; // Max Tx NSS MCS 0-7
    SetMaxNss(maxTxNss, txNssMcs0_7, 0, 7);
    auto rxNssMcs8_9 = (subfield >> 8) & 0xf; // Max Rx NSS MCS 8-9
    SetMaxNss(maxRxNss, rxNssMcs8_9, 8, 9);
    auto txNssMcs8_9 = (subfield >> 12) & 0xf; // Max Tx NSS MCS 8-9
    SetMaxNss(maxTxNss, txNssMcs8_9, 8, 9);
    auto rxNssMcs10_11 = (subfield >> 16) & 0xf; // Max Rx NSS MCS 10-11
    SetMaxNss(maxRxNss, rxNssMcs10_11, 10, 11);
    auto txNssMcs10_11 = (subfield >> 20) & 0xf; // Max Tx NSS MCS 10-11
    SetMaxNss(maxTxNss, txNssMcs10_11, 10, 11);
    auto rxNssMcs12_13 = (subfield >> 24) & 0xf; // Max Rx NSS MCS 12-13
    SetMaxNss(maxRxNss, rxNssMcs12_13, 12, 13);
    auto txNssMcs12_13 = (subfield >> 28) & 0xf; // Max Tx NSS MCS 12-13
    SetMaxNss(maxTxNss, txNssMcs12_13, 12, 13);
    return start.GetDistanceFrom(i);
}

uint16_t
UhrOperation::UhrBasicMcsNssSet::GetSerializedSize() const
{
    return WIFI_UHR_BASIC_MCS_NSS_SET_SIZE_B;
}

void
UhrOperation::DuoOpParams::Serialize(Buffer::Iterator& start) const
{
    start.WriteU8(maxStandaloneDuoBsrp & 0x1f);
}

uint16_t
UhrOperation::DuoOpParams::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    maxStandaloneDuoBsrp = start.ReadU8() & 0x1f;
    return start.GetDistanceFrom(i);
}

uint16_t
UhrOperation::DuoOpParams::GetSerializedSize() const
{
    return WIFI_UHR_DUO_PARAMS_SIZE_B;
}

void
UhrOperation::DpsOpParams::Serialize(Buffer::Iterator& start) const
{
    // TODO: implement once defined in a next 802.11bn draft
    start.WriteU32(0);
}

uint16_t
UhrOperation::DpsOpParams::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    // TODO: implement once defined in a next 802.11bn draft
    start.ReadU32();
    return start.GetDistanceFrom(i);
}

uint16_t
UhrOperation::DpsOpParams::GetSerializedSize() const
{
    return WIFI_UHR_DPS_PARAMS_SIZE_B;
}

void
UhrOperation::NpcaOpParams::Serialize(Buffer::Iterator& start) const
{
    const uint32_t val = primaryChan | (static_cast<uint32_t>(minDurationThresh) << 8) |
                         (static_cast<uint32_t>(switchDelay) << 12) |
                         (static_cast<uint32_t>(switchBackDelay) << 18) |
                         (static_cast<uint32_t>(initialQsrc) << 24) |
                         (static_cast<uint32_t>(moplen) << 26) |
                         (static_cast<uint32_t>(disSubchanBmPresent) << 27);
    start.WriteU32(val);
    if (disSubchanBmPresent == 1)
    {
        NS_ASSERT(disabledSubchBm.has_value());
        start.WriteU16(disabledSubchBm.value());
    }
}

uint16_t
UhrOperation::NpcaOpParams::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    const auto val = start.ReadU32();
    primaryChan = val & 0xff;
    minDurationThresh = (val >> 8) & 0x0f;
    switchDelay = (val >> 12) & 0x3f;
    switchBackDelay = (val >> 18) & 0x3f;
    initialQsrc = (val >> 24) & 0x03;
    moplen = (val >> 26) & 0x01;
    disSubchanBmPresent = (val >> 27) & 0x01;
    disabledSubchBm.reset();
    if (disSubchanBmPresent == 1)
    {
        disabledSubchBm = start.ReadU16();
    }
    return start.GetDistanceFrom(i);
}

uint16_t
UhrOperation::NpcaOpParams::GetSerializedSize() const
{
    auto size = WIFI_UHR_NPCA_PARAMS_SIZE_B;
    if (disSubchanBmPresent == 1)
    {
        size += WIFI_NPCA_DISABLED_SUBCH_BM_SIZE_B;
    }
    return size;
}

void
UhrOperation::PEdcaOpParams::Serialize(Buffer::Iterator& start) const
{
    const uint16_t val = eCWmin | (static_cast<uint16_t>(eCWmax) << 3) |
                         (static_cast<uint16_t>(aifsn) << 6) | (static_cast<uint16_t>(cwDs) << 9) |
                         (static_cast<uint16_t>(psrcThreshold) << 11) |
                         (static_cast<uint16_t>(qsrcThreshold) << 14);
    start.WriteU16(val);
}

uint16_t
UhrOperation::PEdcaOpParams::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    const auto val = start.ReadU16();
    eCWmin = val & 0x07;
    eCWmax = (val >> 3) & 0x07;
    aifsn = (val >> 6) & 0x07;
    cwDs = (val >> 9) & 0x03;
    psrcThreshold = (val >> 11) & 0x07;
    qsrcThreshold = (val >> 14) & 0x03;
    return start.GetDistanceFrom(i);
}

uint16_t
UhrOperation::PEdcaOpParams::GetSerializedSize() const
{
    return WIFI_UHR_PEDCA_PARAMS_SIZE_B;
}

void
UhrOperation::DbeOpParams::Serialize(Buffer::Iterator& start) const
{
    NS_ASSERT(numTransmitPowerEnvelopeElements == transmitPowerEnvelopeElements.size());
    const uint8_t val = dBeBandwidth | (static_cast<uint8_t>(disSubchanBmPresent) << 3) |
                        (static_cast<uint8_t>(numTransmitPowerEnvelopeElements) << 4);
    start.WriteU8(val);
    if (disSubchanBmPresent == 1)
    {
        NS_ASSERT(disabledSubchBm.has_value());
        start.WriteU16(disabledSubchBm.value());
    }
    for (const auto& element : transmitPowerEnvelopeElements)
    {
        start.Write(element.data(), element.size());
    }
}

uint16_t
UhrOperation::DbeOpParams::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    const auto val = start.ReadU8();
    dBeBandwidth = val & 0x07;
    disSubchanBmPresent = (val >> 3) & 0x01;
    numTransmitPowerEnvelopeElements = (val >> 4) & 0x07;
    disabledSubchBm.reset();
    if (disSubchanBmPresent == 1)
    {
        disabledSubchBm = start.ReadU16();
    }
    transmitPowerEnvelopeElements.clear();
    transmitPowerEnvelopeElements.resize(numTransmitPowerEnvelopeElements);
    for (auto& element : transmitPowerEnvelopeElements)
    {
        const auto elementId = start.ReadU8();
        const auto length = start.ReadU8();
        element.resize(2 + length);
        element[0] = elementId;
        element[1] = length;
        start.Read(element.data() + 2, length);
    }
    return start.GetDistanceFrom(i);
}

uint16_t
UhrOperation::DbeOpParams::GetSerializedSize() const
{
    NS_ASSERT(numTransmitPowerEnvelopeElements == transmitPowerEnvelopeElements.size());
    auto size = WIFI_UHR_DBE_PARAMS_SIZE_B;
    if (disSubchanBmPresent == 1)
    {
        size += WIFI_DBE_DISABLED_SUBCH_BM_SIZE_B;
    }
    for (const auto& element : transmitPowerEnvelopeElements)
    {
        size += element.size();
    }
    return size;
}

UhrOperation::UhrOperation()
    : m_duoParams(m_params.duoOpParamsPresent),
      m_dpsParams(m_params.dpsEnabled),
      m_npcaParams(m_params.npcaEnabled),
      m_pEdcaParams(m_params.pEdcaEnabled),
      m_dbeParams(m_params.dbeEnabled)
{
    m_mcsNssSet.maxRxNss.assign(WIFI_UHR_MAX_MCS_INDEX + 1, WIFI_DEFAULT_UHR_MAX_NSS);
    m_mcsNssSet.maxTxNss.assign(WIFI_UHR_MAX_MCS_INDEX + 1, WIFI_DEFAULT_UHR_MAX_NSS);
}

UhrOperation::UhrOperation(const UhrOperation& other)
    : WifiInformationElement(),
      m_params(other.m_params),
      m_mcsNssSet(other.m_mcsNssSet),
      m_duoParams(m_params.duoOpParamsPresent),
      m_dpsParams(m_params.dpsEnabled),
      m_npcaParams(m_params.npcaEnabled),
      m_pEdcaParams(m_params.pEdcaEnabled),
      m_dbeParams(m_params.dbeEnabled)
{
    m_duoParams = other.m_duoParams.ToOptional();
    m_dpsParams = other.m_dpsParams.ToOptional();
    m_npcaParams = other.m_npcaParams.ToOptional();
    m_pEdcaParams = other.m_pEdcaParams.ToOptional();
    m_dbeParams = other.m_dbeParams.ToOptional();
}

UhrOperation&
UhrOperation::operator=(const UhrOperation& other)
{
    if (this != &other)
    {
        m_params = other.m_params;
        m_mcsNssSet = other.m_mcsNssSet;
        m_duoParams = other.m_duoParams.ToOptional();
        m_dpsParams = other.m_dpsParams.ToOptional();
        m_npcaParams = other.m_npcaParams.ToOptional();
        m_pEdcaParams = other.m_pEdcaParams.ToOptional();
        m_dbeParams = other.m_dbeParams.ToOptional();
    }
    return *this;
}

WifiInformationElementId
UhrOperation::ElementId() const
{
    return IE_EXTENSION;
}

WifiInformationElementId
UhrOperation::ElementIdExt() const
{
    return IE_EXT_UHR_OPERATION;
}

uint16_t
UhrOperation::GetInformationFieldSize() const
{
    // IEEE 802.11be D2.0 9.4.2.311
    auto ret = WIFI_IE_ELEMENT_ID_EXT_SIZE + m_params.GetSerializedSize() +
               m_mcsNssSet.GetSerializedSize();
    if (m_duoParams)
    {
        ret += m_duoParams->GetSerializedSize();
    }
    if (m_dpsParams)
    {
        ret += m_dpsParams->GetSerializedSize();
    }
    if (m_npcaParams)
    {
        ret += m_npcaParams->GetSerializedSize();
    }
    if (m_pEdcaParams)
    {
        ret += m_pEdcaParams->GetSerializedSize();
    }
    if (m_dbeParams)
    {
        ret += m_dbeParams->GetSerializedSize();
    }
    return ret;
}

void
UhrOperation::SetMaxRxNss(uint8_t maxNss, uint8_t mcsStart, uint8_t mcsEnd)
{
    NS_ASSERT(mcsStart <= mcsEnd);
    NS_ASSERT((mcsStart >= 0) && (mcsEnd <= WIFI_UHR_MAX_MCS_INDEX));
    NS_ASSERT((maxNss >= 1) && (maxNss <= WIFI_UHR_MAX_NSS_CONFIGURABLE));
    SetMaxNss(m_mcsNssSet.maxRxNss, maxNss, mcsStart, mcsEnd);
}

void
UhrOperation::SetMaxTxNss(uint8_t maxNss, uint8_t mcsStart, uint8_t mcsEnd)
{
    NS_ASSERT(mcsStart <= mcsEnd);
    NS_ASSERT((mcsStart >= 0) && (mcsEnd <= WIFI_UHR_MAX_MCS_INDEX));
    NS_ASSERT((maxNss >= 1) && (maxNss <= WIFI_UHR_MAX_NSS_CONFIGURABLE));
    SetMaxNss(m_mcsNssSet.maxTxNss, maxNss, mcsStart, mcsEnd);
}

void
UhrOperation::SerializeInformationField(Buffer::Iterator start) const
{
    m_params.Serialize(start);
    m_mcsNssSet.Serialize(start);
    if (m_params.duoOpParamsPresent)
    {
        NS_ASSERT_MSG(m_duoParams, "DUO Operation Parameters not set");
        m_duoParams->Serialize(start);
    }
    if (m_params.dpsEnabled)
    {
        NS_ASSERT_MSG(m_dpsParams, "DPS Operation Parameters not set");
        m_dpsParams->Serialize(start);
    }
    if (m_params.npcaEnabled)
    {
        NS_ASSERT_MSG(m_npcaParams, "NPCA Operation Parameters not set");
        m_npcaParams->Serialize(start);
    }
    if (m_params.pEdcaEnabled)
    {
        NS_ASSERT_MSG(m_pEdcaParams, "P-EDCA Operation Parameters not set");
        m_pEdcaParams->Serialize(start);
    }
    if (m_params.dbeEnabled)
    {
        NS_ASSERT_MSG(m_dbeParams, "DBE Operation Parameters not set");
        m_dbeParams->Serialize(start);
    }
}

uint16_t
UhrOperation::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    auto i = start;
    i.Next(m_params.Deserialize(i));
    i.Next(m_mcsNssSet.Deserialize(i));
    if (m_params.duoOpParamsPresent)
    {
        m_duoParams = DuoOpParams();
        i.Next(m_duoParams->Deserialize(i));
    }
    if (m_params.dpsEnabled)
    {
        m_dpsParams = DpsOpParams();
        i.Next(m_dpsParams->Deserialize(i));
    }
    if (m_params.npcaEnabled)
    {
        m_npcaParams = NpcaOpParams();
        i.Next(m_npcaParams->Deserialize(i));
    }
    if (m_params.pEdcaEnabled)
    {
        m_pEdcaParams = PEdcaOpParams();
        i.Next(m_pEdcaParams->Deserialize(i));
    }
    if (m_params.dbeEnabled)
    {
        m_dbeParams = DbeOpParams();
        i.Next(m_dbeParams->Deserialize(i));
    }
    const auto count = i.GetDistanceFrom(start);
    NS_ABORT_MSG_IF(count != length,
                    "UHR Operation Length (" << +length
                                             << ") differs "
                                                "from actual number of bytes read ("
                                             << +count << ")");
    return length;
}

} // namespace ns3
