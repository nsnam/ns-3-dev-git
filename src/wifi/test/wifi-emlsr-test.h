/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_EMLSR_TEST_H
#define WIFI_EMLSR_TEST_H

#include "ns3/ap-wifi-mac.h"
#include "ns3/error-model.h"
#include "ns3/header-serialization-test.h"
#include "ns3/packet-socket-address.h"
#include "ns3/packet-socket-client.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/test.h"
#include "ns3/wifi-mac-queue-scheduler.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-ppdu.h"
#include "ns3/wifi-psdu.h"

#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <vector>

using namespace ns3;

// forward declaration
namespace ns3
{
struct EmlsrMainPhySwitchTrace;
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test EML Operating Mode Notification frame serialization and deserialization
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
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Base class for EMLSR Operations tests
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
     * @param name The name of the new TestCase created
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
     * @param mac the MAC transmitting the PSDUs
     * @param phyId the ID of the PHY transmitting the PSDUs
     * @param psduMap the PSDU map
     * @param txVector the TX vector
     * @param txPowerW the tx power in Watts
     */
    virtual void Transmit(Ptr<WifiMac> mac,
                          uint8_t phyId,
                          WifiConstPsduMap psduMap,
                          WifiTxVector txVector,
                          double txPowerW);

    /**
     * @param dir the traffic direction (downlink/uplink)
     * @param staId the index (starting at 0) of the non-AP MLD generating/receiving packets
     * @param count the number of packets to generate
     * @param pktSize the size of the packets to generate
     * @return an application generating the given number packets of the given size from/to the
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
     * @param mac the MAC of the given device
     * @param dest the MAC address of the given destination
     * @param linkId the ID of the given link
     * @param reason the reason for blocking transmissions to test
     * @param blocked whether transmissions are blocked for the given reason
     * @param description text indicating when this check is performed
     * @param testUnblockedForOtherReasons whether to test if transmissions are unblocked for
     *                                     all the reasons other than the one provided
     */
    void CheckBlockedLink(Ptr<WifiMac> mac,
                          Mac48Address dest,
                          uint8_t linkId,
                          WifiQueueBlockedReason reason,
                          bool blocked,
                          std::string description,
                          bool testUnblockedForOtherReasons = true);

    /**
     * Check whether the MediumSyncDelay timer is running on the given link of the given device.
     *
     * @param staMac the MAC of the given device
     * @param linkId the ID of the given link
     * @param isRunning whether the MediumSyncDelay timer is running
     * @param msg message to print in case the check failed
     */
    void CheckMsdTimerRunning(Ptr<StaWifiMac> staMac,
                              uint8_t linkId,
                              bool isRunning,
                              const std::string& msg);

    /**
     * Check whether aux PHYs of the given device are in sleep mode/awake.
     *
     * @param staMac the MAC of the given device
     * @param sleep whether aux PHYs should be in sleep mode
     */
    void CheckAuxPhysSleepMode(Ptr<StaWifiMac> staMac, bool sleep);

    /**
     * Callback connected to the EMLSR Manager MainPhySwitch trace source.
     *
     * @param index the index of the EMLSR client whose main PHY switch event is logged
     * @param info the information associated with the main PHY switch event
     */
    void MainPhySwitchInfoCallback(std::size_t index, const EmlsrMainPhySwitchTrace& info);

