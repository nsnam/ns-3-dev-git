/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
 * Modified by Emmanuelle Laprise to remove dependence on LLC headers
 */
#ifndef NET_DEVICE_H
#define NET_DEVICE_H

#include <string>
#include <vector>
#include <stdint.h>
#include "ns3/callback.h"
#include "ns3/object.h"
#include "ns3/ptr.h"
#include "address.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"

namespace ns3 {

class Node;
class Channel;
class Packet;
class NetDevice;

/**
 * \ingroup network
 * \defgroup netdevice Network Device
 */
/**
 * \ingroup netdevice
 *
 * \brief Network device transmission queue
 *
 * This class stores information about a single transmission queue
 * of a network device that is exposed to queue discs. Such information
 * includes the state of the transmission queue (whether it has been
 * stopped or not) and data used by techniques such as Byte Queue Limits.
 * Also, multi-queue aware queue discs can aggregate a child queue disc
 * to an object of this class.
 *
 * This class roughly models the struct netdev_queue of Linux.
 * \todo Implement BQL
 */
class NetDeviceQueue : public SimpleRefCount<NetDeviceQueue>
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  NetDeviceQueue ();
  virtual ~NetDeviceQueue();

  /**
   * \return the device which this transmission queue belongs to.
   */
  inline Ptr<NetDevice> GetDevice (void) const;

  /**
   * \param device the device which this transmission queue belongs to.
   *
   * The device can be set just once.
   */
  virtual void SetDevice (Ptr<NetDevice> device);

  /**
   * \return true if the (hardware) transmission queue is stopped.
   *
   * Called by queue discs to enquire about the status of a given transmission queue.
   * This is the analogous to the netif_tx_queue_stopped function of the Linux kernel.
   */
  inline bool IsStopped (void) const;

  /**
   * Called by the device or the stack to start this (hardware) transmission queue.
   * This is the analogous to the netif_tx_start_queue function of the Linux kernel.
   */
  virtual void Start (void);

  /**
   * Called by the device or the stack to stop this (hardware) transmission queue.
   * This is the analogous to the netif_tx_stop_queue function of the Linux kernel.
   */
  virtual void Stop (void);

  /**
   * Called by the device or the stack to wake this (hardware) transmission queue.
   * This is the analogous to the netif_tx_wake_queue function of the Linux kernel.
   */
  virtual void Wake (void);

  typedef Callback< void > WakeCallback;

  /**
   * \param cb callback to invoke whenever it is needed to "wake" the upper layers (i.e.,
   *        solicitate the queue disc aggregated to this transmission queue (in case of
   *        multi-queue aware queue discs) or to the network device (otherwise) to send
   *        packets down to the device).
   *
   * Set the callback to be used to wake the upper layers.
   */
  virtual void SetWakeCallback (WakeCallback cb);

  /**
   * \return true if the wake callback has been set.
   *
   * Used to check whether a queue disc has been installed on the device.
   */
  virtual bool HasWakeCallbackSet (void) const;

private:
  Ptr<NetDevice> m_device;
  bool m_stopped;
  WakeCallback m_wakeCallback;
};

/**
 * \ingroup netdevice
 *
 * \brief Network layer to device interface
 *
 * This interface defines the API which the IP and ARP
 * layers need to access to manage an instance of a network device 
 * layer. It currently does not support MAC-level 
 * multicast but this should not be too hard to add by adding
 * extra methods to register MAC multicast addresses to
 * filter out unwanted packets before handing them to the
 * higher layers.
 *
 * In Linux, this interface is analogous to the interface
 * just above dev_queue_xmit() (i.e., IP packet is fully
 * constructed with destination MAC address already selected).
 * 
 * If you want to write a new MAC layer, you need to subclass
 * this base class and implement your own version of the
 * pure virtual methods in this class.
 *
 * This class was designed to hide as many MAC-level details as 
 * possible from the perspective of layer 3 to allow a single layer 3
 * to work with any kind of MAC layer. Specifically, this class 
 * encapsulates the specific format of MAC addresses used by a
 * device such that the layer 3 does not need any modification
 * to handle new address formats. This means obviously that the
 * NetDevice class must know about the address format of all potential 
 * layer 3 protocols through its GetMulticast methods: the current
 * API has been optimized to make it easy to add new MAC protocols,
 * not to add new layer 3 protocols.
 */
