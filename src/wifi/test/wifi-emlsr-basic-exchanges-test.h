/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_EMLSR_BASIC_EXCHANGES_TEST_H
#define WIFI_EMLSR_BASIC_EXCHANGES_TEST_H

#include "wifi-emlsr-test-base.h"

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
        bool genBackoffIfTxopWithoutTx; //!< whether the backoff should be invoked when the AC gains
                                        //!< a TXOP but it does not transmit any frame
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
    std::optional<bool> m_corruptCts;     //!< whether the transmitted CTS must be corrupted
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
 *
 * It is also checked that, when sending BSRP TF is enabled and the single protection mechanism is
 * used, the QoS Null frames sent in response to the BSRP TF acting as ICF have a Duration/ID of
 * zero but the EMLSR client does not consider the TXOP ended when the transmission of the QoS Null
 * frame ends (because in this case the ns3::EhtFrameExchangeManager::EarlyTxopEndDetect attribute
 * is set to false) and therefore correctly receives the Basic Trigger Frame sent after a SIFS.
 */
class EmlsrUlOfdmaTest : public EmlsrOperationsTestBase
{
  public:
    /**
     * Constructor
     *
     * @param enableBsrp whether MU scheduler sends BSRP TFs
     * @param protectSingleExchange whether single protection mechanism is used
     */
    EmlsrUlOfdmaTest(bool enableBsrp, bool protectSingleExchange);

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

    bool m_enableBsrp;            //!< whether MU scheduler sends BSRP TFs
    bool m_protectSingleExchange; //!< whether single protection mechanism is used
    std::size_t m_txPsdusPos;     //!< position in the vector of TX PSDUs of the first ICF
    Time m_startAccessReq; //!< start time of the first AP MLD access request via MU scheduler
    std::optional<uint8_t> m_1stTfLinkId; //!< ID of the link on which the first TF is sent
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi EMLSR suite to test basic frame exchanges.
 */
class WifiEmlsrBasicExchangesTestSuite : public TestSuite
{
  public:
    WifiEmlsrBasicExchangesTestSuite();
};

#endif /* WIFI_EMLSR_BASIC_EXCHANGES_TEST_H */