    /**
     * Check information provided by the EMLSR Manager MainPhySwitch trace.
     *
     * @param index the ID of the EMLSR client this check refers to
     * @param reason the reason for main PHY to switch
     * @param fromLinkId the ID of the link the main PHY is moving from (if any)
     * @param toLinkId the ID of the link the main PHY is moving to
     * @param checkFromLinkId whether to check the given fromLinkId value
     * @param checkToLinkId whether to check the given toLinkId value
     */
    void CheckMainPhyTraceInfo(std::size_t index,
                               std::string_view reason,
                               const std::optional<uint8_t>& fromLinkId,
                               uint8_t toLinkId,
                               bool checkFromLinkId = true,
                               bool checkToLinkId = true);

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
        {MicroSeconds(16)}};          ///< Transition Delay advertised by the non-AP MLD
    bool m_establishBaDl{false};      /**< whether BA needs to be established (for TID 0)
                                           with the AP as originator */
    bool m_establishBaUl{false};      /**< whether BA needs to be established (for TID 0)
                                           with the AP as recipient */
    bool m_putAuxPhyToSleep{false};   //!< whether aux PHYs are put to sleep during DL/UL TXOPs
    std::vector<FrameInfo> m_txPsdus; ///< transmitted PSDUs
    Ptr<ApWifiMac> m_apMac;           ///< AP wifi MAC
    std::vector<Ptr<StaWifiMac>> m_staMacs;       ///< MACs of the non-AP MLDs
    std::vector<PacketSocketAddress> m_dlSockets; ///< packet socket address for DL traffic
    std::vector<PacketSocketAddress> m_ulSockets; ///< packet socket address for UL traffic
    uint16_t m_lastAid{0};                        ///< AID of last associated station
    Time m_duration{0};                           ///< simulation duration
    std::map<std::size_t, std::shared_ptr<EmlsrMainPhySwitchTrace>>
        m_traceInfo; ///< EMLSR client ID-indexed map of trace info from last main PHY switch

  private:
    /**
     * Set the SSID on the next station that needs to start the association procedure.
     * This method is connected to the ApWifiMac's AssociatedSta trace source.
     * Start generating traffic (if needed) when all stations are associated.
     *
     * @param aid the AID assigned to the previous associated STA
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
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the exchange of EML Operating Mode Notification frames.
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
     * @param linksToEnableEmlsrOn IDs of links on which EMLSR mode should be enabled
     * @param transitionTimeout the Transition Timeout advertised by the AP MLD
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
     * @param mpdu the acknowledged MPDU
     */
    void TxOk(Ptr<const WifiMpdu> mpdu);
    /**
     * Callback invoked when the non-AP MLD drops the given MPDU for the given reason.
     *
     * @param reason the reason why the MPDU was dropped
     * @param mpdu the dropped MPDU
     */
    void TxDropped(WifiMacDropReason reason, Ptr<const WifiMpdu> mpdu);

    /**
     * Check the content of the EML Capabilities subfield of the Multi-Link Element included
     * in the Association Request frame sent by the non-AP MLD.
     *
     * @param mpdu the MPDU containing the Association Request frame
     * @param txVector the TXVECTOR used to transmit the frame
     * @param linkId the ID of the link on which the frame was transmitted
     */
    void CheckEmlCapabilitiesInAssocReq(Ptr<const WifiMpdu> mpdu,
                                        const WifiTxVector& txVector,
                                        uint8_t linkId);
    /**
     * Check the content of the EML Capabilities subfield of the Multi-Link Element included
     * in the Association Response frame sent by the AP MLD to the EMLSR client.
     *
     * @param mpdu the MPDU containing the Association Response frame
     * @param txVector the TXVECTOR used to transmit the frame
     * @param linkId the ID of the link on which the frame was transmitted
     */
    void CheckEmlCapabilitiesInAssocResp(Ptr<const WifiMpdu> mpdu,
                                         const WifiTxVector& txVector,
                                         uint8_t linkId);
    /**
     * Check the content of a received EML Operating Mode Notification frame.
     *
     * @param psdu the PSDU containing the EML Operating Mode Notification frame
     * @param txVector the TXVECTOR used to transmit the frame
     * @param linkId the ID of the link on which the frame was transmitted
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
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the transmission of DL frames to EMLSR clients.
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
 *
 * Also, if the PutAuxPhyToSleep attribute is set to true, it is checked that aux PHYs are in
 * sleep mode after receiving an ICF and are resumed from sleep after receiving the CF-End frame.
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
        bool putAuxPhyToSleep;  //!< whether aux PHYs are put to sleep during DL/UL TXOPs
    };

    /**
     * Constructor
     *
     * @param params parameters for the EMLSR DL TXOP test
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
     * @param address a link address of the given non-AP MLD
     */
    void CheckPmModeAfterAssociation(const Mac48Address& address);

    /**
     * Check that appropriate actions are taken when the AP MLD transmits an EML Operating Mode
     * Notification response frame to an EMLSR client on the given link.
     *
     * @param mpdu the MPDU carrying the EML Operating Mode Notification frame
     * @param txVector the TXVECTOR used to send the PPDU
     * @param linkId the ID of the given link
     */
    void CheckApEmlNotificationFrame(Ptr<const WifiMpdu> mpdu,
                                     const WifiTxVector& txVector,
                                     uint8_t linkId);

    /**
     * Check that appropriate actions are taken when an EMLSR client transmits an EML Operating
     * Mode Notification frame to the AP MLD on the given link.
     *
     * @param mpdu the MPDU carrying the EML Operating Mode Notification frame
     * @param txVector the TXVECTOR used to send the PPDU
     * @param linkId the ID of the given link
     */
    void CheckStaEmlNotificationFrame(Ptr<const WifiMpdu> mpdu,
                                      const WifiTxVector& txVector,
                                      uint8_t linkId);

    /**
     * Check that appropriate actions are taken by the AP MLD transmitting an initial
     * Control frame to an EMLSR client on the given link.
     *
     * @param mpdu the MPDU carrying the MU-RTS TF
     * @param txVector the TXVECTOR used to send the PPDU
     * @param linkId the ID of the given link
     */
    void CheckInitialControlFrame(Ptr<const WifiMpdu> mpdu,
                                  const WifiTxVector& txVector,
                                  uint8_t linkId);

    /**
     * Check that appropriate actions are taken by the AP MLD transmitting a PPDU containing
     * QoS data frames to EMLSR clients on the given link.
     *
     * @param psduMap the PSDU(s) carrying QoS data frames
     * @param txVector the TXVECTOR used to send the PPDU
     * @param linkId the ID of the given link
     */
    void CheckQosFrames(const WifiConstPsduMap& psduMap,
                        const WifiTxVector& txVector,
                        uint8_t linkId);

    /**
     * Check that appropriate actions are taken by the AP MLD receiving a PPDU containing
     * BlockAck frames from EMLSR clients on the given link.
     *
     * @param psduMap the PSDU carrying BlockAck frames
     * @param txVector the TXVECTOR used to send the PPDU
     * @param phyId the ID of the PHY transmitting the PSDU(s)
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
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the transmission of UL frames from EMLSR clients.
 *
 * This test considers an AP MLD and a non-AP MLD that support EMLSR. The non-AP MLD setups three
 * links, while the set of EMLSR links is configurable. Block ack agreements (for TID 0) for both
 * DL and UL directions are established after that the EML Operating Mode Notification frames are
 * exchanged to enable the EMLSR mode on the EMLSR links. Aux PHYs on the EMLSR client do not
 * switch link, hence the main PHY will switch link (if needed) when terminating a TXOP.
 *
 * It is checked that:
 *
 * - Initially, aux PHYs are configured so that they are unable to transmit frames. Before
 *   generating the packets for the first UL data frame, transmissions on the link where the
 *   main PHY is operating and on the non-EMLSR link (if any) are blocked. Thus, the first UL data
 *   frame is held until transmissions on the link where the main PHY is operating are unblocked.
 *   The first UL data frame is sent by the main PHY without RTS protection. When the data frame
 *   exchange terminates, the MediumSyncDelay timer is started on the other EMLSR links and the
 *   CCA ED threshold is set as expected
 * - If there is a non-EMLSR link, another data frame can be sent concurrently (without protection)
 *   on the non-EMLSR link
 * - When the first UL data frame is transmitted, we make the aux PHYs on the EMLSR client capable
 *   of transmitting, we block transmissions on the link where the main PHY is operating and
 *   generate new UL packets, which will then be transmitted on a link where an aux PHY is
 *   operating. Thus, the aux PHY transmits an RTS frame and the main PHY will take over and
 *   transmit the second UL data frame. We check that, while the link on which the main PHY was
 *   operating is blocked because another EMLSR link is being used, new backoff values for that
 *   link are generated if and only if the QosTxop::GenerateBackoffIfTxopWithoutTx attribute is
 *   true; otherwise, a new backoff value is generated when the link is unblocked.
 * - When the exchange of the second UL data frame terminates, we make the aux PHY unable to
 *   transmit, block transmissions on the non-EMLSR link (if any) and generate some more UL
 *   packets, which will then be transmitted by the main PHY. However, a MediumSyncDelay timer
 *   is now running on the link where the main PHY is operating, hence transmissions are protected
 *   by an RTS frame. We install a post reception error model on the AP MLD so that all RTS frames
 *   sent by the EMLSR client are not received by the AP MLD. We check that the EMLSR client makes
 *   at most the configured max number of transmission attempts and that another UL data frame is
 *   sent once the MediumSyncDelay timer is expired. We also check that the TX width of the RTS
 *   frames and the UL data frame equal the channel width used by the main PHY.
 * - We check that no issue arises in case an aux PHY sends an RTS frame but the CTS response is
 *   not transmitted successfully. Specifically, we check that the main PHY is completing the
 *   channel switch when the (unsuccessful) reception of the CTS ends and that a new RTS/CTS
 *   exchange is carried out to protect the transmission of the last data frame.
 * - While the main PHY is operating on the same link as an aux PHY (which does not switch
 *   channel), the aux PHY is put in sleep mode as soon as the main PHY starts operating on the
 *   link, stays in sleep mode until the TXOP ends and is resumed from sleep mode right after the
 *   end of the DL/UL TXOP.
 * - When an aux PHY that is not TX capable gains a TXOP, it checks whether the main PHY can switch
 *   to the auxiliary link a start an UL TXOP. If the main PHY is switching, the aux PHY waits
 *   until the channel switch is completed and checks again; if the remaining backoff time on the
 *   preferred link is greater than the channel switch delay, the main PHY is requested to switch to
 *   the auxiliary link of the aux PHY. When the channel switch is completed, if the medium is
 *   idle on the auxiliary link and the backoff is zero, the main PHY starts an UL TXOP after a
 *   PIFS period; otherwise, the main PHY starts an UL TXOP when the backoff timer counts down to
 *   zero. The QoS data frame sent by the main PHY is not protected by RTS and the bandwidth it
 *   occupies is not affected by possible limitations on the aux PHY TX bandwidth capabilities.
 *
 * Also, if the PutAuxPhyToSleep attribute is set to true, it is checked that aux PHYs are in
 * sleep mode a SIFS after receiving the ICF and are still in sleep mode right before receiving
 * a Block Ack frame, and they are resumed from sleep after receiving the Block Ack frame.
 */
