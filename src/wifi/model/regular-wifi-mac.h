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

#ifndef REGULAR_WIFI_MAC_H
#define REGULAR_WIFI_MAC_H

#include "wifi-mac.h"
#include "qos-txop.h"
#include "ssid.h"
#include <set>
#include <unordered_map>

namespace ns3 {

class MacRxMiddle;
class MacTxMiddle;
class ChannelAccessManager;
class ExtendedCapabilities;
class FrameExchangeManager;
class WifiPsdu;
enum WifiTxTimerReason : uint8_t;

typedef std::unordered_map <uint16_t /* staId */, Ptr<WifiPsdu> /* PSDU */> WifiPsduMap;

/**
 * \brief base class for all MAC-level wifi objects.
 * \ingroup wifi
 *
 * This class encapsulates all the low-level MAC functionality and all the
 * high-level MAC functionality (association/disassociation state machines).
 *
 */
class RegularWifiMac : public WifiMac
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  RegularWifiMac ();
  virtual ~RegularWifiMac ();

  // Implementations of pure virtual methods.
  void SetShortSlotTimeSupported (bool enable) override;
  void SetSsid (Ssid ssid) override;
  void SetAddress (Mac48Address address) override;
  void SetPromisc (void) override;
  bool GetShortSlotTimeSupported (void) const override;
  Ssid GetSsid (void) const override;
  Mac48Address GetAddress (void) const override;
  Mac48Address GetBssid (void) const override;
  void Enqueue (Ptr<Packet> packet, Mac48Address to, Mac48Address from) override;
  bool SupportsSendFrom (void) const override;
  void SetWifiPhy (const Ptr<WifiPhy> phy) override;
  Ptr<WifiPhy> GetWifiPhy (void) const override;
  void ResetWifiPhy (void) override;
  void SetWifiRemoteStationManager (const Ptr<WifiRemoteStationManager> stationManager) override;
  void ConfigureStandard (WifiStandard standard) override;
  TypeOfStation GetTypeOfStation (void) const override;
  void SetForwardUpCallback (ForwardUpCallback upCallback) override;
  void SetLinkUpCallback (Callback<void> linkUp) override;
  void SetLinkDownCallback (Callback<void> linkDown) override;
  Ptr<WifiRemoteStationManager> GetWifiRemoteStationManager (void) const override;

  // Should be implemented by child classes
  void Enqueue (Ptr<Packet> packet, Mac48Address to) override = 0;

  /**
   * Get the Frame Exchange Manager
   *
   * \return the Frame Exchange Manager
   */
  Ptr<FrameExchangeManager> GetFrameExchangeManager (void) const;

  /**
   * Enable or disable CTS-to-self feature.
   *
   * \param enable true if CTS-to-self is to be supported,
   *               false otherwise
   */
  void SetCtsToSelfSupported (bool enable);
  /**
   * \param bssid the BSSID of the network that this device belongs to.
   */
  void SetBssid (Mac48Address bssid);

  /**
   * Accessor for the DCF object
   *
   * \return a smart pointer to Txop
   */
  Ptr<Txop> GetTxop (void) const;
  /**
   * Accessor for a specified EDCA object
   *
   * \param ac the Access Category
   * \return a smart pointer to a QosTxop
   */
  Ptr<QosTxop> GetQosTxop (AcIndex ac) const;
  /**
   * Accessor for a specified EDCA object
   *
   * \param tid the Traffic ID
   * \return a smart pointer to a QosTxop
   */
  Ptr<QosTxop> GetQosTxop (uint8_t tid) const;

  /**
   * Return the extended capabilities of the device.
   *
   * \return the extended capabilities that we support
   */
  ExtendedCapabilities GetExtendedCapabilities (void) const;
  /**
   * Return the HT capabilities of the device.
   *
   * \return the HT capabilities that we support
   */
  HtCapabilities GetHtCapabilities (void) const;
  /**
   * Return the VHT capabilities of the device.
   *
   * \return the VHT capabilities that we support
   */
  VhtCapabilities GetVhtCapabilities (void) const;
  /**
   * Return the HE capabilities of the device.
   *
   * \return the HE capabilities that we support
   */
  HeCapabilities GetHeCapabilities (void) const;

protected:
  void DoInitialize () override;
  void DoDispose () override;
  void SetTypeOfStation (TypeOfStation type) override;

