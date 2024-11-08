/*
 * Copyright (c) 2008 Drexel University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Joe Kopena (tjkopena@cs.drexel.edu)
 */

#ifndef PACKET_DATA_CALCULATORS_H
#define PACKET_DATA_CALCULATORS_H

#include "mac48-address.h"

#include "ns3/basic-data-calculators.h"
#include "ns3/data-calculator.h"
#include "ns3/packet.h"

namespace ns3
{

/**
 * @ingroup stats
 *
 *  A stat for counting packets
 *
 */
class PacketCounterCalculator : public CounterCalculator<uint32_t>
{
  public:
    PacketCounterCalculator();
    ~PacketCounterCalculator() override;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Increments the packet counter by one
     *
     * @param path not used in this method
     * @param packet not used in this method
     */
    void PacketUpdate(std::string path, Ptr<const Packet> packet);

    /**
     * Increments the packet counter by one
     *
     * @param path not used in this method
     * @param packet not used in this method
     * @param realto not used in this method
     */

    void FrameUpdate(std::string path, Ptr<const Packet> packet, Mac48Address realto);

  protected:
    /**
     * Dispose of this Object.
     */
    void DoDispose() override;

    // end class PacketCounterCalculator
};

/**
 * @ingroup stats
 *
 * A stat for collecting packet size statistics: min, max and average
 *
 */
class PacketSizeMinMaxAvgTotalCalculator : public MinMaxAvgTotalCalculator<uint32_t>
{
  public:
    PacketSizeMinMaxAvgTotalCalculator();
    ~PacketSizeMinMaxAvgTotalCalculator() override;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Increments the packet stats by the size of the packet
     *
     * @param path not used in this method
     * @param packet packet size used to update stats
     */
    void PacketUpdate(std::string path, Ptr<const Packet> packet);

    /**
     * Increments the packet stats by the size of the packet
     *
     * @param path not used in this method
     * @param packet packet size used to update stats
     * @param realto not used in this method
     */
    void FrameUpdate(std::string path, Ptr<const Packet> packet, Mac48Address realto);

  protected:
    void DoDispose() override;

    // end class PacketSizeMinMaxAvgTotalCalculator
};

// end namespace ns3
}; // namespace ns3

#endif /* PACKET_DATA_CALCULATORS_H */