class EmlsrUlTxopTest : public EmlsrOperationsTestBase
{
  public:
    /**
     * Parameters for the EMLSR UL TXOP test
     */
    struct Params
    {
        std::set<uint8_t>
            linksToEnableEmlsrOn;       //!< IDs of links on which EMLSR mode should be enabled
        MHz_u channelWidth;             //!< width of the channels used by MLDs
        MHz_u auxPhyChannelWidth;       //!< max width supported by aux PHYs
        Time mediumSyncDuration;        //!< duration of the MediumSyncDelay timer
        uint8_t msdMaxNTxops;           //!< Max number of TXOPs that an EMLSR client is allowed
                                        //!< to attempt to initiate while the MediumSyncDelay
                                        //!< timer is running (zero indicates no limit)
        bool genBackoffAndUseAuxPhyCca; //!< this variable controls two boolean values that are
                                        //!< either both set to true or both set to false;
                                        //!< the first value controls whether the backoff should be
                                        //!< invoked when the AC gains the right to start a TXOP
                                        //!< but it does not transmit any frame, the second value
                                        //!< controls whether CCA info from aux PHY is used when
                                        //!< aux PHY is not TX capable
        uint8_t nSlotsLeftAlert;        //!< value to set the ChannelAccessManager NSlotsLeft
                                        //!< attribute to
        bool putAuxPhyToSleep;          //!< whether aux PHYs are put to sleep during DL/UL TXOPs
        bool switchMainPhyBackDelayTimeout; //!< whether a SwitchMainPhyBackDelay timer expires
                                            //!< after that the main PHY moved to an aux PHY link
    };

