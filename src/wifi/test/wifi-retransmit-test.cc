/*
 * Copyright (c) 2024 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/attribute-container.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/frame-exchange-manager.h"
#include "ns3/he-phy.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/qos-txop.h"
#include "ns3/qos-utils.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-psdu.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiRetransmitTest");

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test retransmit procedure
 *
 * Retransmit procedures are tested for all the combinations of the following options:
 * - RTS/CTS is used or not
 * - Retry count is/is not incremented for MPDUs that are part of a block ack agreement
 * - After a BlockAck timeout, a BlockAckReq or data frames are transmitted
 * - PIFS recovery is used or not
 *
 * Two data frames are generated at a non-AP STA. The first transmission attempt fails, thus retry
 * count (if the IncrementRetryCountUnderBa attribute is set to true) and QSRC are incremented.
 * Two more data frames are generated, thus the second attempt (performed in a second TXOP) includes
 * four data frames. Two of the four data frames (one generated in the first round and one generated
 * in the second round) are corrupted, but the transmission is successful, thus the retry counts are
 * left unchanged and the QSRC is reset.
 * Then, we keep transmitting the two remaining MPDUs until the retry count of the MPDU generated in
 * the first round reaches the retry limit and hence it is discarded (if the
 * IncrementRetryCountUnderBa attribute is set to true). A BlockAckReq is then transmitted to
 * advance the recipient window. Such BlockAckReq is dropped multiple times; every time, the retry
 * count of the remaining MPDU is unchanged and the QSRC increases. When the QSRC exceeds the frame
 * retry limit, the QSRC is reset to 0 and the remaining data frame is not dropped.
 *
 * Note that the above attempts are all performed in the second TXOP because failures occur on
 * non-initial PPDUs, hence PIFS recovery or backoff procedure is invoked. This test verifies that
 * QSRC is unchanged in the former case and incremented in the latter case.
 *
 * In case of multi-link devices, the first TXOP is carried out on link 0 and the second TXOP on
 * link 1. It is checked that QSRC and CW are updated on the link on which the TXOP is carried out.
 */
class WifiRetransmitTest : public TestCase
{
  public:
    /// Parameters for this test
    struct Params
    {
        bool isMld;                 //!< whether devices are MLDs
        bool useRts;                //!< whether RTS is used to protect frame transmissions
        bool incrRetryCountUnderBa; //!< whether retry count is incremented under block ack
        bool useBarAfterBaTimeout;  //!< whether to send a BAR after a missed BlockAck
        bool pifsRecovery; //!< whether PIFS recovery is used after failure of a non-initial
    };

    /**
     * Constructor
     * @param params parameters for the Wi-Fi TXOP test
     */
    WifiRetransmitTest(const Params& params);

    /**
     * Callback invoked when PHY receives a PSDU to transmit
     *
     * @param phyId the ID of the PHY transmitting the PSDUs
     * @param psduMap the PSDU map
     * @param txVector the TX vector
     * @param txPowerW the tx power in Watts
     */
    void Transmit(uint8_t phyId, WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW);

  private:
    void DoSetup() override;
    void DoRun() override;

    /**
     * @param count the number of packets to generate
     * @param pktSize the size of the packets to generate
     * @return an application generating the given number of packets of the given size from the
     *         non-AP STA to the AP
     */
    Ptr<PacketSocketClient> GetApplication(std::size_t count, std::size_t pktSize) const;

    /**
     * Check the retry count of the MPDUs stored in the STA MAC queue, the CW and the QSRC of the
     * given link upon transmitting a PSDU.
     *
     * @param seqNoRetryCountMap (sequence number, retry count) pair for the MPDUs that are expected
     *                           to be in the STA MAC queue
     * @param qsrc the expected QSRC for the BE AC
     * @param linkId the ID of the given link
     * @param qsrcOther the expected QSRC for the BE AC on the other link, in case of MLDs
     */
    void CheckValues(const std::map<uint16_t, uint32_t>& seqNoRetryCountMap,
                     std::size_t qsrc,
                     uint8_t linkId,
                     std::optional<std::size_t> qsrcOther);

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

