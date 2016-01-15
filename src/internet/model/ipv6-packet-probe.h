/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Bucknell University
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
 * Authors: L. Felipe Perrone (perrone@bucknell.edu)
 *          Tiago G. Rodrigues (tgr002@bucknell.edu)
 *
 * Modified by: Mitch Watrous (watrous@u.washington.edu)
 * Adapted to Ipv6 by: Tommaso Pecorella (tommaso.pecorella@unifi.it)
 */

#ifndef IPV6_PACKET_PROBE_H
#define IPV6_PACKET_PROBE_H

#include "ns3/object.h"
#include "ns3/callback.h"
#include "ns3/boolean.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/ipv6.h"
#include "ns3/ipv6-header.h"
#include "ns3/traced-value.h"
#include "ns3/simulator.h"
#include "ns3/probe.h"

namespace ns3 {

/**
 * This class is designed to probe an underlying ns3 TraceSource exporting
 * an IPv6 header, a packet, an IPv6 object, and an interface.  This probe
 * exports a trace source "Output" with arguments of type const Ipv6Header&,
 * Ptr<const Packet>, Ptr<Ipv6>, and uint32_t.  The Output trace source emits
 * a value when either the trace source emits a new value, or when SetValue ()
 * is called.
 */
class Ipv6PacketProbe : public Probe
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId ();

  Ipv6PacketProbe ();
  virtual ~Ipv6PacketProbe ();

  /**
   * \brief Set a probe value
   *
   * \param header set the Ipv6 header equal to this
   * \param packet set the traced packet equal to this
   * \param ipv6 set the IPv6 object for the traced packet equal to this
   * \param interface set the IPv6 interface for the traced packet equal to this
   */
  void SetValue (const Ipv6Header & header, Ptr<const Packet> packet, Ptr<Ipv6> ipv6, uint32_t interface);

  /**
   * \brief Set a probe value by its name in the Config system
   *
   * \param path config path to access the probe
   * \param header set the Ipv6 header equal to this
   * \param packet set the traced packet equal to this
   * \param ipv6 set the IPv6 object for the traced packet equal to this
   * \param interface set the IPv6 interface for the traced packet equal to this
   */
  static void SetValueByPath (std::string path, const Ipv6Header & header, Ptr<const Packet> packet, Ptr<Ipv6> ipv6, uint32_t interface);

  /**
   * \brief connect to a trace source attribute provided by a given object
   *
   * \param traceSource the name of the attribute TraceSource to connect to
   * \param obj ns3::Object to connect to
   * \return true if the trace source was successfully connected
   */
  virtual bool ConnectByObject (std::string traceSource, Ptr<Object> obj);

  /**
   * \brief connect to a trace source provided by a config path
   *
   * \param path Config path to bind to
   *
   * Note, if an invalid path is provided, the probe will not be connected
   * to anything.
   */
  virtual void ConnectByPath (std::string path);

private:
  /**
   * \brief Method to connect to an underlying ns3::TraceSource with arguments
   * of type const Ipv6Header&, Ptr<const Packet>, Ptr<Ipv6>, and uint32_t
   *
   * \param header the Ipv6 header of the traced packet
   * \param packet the traced packet
   * \param ipv6 the IPv6 object for the traced packet
   * \param interface the IPv6 interface for the traced packet
   */
  void TraceSink (const Ipv6Header & header, Ptr<const Packet> packet, Ptr<Ipv6> ipv6, uint32_t interface);

  /// Traced Callback: the Ipv6 header, the packet, the Ipv6 object and the interface.
  ns3::TracedCallback<const Ipv6Header&, Ptr<const Packet>, Ptr<Ipv6>, uint32_t> m_output;
  /// Traced Callback: the previous packet's size and the actual packet's size.
  ns3::TracedCallback<uint32_t, uint32_t> m_outputBytes;

  /// The traced IPv6 header.
  Ipv6Header m_header;

  /// The traced packet.
  Ptr<const Packet> m_packet;

  /// The IPv6 object for the traced packet.
  Ptr<Ipv6> m_ipv6;

  /// The IPv6 interface for the traced packet.
  uint32_t m_interface;

  /// The size of the traced packet.
  uint32_t m_packetSizeOld;
};


} // namespace ns3

#endif // IPV6_PACKET_PROBE_H
