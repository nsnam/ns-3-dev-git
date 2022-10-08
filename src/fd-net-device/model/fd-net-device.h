/*
 * Copyright (c) 2012 INRIA, 2012 University of Washington
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
 * Author: Alina Quereilhac <alina.quereilhac@inria.fr>
 *         Claudio Freire <klaussfreire@sourceforge.net>
 */

#ifndef FD_NET_DEVICE_H
#define FD_NET_DEVICE_H

#include "ns3/address.h"
#include "ns3/callback.h"
#include "ns3/data-rate.h"
#include "ns3/event-id.h"
#include "ns3/fd-reader.h"
#include "ns3/mac48-address.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"

#include <mutex>
#include <queue>
#include <utility>

namespace ns3
{

/**
 * \defgroup fd-net-device File Descriptor Network Device
 * This section documents the API of the ns-3 fd-net-device module.
 * For a generic functional description, please refer to the ns-3 manual.
 */

/**
 * \ingroup fd-net-device
 * \brief This class performs the actual data reading from the sockets.
 */
class FdNetDeviceFdReader : public FdReader
{
  public:
    FdNetDeviceFdReader();

    /**
     * Set size of the read buffer.
     * \param bufferSize the buffer size
     */
    void SetBufferSize(uint32_t bufferSize);

  private:
    FdReader::Data DoRead() override;

    uint32_t m_bufferSize; //!< size of the read buffer
};

class Node;

/**
 * \ingroup fd-net-device
 *
 * \brief a NetDevice to read/write network traffic from/into a file descriptor.
 *
 * A FdNetDevice object will read and write frames/packets from/to a file descriptor.
 * This file descriptor might be associated to a Linux TAP/TUN device, to a socket
 * or to a user space process, allowing the simulation to exchange traffic with the
 * "outside-world"
 *
 */
class FdNetDevice : public NetDevice
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Enumeration of the types of frames supported in the class.
     */
    enum EncapsulationMode
    {
        DIX,   /**< DIX II / Ethernet II packet */
        LLC,   /**< 802.2 LLC/SNAP Packet*/
        DIXPI, /**< When using TAP devices, if flag
                    IFF_NO_PI is not set on the device,
                    IP packets will have an extra header:
                    Flags [2 bytes]
                    Proto [2 bytes]
                    Raw protocol(IP, IPv6, etc) frame. */
    };

    /**
     * Constructor for the FdNetDevice.
     */
    FdNetDevice();

    /**
     * Destructor for the FdNetDevice.
     */
    ~FdNetDevice() override;

    // Delete assignment operator to avoid misuse
    FdNetDevice(const FdNetDevice&) = delete;

    /**
     * Set the link layer encapsulation mode of this device.
     *
     * \param mode The link layer encapsulation mode of this device.
     *
     */
    void SetEncapsulationMode(FdNetDevice::EncapsulationMode mode);

    /**
     * Get the link layer encapsulation mode of this device.
     *
     * \returns The link layer encapsulation mode of this device.
     */
    FdNetDevice::EncapsulationMode GetEncapsulationMode() const;

    /**
     * Set the associated file descriptor.
     * \param fd the file descriptor
     */
    void SetFileDescriptor(int fd);

    /**
     * Set a start time for the device.
     *
     * @param tStart the start time
     */
    void Start(Time tStart);

