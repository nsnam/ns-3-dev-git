/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Davide Magrin <davide@magr.in>
 */

#include "wifi-emlsr-test-base.h"

#include "ns3/ap-wifi-mac.h"
#include "ns3/boolean.h"
#include "ns3/emlsr-manager.h"
#include "ns3/node.h"
#include "ns3/packet-socket-client.h"
#include "ns3/qos-txop.h"
#include "ns3/simulator.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/wifi-mac-queue-scheduler.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-remote-station-manager.h"
#include "ns3/wifi-utils.h"

#include <algorithm>
#include <list>
#include <optional>

using namespace ns3;

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test that a new EMLSR TXOP finalizes the tracking of the previous TXOP.
 *
 * With a non-zero TXOP limit, the AP MLD keeps a pending TXOP end event for a short
 * window after each frame exchange of an EMLSR client's UL TXOP, waiting to see whether
 * the TXOP holder continues its TXOP. If a TXOP-opening frame from a different EMLSR
 * client is received in that window, the tracked TXOP is over: the pending event must
 * be finalized (the previous holder switches back to listening operation, so the
 * blocking of its other links ends) and the start-of-TXOP processing must run for the
 * new holder (its other links become blocked).
 *
 * The AP MLD and the three non-AP MLDs are set up by the EMLSR test base class, with
 * three links each (2.4 GHz, 5 GHz and 6 GHz). Two non-AP MLDs are EMLSR clients, with
 * EMLSR mode enabled on link 1 (5 GHz), where their main PHY operates, and on link 2
 * (6 GHz), where their aux PHY operates; the third one does not enable EMLSR and acts
 * as a background station that only transmits on link 2. Once the traffic of the test
 * starts, no frame is transmitted on link 0. All the packets are generated at times such
 * that:
 *
 * - client 1 runs a two-exchange TXOP on link 1, whose TXOP limit is sized so that the
 *   closing Ack still announces a few tens of microseconds of unused protection: not
 *   enough for another frame exchange (or a CF-End frame), but enough to keep the TXOP
 *   end event pending after the TXOP has actually terminated;
 * - client 2, whose main PHY leaves link 1 during client 1's first frame exchange (an
 *   aux PHY gained a TXOP on link 2, where the background station's first packet kept
 *   the medium busy at the time client 1's TXOP started), transmits its packet on
 *   link 2 and switches the main PHY back to link 1, which was left unattended in the
 *   meantime: the main PHY misses the protection announced by client 1 on link 1;
 * - the main PHY completes the switch around the end of client 1's TXOP and gains
 *   channel access a short AIFS later, inside the TXOP end tracking window: the RTS it
 *   transmits (the MediumSyncDelay timer is running after the switch) reaches the AP
 *   MLD while the TXOP end event for client 1's terminated TXOP is still pending;
 * - the background station's second packet keeps link 2 busy in the meantime, so that
 *   client 2's second packet is transmitted by the main PHY on link 1.
 *
 * The expected frame exchange sequence is depicted below (time flows left to right,
 * consistently across links 1 and 2; frames transmitted by the AP MLD are above the
 * time axes, frames transmitted by the stations below):
 *
 * @verbatim
 *                                                           ┌───┐
 *                                                           │Ack│
 *  [link 1]  ─────────────┬────────┬────────────────────────┴───┴────────────┬────────┬───────
 *                         │QoS Data│                                         │QoS Data│
 *                         │client 1│                                         │client 1│
 *                         └────────┘                                         └────────┘
 *
 *                                    ┌───┐            ┌───┐                             ┌───┐
 *                                    │Ack│            │CTS│                             │Ack│
 *  [link 2]  ──┬────────┬────────────┴───┴─┬────────┬─┴───┴───────┬────────┬────────────┴───┴─
 *              │QoS Data│                  │  RTS   │             │QoS Data│
 *              │ bg STA │                  │client 2│             │client 2│
 *              └────────┘                  └────────┘             └────────┘
 *
 *  (client 2's main PHY completes the switch back to link 1, having missed the protection
 *  announced by client 1; the MediumSyncDelay timer is running:)
 *
 *    client 1's TXOP is over, but the      the AP MLD must have blocked client 2's
 *    announced protection keeps the        link 2 and released client 1's links
 *    TXOP end event pending                (both checked here)
 *                           │                │
 *                         ┌───┐            ┌───┐                  ┌───┐
 *                         │Ack│            │CTS│                  │Ack│
 *  [link 1]  ─────────────┴───┴─┬────────┬─┴───┴─┬────────┬───────┴───┴─
 *                               │  RTS   │       │QoS Data│
 *                               │client 2│       │client 2│
 *                               └────────┘       └────────┘
 *
 *                                                           ┌───┐
 *                                                           │Ack│
 *  [link 2]  ──┬────────┬───────────────────────────────────┴───┴───────
 *              │QoS Data│
 *              │ bg STA │
 *              └────────┘
 * @endverbatim
 *
 * Association, Block Ack agreement establishment and EMLSR mode enabling are performed
 * by the base class before the traffic of the test starts. The test checks the type and
 * the transmitter (and, where relevant, the transmitting PHY) of every frame transmitted
 * on each link against the sequence of that link (frames
 * transmitted on distinct links at about the same time, such as the QoS Data frames of
 * client 1 on link 1 and of client 2 on link 2, may occur in either order) and, upon
 * transmission of the CTS in response to client 2's RTS on link 1, that the AP MLD has
 * finalized the tracking of client 1's TXOP (transmissions to client 1 are no longer
 * blocked as using another link, but are waiting for the transition delay) and has run
 * the start-of-TXOP processing for client 2 (transmissions to client 2 on link 2 are
 * blocked). If the AP MLD mistook the pending TXOP end event for an indication that
 * client 1's TXOP is still ongoing, it would skip the start-of-TXOP processing for
 * client 2 and this test would fail at that CTS, on the assert checking that
 * transmissions to the EMLSR client the CTS is addressed to are blocked on the other
 * links. With asserts disabled, the AP MLD blocks those transmissions anyway when
 * sending the CTS, but it would not switch client 1 back to listening operation, and
 * this test would fail on the checks that client 1 has been released.
 */
