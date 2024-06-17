/*
 * Copyright (c) 2007 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "channel.h"

#include "channel-list.h"
#include "net-device.h"

#include "ns3/log.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Channel");

NS_OBJECT_ENSURE_REGISTERED(Channel);

TypeId
Channel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Channel")
                            .SetParent<Object>()
                            .SetGroupName("Network")
                            .AddAttribute("Id",
                                          "The id (unique integer) of this Channel.",
                                          TypeId::ATTR_GET,
                                          UintegerValue(0),
                                          MakeUintegerAccessor(&Channel::m_id),
                                          MakeUintegerChecker<uint32_t>());
    return tid;
}

Channel::Channel()
    : m_id(0)
{
    NS_LOG_FUNCTION(this);
    m_id = ChannelList::Add(this);
}

Channel::~Channel()
{
    NS_LOG_FUNCTION(this);
}

uint32_t
Channel::GetId() const
{
    NS_LOG_FUNCTION(this);
    return m_id;
}

} // namespace ns3