    /**
     * Constructor
     *
     * @param params parameters for the EMLSR UL TXOP test
     */
    EmlsrUlTxopTest(const Params& params);
    ~EmlsrUlTxopTest() override = default;

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
     * Check that appropriate actions are taken by the EMLSR client when transmitting an RTS
     * frame on the given link.
     *
     * @param mpdu the MPDU carrying the RTS frame
     * @param txVector the TXVECTOR used to send the PPDU
     * @param linkId the ID of the given link
     */
    void CheckRtsFrames(Ptr<const WifiMpdu> mpdu, const WifiTxVector& txVector, uint8_t linkId);

    /**
     * Check that appropriate actions are taken by the EMLSR client when receiving a CTS
     * frame on the given link.
     *
     * @param mpdu the MPDU carrying the CTS frame
     * @param txVector the TXVECTOR used to send the PPDU
     * @param linkId the ID of the given link
     */
    void CheckCtsFrames(Ptr<const WifiMpdu> mpdu, const WifiTxVector& txVector, uint8_t linkId);

    /**
     * Check that appropriate actions are taken when an MLD transmits a PPDU containing
     * QoS data frames on the given link.
     *
     * @param psduMap the PSDU(s) carrying QoS data frames
     * @param txVector the TXVECTOR used to send the PPDU
     * @param linkId the ID of the given link
     */
    void CheckQosFrames(const WifiConstPsduMap& psduMap,
                        const WifiTxVector& txVector,
                        uint8_t linkId);