class EmlsrStaleTxopEndTest : public EmlsrOperationsTestBase
{
  public:
    EmlsrStaleTxopEndTest();

  protected:
    void DoSetup() override;
    void DoRun() override;
    void Transmit(Ptr<WifiMac> mac,
                  uint8_t phyId,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW) override;

  private:
    void StartTraffic() override;

    /// Actions and checks to perform upon the transmission of each frame
    struct Events
    {
        /**
         * Constructor.
         *
         * @param type the frame MAC header type
         * @param id the ID of the link on which the frame is expected
         * @param tx the MAC of the device expected to transmit the frame
         * @param phy the ID of the PHY expected to transmit the frame, if it is to be checked
         * @param f function to perform actions and checks
         */
        Events(WifiMacType type,
               linkId_t id,
               Ptr<WifiMac> tx,
               std::optional<uint8_t> phy = std::nullopt,
               std::function<void(const WifiConstPsduMap&, const WifiTxVector&)>&& f = {})
            : hdrType(type),
              linkId(id),
              txMac(tx),
              phyId(phy),
              func(f)
        {
        }

        WifiMacType hdrType;          ///< MAC header type of the frame
        linkId_t linkId;              ///< ID of the link on which the frame is expected
        Ptr<WifiMac> txMac;           ///< MAC of the device expected to transmit the frame
        std::optional<uint8_t> phyId; ///< ID of the PHY expected to transmit the frame, if any
        std::function<void(const WifiConstPsduMap&, const WifiTxVector&)>
            func; ///< actions and checks to perform
    };

    /// Insert elements in the list of expected events (transmitted frames)
    void InsertEvents();

    /// Check the AP MLD's blocked queues after the takeover of the ended TXOP:
    /// client 2's other link must be blocked, client 1's links must be released
    void CheckQueuesAfterTakeover();

    /// EMLSR link on which the main PHYs operate (5 GHz)
    static constexpr linkId_t MAIN_LINK = 1;
    /// EMLSR link on which the aux PHYs operate (6 GHz)
    static constexpr linkId_t AUX_LINK = 2;
    /// ID of the main PHY of the EMLSR clients (initially operating on MAIN_LINK)
    static constexpr uint8_t MAIN_PHY_ID = 1;
    /// ID of the aux PHY of the EMLSR clients operating on AUX_LINK
    static constexpr uint8_t AUX_PHY_ID = 2;
    /// TID of the packets transmitted in the TXOPs under test and of the background
    /// station's second packet
    static constexpr tid_t TXOP_TID = 5;
    /// AC of TXOP_TID, whose TXOP limit is set
    static constexpr AcIndex TXOP_AC = AC_VI;
    /// TID of the background station's first packet
    static constexpr tid_t BG_STA_TID = 0;