    /// Set the list of events to expect in this test run.
    void SetEvents();

    std::size_t m_nLinks;         //!< number of links for the devices
    bool m_useRts;                //!< whether RTS is used to protect frame transmissions
    bool m_incrRetryCountUnderBa; //!< whether retry count is incremented under block ack
    bool m_useBarAfterBaTimeout;  //!< whether to send a BAR after a missed BlockAck
    bool m_pifsRecovery;          ///< whether to use PIFS recovery
    const Time m_txopLimit{MicroSeconds(4768)};  //!< TXOP limit
    Ptr<StaWifiMac> m_staMac;                    //!< MAC of the non-AP STA
    NetDeviceContainer m_apDevice;               ///< container for AP's NetDevice
    const uint32_t m_frameRetryLimit{4};         //!< frame retry limit
    const std::size_t m_pktSize{1000};           //!< size in bytes of generated packets
    bool m_baEstablished{false};                 //!< whether BA agreement has been established
    std::list<Events> m_events;                  //!< list of events for a test run
    std::list<Events>::const_iterator m_eventIt; //!< iterator over the list of events
    Ptr<ListErrorModel> m_apErrorModel;          ///< error model to install on the AP
    PacketSocketAddress m_ulSocket;              ///< packet socket address for UL traffic
};

WifiRetransmitTest::WifiRetransmitTest(const Params& params)
    : TestCase(std::string("Check retransmit procedure (useRts=") + std::to_string(params.useRts) +
               ", incrRetryCountUnderBa=" + std::to_string(params.incrRetryCountUnderBa) +
               ", useBarAfterBaTimeout=" + std::to_string(params.useBarAfterBaTimeout) +
               ", pifsRecovery=" + std::to_string(params.pifsRecovery) + ")"),
      m_nLinks(params.isMld ? 2 : 1),
      m_useRts(params.useRts),
      m_incrRetryCountUnderBa(params.incrRetryCountUnderBa),
      m_useBarAfterBaTimeout(params.useBarAfterBaTimeout),
      m_pifsRecovery(params.pifsRecovery),
      m_apErrorModel(CreateObject<ListErrorModel>())
{
}

