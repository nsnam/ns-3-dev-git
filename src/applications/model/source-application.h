/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef SOURCE_APPLICATION_H
#define SOURCE_APPLICATION_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/traced-callback.h"

namespace ns3
{

class Packet;
class Socket;

/**
 * @ingroup applications
 * @brief Base class for source applications.
 *
 * This class can be used as a base class for source applications.
 * A source application is one that primarily sources new data towards a single remote client
 * address and port, and may also receive data (such as an HTTP server).
 *
 * The main purpose of this base class application public API is to provide a uniform way to
 * configure remote and local addresses.
 *
 * Unlike the SinkApplication, the SourceApplication does not expose an individual Port attribute.
 * Instead, the port values are embedded in the Local and Remote address attributes, which should be
 * configured to an InetSocketAddress or Inet6SocketAddress value that contains the desired port
 * number.
 */
class SourceApplication : public Application
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Constructor
     * @param allowPacketSocket flag whether the application should allow the use of packet sockets
     */
    explicit SourceApplication(bool allowPacketSocket = true);
    ~SourceApplication() override;

    /**
     * @brief set the remote address
     * @param addr remote address
     */
    virtual void SetRemote(const Address& addr);

    /**
     * @brief get the remote address
     * @return the remote address
     */
    Address GetRemote() const;

    /**
     * @brief Get the socket this application is attached to.
     * @return pointer to associated socket
     */
    Ptr<Socket> GetSocket() const;

  protected:
    void DoDispose() override;

    /**
     * @brief Close the socket
     * @return true if the socket was closed, false if there was no socket to close
     */
    bool CloseSocket();

    /// Traced Callback: transmitted packets.
    TracedCallback<Ptr<const Packet>> m_txTrace;

    /**
     * TracedCallback signature for connection success/failure event.
     *
     * @param [in] socket The socket for which connection succeeded/failed.
     * @param [in] local The local address.
     * @param [in] remote The remote address.
     */
    typedef void (*ConnectionEventCallback)(Ptr<Socket> socket,
                                            const Address& local,
                                            const Address& remote);

    /// Traced Callback: connection success event.
    TracedCallback<Ptr<Socket>, const Address&, const Address&> m_connectionSuccess;

    /// Traced Callback: connection failure event.
    TracedCallback<Ptr<Socket>, const Address&, const Address&> m_connectionFailure;

    Ptr<Socket> m_socket; //!< Socket

    TypeId m_protocolTid; //!< Protocol TypeId value

    Address m_peer;  //!< Peer address
    Address m_local; //!< Local address to bind to
    uint8_t m_tos;   //!< The packets Type of Service

    bool m_connected{false}; //!< flag whether socket is connected

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * @brief Handle a Connection Succeed event
     * @param socket the connected socket
     */
    void ConnectionSucceeded(Ptr<Socket> socket);

    /**
     * @brief Handle a Connection Failed event
     * @param socket the not connected socket
     */
    void ConnectionFailed(Ptr<Socket> socket);

    /**
     * @brief Application specific startup code for child subclasses
     */
    virtual void DoStartApplication();

    /**
     * @brief Application specific shutdown code for child subclasses
     */
    virtual void DoStopApplication();

    /**
     * @brief Application specific code for child subclasses upon a Connection Succeed event
     * @param socket the connected socket
     */
    virtual void DoConnectionSucceeded(Ptr<Socket> socket);

    /**
     * @brief Application specific code for child subclasses upon a Connection Failed event
     * @param socket the not connected socket
     */
    virtual void DoConnectionFailed(Ptr<Socket> socket);

    /**
     * @brief Cancel all pending events.
     */
    virtual void CancelEvents() = 0;

    bool m_allowPacketSocket; //!< Allow use of packet socket
};

} // namespace ns3

#endif /* SOURCE_APPLICATION_H */
