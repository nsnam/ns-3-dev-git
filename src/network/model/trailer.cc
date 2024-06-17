/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@cutebugs.net>
 */

#include "trailer.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Trailer");

NS_OBJECT_ENSURE_REGISTERED(Trailer);

Trailer::~Trailer()
{
    NS_LOG_FUNCTION(this);
}

TypeId
Trailer::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Trailer").SetParent<Chunk>().SetGroupName("Network");
    return tid;
}

// This default implementation is provided for backward compatibility
// reasons.  Subclasses should implement this method themselves.
uint32_t
Trailer::Deserialize(Buffer::Iterator start, Buffer::Iterator end)
{
    return Deserialize(end);
}

std::ostream&
operator<<(std::ostream& os, const Trailer& trailer)
{
    trailer.Print(os);
    return os;
}

} // namespace ns3
