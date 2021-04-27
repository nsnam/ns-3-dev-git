/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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

#ifndef WIFI_MAC_H
#define WIFI_MAC_H

#include "ns3/net-device.h"
#include "wifi-standards.h"
#include "wifi-remote-station-manager.h"
#include "qos-utils.h"

namespace ns3 {

class Ssid;
class Txop;
class HtConfiguration;
class VhtConfiguration;
class HeConfiguration;

/**
 * Enumeration for type of station
 */
enum TypeOfStation
{
  STA,
  AP,
  ADHOC_STA,
  MESH,
  OCB
};

/**
 * \ingroup wifi
  * \enum WifiMacDropReason
  * \brief The reason why an MPDU was dropped
  */
enum WifiMacDropReason : uint8_t
{
  WIFI_MAC_DROP_FAILED_ENQUEUE = 0,
  WIFI_MAC_DROP_EXPIRED_LIFETIME,
  WIFI_MAC_DROP_REACHED_RETRY_LIMIT
};

/**
 * \brief base class for all MAC-level wifi objects.
 * \ingroup wifi
 *
 * This class encapsulates all the low-level MAC functionality
 * DCA, EDCA, etc) and all the high-level MAC functionality
 * (association/disassociation state machines).
 *
 */
class WifiMac : public Object
{
public:
  virtual void DoDispose ();

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Sets the device this PHY is associated with.
   *
   * \param device the device this PHY is associated with
   */
  void SetDevice (const Ptr<NetDevice> device);
  /**
   * Return the device this PHY is associated with
   *
   * \return the device this PHY is associated with
   */
  Ptr<NetDevice> GetDevice (void) const;

   /**
   * This method is invoked by a subclass to specify what type of
   * station it is implementing. This is something that the channel
   * access functions need to know.
   *
   * \param type the type of station.
   */
  virtual void SetTypeOfStation (TypeOfStation type) = 0;
  /**
   * Return the type of station.
   *
   * \return the type of station.
   */
  virtual TypeOfStation GetTypeOfStation (void) const = 0;

 /**
   * \param ssid the current SSID of this MAC layer.
   */
  virtual void SetSsid (Ssid ssid) = 0;
  /**
   * Enable or disable short slot time feature.
   *
   * \param enable true if short slot time is to be supported,
   *               false otherwise
   */
  virtual void SetShortSlotTimeSupported (bool enable) = 0;
  /**
   * \brief Sets the interface in promiscuous mode.
   *
   * Enables promiscuous mode on the interface. Note that any further
   * filtering on the incoming frame path may affect the overall
   * behavior.
   */
  virtual void SetPromisc (void) = 0;

  /**
   * \return the MAC address associated to this MAC layer.
   */
  virtual Mac48Address GetAddress (void) const = 0;
  /**
   * \return the SSID which this MAC layer is going to try to stay in.
   */
  virtual Ssid GetSsid (void) const = 0;
  /**
   * \param address the current address of this MAC layer.
   */
  virtual void SetAddress (Mac48Address address) = 0;
  /**
   * \return the BSSID of the network this device belongs to.
   */
  virtual Mac48Address GetBssid (void) const = 0;
  /**
   * \return whether the device supports short slot time capability.
   */
  virtual bool GetShortSlotTimeSupported (void) const = 0;

  /**
   * \param packet the packet to send.
   * \param to the address to which the packet should be sent.
   * \param from the address from which the packet should be sent.
   *
   * The packet should be enqueued in a TX queue, and should be
   * dequeued as soon as the DCF function determines that
   * access it granted to this MAC. The extra parameter "from" allows
   * this device to operate in a bridged mode, forwarding received
   * frames without altering the source address.
   */
  virtual void Enqueue (Ptr<Packet> packet, Mac48Address to, Mac48Address from) = 0;
  /**
   * \param packet the packet to send.
   * \param to the address to which the packet should be sent.
   *
   * The packet should be enqueued in a TX queue, and should be
   * dequeued as soon as the DCF function determines that
   * access it granted to this MAC.
   */
  virtual void Enqueue (Ptr<Packet> packet, Mac48Address to) = 0;
  /**
   * \return if this MAC supports sending from arbitrary address.
   *
   * The interface may or may not support sending from arbitrary address.
   * This function returns true if sending from arbitrary address is supported,
   * false otherwise.
   */
  virtual bool SupportsSendFrom (void) const = 0;
  /**
   * \param phy the physical layer attached to this MAC.
   */
  virtual void SetWifiPhy (Ptr<WifiPhy> phy) = 0;
  /**
   * \return the physical layer attached to this MAC
   */
  virtual Ptr<WifiPhy> GetWifiPhy (void) const = 0;
  /**
   * Remove currently attached WifiPhy device from this MAC.
   */
  virtual void ResetWifiPhy (void) = 0;
  /**
   * \param stationManager the station manager attached to this MAC.
   */
  virtual void SetWifiRemoteStationManager (Ptr<WifiRemoteStationManager> stationManager) = 0;
  /**
   * \return the station manager attached to this MAC.
   */
  virtual Ptr<WifiRemoteStationManager> GetWifiRemoteStationManager (void) const = 0;

