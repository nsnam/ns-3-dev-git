/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "on-off-helper.h"

#include <ns3/onoff-application.h>
#include <ns3/string.h>
#include <ns3/uinteger.h>

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
