/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
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
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_EMLSR_TEST_H
#define WIFI_EMLSR_TEST_H

#include "ns3/header-serialization-test.h"
#include "ns3/packet-socket-address.h"
#include "ns3/wifi-mac-queue-scheduler.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-ppdu.h"

namespace ns3
{

class ApWifiMac;
class ListErrorModel;
class PacketSocketClient;
class StaWifiMac;
class WifiPsdu;

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test EML Operating Mode Notification frame serialization and deserialization
 */
class EmlOperatingModeNotificationTest : public HeaderSerializationTestCase
{
  public:
    /**
     * Constructor
     */
    EmlOperatingModeNotificationTest();
    ~EmlOperatingModeNotificationTest() override = default;

  private:
    void DoRun() override;
};

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Base class for EMLSR Operations tests
 *
 * This base class setups and configures one AP MLD, a variable number of non-AP MLDs with
 * EMLSR activated and a variable number of non-AP MLD with EMLSR deactivated. Every MLD has
 * three links, each operating on a distinct PHY band (2.4 GHz, 5 GHz and 6 GHz). Therefore,
 * it is expected that three links are setup by the non-AP MLD(s). The values for the Padding
 * Delay, the Transition Delay and the Transition Timeout are provided as argument to the
 * constructor of this class, along with the IDs of the links on which EMLSR mode must be
 * enabled for the non-AP MLDs (this information is used to set the EmlsrLinkSet attribute
 * of the DefaultEmlsrManager installed on the non-AP MLDs).
 */
class EmlsrOperationsTestBase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * \param name The name of the new TestCase created
     */
    EmlsrOperationsTestBase(const std::string& name);
    ~EmlsrOperationsTestBase() override = default;

    /// Enumeration for traffic directions
    enum TrafficDirection : uint8_t
    {
        DOWNLINK = 0,
        UPLINK
    };

  protected:
    /**
     * Callback invoked when a FEM passes PSDUs to the PHY.
     *
     * \param mac the MAC transmitting the PSDUs
     * \param phyId the ID of the PHY transmitting the PSDUs
     * \param psduMap the PSDU map
     * \param txVector the TX vector
     * \param txPowerW the tx power in Watts
     */
    virtual void Transmit(Ptr<WifiMac> mac,
                          uint8_t phyId,
                          WifiConstPsduMap psduMap,
                          WifiTxVector txVector,
                          double txPowerW);

    /**
     * \param dir the traffic direction (downlink/uplink)
     * \param staId the index (starting at 0) of the non-AP MLD generating/receiving packets
     * \param count the number of packets to generate
     * \param pktSize the size of the packets to generate
     * \return an application generating the given number packets of the given size from/to the
     *         AP MLD to/from the given non-AP MLD
     */
    Ptr<PacketSocketClient> GetApplication(TrafficDirection dir,
                                           std::size_t staId,
                                           std::size_t count,
                                           std::size_t pktSize) const;

    /**
     * Check whether QoS data unicast transmissions addressed to the given destination on the
     * given link are blocked or unblocked for the given reason on the given device.
     *
     * \param mac the MAC of the given device
     * \param dest the MAC address of the given destination
     * \param linkId the ID of the given link
     * \param reason the reason for blocking transmissions to test
     * \param blocked whether transmissions are blocked for the given reason
     * \param description text indicating when this check is performed
     * \param testUnblockedForOtherReasons whether to test if transmissions are unblocked for
     *                                     all the reasons other than the one provided
     */
    void CheckBlockedLink(Ptr<WifiMac> mac,
                          Mac48Address dest,
                          uint8_t linkId,
                          WifiQueueBlockedReason reason,
                          bool blocked,
                          std::string description,
                          bool testUnblockedForOtherReasons = true);

    void DoSetup() override;

    /// Information about transmitted frames
    struct FrameInfo
    {
        Time startTx;             ///< TX start time
        WifiConstPsduMap psduMap; ///< transmitted PSDU map
        WifiTxVector txVector;    ///< TXVECTOR
        uint8_t linkId;           ///< link ID
        uint8_t phyId;            ///< ID of the transmitting PHY
    };

