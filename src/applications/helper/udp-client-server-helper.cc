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
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */

#include "udp-client-server-helper.h"

#include <ns3/string.h>
#include <ns3/uinteger.h>

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

UdpClientHelper::UdpClientHelper()
    : ApplicationHelper(UdpClient::GetTypeId())
{
}

UdpClientHelper::UdpClientHelper(const Address& address)
    : UdpClientHelper()
{
    SetAttribute("RemoteAddress", AddressValue(address));
}

UdpClientHelper::UdpClientHelper(const Address& address, uint16_t port)
    : UdpClientHelper(address)
{
    SetAttribute("RemotePort", UintegerValue(port));
}

UdpTraceClientHelper::UdpTraceClientHelper()
    : ApplicationHelper(UdpTraceClient::GetTypeId())
{
}

UdpTraceClientHelper::UdpTraceClientHelper(const Address& address, const std::string& filename)
    : UdpTraceClientHelper()
{
    SetAttribute("RemoteAddress", AddressValue(address));
    SetAttribute("TraceFilename", StringValue(filename));
}

UdpTraceClientHelper::UdpTraceClientHelper(const Address& address,
                                           uint16_t port,
                                           const std::string& filename)
    : UdpTraceClientHelper(address, filename)
{
    SetAttribute("RemotePort", UintegerValue(port));
}

} // namespace ns3
