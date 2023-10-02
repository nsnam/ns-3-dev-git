/*
 * Copyright (c) 2007,2008, 2009 INRIA, UDcast
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
 * Authors: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 *          Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */

#ifndef WIMAX_NET_DEVICE_H
#define WIMAX_NET_DEVICE_H

#include "cid-factory.h"
#include "cid.h"
#include "dl-mac-messages.h"
#include "mac-messages.h"
#include "ul-mac-messages.h"
#include "wimax-connection.h"
#include "wimax-mac-header.h"
#include "wimax-phy.h"

#include "ns3/event-id.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/net-device.h"
#include "ns3/nstime.h"
#include "ns3/traced-callback.h"

namespace ns3
{

class Node;
class Packet;
class TraceContext;
class TraceResolver;
class Channel;
class WimaxChannel;
class PacketBurst;
class BurstProfileManager;
class ConnectionManager;
class ServiceFlowManager;
class BandwidthManager;
class UplinkScheduler;

/**
 * \defgroup wimax WiMAX Models
 * This section documents the API of the ns-3 wimax module. For a generic functional description,
 * please refer to the ns-3 manual.
 */

/**
 * \brief Hold together all WiMAX-related objects in a NetDevice.
 * \ingroup wimax
 *
 * This class holds together ns3::WimaxPhy, ns3::WimaxConnection,
 * ns3::ConnectionManager, ns3::BurstProfileManager, and
 * ns3::BandwidthManager.
 */
class WimaxNetDevice : public NetDevice
{
  public:
    /// Direction enumeration
    enum Direction
    {
        DIRECTION_DOWNLINK,
        DIRECTION_UPLINK
    };

    /// RangingStatus enumeration
    enum RangingStatus
    {
        RANGING_STATUS_EXPIRED,
        RANGING_STATUS_CONTINUE,
        RANGING_STATUS_ABORT,
        RANGING_STATUS_SUCCESS
    };

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    WimaxNetDevice();
    ~WimaxNetDevice() override;
    /**
     * Set transmission/receive transition gap
     * \param ttg transmit/receive transition gap
     */
    void SetTtg(uint16_t ttg);
    /**
     * Get transmission/receive transition gap
     * \returns transmit/receive transition gap
     */
    uint16_t GetTtg() const;
    /**
     * Set receive/transmit transition gap
     * \param rtg receive/transmit transition gap
     */
    void SetRtg(uint16_t rtg);
    /**
     * Get receive/transmit transition gap
     * \returns receive/transmit transition gap
     */
    uint16_t GetRtg() const;
    /**
     * Attach device to channel
     * \param channel channel to attach
     */
    void Attach(Ptr<WimaxChannel> channel);
    /**
     * Set the physical layer object
     * \param phy the phy layer object to use
     */
    void SetPhy(Ptr<WimaxPhy> phy);
    /**
     * Get the physical layer object
     * \returns a pointer to the physical layer object
     */
    Ptr<WimaxPhy> GetPhy() const;

    /**
     * Set the channel object
     * \param wimaxChannel the channel to be used
     */
    void SetChannel(Ptr<WimaxChannel> wimaxChannel);

    /**
     * Get the channel object by index
     * \param index the channel index
     * \returns the channel being used.
     */
    uint64_t GetChannel(uint8_t index) const;

    /**
     * Set the number of frames
     * \param nrFrames the number of frames
     */
    void SetNrFrames(uint32_t nrFrames);
    /**
     * Get the number of frames
     * \returns the number of frames.
     */
    uint32_t GetNrFrames() const;
    /**
     * Set the MAC address
     * \param address the mac address of the net device
     */
    void SetMacAddress(Mac48Address address);
    /**
     * Get the MAC address
     * \returns the mac address of the net device
     */
    Mac48Address GetMacAddress() const;
    /**
     * Set the device state
     * \param state the state
     */
    void SetState(uint8_t state);
    /**
     * Get the device state
     * \returns the state
     */
    uint8_t GetState() const;
    /**
     * Get the initial ranging connection
     * \returns the initial ranging connection
     */
    Ptr<WimaxConnection> GetInitialRangingConnection() const;
    /**
     * Get the broadcast connection
     * \returns the broadcast connection
     */
    Ptr<WimaxConnection> GetBroadcastConnection() const;