  /**
   * This type defines the callback of a higher layer that a
   * WifiMac(-derived) object invokes to pass a packet up the stack.
   *
   * \param packet the packet that has been received.
   * \param from the MAC address of the device that sent the packet.
   * \param to the MAC address of the device that the packet is destined for.
   */
  typedef Callback<void, Ptr<const Packet>, Mac48Address, Mac48Address> ForwardUpCallback;

  /**
   * \param upCallback the callback to invoke when a packet must be
   *        forwarded up the stack.
   */
  virtual void SetForwardUpCallback (ForwardUpCallback upCallback) = 0;
  /**
   * \param linkUp the callback to invoke when the link becomes up.
   */
  virtual void SetLinkUpCallback (Callback<void> linkUp) = 0;
  /**
   * \param linkDown the callback to invoke when the link becomes down.
   */
  virtual void SetLinkDownCallback (Callback<void> linkDown) = 0;
  /* Next functions are not pure virtual so non QoS WifiMacs are not
   * forced to implement them.
   */

  /**
   * \param packet the packet being enqueued
   *
   * Public method used to fire a MacTx trace. Implemented for encapsulation purposes.
   * Note this trace indicates that the packet was accepted by the device only.
   * The packet may be dropped later (e.g. if the queue is full).
   */
  void NotifyTx (Ptr<const Packet> packet);
  /**
   * \param packet the packet being dropped
   *
   * Public method used to fire a MacTxDrop trace.
   * This trace indicates that the packet was dropped before it was queued for
   * transmission (e.g. when a STA is not associated with an AP).
   */
  void NotifyTxDrop (Ptr<const Packet> packet);
  /**
   * \param packet the packet we received
   *
   * Public method used to fire a MacRx trace. Implemented for encapsulation purposes.
   */
  void NotifyRx (Ptr<const Packet> packet);
  /**
   * \param packet the packet we received promiscuously
   *
   * Public method used to fire a MacPromiscRx trace. Implemented for encapsulation purposes.
   */
  void NotifyPromiscRx (Ptr<const Packet> packet);
  /**
   * \param packet the packet we received but is not destined for us
   *
   * Public method used to fire a MacRxDrop trace. Implemented for encapsulation purposes.
   */
  void NotifyRxDrop (Ptr<const Packet> packet);

  /**
   * \param standard the wifi standard to be configured
   *
   * This method completes the configuration process for a requested PHY standard.
   * Subclasses should implement this method to configure their DCF queues
   * according to the requested standard.
   */
  virtual void ConfigureStandard (WifiStandard standard) = 0;

  /**
   * \return pointer to HtConfiguration if it exists
   */
  Ptr<HtConfiguration> GetHtConfiguration (void) const;
  /**
   * \return pointer to VhtConfiguration if it exists
   */
  Ptr<VhtConfiguration> GetVhtConfiguration (void) const;
  /**
   * \return pointer to HeConfiguration if it exists
   */
  Ptr<HeConfiguration> GetHeConfiguration (void) const;


protected:
  /**
   * \param dcf the DCF to be configured
   * \param cwmin the minimum contention window for the DCF
   * \param cwmax the maximum contention window for the DCF
   * \param isDsss flag to indicate whether PHY is DSSS or HR/DSSS
   * \param ac the access category for the DCF
   *
   * Configure the DCF with appropriate values depending on the given access category.
   */
  void ConfigureDcf (Ptr<Txop> dcf, uint32_t cwmin, uint32_t cwmax, bool isDsss, AcIndex ac);


private:
  Ptr<NetDevice> m_device;    ///< Pointer to the device

  /**
   * The trace source fired when packets come into the "top" of the device
   * at the L3/L2 transition, before being queued for transmission.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_macTxTrace;
  /**
   * The trace source fired when packets coming into the "top" of the device
   * are dropped at the MAC layer before being queued for transmission.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_macTxDropTrace;
  /**
   * The trace source fired for packets successfully received by the device
   * immediately before being forwarded up to higher layers (at the L2/L3
   * transition).  This is a promiscuous trace.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_macPromiscRxTrace;
  /**
   * The trace source fired for packets successfully received by the device
   * immediately before being forwarded up to higher layers (at the L2/L3
   * transition).  This is a non- promiscuous trace.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_macRxTrace;
  /**
   * The trace source fired when packets coming into the "top" of the device
   * are dropped at the MAC layer during reception.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_macRxDropTrace;
};

} //namespace ns3

#endif /* WIFI_MAC_H */

