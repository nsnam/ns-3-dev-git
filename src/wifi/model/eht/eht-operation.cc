/*
 * Copyright (c) 2022
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
 * Author: Sharan Naribole <sharan.naribole@gmail.com>
 */

#include "eht-operation.h"

#include <ns3/assert.h>

#include <algorithm>

namespace ns3
{

void
EhtOperation::Print(std::ostream& os) const
{
    os << "EHT Operation=" << +m_params.opInfoPresent << "|" << +m_params.disabledSubchBmPresent
       << "|" << +m_params.defaultPeDur << "|" << +m_params.grpBuIndLimit << "|"
       << +m_params.grpBuExp << "|[";
    for (const auto& maxRxNss : m_mcsNssSet.maxRxNss)
    {
        os << +maxRxNss << "|";
    }
    os << "]|[";
    for (const auto& maxTxNss : m_mcsNssSet.maxTxNss)
    {
        os << +maxTxNss << "|";
    }
    os << "]";
    if (m_opInfo.has_value())
    {
        os << "|" << +m_opInfo->control.channelWidth << "|" << +m_opInfo->ccfs0 << "|"
           << +m_opInfo->ccfs1;
        if (m_opInfo->disabledSubchBm.has_value())
        {
            os << "|" << m_opInfo->disabledSubchBm.value();
        }
    }
}

void
EhtOperation::EhtOpParams::Serialize(Buffer::Iterator& start) const
{
    uint8_t val = opInfoPresent | (disabledSubchBmPresent << 1) | (defaultPeDur << 2) |
                  (grpBuIndLimit << 3) | (grpBuExp << 4);
    start.WriteU8(val);
}

uint16_t
EhtOperation::EhtOpParams::Deserialize(Buffer::Iterator start)
{
    auto params = start.ReadU8();
    opInfoPresent = params & 0x01;
    disabledSubchBmPresent = (params >> 1) & 0x01;
    defaultPeDur = (params >> 2) & 0x01;
    grpBuIndLimit = (params >> 3) & 0x01;
    grpBuExp = (params >> 4) & 0x03;
    return WIFI_EHT_OP_PARAMS_SIZE_B;
}

/**
 * set the max Tx/Rx NSS for input MCS index range
 * \param vec vector of max NSS per MCS
 * \param maxNss max NSS for input MCS range
 * \param mcsStart MCS index start
 * \param mcsEnd MCS index end
 */
void
SetMaxNss(std::vector<uint8_t>& vec, uint8_t maxNss, uint8_t mcsStart, uint8_t mcsEnd)
{
    NS_ASSERT(mcsStart <= mcsEnd);
    NS_ASSERT((mcsStart >= 0) && (mcsEnd <= WIFI_EHT_MAX_MCS_INDEX));
    NS_ASSERT((maxNss >= 1) && (maxNss <= WIFI_EHT_MAX_NSS_CONFIGURABLE));
    for (auto index = mcsStart; index <= mcsEnd; index++)
    {
        vec[index] = maxNss;
    }
}

/**
 * Get the max Tx/Rx NSS for input MCS index range
 * \param vec vector of max NSS per MCS
 * \param mcsStart MCS index start
 * \param mcsEnd MCS index end
 * \return max Rx NSS
 */
uint32_t
GetMaxNss(const std::vector<uint8_t>& vec, uint8_t mcsStart, uint8_t mcsEnd)
{
    NS_ASSERT(mcsStart <= mcsEnd);
    NS_ASSERT((mcsStart >= 0) && (mcsEnd <= WIFI_EHT_MAX_MCS_INDEX));
    auto minMaxNss = WIFI_EHT_MAX_NSS_CONFIGURABLE;
    for (auto index = mcsStart; index <= mcsEnd; index++)
    {
        if (vec[index] < minMaxNss)
        {
            minMaxNss = vec[index];
        }
    }
    return minMaxNss;
}

void
EhtOperation::EhtBasicMcsNssSet::Serialize(Buffer::Iterator& start) const
{
    uint32_t val = GetMaxNss(maxRxNss, 0, 7) | (GetMaxNss(maxTxNss, 0, 7) << 4) |
                   (GetMaxNss(maxRxNss, 8, 9) << 8) | (GetMaxNss(maxTxNss, 8, 9) << 12) |
                   (GetMaxNss(maxRxNss, 10, 11) << 16) | (GetMaxNss(maxTxNss, 10, 11) << 20) |
                   (GetMaxNss(maxRxNss, 12, 13) << 24) | (GetMaxNss(maxTxNss, 12, 13) << 28);
    start.WriteHtolsbU32(val);
}

uint16_t
EhtOperation::EhtBasicMcsNssSet::Deserialize(Buffer::Iterator start)
{
    auto subfield = start.ReadLsbtohU32();
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
    return WIFI_EHT_BASIC_MCS_NSS_SET_SIZE_B;
}

void
EhtOperation::EhtOpInfo::Serialize(Buffer::Iterator& start) const
{
    start.WriteU8(control.channelWidth); // Control
    start.WriteU8(ccfs0);                // CCFS 0
    start.WriteU8(ccfs1);                // CCFS 1
    if (disabledSubchBm.has_value())
    {
        start.WriteU16(disabledSubchBm.value());
    }
}

uint16_t
EhtOperation::EhtOpInfo::Deserialize(Buffer::Iterator start, bool disabledSubchBmPresent)
{
    auto i = start;
    uint16_t count = 0;
    auto controlSubfield = i.ReadU8();
    count++;
    control.channelWidth = controlSubfield & 0x7;
    ccfs0 = i.ReadU8();
    count++;
    ccfs1 = i.ReadU8();
    count++;
    NS_ASSERT_MSG(count == WIFI_EHT_OP_INFO_BASIC_SIZE_B,
                  "Incorrect EHT Operation Info deserialize");
    if (!disabledSubchBmPresent)
    {
        return count;
    }
    disabledSubchBm = i.ReadU16();
    count += 2;
    NS_ASSERT_MSG(count == WIFI_EHT_OP_INFO_BASIC_SIZE_B + WIFI_EHT_DISABLED_SUBCH_BM_SIZE_B,
                  "Incorrect EHT Operation Info deserialize");
    return count;
}

EhtOperation::EhtOperation()
{
    m_mcsNssSet.maxRxNss.assign(WIFI_EHT_MAX_MCS_INDEX + 1, WIFI_DEFAULT_EHT_MAX_NSS);
    m_mcsNssSet.maxTxNss.assign(WIFI_EHT_MAX_MCS_INDEX + 1, WIFI_DEFAULT_EHT_MAX_NSS);
}

WifiInformationElementId
EhtOperation::ElementId() const
{
    return IE_EXTENSION;
}

WifiInformationElementId
EhtOperation::ElementIdExt() const
{
    return IE_EXT_EHT_OPERATION;
}

uint16_t
EhtOperation::GetInformationFieldSize() const
{
    // IEEE 802.11be D2.0 9.4.2.311
    auto ret =
        WIFI_IE_ELEMENT_ID_EXT_SIZE + WIFI_EHT_OP_PARAMS_SIZE_B + WIFI_EHT_BASIC_MCS_NSS_SET_SIZE_B;
    if (!m_params.opInfoPresent)
    {
        return ret;
    }
    ret += WIFI_EHT_OP_INFO_BASIC_SIZE_B;
    if (!m_params.disabledSubchBmPresent)
    {
        return ret;
    }
    return ret + WIFI_EHT_DISABLED_SUBCH_BM_SIZE_B;
}

void
EhtOperation::SetMaxRxNss(uint8_t maxNss, uint8_t mcsStart, uint8_t mcsEnd)
{
    NS_ASSERT(mcsStart <= mcsEnd);
    NS_ASSERT((mcsStart >= 0) && (mcsEnd <= WIFI_EHT_MAX_MCS_INDEX));
    NS_ASSERT((maxNss >= 1) && (maxNss <= WIFI_EHT_MAX_NSS_CONFIGURABLE));
    SetMaxNss(m_mcsNssSet.maxRxNss, maxNss, mcsStart, mcsEnd);
}

void
EhtOperation::SetMaxTxNss(uint8_t maxNss, uint8_t mcsStart, uint8_t mcsEnd)
{
    NS_ASSERT(mcsStart <= mcsEnd);
    NS_ASSERT((mcsStart >= 0) && (mcsEnd <= WIFI_EHT_MAX_MCS_INDEX));
    NS_ASSERT((maxNss >= 1) && (maxNss <= WIFI_EHT_MAX_NSS_CONFIGURABLE));
    SetMaxNss(m_mcsNssSet.maxTxNss, maxNss, mcsStart, mcsEnd);
}

void
EhtOperation::SerializeInformationField(Buffer::Iterator start) const
{
    m_params.Serialize(start);
    m_mcsNssSet.Serialize(start);
    NS_ASSERT_MSG(m_params.opInfoPresent == m_opInfo.has_value(),
                  "Incorrect setting of EHT Operation Information Present bit");

    if (!m_params.opInfoPresent)
    { // EHT Operation Information Present not set
        return;
    }

    auto disabledSubchBmPresent = m_params.disabledSubchBmPresent > 0;
    NS_ASSERT_MSG(disabledSubchBmPresent == m_opInfo->disabledSubchBm.has_value(),
                  "Incorrect setting of Disabled Subchannel Bitmap Present bit");
    m_opInfo->Serialize(start);
}

uint16_t
EhtOperation::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    auto i = start;
    i.Next(m_params.Deserialize(i));
    i.Next(m_mcsNssSet.Deserialize(i));
    uint16_t count = i.GetDistanceFrom(start);

    if (!m_params.opInfoPresent)
    {
        NS_ASSERT_MSG(count == length, "Unexpected EHT Operation size");
    }

    if (m_params.opInfoPresent > 0)
    {
        auto disabledSubchBmPresent = m_params.disabledSubchBmPresent > 0;
        m_opInfo = EhtOpInfo();
        i.Next(m_opInfo->Deserialize(i, disabledSubchBmPresent));
        count = i.GetDistanceFrom(start);
    }

    NS_ABORT_MSG_IF(count != length,
                    "EHT Operation Length (" << +length
                                             << ") differs "
                                                "from actual number of bytes read ("
                                             << +count << ")");
    return length;
}
} // namespace ns3
