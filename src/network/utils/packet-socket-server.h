/*
 * Copyright (c) 2014 Universita' di Firenze
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#ifndef PACKET_SOCKET_SERVER_H
#define PACKET_SOCKET_SERVER_H

#include "packet-socket-address.h"

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"

namespace ns3
{

class Socket;
class Packet;

/**
 * @ingroup socket
 *
 * @brief A server using PacketSocket.
 *
 * Receives packets using PacketSocket. It does not require (or use) IP.
 * The application has the same requirements as the PacketSocket for
 * what concerns the underlying NetDevice and the Address scheme.
 * It is meant to be used in ns-3 tests.
 *
 * Provides a "Rx" Traced Callback (received packets, source address)
 */
class PacketSocketServer : public Application
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    PacketSocketServer();

    ~PacketSocketServer() override;

    /**
     * @brief set the local address and protocol to be used
     * @param addr local address
     */
    void SetLocal(PacketSocketAddress addr);

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * @brief Handle a packet received by the application
     * @param socket the receiving socket
     */
    void HandleRead(Ptr<Socket> socket);

    uint32_t m_pktRx;   //!< The number of received packets
    uint32_t m_bytesRx; //!< Total bytes received

    Ptr<Socket> m_socket;               //!< Socket
    PacketSocketAddress m_localAddress; //!< Local address
    bool m_localAddressSet;             //!< Sanity check

    /// Traced Callback: received packets, source address.
    TracedCallback<Ptr<const Packet>, const Address&> m_rxTrace;
};

} // namespace ns3

#endif /* PACKET_SOCKET_SERVER_H */
