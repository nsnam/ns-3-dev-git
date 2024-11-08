/*
 * Copyright (c) 2007 Georgia Tech Research Corporation
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Raj Bhattacharjea <raj.b@gatech.edu>
 */
#ifndef TCP_SOCKET_FACTORY_H
#define TCP_SOCKET_FACTORY_H

#include "ns3/socket-factory.h"

namespace ns3
{

class Socket;

/**
 * @ingroup socket
 * @ingroup tcp
 *
 * @brief API to create TCP socket instances
 *
 * This abstract class defines the API for TCP sockets.
 * This class also holds the global default variables used to
 * initialize newly created sockets, such as values that are
 * set through the sysctl or proc interfaces in Linux.

 * All TCP socket factory implementations must provide an implementation
 * of CreateSocket
 * below, and should make use of the default values configured below.
 *
 * @see TcpSocketFactoryImpl
 *
 */
class TcpSocketFactory : public SocketFactory
{
  public:
    /**
     * Get the type ID.
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
};

} // namespace ns3

#endif /* TCP_SOCKET_FACTORY_H */