    std::list<Events> m_events;   ///< list of events for the test run
    bool m_trafficStarted{false}; ///< whether the traffic of the test run has started
    /// TXOP limit for TXOP_AC: sized so that client 1's TXOP comprises two frame
    /// exchanges and its closing Ack announces unused protection that is shorter than
    /// a CF-End frame but longer than the TXOP end tracking window
    const Time m_txopLimit{MicroSeconds(992)};
};

EmlsrStaleTxopEndTest::EmlsrStaleTxopEndTest()
    : EmlsrOperationsTestBase("Check that a new EMLSR TXOP finalizes the tracking of the previous "
                              "TXOP")
{
    m_nEmlsrStations = 2;
    m_nNonEmlsrStations = 1;
    m_linksToEnableEmlsrOn = {MAIN_LINK, AUX_LINK};
    m_mainPhyId = MAIN_PHY_ID;
    // the timing of the frame exchange sequence is calibrated on the durations of the frames
    // transmitted on 20 MHz channels at the rates set in DoSetup
    m_channelsStr = {"{2, 20, BAND_2_4GHZ, 0}"s,
                     "{36, 20, BAND_5GHZ, 0}"s,
                     "{1, 20, BAND_6GHZ, 0}"s};
    m_paddingDelay = {MicroSeconds(0), MicroSeconds(0)};
    m_transitionDelay = {MicroSeconds(64), MicroSeconds(64)};
    m_establishBaUl = {BG_STA_TID, TXOP_TID};
    m_duration = Seconds(1);
}

void
EmlsrStaleTxopEndTest::DoSetup()
{
    EmlsrOperationsTestBase::DoSetup();

    // rates and channel switch delay the timing of the frame exchange sequence is calibrated on
    std::vector<Ptr<WifiMac>> macs{m_apMac};
    macs.insert(macs.end(), m_staMacs.cbegin(), m_staMacs.cend());
    for (const auto& mac : macs)
    {
        for (auto linkId : {MAIN_LINK, AUX_LINK})
        {
            mac->GetWifiRemoteStationManager(linkId)->SetAttribute("DataMode",
                                                                   StringValue("HeMcs3"));
            mac->GetWifiRemoteStationManager(linkId)->SetAttribute("ControlMode",
                                                                   StringValue("OfdmRate6Mbps"));
        }
        for (uint8_t phyId = 0; phyId < mac->GetDevice()->GetNPhys(); ++phyId)
        {
            mac->GetDevice()->GetPhy(phyId)->SetAttribute("ChannelSwitchDelay",
                                                          TimeValue(MicroSeconds(64)));
        }
    }

    for (std::size_t i = 0; i < m_nEmlsrStations; ++i)
    {
        // the main PHY switches to the link on which an aux PHY gained a TXOP and
        // switches back to its link when the TXOP ends, leaving that link unattended in
        // the meantime
        m_staMacs.at(i)->GetEmlsrManager()->SetAttribute("SwitchAuxPhy", BooleanValue(false));
    }

    // a non-zero TXOP limit on the main PHYs' link, advertised to the clients upon association
    m_apMac->GetQosTxop(TXOP_AC)->SetTxopLimit(m_txopLimit, MAIN_LINK);
}

