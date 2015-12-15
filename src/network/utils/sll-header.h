/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Université Pierre et Marie Curie
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
 * Author: Matthieu Coudron <matthieu.coudron@lip6.fr>
 */
#ifndef SLL_HEADER_H
#define SLL_HEADER_H

#include "ns3/buffer.h"
#include "ns3/header.h"
#include <stdint.h>

namespace ns3 {

/**
  * \ingroup packet
  *
  * \brief Protocol header serialization and deserialization.
  *
  * Libpcap sometimes add an additional header to provide information that would be
  * lost otherwise due to the link-layer/capture mechanism, for instance when capturing from
  * "nlmon" device on linux
  *
  * \see http://www.tcpdump.org/linktypes/LINKTYPE_LINUX_SLL.html
  * \see https://wiki.wireshark.org/SLL
  *
  * \verbatim
  * +---------------------------+
  * |         Packet type       |
  * |         (2 Octets)        |
  * +---------------------------+
  * |        ARPHRD_ type       |
  * |         (2 Octets)        |
  * +---------------------------+
  * | Link-layer address length |
  * |         (2 Octets)        |
  * +---------------------------+
  * |    Link-layer address     |
  * |         (8 Octets)        |
  * +---------------------------+
  * |        Protocol type      |
  * |         (2 Octets)        |
  * +---------------------------+
  * |           Payload         |
  * .                           .
  * .                           .
  * .                           .
  * \endverbatim
  */
class SllHeader : public Header
{
public:
  enum PacketType
  {
    UNICAST_FROM_PEER_TO_ME = 0, /**< the packet was specifically sent to us by somebody else */
    BROADCAST_BY_PEER = 1, /**< packet was broadcast by somebody else */
    MULTICAST_BY_PEER = 2, /**< packet was multicast, but not broadcast, by somebody else */
    INTERCEPTED_PACKET = 3, /**< packet was sent to somebody else by somebody else **/
    SENT_BY_US  /**< the packet was sent by us */
  };

  /**
    * \brief Get the type ID.
    * \return the object TypeId
    */
  static TypeId GetTypeId (void);
  TypeId GetInstanceTypeId (void) const;

  SllHeader ();
  virtual ~SllHeader ();

  /**
    *  The ARPHRD_ type field is in network byte order; it contains a Linux ARPHRD_ value for the link-layer device type.
    */
  uint16_t GetArpType () const;

  /**
    * \return Packet type
    */
  PacketType GetPacketType () const;

  /**
    * \param arphw ARP protocol hardware identifier
    */
  void SetArpType (uint16_t arphdType);

  /**
    * \param type Depends on source and address of the packet
    */
  void SetPacketType (PacketType type);

  //! Inherited
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  virtual void Print (std::ostream &os) const;

protected:
  // declared in packet order
  PacketType m_packetType;
  uint16_t m_arphdType;   /**< ARP protocol hardware identifier */
  uint16_t m_addressLength;
  uint64_t m_address;
  uint16_t m_protocolType;
};



std::ostream & operator << (std::ostream &os, const SllHeader &header);

} // namespace ns3

#endif /* SLL_HEADER_H */
