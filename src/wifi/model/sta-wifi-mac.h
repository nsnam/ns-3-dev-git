/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#ifndef STA_WIFI_MAC_H
#define STA_WIFI_MAC_H

#include "wifi-mac.h"
#include "mgt-headers.h"
#include <variant>

class TwoLevelAggregationTest;
class AmpduAggregationTest;
class HeAggregationTest;

namespace ns3  {

class SupportedRates;
class CapabilityInformation;
class RandomVariableStream;


/**
 * \ingroup wifi
 *
 * Structure holding scan parameters
 */
struct WifiScanParams
{
  /**
   * Struct identifying a channel to scan.
   * A channel number equal to zero indicates to scan all the channels;
   * an unspecified band (WIFI_PHY_BAND_UNSPECIFIED) indicates to scan
   * all the supported PHY bands.
   */
  struct Channel
  {
    uint16_t number {0};                           ///< channel number
    WifiPhyBand band {WIFI_PHY_BAND_UNSPECIFIED};  ///< PHY band
  };

  /// typedef for a list of channels
  using ChannelList = std::list<Channel>;

  enum : uint8_t
  {
    ACTIVE = 0,
    PASSIVE
  } type;                               ///< indicates either active or passive scanning
  Ssid ssid;                            ///< desired SSID or wildcard SSID
  std::vector<ChannelList> channelList; ///< list of channels to scan, for each link
  Time probeDelay;                      ///< delay prior to transmitting a Probe Request
  Time minChannelTime;                  ///< minimum time to spend on each channel
  Time maxChannelTime;                  ///< maximum time to spend on each channel
};

/**
 * \ingroup wifi
 *
 * The Wifi MAC high model for a non-AP STA in a BSS. The state
 * machine is as follows:
 *
   \verbatim
   ┌───────────┐            ┌────────────────┐                           ┌─────────────┐
   │   Start   │      ┌─────┤   Associated   ◄───────────────────┐    ┌──►   Refused   │
   └─┬─────────┘      │     └────────────────┘                   │    │  └─────────────┘
     │                │                                          │    │
     │                │ ┌─────────────────────────────────────┐  │    │
     │                │ │                                     │  │    │
     │  ┌─────────────▼─▼──┐       ┌──────────────┐       ┌───┴──▼────┴───────────────────┐
     └──►   Unassociated   ├───────►   Scanning   ├───────►   Wait AssociationiResponse   │
        └──────────────────┘       └──────┬──▲────┘       └───────────────┬──▲────────────┘
                                          │  │                            │  │
                                          │  │                            │  │
                                          └──┘                            └──┘
   \endverbatim
 *
 * Notes:
 * 1. The state 'Start' is not included in #MacState and only used
 *    for illustration purpose.
 * 2. The Unassociated state is a transient state before STA starts the
 *    scanning procedure which moves it into the Scanning state.
 * 3. In Scanning, STA is gathering beacon or probe response frames from APs,
 *    resulted in a list of candidate AP. After the timeout, it then tries to
 *    associate to the best AP, which is indicated by the Association Manager.
 *    STA will restart the scanning procedure if SetActiveProbing() called.
 * 4. In the case when AP responded to STA's association request with a
 *    refusal, STA will try to associate to the next best AP until the list
 *    of candidate AP is exhausted which sends STA to Refused state.
 *    - Note that this behavior is not currently tested since ns-3 does not
  *     implement association refusal at present.
 * 5. The transition from Wait Association Response to Unassociated
 *    occurs if an association request fails without explicit
 *    refusal (i.e., the AP fails to respond).
 * 6. The transition from Associated to Wait Association Response
 *    occurs when STA's PHY capabilities changed. In this state, STA
 *    tries to reassociate with the previously associated AP.
 * 7. The transition from Associated to Unassociated occurs if the number
 *    of missed beacons exceeds the threshold.
 */
class StaWifiMac : public WifiMac
{
public:
  /// Allow test cases to access private members
  friend class ::TwoLevelAggregationTest;
  /// Allow test cases to access private members
  friend class ::AmpduAggregationTest;
  /// Allow test cases to access private members
  friend class ::HeAggregationTest;