void
EmlsrStaleTxopEndTest::StartTraffic()
{
    m_trafficStarted = true;
    InsertEvents();

    // deterministic channel access (no random backoff) and a zero TXOP limit for
    // client 2 and the background station, whose exchanges are then self-contained
    // (no CF-End truncation): the frame sequence stays minimal and the protection
    // they announce expires with each exchange
    for (const auto& staMac : m_staMacs)
    {
        const std::vector<uint32_t> zeroCws(staMac->GetNLinks(), 0);
        for (const auto& [aci, ac] : wifiAcList)
        {
            staMac->GetQosTxop(aci)->SetMinCws(zeroCws);
            staMac->GetQosTxop(aci)->SetMaxCws(zeroCws);
        }
    }
    for (std::size_t i : {1, 2})
    {
        m_staMacs.at(i)->GetQosTxop(TXOP_AC)->SetTxopLimits(
            std::vector<Time>(m_staMacs.at(i)->GetNLinks(), Time{0}));
    }

    // the EMLSR clients transmit on the EMLSR links only and the background station on the
    // aux PHYs' link only
    const auto apMld = m_apMac->GetAddress();
    m_staMacs.at(0)->BlockUnicastTxOnLinks(WifiQueueBlockedReason::TID_NOT_MAPPED, apMld, {0});
    m_staMacs.at(1)->BlockUnicastTxOnLinks(WifiQueueBlockedReason::TID_NOT_MAPPED, apMld, {0});
    m_staMacs.at(2)->BlockUnicastTxOnLinks(WifiQueueBlockedReason::TID_NOT_MAPPED,
                                           apMld,
                                           {0, MAIN_LINK});

    // the background station's first packet keeps the aux PHYs' link busy when client 1's
    // first packet arrives
    Simulator::Schedule(MicroSeconds(850), [this]() {
        m_staMacs.at(2)->GetDevice()->GetNode()->AddApplication(
            GetApplication(UPLINK, 2, 1, 1250, BG_STA_TID));
    });
    // client 1's first packet: starts its two-exchange TXOP on the main PHYs' link (the
    // remaining packets are generated upon transmission of the frames in the events list)
    Simulator::Schedule(MicroSeconds(1000), [this]() {
        m_staMacs.at(0)->GetDevice()->GetNode()->AddApplication(
            GetApplication(UPLINK, 0, 1, 1300, TXOP_TID));
    });
}

void
EmlsrStaleTxopEndTest::CheckQueuesAfterTakeover()
{
    const auto client1Mld = m_staMacs.at(0)->GetAddress();
    const auto client2Mld = m_staMacs.at(1)->GetAddress();

    // start-of-TXOP processing for the new holder: its other link is blocked
    CheckBlockedLink(m_apMac,
                     client2Mld,
                     AUX_LINK,
                     WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                     true,
                     "Client 2 holds a TXOP on link 1");
    CheckBlockedLink(m_apMac,
                     client2Mld,
                     MAIN_LINK,
                     WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                     false,
                     "Client 2 holds a TXOP on link 1");

    // the previous holder has been switched back to listening operation: the blocking
    // of its links as using another link has ended and the transition delay has started
    for (auto linkId : {MAIN_LINK, AUX_LINK})
    {
        CheckBlockedLink(m_apMac,
                         client1Mld,
                         linkId,
                         WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY,
                         true,
                         "Client 1's TXOP has ended");
    }

    // after the 64 us transition delay, client 1 is fully unblocked while client 2's
    // TXOP blocking persists
    Simulator::Schedule(MicroSeconds(65), [=, this]() {
        for (auto linkId : {MAIN_LINK, AUX_LINK})
        {
            CheckBlockedLink(m_apMac,
                             client1Mld,
                             linkId,
                             WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY,
                             false,
                             "The transition delay of client 1 has elapsed");
        }
        CheckBlockedLink(m_apMac,
                         client2Mld,
                         AUX_LINK,
                         WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                         true,
                         "Client 2 still holds a TXOP on link 1");
    });
}

