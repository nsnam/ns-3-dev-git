/*
 * Copyright (c) 2015 Sébastien Deronne
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "erp-information.h"

namespace ns3
{

ErpInformation::ErpInformation()
    : m_erpInformation(0)
{
}

WifiInformationElementId
ErpInformation::ElementId() const
{
    return IE_ERP_INFORMATION;
}

void
ErpInformation::SetBarkerPreambleMode(uint8_t barkerPreambleMode)
{
    m_erpInformation |= (barkerPreambleMode & 0x01) << 2;
}

void
ErpInformation::SetUseProtection(uint8_t useProtection)
{
    m_erpInformation |= (useProtection & 0x01) << 1;
}

void
ErpInformation::SetNonErpPresent(uint8_t nonErpPresent)
{
    m_erpInformation |= nonErpPresent & 0x01;
}

uint8_t
ErpInformation::GetBarkerPreambleMode() const
{
    return ((m_erpInformation >> 2) & 0x01);
}

uint8_t
ErpInformation::GetUseProtection() const
{
    return ((m_erpInformation >> 1) & 0x01);
}

uint8_t
ErpInformation::GetNonErpPresent() const
{
    return (m_erpInformation & 0x01);
}

uint16_t
ErpInformation::GetInformationFieldSize() const
{
    return 1;
}

void
ErpInformation::SerializeInformationField(Buffer::Iterator start) const
{
    start.WriteU8(m_erpInformation);
}

uint16_t
ErpInformation::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    Buffer::Iterator i = start;
    m_erpInformation = i.ReadU8();
    return length;
}

std::ostream&
operator<<(std::ostream& os, const ErpInformation& erpInformation)
{
    os << bool(erpInformation.GetBarkerPreambleMode()) << "|"
       << bool(erpInformation.GetUseProtection()) << "|" << bool(erpInformation.GetNonErpPresent());

    return os;
}

} // namespace ns3