    uint8_t m_mainPhyId{0};                   //!< ID of the main PHY
    std::set<uint8_t> m_linksToEnableEmlsrOn; /**< IDs of the links on which EMLSR mode has to
                                                   be enabled */
    std::size_t m_nEmlsrStations{1};          ///< number of stations to create that activate EMLSR
    std::size_t m_nNonEmlsrStations{0};       /**< number of stations to create that do not
                                                activate EMLSR */
    Time m_transitionTimeout{MicroSeconds(128)}; ///< Transition Timeout advertised by the AP MLD
    std::vector<Time> m_paddingDelay{
        {MicroSeconds(32)}}; ///< Padding Delay advertised by the non-AP MLD
    std::vector<Time> m_transitionDelay{
        {MicroSeconds(16)}};                ///< Transition Delay advertised by the non-AP MLD
    bool m_establishBaDl{false};            /**< whether BA needs to be established (for TID 0)
                                                 with the AP as originator */
    bool m_establishBaUl{false};            /**< whether BA needs to be established (for TID 0)
                                                 with the AP as recipient */
    std::vector<FrameInfo> m_txPsdus;       ///< transmitted PSDUs
    Ptr<ApWifiMac> m_apMac;                 ///< AP wifi MAC
    std::vector<Ptr<StaWifiMac>> m_staMacs; ///< MACs of the non-AP MLDs
    std::vector<PacketSocketAddress> m_dlSockets; ///< packet socket address for DL traffic
    std::vector<PacketSocketAddress> m_ulSockets; ///< packet socket address for UL traffic
    uint16_t m_lastAid{0};                        ///< AID of last associated station
    Time m_duration{0};                           ///< simulation duration

  private:
    /**
     * Set the SSID on the next station that needs to start the association procedure.
     * This method is connected to the ApWifiMac's AssociatedSta trace source.
     * Start generating traffic (if needed) when all stations are associated.
     *
     * \param aid the AID assigned to the previous associated STA
     */
    void SetSsid(uint16_t aid, Mac48Address /* addr */);

    /**
     * Start the generation of traffic (needs to be overridden)
     */
    virtual void StartTraffic()
    {
    }
};

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test the exchange of EML Operating Mode Notification frames.
 *
 * This test considers an AP MLD and a non-AP MLD with EMLSR activated. Upon association,
 * the non-AP MLD sends an EML Operating Mode Notification frame, which is however corrupted
 * by using a post reception error model (installed on the AP MLD). We keep corrupting the
 * EML Notification frames transmitted by the non-AP MLD until the frame is dropped due to
 * exceeded max retry limit. It is checked that:
 *
 * - the Association Request contains a Multi-Link Element including an EML Capabilities field
 *   that contains the expected values for Padding Delay and Transition Delay
 * - the Association Response contains a Multi-Link Element including an EML Capabilities field
 *   that contains the expected value for Transition Timeout
 * - all EML Notification frames contain the expected values for EMLSR Mode, EMLMR Mode and
 *   Link Bitmap fields and are transmitted on the link used for association
 * - the correct EMLSR link set is stored by the EMLSR Manager, both when the transition
 *   timeout expires and when an EML Notification response is received from the AP MLD (thus,
 *   the correct EMLSR link set is stored after whichever of the two events occur first)
 */
class EmlOmnExchangeTest : public EmlsrOperationsTestBase
{
  public:
    /**
     * Constructor
     *
     * \param linksToEnableEmlsrOn IDs of links on which EMLSR mode should be enabled
     * \param transitionTimeout the Transition Timeout advertised by the AP MLD
     */
    EmlOmnExchangeTest(const std::set<uint8_t>& linksToEnableEmlsrOn, Time transitionTimeout);
    ~EmlOmnExchangeTest() override = default;

  protected:
    void DoSetup() override;
    void DoRun() override;
    void Transmit(Ptr<WifiMac> mac,
                  uint8_t phyId,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW) override;

    /**
     * Callback invoked when the non-AP MLD receives the acknowledgment for a transmitted MPDU.
     *
     * \param mpdu the acknowledged MPDU
     */
    void TxOk(Ptr<const WifiMpdu> mpdu);
    /**
     * Callback invoked when the non-AP MLD drops the given MPDU for the given reason.
     *
     * \param reason the reason why the MPDU was dropped
     * \param mpdu the dropped MPDU
     */
    void TxDropped(WifiMacDropReason reason, Ptr<const WifiMpdu> mpdu);

