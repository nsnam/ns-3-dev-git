/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Natale Patriciello <natale.patriciello@gmail.com>
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
 */
#ifndef TRAFFICCONTROLLAYER_H
#define TRAFFICCONTROLLAYER_H

#include "ns3/object.h"
#include "ns3/address.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/header.h"

namespace ns3 {

class Packet;

/**
 * \ingroup traffic-control
 *
 * \brief Traffic control layer definition
 *
 * This layer stays between NetDevices (L2) and any network protocol (e.g. IP).
 * When enabled, it is responsible to analyze packets and to perform actions on
 * them: the most common is scheduling.
 *
 * Basically, we manage both IN and OUT directions (sometimes called RX and TX,
 * respectively). The OUT direction is easy to follow, since it involves
 * direct calls: upper layer (e.g. IP) calls the Send method on an instance of
 * this class, which then calls the Enqueue method of the QueueDisc associated
 * with the device. The Dequeue method of the QueueDisc finally calls the Send
 * method of the NetDevice.
 *
 * The IN direction uses a little trick to reduce dependencies between modules.
 * In simple words, we use Callbacks to connect upper layer (which should register
 * their Receive callback through RegisterProtocolHandler) and NetDevices.
 *
 * An example of the IN connection between this layer and IP layer is the following:
 *\verbatim
  Ptr<TrafficControlLayer> tc = m_node->GetObject<TrafficControlLayer> ();

  NS_ASSERT (tc != 0);

  m_node->RegisterProtocolHandler (MakeCallback (&TrafficControlLayer::Receive, tc),
                                   Ipv4L3Protocol::PROT_NUMBER, device);
  m_node->RegisterProtocolHandler (MakeCallback (&TrafficControlLayer::Receive, tc),
                                   ArpL3Protocol::PROT_NUMBER, device);

  tc->RegisterProtocolHandler (MakeCallback (&Ipv4L3Protocol::Receive, this),
                               Ipv4L3Protocol::PROT_NUMBER, device);
  tc->RegisterProtocolHandler (MakeCallback (&ArpL3Protocol::Receive, PeekPointer (GetObject<ArpL3Protocol> ())),
                               ArpL3Protocol::PROT_NUMBER, device);
  \endverbatim
 * On the node, for IPv4 and ARP packet, is registered the
 * TrafficControlLayer::Receive callback. At the same time, on the TrafficControlLayer
 * object, is registered the callbacks associated to the upper layers (IPv4 or ARP).
 *
 * When the node receives an IPv4 or ARP packet, it calls the Receive method
 * on TrafficControlLayer, that calls the right upper-layer callback once it
 * finishes the operations on the packet received.
 *
 * Discrimination through callbacks (in other words: what is the right upper-layer
 * callback for this packet?) is done through checks over the device and the
 * protocol number.
 */
class TrafficControlLayer : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * \brief Get the type ID for the instance
   * \return the instance TypeId
   */
  virtual TypeId GetInstanceTypeId (void) const;

  /**
   * \brief Constructor
   */
  TrafficControlLayer ();

  /**
   * \brief Register an IN handler
   *
   * The handler will be invoked when a packet is received to pass it to
   * upper  layers.
   *
   * \param handler the handler to register
   * \param protocolType the type of protocol this handler is
   *        interested in. This protocol type is a so-called
   *        EtherType, as registered here:
   *        http://standards.ieee.org/regauth/ethertype/eth.txt
   *        the value zero is interpreted as matching all
   *        protocols.
   * \param device the device attached to this handler. If the
   *        value is zero, the handler is attached to all
   *        devices.
   */
  void RegisterProtocolHandler (Node::ProtocolHandler handler,
                                uint16_t protocolType,
                                Ptr<NetDevice> device);

  /**
   * \brief Called by NetDevices, incoming packet
   *
   * After analyses and scheduling, this method will call the right handler
   * to pass the packet up in the stack.
   *
   * \param device network device
   * \param p the packet
   * \param protocol next header value
   * \param from address of the correspondant
   * \param to address of the destination
   * \param packetType type of the packet
   */
  virtual void Receive (Ptr<NetDevice> device, Ptr<const Packet> p,
                        uint16_t protocol, const Address &from,
                        const Address &to, NetDevice::PacketType packetType);
  /**
   * \brief Called from upper layer to queue a packet for the transmission.
   *
   * \param packet packet sent from above
   * \param dest mac address of the destination (already resolved)
   * \param protocolNumber identifies the type of payload contained in
   *        this packet. Used to call the right L3Protocol when the packet
   *        is received.
   *
   */
  virtual void Send (Ptr<NetDevice> device, Ptr<Packet> packet, const Header & hdr,
                     const Address& dest, uint16_t protocolNumber);

protected:
  Node::ProtocolHandlerList m_handlers;  //!< List of upper-layer handlers
};

} // namespace ns3
#endif // TRAFFICCONTROLLAYER_H
