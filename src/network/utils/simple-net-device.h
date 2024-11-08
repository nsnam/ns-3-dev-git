/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef SIMPLE_NET_DEVICE_H
#define SIMPLE_NET_DEVICE_H

#include "data-rate.h"
#include "mac48-address.h"
#include "queue-fwd.h"

#include "ns3/event-id.h"
#include "ns3/net-device.h"
#include "ns3/traced-callback.h"

#include <stdint.h>
#include <string>

namespace ns3
{

class SimpleChannel;
class Node;
class ErrorModel;

/**
 * @ingroup netdevice
 *
 * This device assumes 48-bit mac addressing; there is also the possibility to
 * add an ErrorModel if you want to force losses on the device.
 *
 * The device can be installed on a node through the SimpleNetDeviceHelper.
 * In case of manual creation, the user is responsible for assigning an unique
 * address to the device.
 *
 * By default the device is in Broadcast mode, with infinite bandwidth.
 *
 * @brief simple net device for simple things and testing
 */
class SimpleNetDevice : public NetDevice
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    SimpleNetDevice();

    /**
     * Receive a packet from a connected SimpleChannel.  The
     * SimpleNetDevice receives packets from its connected channel
     * and then forwards them by calling its rx callback method
     *
     * @param packet Packet received on the channel
     * @param protocol protocol number
     * @param to address packet should be sent to
     * @param from address packet was sent from
     */
    void Receive(Ptr<Packet> packet, uint16_t protocol, Mac48Address to, Mac48Address from);

    /**
     * Attach a channel to this net device.  This will be the
     * channel the net device sends on
     *
     * @param channel channel to assign to this net device
     *
     */
    void SetChannel(Ptr<SimpleChannel> channel);

    /**
     * Attach a queue to the SimpleNetDevice.
     *
     * @param queue Ptr to the new queue.
     */
    void SetQueue(Ptr<Queue<Packet>> queue);

    /**
     * Get a copy of the attached Queue.
     *
     * @returns Ptr to the queue.
     */
    Ptr<Queue<Packet>> GetQueue() const;

    /**
     * Attach a receive ErrorModel to the SimpleNetDevice.
     *
     * The SimpleNetDevice may optionally include an ErrorModel in
     * the packet receive chain.
     *
     * @see ErrorModel
     * @param em Ptr to the ErrorModel.
     */
    void SetReceiveErrorModel(Ptr<ErrorModel> em);

    // inherited from NetDevice base class.
    void SetIfIndex(const uint32_t index) override;
    uint32_t GetIfIndex() const override;
    Ptr<Channel> GetChannel() const override;
    void SetAddress(Address address) override;
    Address GetAddress() const override;
    bool SetMtu(const uint16_t mtu) override;
    uint16_t GetMtu() const override;
    bool IsLinkUp() const override;
    void AddLinkChangeCallback(Callback<void> callback) override;
    bool IsBroadcast() const override;
    Address GetBroadcast() const override;
    bool IsMulticast() const override;
    Address GetMulticast(Ipv4Address multicastGroup) const override;
    bool IsPointToPoint() const override;
    bool IsBridge() const override;
    bool Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) override;
    bool SendFrom(Ptr<Packet> packet,
                  const Address& source,
                  const Address& dest,
                  uint16_t protocolNumber) override;
    Ptr<Node> GetNode() const override;
    void SetNode(Ptr<Node> node) override;
    bool NeedsArp() const override;
    void SetReceiveCallback(NetDevice::ReceiveCallback cb) override;

    Address GetMulticast(Ipv6Address addr) const override;

    void SetPromiscReceiveCallback(PromiscReceiveCallback cb) override;
    bool SupportsSendFrom() const override;

  protected:
    void DoDispose() override;

  private:
    Ptr<SimpleChannel> m_channel;                        //!< the channel the device is connected to
    NetDevice::ReceiveCallback m_rxCallback;             //!< Receive callback
    NetDevice::PromiscReceiveCallback m_promiscCallback; //!< Promiscuous receive callback
    Ptr<Node> m_node;                                    //!< Node this netDevice is associated to
    uint16_t m_mtu;                                      //!< MTU
    uint32_t m_ifIndex;                                  //!< Interface index
    Mac48Address m_address;                              //!< MAC address
    Ptr<ErrorModel> m_receiveErrorModel;                 //!< Receive error model.

    /**
     * The trace source fired when the phy layer drops a packet it has received
     * due to the error model being active.  Although SimpleNetDevice doesn't
     * really have a Phy model, we choose this trace source name for alignment
     * with other trace sources.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_phyRxDropTrace;

    /**
     * The StartTransmission method is used internally to start the process
     * of sending a packet out on the channel, by scheduling the
     * FinishTransmission method at a time corresponding to the transmission
     * delay of the packet.
     */
    void StartTransmission();

    /**
     * The FinishTransmission method is used internally to finish the process
     * of sending a packet out on the channel.
     * @param packet The packet to send on the channel
     */
    void FinishTransmission(Ptr<Packet> packet);

    bool m_linkUp; //!< Flag indicating whether or not the link is up

    /**
     * Flag indicating whether or not the NetDevice is a Point to Point model.
     * Enabling this will disable Broadcast and Arp.
     */
    bool m_pointToPointMode;

    Ptr<Queue<Packet>> m_queue;      //!< The Queue for outgoing packets.
    DataRate m_bps;                  //!< The device nominal Data rate. Zero means infinite
    EventId FinishTransmissionEvent; //!< the Tx Complete event

    /**
     * List of callbacks to fire if the link changes state (up or down).
     */
    TracedCallback<> m_linkChangeCallbacks;
};

} // namespace ns3

#endif /* SIMPLE_NET_DEVICE_H */
