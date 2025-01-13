/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Geoge Riley <riley@ece.gatech.edu>
 * Adapted from OnOffHelper by:
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef BULK_SEND_HELPER_H
#define BULK_SEND_HELPER_H

#include "ns3/application-helper.h"

namespace ns3
{

/**
 * @ingroup bulksend
 * @brief A helper to make it easier to instantiate an ns3::BulkSendApplication
 * on a set of nodes.
 */
class BulkSendHelper : public ApplicationHelper
{
  public:
    /**
     * Create an BulkSendHelper to make it easier to work with BulkSendApplications
     *
     * @param protocol the name of the protocol to use to send traffic
     *        by the applications. This string identifies the socket
     *        factory type used to create sockets for the applications.
     *        A typical value would be ns3::TcpSocketFactory.
     * @param address the address of the remote node to send traffic
     *        to.
     */
    BulkSendHelper(const std::string& protocol, const Address& address);
};

} // namespace ns3

#endif /* BULK_SEND_HELPER_H */
