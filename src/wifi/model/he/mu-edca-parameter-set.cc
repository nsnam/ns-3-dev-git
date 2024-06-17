/*
 * Copyright (c) 2021 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "mu-edca-parameter-set.h"

#include <algorithm>
#include <cmath>

namespace ns3
{

MuEdcaParameterSet::MuEdcaParameterSet()
    : m_qosInfo(0),
      m_records{{{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}
{
}

WifiInformationElementId
MuEdcaParameterSet::ElementId() const
{
    return IE_EXTENSION;
}

WifiInformationElementId
MuEdcaParameterSet::ElementIdExt() const
{
    return IE_EXT_MU_EDCA_PARAMETER_SET;
}

void
MuEdcaParameterSet::SetQosInfo(uint8_t qosInfo)
{
    m_qosInfo = qosInfo;
}

void
MuEdcaParameterSet::SetMuAifsn(uint8_t aci, uint8_t aifsn)
{
    NS_ABORT_MSG_IF(aci > 3, "Invalid AC Index value: " << +aci);
    NS_ABORT_MSG_IF(aifsn == 1 || aifsn > 15, "Invalid AIFSN value: " << +aifsn);

    m_records[aci].aifsnField |= (aifsn & 0x0f);
    m_records[aci].aifsnField |= (aci & 0x03) << 5;
}

void
MuEdcaParameterSet::SetMuCwMin(uint8_t aci, uint16_t cwMin)
{
    NS_ABORT_MSG_IF(aci > 3, "Invalid AC Index value: " << +aci);
    NS_ABORT_MSG_IF(cwMin > 32767, "CWmin exceeds the maximum value");

    auto eCwMin = std::log2(cwMin + 1);
    NS_ABORT_MSG_IF(std::trunc(eCwMin) != eCwMin, "CWmin is not a power of 2 minus 1");

    m_records[aci].cwMinMax |= (static_cast<uint8_t>(eCwMin) & 0x0f);
}

void
MuEdcaParameterSet::SetMuCwMax(uint8_t aci, uint16_t cwMax)
{
    NS_ABORT_MSG_IF(aci > 3, "Invalid AC Index value: " << +aci);
    NS_ABORT_MSG_IF(cwMax > 32767, "CWmin exceeds the maximum value");

    auto eCwMax = std::log2(cwMax + 1);
    NS_ABORT_MSG_IF(std::trunc(eCwMax) != eCwMax, "CWmax is not a power of 2 minus 1");

    m_records[aci].cwMinMax |= (static_cast<uint8_t>(eCwMax) & 0x0f) << 4;
}

void
MuEdcaParameterSet::SetMuEdcaTimer(uint8_t aci, Time timer)
{
    NS_ABORT_MSG_IF(aci > 3, "Invalid AC Index value: " << +aci);
    NS_ABORT_MSG_IF(timer.IsStrictlyPositive() && timer < MicroSeconds(8192),
                    "Timer value is below 8.192 ms");
    NS_ABORT_MSG_IF(timer > MicroSeconds(2088960), "Timer value is above 2088.96 ms");

    double value = timer.GetMicroSeconds() / 8192.;
    NS_ABORT_MSG_IF(std::trunc(value) != value, "Timer value is not a multiple of 8 TUs (8192 us)");

    m_records[aci].muEdcaTimer = static_cast<uint8_t>(value);
}

uint8_t
MuEdcaParameterSet::GetQosInfo() const
{
    return m_qosInfo;
}

uint8_t
MuEdcaParameterSet::GetMuAifsn(uint8_t aci) const
{
    NS_ABORT_MSG_IF(aci > 3, "Invalid AC Index value: " << +aci);
    return (m_records[aci].aifsnField & 0x0f);
}

uint16_t
MuEdcaParameterSet::GetMuCwMin(uint8_t aci) const
{
    NS_ABORT_MSG_IF(aci > 3, "Invalid AC Index value: " << +aci);
    uint8_t eCwMin = (m_records[aci].cwMinMax & 0x0f);
    return static_cast<uint16_t>(std::exp2(eCwMin) - 1);
}

uint16_t
MuEdcaParameterSet::GetMuCwMax(uint8_t aci) const
{
    NS_ABORT_MSG_IF(aci > 3, "Invalid AC Index value: " << +aci);
    uint8_t eCwMax = ((m_records[aci].cwMinMax >> 4) & 0x0f);
    return static_cast<uint16_t>(std::exp2(eCwMax) - 1);
}

Time
MuEdcaParameterSet::GetMuEdcaTimer(uint8_t aci) const
{
    NS_ABORT_MSG_IF(aci > 3, "Invalid AC Index value: " << +aci);
    return MicroSeconds(m_records[aci].muEdcaTimer * 8192);
}

uint16_t
MuEdcaParameterSet::GetInformationFieldSize() const
{
    // ElementIdExt (1) + QoS Info (1) + MU Parameter Records (4 * 3)
    return 14;
}

void
MuEdcaParameterSet::SerializeInformationField(Buffer::Iterator start) const
{
    start.WriteU8(GetQosInfo());
    for (const auto& record : m_records)
    {
        start.WriteU8(record.aifsnField);
        start.WriteU8(record.cwMinMax);
        start.WriteU8(record.muEdcaTimer);
    }
}

uint16_t
MuEdcaParameterSet::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    Buffer::Iterator i = start;
    m_qosInfo = i.ReadU8();
    for (auto& record : m_records)
    {
        record.aifsnField = i.ReadU8();
        record.cwMinMax = i.ReadU8();
        record.muEdcaTimer = i.ReadU8();
    }
    return 13;
}

} // namespace ns3