  Ptr<MacRxMiddle> m_rxMiddle;                      //!< RX middle (defragmentation etc.)
  Ptr<MacTxMiddle> m_txMiddle;                      //!< TX middle (aggregation etc.)
  Ptr<ChannelAccessManager> m_channelAccessManager; //!< channel access manager
  Ptr<WifiPhy> m_phy;                               //!< Wifi PHY
  Ptr<FrameExchangeManager> m_feManager;            //!< Frame Exchange Manager

  Ptr<WifiRemoteStationManager> m_stationManager; //!< Remote station manager (rate control, RTS/CTS/fragmentation thresholds etc.)

  ForwardUpCallback m_forwardUp; //!< Callback to forward packet up the stack
  Callback<void> m_linkUp;       //!< Callback when a link is up
  Callback<void> m_linkDown;     //!< Callback when a link is down

  Ssid m_ssid; //!< Service Set ID (SSID)

  /** This holds a pointer to the TXOP instance for this WifiMac - used
  for transmission of frames to non-QoS peers. */
  Ptr<Txop> m_txop;

  /** This type defines a mapping between an Access Category index,
  and a pointer to the corresponding channel access function */
  typedef std::map<AcIndex, Ptr<QosTxop> > EdcaQueues;

  /** This is a map from Access Category index to the corresponding
  channel access function */
  EdcaQueues m_edca;

  /**
   * Accessor for the AC_VO channel access function
   *
   * \return a smart pointer to QosTxop
   */
  Ptr<QosTxop> GetVOQueue (void) const;
  /**
   * Accessor for the AC_VI channel access function
   *
   * \return a smart pointer to QosTxop
   */
  Ptr<QosTxop> GetVIQueue (void) const;
  /**
   * Accessor for the AC_BE channel access function
   *
   * \return a smart pointer to QosTxop
   */
  Ptr<QosTxop> GetBEQueue (void) const;
  /**
   * Accessor for the AC_BK channel access function
   *
   * \return a smart pointer to QosTxop
   */
  Ptr<QosTxop> GetBKQueue (void) const;

  /**
   * \param cwMin the minimum contention window size
   * \param cwMax the maximum contention window size
   *
   * This method is called to set the minimum and the maximum
   * contention window size.
   */
  void ConfigureContentionWindow (uint32_t cwMin, uint32_t cwMax);

  /**
   * This method acts as the MacRxMiddle receive callback and is
   * invoked to notify us that a frame has been received. The
   * implementation is intended to capture logic that is going to be
   * common to all (or most) derived classes. Specifically, handling
   * of Block Ack management frames is dealt with here.
   *
   * This method will need, however, to be overridden by derived
   * classes so that they can perform their data handling before
   * invoking the base version.
   *
   * \param mpdu the MPDU that has been received.
   */
  virtual void Receive (Ptr<WifiMacQueueItem> mpdu);

  /**
   * Forward the packet up to the device.
   *
   * \param packet the packet that we are forwarding up to the device
   * \param from the address of the source
   * \param to the address of the destination
   */
  void ForwardUp (Ptr<const Packet> packet, Mac48Address from, Mac48Address to);

  /**
   * This method can be called to de-aggregate an A-MSDU and forward
   * the constituent packets up the stack.
   *
   * \param mpdu the MPDU containing the A-MSDU.
   */
  virtual void DeaggregateAmsduAndForward (Ptr<WifiMacQueueItem> mpdu);

  /**
   * Enable or disable QoS support for the device.
   *
   * \param enable whether QoS is supported
   */
  virtual void SetQosSupported (bool enable);
  /**
   * Return whether the device supports QoS.
   *
   * \return true if QoS is supported, false otherwise
   */
  bool GetQosSupported () const;

  /**
   * Create a Frame Exchange Manager depending on the supported version
   * of the standard.
   */
  void SetupFrameExchangeManager (void);

  /**
   * Return whether the device supports HT.
   *
   * \return true if HT is supported, false otherwise
   */
  bool GetHtSupported () const;

  /**
   * Return whether the device supports VHT.
   *
   * \return true if VHT is supported, false otherwise
   */
  bool GetVhtSupported () const;

  /**
   * Enable or disable ERP support for the device.
   *
   * \param enable whether ERP is supported
   */
  void SetErpSupported (bool enable);
  /**
   * Return whether the device supports ERP.
   *
   * \return true if ERP is supported, false otherwise
   */
  bool GetErpSupported () const;

