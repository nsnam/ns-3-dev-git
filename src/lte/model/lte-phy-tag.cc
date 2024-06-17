/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marco Miozzo <marco.miozzo@cttc.es>
 *         Nicola Baldo <nbaldo@cttc.es>
 */

#include "lte-phy-tag.h"

#include "ns3/tag.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(LtePhyTag);

TypeId
LtePhyTag::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::LtePhyTag").SetParent<Tag>().SetGroupName("Lte").AddConstructor<LtePhyTag>();
    return tid;
}

TypeId
LtePhyTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

LtePhyTag::LtePhyTag()
{
}

LtePhyTag::LtePhyTag(uint16_t cellId)
    : m_cellId(cellId)
{
}

LtePhyTag::~LtePhyTag()
{
}

uint32_t
LtePhyTag::GetSerializedSize() const
{
    return 2;
}

void
LtePhyTag::Serialize(TagBuffer i) const
{
    i.WriteU16(m_cellId);
}

void
LtePhyTag::Deserialize(TagBuffer i)
{
    m_cellId = i.ReadU16();
}

void
LtePhyTag::Print(std::ostream& os) const
{
    os << m_cellId;
}

uint16_t
LtePhyTag::GetCellId() const
{
    return m_cellId;
}

} // namespace ns3