void
WifiRetransmitTest::DoSetup()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 10;

    NodeContainer wifiApNode(1);
    NodeContainer wifiStaNode(1);

    SpectrumWifiPhyHelper phy(m_nLinks);
    phy.SetChannel(CreateObject<MultiModelSpectrumChannel>());
    // use default 20 MHz channel in 5 GHz band
    phy.Set(0, "ChannelSettings", StringValue("{0, 20, BAND_5GHZ, 0}"));
    if (m_nLinks > 1)
    {
        // use default 20 MHz channel in 6 GHz band
        phy.Set(1, "ChannelSettings", StringValue("{0, 20, BAND_6GHZ, 0}"));
    }

    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold",
                       UintegerValue(m_useRts ? (m_pktSize / 2) : 999999));
    Config::SetDefault("ns3::WifiRemoteStationManager::IncrementRetryCountUnderBa",
                       BooleanValue(m_incrRetryCountUnderBa));
    Config::SetDefault("ns3::WifiMac::FrameRetryLimit", UintegerValue(m_frameRetryLimit));
    Config::SetDefault("ns3::QosTxop::UseExplicitBarAfterMissedBlockAck",
                       BooleanValue(m_useBarAfterBaTimeout));
    Config::SetDefault("ns3::QosFrameExchangeManager::PifsRecovery", BooleanValue(m_pifsRecovery));

    WifiHelper wifi;
    wifi.SetStandard(m_nLinks == 1 ? WIFI_STANDARD_80211ax : WIFI_STANDARD_80211be);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 WifiModeValue(HePhy::GetHeMcs8()),
                                 "ControlMode",
                                 StringValue("OfdmRate6Mbps"));

    WifiMacHelper mac;
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(Ssid("retransmit-ssid")));

    NetDeviceContainer staDevice = wifi.Install(phy, mac, wifiStaNode);
    m_staMac = StaticCast<StaWifiMac>(StaticCast<WifiNetDevice>(staDevice.Get(0))->GetMac());

    mac.SetType("ns3::ApWifiMac");
    mac.SetEdca(AC_BE,
                "TxopLimits",
                AttributeContainerValue<TimeValue>(std::vector(m_nLinks, m_txopLimit)));

    m_apDevice = wifi.Install(phy, mac, wifiApNode);

    // Assign fixed streams to random variables in use
    streamNumber += WifiHelper::AssignStreams(m_apDevice, streamNumber);
    streamNumber += WifiHelper::AssignStreams(staDevice, streamNumber);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNode);

    PacketSocketHelper packetSocket;
    packetSocket.Install(wifiApNode);
    packetSocket.Install(wifiStaNode);

    // install a packet socket server on the AP
    PacketSocketAddress srvAddr;
    srvAddr.SetSingleDevice(m_apDevice.Get(0)->GetIfIndex());
    srvAddr.SetProtocol(1);
    auto server = CreateObject<PacketSocketServer>();
    server->SetLocal(srvAddr);
    wifiApNode.Get(0)->AddApplication(server);
    server->SetStartTime(Time{0}); // now
    server->SetStopTime(Seconds(1.0));

    // set UL packet socket
    m_ulSocket.SetSingleDevice(staDevice.Get(0)->GetIfIndex());
    m_ulSocket.SetPhysicalAddress(m_apDevice.Get(0)->GetAddress());
    m_ulSocket.SetProtocol(1);

    // install the error model on the AP
    auto dev = DynamicCast<WifiNetDevice>(m_apDevice.Get(0));
    for (std::size_t linkId = 0; linkId < m_nLinks; ++linkId)
    {
        dev->GetMac()->GetWifiPhy(linkId)->SetPostReceptionErrorModel(m_apErrorModel);
    }

    Callback<void, Mac48Address, uint8_t, std::optional<Mac48Address>> baEstablished =
        [this](Mac48Address, uint8_t, std::optional<Mac48Address>) {
            m_baEstablished = true;
            // force the first TXOP to be started on link 0 in case of MLDs
            if (m_nLinks > 1)
            {
                m_staMac->BlockTxOnLink(1, WifiQueueBlockedReason::TID_NOT_MAPPED);
            }
        };

    m_staMac->GetQosTxop(AC_BE)->TraceConnectWithoutContext("BaEstablished", baEstablished);

    // Trace PSDUs passed to the PHY on all devices
    for (std::size_t phyId = 0; phyId < m_nLinks; phyId++)
    {
        Config::ConnectWithoutContext(
            "/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phys/" + std::to_string(phyId) +
                "/PhyTxPsduBegin",
            MakeCallback(&WifiRetransmitTest::Transmit, this).Bind(phyId));
    }

    SetEvents();
}

