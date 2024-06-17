/*
 * Copyright (c) 2016 Sébastien Deronne
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "edca-parameter-set.h"

#include <cmath>

namespace ns3
{

EdcaParameterSet::EdcaParameterSet()
    : m_qosInfo(0),
      m_reserved(0),
      m_acBE(0),
      m_acBK(0),
      m_acVI(0),
      m_acVO(0)
{
}

WifiInformationElementId
EdcaParameterSet::ElementId() const
{
    return IE_EDCA_PARAMETER_SET;
}

void
EdcaParameterSet::SetQosInfo(uint8_t qosInfo)
{
    m_qosInfo = qosInfo;
}

void
EdcaParameterSet::SetBeAifsn(uint8_t aifsn)
{
    m_acBE |= (aifsn & 0x0f);
}

void
EdcaParameterSet::SetBeAci(uint8_t aci)
{
    m_acBE |= (aci & 0x03) << 5;
}

void
EdcaParameterSet::SetBeCWmin(uint32_t cwMin)
{
    auto ECWmin = static_cast<uint8_t>(log2(cwMin + 1));
    m_acBE |= (ECWmin & 0x0f) << 8;
}

void
EdcaParameterSet::SetBeCWmax(uint32_t cwMax)
{
    auto ECWmax = static_cast<uint8_t>(log2(cwMax + 1));
    m_acBE |= (ECWmax & 0x0f) << 12;
}

void
EdcaParameterSet::SetBeTxopLimit(uint16_t txop)
{
    m_acBE |= txop << 16;
}

void
EdcaParameterSet::SetBkAifsn(uint8_t aifsn)
{
    m_acBK |= (aifsn & 0x0f);
}

void
EdcaParameterSet::SetBkAci(uint8_t aci)
{
    m_acBK |= (aci & 0x03) << 5;
}

void
EdcaParameterSet::SetBkCWmin(uint32_t cwMin)
{
    auto ECWmin = static_cast<uint8_t>(log2(cwMin + 1));
    m_acBK |= (ECWmin & 0x0f) << 8;
}

void
EdcaParameterSet::SetBkCWmax(uint32_t cwMax)
{
    auto ECWmax = static_cast<uint8_t>(log2(cwMax + 1));
    m_acBK |= (ECWmax & 0x0f) << 12;
}

void
EdcaParameterSet::SetBkTxopLimit(uint16_t txop)
{
    m_acBK |= txop << 16;
}

void
EdcaParameterSet::SetViAifsn(uint8_t aifsn)
{
    m_acVI |= (aifsn & 0x0f);
}

void
EdcaParameterSet::SetViAci(uint8_t aci)
{
    m_acVI |= (aci & 0x03) << 5;
}

void
EdcaParameterSet::SetViCWmin(uint32_t cwMin)
{
    auto ECWmin = static_cast<uint8_t>(log2(cwMin + 1));
    m_acVI |= (ECWmin & 0x0f) << 8;
}

void
EdcaParameterSet::SetViCWmax(uint32_t cwMax)
{
    auto ECWmax = static_cast<uint8_t>(log2(cwMax + 1));
    m_acVI |= (ECWmax & 0x0f) << 12;
}

void
EdcaParameterSet::SetViTxopLimit(uint16_t txop)
{
    m_acVI |= txop << 16;
}

void
EdcaParameterSet::SetVoAifsn(uint8_t aifsn)
{
    m_acVO |= (aifsn & 0x0f);
}

void
EdcaParameterSet::SetVoAci(uint8_t aci)
{
    m_acVO |= (aci & 0x03) << 5;
}

void
EdcaParameterSet::SetVoCWmin(uint32_t cwMin)
{
    auto ECWmin = static_cast<uint8_t>(log2(cwMin + 1));
    m_acVO |= (ECWmin & 0x0f) << 8;
}

void
EdcaParameterSet::SetVoCWmax(uint32_t cwMax)
{
    auto ECWmax = static_cast<uint8_t>(log2(cwMax + 1));
    m_acVO |= (ECWmax & 0x0f) << 12;
}

void
EdcaParameterSet::SetVoTxopLimit(uint16_t txop)
{
    m_acVO |= txop << 16;
}

uint8_t
EdcaParameterSet::GetQosInfo() const
{
    return m_qosInfo;
}

uint8_t
EdcaParameterSet::GetBeAifsn() const
{
    return (m_acBE & 0x0f);
}

uint32_t
EdcaParameterSet::GetBeCWmin() const
{
    uint8_t ECWmin = ((m_acBE >> 8) & 0x0f);
    return static_cast<uint32_t>(exp2(ECWmin) - 1);
}

uint32_t
EdcaParameterSet::GetBeCWmax() const
{
    uint8_t ECWmax = ((m_acBE >> 12) & 0x0f);
    return static_cast<uint32_t>(exp2(ECWmax) - 1);
}

uint16_t
EdcaParameterSet::GetBeTxopLimit() const
{
    return (m_acBE >> 16);
}

uint8_t
EdcaParameterSet::GetBkAifsn() const
{
    return (m_acBK & 0x0f);
}

uint32_t
EdcaParameterSet::GetBkCWmin() const
{
    uint8_t ECWmin = ((m_acBK >> 8) & 0x0f);
    return static_cast<uint32_t>(exp2(ECWmin) - 1);
}

uint32_t
EdcaParameterSet::GetBkCWmax() const
{
    uint8_t ECWmax = ((m_acBK >> 12) & 0x0f);
    return static_cast<uint32_t>(exp2(ECWmax) - 1);
}

uint16_t
EdcaParameterSet::GetBkTxopLimit() const
{
    return (m_acBK >> 16);
}

uint8_t
EdcaParameterSet::GetViAifsn() const
{
    return (m_acVI & 0x0f);
}

uint32_t
EdcaParameterSet::GetViCWmin() const
{
    uint8_t ECWmin = ((m_acVI >> 8) & 0x0f);
    return static_cast<uint32_t>(exp2(ECWmin) - 1);
}

uint32_t
EdcaParameterSet::GetViCWmax() const
{
    uint8_t ECWmax = ((m_acVI >> 12) & 0x0f);
    return static_cast<uint32_t>(exp2(ECWmax) - 1);
}

uint16_t
EdcaParameterSet::GetViTxopLimit() const
{
    return (m_acVI >> 16);
}

uint8_t
EdcaParameterSet::GetVoAifsn() const
{
    return (m_acVO & 0x0f);
}

uint32_t
EdcaParameterSet::GetVoCWmin() const
{
    uint8_t ECWmin = ((m_acVO >> 8) & 0x0f);
    return static_cast<uint32_t>(exp2(ECWmin) - 1);
}

uint32_t
EdcaParameterSet::GetVoCWmax() const
{
    uint8_t ECWmax = ((m_acVO >> 12) & 0x0f);
    return static_cast<uint32_t>(exp2(ECWmax) - 1);
}

uint16_t
EdcaParameterSet::GetVoTxopLimit() const
{
    return (m_acVO >> 16);
}

uint16_t
EdcaParameterSet::GetInformationFieldSize() const
{
    return 18;
}

void
EdcaParameterSet::SerializeInformationField(Buffer::Iterator start) const
{
    start.WriteU8(GetQosInfo());
    start.WriteU8(m_reserved);
    start.WriteU32(m_acBE);
    start.WriteU32(m_acBK);
    start.WriteU32(m_acVI);
    start.WriteU32(m_acVO);
}

uint16_t
EdcaParameterSet::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    Buffer::Iterator i = start;
    m_qosInfo = i.ReadU8();
    m_reserved = i.ReadU8();
    m_acBE = i.ReadU32();
    m_acBK = i.ReadU32();
    m_acVI = i.ReadU32();
    m_acVO = i.ReadU32();
    return length;
}

} // namespace ns3
