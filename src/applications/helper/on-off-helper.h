/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef ON_OFF_HELPER_H
#define ON_OFF_HELPER_H

#include "ns3/application-helper.h"
#include "ns3/data-rate.h"

#include <stdint.h>

namespace ns3
{

/**
 * @ingroup onoff
 * @brief A helper to make it easier to instantiate an ns3::OnOffApplication
 * on a set of nodes.
 */
class OnOffHelper : public ApplicationHelper
{
  public:
    /**
     * Create an OnOffHelper to make it easier to work with OnOffApplications
     *
     * @param protocol the name of the protocol to use to send traffic
     *        by the applications. This string identifies the socket
     *        factory type used to create sockets for the applications.
     *        A typical value would be ns3::UdpSocketFactory.
     * @param address the address of the remote node to send traffic
     *        to.
     */
    OnOffHelper(const std::string& protocol, const Address& address);

    /**
     * Helper function to set a constant rate source.  Equivalent to
     * setting the attributes OnTime to constant 1000 seconds, OffTime to
     * constant 0 seconds, and the DataRate and PacketSize set accordingly
     *
     * @param dataRate DataRate object for the sending rate
     * @param packetSize size in bytes of the packet payloads generated
     */
    void SetConstantRate(DataRate dataRate, uint32_t packetSize = 512);
};

} // namespace ns3

#endif /* ON_OFF_HELPER_H */
