/*
 * Copyright 2007 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Tom Henderson (tomhend@u.washington.edu)
 */

#ifndef PACKET_SINK_H
#define PACKET_SINK_H

#include "seq-ts-size-header.h"
#include "sink-application.h"

#include "ns3/event-id.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"

#include <unordered_map>

namespace ns3
{

class Socket;
class Packet;

/**
 * @ingroup applications
 * @defgroup packetsink PacketSink
 *
 * This application was written to complement OnOffApplication, but it
 * is more general so a PacketSink name was selected.  Functionally it is
 * important to use in multicast situations, so that reception of the layer-2
 * multicast frames of interest are enabled, but it is also useful for
 * unicast as an example of how you can write something simple to receive
 * packets at the application layer.  Also, if an IP stack generates
 * ICMP Port Unreachable errors, receiving applications will be needed.
 */

/**
 * @ingroup packetsink
 *
 * @brief Receive and consume traffic generated to an IP address and port
 *
 * This application was written to complement OnOffApplication, but it
 * is more general so a PacketSink name was selected.  Functionally it is
 * important to use in multicast situations, so that reception of the layer-2
 * multicast frames of interest are enabled, but it is also useful for
 * unicast as an example of how you can write something simple to receive
 * packets at the application layer.  Also, if an IP stack generates
 * ICMP Port Unreachable errors, receiving applications will be needed.
 *
 * The constructor specifies the Address (IP address and port) and the
 * transport protocol to use.   A virtual Receive () method is installed
 * as a callback on the receiving socket.  By default, when logging is
 * enabled, it prints out the size of packets and their address.
 * A tracing source to Receive() is also available.
 */
class PacketSink : public SinkApplication
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    PacketSink();
    ~PacketSink() override;

    /**
     * @return the total bytes received in this sink app
     */
    uint64_t GetTotalRx() const;

    /**
     * @return pointer to listening socket
     */
    Ptr<Socket> GetListeningSocket() const;

    /**
     * @return list of pointers to accepted sockets
     */
    std::list<Ptr<Socket>> GetAcceptedSockets() const;

    /**
     * TracedCallback signature for a reception with addresses and SeqTsSizeHeader
     *
     * @param p The packet received (without the SeqTsSize header)
     * @param from From address
     * @param to Local address
     * @param header The SeqTsSize header
     */
    typedef void (*SeqTsSizeCallback)(Ptr<const Packet> p,
                                      const Address& from,
                                      const Address& to,
                                      const SeqTsSizeHeader& header);

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
    /**
     * @brief Handle an incoming connection
     * @param socket the incoming connection socket
     * @param from the address the connection is from
     */
    void HandleAccept(Ptr<Socket> socket, const Address& from);
    /**
     * @brief Handle an connection close
     * @param socket the connected socket
     */
    void HandlePeerClose(Ptr<Socket> socket);
    /**
     * @brief Handle an connection error
     * @param socket the connected socket
     */
    void HandlePeerError(Ptr<Socket> socket);

    /**
     * @brief Packet received: assemble byte stream to extract SeqTsSizeHeader
     * @param p received packet
     * @param from from address
     * @param localAddress local address
     *
     * The method assembles a received byte stream and extracts SeqTsSizeHeader
     * instances from the stream to export in a trace source.
     */
    void PacketReceived(const Ptr<Packet>& p, const Address& from, const Address& localAddress);

    /**
     * @brief Hashing for the Address class
     */
    struct AddressHash
    {
        /**
         * @brief operator ()
         * @param x the address of which calculate the hash
         * @return the hash of x
         *
         * Should this method go in address.h?
         *
         * It calculates the hash taking the uint32_t hash value of the IPv4 or IPv6 address.
         * It works only for InetSocketAddresses (IPv4 version) or Inet6SocketAddresses (IPv6
         * version)
         */
        size_t operator()(const Address& x) const
        {
            if (InetSocketAddress::IsMatchingType(x))
            {
                InetSocketAddress a = InetSocketAddress::ConvertFrom(x);
                return Ipv4AddressHash()(a.GetIpv4());
            }
            else if (Inet6SocketAddress::IsMatchingType(x))
            {
                Inet6SocketAddress a = Inet6SocketAddress::ConvertFrom(x);
                return Ipv6AddressHash()(a.GetIpv6());
            }

            NS_ABORT_MSG("PacketSink: unexpected address type, neither IPv4 nor IPv6");
            return 0; // silence the warnings.
        }
    };

    std::unordered_map<Address, Ptr<Packet>, AddressHash> m_buffer; //!< Buffer for received packets

    Ptr<Socket> m_socket;  //!< Socket
    Ptr<Socket> m_socket6; //!< IPv6 Socket (used if only port is specified)

    // In the case of TCP, each socket accept returns a new socket, so the
    // listening socket is stored separately from the accepted sockets
    std::list<Ptr<Socket>> m_socketList; //!< the accepted sockets

    uint64_t m_totalRx; //!< Total bytes received
    TypeId m_tid;       //!< Protocol TypeId

    bool m_enableSeqTsSizeHeader; //!< Enable or disable the export of SeqTsSize header

    /// Traced Callback: received packets, source address.
    TracedCallback<Ptr<const Packet>, const Address&> m_rxTrace;
    /// Callback for tracing the packet Rx events, includes source and destination addresses
    TracedCallback<Ptr<const Packet>, const Address&, const Address&> m_rxTraceWithAddresses;
    /// Callbacks for tracing the packet Rx events, includes source, destination addresses, and
    /// headers
    TracedCallback<Ptr<const Packet>, const Address&, const Address&, const SeqTsSizeHeader&>
        m_rxTraceWithSeqTsSize;
};

} // namespace ns3

#endif /* PACKET_SINK_H */
