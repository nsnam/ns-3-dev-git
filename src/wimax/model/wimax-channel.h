/*
 * Copyright (c) 2007,2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 */

#ifndef WIMAX_CHANNEL_H
#define WIMAX_CHANNEL_H

#include "wimax-connection.h"

#include "ns3/channel.h"
#include "ns3/log.h"

#include <list>

namespace ns3
{

class WimaxPhy;
class Packet;
class Position;
class PacketBurst;

/**
 * @ingroup wimax
 * The channel object to attach Wimax NetDevices
 */
class WimaxChannel : public Channel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    WimaxChannel();
    ~WimaxChannel() override;
    /**
     * @brief attach the channel to a physical layer of a device
     * @param phy the physical layer to which the channel will be attached
     */
    void Attach(Ptr<WimaxPhy> phy);
    /**
     * @return the number of attached devices
     */
    std::size_t GetNDevices() const override;
    /**
     * @param i the ith device
     * @return the ith attached device
     */
    Ptr<NetDevice> GetDevice(std::size_t i) const override;

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * @param stream first stream index to use
     * @return the number of stream indices assigned by this model
     */
    virtual int64_t AssignStreams(int64_t stream) = 0;

  private:
    /**
     * Attach a phy to the channel
     * @param phy the phy object to attach
     */
    virtual void DoAttach(Ptr<WimaxPhy> phy) = 0;

    /**
     * Get number of devices on the channel
     * @returns the number of devices
     */
    virtual std::size_t DoGetNDevices() const = 0;
    /**
     * Get device corresponding to index
     * @param i the device index
     * @returns the device
     */
    virtual Ptr<NetDevice> DoGetDevice(std::size_t i) const = 0;
};

} // namespace ns3

#endif /* WIMAX_CHANNEL_H */