void
EmlsrStaleTxopEndTest::InsertEvents()
{
    const auto& ap = m_apMac;
    const auto& client1 = m_staMacs.at(0);
    const auto& client2 = m_staMacs.at(1);
    const auto& bgSta = m_staMacs.at(2);

    // 1. the background station's first packet keeps link 2 busy, so that client 1's
    // TXOP starts on link 1 (and client 2's aux PHY does not gain link 2 immediately)
    m_events.emplace_back(WIFI_MAC_QOSDATA, AUX_LINK, bgSta);

    // 2. first frame exchange of client 1's TXOP on link 1. Client 1's second packet
    // is generated now, so that it is transmitted in a second frame exchange of the
    // same TXOP; client 2's packet is generated while link 1 is busy, so that only its
    // aux PHY on link 2 can gain channel access (once the background station's frame
    // exchange is over)
    m_events.emplace_back(WIFI_MAC_QOSDATA,
                          MAIN_LINK,
                          client1,
                          std::nullopt,
                          [this](const WifiConstPsduMap& psduMap, const WifiTxVector& txVector) {
                              m_staMacs.at(0)->GetDevice()->GetNode()->AddApplication(
                                  GetApplication(UPLINK, 0, 1, 1300, TXOP_TID));
                              Simulator::Schedule(MicroSeconds(50), [this]() {
                                  m_staMacs.at(1)->GetDevice()->GetNode()->AddApplication(
                                      GetApplication(UPLINK, 1, 1, 950, TXOP_TID));
                              });
                          });
    // 3. Ack to the background station on link 2
    m_events.emplace_back(WIFI_MAC_CTL_ACK, AUX_LINK, ap);

    // 4. RTS transmitted on link 2 by client 2's aux PHY; the main PHY leaves link 1
    // before the end of client 1's first QoS Data frame, hence it misses the NAV set
    // by client 1's frames
    m_events.emplace_back(WIFI_MAC_CTL_RTS, AUX_LINK, client2, AUX_PHY_ID);
    // 5. CTS to client 2 on link 2
    m_events.emplace_back(WIFI_MAC_CTL_CTS, AUX_LINK, ap);
    // 6. Ack completing client 1's first frame exchange on link 1
    m_events.emplace_back(WIFI_MAC_CTL_ACK, MAIN_LINK, ap);

    // 7. client 2's QoS Data frame on link 2, transmitted by the main PHY. The
    // background station's second packet is generated now, so that it is transmitted
    // on link 2 as soon as the protection announced by client 2 expires
    m_events.emplace_back(WIFI_MAC_QOSDATA,
                          AUX_LINK,
                          client2,
                          MAIN_PHY_ID,
                          [this](const WifiConstPsduMap& psduMap, const WifiTxVector& txVector) {
                              m_staMacs.at(2)->GetDevice()->GetNode()->AddApplication(
                                  GetApplication(UPLINK, 2, 1, 1000, TXOP_TID));
                          });
    // 8. second frame exchange of client 1's TXOP on link 1
    m_events.emplace_back(WIFI_MAC_QOSDATA, MAIN_LINK, client1);
    // 9. Ack ending client 2's TXOP on link 2: its main PHY starts switching back to
    // link 1
    m_events.emplace_back(WIFI_MAC_CTL_ACK, AUX_LINK, ap);

    // 10. the background station's second packet occupies link 2. Client 2's second
    // packet is generated once this frame is on the air, so that it can only be
    // transmitted on link 1 by the main PHY
    m_events.emplace_back(WIFI_MAC_QOSDATA,
                          AUX_LINK,
                          bgSta,
                          std::nullopt,
                          [this](const WifiConstPsduMap& psduMap, const WifiTxVector& txVector) {
                              Simulator::Schedule(MicroSeconds(10), [this]() {
                                  m_staMacs.at(1)->GetDevice()->GetNode()->AddApplication(
                                      GetApplication(UPLINK, 1, 1, 400, TXOP_TID));
                              });
                          });

    // 11. Ack closing client 1's TXOP on link 1: the announced protection is not
    // enough for another frame exchange or a CF-End frame, but it keeps the TXOP end
    // event pending for the duration of the tracking window
    m_events.emplace_back(WIFI_MAC_CTL_ACK,
                          MAIN_LINK,
                          ap,
                          std::nullopt,
                          [this](const WifiConstPsduMap& psduMap, const WifiTxVector& txVector) {
                              // client 1 is still accounted as being in a TXOP
                              CheckBlockedLink(m_apMac,
                                               m_staMacs.at(0)->GetAddress(),
                                               AUX_LINK,
                                               WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                               true,
                                               "Client 1 closes its TXOP");
                          });

    // 12. RTS transmitted on link 1 by client 2's main PHY (the MediumSyncDelay timer
    // is running after the switch): a TXOP-opening frame from a different EMLSR client
    // received while the TXOP end event of client 1's terminated TXOP is still pending
    m_events.emplace_back(WIFI_MAC_CTL_RTS,
                          MAIN_LINK,
                          client2,
                          MAIN_PHY_ID,
                          [this](const WifiConstPsduMap& psduMap, const WifiTxVector& txVector) {
                              // the TXOP end event has not expired yet (it would have switched
                              // client 1 back to listening operation), otherwise this test would
                              // not exercise the takeover of the ended TXOP
                              CheckBlockedLink(m_apMac,
                                               m_staMacs.at(0)->GetAddress(),
                                               AUX_LINK,
                                               WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                               true,
                                               "Client 2 starts transmitting the RTS while the "
                                               "TXOP end event for client 1's TXOP is pending");
                          });

    // 13. CTS to client 2 on link 1: the AP MLD has processed the start of client 2's
    // TXOP, hence it must have finalized the tracking of client 1's TXOP (had the
    // start-of-TXOP processing been skipped, transmitting this CTS would assert,
    // because transmissions to client 2 on link 2 would not be blocked)
    m_events.emplace_back(WIFI_MAC_CTL_CTS,
                          MAIN_LINK,
                          ap,
                          std::nullopt,
                          [this](const WifiConstPsduMap& psduMap, const WifiTxVector& txVector) {
                              CheckQueuesAfterTakeover();
                          });

    // 14. client 2's QoS Data frame on link 1
    m_events.emplace_back(WIFI_MAC_QOSDATA, MAIN_LINK, client2, MAIN_PHY_ID);
    // 15. Ack to the background station on link 2
    m_events.emplace_back(WIFI_MAC_CTL_ACK, AUX_LINK, ap);
    // 16. Ack to client 2 on link 1
    m_events.emplace_back(WIFI_MAC_CTL_ACK, MAIN_LINK, ap);
}