    /**
     * Set the current DCD
     * \param dcd the DCD
     */
    void SetCurrentDcd(Dcd dcd);
    /**
     * Get the current DCD
     * \returns the DCD
     */
    Dcd GetCurrentDcd() const;
    /**
     * Set the current UCD
     * \param ucd the UCD
     */
    void SetCurrentUcd(Ucd ucd);
    /**
     * Get the current UCD
     * \returns the UCD
     */
    Ucd GetCurrentUcd() const;
    /**
     * Get the connection manager of the device
     * \returns the connection manager of the device
     */
    Ptr<ConnectionManager> GetConnectionManager() const;

    /**
     * Set the connection manager of the device
     * \param  connectionManager the connection manager to be installed in the device
     */
    virtual void SetConnectionManager(Ptr<ConnectionManager> connectionManager);

    /**
     * Get the burst profile manager
     * \returns the burst profile manager currently installed in the device
     */
    Ptr<BurstProfileManager> GetBurstProfileManager() const;

    /**
     * Set the burst profile manager
     * \param burstProfileManager the burst profile manager to be installed on the device
     */
    void SetBurstProfileManager(Ptr<BurstProfileManager> burstProfileManager);

    /**
     * Get the bandwidth manager on the device
     * \returns the bandwidth manager installed on the device
     */
    Ptr<BandwidthManager> GetBandwidthManager() const;

    /**
     * Set the bandwidth manager on the device
     * \param bandwidthManager the bandwidth manager to be installed on the device
     */
    void SetBandwidthManager(Ptr<BandwidthManager> bandwidthManager);

    /**
     * \brief Creates the initial ranging and broadcast connections
     */
    void CreateDefaultConnections();

    /// Start function
    virtual void Start() = 0;
    /// Stop function
    virtual void Stop() = 0;

    /// Set receive callback function
    void SetReceiveCallback();

    /**
     * Forward a packet to the next layer above the device
     * \param packet the packet
     * \param source the source MAC address
     * \param dest the destination MAC address
     */
    void ForwardUp(Ptr<Packet> packet, const Mac48Address& source, const Mac48Address& dest);

    /**
     * Enqueue a packet
     * \param packet the packet
     * \param hdrType the header type
     * \param connection the wimax connection
     * \returns true if successful
     */
    virtual bool Enqueue(Ptr<Packet> packet,
                         const MacHeaderType& hdrType,
                         Ptr<WimaxConnection> connection) = 0;
    /**
     * Forward a packet down the stack
     * \param burst the packet burst
     * \param modulationType the modulation type
     */
    void ForwardDown(Ptr<PacketBurst> burst, WimaxPhy::ModulationType modulationType);

    // temp, shall be private
    static uint8_t m_direction; ///< downlink or uplink

    static Time m_frameStartTime; ///< temp, to determine the frame start time at SS side, shall
                                  ///< actually be determined by frame start preamble

