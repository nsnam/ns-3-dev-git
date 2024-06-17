/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#include "lte-rlc-sdu-status-tag.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(LteRlcSduStatusTag);

LteRlcSduStatusTag::LteRlcSduStatusTag()
{
}

void
LteRlcSduStatusTag::SetStatus(uint8_t status)
{
    m_sduStatus = status;
}

uint8_t
LteRlcSduStatusTag::GetStatus() const
{
    return m_sduStatus;
}

TypeId
LteRlcSduStatusTag::GetTypeId()
{
    static TypeId tid = TypeId("ns3::LteRlcSduStatusTag")
                            .SetParent<Tag>()
                            .SetGroupName("Lte")
                            .AddConstructor<LteRlcSduStatusTag>();
    return tid;
}

TypeId
LteRlcSduStatusTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
LteRlcSduStatusTag::GetSerializedSize() const
{
    return 1;
}

void
LteRlcSduStatusTag::Serialize(TagBuffer i) const
{
    i.WriteU8(m_sduStatus);
}

void
LteRlcSduStatusTag::Deserialize(TagBuffer i)
{
    m_sduStatus = i.ReadU8();
}

void
LteRlcSduStatusTag::Print(std::ostream& os) const
{
    os << "SDU Status=" << (uint32_t)m_sduStatus;
}

}; // namespace ns3
