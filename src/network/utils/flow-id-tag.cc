/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "flow-id-tag.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("FlowIdTag");

NS_OBJECT_ENSURE_REGISTERED(FlowIdTag);

TypeId
FlowIdTag::GetTypeId()
{
    static TypeId tid = TypeId("ns3::FlowIdTag")
                            .SetParent<Tag>()
                            .SetGroupName("Network")
                            .AddConstructor<FlowIdTag>();
    return tid;
}

TypeId
FlowIdTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
FlowIdTag::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return 4;
}

void
FlowIdTag::Serialize(TagBuffer buf) const
{
    NS_LOG_FUNCTION(this << &buf);
    buf.WriteU32(m_flowId);
}

void
FlowIdTag::Deserialize(TagBuffer buf)
{
    NS_LOG_FUNCTION(this << &buf);
    m_flowId = buf.ReadU32();
}

void
FlowIdTag::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << "FlowId=" << m_flowId;
}

FlowIdTag::FlowIdTag()
    : Tag()
{
    NS_LOG_FUNCTION(this);
}

FlowIdTag::FlowIdTag(uint32_t id)
    : Tag(),
      m_flowId(id)
{
    NS_LOG_FUNCTION(this << id);
}

void
FlowIdTag::SetFlowId(uint32_t id)
{
    NS_LOG_FUNCTION(this << id);
    m_flowId = id;
}

uint32_t
FlowIdTag::GetFlowId() const
{
    NS_LOG_FUNCTION(this);
    return m_flowId;
}

uint32_t
FlowIdTag::AllocateFlowId()
{
    NS_LOG_FUNCTION_NOARGS();
    static uint32_t nextFlowId = 1;
    uint32_t flowId = nextFlowId;
    nextFlowId++;
    return flowId;
}

} // namespace ns3
