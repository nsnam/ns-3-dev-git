/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef LOOPBACK_NET_DEVICE_H
#define LOOPBACK_NET_DEVICE_H

#include "ns3/mac48-address.h"
#include "ns3/net-device.h"

#include <stdint.h>
#include <string>

namespace ns3
{

class Node;

/**
 * \ingroup netdevice
 * \ingroup internet
 *
 * \brief Virtual network interface that loops back any data sent to it to
 * be immediately received on the same interface.
 *
 * This NetDevice is automatically added to any node as soon as the Internet
 * stack is initialized.
 */
class LoopbackNetDevice : public NetDevice
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    LoopbackNetDevice();

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
    /**
     * Receive a packet from tge Loopback NetDevice.
     *
     * \param packet a reference to the received packet
     * \param protocol the protocol
     * \param to destination address
     * \param from source address
     */
    void Receive(Ptr<Packet> packet, uint16_t protocol, Mac48Address to, Mac48Address from);

    /**
     * The callback used to notify higher layers that a packet has been received.
     */
    NetDevice::ReceiveCallback m_rxCallback;

    /**
     * The callback used to notify higher layers that a packet has been received in promiscuous
     * mode.
     */
    NetDevice::PromiscReceiveCallback m_promiscCallback;

    Ptr<Node> m_node;       //!< the node this NetDevice is associated with
    uint16_t m_mtu;         //!< device MTU
    uint32_t m_ifIndex;     //!< interface index
    Mac48Address m_address; //!< NetDevice MAC address
};

} // namespace ns3

#endif /* LOOPBACK_NET_DEVICE_H */