    /**
     * Check that appropriate actions are taken when an MLD transmits a PPDU containing
     * BlockAck frames on the given link.
     *
     * @param psduMap the PSDU carrying BlockAck frames
     * @param txVector the TXVECTOR used to send the PPDU
     * @param linkId the ID of the given link
     */
    void CheckBlockAck(const WifiConstPsduMap& psduMap,
                       const WifiTxVector& txVector,
                       uint8_t linkId);

  private:
    void StartTraffic() override;

    /**
     * Callback invoked when a new backoff value is generated by the EMLSR client.
     *
     * @param backoff the generated backoff value
     * @param linkId the ID of the link for which the backoff value has been generated
     */
    void BackoffGenerated(uint32_t backoff, uint8_t linkId);

    std::set<uint8_t> m_emlsrLinks; /**< IDs of the links on which EMLSR mode has to be enabled */
    MHz_u m_channelWidth;           //!< width of the channels used by MLDs
    MHz_u m_auxPhyChannelWidth;     //!< max width supported by aux PHYs
    Time m_mediumSyncDuration;      //!< duration of the MediumSyncDelay timer
    uint8_t m_msdMaxNTxops;         //!< Max number of TXOPs that an EMLSR client is allowed
                                    //!< to attempt to initiate while the MediumSyncDelay
                                    //!< timer is running (zero indicates no limit)
    std::optional<uint8_t> m_nonEmlsrLink; //!< ID of the non-EMLSR link (if any)
    Time m_emlsrEnabledTime;              //!< when EMLSR mode has been enabled on all EMLSR clients
    Time m_firstUlPktsGenTime;            //!< generation time of the first two UL packets
    const Time m_unblockMainPhyLinkDelay; //!< delay between the time the first two UL packets are
                                          //!< generated and the time transmissions are unblocked
                                          //!< on the link where the main PHY is operating on
    Time m_lastMsdExpiryTime;             //!< expiry time of the last MediumSyncDelay timer
    bool m_checkBackoffStarted;           //!< whether we are checking the generated backoff values
    std::optional<Time> m_backoffEndTime; //!< expected backoff end time on main PHY link
    Ptr<ListErrorModel> m_errorModel;     ///< error rate model to corrupt packets
    std::size_t m_countQoSframes;         //!< counter for QoS frames
    std::size_t m_countBlockAck;          //!< counter for BlockAck frames
    std::size_t m_countRtsframes;         //!< counter for RTS frames
    bool m_genBackoffIfTxopWithoutTx;     //!< whether the backoff should be invoked when the AC
                                          //!< gains the right to start a TXOP but it does not
                                          //!< transmit any frame
    bool m_useAuxPhyCca;                  //!< whether CCA info from aux PHY is used when
                                          //!< aux PHY is not TX capable
    uint8_t m_nSlotsLeftAlert;            //!< value for ChannelAccessManager NSlotsLeft attribute
    bool m_switchMainPhyBackDelayTimeout; //!< whether a SwitchMainPhyBackDelay timer expires
                                          //!< after that the main PHY moved to an aux PHY link
    std::optional<bool> m_corruptCts;     //!< whether the transmitted CTS must be corrupted
    Time m_5thQosFrameTxTime;             //!< start transmission time of the 5th QoS data frame
    MHz_u m_5thQosFrameExpWidth;          //!< expected width of the 5th QoS data frame
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Check UL OFDMA operations with EMLSR clients.
 *
 * This test considers an AP MLD and an EMLSR client and a non-AP MLD that setup three links with
 * the AP MLD. Once block ack agreements (for TID 0) are established for the UL direction, the
 * AP MLD starts requesting channel access (on all the links) through the Multi-User scheduler.
 * Given that links are idle, AP MLD accesses the channel on all the links and concurrently sends
 * Trigger Frames. When the transmission of the first Trigger Frame is over, a client application
 * on the EMLSR client generates two packets addressed to the AP MLD.
 *
 * It is checked that:
 * - when sending BSRP TF is disabled, the first Trigger Frame sent is an MU-RTS; otherwise, it is
 *   a BSRP Trigger Frame. In both cases, such Trigger Frame acts as an ICF for the EMLSR client
 * - the other Trigger Frames sent concurrently with the ICF only solicit the non-EMLSR client
 *   (AP MLD has blocked transmissions to the EMLSR client upon preparing the first Trigger Frame)
 * - the buffer status reported in QoS Null frames is as expected
 * - the EMLSR client sends a QoS Data frame in a TB PPDU
 */
class EmlsrUlOfdmaTest : public EmlsrOperationsTestBase
{
  public:
    /**
     * Constructor
     *
     * @param enableBsrp whether MU scheduler sends BSRP TFs
     */
    EmlsrUlOfdmaTest(bool enableBsrp);

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

