/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "addba-extension.h"

namespace
{
/// Size in bytes of ADDBA Extended Parameter Set field, IEEE 802.11-2024 Figure 9-151
constexpr uint32_t WIFI_ADDBA_EXT_PARAMETER_SET_SIZE_B = 1;
} // namespace

namespace ns3
{

uint32_t
AddbaExtension::ExtParamSet::GetSerializedSize() const
{
    return WIFI_ADDBA_EXT_PARAMETER_SET_SIZE_B;
}

Buffer::Iterator
AddbaExtension::ExtParamSet::Serialize(Buffer::Iterator start) const
{
    uint8_t extParamSet = noFragment | (heFragmentOp << 1) | (extBufferSize << 5);
    start.WriteU8(extParamSet);
    return start;
}

Buffer::Iterator
AddbaExtension::ExtParamSet::Deserialize(Buffer::Iterator start)
{
    auto i = start;
    const auto extParamSet = i.ReadU8();
    noFragment = extParamSet & 0x01;
    heFragmentOp = (extParamSet >> 1) & 0x03;
    extBufferSize = (extParamSet >> 5) & 0x07;
    return i;
}

void
AddbaExtension::Print(std::ostream& os) const
{
    os << "ADDBA Extension=[" << m_extParamSet << "]";
}

WifiInformationElementId
AddbaExtension::ElementId() const
{
    return IE_ADDBA_EXTENSION;
}

uint16_t
AddbaExtension::GetInformationFieldSize() const
{
    return m_extParamSet.GetSerializedSize();
}

void
AddbaExtension::SerializeInformationField(Buffer::Iterator start) const
{
    m_extParamSet.Serialize(start);
}

uint16_t
AddbaExtension::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    auto i = start;
    i = m_extParamSet.Deserialize(i);
    return i.GetDistanceFrom(start);
}

std::ostream&
operator<<(std::ostream& os, const AddbaExtension::ExtParamSet& params)
{
    os << "No Fragmentation: " << +params.noFragment
       << ", HE Fragmentation Operation: " << +params.heFragmentOp
       << ", Extended Buffer Size: " << +params.extBufferSize;
    return os;
}

} // namespace ns3
