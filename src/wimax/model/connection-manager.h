/*
 * Copyright (c) 2007,2008,2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 *          Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                               <amine.ismail@UDcast.com>
 */

#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include "cid.h"
#include "wimax-connection.h"

#include "ns3/mac48-address.h"

#include <stdint.h>

namespace ns3
{

class CidFactory;
class SSRecord;
class RngRsp;
class WimaxNetDevice;
class SubscriberStationNetDevice;

/**
 * @ingroup wimax
 * The same connection manager class serves both for BS and SS though some functions are exclusive
 * to only one of them.
 */

class ConnectionManager : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    ConnectionManager();
    ~ConnectionManager() override;
    void DoDispose() override;
    /**
     * Set CID factory
     * @param cidFactory the CID factory
     */
    void SetCidFactory(CidFactory* cidFactory);
    /**
     * @brief allocates the management connection for an ss record. This method is only used by BS
     * @param ssRecord the ss record to which the management connection will be allocated
     * @param rngrsp the ranging response message
     */
    void AllocateManagementConnections(SSRecord* ssRecord, RngRsp* rngrsp);
    /**
     * @brief create a connection of type type
     * @param type type of the connection to create
     * @return a smart pointer to the created WimaxConnection
     */
    Ptr<WimaxConnection> CreateConnection(Cid::Type type);
    /**
     * @brief add a connection to the list of managed connections
     * @param connection the connection to add
     * @param type the type of connection to add
     */
    void AddConnection(Ptr<WimaxConnection> connection, Cid::Type type);
    /**
     * @param cid the connection identifier
     * @return the connection corresponding to cid
     */
    Ptr<WimaxConnection> GetConnection(Cid cid);
    /**
     * @param type the type of connection to add
     * @return a vector of all connections matching the input type
     */
    std::vector<Ptr<WimaxConnection>> GetConnections(Cid::Type type) const;
    /**
     * @brief get number of packets
     * @param type the type of connection to add
     * @param schedulingType the scheduling type
     * @returns number of packets
     */
    uint32_t GetNPackets(Cid::Type type, ServiceFlow::SchedulingType schedulingType) const;
    /**
     * @return true if one of the managed connection has at least one packet to send, false
     * otherwise
     */
    bool HasPackets() const;

  private:
    std::vector<Ptr<WimaxConnection>> m_basicConnections;     ///< basic connections
    std::vector<Ptr<WimaxConnection>> m_primaryConnections;   ///< primary connections
    std::vector<Ptr<WimaxConnection>> m_transportConnections; ///< transport connections
    std::vector<Ptr<WimaxConnection>> m_multicastConnections; ///< multicast connections
    // only for BS
    CidFactory* m_cidFactory; ///< the factory
};

} // namespace ns3

#endif /* CONNECTION_MANAGER_H */
