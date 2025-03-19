/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_EMLSR_LINK_SWITCH_TEST_H
#define WIFI_EMLSR_LINK_SWITCH_TEST_H

#include "wifi-emlsr-test-base.h"

#include <list>

using namespace ns3;
using namespace std::string_literals;

// forward declaration
namespace ns3
{
struct EmlsrMainPhySwitchTrace;
}

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

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test ML setup and data exchange between an AP MLD and a single link EMLSR client.
 *
 * A single link EMLSR client performs ML setup with an AP MLD having three links and then enables
 * EMLSR mode on the unique link. A Block Ack agreement is established (for both the downlink and
 * uplink directions) and QoS data frames (aggregated in an A-MPDU) are transmitted by both the
 * AP MLD and the EMLSR client.
 *
 * It is checked that:
 * - the expected sequence of frames is transmitted, including ICFs before downlink transmissions
 * - EMLSR mode is enabled on the single EMLSR link
 * - the address of the EMLSR client is seen as an MLD address
 * - the AP MLD starts the transition delay timer at the end of each TXOP
 */
class SingleLinkEmlsrTest : public EmlsrOperationsTestBase
{
  public:
    /**
     * Constructor.
     *
     * @param switchAuxPhy whether aux PHYs switch link
     * @param auxPhyTxCapable whether aux PHYs are TX capable
     */
    SingleLinkEmlsrTest(bool switchAuxPhy, bool auxPhyTxCapable);

  protected:
    void DoSetup() override;
    void DoRun() override;

    void Transmit(Ptr<WifiMac> mac,
                  uint8_t phyId,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW) override;

    /// Actions and checks to perform upon the transmission of each frame
    struct Events
    {
        /**
         * Constructor.
         *
         * @param type the frame MAC header type
         * @param f function to perform actions and checks
         */
        Events(WifiMacType type,
               std::function<void(Ptr<const WifiPsdu>, const WifiTxVector&)>&& f = {})
            : hdrType(type),
              func(f)
        {
        }

        WifiMacType hdrType; ///< MAC header type of frame being transmitted
        std::function<void(Ptr<const WifiPsdu>, const WifiTxVector&)>
            func; ///< function to perform actions and checks
    };

  private:
    bool m_switchAuxPhy;                         //!< whether aux PHYs switch link
    bool m_auxPhyTxCapable;                      //!< whether aux PHYs are TX capable
    std::list<Events> m_events;                  //!< list of events for a test run
    std::list<Events>::const_iterator m_eventIt; //!< iterator over the list of events
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Check ICF reception while main PHY is switching.
 *
 * An AP MLD and an EMLSR client, both having 3 links, are considered in this test. Aux PHYs are
 * not TX capable and do not switch links; the preferred link is link 0. In order to control link
 * switches, a TID-to-Link mapping is configured so that TIDs 0 and 3 are mapped onto link 1 in the
 * DL direction, while TID 0 is mapped to link 1 and TID 3 is mapped to link 2 in the UL direction.
 * In this way, the AP MLD always requests channel access on link 1, while the EMLSR client requests
 * channel access on link 1 or link 2, depending on the TID. This test consists in having the AP MLD
 * and the EMLSR client gain channel access simultaneously: the AP MLD starts transmitting an ICF,
 * while the main PHY starts switching to the link on which the EMLSR client gained channel access,
 * which could be either the link on which the ICF is being transmitted or another one, depending
 * on the TID of the MPDU the EMLSR client has to transmit.
 *
 * The channel switch delay for the main PHY varies across test scenarios and is computed so that
 * the channel switch terminates during one of the different steps of the reception of the ICF:
 * during preamble detection period, before the PHY header end, before the MAC header end, before
 * the padding start and after the padding start.
 *
   \verbatim
   ┌────────┬──────┬──────┬────────────────────┬───────┐
   │PREAMBLE│  PHY │  MAC │    MAC PAYLOAD     │       │
   │ DETECT │HEADER│HEADER│(COMMON & USER INFO)│PADDING│
   └────────┴──────┴──────┴────────────────────┴───────┘
   \endverbatim
 *
 * All the combinations of the following are tested:
 * - main PHY switches to the same link as ICF or to another link
 * - channel switch can be interrupted or not
 * - MAC header reception information is available and can be used or not
 *
 * In all the cases, we check that the EMLSR client responds to the ICF:
 * - if the main PHY switches to the same link as the ICF, connecting the main PHY to the link is
 *   postponed until the end of the ICF
 * - if the main PHY switches to another link, the UL TXOP does not start because it is detected
 *   that a frame which could be an ICF is being received on another link
 *
 * At the end of the DL TXOP, it is checked that:
 * - if the KeepMainPhyAfterDlTxop attribute of the AdvancedEmlsrManager is false, the main PHY
 *   switches back to the preferred link
 * - if the KeepMainPhyAfterDlTxop attribute of the AdvancedEmlsrManager is true, the main PHY
 *   stays on the current link to start an UL TXOP, if the UL frame can be sent on the same link
 *   as the DL frame, or switches back to the preferred link, otherwise
 *
 * At the end of the UL TXOP, the main PHY returns to the preferred link.
 *
 * It is also checked that the in-device interference generated by every transmission of the EMLSR
 * client is tracked by all the PHY interfaces of all the PHYs but the PHY that is transmitting
 * for the entire duration of the transmission.
 */
class EmlsrIcfSentDuringMainPhySwitchTest : public EmlsrOperationsTestBase
{
  public:
    /// Constructor.
    EmlsrIcfSentDuringMainPhySwitchTest();

