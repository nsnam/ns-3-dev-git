/*
 * Copyright (c) 2010
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef NON_COMMUNICATING_NET_DEVICE_H
#define NON_COMMUNICATING_NET_DEVICE_H

#include <ns3/address.h>
#include <ns3/callback.h>
#include <ns3/net-device.h>
#include <ns3/node.h>
#include <ns3/packet.h>
#include <ns3/ptr.h>
#include <ns3/traced-callback.h>

#include <cstring>

namespace ns3
{

class SpectrumChannel;
class Channel;
class SpectrumErrorModel;

/**
 * \ingroup spectrum
 *
 * This class implements a device which does not communicate, in the
 * sense that it does not interact with the above protocol stack. The
 * purpose of this NetDevice is to be used for devices such as
 * microwave ovens, waveform generators and spectrum
 * analyzers. Since the ns-3 channel API is strongly based on the presence of
 * NetDevice class instances, it is convenient to provide a NetDevice that can
 * be used with such non-communicating devices.
 */
class NonCommunicatingNetDevice : public NetDevice
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    NonCommunicatingNetDevice();
    ~NonCommunicatingNetDevice() override;

    /**
     * This class doesn't talk directly with the underlying channel (a
     * dedicated PHY class is expected to do it), however the NetDevice
     * specification features a GetChannel() method. This method here
     * is therefore provide to allow NonCommunicatingNetDevice::GetChannel() to have
     * something meaningful to return.
     *
     * @param c the underlying channel
     */
    void SetChannel(Ptr<Channel> c);

    /**
     * Set the Phy object which is attached to this device.
     * This object is needed so that we can set/get attributes and
     * connect to trace sources of the PHY from the net device.
     *
     * @param phy the Phy object embedded within this device.
     */
    void SetPhy(Ptr<Object> phy);

    /**
     * @return a reference to the PHY object embedded in this NetDevice.
     */
    Ptr<Object> GetPhy() const;

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
    bool Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) override;
    bool SendFrom(Ptr<Packet> packet,
                  const Address& source,
                  const Address& dest,
                  uint16_t protocolNumber) override;
    Ptr<Node> GetNode() const override;
    void SetNode(Ptr<Node> node) override;
    bool NeedsArp() const override;
    void SetReceiveCallback(NetDevice::ReceiveCallback cb) override;
    Address GetMulticast(Ipv4Address addr) const override;
    Address GetMulticast(Ipv6Address addr) const override;
    void SetPromiscReceiveCallback(PromiscReceiveCallback cb) override;
    bool SupportsSendFrom() const override;

  private:
    void DoDispose() override;

    Ptr<Node> m_node;       //!< node this NetDevice is associated to
    Ptr<Channel> m_channel; //!< Channel used by the NetDevice
    uint32_t m_ifIndex;     //!< Interface index
    Ptr<Object> m_phy;      //!< Phy object
};

} // namespace ns3

#endif /* NON_COMMUNICATING_NET_DEVICE_H */