  /// type of the management frames used to get info about APs
  using MgtFrameType = std::variant<MgtBeaconHeader, MgtProbeResponseHeader, MgtAssocResponseHeader>;

  /**
  * Struct to hold information regarding observed AP through
  * active/passive scanning
  */
  struct ApInfo
  {
    Mac48Address m_bssid;              ///< BSSID
    Mac48Address m_apAddr;             ///< AP MAC address
    double m_snr;                      ///< SNR in linear scale
    MgtFrameType m_frame;              ///< The body of the management frame used to update AP info
    WifiScanParams::Channel m_channel; ///< The channel the management frame was received on
    uint8_t m_linkId;                  ///< ID of the link used to communicate with the AP
  };

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  StaWifiMac ();
  virtual ~StaWifiMac ();

  /**
   * \param packet the packet to send.
   * \param to the address to which the packet should be sent.
   *
   * The packet should be enqueued in a TX queue, and should be
   * dequeued as soon as the channel access function determines that
   * access is granted to this MAC.
   */
  void Enqueue (Ptr<Packet> packet, Mac48Address to) override;
  bool CanForwardPacketsTo (Mac48Address to) const override;

  /**
   * \param phys the physical layers attached to this MAC.
   */
  void SetWifiPhys (const std::vector<Ptr<WifiPhy>>& phys) override;

  /**
   * Forward a probe request packet to the DCF. The standard is not clear on the correct
   * queue for management frames if QoS is supported. We always use the DCF.
   */
  void SendProbeRequest (void);

  /**
   * This method is called after wait beacon timeout or wait probe request timeout has
   * occurred. This will trigger association process from beacons or probe responses
   * gathered while scanning.
   */
  void ScanningTimeout (void);

  /**
   * Return whether we are associated with an AP.
   *
   * \return true if we are associated with an AP, false otherwise
   */
  bool IsAssociated (void) const;

  /**
   * Return the association ID.
   *
   * \return the association ID
   */
  uint16_t GetAssociationId (void) const;

  void NotifyChannelSwitching (void) override;

  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model.  Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream first stream index to use
   *
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams (int64_t stream);

private:
  /**
   * The current MAC state of the STA.
   */
  enum MacState
  {
    ASSOCIATED,
    SCANNING,
    WAIT_ASSOC_RESP,
    UNASSOCIATED,
    REFUSED
  };

  /**
   * Enable or disable active probing.
   *
   * \param enable enable or disable active probing
   */
  void SetActiveProbing (bool enable);
  /**
   * Return whether active probing is enabled.
   *
   * \return true if active probing is enabled, false otherwise
   */
  bool GetActiveProbing (void) const;

  /**
   * Determine whether the supported rates indicated in a given Beacon frame or
   * Probe Response frame fit with the configured membership selector.
   *
   * \param frame the given Beacon or Probe Response frame
   * \param linkId ID of the link the mgt frame was received over
   * \return whether the the supported rates indicated in the given management
   *         frame fit with the configured membership selector
   */
  bool CheckSupportedRates (std::variant<MgtBeaconHeader, MgtProbeResponseHeader> frame, uint8_t linkId);

  void Receive (Ptr<WifiMacQueueItem> mpdu, uint8_t linkId) override;

  /**
   * Update associated AP's information from the given management frame (Beacon,
   * Probe Response or Association Response). If STA is not associated, this
   * information will be used for the association process.
   *
   * \param frame the body of the given management frame
   * \param apAddr MAC address of the AP
   * \param bssid MAC address of BSSID
   * \param linkId ID of the link the management frame was received over
   */
  void UpdateApInfo (const MgtFrameType& frame, const Mac48Address& apAddr,
                     const Mac48Address& bssid, uint8_t linkId);

  /**
   * Update list of candidate AP to associate. The list should contain ApInfo sorted from
   * best to worst SNR, with no duplicate.
   *
   * \param newApInfo the new ApInfo to be inserted
   */
  void UpdateCandidateApList (ApInfo newApInfo);