    /**
     * Enumeration indicating the duration of a main PHY channel switch compared to the ICF fields
     */
    enum ChannelSwitchEnd : uint8_t
    {
        DURING_PREAMBLE_DETECTION = 0,
        BEFORE_PHY_HDR_END,
        BEFORE_MAC_HDR_END,
        BEFORE_MAC_PAYLOAD_END,
        BEFORE_PADDING_END,
        CSD_COUNT
    };

  protected:
    void DoSetup() override;
    void DoRun() override;
    void Transmit(Ptr<WifiMac> mac,
                  uint8_t phyId,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW) override;

    /// Runs a test case and invokes itself for the next test case
    void RunOne();

    /**
     * Callback connected to the EmlsrLinkSwitch trace source of the StaWifiMac of the EMLSR client.
     *
     * @param linkId the ID of the link which the PHY is connected to/disconnected from
     * @param phy a pointer to the PHY that is connected to/disconnected from the given link
     * @param connected true if the PHY is connected, false if the PHY is disconnected
     */
    void EmlsrLinkSwitchCb(uint8_t linkId, Ptr<WifiPhy> phy, bool connected);

    /**
     * Generate noise on all the links of the given MAC for the given time duration. This is used
     * to align the EDCA backoff boundary on all the links for the given MAC.
     *
     * @param mac the given MAC
     * @param duration the given duration
     */
    void GenerateNoiseOnAllLinks(Ptr<WifiMac> mac, Time duration);

    /**
     * Check that the in-device interference generated by a transmission of the given duration
     * on the given link is tracked by all the PHY interfaces of all the PHYs but the PHY that
     * is transmitting.
     *
     * @param testStr the test string
     * @param linkId the ID of the given link
     * @param duration the duration of the transmission
     */
    void CheckInDeviceInterference(const std::string& testStr, uint8_t linkId, Time duration);

    /// Actions and checks to perform upon the transmission of each frame
    struct Events
    {
        /**
         * Constructor.
         *
         * @param type the frame MAC header type
         * @param f function to perform actions and checks
         */
        Events(WifiMacType type,
               std::function<void(Ptr<const WifiPsdu>, const WifiTxVector&, uint8_t)>&& f = {})
            : hdrType(type),
              func(f)
        {
        }

        WifiMacType hdrType; ///< MAC header type of frame being transmitted
        std::function<void(Ptr<const WifiPsdu>, const WifiTxVector&, uint8_t)>
            func; ///< function to perform actions and checks
    };

    /// Store information about a main PHY switch
    struct MainPhySwitchInfo
    {
        Time time;      ///< the time the main PHY left/was connected to a link
        uint8_t linkId; ///< the ID of the link the main PHY switched from/to
    };

  private:
    void StartTraffic() override;

