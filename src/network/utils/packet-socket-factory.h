/*
 * Copyright (c) 2007 Emmanuelle Laprise
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Emmanuelle Laprise <emmanuelle.laprise@bluekazoo.ca>
 */
#ifndef PACKET_SOCKET_FACTORY_H
#define PACKET_SOCKET_FACTORY_H

#include "ns3/socket-factory.h"

namespace ns3
{

class Socket;

/**
 * @ingroup socket
 *
 * This can be used as an interface in a node in order for the node to
 * generate PacketSockets that can connect to net devices.
 */
class PacketSocketFactory : public SocketFactory
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    PacketSocketFactory();

    /**
     * Creates a PacketSocket and returns a pointer to it.
     *
     * @return a pointer to the created socket
     */
    Ptr<Socket> CreateSocket() override;
};

} // namespace ns3

#endif /* PACKET_SOCKET_FACTORY_H */