  /**
   * Enable or disable DSSS support for the device.
   *
   * \param enable whether DSSS is supported
   */
  void SetDsssSupported (bool enable);
  /**
   * Return whether the device supports DSSS.
   *
   * \return true if DSSS is supported, false otherwise
   */
  bool GetDsssSupported () const;

  /**
   * Return whether the device supports HE.
   *
   * \return true if HE is supported, false otherwise
   */
  bool GetHeSupported () const;

private:
  /// type conversion operator
  RegularWifiMac (const RegularWifiMac &);
  /**
   * assignment operator
   *
   * \param mac the RegularWifiMac to assign
   * \returns the assigned value
   */
  RegularWifiMac & operator= (const RegularWifiMac & mac);

  /**
   * This method is a private utility invoked to configure the channel
   * access function for the specified Access Category.
   *
   * \param ac the Access Category of the queue to initialise.
   */
  void SetupEdcaQueue (AcIndex ac);

  /**
   * Set the block ack threshold for AC_VO.
   *
   * \param threshold the block ack threshold for AC_VO.
   */
  void SetVoBlockAckThreshold (uint8_t threshold);
  /**
   * Set the block ack threshold for AC_VI.
   *
   * \param threshold the block ack threshold for AC_VI.
   */
  void SetViBlockAckThreshold (uint8_t threshold);
  /**
   * Set the block ack threshold for AC_BE.
   *
   * \param threshold the block ack threshold for AC_BE.
   */
  void SetBeBlockAckThreshold (uint8_t threshold);
  /**
   * Set the block ack threshold for AC_BK.
   *
   * \param threshold the block ack threshold for AC_BK.
   */
  void SetBkBlockAckThreshold (uint8_t threshold);

  /**
   * Set VO block ack inactivity timeout.
   *
   * \param timeout the VO block ack inactivity timeout.
   */
  void SetVoBlockAckInactivityTimeout (uint16_t timeout);
  /**
   * Set VI block ack inactivity timeout.
   *
   * \param timeout the VI block ack inactivity timeout.
   */
  void SetViBlockAckInactivityTimeout (uint16_t timeout);
  /**
   * Set BE block ack inactivity timeout.
   *
   * \param timeout the BE block ack inactivity timeout.
   */
  void SetBeBlockAckInactivityTimeout (uint16_t timeout);
  /**
   * Set BK block ack inactivity timeout.
   *
   * \param timeout the BK block ack inactivity timeout.
   */
  void SetBkBlockAckInactivityTimeout (uint16_t timeout);

  TypeOfStation m_typeOfStation;                        //!< the type of station

  /**
   * This Boolean is set \c true iff this WifiMac is to model
   * 802.11e/WMM style Quality of Service. It is exposed through the
   * attribute system.
   *
   * At the moment, this flag is the sole selection between QoS and
   * non-QoS operation for the STA (whether IBSS, AP, or
   * non-AP). Ultimately, we will want a QoS-enabled STA to be able to
   * fall back to non-QoS operation with a non-QoS peer. This'll
   * require further intelligence - i.e., per-association QoS
   * state. Having a big switch seems like a good intermediate stage,
   * however.
   */
  bool m_qosSupported;
  /**
   * This Boolean is set \c true iff this WifiMac is to model
   * 802.11g. It is exposed through the attribute system.
   */
  bool m_erpSupported;
  /**
   * This Boolean is set \c true iff this WifiMac is to model
   * 802.11b. It is exposed through the attribute system.
   */
  bool m_dsssSupported;

  Mac48Address m_address;   ///< MAC address of this station
  Mac48Address m_bssid;     ///< the BSSID

  uint16_t m_voMaxAmsduSize; ///< maximum A-MSDU size for AC_VO (in bytes)
  uint16_t m_viMaxAmsduSize; ///< maximum A-MSDU size for AC_VI (in bytes)
  uint16_t m_beMaxAmsduSize; ///< maximum A-MSDU size for AC_BE (in bytes)
  uint16_t m_bkMaxAmsduSize; ///< maximum A-MSDU size for AC_BK (in bytes)

  uint32_t m_voMaxAmpduSize; ///< maximum A-MPDU size for AC_VO (in bytes)
  uint32_t m_viMaxAmpduSize; ///< maximum A-MPDU size for AC_VI (in bytes)
  uint32_t m_beMaxAmpduSize; ///< maximum A-MPDU size for AC_BE (in bytes)
  uint32_t m_bkMaxAmpduSize; ///< maximum A-MPDU size for AC_BK (in bytes)

