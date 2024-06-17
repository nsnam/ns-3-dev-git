/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@cutebugs.net>
 */

#include "chunk.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(Chunk);

TypeId
Chunk::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Chunk").SetParent<ObjectBase>().SetGroupName("Network");
    return tid;
}

// This default implementation is provided for backward compatibility
// reasons.  Subclasses should implement this method themselves.
uint32_t
Chunk::Deserialize(Buffer::Iterator start, Buffer::Iterator end)
{
    return Deserialize(start);
}

} // namespace ns3
