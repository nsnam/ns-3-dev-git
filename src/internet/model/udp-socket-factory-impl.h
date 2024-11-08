/*
 * Copyright (c) 2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef UDP_SOCKET_FACTORY_IMPL_H
#define UDP_SOCKET_FACTORY_IMPL_H

#include "udp-socket-factory.h"

#include "ns3/ptr.h"

namespace ns3
{

class UdpL4Protocol;

/**
 * @ingroup socket
 * @ingroup udp
 *
 * @brief Object to create UDP socket instances
 *
 * This class implements the API for creating UDP sockets.
 * It is a socket factory (deriving from class SocketFactory).
 */
class UdpSocketFactoryImpl : public UdpSocketFactory
{
  public:
    UdpSocketFactoryImpl();
    ~UdpSocketFactoryImpl() override;

    /**
     * @brief Set the associated UDP L4 protocol.
     * @param udp the UDP L4 protocol
     */
    void SetUdp(Ptr<UdpL4Protocol> udp);

    /**
     * @brief Implements a method to create a Udp-based socket and return
     * a base class smart pointer to the socket.
     *
     * @return smart pointer to Socket
     */
    Ptr<Socket> CreateSocket() override;

  protected:
    void DoDispose() override;

  private:
    Ptr<UdpL4Protocol> m_udp; //!< the associated UDP L4 protocol
};

} // namespace ns3

#endif /* UDP_SOCKET_FACTORY_IMPL_H */
