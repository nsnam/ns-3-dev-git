/*
 * Copyright (c) 2014 Universita' di Firenze
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#ifndef PACKET_SOCKET_CLIENT_H
#define PACKET_SOCKET_CLIENT_H

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
 * @brief A simple client.
 *
 * Sends packets using PacketSocket. It does not require (or use) IP.
 *
 * Packets are sent as soon as PacketSocketClient::Start is called.
 * The application has the same requirements as the PacketSocket for
 * what concerns the underlying NetDevice and the Address scheme.
 * It is meant to be used in ns-3 tests.
 *
 * The application will send `MaxPackets' packets, one every `Interval'
 * time. Packet size (`PacketSize') can be configured.
 * Provides a "Tx" Traced Callback (transmitted packets, source address).
 *
 * Note: packets larger than the NetDevice MTU will not be sent.
 */
class PacketSocketClient : public Application
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    PacketSocketClient();

    ~PacketSocketClient() override;

    /**
     * @brief set the remote address and protocol to be used
     * @param addr remote address
     */
    void SetRemote(PacketSocketAddress addr);

    /**
     * @brief Query the priority value of this socket
     * @return The priority value
     */
    uint8_t GetPriority() const;

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * @brief Manually set the socket priority
     * @param priority The socket priority (in the range 0..6)
     */
    void SetPriority(uint8_t priority);

    /**
     * @brief Send a packet
     *
     * Either <i>Interval</i> and <i>MaxPackets</i> may be zero, but not both.  If <i>Interval</i>
     * is zero, the PacketSocketClient will send <i>MaxPackets</i> packets without any delay into
     * the socket.  If <i>MaxPackets</i> is zero, then the PacketSocketClient will send every
     * <i>Interval</i> until the application is stopped.
     */
    void Send();

    uint32_t m_maxPackets; //!< Maximum number of packets the application will send
    Time m_interval;       //!< Packet inter-send time
    uint32_t m_size;       //!< Size of the sent packet
    uint8_t m_priority;    //!< Priority of the sent packets

    uint32_t m_sent;                   //!< Counter for sent packets
    Ptr<Socket> m_socket;              //!< Socket
    PacketSocketAddress m_peerAddress; //!< Remote peer address
    bool m_peerAddressSet;             //!< Sanity check
    EventId m_sendEvent;               //!< Event to send the next packet

    /// Traced Callback: sent packets, source address.
    TracedCallback<Ptr<const Packet>, const Address&> m_txTrace;
};

} // namespace ns3

#endif /* PACKET_SOCKET_CLIENT_H */