void
WifiRetransmitTest::CheckValues(const std::map<uint16_t, uint32_t>& seqNoRetryCountMap,
                                std::size_t qsrc,
                                uint8_t linkId,
                                std::optional<std::size_t> qsrcOther)
{
    const auto psduNumber = std::distance(m_events.cbegin(), m_eventIt);
    const auto apAddr = Mac48Address::ConvertFrom(m_apDevice.Get(0)->GetAddress());
    WifiContainerQueueId queueId{WIFI_QOSDATA_QUEUE, WIFI_UNICAST, apAddr, 0};
    const auto staQueue = m_staMac->GetTxopQueue(AC_BE);
    NS_TEST_EXPECT_MSG_EQ(seqNoRetryCountMap.size(),
                          staQueue->GetNPackets(queueId),
                          "Unexpected number of queued MPDUs when transmitting frame #"
                              << psduNumber);

    Ptr<WifiMpdu> mpdu;
    while ((mpdu = staQueue->PeekByQueueId(queueId, mpdu)))
    {
        const auto seqNo = mpdu->GetHeader().GetSequenceNumber();
        const auto it = seqNoRetryCountMap.find(seqNo);
        NS_TEST_ASSERT_MSG_EQ((it != seqNoRetryCountMap.cend()),
                              true,
                              "SeqNo " << seqNo << " not found in PSDU #" << psduNumber);
        NS_TEST_EXPECT_MSG_EQ(mpdu->GetRetryCount(),
                              it->second,
                              "Unexpected retry count for MPDU with SeqNo=" << seqNo << " in PSDU #"
                                                                            << psduNumber);
    }

    NS_TEST_EXPECT_MSG_EQ(qsrcOther.has_value(),
                          (m_nLinks > 1),
                          "QSRC for other link can be provided iff devices are multi-link");

    std::map<uint8_t, std::size_t> qsrcLinkIdMap{{linkId, qsrc}};
    // check the QSRC on the other link in case of MLDs
    if (m_nLinks > 1)
    {
        qsrcLinkIdMap.emplace((linkId == 0 ? 1 : 0), qsrcOther.value());
    }

    const auto txop = m_staMac->GetQosTxop(AC_BE);

    for (const auto& [id, expectedQsrc] : qsrcLinkIdMap)
    {
        NS_TEST_EXPECT_MSG_EQ(txop->GetStaRetryCount(id),
                              expectedQsrc,
                              "Unexpected QSRC value on link " << +id << " when transmitting PSDU #"
                                                               << psduNumber);

        const auto cwMin = txop->GetMinCw(id);
        const auto cwMax = txop->GetMaxCw(id);
        const auto expectedCw = std::min(cwMax, (1 << expectedQsrc) * (cwMin + 1) - 1);

        NS_TEST_EXPECT_MSG_EQ(txop->GetCw(id),
                              expectedCw,
                              "Unexpected CW value on link " << +id << " when transmitting PSDU #"
                                                             << psduNumber);
    }
}

void
WifiRetransmitTest::Transmit(uint8_t phyId,
                             WifiConstPsduMap psduMap,
                             WifiTxVector txVector,
                             double txPowerW)
{
    const auto psdu = psduMap.cbegin()->second;
    const auto& hdr = psdu->GetHeader(0);
    const auto printAndQuit = (!m_baEstablished || hdr.IsBeacon() || hdr.IsCfEnd());

    std::stringstream ss;
    ss << " Phy ID " << +phyId;
    if (!printAndQuit && m_eventIt != m_events.cend())
    {
        ss << " PSDU #" << std::distance(m_events.cbegin(), m_eventIt);
    }
    for (const auto& mpdu : *PeekPointer(psdu))
    {
        ss << "\n" << *mpdu;
    }
    ss << "\nTXVECTOR = " << txVector << "\n";
    NS_LOG_INFO(ss.str());

    // do nothing if block ack agreement has not been established yet or the frame being
    // transmitted is a Beacon frame or a CF-End frame
    if (printAndQuit)
    {
        return;
    }

    if (m_eventIt != m_events.cend())
    {
        // check that the expected frame is being transmitted
        NS_TEST_EXPECT_MSG_EQ(m_eventIt->hdrType,
                              hdr.GetType(),
                              "Unexpected MAC header type for frame #"
                                  << std::distance(m_events.cbegin(), m_eventIt));
        // perform actions/checks, if any
        if (m_eventIt->func)
        {
            m_eventIt->func(psdu, txVector);
        }

        ++m_eventIt;
    }
}

