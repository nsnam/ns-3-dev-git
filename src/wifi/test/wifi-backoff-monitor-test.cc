/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/ap-wifi-mac.h"
#include "ns3/boolean.h"
#include "ns3/frame-exchange-manager.h"
#include "ns3/interference-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/object-factory.h"
#include "ns3/packet-socket-address.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/pointer.h"
#include "ns3/qos-txop.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/test.h"
#include "ns3/wifi-assoc-manager.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-static-setup-helper.h"

#include <array>
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiBackoffMonTest");

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test backoff interruptions provided by the backoff monitor
 *
 * This test comprises an AP and an associated non-AP STA. The backoff monitor is enabled on the
 * non-AP STA. A callback is connected to the BackoffStatus trace of the AC BE EDCAF of the non-AP
 * STA. It is checked that:
 * - the previous status provided in a trace notification matches the current status provided by the
 *   previous trace notification
 * - the backoff status is changed to ONGOING post initialization
 * - the backoff status is changed to PAUSED when a DL packet is transmitted
 * - the backoff status is changed to ONGOING a SIFS + slot after the end of the corresponding Ack
 * - when an UL packet is transmitted, the backoff status is changed to ZERO
 * - after the termination of the UL TXOP, a backoff value is generated and the backoff status is
 *   changed to ONGOING
 * - even if channel access is not requested because no other packet is queued at the STA, the
 *   backoff status is changed to ZERO when the backoff counter reaches zero.
 */
class WifiBackoffMonitorTest : public TestCase
{
  public:
    WifiBackoffMonitorTest();

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
     * Callback connected to the BackoffStatus trace of the given AC. Store the information provided
     * by the trace source into a member variable.
     *
     * @param aci the index of the given AC
     * @param info the information provided by the trace source
     */
    void BackoffStatusChangeCallback(AcIndex aci, const BackoffMonitor::StatusTrace& info);

    /**
     * Set the backoff counter of the given AC of the given MAC to the given value.
     *
     * @param mac the given MAC
     * @param aci the index of the given AC
     * @param value the given value
     */
    void SetBackoff(Ptr<WifiMac> mac, AcIndex aci, uint32_t value);

    /**
     * Check the current backoff status.
     *
     * @param context a string indicating the context
     * @param status the expected backoff status
     * @param value the expected backoff counter value
     */
    void CheckBackoffStatus(const std::string& context, BackoffStatus status, uint32_t value);

  private:
    void DoSetup() override;
    void DoRun() override;

    /**
     * @param sockAddr the packet socket address identifying local outgoing interface
     *                 and remote address
     * @param count the number of packets to generate
     * @param pktSize the size of the packets to generate
     * @param delay the delay with which traffic generation starts
     * @param priority user priority for generated packets
     * @return an application generating the given number packets of the given size destined
     *         to the given packet socket address
     */
    Ptr<PacketSocketClient> GetApplication(const PacketSocketAddress& sockAddr,
                                           std::size_t count,
                                           std::size_t pktSize,
                                           Time delay = Time{0},
                                           uint8_t priority = 0) const;

    /// Insert elements in the list of expected events (transmitted frames)
    void InsertEvents();

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
               std::function<void(Ptr<const WifiPsdu>, const WifiTxVector&, linkId_t)>&& f = {})
            : hdrType(type),
              func(f)
        {
        }

        WifiMacType hdrType; ///< MAC header type of frame being transmitted
        std::function<void(Ptr<const WifiPsdu>, const WifiTxVector&, linkId_t)>
            func; ///< function to perform actions and checks
    };

    std::list<Events> m_events; //!< list of events for a test run

    Ptr<ApWifiMac> m_apMac;                  ///< AP wifi MAC
    Ptr<StaWifiMac> m_staMac;                ///< STA wifi MAC
    Time m_duration{Seconds(1)};             ///< simulation duration
    BackoffMonitor::StatusTrace m_traceInfo; ///< last traced backoff status update
    const uint32_t m_initStaBackoff{12};     ///< initial backoff value for STA
    const uint32_t m_initApBackoff{2};       ///< initial backoff value for AP
};

