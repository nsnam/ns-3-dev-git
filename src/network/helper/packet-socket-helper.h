/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef PACKET_SOCKET_HELPER_H
#define PACKET_SOCKET_HELPER_H

#include "node-container.h"

namespace ns3
{

/**
 * @brief Give ns3::PacketSocket powers to ns3::Node.
 */
class PacketSocketHelper
{
  public:
    /**
     * Aggregate an instance of a ns3::PacketSocketFactory onto the provided
     * node.
     *
     * @param node Node on which to aggregate the ns3::PacketSocketFactory.
     */
    void Install(Ptr<Node> node) const;

    /**
     * Aggregate an instance of a ns3::PacketSocketFactory onto the provided
     * node.
     *
     * @param nodeName The name of the node on which to aggregate the ns3::PacketSocketFactory.
     */
    void Install(std::string nodeName) const;

    /**
     * For each node in the provided container, aggregate an instance of a
     * ns3::PacketSocketFactory.
     *
     * @param c NodeContainer of the set of nodes to aggregate the
     * ns3::PacketSocketFactory on.
     */
    void Install(NodeContainer c) const;
};

} // namespace ns3

#endif /* PACKET_SOCKET_HELPER_H */