    /**
     * Set device name
     * \param name the device name
     */
    virtual void SetName(const std::string name);
    /**
     * Get device name
     * \returns the device name
     */
    virtual std::string GetName() const;
    /**
     * Set interface index
     * \param index the index
     */
    void SetIfIndex(const uint32_t index) override;
    /**
     * Get interface index
     * \returns the interface index
     */
    uint32_t GetIfIndex() const override;
    /**
     * Get the channel (this method is redundant with GetChannel())
     * \returns the channel used by the phy layer
     */
    virtual Ptr<Channel> GetPhyChannel() const;
    /**
     * Get the channel
     * \returns the channel
     */
    Ptr<Channel> GetChannel() const override;
    /**
     * Set address of the device
     * \param address the address
     */
    void SetAddress(Address address) override;
    /**
     * Get address of the device
     * \returns the address
     */
    Address GetAddress() const override;
    /**
     * Set MTU value for the device
     * \param mtu the MTU
     * \returns true if successful
     */
    bool SetMtu(const uint16_t mtu) override;
    /**
     * Get MTU of the device
     * \returns the MTU
     */
    uint16_t GetMtu() const override;
    /**
     * Check if link is up
     * \return true if the link is up
     */
    bool IsLinkUp() const override;
    /**
     * Set link change callback function
     * \param callback the callback function
     */
    virtual void SetLinkChangeCallback(Callback<void> callback);
    /**
     * Check if broadcast enabled
     * \returns true if broadcast
     */
    bool IsBroadcast() const override;
    /**
     * Get broadcast address
     * \returns the address
     */
    Address GetBroadcast() const override;
    /**
     * Check if multicast enabled
     * \returns true if multicast
     */
    bool IsMulticast() const override;
    /**
     * Get multicast address
     * \returns the multicast address
     */
    virtual Address GetMulticast() const;
    /**
     * Make multicast address
     * \param multicastGroup the IPv4 address
     * \returns the multicast address
     */
    virtual Address MakeMulticastAddress(Ipv4Address multicastGroup) const;
    /**
     * Check if device is a point-to-point device
     * \returns true if point to point
     */
    bool IsPointToPoint() const override;
    /**
     * Send function
     * \param packet the packet
     * \param dest the destination address
     * \param protocolNumber the protocol number
     * \returns true if successful
     */
    bool Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) override;
    /**
     * Set node pointer
     * \param node the node pointer
     */
    void SetNode(Ptr<Node> node) override;
    /**
     * Get node pointer
     * \returns the node pointer
     */
    Ptr<Node> GetNode() const override;
    /**
     * Check if device needs ARP
     * \returns true if ARP required
     */
    bool NeedsArp() const override;
    /**
     * Set receive callback function
     * \param cb the receive callback function
     */
    void SetReceiveCallback(NetDevice::ReceiveCallback cb) override;
    /**
     * Add link change callback function
     * \param callback the link change callback function
     */
    void AddLinkChangeCallback(Callback<void> callback) override;
    /**
     * Send a packet
     * \param packet the packet
     * \param source the source address
     * \param dest the destination address
     * \param protocolNumber the protocol number
     * \returns true if successful
     */
    bool SendFrom(Ptr<Packet> packet,
                  const Address& source,
                  const Address& dest,
                  uint16_t protocolNumber) override;
    /**
     * Set promiscuous receive callback function
     * \param cb the promiscuous mode callback
     */
    void SetPromiscReceiveCallback(PromiscReceiveCallback cb) override;
    /**
     * Get promiscuous receive callback function
     * \returns the promiscuous mode callback
     */
    NetDevice::PromiscReceiveCallback GetPromiscReceiveCallback();
    /**
     * Check if device supports the SendFrom method
     * \returns true if SendFrom is supported
     */
    bool SupportsSendFrom() const override;

    /**
     * TracedCallback signature for packet and Mac48Address.
     *
     * \param [in] packet The packet.
     * \param [in] mac The Mac48Address.
     * \deprecated The `const Mac48Address &` argument is deprecated
     * and will be changed to \c Mac48Address in a future release.
     * The TracedCallback signature will then match \c Packet::Mac48Address
     * and this typedef can be removed.
     */
    typedef void (*TxRxTracedCallback)(Ptr<const Packet> packet, const Mac48Address& mac);
    /**
     * \deprecated The `const Mac48Address &` argument is deprecated
     * and will be changed to \c Mac48Address in a future release.
     * The TracedCallback signature will then match \c Packet::Mac48Address
     * and this typedef can be removed.
     * \todo This member variable should be private.
     */
    TracedCallback<Ptr<const Packet>, const Mac48Address&> m_traceRx;
    /**
     * \deprecated The `const Mac48Address &` argument is deprecated
     * and will be changed to \c Mac48Address in a future release.
     * The TracedCallback signature will then match \c Packet::Mac48Address
     * and this typedef can be removed.
     * \todo This member variable should be private.
     */
    TracedCallback<Ptr<const Packet>, const Mac48Address&> m_traceTx;