    /**
     * Check the content of the EML Capabilities subfield of the Multi-Link Element included
     * in the Association Request frame sent by the non-AP MLD.
     *
     * \param mpdu the MPDU containing the Association Request frame
     * \param txVector the TXVECTOR used to transmit the frame
     * \param linkId the ID of the link on which the frame was transmitted
     */
    void CheckEmlCapabilitiesInAssocReq(Ptr<const WifiMpdu> mpdu,
                                        const WifiTxVector& txVector,
                                        uint8_t linkId);
    /**
     * Check the content of the EML Capabilities subfield of the Multi-Link Element included
     * in the Association Response frame sent by the AP MLD to the EMLSR client.
     *
     * \param mpdu the MPDU containing the Association Response frame
     * \param txVector the TXVECTOR used to transmit the frame
     * \param linkId the ID of the link on which the frame was transmitted
     */
    void CheckEmlCapabilitiesInAssocResp(Ptr<const WifiMpdu> mpdu,
                                         const WifiTxVector& txVector,
                                         uint8_t linkId);
    /**
     * Check the content of a received EML Operating Mode Notification frame.
     *
     * \param psdu the PSDU containing the EML Operating Mode Notification frame
     * \param txVector the TXVECTOR used to transmit the frame
     * \param linkId the ID of the link on which the frame was transmitted
     */
    void CheckEmlNotification(Ptr<const WifiPsdu> psdu,
                              const WifiTxVector& txVector,
                              uint8_t linkId);
    /**
     * Check that the EMLSR mode has been enabled on the expected EMLSR links.
     */
    void CheckEmlsrLinks();

  private:
    std::size_t m_checkEmlsrLinksCount;        /**< counter for the number of times CheckEmlsrLinks
                                                    is called (should be two: when the transition
                                                    timeout expires and when the EML Notification
                                                    response from the AP MLD is received */
    std::size_t m_emlNotificationDroppedCount; /**< counter for the number of times the EML
                                                    Notification frame sent by the non-AP MLD
                                                    has been dropped due to max retry limit */
    Ptr<ListErrorModel> m_errorModel;          ///< error rate model to corrupt packets at AP MLD
    std::list<uint64_t> m_uidList;             ///< list of UIDs of packets to corrupt
};

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test the transmission of DL frames to EMLSR clients.
 *
 * This test considers an AP MLD and a configurable number of non-AP MLDs that support EMLSR
 * and a configurable number of non-AP MLDs that do not support EMLSR. All MLDs have three
 * setup links, while the set of EMLSR links for the EMLSR clients is configurable.
 * Block ack agreements (for TID 0, with the AP MLD as originator) are established with all
 * the non-AP MLDs before that EMLSR clients send the EML Operating Mode Notification frame
 * to enable the EMLSR mode on their EMLSR links.
 *
 * Before enabling EMLSR mode, it is checked that:
 *
 * - all EMLSR links (but the link used for ML setup) of the EMLSR clients are considered to
 *   be in power save mode and are blocked by the AP MLD; all the other links have transitioned
 *   to active mode and are not blocked
 * - no MU-RTS Trigger Frame is sent as Initial control frame
 * - In case of EMLSR clients having no link that is not an EMLSR link and is different than
 *   the link used for ML setup, the two A-MPDUs used to trigger BA establishment are
 *   transmitted one after another on the link used for ML setup. Otherwise, the two A-MPDUs
 *   are sent concurrently on two distinct links
 *
 * After enabling EMLSR mode, it is checked that:
 *
 * - all EMLSR links of the EMLSR clients are considered to be in active mode and are not
 *   blocked by the AP MLD
 * - If all setup links are EMLSR links, the first two frame exchanges are both protected by
 *   MU-RTS TF and occur one after another. Otherwise, one frame exchange occurs on the
 *   non-EMLSR link and is not protected by MU-RTS TF; the other frame exchange occurs on an
 *   EMLSR link and is protected by MU-RTS TF
 * - the AP MLD blocks transmission on all other EMLSR links when sending an ICF to an EMLSR client
 * - After completing a frame exchange with an EMLSR client, the AP MLD can start another frame
 *   exchange with that EMLSR client within the same TXOP (after a SIFS) without sending an ICF
 * - During the transition delay, all EMLSR links are not used for DL transmissions
 * - The padding added to Initial Control frames is the largest among all the EMLSR clients
 *   solicited by the ICF
 *
 * After disabling EMLSR mode, it is checked that:
 *
 * - all EMLSR links (but the link used to exchange EML Notification frames) of the EMLSR clients
 *   are considered to be in power save mode and are blocked by the AP MLD
 * - an MU-RTS Trigger Frame is sent by the AP MLD as ICF for sending the EML Notification
 *   response, unless the link used to exchange EML Notification frames is a non-EMLSR link
 * - no MU-RTS Trigger Frame is used as ICF for QoS data frames
 * - In case of EMLSR clients having no link that is not an EMLSR link and is different than
 *   the link used to exchange EML Notification frames, the two A-MPDUs are transmitted one
 *   after another on the link used to exchange EML Notification frames. Otherwise, the two
 *   A-MPDUs are sent concurrently on two distinct links
 */
