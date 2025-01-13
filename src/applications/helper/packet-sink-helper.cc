/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "packet-sink-helper.h"

#include "ns3/string.h"

namespace ns3
{

PacketSinkHelper::PacketSinkHelper(const std::string& protocol, const Address& address)
    : ApplicationHelper("ns3::PacketSink")
{
    m_factory.Set("Protocol", StringValue(protocol));
    m_factory.Set("Local", AddressValue(address));
}

} // namespace ns3
