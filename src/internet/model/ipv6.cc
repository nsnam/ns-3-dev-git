/*
 * Copyright (c) 2007 INRIA
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

/* taken from src/node/ipv4.h and adapted to IPv6 */

#include "ipv6.h"

#include "ns3/assert.h"
#include "ns3/boolean.h"
#include "ns3/node.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(Ipv6);

TypeId
Ipv6::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::Ipv6")
            .SetParent<Object>()
            .SetGroupName("Internet")
            .AddAttribute(
                "IpForward",
                "Globally enable or disable IP forwarding for all current and future IPv6 devices.",
                BooleanValue(false),
                MakeBooleanAccessor(&Ipv6::SetIpForward, &Ipv6::GetIpForward),
                MakeBooleanChecker())
            .AddAttribute("MtuDiscover",
                          "If disabled, every interface will have its MTU set to 1280 bytes.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&Ipv6::SetMtuDiscover, &Ipv6::GetMtuDiscover),
                          MakeBooleanChecker())
            .AddAttribute(
                "StrongEndSystemModel",
                "Reject packets for an address not configured on the interface they're "
                "coming from (RFC1122, section 3.3.4.2).",
                BooleanValue(true),
                MakeBooleanAccessor(&Ipv6::SetStrongEndSystemModel, &Ipv6::GetStrongEndSystemModel),
                MakeBooleanChecker());
    return tid;
}

Ipv6::Ipv6()
{
}

Ipv6::~Ipv6()
{
}

} /* namespace ns3 */