void
WifiRetransmitTest::SetEvents()
{
    // lambda to drop all MPDUs in the given PSDU
    auto dropPsdu = [this](Ptr<const WifiPsdu> psdu, const WifiTxVector&) {
        std::list<uint64_t> uids;
        for (const auto& mpdu : *PeekPointer(psdu))
        {
            uids.push_back(mpdu->GetPacket()->GetUid());
        }
        m_apErrorModel->SetList(uids);
    };

    // lambda to block transmissions on link 0 and unblock transmissions on link 1 after the
    // given amount of time past the end of the transmission of the current frame
    auto alternateLinks =
        [this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, Time delay) {
            m_staMac->BlockTxOnLink(0, WifiQueueBlockedReason::TID_NOT_MAPPED);

            const auto txDuration =
                WifiPhy::CalculateTxDuration(psdu, txVector, m_staMac->GetWifiPhy(0)->GetPhyBand());

            Simulator::Schedule(txDuration + delay, [this]() {
                m_staMac->UnblockTxOnLink({1}, WifiQueueBlockedReason::TID_NOT_MAPPED);
            });
        };

    std::size_t qsrc = 0;
    uint8_t linkId = 0; // first TXOP takes place on the link 0
    std::optional<std::size_t> qsrcOther;
    if (m_nLinks > 1)
    {
        qsrcOther = 0; // QSRC on link 1
    }

    // 1st TXOP: the first transmission (RTS or data frames) fails (no response)
    m_events.emplace_back(
        (m_useRts ? WIFI_MAC_CTL_RTS : WIFI_MAC_QOSDATA),
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) {
            // initially, QoS data 0 and QoS data 1 have retry count equal to zero
            CheckValues({{0, 0}, {1, 0}}, 0, linkId, qsrcOther);
            // drop the entire PSDU
            dropPsdu(psdu, txVector);
            // generate two more QoS data frames
            m_staMac->GetDevice()->GetNode()->AddApplication(GetApplication(2, m_pktSize));
            // in case of MLDs, force the second TXOP to be started on link 1 by blocking TX on
            // link 0 and unblocking TX on link 1; this is done at block ack timeout unless a BAR
            // is going to be sent after this transmission failure
            if (m_nLinks > 1 && (m_useRts || !m_useBarAfterBaTimeout))
            {
                alternateLinks(
                    psdu,
                    txVector,
                    m_staMac->GetFrameExchangeManager(0)->GetWifiTxTimer().GetDelayLeft());
            }
        });

    if (m_nLinks > 1)
    {
        // 2nd TXOP occurs on link 1
        linkId = 1;
        qsrc = 0;
        // last transmission on link 0 failed, unless RTS is not used and a BAR is sent after a
        // missed BlockAck (in which case, the last transmission is a successful BAR-BA exchange)
        qsrcOther = ((m_useRts || !m_useBarAfterBaTimeout) ? 1 : 0);
    }
    else
    {
        // QSRC is increased after the previous TX failure
        ++qsrc;
    }

    // 2nd TXOP
    if (m_useRts)
    {
        // RTS and CTS are sent before the data frames
        m_events.emplace_back(WIFI_MAC_CTL_RTS);
        m_events.emplace_back(WIFI_MAC_CTL_CTS);
    }
    else if (m_useBarAfterBaTimeout)
    {
        // BAR and BA are sent before the data frames
        m_events.emplace_back(WIFI_MAC_CTL_BACKREQ);
        m_events.emplace_back(WIFI_MAC_CTL_BACKRESP,
                              [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) {
                                  // in case of MLDs, we can block TX on link 0 and unblock TX on
                                  // link 1 as soon as the block ack response is received
                                  if (m_nLinks > 1)
                                  {
                                      alternateLinks(psdu, txVector, Time{0});
                                  }
                              });
        // QSRC is reset because the BAR/BA exchange succeeded
        qsrc = 0;
    }

    // for the second transmission, two MPDUs in the A-MPDU out of four are received correctly

    // 4 QoS data frames are now sent in an A-MPDU
    m_events.emplace_back(
        WIFI_MAC_QOSDATA,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) {
            // after previous failure, QoS data 0 and QoS data 1 have retry count
            // equal to 1, unless incrRetryCountUnderBa is false
            if (m_incrRetryCountUnderBa)
            {
                CheckValues({{0, 1}, {1, 1}, {2, 0}, {3, 0}}, qsrc, linkId, qsrcOther);
            }
            else
            {
                CheckValues({{0, 0}, {1, 0}, {2, 0}, {3, 0}}, qsrc, linkId, qsrcOther);
            }
            // drop QoS data 1 and 2
            m_apErrorModel->SetList({psdu->GetPayload(1)->GetUid(), psdu->GetPayload(2)->GetUid()});
        });

    // Block Ack response after A-MPDU
    m_events.emplace_back(WIFI_MAC_CTL_BACKRESP);

    // previous transmission succeeded, reset QSRC
    qsrc = 0;

    // 2nd TXOP continues with the STA attempting to transmit the remaining two QoS data frames
    // (always without RTS because STA is already protected); this attempt fails (no response)
    m_events.emplace_back(WIFI_MAC_QOSDATA,
                          [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) {
                              // the previous successful TX has not modified the retry count of the
                              // MPDUs
                              if (m_incrRetryCountUnderBa)
                              {
                                  CheckValues({{1, 1}, {2, 0}}, qsrc, linkId, qsrcOther);
                              }
                              else
                              {
                                  CheckValues({{1, 0}, {2, 0}}, qsrc, linkId, qsrcOther);
                              }
                              // the error model already has the UIDs of the two remaining MPDUs
                          });

    // keep transmitting the A-MPDU until the retry count of QoS data 0 reaches the limit
    for (uint32_t retryCount = 2; retryCount < m_frameRetryLimit; ++retryCount)
    {
        // if PIFS recovery is used, QSRC is not modified; otherwise, QSRC is incremented
        if (!m_pifsRecovery)
        {
            ++qsrc;
        }

        if (m_useBarAfterBaTimeout)
        {
            // 2nd TXOP is resumed with BAR/BA exchange (even if RTS is enabled because the BAR size
            // is smaller than the RTS threshold)
            m_events.emplace_back(
                WIFI_MAC_CTL_BACKREQ,
                [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) {
                    // the retry count of the MPDUs has increased after the previous failed
                    // TX
                    if (m_incrRetryCountUnderBa)
                    {
                        CheckValues({{1, retryCount}, {2, retryCount - 1}},
                                    qsrc,
                                    linkId,
                                    qsrcOther);
                    }
                    else
                    {
                        CheckValues({{1, 0}, {2, 0}}, qsrc, linkId, qsrcOther);
                    }
                });
            m_events.emplace_back(WIFI_MAC_CTL_BACKRESP);
            // QSRC is reset because the BAR/BA exchange succeeded
            qsrc = 0;
        }

        // 2nd TXOP continues with the STA attempting to transmit the remaining two QoS data frames
        // (with RTS, if enabled, because STA is no longer protected after previous TX failure);
        // this attempt fails (no response)
        m_events.emplace_back(
            (m_useRts && !m_useBarAfterBaTimeout ? WIFI_MAC_CTL_RTS : WIFI_MAC_QOSDATA),
            [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) {
                // the retry count of the MPDUs has increased after the previous failed TX
                if (m_incrRetryCountUnderBa)
                {
                    CheckValues({{1, retryCount}, {2, retryCount - 1}}, qsrc, linkId, qsrcOther);
                }
                else
                {
                    CheckValues({{1, 0}, {2, 0}}, qsrc, linkId, qsrcOther);
                }
                // drop the entire PSDU
                dropPsdu(psdu, txVector);
            });
    }

    // if PIFS recovery is used, QSRC is not modified; otherwise, QSRC is incremented
    if (!m_pifsRecovery)
    {
        ++qsrc;
    }

    // if retry count is incremented, QoS data 0 has reached the retry limit and has been dropped,
    // hence STA sends a BAR to advance the recipient window. If PIFS recovery is not used, we
    // drop the BAR multiple times to observe a QSRC reset while QoS data 2 is still not dropped
    if (m_incrRetryCountUnderBa)
    {
        for (std::size_t count = 0; count < 4; ++count)
        {
            // if the QSRC has been incremented after a TX failure (i.e., PIFS recovery is not used)
            // and has not been reset after a successful BAR/BA exchange, it is now equal to the
            // frame retry limit minus one.
            if (!m_pifsRecovery && !m_useBarAfterBaTimeout)
            {
                qsrc = (m_frameRetryLimit - 1 + count) % (m_frameRetryLimit + 1);
            }

            m_events.emplace_back(
                WIFI_MAC_CTL_BACKREQ,
                [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) {
                    CheckValues({{2, m_frameRetryLimit - 1}}, qsrc, linkId, qsrcOther);
                    // drop the BlockAckReq
                    dropPsdu(psdu, txVector);
                });

            // if PIFS recovery is used, QSRC is not modified; otherwise, QSRC is incremented
            if (!m_pifsRecovery)
            {
                ++qsrc;
            }
        }
    }
    else
    {
        m_events.emplace_back((m_useBarAfterBaTimeout
                                   ? WIFI_MAC_CTL_BACKREQ
                                   : (m_useRts ? WIFI_MAC_CTL_RTS : WIFI_MAC_QOSDATA)),
                              [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) {
                                  CheckValues({{1, 0}, {2, 0}}, qsrc, linkId, qsrcOther);
                                  // QoS data frames transmission succeeds
                                  m_apErrorModel->SetList({});
                              });
    }

    m_eventIt = m_events.cbegin();
}

