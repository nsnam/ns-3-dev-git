/*
 * Copyright (c) 2010 TELEMATICS LAB, DEE - Politecnico di Bari
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
 * Author: Giuseppe Piro <g.piro@poliba.it>
 *         Nicola Baldo  <nbaldo@cttc.es>
 */

#ifndef LTE_NET_DEVICE_H
#define LTE_NET_DEVICE_H

#include <ns3/event-id.h>
#include <ns3/mac64-address.h>
#include <ns3/net-device.h>
#include <ns3/nstime.h>
#include <ns3/traced-callback.h>

namespace ns3
{

class Node;
class Packet;

/**
 * \defgroup lte LTE Models
 *
 */

/**
 * \ingroup lte
 *
 * LteNetDevice provides  basic implementation for all LTE network devices
 */
class LteNetDevice : public NetDevice
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    LteNetDevice();
    ~LteNetDevice() override;

    // Delete copy constructor and assignment operator to avoid misuse
    LteNetDevice(const LteNetDevice&) = delete;
    LteNetDevice& operator=(const LteNetDevice&) = delete;

    void DoDispose() override;

    // inherited from NetDevice
    void SetIfIndex(const uint32_t index) override;
    uint32_t GetIfIndex() const override;
    Ptr<Channel> GetChannel() const override;
    bool SetMtu(const uint16_t mtu) override;
    uint16_t GetMtu() const override;
    void SetAddress(Address address) override;
    Address GetAddress() const override;
    bool IsLinkUp() const override;
    void AddLinkChangeCallback(Callback<void> callback) override;
    bool IsBroadcast() const override;
    Address GetBroadcast() const override;
    bool IsMulticast() const override;
    bool IsPointToPoint() const override;
    bool IsBridge() const override;
    Ptr<Node> GetNode() const override;
    void SetNode(Ptr<Node> node) override;
    bool NeedsArp() const override;
    void SetReceiveCallback(NetDevice::ReceiveCallback cb) override;
    Address GetMulticast(Ipv4Address addr) const override;
    Address GetMulticast(Ipv6Address addr) const override;
    void SetPromiscReceiveCallback(PromiscReceiveCallback cb) override;
    bool SendFrom(Ptr<Packet> packet,
                  const Address& source,
                  const Address& dest,
                  uint16_t protocolNumber) override;
    bool SupportsSendFrom() const override;

    /**
     * receive a packet from the lower layers in order to forward it to the upper layers
     *
     * \param p the packet
     */
    void Receive(Ptr<Packet> p);

  protected:
    NetDevice::ReceiveCallback m_rxCallback; ///< receive callback

  private:
    Ptr<Node> m_node; ///< the node

    TracedCallback<> m_linkChangeCallbacks; ///< link change callback

    uint32_t m_ifIndex;     ///< interface index
    bool m_linkUp;          ///< link uo
    mutable uint16_t m_mtu; ///< MTU

    Mac64Address m_address; ///< MAC address - only relevant for UEs.
};

} // namespace ns3

#endif /* LTE_NET_DEVICE_H */
