/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */

#include "udp-client-server-helper.h"

#include "ns3/address-utils.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

namespace ns3
{

UdpServerHelper::UdpServerHelper()
    : ApplicationHelper(UdpServer::GetTypeId())
{
}

UdpServerHelper::UdpServerHelper(uint16_t port)
    : UdpServerHelper()
{
    SetAttribute("Port", UintegerValue(port));
}

UdpServerHelper::UdpServerHelper(const Address& address)
    : UdpServerHelper()
{
    SetAttribute("Local", AddressValue(address));
}

UdpClientHelper::UdpClientHelper()
    : ApplicationHelper(UdpClient::GetTypeId())
{
}

UdpClientHelper::UdpClientHelper(const Address& address)
    : UdpClientHelper()
{
    SetAttribute("Remote", AddressValue(address));
}

UdpClientHelper::UdpClientHelper(const Address& address, uint16_t port)
    : UdpClientHelper(addressUtils::ConvertToSocketAddress(address, port))
{
}

UdpTraceClientHelper::UdpTraceClientHelper()
    : ApplicationHelper(UdpTraceClient::GetTypeId())
{
}

UdpTraceClientHelper::UdpTraceClientHelper(const Address& address, const std::string& filename)
    : UdpTraceClientHelper()
{
    SetAttribute("Remote", AddressValue(address));
    SetAttribute("TraceFilename", StringValue(filename));
}

UdpTraceClientHelper::UdpTraceClientHelper(const Address& address,
                                           uint16_t port,
                                           const std::string& filename)
    : UdpTraceClientHelper(addressUtils::ConvertToSocketAddress(address, port), filename)
{
}

} // namespace ns3
