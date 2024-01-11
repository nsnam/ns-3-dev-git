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

#include "udp-echo-helper.h"

#include "ns3/udp-echo-client.h"
#include "ns3/udp-echo-server.h"
#include "ns3/uinteger.h"

namespace ns3
{

UdpEchoServerHelper::UdpEchoServerHelper(uint16_t port)
    : ApplicationHelper(UdpEchoServer::GetTypeId())
{
    SetAttribute("Port", UintegerValue(port));
}

UdpEchoClientHelper::UdpEchoClientHelper(const Address& address, uint16_t port)
    : ApplicationHelper(UdpEchoClient::GetTypeId())
{
    SetAttribute("RemoteAddress", AddressValue(address));
    SetAttribute("RemotePort", UintegerValue(port));
}

UdpEchoClientHelper::UdpEchoClientHelper(const Address& address)
    : ApplicationHelper(UdpEchoClient::GetTypeId())
{
    SetAttribute("RemoteAddress", AddressValue(address));
}

void
UdpEchoClientHelper::SetFill(Ptr<Application> app, const std::string& fill)
{
    app->GetObject<UdpEchoClient>()->SetFill(fill);
}

void
UdpEchoClientHelper::SetFill(Ptr<Application> app, uint8_t fill, uint32_t dataLength)
{
    app->GetObject<UdpEchoClient>()->SetFill(fill, dataLength);
}

void
UdpEchoClientHelper::SetFill(Ptr<Application> app,
                             uint8_t* fill,
                             uint32_t fillLength,
                             uint32_t dataLength)
{
    app->GetObject<UdpEchoClient>()->SetFill(fill, fillLength, dataLength);
}

} // namespace ns3
