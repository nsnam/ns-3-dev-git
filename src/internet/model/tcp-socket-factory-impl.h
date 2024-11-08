/*
 * Copyright (c) 2007 Georgia Tech Research Corporation
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Raj Bhattacharjea <raj.b@gatech.edu>
 */
#ifndef TCP_SOCKET_FACTORY_IMPL_H
#define TCP_SOCKET_FACTORY_IMPL_H

#include "tcp-socket-factory.h"

#include "ns3/ptr.h"

namespace ns3
{

class TcpL4Protocol;

/**
 * @ingroup socket
 * @ingroup tcp
 *
 * @brief socket factory implementation for native ns-3 TCP
 *
 *
 * This class serves to create sockets of the TcpSocketBase type.
 */
class TcpSocketFactoryImpl : public TcpSocketFactory
{
  public:
    TcpSocketFactoryImpl();
    ~TcpSocketFactoryImpl() override;

    /**
     * @brief Set the associated TCP L4 protocol.
     * @param tcp the TCP L4 protocol
     */
    void SetTcp(Ptr<TcpL4Protocol> tcp);

    Ptr<Socket> CreateSocket() override;

  protected:
    void DoDispose() override;

  private:
    Ptr<TcpL4Protocol> m_tcp; //!< the associated TCP L4 protocol
};

} // namespace ns3

#endif /* TCP_SOCKET_FACTORY_IMPL_H */
