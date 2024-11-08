/*
 * Copyright (c) 2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef IPV4_RAW_SOCKET_FACTORY_H
#define IPV4_RAW_SOCKET_FACTORY_H

#include "ns3/socket-factory.h"

namespace ns3
{

class Socket;

/**
 * @ingroup socket
 * @ingroup ipv4
 *
 * @brief API to create RAW socket instances
 *
 * This abstract class defines the API for RAW socket factory.
 *
 */
class Ipv4RawSocketFactory : public SocketFactory
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
};

} // namespace ns3

#endif /* IPV4_RAW_SOCKET_FACTORY_H */