    ChannelSwitchEnd m_csdIndex{
        BEFORE_PHY_HDR_END}; //!< index to iterate over channel switch durations
    uint8_t m_testIndex{0};  //!< index to iterate over test scenarios
    std::string m_testStr;   //!< test scenario description
    bool m_setupDone{false}; //!< whether association, BA, ... have been done
    std::optional<MainPhySwitchInfo> m_switchFrom; //!< info for main PHY leaving a link
    std::optional<MainPhySwitchInfo> m_switchTo;   //!< info for main PHY connected to a link
    std::array<WifiSpectrumBandInfo, 3> m_bands;   //!< bands of the 3 frequency channels
    std::list<Events> m_events;                    //!< list of events for a test run
    std::size_t m_processedEvents{0};              //!< number of processed events
    const uint8_t m_linkIdForTid3{2};              //!< ID of the link on which TID 3 is mapped
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Switch main PHY back timer test
 *
 * An AP MLD and an EMLSR client, both having 3 links, are considered in this test. Aux PHYs are
 * not TX capable, do not switch links and support up to the HT modulation class; the preferred link
 * is link 2. In order to control link switches, a TID-to-Link mapping is configured so that TID 0
 * is mapped onto link 1 and TID 4 is mapped onto link 0 (for both DL and UL). In this test, the
 * main PHY switches to link 1 to start an UL TXOP but, while the main PHY is switching or shortly
 * after the channel switch ends, the AP MLD transmits a QoS Data broadcast frame on link 1 using a
 * modulation supported by the aux PHYs. Different situations are tested and it is verified that
 * the main PHY switches back to the preferred link as expected. Scenarios:
 *
 * - RXSTART_WHILE_SWITCH_NO_INTERRUPT: the AP MLD transmits an HT PPDU while the main PHY is
 *   switching; at the end of the PHY header reception (while the main PHY is still switching), the
 *   MAC of the EMLSR client receives the RX start notification, which indicates that the modulation
 *   is HT (hence the PPDU does not carry an ICF) and the PPDU duration exceeds the switch main PHY
 *   back delay. The EMLSR client decides to switch the main PHY back to the preferred link (with
 *   reason RX_END), but the actual main PHY switch is postponed until the ongoing channel switch
 *   terminates.
 * - RXSTART_WHILE_SWITCH_INTERRUPT: same as previous scenario, except that the main PHY switch can
 *   be interrupted, hence the main PHY switches back to the preferred link as soon as the reception
 *   of the PHY header ends.
 * - RXSTART_AFTER_SWITCH_HT_PPDU: the AP MLD transmits an HT PPDU some time after the main PHY
 *   starts switching to link 1; the delay is computed so that the RX START notification is sent
 *   after that the main PHY has completed the channel switch. When the main PHY completes the
 *   switch to link 1, it is determined that the PPDU being received (using HT modulation) cannot
 *   be an ICF, hence the main PHY is connected to link 1. Connecting the main PHY to link 1
 *   triggers a CCA busy notification until the end of the PPDU (we assume this information is
 *   available from the PHY header decoded by the aux PHY), thus the main PHY switches back to the
 *   preferred link (with reason BUSY_END).
 * - NON_HT_PPDU_DONT_USE_MAC_HDR: the AP MLD transmits a non-HT PPDU on link 1 (it does not really
 *   matter if the RX START notification is sent before or after the end of main PHY switch). When
 *   the main PHY completes the switch to link 1, it is detected that the aux PHY on link 1 is
 *   receiving a PPDU which may be an ICF (the modulation is non-HT), hence the main PHY is not
 *   connected to link 1 until the end of the PPDU reception (MAC header info is not used). At that
 *   time, it is detected that the PPDU does not contain an ICF, but it is determined that channel
 *   access can be gained before the end of the switch main PHY back timer, hence the main PHY stays
 *   on link 1 and transmits its unicast data frame. The start of the UL TXOP cancels the main PHY
 *   switch back timer and the main PHY switches back to the preferred link at the end of the TXOP.
 * - NON_HT_PPDU_USE_MAC_HDR: same as previous scenario, except that the MAC header info can be
 *   used. After completing the channel switch, the main PHY is not connected to link 1 because the
 *   non-HT PPDU being received may be an ICF. When the MAC header info is notified, it is detected
 *   that the PPDU does not contain an ICF, channel access would not be gained before the end of the
 *   switch main PHY back timer and therefore the main PHY switches back to the preferred link (with
 *   reason RX_END).
 * - LONG_SWITCH_BACK_DELAY_DONT_USE_MAC_HDR: same as the NON_HT_PPDU_DONT_USE_MAC_HDR scenario,
 *   except that the switch main PHY back delay is longer and exceeds the PPDU duration, but it is
 *   does not exceed the PPDU duration plus AIFS and the backoff slots. Therefore, at the end of the
 *   PPDU reception, it is determined that the backoff counter will not reach zero before the end of
 *   the switch main PHY back timer plus a channel switch delay and the main PHY switches back to
 *   the preferred link (with reason BACKOFF_END).
 * - LONG_SWITCH_BACK_DELAY_USE_MAC_HDR: same as the NON_HT_PPDU_USE_MAC_HDR scenario,
 *   except that the switch main PHY back delay is longer and exceeds the PPDU duration, but it
 *   does not exceed the PPDU duration plus AIFS and the backoff slots. Therefore, at the end of the
 *   MAC header reception, it is determined that the backoff counter will not reach zero before the
 *   end of the switch main PHY back timer plus a channel switch delay and the main PHY switches
 *   back to the preferred link (with reason BACKOFF_END).
 *
 * Except for the NON_HT_PPDU_DONT_USE_MAC_HDR case, in which the main PHY stays on link 1 and
 * transmits a data frame, receives the Ack and switches back to the preferred link at the TXOP end,
 * in all other cases the main PHY switches back to the preferred link without sending a frame on
 * link 1. A few microseconds after starting the switch to the preferred link, a frame with TID 4
 * is queued. If interrupt switching is enabled, the switch to the preferred link is interrupted
 * and the main PHY switches to link 0, where it transmits the data frame with TID 4 as soon as
 * completing the switch. Afterwards, the main PHY switches back to the preferred link and, except
 * for the NON_HT_PPDU_DONT_USE_MAC_HDR case, it switches to link 1 to transmit the queued frame
 * with TID 0.
 */
class EmlsrSwitchMainPhyBackTest : public EmlsrOperationsTestBase
{
  public:
    /// Constructor.
    EmlsrSwitchMainPhyBackTest();