    void DoDispose() override;
    Address GetMulticast(Ipv6Address addr) const override;
    Address GetMulticast(Ipv4Address multicastGroup) const override;
    bool IsBridge() const override;

    /**
     * Check if device is promiscuous
     * \returns true if promiscuous
     */
    bool IsPromisc();
    /**
     * Notify promiscuous trace of a packet arrival
     * \param p the packet
     */
    void NotifyPromiscTrace(Ptr<Packet> p);

  private:
    /// copy constructor (disabled)
    WimaxNetDevice(const WimaxNetDevice&);
    /**
     * assignment operator (disabled)
     * \returns the wimax net device
     */
    WimaxNetDevice& operator=(const WimaxNetDevice&);

    /// Maximum MSDU size
    static const uint16_t MAX_MSDU_SIZE = 1500;
    /// recommended by wimax forum.
    static const uint16_t DEFAULT_MSDU_SIZE = 1400;

    /**
     * Send a packet
     * \param packet the packet
     * \param source the source MAC address
     * \param dest the destination MAC address
     * \param protocolNumber the protocol number
     * \returns true if successful
     */
    virtual bool DoSend(Ptr<Packet> packet,
                        const Mac48Address& source,
                        const Mac48Address& dest,
                        uint16_t protocolNumber) = 0;
    /**
     * Receive a packet
     * \param packet the packet received
     */
    virtual void DoReceive(Ptr<Packet> packet) = 0;
    /**
     * Get the channel
     * \returns the wimax channel
     */
    virtual Ptr<WimaxChannel> DoGetChannel() const;
    /**
     * Receive a packet burst
     * \param burst the packet burst
     */
    void Receive(Ptr<const PacketBurst> burst);
    /// Initialize channels function
    void InitializeChannels();

    Ptr<Node> m_node;                              ///< the node
    Ptr<WimaxPhy> m_phy;                           ///< the phy
    NetDevice::ReceiveCallback m_forwardUp;        ///< forward up callback function
    NetDevice::PromiscReceiveCallback m_promiscRx; ///< promiscuous receive callback function

    uint32_t m_ifIndex;          ///< IF index
    std::string m_name;          ///< service name
    bool m_linkUp;               ///< link up?
    Callback<void> m_linkChange; ///< link change callback
    mutable uint16_t m_mtu;      ///< MTU

    /// temp, shall be in BS. defined here to allow SS to access. SS shall actually determine it
    /// from DLFP, shall be moved to BS after DLFP is implemented
    static uint32_t m_nrFrames;

    /// not sure if it shall be included here
    std::vector<uint64_t> m_dlChannels;

    Mac48Address m_address; ///< MAC address
    uint8_t m_state;        ///< state
    uint32_t m_symbolIndex; ///< symbol index

    /// length of TTG in units of PSs
    uint16_t m_ttg;
    /// length of RTG in units of PSs
    uint16_t m_rtg;

    Dcd m_currentDcd; ///< DCD
    Ucd m_currentUcd; ///< UCD

    Ptr<WimaxConnection> m_initialRangingConnection; ///< initial rnaging connection
    Ptr<WimaxConnection> m_broadcastConnection;      ///< broadcast connection

    Ptr<ConnectionManager> m_connectionManager;     ///< connection manager
    Ptr<BurstProfileManager> m_burstProfileManager; ///< burst profile manager
    Ptr<BandwidthManager> m_bandwidthManager;       ///< badnwidth manager
};

} // namespace ns3

#endif /* WIMAX_NET_DEVICE_H */
