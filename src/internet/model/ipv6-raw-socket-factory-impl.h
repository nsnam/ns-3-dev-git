/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef IPV6_RAW_SOCKET_FACTORY_IMPL_H
#define IPV6_RAW_SOCKET_FACTORY_IMPL_H

#include "ipv6-raw-socket-factory.h"

namespace ns3
{

/**
 * @ingroup socket
 * @ingroup ipv6
 *
 * @brief Implementation of IPv6 raw socket factory.
 */
class Ipv6RawSocketFactoryImpl : public Ipv6RawSocketFactory
{
  public:
    /**
     * @brief Create a raw IPv6 socket.
     * @returns A new RAW IPv6 socket.
     */
    Ptr<Socket> CreateSocket() override;
};

} /* namespace ns3 */

#endif /* IPV6_RAW_SOCKET_FACTORY_IMPL_H */