    /**
     * Enumeration indicating the tested scenario
     */
    enum TestScenario : uint8_t
    {
        RXSTART_WHILE_SWITCH_NO_INTERRUPT = 0,
        RXSTART_WHILE_SWITCH_INTERRUPT,
        RXSTART_AFTER_SWITCH_HT_PPDU,
        NON_HT_PPDU_DONT_USE_MAC_HDR,
        NON_HT_PPDU_USE_MAC_HDR,
        LONG_SWITCH_BACK_DELAY_DONT_USE_MAC_HDR,
        LONG_SWITCH_BACK_DELAY_USE_MAC_HDR,
        COUNT
    };

  protected:
    void DoSetup() override;
    void DoRun() override;
    void Transmit(Ptr<WifiMac> mac,
                  uint8_t phyId,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW) override;
    void MainPhySwitchInfoCallback(std::size_t index, const EmlsrMainPhySwitchTrace& info) override;

    /// Insert events corresponding to the UL TXOP to transmit the QoS Data frame with TID 4
    void InsertEventsForQosTid4();

    /// Runs a test case and invokes itself for the next test case
    void RunOne();

    /// Actions and checks to perform upon the transmission of each frame
    struct Events
    {
        /**
         * Constructor.
         *
         * @param type the frame MAC header type
         * @param f function to perform actions and checks
         */
        Events(WifiMacType type,
               std::function<void(Ptr<const WifiPsdu>, const WifiTxVector&, uint8_t)>&& f = {})
            : hdrType(type),
              func(f)
        {
        }

        WifiMacType hdrType; ///< MAC header type of frame being transmitted
        std::function<void(Ptr<const WifiPsdu>, const WifiTxVector&, uint8_t)>
            func; ///< function to perform actions and checks
    };

  private:
    void StartTraffic() override;

