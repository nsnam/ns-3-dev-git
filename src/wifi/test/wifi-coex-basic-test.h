/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_COEX_BASIC_TEST_H
#define WIFI_COEX_BASIC_TEST_H

#include "wifi-coex-test-base.h"

using namespace ns3;

/**
 * @brief Verify that a DL frame exchange sequence is interrupted when a coex event starts.
 *
 * This test considers an AP transmitting a DL A-MPDU (comprising 2 MPDUs) to an associated non-AP
 * STA. Different scenarios are considered, varying for the time a coex event starts:
 *
 * 1) before the start of the PPDU
 * 2) during the preamble detection period
 * 3) during the reception of the PHY header
 * 4) during the reception of the first MPDU
 * 5) during the reception of the second MPDU
 * 6) before the expected start time of the BlockAck transmission
 * 7) after the expected start time of the BlockAck transmission
 *
 * BEFORE_PPDU_START scenario:
 *
 *    Coex event
 *   notification        ┌────────┬──────────┬──────┬──────┐
 *    and start          │Preamble│   PHY    │ MPDU │ MPDU │
 *        │              │ detect │  Header  │  1   │  2   │
 *  ──────▼──────────────┴────────┴▲─────────┴──────┴──────┴────────────────
 *                                 │
 *                             Coex event
 *                                end
 *
 * Other scenarios (the duration of coex event is constant):
 *
 *                   Coex event
 *                  notification
 *              (PPDU start + offset)
 *                        │
 *                       ┌▼─────────┬──────────┬──────────┬──────────┐
 *                       │ Preamble │   PHY    │   MPDU   │   MPDU   │   SIFS
 *                       │  detect  │  Header  │    1     │    2     │- - - - -|
 *  ─────────────────────┴▲─────────┴▲─────────┴▲─────────┴▲─────────┴▲──────────▲───────────
 *                        │          │          │          │          │          │
 *                   Coex event Coex event Coex event Coex event Coex event Coex event
 *                     start      start      start      start      start      start
 *                   (DURING    (DURING    (DURING    (DURING    (BEFORE    (DURING
 *                  PREAMBLE       PHY       FIRST     SECOND     BLOCKACK)  BLOCKACK)
 *                  DETECTION)   HEADER)      MPDU)     MPDU)
 *
 * It is verified that:
 * - the PHY of the STA is in OFF state right after the coex event starts and is in CCA_BUSY state
 *   right after the coex event ends
 * - if the coex event starts during the preamble detection period, the event associated with the
 *   end of the preamble detection period is cancelled
 * - if the coex event starts during the reception of the PHY header, the reception of the PHY
 *   header is cancelled
 * - if the coex event starts during the reception of an MPDU, the reception of the MPDU and of the
 *   remaining MPDUs is cancelled
 * - in the DURING_SECOND_MPDU scenario, the first MPDU is successfully received and passed to the
 *   upper layers; in the BEFORE_BLOCKACK and DURING_BLOCKACK scenarios, both MPDUs are successfully
 *   received and passed to the upper layers; in the other scenarios, no MPDU is successfully
 *   received
 * - in all the scenarios but BEFORE_BLOCKACK and DURING_BLOCKACK, the reception of the PPDU fails;
 *   in all the scenarios, the non-AP STA does not send any acknowledgment
 * - the A-MPDU (comprising both MPDUs) is retransmitted after the BlockAck timeout and acknowledged
 */
class WifiCoexDlTxopTest : public WifiCoexTestBase
{
  public:
    /**
     * Enumeration of the scenarios covered by this unit test. Scenarios vary for the time when the
     * coex event starts.
     */
    enum Scenario : uint8_t
    {
        BEFORE_PPDU_START = 0,
        DURING_PREAMBLE_DETECTION,
        DURING_PHY_HEADER,
        DURING_FIRST_MPDU,
        DURING_SECOND_MPDU,
        BEFORE_BLOCKACK,
        DURING_BLOCKACK
    };

    /**
     * Constructor
     *
     * @param scenario the scenario to test
     */
    WifiCoexDlTxopTest(Scenario scenario);

  protected:
    void DoSetup() override;
    void DoRun() override;
    void CoexEventCallback(const coex::Event& coexEvent) override;

    /// Insert elements in the list of expected events (transmitted frames)
    void InsertEvents();

  private:
    const Scenario m_scenario;                          ///< the scenario to test
    const Time m_coexEventPpduOverlap{MicroSeconds(5)}; ///< time overlap between the coex event and
                                                        ///< the received PPDU
    const Time m_coexEventPpduOffset{MicroSeconds(2)};  ///< the offset between the PPDU start and
                                                       ///< the coex event notification (if scenario
                                                       ///< is not BEFORE_PPDU_START)
    Time m_dlPktTxTime;         ///< TX time for the DL packet
    Time m_mockInterEventTime;  ///< time interval between two consecutive coex events generated by
                                ///< the mock generator
    Time m_mockEventStartDelay; ///< delay between the time the coex event is notified and the time
                                ///< the coex event starts
    Time m_mockEventDuration;   ///< duration of a coex event
    const std::size_t m_nPackets{2}; ///< number of generated packets
};

