/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "udp-echo-helper.h"

#include "ns3/address-utils.h"
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

UdpEchoServerHelper::UdpEchoServerHelper(const Address& address)
    : ApplicationHelper(UdpEchoServer::GetTypeId())
{
    SetAttribute("Local", AddressValue(address));
}

UdpEchoClientHelper::UdpEchoClientHelper(const Address& address, uint16_t port)
    : UdpEchoClientHelper(addressUtils::ConvertToSocketAddress(address, port))
{
}

UdpEchoClientHelper::UdpEchoClientHelper(const Address& address)
    : ApplicationHelper(UdpEchoClient::GetTypeId())
{
    SetAttribute("Remote", AddressValue(address));
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
