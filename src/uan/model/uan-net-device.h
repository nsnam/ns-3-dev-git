/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */

#ifndef UAN_NET_DEVICE_H
#define UAN_NET_DEVICE_H

#include "ns3/mac8-address.h"
#include "ns3/net-device.h"
#include "ns3/pointer.h"
#include "ns3/traced-callback.h"

#include <list>

namespace ns3
{

class UanChannel;
class UanPhy;
class UanMac;
class UanTransducer;

/**
 * @defgroup uan UAN Models
 * This section documents the API of the ns-3 UAN module. For a generic functional description,
 * please refer to the ns-3 manual.
 */

/**
 * @ingroup uan
 *
 * Net device for UAN models.
 */
class UanNetDevice : public NetDevice
{
  public:
    /** List of UanPhy objects. */
    typedef std::list<Ptr<UanPhy>> UanPhyList;
    /** List of UanTransducer objects. */
    typedef std::list<Ptr<UanTransducer>> UanTransducerList;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /** Default constructor */
    UanNetDevice();
    /** Dummy destructor, DoDispose. */
    ~UanNetDevice() override;

    /**
     * Set the MAC layer for this device.
     *
     * @param mac The MAC layer.
     */
    void SetMac(Ptr<UanMac> mac);

    /**
     * Set the Phy layer for this device.
     *
     * @param phy The PHY layer.
     */
    void SetPhy(Ptr<UanPhy> phy);

    /**
     * Attach a channel.
     *
     * @param channel The channel.
     */
    void SetChannel(Ptr<UanChannel> channel);

    /**
     * Get the MAC used by this device.
     *
     * @return The MAC.
     */
    Ptr<UanMac> GetMac() const;

    /**
     * Get the Phy used by this device.
     *
     * @return The Phy.
     */
    Ptr<UanPhy> GetPhy() const;

    /**
     * Get the transducer associated with this device.
     *
     * @return The transducer.
     */
    Ptr<UanTransducer> GetTransducer() const;
    /**
     * Set the transdcuer used by this device.
     *
     * @param trans The transducer.
     */
    void SetTransducer(Ptr<UanTransducer> trans);

    /** Clear all pointer references. */
    void Clear();

    /**
     * Set the Phy SLEEP mode.
     *
     * @param sleep SLEEP on or off.
     */
    void SetSleepMode(bool sleep);

    // Inherited methods
    void SetIfIndex(const uint32_t index) override;
    uint32_t GetIfIndex() const override;
    Ptr<Channel> GetChannel() const override;
    Address GetAddress() const override;
    bool SetMtu(const uint16_t mtu) override;
    uint16_t GetMtu() const override;
    bool IsLinkUp() const override;
    bool IsBroadcast() const override;
    Address GetBroadcast() const override;
    bool IsMulticast() const override;
    Address GetMulticast(Ipv4Address multicastGroup) const override;
    Address GetMulticast(Ipv6Address addr) const override;
    bool IsBridge() const override;
    bool IsPointToPoint() const override;
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
    void AddLinkChangeCallback(Callback<void> callback) override;
    void SetAddress(Address address) override;

    /**
     * Get the Tx mode index (Modulation type).
     * @return the Tx mode index
     */
    uint32_t GetTxModeIndex();

    /**
     * Set the Tx mode index (Modulation type).
     * @param txModeIndex the Tx mode index
     */
    void SetTxModeIndex(uint32_t txModeIndex);

    /**
     * TracedCallback signature for MAC send/receive events.
     *
     * @param [in] packet The Packet.
     * @param [in] address The source address.
     */
    typedef void (*RxTxTracedCallback)(Ptr<const Packet> packet, Mac8Address address);

  private:
    /**
     * Forward the packet to a higher level, set with SetReceiveCallback.
     *
     * @param pkt The packet.
     * @param src The source address.
     * @param protocolNumber The layer 3 protocol number.
     */
    virtual void ForwardUp(Ptr<Packet> pkt, uint16_t protocolNumber, const Mac8Address& src);

    /** @return The channel attached to this device. */
    Ptr<UanChannel> DoGetChannel() const;

    Ptr<UanTransducer> m_trans; //!< The Transducer attached to this device.
    Ptr<Node> m_node;           //!< The node hosting this device.
    Ptr<UanChannel> m_channel;  //!< The channel attached to this device.
    Ptr<UanMac> m_mac;          //!< The MAC layer attached to this device.
    Ptr<UanPhy> m_phy;          //!< The PHY layer attached to this device.

    // unused: std::string m_name;
    uint32_t m_ifIndex;             //!< The interface index of this device.
    uint16_t m_mtu;                 //!< The device MTU value, in bytes.
    bool m_linkup;                  //!< The link state, true if up.
    TracedCallback<> m_linkChanges; //!< Callback to invoke when the link state changes to UP.
    ReceiveCallback m_forwardUp;    //!< The receive callback.

    /** Trace source triggered when forwarding up received payload from the MAC layer. */
    TracedCallback<Ptr<const Packet>, Mac8Address> m_rxLogger;
    /** Trace source triggered when sending to the MAC layer */
    TracedCallback<Ptr<const Packet>, Mac8Address> m_txLogger;

    /** Flag when we've been cleared. */
    bool m_cleared;

  protected:
    void DoDispose() override;
    void DoInitialize() override;
};

} // namespace ns3

#endif /* UAN_NET_DEVICE_H */