void
WifiRetransmitTest::DoRun()
{
    // 500 milliseconds are more than enough to complete association
    Simulator::Schedule(MilliSeconds(500),
                        &Node::AddApplication,
                        m_staMac->GetDevice()->GetNode(),
                        GetApplication(2, m_pktSize));

    Simulator::Stop(Seconds(1));
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ((m_eventIt == m_events.cend()), true, "Not all events took place");

    Simulator::Destroy();
}

Ptr<PacketSocketClient>
WifiRetransmitTest::GetApplication(std::size_t count, std::size_t pktSize) const
{
    auto client = CreateObject<PacketSocketClient>();
    client->SetAttribute("PacketSize", UintegerValue(pktSize));
    client->SetAttribute("MaxPackets", UintegerValue(count));
    client->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
    client->SetRemote(m_ulSocket);
    client->SetStartTime(Time{0}); // now
    client->SetStopTime(Seconds(1.0));

    return client;
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi retransmit procedure Test Suite
 */
class WifiRetransmitTestSuite : public TestSuite
{
  public:
    WifiRetransmitTestSuite();
};

WifiRetransmitTestSuite::WifiRetransmitTestSuite()
    : TestSuite("wifi-retransmit", Type::UNIT)
{
    for (auto isMld : {true, false})
    {
        for (auto useRts : {true, false})
        {
            for (auto incrRetryCountUnderBa : {true, false})
            {
                for (auto useBarAfterBaTimeout : {true, false})
                {
                    for (auto pifsRecovery : {true, false})
                    {
                        AddTestCase(
                            new WifiRetransmitTest({.isMld = isMld,
                                                    .useRts = useRts,
                                                    .incrRetryCountUnderBa = incrRetryCountUnderBa,
                                                    .useBarAfterBaTimeout = useBarAfterBaTimeout,
                                                    .pifsRecovery = pifsRecovery}),
                            TestCase::Duration::QUICK);
                    }
                }
            }
        }
    }
}

static WifiRetransmitTestSuite g_wifiRetransmitTestSuite; ///< the test suite