/**
 * @brief Verify that an UL frame exchange sequence is deferred/interrupted when a coex event
 * starts.
 *
 * This test considers a non-AP STA having to transmit two packets to the associated AP. The
 * following scenarios are considered (for both zero and non-zero TXOP limit):
 *
 *
 * DEFER_TXOP_START scenario:
 * A coex event is notified before packets are enqueued. The start of the coex event is such that
 * there is not enough time to transmit even a single packet. The STA defer the transmission of
 * both packets until after the coex event ends.
 *
 *
 *                                    <--- TXOP 1 -->
 *    Coex event                      ┌──────┬──────┐
 *   notification                     │ MPDU │ MPDU │
 *        │                           │  1   │  2   │
 *  ──────▼────▲───────▲───────────▲──┴──────┴──────┴┬──┬─────────────
 *             │       │           │                 │BA│
 *         2 packets  Coex event  Coex event         └──┘
 *         enqueued   start       end
 *
 *
 * DEFER_ONE_PACKET scenario:
 * A coex event is notified before packets are enqueued. The start of the coex event is such that
 * there is enough time to transmit a single packet only. The STA transmits the first packet before
 * the coex event and the second packet after the coex event ends.
 *
 *                <- TXOP 1 ->                   <- TXOP 2 ->
 *    Coex event  ┌──────┐                       ┌──────┐
 *   notification │ MPDU │                       │ MPDU │
 *        │       │  1   │                       │  2   │
 *  ──────▼────▲──┴──────┴┬───┬─▲─────────────▲──┴──────┴┬───┬─────────────
 *             │          │ACK│ │             │          │ACK│
 *          2 packets     └───┘ Coex event   Coex event  └───┘
 *          enqueued            start        end
 *
 *
 * TXOP_BEFORE_COEX scenario:
 * A coex event is notified before packets are enqueued. The start of the coex event is such that
 * there is enough time to transmit both packets.
 *
 *                    <--- TXOP 1 -->
 *    Coex event      ┌──────┬──────┐
 *   notification     │ MPDU │ MPDU │
 *        │           │  1   │  2   │
 *  ──────▼────▲──────┴──────┴──────┴┬──┬────▲───────────▲────────────
 *             │                     │BA│    │           │
 *         2 packets                 └──┘   Coex event  Coex event
 *         enqueued                         start       end
 *
 *
 * INTRA_TXOP_COEX scenario:
 * A packet is enqueued and is promptly transmitted. During the transmission of the first packet,
 * a second packet is enqueued. Shortly (half a SIFS) after the first frame exchange sequence is
 * completed, a coex event starting a PIFS later is notified. The first TXOP is terminated and the
 * second packet is transmitted once the coex event ends. This scenario demonstrates that, in case
 * of non-zero TXOP limit, a second frame exchange sequence in the same TXOP is not started if an
 * overlapping coex event is notified during the first frame exchange sequence.
 *
 *            <- TXOP 1 ->                    <- TXOP 2 ->
 *   1 packet ┌──────┐  Coex event            ┌──────┐
 *   enqueued │ MPDU │  notification          │ MPDU │
 *        │   │  1   │      │                 │  2   │
 *  ──────▼───┴▲─────┴┬───┬─▼──▲───────────▲──┴──────┴┬───┬─────────────
 *             │      │ACK│    │           │          │ACK│
 *          1 packet  └───┘ Coex event   Coex event   └───┘
 *          enqueued         start        end
 */
class WifiCoexUlTxopTest : public WifiCoexTestBase
{
  public:
    /**
     * Enumeration of the scenarios covered by this unit test.
     */
    enum Scenario : uint8_t
    {
        DEFER_TXOP_START = 0,
        DEFER_ONE_PACKET,
        TXOP_BEFORE_COEX,
        INTRA_TXOP_COEX
    };

    /**
     * Constructor
     *
     * @param scenario the scenario to test
     * @param txopLimit the TXOP limit
     */
    WifiCoexUlTxopTest(Scenario scenario, const Time& txopLimit);

  protected:
    void DoSetup() override;
    void DoRun() override;
    void CoexEventCallback(const coex::Event& coexEvent) override;

    /// Insert elements in the list of expected events (transmitted frames)
    void InsertEvents();

  private:
    /// Insert events for the DEFER_TXOP_START scenario
    void InsertDeferTxopStartEvents();

    /// Insert events for the DEFER_ONE_PACKET scenario
    void InsertDeferOnePacketEvents();

    /// Insert events for the TXOP_BEFORE_COEX scenario
    void InsertTxopBeforeCoexEvents();

    /// Insert events for the INTRA_COEX_TXOP scenario
    void InsertIntraCoexTxopEvents();

    const Scenario m_scenario;  ///< the scenario to test
    Time m_txopLimit;           ///< the TXOP limit
    Time m_pktGenTime;          ///< generation time for the first UL packet(s)
    Time m_mockInterEventTime;  ///< time interval between two consecutive coex events generated by
                                ///< the mock generator
    Time m_mockEventStartDelay; ///< delay between the time the coex event is notified and the time
                                ///< the coex event starts
    Time m_mockEventDuration;   ///< duration of a coex event
    const std::size_t m_pktSize{1000}; ///< application packet size
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi coex basic Test Suite
 */
class WifiCoexBasicTestSuite : public TestSuite
{
  public:
    WifiCoexBasicTestSuite();
};

#endif /* WIFI_COEX_BASIC_TEST_H */