WifiBackoffMonitorTest::WifiBackoffMonitorTest()
    : TestCase("Test the backoff interruptions notified by the backoff monitor")
{
}

Ptr<PacketSocketClient>
WifiBackoffMonitorTest::GetApplication(const PacketSocketAddress& sockAddr,
                                       std::size_t count,
                                       std::size_t pktSize,
                                       Time delay,
                                       uint8_t priority) const
{
    auto client = CreateObject<PacketSocketClient>();
    client->SetAttribute("PacketSize", UintegerValue(pktSize));
    client->SetAttribute("MaxPackets", UintegerValue(count));
    client->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
    client->SetAttribute("Priority", UintegerValue(priority));
    client->SetRemote(sockAddr);
    client->SetStartTime(delay);
    client->SetStopTime(m_duration - Simulator::Now());

    return client;
}

void
WifiBackoffMonitorTest::DoSetup()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 30;

    NodeContainer nodes(2); // AP, STA
    NetDeviceContainer devices;

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211be);

    auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    spectrumChannel->AddPropagationLossModel(CreateObject<FriisPropagationLossModel>());

    SpectrumWifiPhyHelper phy;
    phy.Set("ChannelSettings", StringValue("{0,80,BAND_5GHZ,0}"));
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.SetChannel(spectrumChannel);

    WifiMacHelper mac;
    mac.SetType("ns3::ApWifiMac", "BeaconGeneration", BooleanValue(false));

    devices.Add(wifi.Install(phy, mac, nodes.Get(0)));

    mac.SetType("ns3::StaWifiMac", "EnableBackoffMon", BooleanValue(true));

    devices.Add(wifi.Install(phy, mac, nodes.Get(1)));

    // Assign fixed streams to random variables in use
    streamNumber += WifiHelper::AssignStreams(devices, streamNumber);

    MobilityHelper mobility;
    auto positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));   // AP
    positionAlloc->Add(Vector(10.0, 10.0, 0.0)); // non-AP STA
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    m_apMac = DynamicCast<ApWifiMac>(DynamicCast<WifiNetDevice>(devices.Get(0))->GetMac());
    m_staMac = DynamicCast<StaWifiMac>(DynamicCast<WifiNetDevice>(devices.Get(1))->GetMac());

    const auto apDev = m_apMac->GetDevice();
    const auto staDev = m_staMac->GetDevice();

    // statically configure and Block Ack agreements for TID 0
    WifiStaticSetupHelper::SetStaticAssoc(apDev, staDev);
    WifiStaticSetupHelper::SetStaticBlockAck(apDev, staDev, 0);
    WifiStaticSetupHelper::SetStaticBlockAck(staDev, apDev, 0);

    // Trace PSDUs passed to the PHY on all devices
    for (uint32_t i = 0; i < devices.GetN(); ++i)
    {
        auto dev = DynamicCast<WifiNetDevice>(devices.Get(i));
        for (uint8_t phyId = 0; phyId < dev->GetNPhys(); ++phyId)
        {
            dev->GetPhy(phyId)->TraceConnectWithoutContext(
                "PhyTxPsduBegin",
                MakeCallback(&WifiBackoffMonitorTest::Transmit, this).Bind(dev->GetMac(), phyId));
        }
    }

    // Trace backoff interruptions for AC BE of the non-AP STA
    m_staMac->GetQosTxop(AC_BE)->TraceConnectWithoutContext(
        "BackoffStatus",
        MakeCallback(&WifiBackoffMonitorTest::BackoffStatusChangeCallback, this).Bind(AC_BE));

    // install packet socket on all nodes
    PacketSocketHelper packetSocket;
    packetSocket.Install(nodes);

    // install a packet socket server on all nodes
    for (uint32_t i = 0; i < devices.GetN(); ++i)
    {
        PacketSocketAddress srvAddr;
        auto device = DynamicCast<WifiNetDevice>(devices.Get(i));
        NS_TEST_ASSERT_MSG_NE(device, nullptr, "Expected a WifiNetDevice");
        srvAddr.SetSingleDevice(device->GetIfIndex());
        srvAddr.SetProtocol(1);

        auto server = CreateObject<PacketSocketServer>();
        server->SetLocal(srvAddr);
        device->GetNode()->AddApplication(server);
        server->SetStartTime(Seconds(0)); // now
        server->SetStopTime(m_duration);
    }

    // configure initial backoff values
    Simulator::ScheduleNow([=, this] {
        SetBackoff(m_staMac, AC_BE, m_initStaBackoff);
        SetBackoff(m_apMac, AC_BE, m_initApBackoff);
    });

    InsertEvents();
}

