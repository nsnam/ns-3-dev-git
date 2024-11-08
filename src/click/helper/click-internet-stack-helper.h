/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Author: Lalith Suresh  <suresh.lalith@gmail.com>
 */

#ifndef CLICK_INTERNET_STACK_HELPER_H
#define CLICK_INTERNET_STACK_HELPER_H

#include "ns3/internet-trace-helper.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"

#include <map>

namespace ns3
{

class Node;
class Ipv4RoutingHelper;

/**
 * @brief aggregate Click/IP/TCP/UDP functionality to existing Nodes.
 *
 * This helper has been adapted from the InternetStackHelper class and
 * nodes will not be able to use Ipv6 functionalities.
 *
 */
class ClickInternetStackHelper : public PcapHelperForIpv4, public AsciiTraceHelperForIpv4
{
  public:
    /**
     * Create a new ClickInternetStackHelper which uses Ipv4ClickRouting for routing
     */
    ClickInternetStackHelper();

    /**
     * Destroy the ClickInternetStackHelper
     */
    ~ClickInternetStackHelper() override;

    /**
     * Copy constructor.
     *
     * @param o Object to copy from.
     */
    ClickInternetStackHelper(const ClickInternetStackHelper& o);

    /**
     * Assignment operator.
     *
     * @param o Object to copy from.
     * @return Reference to updated object.
     */
    ClickInternetStackHelper& operator=(const ClickInternetStackHelper& o);

    /**
     * Return helper internal state to that of a newly constructed one
     */
    void Reset();

    /**
     * Aggregate implementations of the ns3::Ipv4L3ClickProtocol, ns3::ArpL3Protocol,
     * ns3::Udp, and ns3::Tcp classes onto the provided node.  This method will
     * assert if called on a node that already has an Ipv4 object aggregated to it.
     *
     * @param nodeName The name of the node on which to install the stack.
     */
    void Install(std::string nodeName) const;

    /**
     * Aggregate implementations of the ns3::Ipv4L3ClickProtocol, ns3::ArpL3Protocol,
     * ns3::Udp, and ns3::Tcp classes onto the provided node.  This method will
     * assert if called on a node that already has an Ipv4 object aggregated to it.
     *
     * @param node The node on which to install the stack.
     */
    void Install(Ptr<Node> node) const;

    /**
     * For each node in the input container, aggregate implementations of the
     * ns3::Ipv4L3ClickProtocol, ns3::ArpL3Protocol, ns3::Udp, and, ns3::Tcp classes.
     * The program will assert if this method is called on a container with a
     * node that already has an Ipv4 object aggregated to it.
     *
     * @param c NodeContainer that holds the set of nodes on which to install the
     * new stacks.
     */
    void Install(NodeContainer c) const;

    /**
     * Aggregate IPv4, UDP, and TCP stacks to all nodes in the simulation
     */
    void InstallAll() const;

    /**
     * @brief Set a Click file to be used for a group of nodes.
     * @param c NodeContainer of nodes
     * @param clickfile Click file to be used
     */
    void SetClickFile(NodeContainer c, std::string clickfile);

    /**
     * @brief Set a Click file to be used for a node.
     * @param node Node for which Click file is to be set
     * @param clickfile Click file to be used
     */
    void SetClickFile(Ptr<Node> node, std::string clickfile);

    /**
     * @brief Set defines to be used for a group of nodes.
     * @param c NodeContainer of nodes
     * @param defines Defines mapping to be used
     */
    void SetDefines(NodeContainer c, std::map<std::string, std::string> defines);

    /**
     * @brief Set defines to be used for a node.
     * @param node Node for which the defines are to be set
     * @param defines Defines mapping to be used
     */
    void SetDefines(Ptr<Node> node, std::map<std::string, std::string> defines);

    /**
     * @brief Set a Click routing table element for a group of nodes.
     * @param c NodeContainer of nodes
     * @param rt Click Routing Table element name
     */
    void SetRoutingTableElement(NodeContainer c, std::string rt);

    /**
     * @brief Set a Click routing table element for a node.
     * @param node Node for which Click file is to be set
     * @param rt Click Routing Table element name
     */
    void SetRoutingTableElement(Ptr<Node> node, std::string rt);

  private:
    /**
     * @brief Enable pcap output the indicated Ipv4 and interface pair.
     *
     * @param prefix Filename prefix to use for pcap files.
     * @param ipv4 Ptr to the Ipv4 interface on which you want to enable tracing.
     * @param interface Interface ID on the Ipv4 on which you want to enable tracing.
     * @param explicitFilename Whether the filename is explicit or not.
     */
    void EnablePcapIpv4Internal(std::string prefix,
                                Ptr<Ipv4> ipv4,
                                uint32_t interface,
                                bool explicitFilename) override;

    /**
     * @brief Enable ascii trace output on the indicated Ipv4 and interface pair.
     *
     * @param stream An OutputStreamWrapper representing an existing file to use
     *               when writing trace data.
     * @param prefix Filename prefix to use for ascii trace files.
     * @param ipv4 Ptr to the Ipv4 interface on which you want to enable tracing.
     * @param interface Interface ID on the Ipv4 on which you want to enable tracing.
     * @param explicitFilename Whether the filename is explicit or not.
     */
    void EnableAsciiIpv4Internal(Ptr<OutputStreamWrapper> stream,
                                 std::string prefix,
                                 Ptr<Ipv4> ipv4,
                                 uint32_t interface,
                                 bool explicitFilename) override;

    /**
     * Initialize stack helper.
     * Called by both constructor and Reset().
     */
    void Initialize();

    /**
     * Create and aggregate object from type ID.
     *
     * @param node Node.
     * @param typeId Type ID.
     */
    static void CreateAndAggregateObjectFromTypeId(Ptr<Node> node, const std::string typeId);

    /**
     * Check if PCAP is hooked.
     *
     * @param ipv4 IPv4 stack.
     * @return True if PCAP is hooked.
     */
    bool PcapHooked(Ptr<Ipv4> ipv4);

    /**
     * Check if ASCII is hooked.
     *
     * @param ipv4 IPv4 stack.
     * @return True if ASCII is hooked.
     */
    bool AsciiHooked(Ptr<Ipv4> ipv4);

    /**
     * @brief IPv4 install state (enabled/disabled) ?
     */
    bool m_ipv4Enabled;

    /**
     * @brief Node to Click file mapping
     */
    std::map<Ptr<Node>, std::string> m_nodeToClickFileMap;

    /**
     * @brief Node to Click defines mapping
     */
    std::map<Ptr<Node>, std::map<std::string, std::string>> m_nodeToDefinesMap;

    /**
     * @brief Node to Routing Table Element mapping
     */
    std::map<Ptr<Node>, std::string> m_nodeToRoutingTableElementMap;
};

} // namespace ns3

#endif /* CLICK_INTERNET_STACK_HELPER_H */
