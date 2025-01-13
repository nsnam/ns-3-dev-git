/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "on-off-helper.h"

#include "ns3/onoff-application.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

namespace ns3
{

OnOffHelper::OnOffHelper(const std::string& protocol, const Address& address)
    : ApplicationHelper("ns3::OnOffApplication")
{
    m_factory.Set("Protocol", StringValue(protocol));
    m_factory.Set("Remote", AddressValue(address));
}

void
OnOffHelper::SetConstantRate(DataRate dataRate, uint32_t packetSize)
{
    m_factory.Set("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1000]"));
    m_factory.Set("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    m_factory.Set("DataRate", DataRateValue(dataRate));
    m_factory.Set("PacketSize", UintegerValue(packetSize));
}

} // namespace ns3