  private:
    void StartTraffic() override;

    bool m_enableBsrp;        //!< whether MU scheduler sends BSRP TFs
    std::size_t m_txPsdusPos; //!< position in the vector of TX PSDUs of the first ICF
    Time m_startAccessReq;    //!< start time of the first AP MLD access request via MU scheduler
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the switching of PHYs on EMLSR clients.
 *
 * An AP MLD and an EMLSR client setup 3 links, on which EMLSR mode is enabled. The AP MLD
 * transmits 4 QoS data frames (one after another, each protected by ICF):
 *
 * - the first one on the link used for ML setup, hence no PHY switch occurs
 * - the second one on another link, thus causing the main PHY to switch link
 * - the third one on the remaining link, thus causing the main PHY to switch link again
 * - the fourth one on the link used for ML setup
 *
 * Afterwards, the EMLSR client transmits 2 QoS data frames; the first one on the link used for
 * ML setup (hence, no RTS is sent), the second one on another link.
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
        bool resetCamStateAndInterruptSwitch; //!< this variable controls two boolean values that
                                              //!< are either both set to true or both set to false;
                                              //!< the first value controls whether to reset the
                                              //!< state of the ChannelAccessManager associated
                                              //!< with the link on which the main PHY has just
                                              //!< switched to, the second value controls whether
                                              //!< a main PHY channel switch can be interrupted
        MHz_u auxPhyMaxChWidth;               //!< max channel width supported by aux PHYs
    };

    /**
     * Constructor
     *
     * @param params parameters for the EMLSR link switching test
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
     * @param psduMap the PSDU carrying the MU-RTS TF
     * @param txVector the TXVECTOR used to send the PPDU
     * @param linkId the ID of the given link
     */
    void CheckInitialControlFrame(const WifiConstPsduMap& psduMap,
                                  const WifiTxVector& txVector,
                                  uint8_t linkId);

