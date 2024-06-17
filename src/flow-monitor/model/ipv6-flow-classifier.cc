//
// Copyright (c) 2009 INESC Porto
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Gustavo J. A. M. Carneiro  <gjc@inescporto.pt> <gjcarneiro@gmail.com>
// Modifications: Tommaso Pecorella <tommaso.pecorella@unifi.it>
//

#include "ipv6-flow-classifier.h"

#include "ns3/packet.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"

#include <algorithm>

namespace ns3
{

/* see http://www.iana.org/assignments/protocol-numbers */
const uint8_t TCP_PROT_NUMBER = 6;  //!< TCP Protocol number
const uint8_t UDP_PROT_NUMBER = 17; //!< UDP Protocol number

bool
operator<(const Ipv6FlowClassifier::FiveTuple& t1, const Ipv6FlowClassifier::FiveTuple& t2)
{
    if (t1.sourceAddress < t2.sourceAddress)
    {
        return true;
    }
    if (t1.sourceAddress != t2.sourceAddress)
    {
        return false;
    }

    if (t1.destinationAddress < t2.destinationAddress)
    {
        return true;
    }
    if (t1.destinationAddress != t2.destinationAddress)
    {
        return false;
    }

    if (t1.protocol < t2.protocol)
    {
        return true;
    }
    if (t1.protocol != t2.protocol)
    {
        return false;
    }

    if (t1.sourcePort < t2.sourcePort)
    {
        return true;
    }
    if (t1.sourcePort != t2.sourcePort)
    {
        return false;
    }

    if (t1.destinationPort < t2.destinationPort)
    {
        return true;
    }
    if (t1.destinationPort != t2.destinationPort)
    {
        return false;
    }

    return false;
}

bool
operator==(const Ipv6FlowClassifier::FiveTuple& t1, const Ipv6FlowClassifier::FiveTuple& t2)
{
    return (t1.sourceAddress == t2.sourceAddress &&
            t1.destinationAddress == t2.destinationAddress && t1.protocol == t2.protocol &&
            t1.sourcePort == t2.sourcePort && t1.destinationPort == t2.destinationPort);
}

Ipv6FlowClassifier::Ipv6FlowClassifier()
{
}

bool
Ipv6FlowClassifier::Classify(const Ipv6Header& ipHeader,
                             Ptr<const Packet> ipPayload,
                             uint32_t* out_flowId,
                             uint32_t* out_packetId)
{
    if (ipHeader.GetDestination().IsMulticast())
    {
        // we are not prepared to handle multicast yet
        return false;
    }

    FiveTuple tuple;
    tuple.sourceAddress = ipHeader.GetSource();
    tuple.destinationAddress = ipHeader.GetDestination();
    tuple.protocol = ipHeader.GetNextHeader();

    if ((tuple.protocol != UDP_PROT_NUMBER) && (tuple.protocol != TCP_PROT_NUMBER))
    {
        return false;
    }

    if (ipPayload->GetSize() < 4)
    {
        // the packet doesn't carry enough bytes
        return false;
    }

    // we rely on the fact that for both TCP and UDP the ports are
    // carried in the first 4 octets.
    // This allows to read the ports even on fragmented packets
    // not carrying a full TCP or UDP header.

    uint8_t data[4];
    ipPayload->CopyData(data, 4);

    uint16_t srcPort = 0;
    srcPort |= data[0];
    srcPort <<= 8;
    srcPort |= data[1];

    uint16_t dstPort = 0;
    dstPort |= data[2];
    dstPort <<= 8;
    dstPort |= data[3];

    tuple.sourcePort = srcPort;
    tuple.destinationPort = dstPort;

    // try to insert the tuple, but check if it already exists
    auto insert = m_flowMap.insert(std::pair<FiveTuple, FlowId>(tuple, 0));

    // if the insertion succeeded, we need to assign this tuple a new flow identifier
    if (insert.second)
    {
        FlowId newFlowId = GetNewFlowId();
        insert.first->second = newFlowId;
        m_flowPktIdMap[newFlowId] = 0;
        m_flowDscpMap[newFlowId];
    }
    else
    {
        m_flowPktIdMap[insert.first->second]++;
    }

    // increment the counter of packets with the same DSCP value
    Ipv6Header::DscpType dscp = ipHeader.GetDscp();
    auto dscpInserter = m_flowDscpMap[insert.first->second].insert(
        std::pair<Ipv6Header::DscpType, uint32_t>(dscp, 1));

    // if the insertion did not succeed, we need to increment the counter
    if (!dscpInserter.second)
    {
        m_flowDscpMap[insert.first->second][dscp]++;
    }

    *out_flowId = insert.first->second;
    *out_packetId = m_flowPktIdMap[*out_flowId];

    return true;
}

Ipv6FlowClassifier::FiveTuple
Ipv6FlowClassifier::FindFlow(FlowId flowId) const
{
    for (auto iter = m_flowMap.begin(); iter != m_flowMap.end(); iter++)
    {
        if (iter->second == flowId)
        {
            return iter->first;
        }
    }
    NS_FATAL_ERROR("Could not find the flow with ID " << flowId);
    FiveTuple retval = {Ipv6Address::GetZero(), Ipv6Address::GetZero(), 0, 0, 0};
    return retval;
}

bool
Ipv6FlowClassifier::SortByCount::operator()(std::pair<Ipv6Header::DscpType, uint32_t> left,
                                            std::pair<Ipv6Header::DscpType, uint32_t> right)
{
    return left.second > right.second;
}

std::vector<std::pair<Ipv6Header::DscpType, uint32_t>>
Ipv6FlowClassifier::GetDscpCounts(FlowId flowId) const
{
    auto flow = m_flowDscpMap.find(flowId);

    if (flow == m_flowDscpMap.end())
    {
        NS_FATAL_ERROR("Could not find the flow with ID " << flowId);
    }

    std::vector<std::pair<Ipv6Header::DscpType, uint32_t>> v(flow->second.begin(),
                                                             flow->second.end());
    std::sort(v.begin(), v.end(), SortByCount());
    return v;
}

void
Ipv6FlowClassifier::SerializeToXmlStream(std::ostream& os, uint16_t indent) const
{
    Indent(os, indent);
    os << "<Ipv6FlowClassifier>\n";

    indent += 2;
    for (auto iter = m_flowMap.begin(); iter != m_flowMap.end(); iter++)
    {
        Indent(os, indent);
        os << "<Flow flowId=\"" << iter->second << "\""
           << " sourceAddress=\"" << iter->first.sourceAddress << "\""
           << " destinationAddress=\"" << iter->first.destinationAddress << "\""
           << " protocol=\"" << int(iter->first.protocol) << "\""
           << " sourcePort=\"" << iter->first.sourcePort << "\""
           << " destinationPort=\"" << iter->first.destinationPort << "\">\n";

        indent += 2;
        auto flow = m_flowDscpMap.find(iter->second);

        if (flow != m_flowDscpMap.end())
        {
            for (auto i = flow->second.begin(); i != flow->second.end(); i++)
            {
                Indent(os, indent);
                os << "<Dscp value=\"0x" << std::hex << static_cast<uint32_t>(i->first) << "\""
                   << " packets=\"" << std::dec << i->second << "\" />\n";
            }
        }

        indent -= 2;
        Indent(os, indent);
        os << "</Flow>\n";
    }

    indent -= 2;
    Indent(os, indent);
    os << "</Ipv6FlowClassifier>\n";
}

} // namespace ns3
