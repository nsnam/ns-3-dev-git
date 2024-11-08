/*
 * Copyright (c) 2010 Universita' di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella (tommaso.pecorella@unifi.it)
 * Author: Valerio Sartini (Valesar@gmail.com)
 */

#include "inet-topology-reader.h"

#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/node-container.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

/**
 * @file
 * @ingroup topology
 * ns3::InetTopologyReader implementation.
 */

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("InetTopologyReader");

NS_OBJECT_ENSURE_REGISTERED(InetTopologyReader);

TypeId
InetTopologyReader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::InetTopologyReader")
                            .SetParent<TopologyReader>()
                            .SetGroupName("TopologyReader")
                            .AddConstructor<InetTopologyReader>();
    return tid;
}

InetTopologyReader::InetTopologyReader()
{
    NS_LOG_FUNCTION(this);
}

InetTopologyReader::~InetTopologyReader()
{
    NS_LOG_FUNCTION(this);
}

NodeContainer
InetTopologyReader::Read()
{
    std::ifstream topgen;
    topgen.open(GetFileName());
    std::map<std::string, Ptr<Node>> nodeMap;
    NodeContainer nodes;

    if (!topgen.is_open())
    {
        NS_LOG_WARN("Inet topology file object is not open, check file name and permissions");
        return nodes;
    }

    std::string from;
    std::string to;
    std::string linkAttr;

    int linksNumber = 0;
    int nodesNumber = 0;

    int totnode = 0;
    int totlink = 0;

    std::istringstream lineBuffer;
    std::string line;

    getline(topgen, line);
    lineBuffer.str(line);

    lineBuffer >> totnode;
    lineBuffer >> totlink;
    NS_LOG_INFO("Inet topology should have " << totnode << " nodes and " << totlink << " links");

    for (int i = 0; i < totnode && !topgen.eof(); i++)
    {
        getline(topgen, line);
    }

    for (int i = 0; i < totlink && !topgen.eof(); i++)
    {
        getline(topgen, line);
        lineBuffer.clear();
        lineBuffer.str(line);

        lineBuffer >> from;
        lineBuffer >> to;
        lineBuffer >> linkAttr;

        if ((!from.empty()) && (!to.empty()))
        {
            NS_LOG_INFO("Link " << linksNumber << " from: " << from << " to: " << to);

            if (!nodeMap[from])
            {
                NS_LOG_INFO("Node " << nodesNumber << " name: " << from);
                Ptr<Node> tmpNode = CreateObject<Node>();
                std::string nodeName = "InetTopology/NodeName/" + from;
                Names::Add(from, tmpNode);
                nodeMap[from] = tmpNode;
                nodes.Add(tmpNode);
                nodesNumber++;
            }

            if (!nodeMap[to])
            {
                NS_LOG_INFO("Node " << nodesNumber << " name: " << to);
                Ptr<Node> tmpNode = CreateObject<Node>();
                std::string nodename = "InetTopology/NodeName/" + to;
                Names::Add(nodename, tmpNode);
                nodeMap[to] = tmpNode;
                nodes.Add(tmpNode);
                nodesNumber++;
            }

            Link link(nodeMap[from], from, nodeMap[to], to);
            if (!linkAttr.empty())
            {
                NS_LOG_INFO("Link " << linksNumber << " weight: " << linkAttr);
                link.SetAttribute("Weight", linkAttr);
            }
            AddLink(link);

            linksNumber++;
        }
    }

    NS_LOG_INFO("Inet topology created with " << nodesNumber << " nodes and " << linksNumber
                                              << " links");
    topgen.close();

    return nodes;
}

} /* namespace ns3 */