class EmlsrDlTxopTest : public EmlsrOperationsTestBase
{
  public:
    /**
     * Parameters for the EMLSR DL TXOP test
     */
    struct Params
    {
        std::size_t nEmlsrStations;    //!< number of non-AP MLDs that support EMLSR
        std::size_t nNonEmlsrStations; //!< number of non-AP MLDs that do not support EMLSR
        std::set<uint8_t>
            linksToEnableEmlsrOn;       //!< IDs of links on which EMLSR mode should be enabled
        std::vector<Time> paddingDelay; //!< vector (whose size equals <i>nEmlsrStations</i>) of the
                                        //!< padding delay values advertised by non-AP MLDs
        std::vector<Time>
            transitionDelay;    //!< vector (whose size equals <i>nEmlsrStations</i>) of
                                //!< transition the delay values advertised by non-AP MLDs
        Time transitionTimeout; //!< the Transition Timeout advertised by the AP MLD
    };

    /**
     * Constructor
     *
     * \param params parameters for the EMLSR DL TXOP test
     */
    EmlsrDlTxopTest(const Params& params);
    ~EmlsrDlTxopTest() override = default;

  protected:
    void DoSetup() override;
    void DoRun() override;
    void Transmit(Ptr<WifiMac> mac,
                  uint8_t phyId,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW) override;

    /**
     * Check that the simulation produced the expected results.
     */
    void CheckResults();

    /**
     * Check that the AP MLD considers the correct Power Management mode for the links setup
     * with the given non-AP MLD. This method is intended to be called shortly after ML setup.
     *
     * \param address a link address of the given non-AP MLD
     */
    void CheckPmModeAfterAssociation(const Mac48Address& address);

    /**
     * Check that appropriate actions are taken by the AP MLD transmitting an EML Operating Mode
     * Notification response frame to an EMLSR client on the given link.
     *
     * \param mpdu the MPDU carrying the EML Operating Mode Notification frame
     * \param txVector the TXVECTOR used to send the PPDU
     * \param linkId the ID of the given link
     */
    void CheckEmlNotificationFrame(Ptr<const WifiMpdu> mpdu,
                                   const WifiTxVector& txVector,
                                   uint8_t linkId);

    /**
     * Check that appropriate actions are taken by the AP MLD transmitting an initial
     * Control frame to an EMLSR client on the given link.
     *
     * \param mpdu the MPDU carrying the MU-RTS TF
     * \param txVector the TXVECTOR used to send the PPDU
     * \param linkId the ID of the given link
     */
    void CheckInitialControlFrame(Ptr<const WifiMpdu> mpdu,
                                  const WifiTxVector& txVector,
                                  uint8_t linkId);

    /**
     * Check that appropriate actions are taken by the AP MLD transmitting a PPDU containing
     * QoS data frames to EMLSR clients on the given link.
     *
     * \param psduMap the PSDU(s) carrying QoS data frames
     * \param txVector the TXVECTOR used to send the PPDU
     * \param linkId the ID of the given link
     */
    void CheckQosFrames(const WifiConstPsduMap& psduMap,
                        const WifiTxVector& txVector,
                        uint8_t linkId);

    /**
     * Check that appropriate actions are taken by the AP MLD receiving a PPDU containing
     * BlockAck frames from EMLSR clients on the given link.
     *
     * \param psduMap the PSDU carrying BlockAck frames
     * \param txVector the TXVECTOR used to send the PPDU
     * \param phyId the ID of the PHY transmitting the PSDU(s)
     */
    void CheckBlockAck(const WifiConstPsduMap& psduMap,
                       const WifiTxVector& txVector,
                       uint8_t phyId);

