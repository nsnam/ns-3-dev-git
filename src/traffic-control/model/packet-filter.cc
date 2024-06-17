/*
 * Copyright (c) 2016 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "packet-filter.h"

#include "queue-disc.h"

#include "ns3/integer.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PacketFilter");

NS_OBJECT_ENSURE_REGISTERED(PacketFilter);

TypeId
PacketFilter::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::PacketFilter").SetParent<Object>().SetGroupName("TrafficControl");
    return tid;
}

PacketFilter::PacketFilter()
{
    NS_LOG_FUNCTION(this);
}

PacketFilter::~PacketFilter()
{
    NS_LOG_FUNCTION(this);
}

int32_t
PacketFilter::Classify(Ptr<QueueDiscItem> item) const
{
    NS_LOG_FUNCTION(this << item);

    if (!CheckProtocol(item))
    {
        NS_LOG_LOGIC("Unable to classify packets of this protocol");
        return PF_NO_MATCH;
    }

    return DoClassify(item);
}

} // namespace ns3
