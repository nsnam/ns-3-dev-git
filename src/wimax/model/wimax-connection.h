/*
 * Copyright (c) 2007,2008, 2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 *         Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */

#ifndef WIMAX_CONNECTION_H
#define WIMAX_CONNECTION_H

#include "cid.h"
#include "service-flow.h"
#include "wimax-mac-header.h"
#include "wimax-mac-queue.h"

#include "ns3/object.h"

#include <ostream>
#include <stdint.h>

namespace ns3
{

class ServiceFlow;
class Cid;

/**
 * @ingroup wimax
 * Class to represent WiMAX connections
 */
class WimaxConnection : public Object
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
     * @param cid connection ID
     * @param type CID type
     */
    WimaxConnection(Cid cid, Cid::Type type);
    ~WimaxConnection() override;

    /**
     * Get CID function
     * @returns the CID
     */
    Cid GetCid() const;

    /**
     * Get type function
     * @returns the type
     */
    Cid::Type GetType() const;
    /**
     * @return the queue of the connection
     */
    Ptr<WimaxMacQueue> GetQueue() const;
    /**
     * @brief set the service flow associated to this connection
     * @param serviceFlow The service flow to be associated to this connection
     */
    void SetServiceFlow(ServiceFlow* serviceFlow);
    /**
     * @return the service flow associated to this connection
     */
    ServiceFlow* GetServiceFlow() const;

    // wrapper functions
    /**
     * @return the scheduling type of this connection
     */
    uint8_t GetSchedulingType() const;
    /**
     * @brief enqueue a packet in the queue of the connection
     * @param packet the packet to be enqueued
     * @param hdrType the header type of the packet
     * @param hdr the header of the packet
     * @return true if successful
     */
    bool Enqueue(Ptr<Packet> packet, const MacHeaderType& hdrType, const GenericMacHeader& hdr);
    /**
     * @brief dequeue a packet from the queue of the connection
     * @param packetType the type of the packet to dequeue
     * @return packet dequeued
     */
    Ptr<Packet> Dequeue(MacHeaderType::HeaderType packetType = MacHeaderType::HEADER_TYPE_GENERIC);
    /**
     * @brief dequeue a packet from the queue of the connection
     * Dequeue the first packet in the queue if its size is lower than
     * availableByte, the first availableByte of the first packet otherwise
     *
     * @param packetType the type of the packet to dequeue
     * @param availableByte the number of available bytes
     * @return packet dequeued
     */
    Ptr<Packet> Dequeue(MacHeaderType::HeaderType packetType, uint32_t availableByte);
    /**
     * @return true if the connection has at least one packet in its queue, false otherwise
     */
    bool HasPackets() const;
    /**
     * @return true if the connection has at least one packet of type packetType in its queue, false
     * otherwise
     * @param packetType type of packet to check in the queue
     * @return true if packets available
     */
    bool HasPackets(MacHeaderType::HeaderType packetType) const;

    /**
     * Get type string
     * @returns the type string
     */
    std::string GetTypeStr() const;

    /// Definition of Fragments Queue data type
    typedef std::list<Ptr<const Packet>> FragmentsQueue;
    /**
     * @brief get a queue of received fragments
     * @returns the fragments queue
     */
    const FragmentsQueue GetFragmentsQueue() const;
    /**
     * @brief enqueue a received packet (that is a fragment) into the fragments queue
     * @param fragment received fragment
     */
    void FragmentEnqueue(Ptr<const Packet> fragment);
    /**
     * @brief delete all enqueued fragments
     */
    void ClearFragmentsQueue();

  private:
    void DoDispose() override;

    Cid m_cid;                  ///< CID
    Cid::Type m_cidType;        ///< CID type
    Ptr<WimaxMacQueue> m_queue; ///< queue
    ServiceFlow* m_serviceFlow; ///< service flow

    // FragmentsQueue stores all received fragments
    FragmentsQueue m_fragmentsQueue; ///< fragments queue
};

} // namespace ns3

#endif /* WIMAX_CONNECTION_H */