    uint8_t m_testIndex{0};               //!< index to iterate over test scenarios
    bool m_setupDone{false};              //!< whether association, BA, ... have been done
    bool m_dlPktDone{false};              //!< whether the DL packet has been generated
    std::list<Events> m_events;           //!< list of events for a test run
    std::size_t m_processedEvents{0};     //!< number of processed events
    const uint8_t m_linkIdForTid0{1};     //!< ID of the link on which TID 0 is mapped
    const uint8_t m_linkIdForTid4{0};     //!< ID of the link on which TID 4 is mapped
    Ptr<WifiMpdu> m_bcastFrame;           //!< the broadcast frame sent by the AP MLD
    Time m_switchMainPhyBackDelay;        //!< the switch main PHY back delay
    Time m_expectedMainPhySwitchBackTime; //!< expected main PHY switch back time
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Check NAV and CCA in the last PIFS test
 *
 * An AP MLD and an EMLSR client, both having 3 links, are considered in this test. Aux PHYs are
 * not TX capable, do not switch links and operate on 20 MHz channels; the main PHY operates on
 * 40 MHz channels and the preferred link is link 1. In order to control link switches, a
 * TID-to-Link mapping is configured so that TID 0 is mapped onto link 2 for both DL and UL. In this
 * test, the main PHY switches to link 2 to start an UL TXOP a predefined number of slots before
 * the backoff ends on link 2. We consider different durations of the channel switch delay to
 * verify the time the data frame is transmitted by the EMLSR client on link 2 and the data frame
 * TX width in various situations:
 *
 *        AuxPhyCca = false                           AuxPhyCca = true
 *                          ┌────┐                             ┌────┐
 *                          │QoS │40                           │QoS │20
 *                 |--PIFS--│Data│MHz                 |--PIFS--│Data│MHz
 * ──────┬─────────┬────────┴────┴────         ──────┬─────────┼────┴─────────────
 *    Backoff    Switch                           Backoff    Switch
 *      end       end                               end       end
 *
 *
 *        AuxPhyCca = false                           AuxPhyCca = true
 *                   ┌────┐                                    ┌────┐
 *                   │QoS │40                                  │QoS │20
 *          |--PIFS--│Data│MHz                        |--PIFS--│Data│MHz
 * ─────────┬──────┬─┴────┴───────────         ──────────┬─────┼────┴─────────────
 *       Switch  Backoff                              Switch Backoff
 *         end    end                                   end   end
 *
 *
 *        AuxPhyCca = false/true
 *                      ┌────┐
 *                      │QoS │40
 *          |--PIFS--|  │Data│MHz
 * ─────────┬───────────┼────┴────────
 *       Switch      Backoff
 *         end         end
 *
 * In all the cases, it is verified that the EMLSR client transmits the data frame, at the expected
 * time and on the expected channel width, and receives the acknowledgment.
 */
class EmlsrCheckNavAndCcaLastPifsTest : public EmlsrOperationsTestBase
{
  public:
    /// Constructor.
    EmlsrCheckNavAndCcaLastPifsTest();

    /**
     * Enumeration indicating the tested scenario
     */
    enum TestScenario : uint8_t
    {
        BACKOFF_END_BEFORE_SWITCH_END = 0,
        LESS_THAN_PIFS_UNTIL_BACKOFF_END,
        MORE_THAN_PIFS_UNTIL_BACKOFF_END,
        COUNT
    };

  protected:
    void DoSetup() override;
    void DoRun() override;
    void Transmit(Ptr<WifiMac> mac,
                  uint8_t phyId,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW) override;

    /// Runs a test case and invokes itself for the next test case
    void RunOne();

    /// Actions and checks to perform upon the transmission of each frame
    struct Events
    {
        /**
         * Constructor.
         *
         * @param type the frame MAC header type
         * @param f function to perform actions and checks
         */
        Events(WifiMacType type,
               std::function<void(Ptr<const WifiPsdu>, const WifiTxVector&, uint8_t)>&& f = {})
            : hdrType(type),
              func(f)
        {
        }

        WifiMacType hdrType; ///< MAC header type of frame being transmitted
        std::function<void(Ptr<const WifiPsdu>, const WifiTxVector&, uint8_t)>
            func; ///< function to perform actions and checks
    };

  private:
    void StartTraffic() override;

    std::size_t m_testIndex{0};       //!< index to iterate over test scenarios
    bool m_setupDone{false};          //!< whether association, BA, ... have been done
    std::list<Events> m_events;       //!< list of events for a test run
    std::size_t m_processedEvents{0}; //!< number of processed events
    const uint8_t m_linkIdForTid0{2}; //!< ID of the link on which TID 0 is mapped
    const uint8_t m_nSlotsLeft{4};    //!< value for the CAM NSlotsLeft attribute
    const MHz_u m_mainPhyWidth{40};   //!< main PHY channel width
    const MHz_u m_auxPhyWidth{20};    //!< aux PHY channel width
    Time m_expectedTxStart;           //!< expected start time for frame transmission
    MHz_u m_expectedWidth;            //!< expected channel width for frame transmission
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi EMLSR suite to test link switch operations
 */
class WifiEmlsrLinkSwitchTestSuite : public TestSuite
{
  public:
    WifiEmlsrLinkSwitchTestSuite();
};

#endif /* WIFI_EMLSR_LINK_SWITCH_TEST_H */
