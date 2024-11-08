/*
 * Copyright (c) 2020 Universita' di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */
#ifndef MOCK_NET_DEVICE_H
#define MOCK_NET_DEVICE_H

#include "ns3/net-device.h"
#include "ns3/traced-callback.h"

#include <stdint.h>
#include <string>

namespace ns3
{

class Node;

/**
 * @ingroup netdevice
 *
 * This device assumes 48-bit mac addressing; there is also the possibility to
 * add an ErrorModel if you want to force losses on the device.
 *
 * The device can be installed on a node through the MockNetDeviceHelper.
 * In case of manual creation, the user is responsible for assigning an unique
 * address to the device.
 *
 * By default the device is in Broadcast mode, with infinite bandwidth.
 *
 * @brief simple net device for simple things and testing
 */
class MockNetDevice : public NetDevice
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    MockNetDevice();

    /**
     * Pretend that a packet has been received from a connected Channel.
     *
     * Note that no analysis is performed on the addresses, and the
     * packet is forwarded to the callbacks according to the packetType.
     *
     * @param packet Packet received on the channel
     * @param protocol protocol number
     * @param to address packet should be sent to
     * @param from address packet was sent from
     * @param packetType type of the packet (e.g., NetDevice::PACKET_HOST,
     * NetDevice::PACKET_OTHERHOST, etc.)
     */
    void Receive(Ptr<Packet> packet,
                 uint16_t protocol,
                 Address to,
                 Address from,
                 NetDevice::PacketType packetType);

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
    Address GetMulticast(Ipv6Address addr) const override;
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
    void SetPromiscReceiveCallback(PromiscReceiveCallback cb) override;
    bool SupportsSendFrom() const override;

    /**
     *
     * Add a callback to be invoked when the MockNetDevice has a packet to "send".
     *
     * In the callback the PacketType is always set to NetDevice::PACKET_HOST.
     *
     * @param cb callback to invoke whenever the MockNetDevice has one packet to "send".
     *
     */
    void SetSendCallback(PromiscReceiveCallback cb);

  protected:
    void DoDispose() override;

  private:
    NetDevice::ReceiveCallback m_rxCallback;             //!< Receive callback
    NetDevice::PromiscReceiveCallback m_promiscCallback; //!< Promiscuous receive callback
    NetDevice::PromiscReceiveCallback m_sendCallback;    //!< Send callback
    Ptr<Node> m_node;                                    //!< Node this netDevice is associated to
    uint16_t m_mtu;                                      //!< MTU
    uint32_t m_ifIndex;                                  //!< Interface index
    Address m_address;                                   //!< MAC address

    bool m_linkUp;           //!< Flag indicating whether or not the link is up
    bool m_pointToPointMode; //!< Enabling this will disable Broadcast and Arp.

    /**
     * List of callbacks to fire if the link changes state (up or down).
     */
    TracedCallback<> m_linkChangeCallbacks;
};

} // namespace ns3

#endif /* MOCK_NET_DEVICE_H */