  /**
   * Forward an association or reassociation request packet to the DCF.
   * The standard is not clear on the correct queue for management frames if QoS is supported.
   * We always use the DCF.
   *
   * \param isReassoc flag whether it is a reassociation request
   *
   */
  void SendAssociationRequest (bool isReassoc);
  /**
   * Try to ensure that we are associated with an AP by taking an appropriate action
   * depending on the current association status.
   */
  void TryToEnsureAssociated (void);
  /**
   * This method is called after the association timeout occurred. We switch the state to
   * WAIT_ASSOC_RESP and re-send an association request.
   */
  void AssocRequestTimeout (void);
  /**
   * Start the scanning process which trigger active or passive scanning based on the
   * active probing flag.
   */
  void StartScanning (void);
  /**
   * Return whether we are waiting for an association response from an AP.
   *
   * \return true if we are waiting for an association response from an AP, false otherwise
   */
  bool IsWaitAssocResp (void) const;
  /**
   * This method is called after we have not received a beacon from the AP
   */
  void MissedBeacons (void);
  /**
   * Restarts the beacon timer.
   *
   * \param delay the delay before the watchdog fires
   */
  void RestartBeaconWatchdog (Time delay);
  /**
   * Take actions after disassociation.
   */
  void Disassociated (void);
  /**
   * Return an instance of SupportedRates that contains all rates that we support
   * including HT rates.
   *
   * \return SupportedRates all rates that we support
   */
  SupportedRates GetSupportedRates (void) const;
  /**
   * Set the current MAC state.
   *
   * \param value the new state
   */
  void SetState (MacState value);
  /**
   * Set the EDCA parameters.
   *
   * \param ac the access class
   * \param cwMin the minimum contention window size
   * \param cwMax the maximum contention window size
   * \param aifsn the number of slots that make up an AIFS
   * \param txopLimit the TXOP limit
   */
  void SetEdcaParameters (AcIndex ac, uint32_t cwMin, uint32_t cwMax, uint8_t aifsn, Time txopLimit);
  /**
   * Set the MU EDCA parameters.
   *
   * \param ac the Access Category
   * \param cwMin the minimum contention window size
   * \param cwMax the maximum contention window size
   * \param aifsn the number of slots that make up an AIFS
   * \param muEdcaTimer the MU EDCA timer
   */
  void SetMuEdcaParameters (AcIndex ac, uint16_t cwMin, uint16_t cwMax, uint8_t aifsn, Time muEdcaTimer);
  /**
   * Return the Capability information of the current STA.
   *
   * \return the Capability information that we support
   */
  CapabilityInformation GetCapabilities (void) const;

  /**
   * Indicate that PHY capabilities have changed.
   */
  void PhyCapabilitiesChanged (void);

  void DoInitialize (void) override;

  MacState m_state;            ///< MAC state
  uint16_t m_aid;              ///< Association AID
  Time m_waitBeaconTimeout;    ///< wait beacon timeout
  Time m_probeRequestTimeout;  ///< probe request timeout
  Time m_assocRequestTimeout;  ///< association request timeout
  EventId m_waitBeaconEvent;   ///< wait beacon event
  EventId m_probeRequestEvent; ///< probe request event
  EventId m_assocRequestEvent; ///< association request event
  EventId m_beaconWatchdog;    ///< beacon watchdog
  Time m_beaconWatchdogEnd;    ///< beacon watchdog end
  uint32_t m_maxMissedBeacons; ///< maximum missed beacons
  bool m_activeProbing;        ///< active probing
  Ptr<RandomVariableStream> m_probeDelay;  ///< RandomVariable used to randomize the time
                                           ///< of the first Probe Response on each channel
  std::vector<ApInfo> m_candidateAps; ///< list of candidate APs to associate to
  // Note: std::multiset<ApInfo> might be a candidate container to implement
  // this sorted list, but we are using a std::vector because we want to sort
  // based on SNR but find duplicates based on BSSID, and in practice this
  // candidate vector should not be too large.

  TracedCallback<Mac48Address> m_assocLogger;   ///< association logger
  TracedCallback<Mac48Address> m_deAssocLogger; ///< disassociation logger
  TracedCallback<Time>         m_beaconArrival; ///< beacon arrival logger
};

} //namespace ns3

#endif /* STA_WIFI_MAC_H */
