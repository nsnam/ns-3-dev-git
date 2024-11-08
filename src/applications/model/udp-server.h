/*
 * Copyright (c) 2007,2008,2009 INRIA, UDCAST
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
 *
 */

#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include "packet-loss-counter.h"
#include "sink-application.h"

#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"

namespace ns3
{
/**
 * @ingroup applications
 * @defgroup udpclientserver UdpClientServer
 */

/**
 * @ingroup udpclientserver
 *
 * @brief A UDP server, receives UDP packets from a remote host.
 *
 * UDP packets carry a 32bits sequence number followed by a 64bits time
 * stamp in their payloads. The application uses the sequence number
 * to determine if a packet is lost, and the time stamp to compute the delay.
 */
class UdpServer : public SinkApplication
{
  public:
    static constexpr uint16_t DEFAULT_PORT{100}; //!< default port

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    UdpServer();
    ~UdpServer() override;

    /**
     * @brief Returns the number of lost packets
     * @return the number of lost packets
     */
    uint32_t GetLost() const;

    /**
     * @brief Returns the number of received packets
     * @return the number of received packets
     */
    uint64_t GetReceived() const;

    /**
     * @brief Returns the size of the window used for checking loss.
     * @return the size of the window used for checking loss.
     */
    uint16_t GetPacketWindowSize() const;

    /**
     * @brief Set the size of the window used for checking loss. This value should
     *  be a multiple of 8
     * @param size the size of the window used for checking loss. This value should
     *  be a multiple of 8
     */
    void SetPacketWindowSize(uint16_t size);

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * @brief Handle a packet reception.
     *
     * This function is called by lower layers.
     *
     * @param socket the socket the packet was received to.
     */
    void HandleRead(Ptr<Socket> socket);

    Ptr<Socket> m_socket;            //!< Socket
    Ptr<Socket> m_socket6;           //!< IPv6 Socket (used if only port is specified)
    uint64_t m_received;             //!< Number of received packets
    PacketLossCounter m_lossCounter; //!< Lost packet counter

    /// Callbacks for tracing the packet Rx events
    TracedCallback<Ptr<const Packet>> m_rxTrace;

    /// Callbacks for tracing the packet Rx events, includes source and destination addresses
    TracedCallback<Ptr<const Packet>, const Address&, const Address&> m_rxTraceWithAddresses;
};

} // namespace ns3

#endif /* UDP_SERVER_H */
