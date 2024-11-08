/*
 * Copyright (c) 2006,2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage, <mathieu.lacage@sophia.inria.fr>
 */

#ifndef YANS_WIFI_CHANNEL_H
#define YANS_WIFI_CHANNEL_H

#include "wifi-units.h"

#include "ns3/channel.h"

namespace ns3
{

class NetDevice;
class PropagationLossModel;
class PropagationDelayModel;
class YansWifiPhy;
class Packet;
class Time;
class WifiPpdu;

/**
 * @brief a channel to interconnect ns3::YansWifiPhy objects.
 * @ingroup wifi
 *
 * This class is expected to be used in tandem with the ns3::YansWifiPhy
 * class and supports an ns3::PropagationLossModel and an
 * ns3::PropagationDelayModel.  By default, no propagation models are set;
 * it is the caller's responsibility to set them before using the channel.
 */
class YansWifiChannel : public Channel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    YansWifiChannel();
    ~YansWifiChannel() override;

    std::size_t GetNDevices() const override;
    Ptr<NetDevice> GetDevice(std::size_t i) const override;

    /**
     * Adds the given YansWifiPhy to the PHY list
     *
     * @param phy the YansWifiPhy to be added to the PHY list
     */
    void Add(Ptr<YansWifiPhy> phy);

    /**
     * @param loss the new propagation loss model.
     */
    void SetPropagationLossModel(const Ptr<PropagationLossModel> loss);
    /**
     * @param delay the new propagation delay model.
     */
    void SetPropagationDelayModel(const Ptr<PropagationDelayModel> delay);

    /**
     * @param sender the PHY object from which the packet is originating.
     * @param ppdu the PPDU to send
     * @param txPower the TX power associated to the packet
     *
     * This method should not be invoked by normal users. It is
     * currently invoked only from YansWifiPhy::StartTx.  The channel
     * attempts to deliver the PPDU to all other YansWifiPhy objects
     * on the channel (except for the sender).
     */
    void Send(Ptr<YansWifiPhy> sender, Ptr<const WifiPpdu> ppdu, dBm_u txPower) const;

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * @param stream first stream index to use
     *
     * @return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

  private:
    /**
     * A vector of pointers to YansWifiPhy.
     */
    typedef std::vector<Ptr<YansWifiPhy>> PhyList;

    /**
     * This method is scheduled by Send for each associated YansWifiPhy.
     * The method then calls the corresponding YansWifiPhy that the first
     * bit of the PPDU has arrived.
     *
     * @param receiver the device to which the packet is destined
     * @param ppdu the PPDU being sent
     * @param txPower the TX power associated to the packet being sent
     */
    static void Receive(Ptr<YansWifiPhy> receiver, Ptr<const WifiPpdu> ppdu, dBm_u txPower);

    PhyList m_phyList;                  //!< List of YansWifiPhys connected to this YansWifiChannel
    Ptr<PropagationLossModel> m_loss;   //!< Propagation loss model
    Ptr<PropagationDelayModel> m_delay; //!< Propagation delay model
};

} // namespace ns3

#endif /* YANS_WIFI_CHANNEL_H */