  TracedCallback<const WifiMacHeader &> m_txOkCallback; ///< transmit OK callback
  TracedCallback<const WifiMacHeader &> m_txErrCallback; ///< transmit error callback

  /// TracedCallback for acked/nacked MPDUs typedef
  typedef TracedCallback<Ptr<const WifiMacQueueItem>> MpduTracedCallback;

  MpduTracedCallback m_ackedMpduCallback;  ///< ack'ed MPDU callback
  MpduTracedCallback m_nackedMpduCallback; ///< nack'ed MPDU callback

  /**
   * TracedCallback signature for MPDU drop events.
   *
   * \param reason the reason why the MPDU was dropped (\see WifiMacDropReason)
   * \param mpdu the dropped MPDU
   */
  typedef void (* DroppedMpduCallback)(WifiMacDropReason reason, Ptr<const WifiMacQueueItem> mpdu);

  /// TracedCallback for MPDU drop events typedef
  typedef TracedCallback<WifiMacDropReason, Ptr<const WifiMacQueueItem>> DroppedMpduTracedCallback;

  /**
   * This trace indicates that an MPDU was dropped for the given reason.
   */
  DroppedMpduTracedCallback m_droppedMpduCallback;

  /**
   * TracedCallback signature for MPDU response timeout events.
   *
   * \param reason the reason why the timer was started
   * \param mpdu the MPDU whose response was not received before the timeout
   * \param txVector the TXVECTOR used to transmit the MPDU
   */
  typedef void (* MpduResponseTimeoutCallback)(uint8_t reason, Ptr<const WifiMacQueueItem> mpdu,
                                               const WifiTxVector& txVector);

  /// TracedCallback for MPDU response timeout events typedef
  typedef TracedCallback<uint8_t, Ptr<const WifiMacQueueItem>, const WifiTxVector&> MpduResponseTimeoutTracedCallback;

  /**
   * MPDU response timeout traced callback.
   * This trace source is fed by a WifiTxTimer object.
   */
  MpduResponseTimeoutTracedCallback m_mpduResponseTimeoutCallback;

  /**
   * TracedCallback signature for PSDU response timeout events.
   *
   * \param reason the reason why the timer was started
   * \param psdu the PSDU whose response was not received before the timeout
   * \param txVector the TXVECTOR used to transmit the PSDU
   */
  typedef void (* PsduResponseTimeoutCallback)(uint8_t reason, Ptr<const WifiPsdu> psdu,
                                               const WifiTxVector& txVector);

  /// TracedCallback for PSDU response timeout events typedef
  typedef TracedCallback<uint8_t, Ptr<const WifiPsdu>, const WifiTxVector&> PsduResponseTimeoutTracedCallback;

  /**
   * PSDU response timeout traced callback.
   * This trace source is fed by a WifiTxTimer object.
   */
  PsduResponseTimeoutTracedCallback m_psduResponseTimeoutCallback;

  /**
   * TracedCallback signature for PSDU map response timeout events.
   *
   * \param reason the reason why the timer was started
   * \param psduMap the PSDU map for which not all responses were received before the timeout
   * \param missingStations the MAC addresses of the stations that did not respond
   * \param nTotalStations the total number of stations that had to respond
   */
  typedef void (* PsduMapResponseTimeoutCallback)(uint8_t reason, WifiPsduMap* psduMap,
                                                  const std::set<Mac48Address>* missingStations,
                                                  std::size_t nTotalStations);

  /// TracedCallback for PSDU map response timeout events typedef
  typedef TracedCallback<uint8_t, WifiPsduMap*, const std::set<Mac48Address>*, std::size_t> PsduMapResponseTimeoutTracedCallback;

  /**
   * PSDU map response timeout traced callback.
   * This trace source is fed by a WifiTxTimer object.
   */
  PsduMapResponseTimeoutTracedCallback m_psduMapResponseTimeoutCallback;

  bool m_shortSlotTimeSupported; ///< flag whether short slot time is supported
  bool m_ctsToSelfSupported;     ///< flag indicating whether CTS-To-Self is supported
};

} //namespace ns3

#endif /* REGULAR_WIFI_MAC_H */
