/*
 *  Copyright (c) 2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *         Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *
 */

#include "ipcs-classifier.h"

#include "service-flow.h"

#include "ns3/ipv4-header.h"
#include "ns3/llc-snap-header.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/tcp-header.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/udp-header.h"
#include "ns3/udp-l4-protocol.h"

#include <stdint.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("IpcsClassifier");

NS_OBJECT_ENSURE_REGISTERED(IpcsClassifier);

TypeId
IpcsClassifier::GetTypeId()
{
    static TypeId tid = TypeId("ns3::IpcsClassifier").SetParent<Object>().SetGroupName("Wimax");
    return tid;
}

IpcsClassifier::IpcsClassifier()
{
}

IpcsClassifier::~IpcsClassifier()
{
}

ServiceFlow*
IpcsClassifier::Classify(Ptr<const Packet> packet,
                         Ptr<ServiceFlowManager> sfm,
                         ServiceFlow::Direction dir)
{
    Ptr<Packet> C_Packet = packet->Copy();

    LlcSnapHeader llc;
    C_Packet->RemoveHeader(llc);

    Ipv4Header ipv4Header;
    C_Packet->RemoveHeader(ipv4Header);
    Ipv4Address source_address = ipv4Header.GetSource();
    Ipv4Address dest_address = ipv4Header.GetDestination();
    uint8_t protocol = ipv4Header.GetProtocol();

    uint16_t sourcePort = 0;
    uint16_t destPort = 0;
    if (protocol == UdpL4Protocol::PROT_NUMBER)
    {
        UdpHeader udpHeader;
        C_Packet->RemoveHeader(udpHeader);
        sourcePort = udpHeader.GetSourcePort();
        destPort = udpHeader.GetDestinationPort();
    }
    else if (protocol == TcpL4Protocol::PROT_NUMBER)
    {
        TcpHeader tcpHeader;
        C_Packet->RemoveHeader(tcpHeader);
        sourcePort = tcpHeader.GetSourcePort();
        destPort = tcpHeader.GetDestinationPort();
    }
    else
    {
        NS_LOG_INFO("\t\t\tUnknown protocol: " << protocol);
        return nullptr;
    }

    NS_LOG_INFO("Classifing packet: src_addr=" << source_address << " dst_addr=" << dest_address
                                               << " src_port=" << sourcePort << " dst_port="
                                               << destPort << " proto=" << (uint16_t)protocol);
    return (sfm->DoClassify(source_address, dest_address, sourcePort, destPort, protocol, dir));
}

} // namespace ns3
