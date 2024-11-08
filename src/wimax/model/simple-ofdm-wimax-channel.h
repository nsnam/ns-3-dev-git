/*
 *  Copyright (c) 2007,2008, 2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                              <amine.ismail@udcast.com>
 */

#ifndef SIMPLE_OFDM_WIMAX_CHANNEL_H
#define SIMPLE_OFDM_WIMAX_CHANNEL_H

#include "bvec.h"
#include "simple-ofdm-send-param.h"
#include "wimax-channel.h"
#include "wimax-phy.h"

#include "ns3/propagation-loss-model.h"

#include <list>

namespace ns3
{

class Packet;
class PacketBurst;
class SimpleOfdmWimaxPhy;

/**
 * @ingroup wimax
 * @brief SimpleOfdmWimaxChannel class
 */
class SimpleOfdmWimaxChannel : public WimaxChannel
{
  public:
    SimpleOfdmWimaxChannel();
    ~SimpleOfdmWimaxChannel() override;

    /// PropModel enumeration
    enum PropModel
    {
        RANDOM_PROPAGATION,
        FRIIS_PROPAGATION,
        LOG_DISTANCE_PROPAGATION,
        COST231_PROPAGATION
    };

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * @brief Creates a channel and sets the propagation model
     * @param propModel the propagation model to use
     */
    SimpleOfdmWimaxChannel(PropModel propModel);

    /**
     * @brief Sends a dummy fec block to all connected physical devices
     * @param BlockTime the time needed to send the block
     * @param burstSize the size of the burst
     * @param phy the sender device
     * @param isFirstBlock true if this block is the first one, false otherwise
     * @param isLastBlock true if this block is the last one, false otherwise
     * @param frequency the frequency on which the block is sent
     * @param modulationType the modulation used to send the fec block
     * @param direction uplink or downlink
     * @param txPowerDbm the transmission power
     * @param burst the packet burst to send
     */
    void Send(Time BlockTime,
              uint32_t burstSize,
              Ptr<WimaxPhy> phy,
              bool isFirstBlock,
              bool isLastBlock,
              uint64_t frequency,
              WimaxPhy::ModulationType modulationType,
              uint8_t direction,
              double txPowerDbm,
              Ptr<PacketBurst> burst);
    /**
     * @brief sets the propagation model
     * @param propModel the propagation model to used
     */
    void SetPropagationModel(PropModel propModel);

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * @param stream first stream index to use
     * @return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream) override;

  private:
    /**
     * Attach function
     * @param phy the phy layer
     */
    void DoAttach(Ptr<WimaxPhy> phy) override;
    std::list<Ptr<SimpleOfdmWimaxPhy>> m_phyList; ///< phy list
    /**
     * Get number of devices function
     * @returns the number of devices
     */
    std::size_t DoGetNDevices() const override;
    /**
     * End send dummy block function
     * @param rxphy the Ptr<SimpleOfdmWimaxPhy>
     * @param param the SimpleOfdmSendParam *
     */
    void EndSendDummyBlock(Ptr<SimpleOfdmWimaxPhy> rxphy, SimpleOfdmSendParam* param);
    /**
     * Get device function
     * @param i the device index
     * @returns the device
     */
    Ptr<NetDevice> DoGetDevice(std::size_t i) const override;
    Ptr<PropagationLossModel> m_loss; ///< loss
};

} // namespace ns3

#endif /* SIMPLE_OFDM_WIMAX_CHANNEL_H */