  private:
    void StartTraffic() override;

    /**
     * Enable EMLSR mode on the next EMLSR client
     */
    void EnableEmlsrMode();

    std::set<uint8_t> m_emlsrLinks; /**< IDs of the links on which EMLSR mode has to be enabled */
    Time m_emlsrEnabledTime;        //!< when EMLSR mode has been enabled on all EMLSR clients
    const Time m_fe2to3delay;       /**< time interval between 2nd and 3rd frame exchange sequences
                                         after the enablement of EMLSR mode */
    std::size_t m_countQoSframes;   //!< counter for QoS frames (transition delay test)
    std::size_t m_countBlockAck;    //!< counter for BlockAck frames (transition delay test)
    Ptr<ListErrorModel> m_errorModel; ///< error rate model to corrupt BlockAck at AP MLD
};

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test the switching of PHYs on EMLSR clients.
 *
 * An AP MLD and an EMLSR client setup 3 links, on which EMLSR mode is enabled. The AP MLD
 * transmits 4 QoS data frames (one after another, each protected by ICF):
 *
 * - the first one on the link used for ML setup, hence no PHY switch occurs
 * - the second one on another link, thus causing the main PHY to switch link
 * - the third one on the remaining link, thus causing the main PHY to switch link again
 * - the fourth one on the link used for ML setup; if the aux PHYs switches link, there is
 *   one aux PHY listening on such a link and the main PHY switches to this link, otherwise
 *   no PHY is listening on such a link and there is no response to the ICF sent by the AP MLD
 */
class EmlsrLinkSwitchTest : public EmlsrOperationsTestBase
{
  public:
    /**
     * Parameters for the EMLSR link switching test
     */
    struct Params
    {
        bool
            switchAuxPhy; //!< whether AUX PHY should switch channel to operate on the link on which
                          //!<  the Main PHY was operating before moving to the link of the Aux PHY
        bool resetCamState; //!< whether to reset the state of the ChannelAccessManager associated
                            //!< with the link on which the main PHY has just switched to
        uint16_t auxPhyMaxChWidth; //!< max channel width (MHz) supported by aux PHYs
    };

    /**
     * Constructor
     *
     * \param params parameters for the EMLSR link switching test
     */
    EmlsrLinkSwitchTest(const Params& params);

    ~EmlsrLinkSwitchTest() override = default;

  protected:
    void DoSetup() override;
    void DoRun() override;
    void Transmit(Ptr<WifiMac> mac,
                  uint8_t phyId,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW) override;

    /**
     * Check that the simulation produced the expected results.
     */
    void CheckResults();

    /**
     * Check that the Main PHY (and possibly the Aux PHY) correctly switches channel when the
     * reception of an ICF ends.
     *
     * \param psduMap the PSDU carrying the MU-RTS TF
     * \param txVector the TXVECTOR used to send the PPDU
     * \param linkId the ID of the given link
     */
    void CheckInitialControlFrame(const WifiConstPsduMap& psduMap,
                                  const WifiTxVector& txVector,
                                  uint8_t linkId);

    /**
     * Check that appropriate actions are taken by the AP MLD transmitting a PPDU containing
     * QoS data frames to the EMLSR client on the given link.
     *
     * \param psduMap the PSDU(s) carrying QoS data frames
     * \param txVector the TXVECTOR used to send the PPDU
     * \param linkId the ID of the given link
     */
    void CheckQosFrames(const WifiConstPsduMap& psduMap,
                        const WifiTxVector& txVector,
                        uint8_t linkId);

  private:
    bool m_switchAuxPhy;  /**< whether AUX PHY should switch channel to operate on the link on which
                               the Main PHY was operating before moving to the link of Aux PHY */
    bool m_resetCamState; /**< whether to reset the state of the ChannelAccessManager associated
                               with the link on which the main PHY has just switched to */
    uint16_t m_auxPhyMaxChWidth;  //!< max channel width (MHz) supported by aux PHYs
    std::size_t m_countQoSframes; //!< counter for QoS data frames
    std::size_t m_txPsdusPos;     //!< a position in the vector of TX PSDUs
};

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief wifi EMLSR Test Suite
 */
class WifiEmlsrTestSuite : public TestSuite
{
  public:
    WifiEmlsrTestSuite();
};

} // namespace ns3

#endif /* WIFI_EMLSR_TEST_H */