class NetDevice : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  NetDevice ();
  virtual ~NetDevice();

  /**
   * \param index ifIndex of the device 
   */
  virtual void SetIfIndex (const uint32_t index) = 0;
  /**
   * \return index ifIndex of the device 
   */
  virtual uint32_t GetIfIndex (void) const = 0;


  /**
   * \return the channel this NetDevice is connected to. The value
   *         returned can be zero if the NetDevice is not yet connected
   *         to any channel or if the underlying NetDevice has no
   *         concept of a channel. i.e., callers _must_ check for zero
   *         and be ready to handle it.
   */
  virtual Ptr<Channel> GetChannel (void) const = 0;

  /**
   * Set the address of this interface
   * \param address address to set
   */
  virtual void SetAddress (Address address) = 0;

  /**
   * \return the current Address of this interface.
   */
  virtual Address GetAddress (void) const = 0;

  /**
   * \param mtu MTU value, in bytes, to set for the device
   * \return whether the MTU value was within legal bounds
   * 
   * Override for default MTU defined on a per-type basis.
   */
  virtual bool SetMtu (const uint16_t mtu) = 0;
  /**
   * \return the link-level MTU in bytes for this interface.
   * 
   * This value is typically used by the IP layer to perform
   * IP fragmentation when needed.
   */
  virtual uint16_t GetMtu (void) const = 0;
  /**
   * \return true if link is up; false otherwise
   */
  virtual bool IsLinkUp (void) const = 0;
  /**
   * TracedCallback signature for link changed event.
   */
  typedef void (* LinkChangeTracedCallback) (void);
  /**
   * \param callback the callback to invoke
   *
   * Add a callback invoked whenever the link 
   * status changes to UP. This callback is typically used
   * by the IP/ARP layer to flush the ARP cache and by IPv6 stack
   * to flush NDISC cache whenever the link goes up.
   */
  virtual void AddLinkChangeCallback (Callback<void> callback) = 0;
  /**
   * \return true if this interface supports a broadcast address,
   *         false otherwise.
   */
  virtual bool IsBroadcast (void) const = 0;
  /**
   * \return the broadcast address supported by
   *         this netdevice.
   *
   * Calling this method is invalid if IsBroadcast returns
   * not true.
   */
  virtual Address GetBroadcast (void) const = 0;

  /**
   * \return value of m_isMulticast flag
   */
  virtual bool IsMulticast (void) const = 0;

  /**
   * \brief Make and return a MAC multicast address using the provided
   *        multicast group
   *
   * \RFC{1112} says that an Ipv4 host group address is mapped to an Ethernet 
   * multicast address by placing the low-order 23-bits of the IP address into 
   * the low-order 23 bits of the Ethernet multicast address 
   * 01-00-5E-00-00-00 (hex).  Similar RFCs exist for Ipv6 and Eui64 mappings.
   * This method performs the multicast address creation function appropriate
   * to the underlying MAC address of the device.  This MAC address is
   * encapsulated in an abstract Address to avoid dependencies on the exact
   * MAC address format.
   *
   * In the case of net devices that do not support
   * multicast, clients are expected to test NetDevice::IsMulticast and avoid
   * attempting to map multicast packets.  Subclasses of NetDevice that do
   * support multicasting are expected to override this method and provide an
   * implementation appropriate to the particular device.
   *
   * \param multicastGroup The IP address for the multicast group destination
   * of the packet.
   * \return The MAC multicast Address used to send packets to the provided
   * multicast group.
   *
   * \warning Calling this method is invalid if IsMulticast returns not true.
   * \see IsMulticast()
   */
  virtual Address GetMulticast (Ipv4Address multicastGroup) const = 0;

  /**
   * \brief Get the MAC multicast address corresponding
   * to the IPv6 address provided.
   * \param addr IPv6 address
   * \return the MAC multicast address
   * \warning Calling this method is invalid if IsMulticast returns not true.
   */
  virtual Address GetMulticast (Ipv6Address addr) const = 0;

  /**
   * \brief Return true if the net device is acting as a bridge.
   *
   * \return value of m_isBridge flag
   */
  virtual bool IsBridge (void) const = 0;

  /**
   * \brief Return true if the net device is on a point-to-point link.
   *
   * \return value of m_isPointToPoint flag
   */
  virtual bool IsPointToPoint (void) const = 0;
  /**
   * \param packet packet sent from above down to Network Device
   * \param dest mac address of the destination (already resolved)
   * \param protocolNumber identifies the type of payload contained in
   *        this packet. Used to call the right L3Protocol when the packet
   *        is received.
   * 
   *  Called from higher layer to send packet into Network Device
   *  to the specified destination Address
   * 
   * \return whether the Send operation succeeded 
   */
  virtual bool Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) = 0;
  /**
   * \param packet packet sent from above down to Network Device
   * \param source source mac address (so called "MAC spoofing")
   * \param dest mac address of the destination (already resolved)
   * \param protocolNumber identifies the type of payload contained in
   *        this packet. Used to call the right L3Protocol when the packet
   *        is received.
   * 
   *  Called from higher layer to send packet into Network Device
   *  with the specified source and destination Addresses.
   * 
   * \return whether the Send operation succeeded 
   */
  virtual bool SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber) = 0;
  /**
   * \returns the node base class which contains this network
   *          interface.
   *
   * When a subclass needs to get access to the underlying node
   * base class to print the nodeid for example, it can invoke
   * this method.
   */
  virtual Ptr<Node> GetNode (void) const = 0;

  /**
   * \param node the node associated to this netdevice.
   *
   * This method is called from ns3::Node::AddDevice.
   */
  virtual void SetNode (Ptr<Node> node) = 0;

  /**
   * \returns true if ARP is needed, false otherwise.
   *
   * Called by higher-layers to check if this NetDevice requires
   * ARP to be used.
   */
  virtual bool NeedsArp (void) const = 0;


  /** 
   * Packet types are used as they are in Linux.  GCC name resolution on 
   * typedef enum {} PacketType is broken for the foreseeable future, so
   * if you need to use ns-3 PacketType in a driver that also uses the 
   * Linux packet types you're hosed unless we define a shadow type, 
   * which we do here.
   */
  enum PacketType
  {
    PACKET_HOST = 1,   /**< Packet addressed oo us */
    NS3_PACKET_HOST = PACKET_HOST,
    PACKET_BROADCAST,   /**< Packet addressed to all */
    NS3_PACKET_BROADCAST = PACKET_BROADCAST,
    PACKET_MULTICAST,   /**< Packet addressed to multicast group */
    NS3_PACKET_MULTICAST = PACKET_MULTICAST,
    PACKET_OTHERHOST,   /**< Packet addressed to someone else */
    NS3_PACKET_OTHERHOST = PACKET_OTHERHOST,
  };

  /**
   * \param device a pointer to the net device which is calling this callback
   * \param packet the packet received
   * \param protocol the 16 bit protocol number associated with this packet.
   *        This protocol number is expected to be the same protocol number
   *        given to the Send method by the user on the sender side.
   * \param sender the address of the sender
   * \returns true if the callback could handle the packet successfully, false
   *          otherwise.
   */
  typedef Callback< bool, Ptr<NetDevice>, Ptr<const Packet>, uint16_t, const Address & > ReceiveCallback;

  /**
   * \param cb callback to invoke whenever a packet has been received and must
   *        be forwarded to the higher layers.
   *
   * Set the callback to be used to notify higher layers when a packet has been
   * received.
   */
  virtual void SetReceiveCallback (ReceiveCallback cb) = 0;


  /**
   * \param device a pointer to the net device which is calling this callback
   * \param packet the packet received
   * \param protocol the 16 bit protocol number associated with this packet.
   *        This protocol number is expected to be the same protocol number
   *        given to the Send method by the user on the sender side.
   * \param sender the address of the sender
   * \param receiver the address of the receiver
   * \param packetType type of packet received (broadcast/multicast/unicast/otherhost)
   * \returns true if the callback could handle the packet successfully, false
   *          otherwise.
   */
  typedef Callback< bool, Ptr<NetDevice>, Ptr<const Packet>, uint16_t,
                    const Address &, const Address &, enum PacketType > PromiscReceiveCallback;

  /**
   * \param cb callback to invoke whenever a packet has been received in promiscuous mode and must
   *        be forwarded to the higher layers.
   * 
   * Enables netdevice promiscuous mode and sets the callback that
   * will handle promiscuous mode packets.  Note, promiscuous mode
   * packets means _all_ packets, including those packets that can be
   * sensed by the netdevice but which are intended to be received by
   * other hosts.
   */
  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb) = 0;

  /**
   * \return true if this interface supports a bridging mode, false otherwise.
   */
  virtual bool SupportsSendFrom (void) const = 0;

  /**
   * \return the i-th transmission queue of the device.
   *
   * The index of the first transmission queue is zero.
   */
  inline Ptr<NetDeviceQueue> GetTxQueue (uint8_t i) const;

  /**
   * \return the number of (hardware) transmission queues.
   */
  inline uint8_t GetTxQueuesN (void) const;

  /**
   * \return the id of the transmission queue selected for the given packet
   */
  virtual uint8_t GetSelectedQueue (Ptr<Packet> packet) const;

protected:
  /**
   * \param numTxQueues number of (hardware) transmission queues.
   *
   * Modifying the number of transmission queues is only permitted before a queue
   * disc is aggregated to the device. This is because we have to set a wake
   * callback for each transmission queue, and this is done when setting up
   * queue discs.
   */
  virtual void SetTxQueuesN (uint8_t numTxQueues);

private:
  std::vector< Ptr<NetDeviceQueue> > m_txQueuesVector;
};

} // namespace ns3


namespace ns3 {

Ptr<NetDevice>
NetDeviceQueue::GetDevice (void) const
{
  return m_device;
}

bool
NetDeviceQueue::IsStopped (void) const
{
  return m_stopped;
}

Ptr<NetDeviceQueue>
NetDevice::GetTxQueue (uint8_t i) const
{
  NS_ASSERT (i < m_txQueuesVector.size ());
  return m_txQueuesVector[i];
}

uint8_t
NetDevice::GetTxQueuesN (void) const
{
  return m_txQueuesVector.size ();
}

} // namespace ns3

#endif /* NET_DEVICE_H */
