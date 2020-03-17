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
 */

#ifndef WIFI_NET_DEVICE_H
#define WIFI_NET_DEVICE_H

#include "ns3/net-device.h"
#include "ns3/traced-callback.h"

namespace ns3 {

class WifiRemoteStationManager;
class WifiPhy;
class WifiMac;
class HtConfiguration;
class VhtConfiguration;
class HeConfiguration;

/// This value conforms to the 802.11 specification
static const uint16_t MAX_MSDU_SIZE = 2304;

/**
 * \defgroup wifi Wifi Models
 *
 * This section documents the API of the ns-3 Wifi module. For a generic functional description, please refer to the ns-3 manual.
 */


/**
 * \brief Hold together all Wifi-related objects.
 * \ingroup wifi
 *
 * This class holds together ns3::Channel, ns3::WifiPhy,
 * ns3::WifiMac, and, ns3::WifiRemoteStationManager.
 */
class WifiNetDevice : public NetDevice
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  WifiNetDevice ();
  virtual ~WifiNetDevice ();

  /**
   * \param mac the MAC layer to use.
   */
  void SetMac (const Ptr<WifiMac> mac);
  /**
   * \param phy the PHY layer to use.
   */
  void SetPhy (const Ptr<WifiPhy> phy);
  /**
   * \param manager the manager to use.
   */
  void SetRemoteStationManager (const Ptr<WifiRemoteStationManager> manager);
  /**
   * \returns the MAC we are currently using.
   */
  Ptr<WifiMac> GetMac (void) const;
  /**
   * \returns the PHY we are currently using.
   */
  Ptr<WifiPhy> GetPhy (void) const;
  /**
   * \returns the remote station manager we are currently using.
   */
  Ptr<WifiRemoteStationManager> GetRemoteStationManager (void) const;

  /**
   * \param htConfiguration pointer to HtConfiguration
   */
  void SetHtConfiguration (Ptr<HtConfiguration> htConfiguration);
  /**
   * \return pointer to HtConfiguration if it exists
   */
  Ptr<HtConfiguration> GetHtConfiguration (void) const;
  /**
   * \param vhtConfiguration pointer to VhtConfiguration
   */
  void SetVhtConfiguration (Ptr<VhtConfiguration> vhtConfiguration);
  /**
   * \return pointer to VhtConfiguration if it exists
   */
  Ptr<VhtConfiguration> GetVhtConfiguration (void) const;
  /**
   * \param heConfiguration pointer to HeConfiguration
   */
  void SetHeConfiguration (Ptr<HeConfiguration> heConfiguration);
  /**
   * \return pointer to HeConfiguration if it exists
   */
  Ptr<HeConfiguration> GetHeConfiguration (void) const;

  //inherited from NetDevice base class.
  void SetIfIndex (const uint32_t index);
  uint32_t GetIfIndex (void) const;
  Ptr<Channel> GetChannel (void) const;
  void SetAddress (Address address);
  Address GetAddress (void) const;
  bool SetMtu (const uint16_t mtu);
  uint16_t GetMtu (void) const;
  bool IsLinkUp (void) const;
  void AddLinkChangeCallback (Callback<void> callback);
  bool IsBroadcast (void) const;
  Address GetBroadcast (void) const;
  bool IsMulticast (void) const;
  Address GetMulticast (Ipv4Address multicastGroup) const;
  bool IsPointToPoint (void) const;
  bool IsBridge (void) const;
  bool Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber);
  Ptr<Node> GetNode (void) const;
  void SetNode (const Ptr<Node> node);
  bool NeedsArp (void) const;
  void SetReceiveCallback (NetDevice::ReceiveCallback cb);
  Address GetMulticast (Ipv6Address addr) const;
  bool SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);
  void SetPromiscReceiveCallback (PromiscReceiveCallback cb);
  bool SupportsSendFrom (void) const;


protected:
  void DoDispose (void);
  void DoInitialize (void);
  /**
   * Receive a packet from the lower layer and pass the
   * packet up the stack.
   *
   * \param packet the packet to forward up
   * \param from the source address
   * \param to the destination address
   */
  void ForwardUp (Ptr<const Packet> packet, Mac48Address from, Mac48Address to);


private:
  /**
   * \brief Copy constructor
   * \param o object to copy
   *
   * Defined and unimplemented to avoid misuse
   */
  WifiNetDevice (const WifiNetDevice &o);

  /**
   * \brief Assignment operator
   * \param o object to copy
   * \returns the copied object
   *
   * Defined and unimplemented to avoid misuse
   */
  WifiNetDevice &operator = (const WifiNetDevice &o);

  /**
   * Set that the link is up. A link is always up in ad-hoc mode.
   * For a STA, a link is up when the STA is associated with an AP.
   */
  void LinkUp (void);
  /**
   * Set that the link is down (i.e. STA is not associated).
   */
  void LinkDown (void);
  /**
   * Complete the configuration of this Wi-Fi device by
   * connecting all lower components (e.g. MAC, WifiRemoteStation) together.
   */
  void CompleteConfig (void);

  Ptr<Node> m_node; //!< the node
  Ptr<WifiPhy> m_phy; //!< the phy
  Ptr<WifiMac> m_mac; //!< the MAC
  Ptr<WifiRemoteStationManager> m_stationManager; //!< the station manager
  Ptr<HtConfiguration> m_htConfiguration; //!< the HtConfiguration
  Ptr<VhtConfiguration> m_vhtConfiguration; //!< the VhtConfiguration
  Ptr<HeConfiguration> m_heConfiguration; //!< the HeConfiguration
  NetDevice::ReceiveCallback m_forwardUp; //!< forward up callback
  NetDevice::PromiscReceiveCallback m_promiscRx; //!< promiscuous receive callback

  TracedCallback<Ptr<const Packet>, Mac48Address> m_rxLogger; //!< receive trace callback
  TracedCallback<Ptr<const Packet>, Mac48Address> m_txLogger; //!< transmit trace callback

  uint32_t m_ifIndex; //!< IF index
  bool m_linkUp; //!< link up
  TracedCallback<> m_linkChanges; //!< link change callback
  mutable uint16_t m_mtu; //!< MTU
  bool m_configComplete; //!< configuration complete
};

} //namespace ns3

#endif /* WIFI_NET_DEVICE_H */
