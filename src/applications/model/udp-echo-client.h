/*
 * Copyright 2007 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef UDP_ECHO_CLIENT_H
#define UDP_ECHO_CLIENT_H

#include "source-application.h"

#include "ns3/deprecated.h"
#include "ns3/event-id.h"
#include "ns3/ipv4-address.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"

#include <optional>

namespace ns3
{

class Socket;
class Packet;

/**
 * @ingroup udpecho
 * @brief A Udp Echo client
 *
 * Every packet sent should be returned by the server and received here.
 */
class UdpEchoClient : public SourceApplication
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    UdpEchoClient();
    ~UdpEchoClient() override;

    static constexpr uint16_t DEFAULT_PORT{0}; //!< default port

    /**
     * @brief set the remote address and port
     * @param ip remote IP address
     * @param port remote port
     */
    NS_DEPRECATED_3_44("Use SetRemote without port parameter instead")
    void SetRemote(const Address& ip, uint16_t port);
    void SetRemote(const Address& addr) override;

    /**
     * Set the data size of the packet (the number of bytes that are sent as data
     * to the server).  The contents of the data are set to unspecified (don't
     * care) by this call.
     *
     * @warning If you have set the fill data for the echo client using one of the
     * SetFill calls, this will undo those effects.
     *
     * @param dataSize The size of the echo data you want to sent.
     */
    void SetDataSize(uint32_t dataSize);

    /**
     * Get the number of data bytes that will be sent to the server.
     *
     * @warning The number of bytes may be modified by calling any one of the
     * SetFill methods.  If you have called SetFill, then the number of
     * data bytes will correspond to the size of an initialized data buffer.
     * If you have not called a SetFill method, the number of data bytes will
     * correspond to the number of don't care bytes that will be sent.
     *
     * @returns The number of data bytes.
     */
    uint32_t GetDataSize() const;

    /**
     * Set the data fill of the packet (what is sent as data to the server) to
     * the zero-terminated contents of the fill string string.
     *
     * @warning The size of resulting echo packets will be automatically adjusted
     * to reflect the size of the fill string -- this means that the PacketSize
     * attribute may be changed as a result of this call.
     *
     * @param fill The string to use as the actual echo data bytes.
     */
    void SetFill(std::string fill);

    /**
     * Set the data fill of the packet (what is sent as data to the server) to
     * the repeated contents of the fill byte.  i.e., the fill byte will be
     * used to initialize the contents of the data packet.
     *
     * @warning The size of resulting echo packets will be automatically adjusted
     * to reflect the dataSize parameter -- this means that the PacketSize
     * attribute may be changed as a result of this call.
     *
     * @param fill The byte to be repeated in constructing the packet data..
     * @param dataSize The desired size of the resulting echo packet data.
     */
    void SetFill(uint8_t fill, uint32_t dataSize);

    /**
     * Set the data fill of the packet (what is sent as data to the server) to
     * the contents of the fill buffer, repeated as many times as is required.
     *
     * Initializing the packet to the contents of a provided single buffer is
     * accomplished by setting the fillSize set to your desired dataSize
     * (and providing an appropriate buffer).
     *
     * @warning The size of resulting echo packets will be automatically adjusted
     * to reflect the dataSize parameter -- this means that the PacketSize
     * attribute of the Application may be changed as a result of this call.
     *
     * @param fill The fill pattern to use when constructing packets.
     * @param fillSize The number of bytes in the provided fill pattern.
     * @param dataSize The desired size of the final echo data.
     */
    void SetFill(uint8_t* fill, uint32_t fillSize, uint32_t dataSize);

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * @brief Set the remote port (temporary function until deprecated attributes are removed)
     * @param port remote port
     */
    void SetPort(uint16_t port);

    /**
     * @brief Get the remote port (temporary function until deprecated attributes are removed)
     * @return the remote port
     */
    uint16_t GetPort() const;

    /**
     * @brief Get the remote address (temporary function until deprecated attributes are removed)
     * @return the remote address
     */
    Address GetRemote() const;

    /**
     * @brief Schedule the next packet transmission
     * @param dt time interval between packets.
     */
    void ScheduleTransmit(Time dt);
    /**
     * @brief Send a packet
     */
    void Send();

    /**
     * @brief Handle a packet reception.
     *
     * This function is called by lower layers.
     *
     * @param socket the socket the packet was received to.
     */
    void HandleRead(Ptr<Socket> socket);

    uint32_t m_count; //!< Maximum number of packets the application will send
    Time m_interval;  //!< Packet inter-send time
    uint32_t m_size;  //!< Size of the sent packet

    uint32_t m_dataSize; //!< packet payload size (must be equal to m_size)
    uint8_t* m_data;     //!< packet payload data

    uint32_t m_sent;                    //!< Counter for sent packets
    Ptr<Socket> m_socket;               //!< Socket
    std::optional<uint16_t> m_peerPort; //!< Remote peer port (deprecated) // NS_DEPRECATED_3_44
    EventId m_sendEvent;                //!< Event to send the next packet

    /// Callbacks for tracing the packet Tx events
    TracedCallback<Ptr<const Packet>> m_txTrace;

    /// Callbacks for tracing the packet Rx events
    TracedCallback<Ptr<const Packet>> m_rxTrace;

    /// Callbacks for tracing the packet Tx events, includes source and destination addresses
    TracedCallback<Ptr<const Packet>, const Address&, const Address&> m_txTraceWithAddresses;

    /// Callbacks for tracing the packet Rx events, includes source and destination addresses
    TracedCallback<Ptr<const Packet>, const Address&, const Address&> m_rxTraceWithAddresses;
};

} // namespace ns3

#endif /* UDP_ECHO_CLIENT_H */
