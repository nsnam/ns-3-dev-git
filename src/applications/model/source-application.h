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

namespace ns3
{

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

    SourceApplication();
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

  protected:
    Address m_peer;  //!< Peer address
    Address m_local; //!< Local address to bind to
    uint8_t m_tos;   //!< The packets Type of Service
};

} // namespace ns3

#endif /* SOURCE_APPLICATION_H */
