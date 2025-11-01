/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef SINK_APPLICATION_H
#define SINK_APPLICATION_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/traced-callback.h"

#include <limits>

namespace ns3
{

class Packet;
class Address;
class Socket;

/**
 * @ingroup applications
 * @brief Base class for sink applications.
 *
 * This class can be used as a base class for sink applications.
 * A sink application is an application that is primarily used to only receive or echo packets.
 *
 * The main purpose of this base class application public API is to hold attributes for the local
 * (IPv4 or IPv6) address and port to bind to.
 *
 * There are three ways that the port value can be configured. First, and most typically, through
 * the use of a socket address (InetSocketAddress or Inet6SocketAddress) that is configured as the
 * Local address to bind to. Second, through direct configuration of the Port attribute. Third,
 * through the use of an optional constructor argument. If multiple of these port configuration
 * methods are used, it is up to subclass definition which one takes precedence; in the existing
 * subclasses in this directory, the port value configured in the Local socket address (if a socket
 * address is configured there) will take precedence.
 */
class SinkApplication : public Application
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor
     *
     * @param defaultPort the default port number
     */
    SinkApplication(uint16_t defaultPort = 0);
    ~SinkApplication() override;

    static constexpr uint32_t INVALID_PORT{std::numeric_limits<uint32_t>::max()}; //!< invalid port

  protected:
    void DoDispose() override;

    /**
     * @brief Close all the sockets
     * @return true if all sockets closed successfully, false otherwise
     */
    bool CloseAllSockets();

    /// Callbacks for tracing the packet Rx events
    ns3::TracedCallback<Ptr<const Packet>> m_rxTraceWithoutAddress;

    /// Traced Callback: received packets, source address.
    TracedCallback<Ptr<const Packet>, const Address&> m_rxTrace;

    Ptr<Socket> m_socket;  //!< Socket (IPv4 or IPv6, depending on local address)
    Ptr<Socket> m_socket6; //!< IPv6 Socket (used if only port is specified)

    TypeId m_protocolTid; //!< Protocol TypeId value

    Address m_local; //!< Local address to bind to (address and port)
    uint32_t m_port; //!< Local port to bind to

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * @brief set the local address
     * @param addr local address
     */
    virtual void SetLocal(const Address& addr);

    /**
     * @brief get the local address
     * @return the local address
     */
    Address GetLocal() const;

    /**
     * @brief set the server port
     * @param port server port
     */
    virtual void SetPort(uint32_t port);

    /**
     * @brief get the server port
     * @return the server port
     */
    uint32_t GetPort() const;

    /**
     * @brief Close the socket
     * @param socket the socket to close
     * @return true if the socket closed successfully, false otherwise
     */
    bool CloseSocket(Ptr<Socket> socket);

    /**
     * @brief Application specific startup code for child subclasses
     */
    virtual void DoStartApplication();

    /**
     * @brief Application specific shutdown code for child subclasses
     */
    virtual void DoStopApplication();
};

} // namespace ns3

#endif /* SINK_APPLICATION_H */
