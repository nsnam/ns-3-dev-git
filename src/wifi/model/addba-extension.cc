/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "addba-extension.h"

namespace ns3
{

void
AddbaExtension::Print(std::ostream& os) const
{
    os << "extBufferSize=" << +m_extParamSet.extBufferSize;
}

WifiInformationElementId
AddbaExtension::ElementId() const
{
    return IE_ADDBA_EXTENSION;
}

uint16_t
AddbaExtension::GetInformationFieldSize() const
{
    return 1U; // ADDBA Extended Parameter Set field
}

void
AddbaExtension::SerializeInformationField(Buffer::Iterator start) const
{
    uint8_t extParamSet = m_extParamSet.noFragment | (m_extParamSet.heFragmentOp << 1) |
                          (m_extParamSet.extBufferSize << 5);
    start.WriteU8(extParamSet);
}

uint16_t
AddbaExtension::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    auto extParamSet = start.ReadU8();
    m_extParamSet.noFragment = extParamSet & 0x01;
    m_extParamSet.heFragmentOp = (extParamSet >> 1) & 0x03;
    m_extParamSet.extBufferSize = (extParamSet >> 5) & 0x07;
    return 1;
}

} // namespace ns3
