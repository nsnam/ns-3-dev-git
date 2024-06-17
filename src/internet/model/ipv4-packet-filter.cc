/*
 * Copyright (c) 2016 Universita' degli Studi di Napoli Federico II
 *               2016 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:  Stefano Avallone <stavallo@unina.it>
 *           Tom Henderson <tomhend@u.washington.edu>
 *           Pasquale Imputato <p.imputato@gmail.com>
 */

#include "ipv4-packet-filter.h"

#include "ipv4-queue-disc-item.h"

#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4PacketFilter");

NS_OBJECT_ENSURE_REGISTERED(Ipv4PacketFilter);

TypeId
Ipv4PacketFilter::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::Ipv4PacketFilter").SetParent<PacketFilter>().SetGroupName("Internet");
    return tid;
}

Ipv4PacketFilter::Ipv4PacketFilter()
{
    NS_LOG_FUNCTION(this);
}

Ipv4PacketFilter::~Ipv4PacketFilter()
{
    NS_LOG_FUNCTION(this);
}

bool
Ipv4PacketFilter::CheckProtocol(Ptr<QueueDiscItem> item) const
{
    NS_LOG_FUNCTION(this << item);
    return bool(DynamicCast<Ipv4QueueDiscItem>(item));
}

// ------------------------------------------------------------------------- //

} // namespace ns3
