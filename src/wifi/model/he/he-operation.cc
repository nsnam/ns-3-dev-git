/*
 * Copyright (c) 2017 Sébastien Deronne
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Sébastien Deronne <sebastien.deronne@gmail.com>
 *          Stefano Avallone <stavallo@unina.it>
 */

#include "he-operation.h"

namespace ns3
{

HeOperation::HeOperation()
    : m_basicHeMcsAndNssSet(0xffff),
      m_6GHzOpInfo(m_heOpParams.m_6GHzOpPresent)
{
}

WifiInformationElementId
HeOperation::ElementId() const
{
    return IE_EXTENSION;
}

WifiInformationElementId
HeOperation::ElementIdExt() const
{
    return IE_EXT_HE_OPERATION;
}

void
HeOperation::HeOperationParams::Print(std::ostream& os) const
{
    os << "Default PE Duration: " << +m_defaultPeDuration << " TWT Required: " << +m_twtRequired
       << " TXOP Duration RTS Threshold: " << m_txopDurRtsThresh
       << " VHT Operation Information Present: " << +m_vhOpPresent
       << " Co-Hosted BSS: " << +m_coHostedBss << " ER SU Disable: " << +m_erSuDisable
       << " 6 GHz Operation Information Present: " << m_6GHzOpPresent;
}

uint16_t
HeOperation::HeOperationParams::GetSerializedSize() const
{
    return 3;
}

void
HeOperation::HeOperationParams::Serialize(Buffer::Iterator& start) const
{
    uint16_t twoBytes = m_defaultPeDuration | (m_twtRequired << 3) | (m_txopDurRtsThresh << 4) |
                        (m_vhOpPresent << 14) | (m_coHostedBss << 15);
    uint8_t oneByte = m_erSuDisable | ((m_6GHzOpPresent ? 1 : 0) << 1);
    start.WriteHtolsbU16(twoBytes);
    start.WriteU8(oneByte);
}

uint16_t
HeOperation::HeOperationParams::Deserialize(Buffer::Iterator& start)
{
    Buffer::Iterator tmp = start;
    uint16_t twoBytes = start.ReadLsbtohU16();
    uint8_t oneByte = start.ReadU8();
    m_defaultPeDuration = twoBytes & 0x07;
    m_twtRequired = (twoBytes >> 3) & 0x01;
    m_txopDurRtsThresh = (twoBytes >> 4) & 0x03ff;
    m_vhOpPresent = (twoBytes >> 14) & 0x01;
    m_coHostedBss = (twoBytes >> 15) & 0x01;
    m_erSuDisable = oneByte & 0x01;
    m_6GHzOpPresent = ((oneByte >> 1) & 0x01) == 1;
    return start.GetDistanceFrom(tmp);
}

void
HeOperation::BssColorInfo::Print(std::ostream& os) const
{
    os << "BSS Color: " << +m_bssColor << " Partial BSS Color: " << +m_partialBssColor
       << " BSS Color Disabled: " << +m_bssColorDisabled;
}

uint16_t
HeOperation::BssColorInfo::GetSerializedSize() const
{
    return 1;
}

void
HeOperation::BssColorInfo::Serialize(Buffer::Iterator& start) const
{
    uint8_t oneByte = m_bssColor | (m_partialBssColor << 6) | (m_bssColorDisabled << 7);
    start.WriteU8(oneByte);
}

uint16_t
HeOperation::BssColorInfo::Deserialize(Buffer::Iterator& start)
{
    Buffer::Iterator tmp = start;
    uint8_t oneByte = start.ReadU8();
    m_bssColor = oneByte & 0x3f;
    m_partialBssColor = (oneByte >> 6) & 0x01;
    m_bssColorDisabled = (oneByte >> 7) & 0x01;
    return start.GetDistanceFrom(tmp);
}

void
HeOperation::OpInfo6GHz::Print(std::ostream& os) const
{
    os << "Primary channel: " << +m_primCh << " Channel Width: " << +m_chWid
       << " Duplicate Beacon: " << +m_dupBeacon << " Regulatory Info: " << +m_regInfo
       << " Channel center frequency segment 0: " << +m_chCntrFreqSeg0
       << " Channel center frequency segment 1: " << +m_chCntrFreqSeg1
       << " Minimum Rate: " << +m_minRate;
}

uint16_t
HeOperation::OpInfo6GHz::GetSerializedSize() const
{
    return 5;
}

void
HeOperation::OpInfo6GHz::Serialize(Buffer::Iterator& start) const
{
    start.WriteU8(m_primCh);
    uint8_t control = m_chWid | (m_dupBeacon << 2) | (m_regInfo << 3);
    start.WriteU8(control);
    start.WriteU8(m_chCntrFreqSeg0);
    start.WriteU8(m_chCntrFreqSeg1);
    start.WriteU8(m_minRate);
}

uint16_t
HeOperation::OpInfo6GHz::Deserialize(Buffer::Iterator& start)
{
    Buffer::Iterator i = start;
    m_primCh = i.ReadU8();
    uint8_t control = i.ReadU8();
    m_chWid = control & 0x03;
    m_dupBeacon = (control >> 2) & 0x01;
    m_regInfo = (control >> 3) & 0x07;
    m_chCntrFreqSeg0 = i.ReadU8();
    m_chCntrFreqSeg1 = i.ReadU8();
    m_minRate = i.ReadU8();
    return i.GetDistanceFrom(start);
}

void
HeOperation::SetMaxHeMcsPerNss(uint8_t nss, uint8_t maxHeMcs)
{
    NS_ASSERT((maxHeMcs >= 7 && maxHeMcs <= 11) && (nss >= 1 && nss <= 8));

    // IEEE 802.11ax-2021 9.4.2.248.4 Supported HE-MCS And NSS Set field
    uint8_t val = 0x03; // not supported
    if (maxHeMcs == 11) // MCS 0 - 11
    {
        val = 0x02;
    }
    else if (maxHeMcs >= 9) // MCS 0 - 9
    {
        val = 0x01;
    }
    else // MCS 0 - 7
    {
        val = 0x00;
    }

    // clear bits for that nss
    const uint16_t mask = ~(0x03 << ((nss - 1) * 2));
    m_basicHeMcsAndNssSet &= mask;

    // update bits for that nss
    m_basicHeMcsAndNssSet |= ((val & 0x03) << ((nss - 1) * 2));
}

void
HeOperation::Print(std::ostream& os) const
{
    os << "HE Operation=[HE Operation Parameters|";
    m_heOpParams.Print(os);
    os << "][BSS Color|";
    m_bssColorInfo.Print(os);
    os << "][Basic HE-MCS And NSS Set: " << m_basicHeMcsAndNssSet << "]";
    if (m_6GHzOpInfo)
    {
        os << "[6 GHz Operation Info|";
        m_6GHzOpInfo->Print(os);
        os << "]";
    }
}

uint16_t
HeOperation::GetInformationFieldSize() const
{
    uint16_t ret = 1 /* Element ID Ext */ + m_heOpParams.GetSerializedSize() +
                   m_bssColorInfo.GetSerializedSize() + 2 /* Basic HE-MCS And NSS Set */;
    if (m_6GHzOpInfo)
    {
        ret += m_6GHzOpInfo->GetSerializedSize();
    }
    return ret;
}

void
HeOperation::SerializeInformationField(Buffer::Iterator start) const
{
    m_heOpParams.Serialize(start);
    m_bssColorInfo.Serialize(start);
    start.WriteHtolsbU16(m_basicHeMcsAndNssSet);
    if (m_6GHzOpInfo)
    {
        m_6GHzOpInfo->Serialize(start);
    }
    // todo: VHT Operation Information (variable)
}

uint16_t
HeOperation::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    Buffer::Iterator i = start;
    m_heOpParams.Deserialize(i);
    m_bssColorInfo.Deserialize(i);
    m_basicHeMcsAndNssSet = i.ReadLsbtohU16();
    if (m_heOpParams.m_6GHzOpPresent)
    {
        OpInfo6GHz opInfo6GHz;
        opInfo6GHz.Deserialize(i);
        m_6GHzOpInfo = opInfo6GHz;
    }

    // todo: VHT Operation Information (variable)
    return i.GetDistanceFrom(start);
}

} // namespace ns3
