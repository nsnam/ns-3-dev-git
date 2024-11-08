/*
 * Copyright (c) 2007 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: George Riley <riley@ece.gatech.edu>
 */

// This object connects two point-to-point net devices where at least one
// is not local to this simulator object.  It simply over-rides the transmit
// method and uses an MPI Send operation instead.

#ifndef POINT_TO_POINT_REMOTE_CHANNEL_H
#define POINT_TO_POINT_REMOTE_CHANNEL_H

#include "point-to-point-channel.h"

namespace ns3
{

/**
 * @ingroup point-to-point
 *
 * @brief A Remote Point-To-Point Channel
 *
 * This object connects two point-to-point net devices where at least one
 * is not local to this simulator object. It simply override the transmit
 * method and uses an MPI Send operation instead.
 */
class PointToPointRemoteChannel : public PointToPointChannel
{
  public:
    /**
     * @brief Get the TypeId
     *
     * @return The TypeId for this class
     */
    static TypeId GetTypeId();

    /**
     * @brief Constructor
     */
    PointToPointRemoteChannel();

    /**
     * @brief Deconstructor
     */
    ~PointToPointRemoteChannel() override;

    /**
     * @brief Transmit the packet
     *
     * @param p Packet to transmit
     * @param src Source PointToPointNetDevice
     * @param txTime Transmit time to apply
     * @returns true if successful (currently always true)
     */
    bool TransmitStart(Ptr<const Packet> p, Ptr<PointToPointNetDevice> src, Time txTime) override;
};

} // namespace ns3

#endif
