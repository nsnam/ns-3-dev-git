/*
 * Copyright (c) 2007 Emmanuelle Laprise
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
 * Author: Emmanuelle Laprise <emmanuelle.laprise@bluekazoo.ca>
 */
#include "packet-socket-factory.h"

#include "packet-socket.h"

#include "ns3/log.h"
#include "ns3/node.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PacketSocketFactory");

NS_OBJECT_ENSURE_REGISTERED(PacketSocketFactory);

TypeId
PacketSocketFactory::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::PacketSocketFactory").SetParent<SocketFactory>().SetGroupName("Network");
    return tid;
}

PacketSocketFactory::PacketSocketFactory()
{
    NS_LOG_FUNCTION(this);
}

Ptr<Socket>
PacketSocketFactory::CreateSocket()
{
    NS_LOG_FUNCTION(this);
    Ptr<Node> node = GetObject<Node>();
    Ptr<PacketSocket> socket = CreateObject<PacketSocket>();
    socket->SetNode(node);
    return socket;
}
} // namespace ns3
