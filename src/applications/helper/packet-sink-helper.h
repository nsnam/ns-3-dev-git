/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef PACKET_SINK_HELPER_H
#define PACKET_SINK_HELPER_H

#include "ns3/application-helper.h"

namespace ns3
{

/**
 * @ingroup packetsink
 * @brief A helper to make it easier to instantiate an ns3::PacketSinkApplication
 * on a set of nodes.
 */
class PacketSinkHelper : public ApplicationHelper
{
  public:
    /**
     * Create a PacketSinkHelper to make it easier to work with PacketSinkApplications
     *
     * @param protocol the name of the protocol to use to receive traffic
     *        This string identifies the socket factory type used to create
     *        sockets for the applications.  A typical value would be
     *        ns3::TcpSocketFactory.
     * @param address the address of the sink,
     *
     */
    PacketSinkHelper(const std::string& protocol, const Address& address);
};

} // namespace ns3

#endif /* PACKET_SINK_HELPER_H */
