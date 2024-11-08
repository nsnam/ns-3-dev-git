/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef IPV4_RAW_SOCKET_FACTORY_IMPL_H
#define IPV4_RAW_SOCKET_FACTORY_IMPL_H

#include "ipv4-raw-socket-factory.h"

namespace ns3
{

/**
 * @ingroup socket
 * @ingroup ipv4
 *
 * @brief Implementation of IPv4 raw socket factory.
 */
class Ipv4RawSocketFactoryImpl : public Ipv4RawSocketFactory
{
  public:
    Ptr<Socket> CreateSocket() override;
};

} // namespace ns3

#endif /* IPV4_RAW_SOCKET_FACTORY_IMPL_H */
