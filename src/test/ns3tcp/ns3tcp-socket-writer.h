/*
 * Copyright (c) 2010 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/node.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"

namespace ns3
{

/**
 * @ingroup system-tests-tcp
 *
 * @brief Simple class to write data to sockets.
 */
class SocketWriter : public Application
{
  public:
    SocketWriter();
    ~SocketWriter() override;
    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Setup the socket.
     * @param node The node owning the socket.
     * @param peer The destination address.
     */
    void Setup(Ptr<Node> node, Address peer);
    /**
     * Connect the socket.
     */
    void Connect();
    /**
     * Write to the socket.
     * @param numBytes The number of bytes to write.
     */
    void Write(uint32_t numBytes);
    /**
     * Close the socket.
     */
    void Close();

  private:
    void StartApplication() override;
    void StopApplication() override;
    Address m_peer;       //!< Peer's address.
    Ptr<Node> m_node;     //!< Node pointer
    Ptr<Socket> m_socket; //!< Socket.
    bool m_isSetup;       //!< True if the socket is connected.
    bool m_isConnected;   //!< True if the socket setup has been done.
};
} // namespace ns3
