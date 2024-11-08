/*
 * Copyright (c) 2007,2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 */

#ifndef PACKET_BURST_H
#define PACKET_BURST_H

#include "ns3/object.h"

#include <list>
#include <stdint.h>

namespace ns3
{

class Packet;

/**
 * @brief this class implement a burst as a list of packets
 */
class PacketBurst : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    PacketBurst();
    ~PacketBurst() override;
    /**
     * @return a copy the packetBurst
     */
    Ptr<PacketBurst> Copy() const;
    /**
     * @brief add a packet to the list of packet
     * @param packet the packet to add
     */
    void AddPacket(Ptr<Packet> packet);
    /**
     * @return the list of packet of this burst
     */
    std::list<Ptr<Packet>> GetPackets() const;
    /**
     * @return the number of packet in the burst
     */
    uint32_t GetNPackets() const;
    /**
     * @return the size of the burst in byte (the size of all packets)
     */
    uint32_t GetSize() const;

    /**
     * @brief Returns an iterator to the begin of the burst
     * @return iterator to the burst list start
     */
    std::list<Ptr<Packet>>::const_iterator Begin() const;
    /**
     * @brief Returns an iterator to the end of the burst
     * @return iterator to the burst list end
     */
    std::list<Ptr<Packet>>::const_iterator End() const;

    /**
     * TracedCallback signature for Ptr<PacketBurst>
     *
     * @param [in] burst The PacketBurst
     */
    typedef void (*TracedCallback)(Ptr<const PacketBurst> burst);

  private:
    void DoDispose() override;
    std::list<Ptr<Packet>> m_packets; //!< the list of packets in the burst
};
} // namespace ns3

#endif /* PACKET_BURST */
