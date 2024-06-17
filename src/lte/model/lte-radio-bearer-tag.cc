/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marco Miozzo <marco.miozzo@cttc.es>
 */

#include "lte-radio-bearer-tag.h"

#include "ns3/tag.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(LteRadioBearerTag);

TypeId
LteRadioBearerTag::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::LteRadioBearerTag")
            .SetParent<Tag>()
            .SetGroupName("Lte")
            .AddConstructor<LteRadioBearerTag>()
            .AddAttribute("rnti",
                          "The rnti that indicates the UE to which packet belongs",
                          UintegerValue(0),
                          MakeUintegerAccessor(&LteRadioBearerTag::GetRnti),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute(
                "lcid",
                "The id within the UE identifying the logical channel to which the packet belongs",
                UintegerValue(0),
                MakeUintegerAccessor(&LteRadioBearerTag::GetLcid),
                MakeUintegerChecker<uint8_t>());
    return tid;
}

TypeId
LteRadioBearerTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

LteRadioBearerTag::LteRadioBearerTag()
    : m_rnti(0),
      m_lcid(0),
      m_layer(0)
{
}

LteRadioBearerTag::LteRadioBearerTag(uint16_t rnti, uint8_t lcid)
    : m_rnti(rnti),
      m_lcid(lcid)
{
}

LteRadioBearerTag::LteRadioBearerTag(uint16_t rnti, uint8_t lcid, uint8_t layer)
    : m_rnti(rnti),
      m_lcid(lcid),
      m_layer(layer)
{
}

void
LteRadioBearerTag::SetRnti(uint16_t rnti)
{
    m_rnti = rnti;
}

void
LteRadioBearerTag::SetLcid(uint8_t lcid)
{
    m_lcid = lcid;
}

void
LteRadioBearerTag::SetLayer(uint8_t layer)
{
    m_layer = layer;
}

uint32_t
LteRadioBearerTag::GetSerializedSize() const
{
    return 4;
}

void
LteRadioBearerTag::Serialize(TagBuffer i) const
{
    i.WriteU16(m_rnti);
    i.WriteU8(m_lcid);
    i.WriteU8(m_layer);
}

void
LteRadioBearerTag::Deserialize(TagBuffer i)
{
    m_rnti = (uint16_t)i.ReadU16();
    m_lcid = (uint8_t)i.ReadU8();
    m_layer = (uint8_t)i.ReadU8();
}

uint16_t
LteRadioBearerTag::GetRnti() const
{
    return m_rnti;
}

uint8_t
LteRadioBearerTag::GetLcid() const
{
    return m_lcid;
}

uint8_t
LteRadioBearerTag::GetLayer() const
{
    return m_layer;
}

void
LteRadioBearerTag::Print(std::ostream& os) const
{
    os << "rnti=" << m_rnti << ", lcid=" << (uint16_t)m_lcid << ", layer=" << (uint16_t)m_layer;
}

} // namespace ns3