void
WifiBackoffMonitorTest::Transmit(Ptr<WifiMac> mac,
                                 uint8_t phyId,
                                 WifiConstPsduMap psduMap,
                                 WifiTxVector txVector,
                                 double txPowerW)
{
    auto linkId = mac->GetLinkForPhy(phyId);
    NS_TEST_ASSERT_MSG_EQ(linkId.has_value(), true, "No link found for PHY ID " << +phyId);

    for (const auto& [aid, psdu] : psduMap)
    {
        std::stringstream ss;
        ss << std::setprecision(10) << " Link ID " << +linkId.value() << " Phy ID " << +phyId
           << " #MPDUs " << psdu->GetNMpdus();
        for (auto it = psdu->begin(); it != psdu->end(); ++it)
        {
            ss << "\n" << **it;
        }
        NS_LOG_INFO(ss.str());
    }
    NS_LOG_INFO("TXVECTOR = " << txVector << "\n");

    const auto psdu = psduMap.cbegin()->second;
    const auto& hdr = psdu->GetHeader(0);

    if (!m_events.empty())
    {
        // check that the expected frame is being transmitted
        NS_TEST_EXPECT_MSG_EQ(WifiMacHeader(m_events.front().hdrType).GetTypeString(),
                              hdr.GetTypeString(),
                              "Unexpected MAC header type for frame transmitted at time "
                                  << Simulator::Now().As(Time::US));
        // perform actions/checks, if any
        if (m_events.front().func)
        {
            m_events.front().func(psdu, txVector, linkId.value());
        }

        m_events.pop_front();
    }
}

void
WifiBackoffMonitorTest::SetBackoff(Ptr<WifiMac> mac, AcIndex aci, uint32_t value)
{
    mac->GetTxopFor(aci)->StartBackoffNow(value, SINGLE_LINK_OP_ID);
    mac->GetChannelAccessManager(SINGLE_LINK_OP_ID)->DoRestartAccessTimeoutIfNeeded();
}

void
WifiBackoffMonitorTest::BackoffStatusChangeCallback(AcIndex aci,
                                                    const BackoffMonitor::StatusTrace& info)
{
    NS_LOG_DEBUG(aci << ": from " << info.prevStatus << " to " << info.currStatus << " on link "
                     << +info.linkId << ", counter: " << info.counter << "\n");

    NS_TEST_EXPECT_MSG_EQ(info.prevStatus,
                          m_traceInfo.currStatus,
                          "BackoffStatus trace provided unexpected previous status at time "
                              << Simulator::Now());

    m_traceInfo = info;
}

void
WifiBackoffMonitorTest::CheckBackoffStatus(const std::string& context,
                                           BackoffStatus status,
                                           uint32_t value)
{
    NS_TEST_EXPECT_MSG_EQ(m_traceInfo.currStatus, status, "Unexpected backoff status " << context);
    NS_TEST_EXPECT_MSG_EQ(m_traceInfo.counter, value, "Unexpected backoff counter " << context);
}