    /**
     * Set a stop time for the device.
     *
     * @param tStop the stop time
     */
    void Stop(Time tStop);

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
    void SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb) override;
    bool SupportsSendFrom() const override;
    Address GetMulticast(Ipv6Address addr) const override;

    /**
     * Set if the NetDevice is able to send Broadcast messages
     * \param broadcast true if the NetDevice can send Broadcast
     */
    virtual void SetIsBroadcast(bool broadcast);
    /**
     * Set if the NetDevice is able to send Multicast messages
     * \param multicast true if the NetDevice can send Multicast
     */
    virtual void SetIsMulticast(bool multicast);

    /**
     * Write packet data to device.
     * \param buffer The data.
     * \param length The data length.
     * \return The size of data written.
     */
    virtual ssize_t Write(uint8_t* buffer, size_t length);

  protected:
    /**
     * Method Initialization for start and stop attributes.
     */
    void DoInitialize() override;

    void DoDispose() override;

    /**
     * Get the associated file descriptor.
     * \return the associated file descriptor
     */
    int GetFileDescriptor() const;

    /**
     * Allocate packet buffer.
     * \param len the length of the buffer
     * \return A pointer to the newly allocated buffer.
     */
    virtual uint8_t* AllocateBuffer(size_t len);

    /**
     * Free the given packet buffer.
     * \param buf the buffer to free
     */
    virtual void FreeBuffer(uint8_t* buf);

    /**
     * Callback to invoke when a new frame is received
     * \param buf a buffer containing the received frame
     * \param len the length of the frame
     */
    void ReceiveCallback(uint8_t* buf, ssize_t len);

    /**
     * Mutex to increase pending read counter.
     */
    std::mutex m_pendingReadMutex;

    /**
     * Number of packets that were received and scheduled for read but not yet read.
     */
    std::queue<std::pair<uint8_t*, ssize_t>> m_pendingQueue;

  private:
    /**
     * Spin up the device
     */
    void StartDevice();

    /**
     * Tear down the device
     */
    void StopDevice();

    /**
     * Create the FdReader object
     * \return the created FdReader object
     */
    virtual Ptr<FdReader> DoCreateFdReader();

    /**
     * Complete additional actions, if any, to spin up down the device
     */
    virtual void DoFinishStartingDevice();

    /**
     * Complete additional actions, if any, to tear down the device
     */
    virtual void DoFinishStoppingDevice();

    /**
     * Forward the frame to the appropriate callback for processing
     */
    void ForwardUp();

    /**
     * Start Sending a Packet Down the Wire.
     * @param p packet to send
     * @returns true if success, false on failure
     */
    bool TransmitStart(Ptr<Packet> p);

    /**
     * Notify that the link is up and ready
     */
    void NotifyLinkUp();

    /**
     * The ns-3 node associated to the net device.
     */
    Ptr<Node> m_node;

    /**
     * a copy of the node id so the read thread doesn't have to GetNode() in
     * in order to find the node ID.  Thread unsafe reference counting in
     * multithreaded apps is not a good thing.
     */
    uint32_t m_nodeId;

    /**
     * The ns-3 interface index (in the sense of net device index) that has been assigned to this
     * network device.
     */
    uint32_t m_ifIndex;

    /**
     * The MTU associated to the file descriptor technology
     */
    uint16_t m_mtu;

    /**
     * The file descriptor used for receive/send network traffic.
     */
    int m_fd;

    /**
     * Reader for the file descriptor.
     */
    Ptr<FdReader> m_fdReader;

    /**
     * The net device mac address.
     */
    Mac48Address m_address;

    /**
     * The type of encapsulation of the received/transmitted frames.
     */
    EncapsulationMode m_encapMode;

    /**
     * Flag indicating whether or not the link is up.  In this case,
     * whether or not the device is connected to a channel.
     */
    bool m_linkUp;

    /**
     * Callbacks to fire if the link changes state (up or down).
     */
    TracedCallback<> m_linkChangeCallbacks;

    /**
     * Flag indicating whether or not the underlying net device supports
     * broadcast.
     */
    bool m_isBroadcast;

    /**
     * Flag indicating whether or not the underlying net device supports
     * multicast.
     */
    bool m_isMulticast;

    /**
     * Maximum number of packets that can be received and scheduled for read but not yet read.
     */
    uint32_t m_maxPendingReads;

    /**
     * Time to start spinning up the device
     */
    Time m_tStart;

    /**
     * Time to start tearing down the device
     */
    Time m_tStop;

    /**
     * NetDevice start event
     */
    EventId m_startEvent;
    /**
     * NetDevice stop event
     */
    EventId m_stopEvent;

    /**
     * The callback used to notify higher layers that a packet has been received.
     */
    NetDevice::ReceiveCallback m_rxCallback;

    /**
     * The callback used to notify higher layers that a packet has been received in promiscuous
     * mode.
     */
    NetDevice::PromiscReceiveCallback m_promiscRxCallback;

    /**
     * The trace source fired when packets come into the "top" of the device
     * at the L3/L2 transition, before being queued for transmission.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxTrace;

    /**
     * The trace source fired when packets coming into the "top" of the device
     * at the L3/L2 transition are dropped before being queued for transmission.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxDropTrace;

    /**
     * The trace source fired for packets successfully received by the device
     * immediately before being forwarded up to higher layers (at the L2/L3
     * transition).  This is a promiscuous trace.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macPromiscRxTrace;

    /**
     * The trace source fired for packets successfully received by the device
     * immediately before being forwarded up to higher layers (at the L2/L3
     * transition).  This is a non-promiscuous trace.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macRxTrace;

    /**
     * The trace source fired for packets successfully received by the device
     * but which are dropped before being forwarded up to higher layers (at the
     * L2/L3 transition).
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macRxDropTrace;

    /**
     * The trace source fired when the phy layer drops a packet as it tries
     * to transmit it.
     *
     * \todo Remove: this TracedCallback is never invoked.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_phyTxDropTrace;

    /**
     * The trace source fired when the phy layer drops a packet it has received.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_phyRxDropTrace;

    /**
     * A trace source that emulates a non-promiscuous protocol sniffer connected
     * to the device.  Unlike your average everyday sniffer, this trace source
     * will not fire on PACKET_OTHERHOST events.
     *
     * On the transmit size, this trace hook will fire after a packet is dequeued
     * from the device queue for transmission.  In Linux, for example, this would
     * correspond to the point just before a device hard_start_xmit where
     * dev_queue_xmit_nit is called to dispatch the packet to the PF_PACKET
     * ETH_P_ALL handlers.
     *
     * On the receive side, this trace hook will fire when a packet is received,
     * just before the receive callback is executed.  In Linux, for example,
     * this would correspond to the point at which the packet is dispatched to
     * packet sniffers in netif_receive_skb.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_snifferTrace;

    /**
     * A trace source that emulates a promiscuous mode protocol sniffer connected
     * to the device.  This trace source fire on packets destined for any host
     * just like your average everyday packet sniffer.
     *
     * On the transmit size, this trace hook will fire after a packet is dequeued
     * from the device queue for transmission.  In Linux, for example, this would
     * correspond to the point just before a device hard_start_xmit where
     * dev_queue_xmit_nit is called to dispatch the packet to the PF_PACKET
     * ETH_P_ALL handlers.
     *
     * On the receive side, this trace hook will fire when a packet is received,
     * just before the receive callback is executed.  In Linux, for example,
     * this would correspond to the point at which the packet is dispatched to
     * packet sniffers in netif_receive_skb.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_promiscSnifferTrace;
};

} // namespace ns3

#endif /* FD_NET_DEVICE_H */