void
EmlsrStaleTxopEndTest::Transmit(Ptr<WifiMac> mac,
                                uint8_t phyId,
                                WifiConstPsduMap psduMap,
                                WifiTxVector txVector,
                                double txPowerW)
{
    EmlsrOperationsTestBase::Transmit(mac, phyId, psduMap, txVector, txPowerW);

    if (!m_trafficStarted)
    {
        return;
    }

    const auto linkId = mac->GetLinkForPhy(phyId);
    NS_TEST_ASSERT_MSG_EQ(linkId.has_value(), true, "No link found for PHY ID " << +phyId);
    const auto& hdr = psduMap.cbegin()->second->GetHeader(0);

    // the expected frame is the first pending event on the link the frame is transmitted
    // on: frames transmitted on distinct links at about the same time may occur in either
    // order, depending on the slot boundaries on each link
    auto it = std::find_if(m_events.begin(), m_events.end(), [&](const Events& event) {
        return event.linkId == *linkId;
    });
    NS_TEST_ASSERT_MSG_EQ((it != m_events.end()),
                          true,
                          "Unexpected frame of type "
                              << hdr.GetTypeString() << " transmitted on link " << +linkId.value()
                              << " at time " << Simulator::Now().As(Time::US));

    // check that the expected frame is being transmitted
    NS_TEST_ASSERT_MSG_EQ(hdr.GetTypeString(),
                          std::string(WifiMacHeader(it->hdrType).GetTypeString()),
                          "Unexpected MAC header type for frame transmitted on link "
                              << +linkId.value() << " at time " << Simulator::Now().As(Time::US));

    NS_TEST_ASSERT_MSG_EQ(mac,
                          it->txMac,
                          "Unexpected transmitter for frame of type "
                              << hdr.GetTypeString() << " transmitted on link " << +linkId.value()
                              << " at time " << Simulator::Now().As(Time::US));
    if (it->phyId.has_value())
    {
        NS_TEST_ASSERT_MSG_EQ(+phyId,
                              +it->phyId.value(),
                              "Unexpected transmitting PHY for frame of type "
                                  << hdr.GetTypeString() << " transmitted on link "
                                  << +linkId.value() << " at time "
                                  << Simulator::Now().As(Time::US));
    }

    // perform actions and checks, if any
    if (it->func)
    {
        it->func(psduMap, txVector);
    }

    m_events.erase(it);
}

void
EmlsrStaleTxopEndTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(m_events.empty(), true, "Not all events took place");

    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi EMLSR TXOP end tracking Test Suite
 */
class WifiEmlsrTxopEndTrackingTestSuite : public TestSuite
{
  public:
    WifiEmlsrTxopEndTrackingTestSuite();
};

WifiEmlsrTxopEndTrackingTestSuite::WifiEmlsrTxopEndTrackingTestSuite()
    : TestSuite("wifi-emlsr-txop-end-tracking", Type::UNIT)
{
    AddTestCase(new EmlsrStaleTxopEndTest, TestCase::Duration::QUICK);
}

static WifiEmlsrTxopEndTrackingTestSuite g_wifiEmlsrTxopEndTrackingTestSuite; ///< the test suite