void
WifiBackoffMonitorTest::InsertEvents()
{
    // check m_traceInfo a nanosecond after initialization to avoid issues with the scheduling
    // of events occurring at the same time
    Simulator::Schedule(NanoSeconds(1), [this] {
        CheckBackoffStatus("post initialization", BackoffStatus::ONGOING, m_initStaBackoff);
    });

    PacketSocketAddress staAddr;
    staAddr.SetSingleDevice(m_apMac->GetDevice()->GetIfIndex());
    staAddr.SetPhysicalAddress(m_staMac->GetDevice()->GetAddress());
    staAddr.SetProtocol(1);

    PacketSocketAddress apAddr;
    apAddr.SetSingleDevice(m_staMac->GetDevice()->GetIfIndex());
    apAddr.SetPhysicalAddress(m_apMac->GetDevice()->GetAddress());
    apAddr.SetProtocol(1);

    // AP generates a DL packet for the STA
    m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(staAddr, 1, 1000));

    // the first backoff interruption occurs when the AP transmits the DL packet
    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).GetAddr2(),
                                  m_apMac->GetAddress(),
                                  "First QoS data frame not sent by the AP");

            // first interruption starts after the preamble of the DL packet is detected, thus
            // by the end of PHY header it has been certainly notified
            auto phyHdrDuration = WifiPhy::CalculatePhyPreambleAndHeaderDuration(txVector);
            Simulator::Schedule(phyHdrDuration, [this] {
                // The backoff counter at STA is decremented after the AIFS and then as many times
                // as the number of slots awaited by the AP before transmitting
                CheckBackoffStatus("after PHY header of first QoS data frame",
                                   BackoffStatus::PAUSED,
                                   m_initStaBackoff - m_initApBackoff - 1);
            });
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            // STA generates an UL packet for the AP
            m_staMac->GetDevice()->GetNode()->AddApplication(GetApplication(apAddr, 1, 1000));

            const auto txDuration =
                WifiPhy::CalculateTxDuration(psdu,
                                             txVector,
                                             m_apMac->GetWifiPhy(linkId)->GetPhyBand());
            Simulator::Schedule(txDuration, [=, this] {
                // at the end of the Ack, the backoff status is still paused
                CheckBackoffStatus("after the Ack for the first QoS data frame",
                                   BackoffStatus::PAUSED,
                                   m_initStaBackoff - m_initApBackoff - 1);
                // after a SIFS + slot, the backoff is resumed
                auto apPhy = m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID);
                Simulator::Schedule(apPhy->GetSifs() + apPhy->GetSlot(), [this] {
                    CheckBackoffStatus("a SIFS + slot after the Ack for the first QoS data frame",
                                       BackoffStatus::ONGOING,
                                       m_initStaBackoff - m_initApBackoff - 1);
                });
            });
        });

    // the backoff status is ZERO when the STA transmits the UL packet
    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).GetAddr2(),
                                  m_staMac->GetAddress(),
                                  "Second QoS data frame not sent by the STA");

            Simulator::Schedule(TimeStep(1), [this] {
                CheckBackoffStatus("when starting the transmission of the UL frame",
                                   BackoffStatus::ZERO,
                                   0);
            });
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, linkId_t linkId) {
            const auto txDuration =
                WifiPhy::CalculateTxDuration(psdu,
                                             txVector,
                                             m_apMac->GetWifiPhy(linkId)->GetPhyBand());
            Simulator::Schedule(txDuration + TimeStep(1), [=, this] {
                auto beTxop = m_staMac->GetQosTxop(AC_BE);
                // at the end of the Ack, the backoff status is ongoing again
                CheckBackoffStatus("after the Ack for the UL QoS data frame",
                                   BackoffStatus::ONGOING,
                                   beTxop->GetBackoffSlots(linkId));
                auto backoffEnd =
                    m_staMac->GetChannelAccessManager(linkId)->GetBackoffEndFor(beTxop);
                Simulator::Schedule(backoffEnd - Simulator::Now() + TimeStep(1), [=, this] {
                    CheckBackoffStatus("when the backoff counter is expected to reach zero",
                                       BackoffStatus::ZERO,
                                       0);
                });
            });
        });
}

void
WifiBackoffMonitorTest::DoRun()
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
 * @brief wifi scanning Test Suite
 */
class WifiBackoffMonTestSuite : public TestSuite
{
  public:
    WifiBackoffMonTestSuite();
};

WifiBackoffMonTestSuite::WifiBackoffMonTestSuite()
    : TestSuite("wifi-backoff-mon-test", Type::UNIT)
{
    AddTestCase(new WifiBackoffMonitorTest(), TestCase::Duration::QUICK);
}

static WifiBackoffMonTestSuite g_wifiBackoffMonTestSuite; ///< the test suite
