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

#include "mgt-headers.h"
#include "wifi-mac.h"

#include <set>
#include <variant>

class TwoLevelAggregationTest;
class AmpduAggregationTest;
class HeAggregationTest;
class MultiLinkOperationsTestBase;

namespace ns3
{

class SupportedRates;
class CapabilityInformation;
class RandomVariableStream;
class WifiAssocManager;

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
        uint16_t number{0};                          ///< channel number
        WifiPhyBand band{WIFI_PHY_BAND_UNSPECIFIED}; ///< PHY band
    };

    /// typedef for a list of channels
    using ChannelList = std::list<Channel>;

    enum : uint8_t
    {
        ACTIVE = 0,
        PASSIVE
    } type; ///< indicates either active or passive scanning

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
    /// Allow test cases to access private members
    friend class ::MultiLinkOperationsTestBase;

    /// type of the management frames used to get info about APs
    using MgtFrameType =
        std::variant<MgtBeaconHeader, MgtProbeResponseHeader, MgtAssocResponseHeader>;

    /**
     * Struct to hold information regarding observed AP through
     * active/passive scanning
     */
    struct ApInfo
    {
        Mac48Address m_bssid;  ///< BSSID
        Mac48Address m_apAddr; ///< AP MAC address
        double m_snr;          ///< SNR in linear scale
        MgtFrameType m_frame;  ///< The body of the management frame used to update AP info
        WifiScanParams::Channel m_channel; ///< The channel the management frame was received on
        uint8_t m_linkId;                  ///< ID of the link used to communicate with the AP
        /// list of (local link ID, AP link ID) pairs identifying the links to setup between MLDs
        std::list<std::pair<std::uint8_t, uint8_t>> m_setupLinks;
    };

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    StaWifiMac();
    ~StaWifiMac() override;

    /**
     * \param packet the packet to send.
     * \param to the address to which the packet should be sent.
     *
     * The packet should be enqueued in a TX queue, and should be
     * dequeued as soon as the channel access function determines that
     * access is granted to this MAC.
     */
    void Enqueue(Ptr<Packet> packet, Mac48Address to) override;
    bool CanForwardPacketsTo(Mac48Address to) const override;

    /**
     * \param phys the physical layers attached to this MAC.
     */
    void SetWifiPhys(const std::vector<Ptr<WifiPhy>>& phys) override;

    /**
     * Set the Association Manager.
     *
     * \param assocManager the Association Manager
     */
    void SetAssocManager(Ptr<WifiAssocManager> assocManager);

    /**
     * Forward a probe request packet to the DCF. The standard is not clear on the correct
     * queue for management frames if QoS is supported. We always use the DCF.
     */
    void SendProbeRequest();

    /**
     * This method is called after wait beacon timeout or wait probe request timeout has
     * occurred. This will trigger association process from beacons or probe responses
     * gathered while scanning.
     *
     * \param bestAp the info about the best AP to associate with, if one was found
     */
    void ScanningTimeout(const std::optional<ApInfo>& bestAp);

    /**
     * Return whether we are associated with an AP.
     *
     * \return true if we are associated with an AP, false otherwise
     */
    bool IsAssociated() const;

    /**
     * Get the IDs of the setup links (if any).
     *
     * \return the IDs of the setup links
     */
    std::set<uint8_t> GetSetupLinkIds() const;

    /**
     * Return the association ID.
     *
     * \return the association ID
     */
    uint16_t GetAssociationId() const;

    void NotifyChannelSwitching(uint8_t linkId) override;

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * \param stream first stream index to use
     *
     * \return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

  protected:
    /**
     * Structure holding information specific to a single link. Here, the meaning of
     * "link" is that of the 11be amendment which introduced multi-link devices. For
     * previous amendments, only one link can be created.
     */
    struct StaLinkEntity : public WifiMac::LinkEntity
    {
        /// Destructor (a virtual method is needed to make this struct polymorphic)
        ~StaLinkEntity() override;

        bool sendAssocReq;                 //!< whether this link is used to send the
                                           //!< Association Request frame
        std::optional<uint8_t> apLinkId;   //!< ID (as set by the AP) of the link we have
                                           //!< setup or are setting up
        std::optional<Mac48Address> bssid; //!< BSSID of the AP to associate with over this link
        EventId beaconWatchdog;            //!< beacon watchdog
        Time beaconWatchdogEnd{0};         //!< beacon watchdog end
    };

    /**
     * Get a reference to the link associated with the given ID.
     *
     * \param linkId the given link ID
     * \return a reference to the link associated with the given ID
     */
    StaLinkEntity& GetLink(uint8_t linkId) const;

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
    void SetActiveProbing(bool enable);
    /**
     * Return whether active probing is enabled.
     *
     * \return true if active probing is enabled, false otherwise
     */
    bool GetActiveProbing() const;

    /**
     * Determine whether the supported rates indicated in a given Beacon frame or
     * Probe Response frame fit with the configured membership selector.
     *
     * \param frame the given Beacon or Probe Response frame
     * \param linkId ID of the link the mgt frame was received over
     * \return whether the the supported rates indicated in the given management
     *         frame fit with the configured membership selector
     */
    bool CheckSupportedRates(std::variant<MgtBeaconHeader, MgtProbeResponseHeader> frame,
                             uint8_t linkId);

    void Receive(Ptr<const WifiMpdu> mpdu, uint8_t linkId) override;
    std::unique_ptr<LinkEntity> CreateLinkEntity() const override;
    Mac48Address DoGetLocalAddress(const Mac48Address& remoteAddr) const override;

    /**
     * Process the Beacon frame received on the given link.
     *
     * \param mpdu the MPDU containing the Beacon frame
     * \param linkId the ID of the given link
     */
    void ReceiveBeacon(Ptr<const WifiMpdu> mpdu, uint8_t linkId);

    /**
     * Process the Probe Response frame received on the given link.
     *
     * \param mpdu the MPDU containing the Probe Response frame
     * \param linkId the ID of the given link
     */
    void ReceiveProbeResp(Ptr<const WifiMpdu> mpdu, uint8_t linkId);

    /**
     * Process the (Re)Association Response frame received on the given link.
     *
     * \param mpdu the MPDU containing the (Re)Association Response frame
     * \param linkId the ID of the given link
     */
    void ReceiveAssocResp(Ptr<const WifiMpdu> mpdu, uint8_t linkId);

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
    void UpdateApInfo(const MgtFrameType& frame,
                      const Mac48Address& apAddr,
                      const Mac48Address& bssid,
                      uint8_t linkId);

    /**
     * Get the (Re)Association Request frame to send on a given link. The returned frame
     * never includes a Multi-Link Element.
     *
     * \param isReassoc whether a Reassociation Request has to be returned
     * \param linkId the ID of the given link
     * \return the (Re)Association Request frame
     */
    std::variant<MgtAssocRequestHeader, MgtReassocRequestHeader> GetAssociationRequest(
        bool isReassoc,
        uint8_t linkId) const;

    /**
     * Forward an association or reassociation request packet to the DCF.
     * The standard is not clear on the correct queue for management frames if QoS is supported.
     * We always use the DCF.
     *
     * \param isReassoc flag whether it is a reassociation request
     *
     */
    void SendAssociationRequest(bool isReassoc);
    /**
     * Try to ensure that we are associated with an AP by taking an appropriate action
     * depending on the current association status.
     */
    void TryToEnsureAssociated();
    /**
     * This method is called after the association timeout occurred. We switch the state to
     * WAIT_ASSOC_RESP and re-send an association request.
     */
    void AssocRequestTimeout();
    /**
     * Start the scanning process which trigger active or passive scanning based on the
     * active probing flag.
     */
    void StartScanning();
    /**
     * Return whether we are waiting for an association response from an AP.
     *
     * \return true if we are waiting for an association response from an AP, false otherwise
     */
    bool IsWaitAssocResp() const;
    /**
     * This method is called after we have not received a beacon from the AP
     * on the given link.
     *
     * \param linkId the ID of the given link
     */
    void MissedBeacons(uint8_t linkId);
    /**
     * Restarts the beacon timer for the given link.
     *
     * \param delay the delay before the watchdog fires
     * \param linkId the ID of the given link
     */
    void RestartBeaconWatchdog(Time delay, uint8_t linkId);
    /**
     * Check if any enabled link remains after the given link is disabled (because,
     * e.g., the maximum number of beacons is missed or the channel is switched).
     * If no enabled link remains, proceed with disassociation.
     *
     * \param linkId the ID of the given link
     */
    void Disassociated(uint8_t linkId);
    /**
     * Return an instance of SupportedRates that contains all rates that we support
     * including HT rates.
     *
     * \param linkId the ID of the link for which the request is made
     * \return SupportedRates all rates that we support
     */
    SupportedRates GetSupportedRates(uint8_t linkId) const;
    /**
     * Return the Multi-Link Element to include in the management frames transmitted
     * on the given link
     *
     * \param isReassoc whether the Multi-Link Element is included in a Reassociation Request
     * \param linkId the ID of the given link
     * \return the Multi-Link Element
     */
    MultiLinkElement GetMultiLinkElement(bool isReassoc, uint8_t linkId) const;
    /**
     * Set the current MAC state.
     *
     * \param value the new state
     */
    void SetState(MacState value);
    /**
     * Set the EDCA parameters.
     *
     * \param ac the access class
     * \param cwMin the minimum contention window size
     * \param cwMax the maximum contention window size
     * \param aifsn the number of slots that make up an AIFS
     * \param txopLimit the TXOP limit
     */
    void SetEdcaParameters(AcIndex ac,
                           uint32_t cwMin,
                           uint32_t cwMax,
                           uint8_t aifsn,
                           Time txopLimit);
    /**
     * Set the MU EDCA parameters.
     *
     * \param ac the Access Category
     * \param cwMin the minimum contention window size
     * \param cwMax the maximum contention window size
     * \param aifsn the number of slots that make up an AIFS
     * \param muEdcaTimer the MU EDCA timer
     */
    void SetMuEdcaParameters(AcIndex ac,
                             uint16_t cwMin,
                             uint16_t cwMax,
                             uint8_t aifsn,
                             Time muEdcaTimer);
    /**
     * Return the Capability information for the given link.
     *
     * \param linkId the ID of the given link
     * \return the Capability information that we support
     */
    CapabilityInformation GetCapabilities(uint8_t linkId) const;

    /**
     * Indicate that PHY capabilities have changed.
     */
    void PhyCapabilitiesChanged();

    /**
     * Get the current primary20 channel used on the given link as a
     * (channel number, PHY band) pair.
     *
     * \param linkId the ID of the given link
     * \return a (channel number, PHY band) pair
     */
    WifiScanParams::Channel GetCurrentChannel(uint8_t linkId) const;

    void DoInitialize() override;
    void DoDispose() override;

    MacState m_state;                       ///< MAC state
    uint16_t m_aid;                         ///< Association AID
    Ptr<WifiAssocManager> m_assocManager;   ///< Association Manager
    Time m_waitBeaconTimeout;               ///< wait beacon timeout
    Time m_probeRequestTimeout;             ///< probe request timeout
    Time m_assocRequestTimeout;             ///< association request timeout
    EventId m_assocRequestEvent;            ///< association request event
    uint32_t m_maxMissedBeacons;            ///< maximum missed beacons
    bool m_activeProbing;                   ///< active probing
    Ptr<RandomVariableStream> m_probeDelay; ///< RandomVariable used to randomize the time
                                            ///< of the first Probe Response on each channel

    TracedCallback<Mac48Address> m_assocLogger;             ///< association logger
    TracedCallback<uint8_t, Mac48Address> m_setupCompleted; ///< link setup completed logger
    TracedCallback<Mac48Address> m_deAssocLogger;           ///< disassociation logger
    TracedCallback<uint8_t, Mac48Address> m_setupCanceled;  ///< link setup canceled logger
    TracedCallback<Time> m_beaconArrival;                   ///< beacon arrival logger
    TracedCallback<ApInfo> m_beaconInfo;                    ///< beacon info logger

    /// TracedCallback signature for link setup completed/canceled events
    using LinkSetupCallback = void (*)(uint8_t /* link ID */, Mac48Address /* AP address */);
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the output stream
 * \param apInfo the AP information
 * \returns a reference to the stream
 */
std::ostream& operator<<(std::ostream& os, const StaWifiMac::ApInfo& apInfo);

} // namespace ns3

#endif /* STA_WIFI_MAC_H */