    /**
     * Check that appropriate actions are taken by the AP MLD transmitting a PPDU containing
     * QoS data frames to the EMLSR client on the given link.
     *
     * @param psduMap the PSDU(s) carrying QoS data frames
     * @param txVector the TXVECTOR used to send the PPDU
     * @param linkId the ID of the given link
     */
    void CheckQosFrames(const WifiConstPsduMap& psduMap,
                        const WifiTxVector& txVector,
                        uint8_t linkId);

    /**
     * Check that appropriate actions are taken by the EMLSR client transmitting a PPDU containing
     * an RTS frame to the AP MLD on the given link.
     *
     * @param psduMap the PSDU carrying RTS frame
     * @param txVector the TXVECTOR used to send the PPDU
     * @param linkId the ID of the given link
     */
    void CheckRtsFrame(const WifiConstPsduMap& psduMap,
                       const WifiTxVector& txVector,
                       uint8_t linkId);

  private:
    bool m_switchAuxPhy; /**< whether AUX PHY should switch channel to operate on the link on which
                              the Main PHY was operating before moving to the link of Aux PHY */
    bool
        m_resetCamStateAndInterruptSwitch; /**< whether to reset the state of the
                              ChannelAccessManager associated with the link on which the main PHY
                              has just switched to and whether main PHY switch can be interrupted */
    MHz_u m_auxPhyMaxChWidth;              //!< max channel width supported by aux PHYs
    std::size_t m_countQoSframes;          //!< counter for QoS data frames
    std::size_t m_countIcfFrames;          //!< counter for ICF frames
    std::size_t m_countRtsFrames;          //!< counter for RTS frames
    std::size_t m_txPsdusPos;              //!< position in the vector of TX PSDUs of the first ICF
    Ptr<ListErrorModel> m_errorModel;      ///< error rate model to corrupt packets at AP MLD
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi EMLSR Test Suite
 */
class WifiEmlsrTestSuite : public TestSuite
{
  public:
    WifiEmlsrTestSuite();
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test CCA busy notifications on EMLSR clients.
 *
 * SwitchAuxPhy is set to true, so that the aux PHY starts switching when the main PHY switch is
 * completed.
 *
 * - Main PHY switches to a link on which an aux PHY is operating. Right after the start of the
 *   channel switch, the AP transmits a frame to another device on the aux PHY link. Verify that,
 *   once the main PHY is operating on the new link, the channel access manager on that link is
 *   notified of CCA busy until the end of the transmission
 * - When the main PHY switch is completed, the aux PHY switches to a link on which no PHY is
 *   operating. Before the aux PHY starts switching, the AP starts transmitting a frame to another
 *   device on the link on which no PHY is operating. Verify that, once the aux PHY is operating
 *   on the new link, the channel access manager on that link is notified of CCA busy until the
 *   end of the transmission
 */
class EmlsrCcaBusyTest : public EmlsrOperationsTestBase
{
  public:
    /**
     * Constructor
     *
     * @param auxPhyMaxChWidth max channel width supported by aux PHYs
     */
    EmlsrCcaBusyTest(MHz_u auxPhyMaxChWidth);

    ~EmlsrCcaBusyTest() override = default;

  protected:
    void DoSetup() override;
    void DoRun() override;

  private:
    void StartTraffic() override;

    /**
     * Make the other MLD transmit a packet to the AP on the given link.
     *
     * @param linkId the ID of the given link
     */
    void TransmitPacketToAp(uint8_t linkId);

    /**
     * Perform checks after that the preamble of the first PPDU has been received.
     */
    void CheckPoint1();

    /**
     * Perform checks after that the main PHY completed the link switch.
     */
    void CheckPoint2();

    /**
     * Perform checks after that the aux PHY completed the link switch.
     */
    void CheckPoint3();

    MHz_u m_auxPhyMaxChWidth;    //!< max channel width supported by aux PHYs
    Time m_channelSwitchDelay;   //!< the PHY channel switch delay
    uint8_t m_currMainPhyLinkId; //!< the ID of the link the main PHY switches from
    uint8_t m_nextMainPhyLinkId; //!< the ID of the link the main PHY switches to
};

#endif /* WIFI_EMLSR_TEST_H */
