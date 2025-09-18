/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "wifi-dso-test.h"

#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/default-dso-manager.h"
#include "ns3/dso-multi-user-scheduler.h"
#include "ns3/eht-configuration.h"
#include "ns3/he-configuration.h"
#include "ns3/interference-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/node-list.h"
#include "ns3/non-communicating-net-device.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/qos-txop.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/string.h"
#include "ns3/uhr-frame-exchange-manager.h"
#include "ns3/uhr-phy.h"
#include "ns3/vht-configuration.h"
#include "ns3/wifi-acknowledgment.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-ns3-constants.h"
#include "ns3/wifi-protection.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-static-setup-helper.h"

#include <algorithm>
#include <initializer_list>
#include <optional>
#include <utility>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiDsoTest");

/**
 * Extended Default DSO manager class for the purpose of the tests.
 */
class TestDsoManager : public DefaultDsoManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::TestDsoManager")
                                .SetParent<DefaultDsoManager>()
                                .SetGroupName("Wifi")
                                .AddConstructor<TestDsoManager>();
        return tid;
    }

    using DefaultDsoManager::GetDsoSubbands;
    using DefaultDsoManager::GetPrimarySubband;
};

NS_OBJECT_ENSURE_REGISTERED(TestDsoManager);

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Extend DSO Multi User Scheduler
 *
 */
class TestDsoMultiUserScheduler : public DsoMultiUserScheduler
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TestDsoMultiUserScheduler();
    ~TestDsoMultiUserScheduler() override;

    /**
     * Force the transmission format to be used by the scheduler.
     * @param forcedFormat the transmission format to be set, std::nullopt for no forced
     * transmission format
     */
    void SetForcedFormat(std::optional<TxFormat> forcedFormat);

  protected:
    std::optional<TxFormat> DoSelectTxFormat() override;

  private:
    std::optional<TxFormat> m_forcedFormat; ///< forced transmission format
};

NS_OBJECT_ENSURE_REGISTERED(TestDsoMultiUserScheduler);

TypeId
TestDsoMultiUserScheduler::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TestDsoMultiUserScheduler")
                            .SetParent<DsoMultiUserScheduler>()
                            .SetGroupName("Wifi")
                            .AddConstructor<TestDsoMultiUserScheduler>();
    return tid;
}

TestDsoMultiUserScheduler::TestDsoMultiUserScheduler()
{
    NS_LOG_FUNCTION(this);
}

TestDsoMultiUserScheduler::~TestDsoMultiUserScheduler()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
TestDsoMultiUserScheduler::SetForcedFormat(std::optional<MultiUserScheduler::TxFormat> forcedFormat)
{
    NS_LOG_FUNCTION(this << forcedFormat.value_or(MultiUserScheduler::TxFormat::NO_TX));
    m_forcedFormat = forcedFormat;
}

std::optional<MultiUserScheduler::TxFormat>
TestDsoMultiUserScheduler::DoSelectTxFormat()
{
    NS_LOG_FUNCTION(this);
    return m_forcedFormat;
}

DsoTestBase::DsoTestBase(const std::string& name)
    : TestCase(name)
{
}

void
DsoTestBase::DoSetup()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(10);
    int64_t streamNumber = 100;

    // prioritize trigger-based UL
    const auto muEdcaTimer = 255 * 8 * WIFI_TU;
    Config::SetDefault("ns3::HeConfiguration::BeMuEdcaTimer", TimeValue(muEdcaTimer));
    Config::SetDefault("ns3::HeConfiguration::BkMuEdcaTimer", TimeValue(muEdcaTimer));
    Config::SetDefault("ns3::HeConfiguration::ViMuEdcaTimer", TimeValue(muEdcaTimer));
    Config::SetDefault("ns3::HeConfiguration::VoMuEdcaTimer", TimeValue(muEdcaTimer));

    NodeContainer wifiApNode(1);
    NodeContainer wifiStaNodes;

    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue(m_mode),
                                 "ControlMode",
                                 StringValue("UhrMcs0"));

    SpectrumWifiPhyHelper stasPhyHelper;
    stasPhyHelper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

    m_channel = CreateObject<MultiModelSpectrumChannel>();
    stasPhyHelper.SetChannel(m_channel);

    WifiMacHelper mac;
    mac.SetType("ns3::StaWifiMac",
                "Ssid",
                SsidValue(Ssid("ns-3-ssid")),
                "ActiveProbing",
                BooleanValue(false));
    mac.SetDsoManager("ns3::TestDsoManager");

    NetDeviceContainer staDevices;
    for (std::size_t i = 0; i < m_nDsoStas + m_nNonDsoStas + m_nNonUhrStas; ++i)
    {
        wifi.ConfigUhrOptions("DsoActivated", BooleanValue(i < m_nDsoStas));
        stasPhyHelper.Set("ChannelSettings", StringValue(m_stasOpChannel.at(i)));
        NodeContainer staNode(1);
        wifi.SetStandard(i < (m_nDsoStas + m_nNonDsoStas) ? WIFI_STANDARD_80211bn
                                                          : WIFI_STANDARD_80211be);
        staDevices.Add(wifi.Install(stasPhyHelper, mac, staNode));
        wifiStaNodes.Add(staNode);
    }

    for (uint32_t i = 0; i < staDevices.GetN(); i++)
    {
        auto staDevice = DynamicCast<WifiNetDevice>(staDevices.Get(i));
        m_staMacs.push_back(DynamicCast<StaWifiMac>(staDevice->GetMac()));
    }

    SpectrumWifiPhyHelper apPhyHelper;
    apPhyHelper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    apPhyHelper.SetChannel(m_channel);

    mac.SetType("ns3::ApWifiMac",
                "Ssid",
                SsidValue(Ssid("ns-3-ssid")),
                "BeaconGeneration",
                BooleanValue(false));
    mac.SetMultiUserScheduler("ns3::TestDsoMultiUserScheduler");

    apPhyHelper.Set("ChannelSettings", StringValue(m_apOpChannel));
    wifi.SetStandard(WIFI_STANDARD_80211bn);
    auto apDevice = wifi.Install(apPhyHelper, mac, wifiApNode);
    m_apMac = DynamicCast<ApWifiMac>(DynamicCast<WifiNetDevice>(apDevice.Get(0))->GetMac());

    m_apMac->GetFrameExchangeManager()->GetAckManager()->SetAttribute(
        "DlMuAckSequenceType",
        EnumValue(WifiAcknowledgment::DL_MU_AGGREGATE_TF));

    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxPsduBegin",
                                  MakeCallback(&DsoTestBase::Transmit, this));

    // Uncomment the lines below to write PCAP files
    // apPhyHelper.EnablePcap("wifi-dso_AP", apDevice);
    // stasPhyHelper.EnablePcap("wifi-dso_STA", staDevices);

    // Assign fixed streams to random variables in use
    streamNumber += WifiHelper::AssignStreams(apDevice, streamNumber);
    streamNumber += WifiHelper::AssignStreams(staDevices, streamNumber);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    for (std::size_t i = 0; i <= m_nDsoStas + m_nNonDsoStas + m_nNonUhrStas; ++i)
    {
        positionAlloc->Add(Vector(i, 0.0, 0.0));
    }
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);

    auto apDev = DynamicCast<WifiNetDevice>(apDevice.Get(0));
    NS_ASSERT(apDev);
    WifiStaticSetupHelper::SetStaticAssociation(apDev, staDevices);
    WifiStaticSetupHelper::SetStaticBlockAck(apDev, staDevices, {0});

    // install packet socket on all nodes
    PacketSocketHelper packetSocket;
    packetSocket.Install(wifiApNode);
    packetSocket.Install(wifiStaNodes);

    // install a packet socket server on all nodes
    for (auto nodeIt = NodeList::Begin(); nodeIt != NodeList::End(); nodeIt++)
    {
        PacketSocketAddress srvAddr;
        auto device = DynamicCast<WifiNetDevice>((*nodeIt)->GetDevice(0));
        NS_TEST_ASSERT_MSG_NE(device, nullptr, "Expected a WifiNetDevice");
        srvAddr.SetSingleDevice(device->GetIfIndex());
        srvAddr.SetProtocol(1);

        auto server = CreateObject<PacketSocketServer>();
        server->SetLocal(srvAddr);
        (*nodeIt)->AddApplication(server);
        server->SetStartTime(Seconds(0)); // now
        server->SetStopTime(m_duration);
    }

    // set DL and UL packet sockets
    for (const auto& staMac : m_staMacs)
    {
        m_dlSockets.emplace_back();
        m_dlSockets.back().SetSingleDevice(m_apMac->GetDevice()->GetIfIndex());
        m_dlSockets.back().SetPhysicalAddress(staMac->GetDevice()->GetAddress());
        m_dlSockets.back().SetProtocol(1);

        m_ulSockets.emplace_back();
        m_ulSockets.back().SetSingleDevice(staMac->GetDevice()->GetIfIndex());
        m_ulSockets.back().SetPhysicalAddress(m_apMac->GetDevice()->GetAddress());
        m_ulSockets.back().SetProtocol(1);
    }

    Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::PacketSocketServer/Rx",
                                  MakeCallback(&DsoTestBase::Receive, this));

    Simulator::ScheduleNow([&]() { StartTraffic(); });
}

Ptr<PacketSocketClient>
DsoTestBase::GetApplication(TrafficDirection dir,
                            std::size_t staId,
                            std::size_t count,
                            std::size_t pktSize,
                            uint8_t priority) const
{
    NS_LOG_FUNCTION(this << +dir << staId << count << pktSize << priority);

    auto client = CreateObject<PacketSocketClient>();
    client->SetAttribute("PacketSize", UintegerValue(pktSize));
    client->SetAttribute("MaxPackets", UintegerValue(count));
    client->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
    client->SetAttribute("Priority", UintegerValue(priority));
    client->SetRemote(dir == DOWNLINK ? m_dlSockets.at(staId) : m_ulSockets.at(staId));
    client->SetStartTime(Seconds(0)); // now
    client->SetStopTime(m_duration - Simulator::Now());

    return client;
}

void
DsoTestBase::StartTraffic()
{
    NS_LOG_FUNCTION(this);
}

void
DsoTestBase::Transmit(WifiConstPsduMap psduMap, WifiTxVector txVector, double /*txPowerW*/)
{
    NS_LOG_FUNCTION(this << psduMap << txVector);
    m_txPsdus.push_back({Simulator::Now(), psduMap, txVector});
}

void
DsoTestBase::Receive(Ptr<const Packet> p, const Address& adr)
{
    NS_LOG_FUNCTION(this << p << adr);
    ++m_receivedPackets;
}

DsoSubbandsTest::DsoSubbandsTest(
    const std::string& apChannel,
    const std::string& stasChannel,
    const std::map<EhtRu::PrimaryFlags, WifiPhyOperatingChannel>& expectedDsoSubbands)
    : DsoTestBase("Check computation of DSO subbands: AP operating channel is " + apChannel +
                  " and STA operating channel is " + stasChannel),
      m_expectedDsoSubbands{expectedDsoSubbands}
{
    m_nDsoStas = 1;
    m_apOpChannel = apChannel;
    m_stasOpChannel = {stasChannel};
}

void
DsoSubbandsTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    const auto dsoManager = DynamicCast<TestDsoManager>(m_staMacs.at(0)->GetDsoManager());
    NS_TEST_EXPECT_MSG_EQ(dsoManager->GetPrimarySubband(SINGLE_LINK_OP_ID),
                          m_staMacs.at(0)->GetWifiPhy()->GetOperatingChannel(),
                          "Unexpected primary subband");
    const auto dsoSubbands = dsoManager->GetDsoSubbands(SINGLE_LINK_OP_ID);
    NS_TEST_EXPECT_MSG_EQ(dsoSubbands.size(),
                          m_expectedDsoSubbands.size(),
                          "Unexpected number of DSO subbands");

    for (const auto& [primaryFlag, expectedSubband] : m_expectedDsoSubbands)
    {
        std::ostringstream oss;
        oss << "DSO subband '" << expectedSubband << "' not found for " << primaryFlag;
        NS_TEST_EXPECT_MSG_EQ(dsoSubbands.count(primaryFlag), 1, oss.str());
        oss.str("");
        oss << "Unexpected DSO subband '" << dsoSubbands.at(primaryFlag) << "' for " << primaryFlag;
        NS_TEST_EXPECT_MSG_EQ(dsoSubbands.at(primaryFlag), expectedSubband, oss.str());
    }

    Simulator::Destroy();
}

DsoTxopTest::DsoTxopTest(const Params& params)
    : DsoTestBase(params.testName),
      m_params{params}
{
    m_nDsoStas = 2;
    m_apOpChannel = "{114, 0, BAND_5GHZ, 0}";
    m_stasOpChannel = {"{106, 0, BAND_5GHZ, 0}", "{106, 0, BAND_5GHZ, 0}"};
    if (m_params.generateInterferenceAfterIcf)
    {
        // use more robust modulation if interference is generated
        m_mode = "UhrMcs0";
    }
    // use default DSO channel switch back delay if not provided
    if (m_params.channelSwitchBackDelays.empty())
    {
        m_params.channelSwitchBackDelays =
            std::vector<Time>(m_nDsoStas, DEFAULT_CHANNEL_SWITCH_DELAY);
    }
    // use default DSO padding delay if not provided
    if (m_params.paddingDelays.empty())
    {
        m_params.paddingDelays = std::vector<Time>(m_nDsoStas, MicroSeconds(0));
    }
}

void
DsoTxopTest::GenerateObssFrame()
{
    NS_LOG_FUNCTION(this);

    WifiTxVector txVector{OfdmPhy::GetOfdmRate6Mbps(),
                          WIFI_MIN_TX_PWR_LEVEL,
                          WIFI_PREAMBLE_LONG,
                          NanoSeconds(800),
                          1,
                          1,
                          0,
                          m_obssPhy->GetChannelWidth(),
                          false,
                          false};

    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_CTL_CTS);
    hdr.SetDsNotFrom();
    hdr.SetDsNotTo();
    hdr.SetNoMoreFragments();
    hdr.SetNoRetry();
    hdr.SetAddr1(Mac48Address("00:00:00:00:00:10"));

    auto pkt = Create<Packet>();
    auto psdu = Create<WifiPsdu>(pkt, hdr);

    // ensure DSO STA latches on OBSS instead of MU PPDU by setting a higher TX power to the OBSS
    // PHY
    m_obssPhy->SetTxPowerStart(dBm_t{30});
    m_obssPhy->SetTxPowerEnd(dBm_t{30});
    m_apMac->GetWifiPhy()->SetTxPowerStart(dBm_t{10});
    m_apMac->GetWifiPhy()->SetTxPowerEnd(dBm_t{10});

    auto txDuration = WifiPhy::CalculateTxDuration(psdu, txVector, m_obssPhy->GetPhyBand());
    for (std::size_t i = 0; i < m_nDsoStas; ++i)
    {
        ScheduleChecksSwitchBack(i, txDuration);
    }

    m_obssPhy->Send(psdu, txVector);
}

void
DsoTxopTest::StartInterference(const Time& duration)
{
    NS_LOG_FUNCTION(this << duration);

    // generate interference on the second half of the DSO subband
    auto dsoManager =
        DynamicCast<TestDsoManager>(m_staMacs.at(m_idxStaInDsoSubband)->GetDsoManager());
    const auto centerFreq =
        dsoManager->GetDsoSubbands(SINGLE_LINK_OP_ID).cbegin()->second.GetFrequency();
    const auto bw = dsoManager->GetDsoSubbands(SINGLE_LINK_OP_ID).cbegin()->second.GetWidth();
    BandInfo bandInfo{
        .fl = centerFreq.in_Hz(),
        .fc = centerFreq.in_Hz() + (bw / 4).in_Hz(),
        .fh = centerFreq.in_Hz() + (bw / 2).in_Hz(),
    };

    Bands bands;
    bands.push_back(bandInfo);
    auto spectrumInterference = Create<SpectrumModel>(bands);
    auto interferencePsd = Create<SpectrumValue>(spectrumInterference);
    Watt_t interferencePower{0.1};
    *interferencePsd = interferencePower.in_Watt() / (bw / 2).in_Hz();

    m_interferer->SetTxPowerSpectralDensity(interferencePsd);
    m_interferer->SetPeriod(duration);
    m_interferer->Start();
}

void
DsoTxopTest::StopInterference()
{
    NS_LOG_FUNCTION(this);
    m_interferer->Stop();
}

void
DsoTxopTest::StartTraffic()
{
    NS_LOG_FUNCTION(this);

    auto dsoManager =
        DynamicCast<TestDsoManager>(m_staMacs.at(m_idxStaInDsoSubband)->GetDsoManager());
    m_obssPhy->SetOperatingChannel(dsoManager->GetDsoSubbands(SINGLE_LINK_OP_ID).cbegin()->second);

    DsoTestBase::StartTraffic();

    auto numDlMuPpdus{m_params.numDlMuPpdus};
    if ((m_params.numDlMuPpdus == 0) && (m_params.numUlMuPpdus == 0))
    {
        // we need to generate at least one packet to trigger the DSO frame exchange to be
        // initiated, but we will ensure the packet gets dropped before it has a chance to be
        // transmitted
        numDlMuPpdus = 1;
    }

    if (numDlMuPpdus > 0)
    {
        for (std::size_t i = 0; i < m_nDsoStas; ++i)
        {
            m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(DOWNLINK, i, 1, 1000));
        }
    }

    auto muScheduler =
        DynamicCast<TestDsoMultiUserScheduler>(m_apMac->GetObject<MultiUserScheduler>());
    if (m_params.numUlMuPpdus > 0)
    {
        muScheduler->SetAttribute("EnableUlOfdma", BooleanValue(true));
        muScheduler->SetAccessReqInterval(MilliSeconds(1));
    }
    else
    {
        muScheduler->SetAttribute("EnableUlOfdma", BooleanValue(false));
        muScheduler->SetAccessReqInterval(Time());
    }
}

void
DsoTxopTest::Transmit(WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW)
{
    DsoTestBase::Transmit(psduMap, txVector, txPowerW);
    auto psdu = psduMap.begin()->second;

    switch (psdu->GetHeader(0).GetType())
    {
    case WIFI_MAC_CTL_TRIGGER:
        CheckIcf(psduMap, txVector);
        break;

    case WIFI_MAC_QOSDATA_NULL:
        CheckQosNullFrames(psduMap, txVector);
        break;

    case WIFI_MAC_QOSDATA:
        CheckQosFrames(psduMap, txVector);
        break;

    case WIFI_MAC_CTL_BACKRESP:
        CheckBlockAck(psduMap, txVector);
        break;

    default:
        break;
    }
}

void
DsoTxopTest::CheckIcf(const WifiConstPsduMap& psduMap, const WifiTxVector& txVector)
{
    CtrlTriggerHeader trigger;
    auto mpdu = *psduMap.begin()->second->begin();
    mpdu->GetPacket()->PeekHeader(trigger);
    if (!trigger.IsBsrp())
    {
        // not a DSO ICF
        return;
    }

    NS_LOG_DEBUG("Send DSO ICF");
    ++m_countIcf;

    NS_TEST_EXPECT_MSG_EQ(txVector.IsNonHtDuplicate(),
                          true,
                          "DSO ICF shall be sent in the non-HT duplicate PPDU format");

    const auto dataRate = txVector.GetMode().GetDataRate(MHz_t{20});
    NS_TEST_EXPECT_MSG_EQ(
        ((dataRate == 6'000'000) || (dataRate == 12'000'000) || (dataRate == 24'000'000)),
        true,
        "DSO ICF shall be sent at 6 Mb/s, 12 Mb/s, or 24 Mb/s");

    auto apPhy = m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID);
    const auto icfDuration = WifiPhy::CalculateTxDuration(psduMap, txVector, apPhy->GetPhyBand());

    const auto maxPaddingDelay =
        *std::max_element(m_params.paddingDelays.cbegin(), m_params.paddingDelays.cend());
    if (maxPaddingDelay.IsStrictlyPositive())
    {
        // compare the TX duration of this Trigger Frame to that of the Trigger Frame with no
        // padding added
        trigger.SetPaddingSize(0);
        auto pkt = Create<Packet>();
        pkt->AddHeader(trigger);
        auto icfDurationWithout =
            WifiPhy::CalculateTxDuration(Create<WifiPsdu>(pkt, mpdu->GetHeader()),
                                         txVector,
                                         apPhy->GetPhyBand());

        NS_TEST_EXPECT_MSG_EQ(icfDuration,
                              icfDurationWithout + maxPaddingDelay,
                              "Unexpected TX duration of the DSO ICF with padding "
                                  << maxPaddingDelay.As(Time::US));
    }

    if (m_countIcf > 1)
    {
        return; // only check the ICF of the first DSO TXOP
    }

    if (m_params.corruptIcf)
    {
        NS_LOG_DEBUG("CORRUPTED");
        for (std::size_t i = 0; i < m_staErrorModels.size(); ++i)
        {
            m_staErrorModels.at(i)->SetList({mpdu->GetPacket()->GetUid()});
            const auto uhrFem = DynamicCast<UhrFrameExchangeManager>(
                m_staMacs.at(i)->GetFrameExchangeManager(SINGLE_LINK_OP_ID));
            auto tbTxVector = uhrFem->GetHeTbTxVector(trigger, m_apMac->GetAddress());
            const auto icrDuration =
                HePhy::ConvertLSigLengthToHeTbPpduDuration(trigger.GetUlLength(),
                                                           tbTxVector,
                                                           apPhy->GetPhyBand());
            const auto timeout =
                apPhy->GetSifs() + apPhy->GetSlot() + EMLSR_OR_DSO_RX_PHY_START_DELAY;
            const auto delay = icfDuration + apPhy->GetSifs() + icrDuration + timeout;
            ScheduleChecksSwitchBack(i, delay);
            const auto tbPpduTimeout = icfDuration + apPhy->GetSifs() + apPhy->GetSlot() +
                                       WifiPhy::CalculatePhyPreambleAndHeaderDuration(tbTxVector);
            const auto remainingIcrDurationAfterTbPpduTimeout =
                icrDuration - WifiPhy::CalculatePhyPreambleAndHeaderDuration(tbTxVector) -
                apPhy->GetSlot();
            ScheduleChecksBlockedDlTx(tbPpduTimeout,
                                      remainingIcrDurationAfterTbPpduTimeout + timeout);
        }
    }

    if (m_params.generateInterferenceAfterIcf)
    {
        // start interference right before the DSO STA is expected to switch to the DSO subband
        Simulator::Schedule(icfDuration - TimeStep(1),
                            &DsoTxopTest::StartInterference,
                            this,
                            m_duration);
        // increase power of STAs to have the AP able to latch on their signals despite the
        // interference
        for (std::size_t i = 0; i < m_nDsoStas; ++i)
        {
            auto staPhy = m_staMacs.at(i)->GetWifiPhy(SINGLE_LINK_OP_ID);
            staPhy->SetTxPowerStart(dBm_t{30});
            staPhy->SetTxPowerEnd(dBm_t{30});
        }
    }

    Simulator::Schedule(icfDuration - TimeStep(1), [=, this]() {
        // all DSO STAs should be operating on the primary subband before the reception of the ICF
        for (std::size_t i = 0; i < m_nDsoStas; ++i)
        {
            auto dsoManager = DynamicCast<TestDsoManager>(m_staMacs.at(i)->GetDsoManager());
            NS_TEST_EXPECT_MSG_EQ(
                m_staMacs.at(i)->GetWifiPhy(SINGLE_LINK_OP_ID)->GetOperatingChannel(),
                dsoManager->GetPrimarySubband(SINGLE_LINK_OP_ID),
                "PHY of the STA " << i + 1
                                  << " should be operating on its primary subband before ICF");
        }
    });

    if (m_params.switchingDelayToDso.IsStrictlyPositive())
    {
        Simulator::Schedule(icfDuration + TimeStep(1), [=, this]() {
            NS_LOG_DEBUG("Check STA " << m_idxStaInPrimarySubband + 1
                                      << " is not switching to the DSO subband");
            auto dsoManager = DynamicCast<TestDsoManager>(
                m_staMacs.at(m_idxStaInPrimarySubband)->GetDsoManager());
            auto phy = m_staMacs.at(m_idxStaInPrimarySubband)->GetWifiPhy(SINGLE_LINK_OP_ID);
            NS_TEST_EXPECT_MSG_EQ(
                ((!phy->IsStateSwitching()) &&
                 (phy->GetOperatingChannel() == dsoManager->GetPrimarySubband(SINGLE_LINK_OP_ID))),
                true,
                "PHY of STA " << (m_idxStaInPrimarySubband + 1)
                              << " should not be switching to the DSO subband after "
                                 "the reception of the ICF");

            dsoManager =
                DynamicCast<TestDsoManager>(m_staMacs.at(m_idxStaInDsoSubband)->GetDsoManager());
            phy = m_staMacs.at(m_idxStaInDsoSubband)->GetWifiPhy(SINGLE_LINK_OP_ID);
            const auto expectedSubband =
                m_params.corruptIcf
                    ? dsoManager->GetPrimarySubband(SINGLE_LINK_OP_ID)
                    : dsoManager->GetDsoSubbands(SINGLE_LINK_OP_ID).cbegin()->second;
            const auto isExpectedSwitching = !m_params.corruptIcf;
            NS_LOG_DEBUG("Check STA " << m_idxStaInDsoSubband + 1
                                      << (isExpectedSwitching ? " is switching to" : " stayed on")
                                      << " the expected subband");
            NS_TEST_EXPECT_MSG_EQ(
                ((phy->IsStateSwitching() == isExpectedSwitching) &&
                 (phy->GetOperatingChannel() == expectedSubband)),
                true,
                "PHY of STA " << (m_idxStaInDsoSubband + 1) << " should "
                              << (m_params.corruptIcf ? "not " : "")
                              << " be switching to the DSO subband after the reception of the ICF");
        });
    }

    // check that STAs are operating on the expected subband after the reception of the ICF
    Simulator::Schedule(icfDuration + m_params.switchingDelayToDso + TimeStep(1), [=, this]() {
        NS_LOG_DEBUG("Check STA " << m_idxStaInPrimarySubband + 1
                                  << " is still operating on the primary subband");
        auto dsoManager =
            DynamicCast<TestDsoManager>(m_staMacs.at(m_idxStaInPrimarySubband)->GetDsoManager());
        auto phy = m_staMacs.at(m_idxStaInPrimarySubband)->GetWifiPhy(SINGLE_LINK_OP_ID);
        NS_TEST_EXPECT_MSG_EQ(
            ((!phy->IsStateSwitching()) &&
             (phy->GetOperatingChannel() == dsoManager->GetPrimarySubband(SINGLE_LINK_OP_ID))),
            true,
            "PHY of STA " << (m_idxStaInPrimarySubband + 1)
                          << " should have stayed on the primary subband");

        NS_LOG_DEBUG("Check STA " << (m_idxStaInDsoSubband + 1)
                                  << " is operating on the expected subband");
        dsoManager =
            DynamicCast<TestDsoManager>(m_staMacs.at(m_idxStaInDsoSubband)->GetDsoManager());
        const auto expectedSubband =
            m_params.corruptIcf ? dsoManager->GetPrimarySubband(SINGLE_LINK_OP_ID)
                                : dsoManager->GetDsoSubbands(SINGLE_LINK_OP_ID).cbegin()->second;
        phy = m_staMacs.at(m_idxStaInDsoSubband)->GetWifiPhy(SINGLE_LINK_OP_ID);
        NS_TEST_EXPECT_MSG_EQ(
            ((!phy->IsStateSwitching()) && (phy->GetOperatingChannel() == expectedSubband)),
            true,
            "PHY of STA " << (m_idxStaInDsoSubband + 1) << " should "
                          << (m_params.corruptIcf ? "not " : "")
                          << "have switched to the DSO subband after the reception of the ICF");
    });

    if (m_params.numUlMuPpdus > 0)
    {
        for (std::size_t i = 0; i < m_nDsoStas; ++i)
        {
            m_staMacs.at(i)->GetDevice()->GetNode()->AddApplication(
                GetApplication(UPLINK, i, 1, 500));
        }
    }
}

void
DsoTxopTest::CheckQosNullFrames(const WifiConstPsduMap& psduMap, const WifiTxVector& txVector)
{
    NS_LOG_DEBUG("Send DSO ICF response");

    NS_TEST_EXPECT_MSG_EQ(txVector.GetHeMuUserInfoMap().cbegin()->second.nss,
                          1,
                          "number of spatial streams in DSO ICF response should be limited to 1");

    const auto ta = psduMap.cbegin()->second->GetAddr2();
    std::size_t clientId;
    if (m_staMacs.at(0)->GetLinkIdByAddress(ta))
    {
        clientId = 0;
    }
    else
    {
        NS_TEST_ASSERT_MSG_EQ(m_staMacs.at(1)->GetLinkIdByAddress(ta).has_value(),
                              true,
                              "Unexpected TA for QosNull: " << ta);
        clientId = 1;
    }

    const auto phy = m_staMacs.at(clientId)->GetWifiPhy(SINGLE_LINK_OP_ID);
    const auto timeout = phy->GetSifs() + phy->GetSlot() + EMLSR_OR_DSO_RX_PHY_START_DELAY;
    if ((m_countIcf == 1) && m_params.corruptedIcfResponses.contains(clientId))
    {
        NS_LOG_DEBUG("CORRUPTED");
        auto list = m_apErrorModel->GetList();
        list.push_back(psduMap.cbegin()->second->GetPayload(0)->GetUid());
        m_apErrorModel->SetList(list);

        const auto isTxopExpectedToTerminate =
            ((clientId == m_idxStaInPrimarySubband) ||
             (m_params.corruptedIcfResponses.size() == m_nDsoStas));
        if (isTxopExpectedToTerminate)
        {
            // check AP blocks transmission to all DSO STAs for the switch back delay after the
            // expected end of the ICF response plus timeout, and check DSO STAs are switching back
            // to their primary subband during that period
            const auto icfResponseDuration =
                WifiPhy::CalculateTxDuration(psduMap, txVector, phy->GetPhyBand());
            const auto delay = icfResponseDuration + timeout;
            ScheduleChecksSwitchBack(clientId, delay);
            const auto remainingIcrDurationAfterTbPpduTimeout =
                icfResponseDuration - WifiPhy::CalculatePhyPreambleAndHeaderDuration(txVector) -
                phy->GetSlot();
            ScheduleChecksBlockedDlTx(icfResponseDuration,
                                      timeout + remainingIcrDurationAfterTbPpduTimeout);
        }
    }

    if ((m_params.numDlMuPpdus == 0) && (m_params.numUlMuPpdus == 0))
    {
        // In this case, the DSO TXOP is directly terminated
        auto delay = GetDelayUntilTxopCompletion(clientId, psduMap, txVector);
        auto delayBeforeDlBlocked = delay;
        if (m_params
                .protectSingleExchange) // switch back immediately if TXOP is terminated by CF-END
        {
            NS_LOG_DEBUG(
                "Switch back to primary subband for STA "
                << clientId + 1
                << " is expected to be delayed by aSIFSTime + aSlotTime + aRxPHYStartDelay");
            delay += timeout;
            delayBeforeDlBlocked += phy->GetSifs(); // if no CF-END, the FEM is told the channel has
                                                    // been released a SIFS after the last frame
        }
        ScheduleChecksSwitchBack(clientId, delay);
        ScheduleChecksBlockedDlTx(delayBeforeDlBlocked, timeout);
    }
}

Time
DsoTxopTest::GetDelayUntilTxopCompletion(std::size_t clientId,
                                         const WifiConstPsduMap& psduMap,
                                         const WifiTxVector& txVector)
{
    auto phy = m_staMacs.at(clientId)->GetWifiPhy(SINGLE_LINK_OP_ID);
    const auto txDuration = WifiPhy::CalculateTxDuration(psduMap, txVector, phy->GetPhyBand());
    auto delay = txDuration;
    if (!m_params.protectSingleExchange)
    {
        const auto cfEndDuration = WifiPhy::CalculateTxDuration(
            Create<WifiPsdu>(Create<Packet>(), WifiMacHeader(WIFI_MAC_CTL_END)),
            m_apMac->GetWifiRemoteStationManager(SINGLE_LINK_OP_ID)
                ->GetRtsTxVector(Mac48Address::GetBroadcast(), txVector.GetChannelWidth()),
            phy->GetPhyBand());
        if (const auto remainingTxopDuration =
                m_apMac->GetQosTxop(AC_BE)->GetRemainingTxop(SINGLE_LINK_OP_ID);
            remainingTxopDuration - txDuration > cfEndDuration)
        {
            delay += phy->GetSifs() + cfEndDuration;
        }
        else
        {
            delay = remainingTxopDuration;
        }
    }
    return delay;
}

void
DsoTxopTest::ScheduleChecksSwitchBack(std::size_t clientId, const Time& delay)
{
    NS_LOG_FUNCTION(this << clientId << delay);
    // If the ICR of the DSO STA that operated on the DSO subband during the first DSO TXOP
    // was corrupted, it is expected to transmit on the primary subband in the next DSO TXOP
    // since there is no pending DL traffic to the other STA and hence another RU does not
    // need to be allocated.
    const auto isStaOperatingOnDsoSubband =
        (clientId == m_idxStaInDsoSubband) &&
        ((!m_params.corruptedIcfResponses.contains(clientId) && !m_params.corruptIcf) ||
         (m_countIcf > 1));

    if (isStaOperatingOnDsoSubband)
    {
        NS_TEST_EXPECT_MSG_NE(
            m_staMacs.at(clientId)->GetWifiPhy(SINGLE_LINK_OP_ID)->GetOperatingChannel(),
            DynamicCast<TestDsoManager>(m_staMacs.at(clientId)->GetDsoManager())
                ->GetPrimarySubband(SINGLE_LINK_OP_ID),
            "STA " << clientId + 1 << " should not be operating on its primary subband");
    }

    // Check that STAs are operating on the expected channel before the end of the switch back
    // delay and DL transmissions to these STAs are not blocked by AP yet.
    Simulator::Schedule(delay - TimeStep(1), [=, this]() {
        if (isStaOperatingOnDsoSubband)
        {
            CheckChannelSwitchingBack(clientId, SwitchBackStatus::NOT_SWITCHING);
        }
    });

    // Check that STAs are operating on the expected channel after the end of the switch back
    // delay and DL transmissions to these STAs are blocked by AP during the switch back delay.
    if (m_params.channelSwitchBackDelays.at(clientId).IsStrictlyPositive())
    {
        Simulator::Schedule(delay + MAX_PROPAGATION_DELAY, [=, this]() {
            if (isStaOperatingOnDsoSubband)
            {
                CheckChannelSwitchingBack(clientId, SwitchBackStatus::SWITCHING_BACK_TO_PRIMARY);
            }
        });
    }
    if (isStaOperatingOnDsoSubband)
    {
        Simulator::Schedule(
            delay + m_params.channelSwitchBackDelays.at(clientId) + TimeStep(1),
            [=, this]() {
                CheckChannelSwitchingBack(clientId, SwitchBackStatus::SWITCHED_BACK_TO_PRIMARY);
            });
    }
}

void
DsoTxopTest::CheckChannelSwitchingBack(std::size_t staId, SwitchBackStatus status)
{
    auto dsoManager = DynamicCast<TestDsoManager>(m_staMacs.at(staId)->GetDsoManager());
    auto phy = m_staMacs.at(staId)->GetWifiPhy(SINGLE_LINK_OP_ID);
    const auto operatingChannel = phy->GetOperatingChannel();
    const auto isPhySwitching = phy->IsStateSwitching();
    switch (status)
    {
    case SwitchBackStatus::NOT_SWITCHING:
        NS_LOG_DEBUG("Check STA " << staId + 1 << " is still operating on the DSO subband");
        NS_TEST_EXPECT_MSG_EQ(operatingChannel,
                              dsoManager->GetDsoSubbands(SINGLE_LINK_OP_ID).cbegin()->second,
                              "PHY of STA " << std::to_string(staId + 1)
                                            << " should be operating on the DSO "
                                               "subband before the switch back delay");
        break;
    case SwitchBackStatus::SWITCHING_BACK_TO_PRIMARY:
        NS_LOG_DEBUG("Check STA " << staId + 1 << " is switching back to the primary subband");
        NS_TEST_EXPECT_MSG_EQ(
            (isPhySwitching &&
             (operatingChannel == dsoManager->GetPrimarySubband(SINGLE_LINK_OP_ID))),
            true,
            "PHY of STA " << std::to_string(staId + 1)
                          << " should be switching back to the primary subband after the "
                             "switch back delay");
        break;
    case SwitchBackStatus::SWITCHED_BACK_TO_PRIMARY:
        NS_LOG_DEBUG("Check STA " << staId + 1 << " has switched back to the primary subband");
        NS_TEST_EXPECT_MSG_EQ(
            ((!isPhySwitching) &&
             (operatingChannel == dsoManager->GetPrimarySubband(SINGLE_LINK_OP_ID))),
            true,
            "PHY of STA "
                << std::to_string(staId + 1)
                << " should have switched back to the primary subband after the switch back delay");
        break;
    }
}

void
DsoTxopTest::ScheduleChecksBlockedDlTx(const Time& delay, const Time& timeout)
{
    NS_LOG_FUNCTION(this << delay << timeout);
    const auto maxSwitchBackDelay = *std::max_element(m_params.channelSwitchBackDelays.cbegin(),
                                                      m_params.channelSwitchBackDelays.cend());

    using enum WifiQueueBlockedReason;
    for (std::size_t i = 0; i < m_nDsoStas; ++i)
    {
        // check downlink transmissions are not blocked yet.
        Simulator::Schedule(delay - TimeStep(1), [=, this]() {
            CheckBlockedLink(m_apMac,
                             m_staMacs.at(i)->GetAddress(),
                             SINGLE_LINK_OP_ID,
                             WAITING_DSO_SWITCH_BACK_DELAY,
                             false);
        });

        // check downlink transmissions are blocked during the switch back delay at AP.
        if (m_params.channelSwitchBackDelays.at(i).IsStrictlyPositive())
        {
            Simulator::Schedule(delay + MAX_PROPAGATION_DELAY, [=, this]() {
                CheckBlockedLink(m_apMac,
                                 m_staMacs.at(i)->GetAddress(),
                                 SINGLE_LINK_OP_ID,
                                 WAITING_DSO_SWITCH_BACK_DELAY,
                                 true);
            });
        }
        Simulator::Schedule(delay + timeout + maxSwitchBackDelay + MAX_PROPAGATION_DELAY,
                            [=, this]() {
                                CheckBlockedLink(m_apMac,
                                                 m_staMacs.at(i)->GetAddress(),
                                                 SINGLE_LINK_OP_ID,
                                                 WAITING_DSO_SWITCH_BACK_DELAY,
                                                 false);
                            });
    }
}

void
DsoTxopTest::CheckBlockedLink(Ptr<WifiMac> mac,
                              const Mac48Address& dest,
                              uint8_t linkId,
                              WifiQueueBlockedReason reason,
                              bool blocked)
{
    NS_LOG_DEBUG("Check " << ((mac == m_apMac) ? "DL" : "UL") << " transmissions to " << dest
                          << " for link ID " << +linkId << " are "
                          << (blocked ? "blocked" : "unblocked"));
    WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WifiRcvAddr::UNICAST, dest, 0);
    auto mask = mac->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, linkId);
    NS_TEST_EXPECT_MSG_EQ(mask.has_value(), true, "Expected to find a mask for link " << +linkId);
    NS_TEST_EXPECT_MSG_EQ(mask->test(static_cast<std::size_t>(reason)),
                          blocked,
                          "Expected link " << +linkId << " to be "
                                           << (blocked ? "blocked" : "unblocked") << " for reason "
                                           << reason);
}

void
DsoTxopTest::CheckQosFrames(const WifiConstPsduMap& psduMap, const WifiTxVector& txVector)
{
    ++m_countQoSframes;
    NS_LOG_DEBUG("Send QoS DATA frame #" << m_countQoSframes);

    if (m_countQoSframes > m_params.numDlMuPpdus)
    {
        return;
    }

    NS_TEST_EXPECT_MSG_EQ(txVector.IsDlMu(), true, "DL MU PPDU should be sent by the AP");

    NS_TEST_EXPECT_MSG_EQ(txVector.GetChannelWidth(),
                          MHz_t{160},
                          "DL MU PPDU sent with an unexpected width");

    const auto& userInfoMap = txVector.GetHeMuUserInfoMap();

    // If the ICR of a DSO STA allocated to a RU in the DSO subchannel is not received whereas
    // the ICR of the DSO STA allocated to a RU in the primary subchannel is received, the
    // DSO TXOP is continued but DL MU PPDU(s) won't contain any PSDU for the DSO STA operating
    // in the DSO subchannel.
    const auto isDsoContinuedIfIcrNotReceived =
        (m_params.corruptedIcfResponses.contains(m_idxStaInDsoSubband) &&
         !m_params.corruptedIcfResponses.contains(m_idxStaInPrimarySubband));

    NS_TEST_ASSERT_MSG_EQ(userInfoMap.size(),
                          (isDsoContinuedIfIcrNotReceived ? 1 : 2),
                          "Incorrect number of users addressed by DL MU PPDU");

    const auto ruStaPrimary = std::get<EhtRu::RuSpec>(
        userInfoMap.at(m_staMacs.at(m_idxStaInPrimarySubband)->GetAssociationId()).ru);
    NS_TEST_EXPECT_MSG_EQ(ruStaPrimary.GetPrimary160MHz() &&
                              ruStaPrimary.GetPrimary80MHzOrLower80MHz(),
                          true,
                          "PSDU for STA " << m_idxStaInPrimarySubband + 1
                                          << " should be sent over the primary 80 MHz");

    if (!isDsoContinuedIfIcrNotReceived)
    {
        const auto ruStaDso = std::get<EhtRu::RuSpec>(
            userInfoMap.at(m_staMacs.at(m_idxStaInDsoSubband)->GetAssociationId()).ru);
        NS_TEST_EXPECT_MSG_EQ(
            (ruStaDso.GetPrimary160MHz() && !ruStaDso.GetPrimary80MHzOrLower80MHz()),
            true,
            "PSDU for STA " << (m_idxStaInDsoSubband + 1)
                            << " should be sent over the secondary 80 MHz");
    }

    if (m_params.generateInterferenceAfterIcf)
    {
        const auto txDuration =
            WifiPhy::CalculateTxDuration(psduMap,
                                         txVector,
                                         m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetPhyBand());
        for (std::size_t i = 0; i < m_nDsoStas; ++i)
        {
            ScheduleChecksSwitchBack(i, txDuration);
        }
    }
}

void
DsoTxopTest::CheckBlockAck(const WifiConstPsduMap& psduMap, const WifiTxVector& txVector)
{
    ++m_countBlockAck;
    NS_LOG_DEBUG("Send Block ACK #" << m_countBlockAck);

    const auto ta = psduMap.cbegin()->second->GetAddr2();
    std::optional<std::size_t> clientId;
    if (m_apMac->GetLinkIdByAddress(ta))
    {
        // nothing to do for AP
    }
    else if (m_staMacs.at(0)->GetLinkIdByAddress(ta))
    {
        clientId = 0;
    }
    else
    {
        NS_TEST_ASSERT_MSG_EQ(m_staMacs.at(1)->GetLinkIdByAddress(ta).has_value(),
                              true,
                              "Unexpected TA for BlockAck: " << ta);
        clientId = 1;
    }

    const auto txDuration =
        WifiPhy::CalculateTxDuration(psduMap,
                                     txVector,
                                     m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetPhyBand());
    if (m_params.generateObssDuringDsoTxop && (m_countBlockAck == 1))
    {
        Simulator::Schedule(txDuration + TimeStep(1), &DsoTxopTest::GenerateObssFrame, this);
    }

    auto muScheduler =
        DynamicCast<TestDsoMultiUserScheduler>(m_apMac->GetObject<MultiUserScheduler>());

    if (m_params.numDlMuPpdus > m_countQoSframes)
    {
        NS_LOG_DEBUG("Generate one more packet for STA " << *clientId + 1);
        m_apMac->GetDevice()->GetNode()->AddApplication(
            GetApplication(DOWNLINK, *clientId, 1, 1000));
    }
    else if ((m_nDsoStas * m_params.numUlMuPpdus) > m_countQoSframes)
    {
        for (std::size_t i = 0; i < m_nDsoStas; ++i)
        {
            NS_LOG_DEBUG("Generate one more packet for STA " << i + 1);
            m_staMacs.at(i)->GetDevice()->GetNode()->AddApplication(
                GetApplication(UPLINK, i, 1, 500));
        }
    }
    else if (m_params.numDlMuPpdus == m_countQoSframes)
    {
        if (!m_params.generateInterferenceAfterIcf && !m_params.generateObssDuringDsoTxop)
        {
            NS_TEST_ASSERT_MSG_EQ(clientId.has_value(),
                                  true,
                                  "Expected to find a client ID for BlockAck: " << ta);
            const auto phy = m_staMacs.at(*clientId)->GetWifiPhy(SINGLE_LINK_OP_ID);
            const auto timeout = phy->GetSifs() + phy->GetSlot() + EMLSR_OR_DSO_RX_PHY_START_DELAY;
            NS_LOG_DEBUG("DSO frame exchange(s) completed, channel shall switch back for STA "
                         << *clientId + 1);
            auto delay = GetDelayUntilTxopCompletion(*clientId, psduMap, txVector);
            const auto cfEndDuration = WifiPhy::CalculateTxDuration(
                Create<WifiPsdu>(Create<Packet>(), WifiMacHeader(WIFI_MAC_CTL_END)),
                m_apMac->GetWifiRemoteStationManager(SINGLE_LINK_OP_ID)
                    ->GetRtsTxVector(Mac48Address::GetBroadcast(), txVector.GetChannelWidth()),
                phy->GetPhyBand());
            auto delayBeforeDlBlocked = txDuration + phy->GetSifs() + cfEndDuration;
            if (delay - txDuration <
                cfEndDuration) // switch back immediately if TXOP is terminated by CF-END
            {
                NS_LOG_DEBUG(
                    "Switch back to primary subband for STA "
                    << *clientId + 1
                    << " is expected to be delayed by aSIFSTime + aSlotTime + aRxPHYStartDelay");
                const auto phy = m_staMacs.at(*clientId)->GetWifiPhy(SINGLE_LINK_OP_ID);
                delay = txDuration + timeout;
                delayBeforeDlBlocked =
                    txDuration + phy->GetSifs(); // if no CF-END, the FEM is told the channel has
                                                 // been released a SIFS after the last frame
            }
            ScheduleChecksSwitchBack(*clientId, delay);
            ScheduleChecksBlockedDlTx(delayBeforeDlBlocked,
                                      timeout); // the channel has been released is notified to the
                                                // FEM a SIFS after the last frame
        }
    }
    else if ((m_nDsoStas * m_params.numUlMuPpdus) == m_countQoSframes)
    {
        for (std::size_t i = 0; i < m_nDsoStas; ++i)
        {
            NS_LOG_DEBUG("DSO frame exchange(s) completed, channel shall switch back for STA "
                         << i + 1);
            const auto phy = m_staMacs.at(i)->GetWifiPhy(SINGLE_LINK_OP_ID);
            const auto timeout = phy->GetSifs() + phy->GetSlot() + EMLSR_OR_DSO_RX_PHY_START_DELAY;
            const auto delay = GetDelayUntilTxopCompletion(i, psduMap, txVector);
            ScheduleChecksSwitchBack(i, delay);
            ScheduleChecksBlockedDlTx(delay, timeout);

            // schedule one last packet to be transmitted in a following TXOP (non-DSO)
            NS_LOG_DEBUG("Generate one last packet for STA " << i + 1);
            Simulator::Schedule(txDuration + phy->GetSifs(), [this, i]() {
                m_apMac->GetDevice()->GetNode()->AddApplication(
                    GetApplication(DOWNLINK, i, 1, 1000));
            });
        }
        if (!m_params.nextTxopIsDso)
        {
            muScheduler->SetForcedFormat(MultiUserScheduler::TxFormat::SU_TX);
        }
        muScheduler->SetAccessReqInterval(Time());
    }

    if (m_countBlockAck == (m_nDsoStas * m_params.numDlMuPpdus))
    {
        // all STAs have sent all their Block ACKs, schedule one last packet per STA for next TXOP
        for (std::size_t i = 0; i < m_nDsoStas; ++i)
        {
            if (const auto mpdu =
                    m_apMac->GetQosTxop(AC_BE)->GetWifiMacQueue()->PeekByTidAndAddress(
                        0,
                        m_staMacs.at(i)->GetAddress());
                !mpdu || mpdu->IsInFlight()) // otherwise it means there is already one packet
                                             // enqueued and hence it is not needed to schedule more
            {
                NS_LOG_DEBUG("Generate one last packet for STA " << i + 1);
                Simulator::Schedule(
                    txDuration + m_staMacs.at(*clientId)->GetWifiPhy(SINGLE_LINK_OP_ID)->GetSifs(),
                    [this, i]() {
                        m_apMac->GetDevice()->GetNode()->AddApplication(
                            GetApplication(DOWNLINK, i, 1, 1000));
                    });
            }
        }
        if (!m_params.nextTxopIsDso)
        {
            muScheduler->SetForcedFormat(MultiUserScheduler::TxFormat::SU_TX);
        }
    }
}

void
DsoTxopTest::DsoTxopEventCallback(std::size_t index, DsoTxopEvent event, uint8_t /*linkId*/)
{
    NS_LOG_DEBUG("DSO TXOP event for STA " << index + 1 << ": " << event);
    const auto& [it, inserted] = m_dsoTxopEventInfos.emplace(index, std::vector<DsoTxopEvent>{});
    it->second.emplace_back(event);

    if ((event == DsoTxopEvent::RX_ICF) && (m_params.numDlMuPpdus == 0) &&
        (m_params.numUlMuPpdus == 0))
    {
        // scenario where no DL nor UL MU PPDU should be transmitted, force the packet enqueued to
        // be dropped because of lifetime expiry
        m_apMac->GetQosTxop(AC_BE)->GetWifiMacQueue()->SetAttribute("MaxDelay",
                                                                    TimeValue(TimeStep(1)));
        m_apMac->GetQosTxop(AC_BE)->GetWifiMacQueue()->Flush();
    }
}

void
DsoTxopTest::CheckResults()
{
    NS_TEST_ASSERT_MSG_EQ(m_txPsdus.empty(), false, "No frame exchanged during the test");

    auto psduIt = m_txPsdus.cbegin();

    std::size_t numDsoTxops = m_params.nextTxopIsDso ? 2 : 1;
    // If the ICF is corrupted or if there are corrupted ICF responses, another
    // DSO TXOP is expected to be initiated by the AP.
    if (!m_params.corruptedIcfResponses.empty() || m_params.corruptIcf)
    {
        ++numDsoTxops;
    }

    const auto isDsoContinuedIfIcrNotReceived =
        (m_params.corruptedIcfResponses.contains(m_idxStaInDsoSubband) &&
         !m_params.corruptedIcfResponses.contains(m_idxStaInPrimarySubband));

    for (std::size_t dsoTxopId = 0; dsoTxopId < numDsoTxops; ++dsoTxopId)
    {
        // Check the DSO ICF sent by the AP
        auto psdu = psduIt->psduMap.cbegin()->second;
        NS_TEST_ASSERT_MSG_EQ(psdu->GetHeader(0).IsTrigger(),
                              true,
                              "The first frame of a DSO TXOP should be a trigger frame (ICF)");

        CtrlTriggerHeader trigger;
        psdu->GetPayload(0)->PeekHeader(trigger);
        NS_TEST_ASSERT_MSG_EQ(trigger.IsBsrp(), true, "DSO ICF should be a BSRP trigger frame");

        NS_TEST_EXPECT_MSG_EQ(psduIt->txVector.IsNonHtDuplicate(),
                              true,
                              "DSO ICF should be sent using non-HT duplicate");

        const auto icfTxEnd =
            psduIt->startTx +
            WifiPhy::CalculateTxDuration(psduIt->psduMap,
                                         psduIt->txVector,
                                         m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetPhyBand());

        ++psduIt;

        if ((dsoTxopId == 0) && m_params.corruptIcf)
        {
            // First ICF is corrupted, so no ICRs are expected to be sent
            continue;
        }

        // Check ICR responses to DSO ICF sent by the non-AP STAs
        std::size_t qosNullCount{0};
        Time endPrevious;
        for (std::size_t i = 0; i < m_nDsoStas; ++i)
        {
            if ((dsoTxopId == 1) && isDsoContinuedIfIcrNotReceived &&
                !m_params.corruptedIcfResponses.contains(i))
            {
                // STA was served in the previous DSO TXOP
                continue;
            }
            const auto& hdr = psduIt->psduMap.cbegin()->second->GetHeader(0);
            if (hdr.IsQosData() && !hdr.HasData())
            {
                ++qosNullCount;
            }
            NS_TEST_EXPECT_MSG_EQ(psduIt->txVector.GetPreambleType(),
                                  WIFI_PREAMBLE_UHR_TB,
                                  "ICR should be sent as a UHR TB PPDU");
            NS_TEST_EXPECT_MSG_EQ(icfTxEnd + m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetSifs(),
                                  psduIt->startTx,
                                  "Expected the ICR to start a SIFS after the ICF");

            endPrevious =
                psduIt->startTx +
                WifiPhy::CalculateTxDuration(psduIt->psduMap,
                                             psduIt->txVector,
                                             m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetPhyBand());

            ++psduIt;
        }

        // If it is the second DSO TXOP following a DSO TXOP with corrupted ICF responses, only
        // the STAs that previous had no DL MU PPDU(s) are expected to be addressed in this DSO TXOP
        // since there is no more downlink traffic pending for the other STAs
        const auto expectedNumIcfResponses = ((dsoTxopId == 0) || !isDsoContinuedIfIcrNotReceived)
                                                 ? m_nDsoStas
                                                 : m_params.corruptedIcfResponses.size();
        NS_TEST_EXPECT_MSG_EQ(qosNullCount,
                              expectedNumIcfResponses,
                              "Unexpected number of QoS Null frames (ICR)");

        if ((m_params.numDlMuPpdus == 0) && (m_params.numUlMuPpdus == 0))
        {
            return;
        }

        if ((dsoTxopId == 0) && m_params.corruptedIcfResponses.contains(m_idxStaInPrimarySubband))
        {
            // If the ICF response of the STA operating on the primary subband is not received,
            // the DSO TXOP is expected to be terminated.
            continue;
        }

        for (std::size_t i = 0; i < ((dsoTxopId == 0) ? m_params.numDlMuPpdus : 1); ++i)
        {
            // Check the DL MU PPDU sent by the AP
            NS_TEST_EXPECT_MSG_EQ(psduIt->txVector.IsDlMu(),
                                  true,
                                  "A DL MU PPDU should be sent by the AP");
            const std::size_t numUsersIfIcrNotReceived =
                ((dsoTxopId == 0) ? (m_nDsoStas - m_params.corruptedIcfResponses.size())
                                  : m_params.corruptedIcfResponses.size());
            NS_TEST_ASSERT_MSG_EQ(
                psduIt->psduMap.size(),
                (isDsoContinuedIfIcrNotReceived ? numUsersIfIcrNotReceived : m_nDsoStas),
                "Unexpected number of STAs addressed by the DL MU PPDU");
            NS_TEST_ASSERT_MSG_EQ_TOL(
                endPrevious + m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetSifs(),
                psduIt->startTx,
                MAX_PROPAGATION_DELAY,
                "Expected the MU PPDU to start a SIFS after the previous frame");

            endPrevious =
                psduIt->startTx +
                WifiPhy::CalculateTxDuration(psduIt->psduMap,
                                             psduIt->txVector,
                                             m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetPhyBand());

            // Check Block ACK responses sent by the non-AP STAs
            std::size_t blockAckCount{0};
            for (std::size_t j = 0; j < m_nDsoStas; ++j)
            {
                if ((dsoTxopId == 0) && m_params.corruptedIcfResponses.contains(j))
                {
                    // this STA is not sollicited
                    continue;
                }
                if ((dsoTxopId == 1) && isDsoContinuedIfIcrNotReceived &&
                    !m_params.corruptedIcfResponses.contains(j))
                {
                    // this STA was served in the previous DSO TXOP
                    continue;
                }
                if ((i == 0) && (j == 1) && (m_params.generateInterferenceAfterIcf > 0))
                {
                    // the second STA is not expected to send a Block ACK response
                    // because it did not receive the DL MU PPDU due to interference
                    continue;
                }
                if ((i == 1) && (j == 1) && (m_params.generateObssDuringDsoTxop > 0))
                {
                    // the second STA is not expected to send a Block ACK response
                    // because it latched on the OBSS frame instead of the DL MU PPDU
                    continue;
                }
                ++psduIt;
                if (psduIt->psduMap.cbegin()->second->GetHeader(0).IsBlockAck())
                {
                    ++blockAckCount;
                }
                NS_TEST_EXPECT_MSG_EQ(psduIt->txVector.GetPreambleType(),
                                      WIFI_PREAMBLE_UHR_TB,
                                      "Block ACK should be sent as a UHR TB PPDU");
                NS_TEST_EXPECT_MSG_EQ(
                    endPrevious + m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetSifs(),
                    psduIt->startTx,
                    "Expected the Block ACK to start a SIFS after the DL MU PPDU");
            }
            const auto expectedNumBlockAcks =
                ((i == 0) && m_params.generateInterferenceAfterIcf) ||
                        ((i == 1) && m_params.generateObssDuringDsoTxop)
                    ? (m_nDsoStas - 1)
                    : (isDsoContinuedIfIcrNotReceived ? numUsersIfIcrNotReceived : m_nDsoStas);
            NS_TEST_EXPECT_MSG_EQ(blockAckCount,
                                  expectedNumBlockAcks,
                                  "Unexpected number of Block ACK responses");
            endPrevious =
                psduIt->startTx +
                WifiPhy::CalculateTxDuration(psduIt->psduMap,
                                             psduIt->txVector,
                                             m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetPhyBand());
            ++psduIt;
        }

        for (std::size_t i = 0; i < m_params.numUlMuPpdus; ++i)
        {
            // Check the Basic Trigger sent by the AP
            CtrlTriggerHeader trigger;
            psduIt->psduMap.cbegin()->second->GetPayload(0)->PeekHeader(trigger);
            NS_TEST_EXPECT_MSG_EQ(psduIt->psduMap.cbegin()->second->GetHeader(0).IsTrigger() &&
                                      trigger.IsBasic(),
                                  true,
                                  "A Basic Trigger frame should be sent by the AP");

            // Check the UL MU PPDUs sent by the non-AP STAs
            for (std::size_t j = 0; j < m_nDsoStas; ++j)
            {
                ++psduIt;
                NS_TEST_EXPECT_MSG_EQ(psduIt->txVector.IsUlMu(),
                                      true,
                                      "An UL MU PPDU should be sent by the non-AP STAs");
            }

            endPrevious =
                psduIt->startTx +
                WifiPhy::CalculateTxDuration(psduIt->psduMap,
                                             psduIt->txVector,
                                             m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetPhyBand());

            // Check the Block ACK sent by the AP
            ++psduIt;
            NS_TEST_EXPECT_MSG_EQ(psduIt->psduMap.cbegin()->second->GetHeader(0).IsBlockAck(),
                                  true,
                                  "A Block ACK frame should be sent by the AP");
            NS_TEST_EXPECT_MSG_EQ(endPrevious + m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetSifs(),
                                  psduIt->startTx,
                                  "Expected the Block ACK to start a SIFS after the UL MU PPDU");
            endPrevious =
                psduIt->startTx +
                WifiPhy::CalculateTxDuration(psduIt->psduMap,
                                             psduIt->txVector,
                                             m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetPhyBand());

            ++psduIt;
        }

        if (m_params.generateInterferenceAfterIcf || m_params.generateObssDuringDsoTxop)
        {
            do
            {
                NS_TEST_ASSERT_MSG_EQ(
                    (psduIt->psduMap.cbegin()->second->GetHeader(0).IsBlockAckReq() &&
                     (psduIt->psduMap.cbegin()->second->GetHeader(0).GetAddr2() ==
                      m_apMac->GetAddress()) &&
                     (psduIt->psduMap.cbegin()->second->GetHeader(0).GetAddr1() ==
                      m_staMacs.at(m_idxStaInDsoSubband)->GetAddress()) &&
                     psduIt->psduMap.cbegin()->second->GetHeader(0).IsRetry()),
                    true,
                    "BARs are expected to be transmitted to the DSO STA while it is switching back "
                    "to its primary subband during the first BAR");
                endPrevious =
                    psduIt->startTx + WifiPhy::CalculateTxDuration(
                                          psduIt->psduMap,
                                          psduIt->txVector,
                                          m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetPhyBand());
                ++psduIt;
            } while (psduIt != m_txPsdus.cend() &&
                     psduIt->psduMap.cbegin()->second->GetHeader(0).IsBlockAckReq());
            NS_TEST_EXPECT_MSG_EQ(
                psduIt->psduMap.cbegin()->second->GetHeader(0).IsBlockAck(),
                true,
                "A Block ACK frame should be sent by the DSO STA as a response to the BAR");
            NS_TEST_EXPECT_MSG_EQ(endPrevious + m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetSifs(),
                                  psduIt->startTx,
                                  "Expected the Block ACK to start a SIFS after the BAR");
            ++psduIt;

            const auto& hdr = psduIt->psduMap.cbegin()->second->GetHeader(0);
            NS_TEST_EXPECT_MSG_EQ(hdr.IsQosData() && hdr.IsRetry() &&
                                      (hdr.GetAddr2() == m_apMac->GetAddress()),
                                  true,
                                  "Expected the QoS DATA frame to be retransmitted by the AP");
            const auto recipient = hdr.GetAddr1();
            ++psduIt;

            NS_TEST_EXPECT_MSG_EQ((psduIt->psduMap.cbegin()->second->GetHeader(0).IsAck() &&
                                   (recipient == m_staMacs.at(m_idxStaInDsoSubband)->GetAddress())),
                                  true,
                                  "Expected a ACK response sent by the DSO STA");
            endPrevious =
                psduIt->startTx +
                WifiPhy::CalculateTxDuration(psduIt->psduMap,
                                             psduIt->txVector,
                                             m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetPhyBand());
            ++psduIt;
        }

        if (const auto numRemainingPsdus = std::distance(psduIt, m_txPsdus.cend());
            !m_params.protectSingleExchange && (numRemainingPsdus >= ((numDsoTxops == 1) ? 6 : 8)))
        {
            // Check the CF-END sent by the AP
            NS_TEST_EXPECT_MSG_EQ((psduIt->psduMap.cbegin()->second->GetHeader(0).IsCfEnd() &&
                                   (psduIt->psduMap.cbegin()->second->GetHeader(0).GetAddr2() ==
                                    m_apMac->GetAddress())),
                                  true,
                                  "The last frame of the DSO TXOP is expected to be a CF-END");
            NS_TEST_EXPECT_MSG_EQ(endPrevious + m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetSifs(),
                                  psduIt->startTx,
                                  "Expected the CF-END to start a SIFS after the Block ACK");
            endPrevious =
                psduIt->startTx +
                WifiPhy::CalculateTxDuration(psduIt->psduMap,
                                             psduIt->txVector,
                                             m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetPhyBand());
            ++psduIt;
        }
    }

    // checks for non-DSO TXOP(s) following the DSO TXOP(s)
    if (!m_params.nextTxopIsDso)
    {
        for (std::size_t i = 0; i < m_nDsoStas; ++i)
        {
            NS_TEST_EXPECT_MSG_EQ((psduIt->psduMap.cbegin()->second->GetHeader(0).IsQosData() &&
                                   (psduIt->psduMap.cbegin()->second->GetHeader(0).GetAddr2() ==
                                    m_apMac->GetAddress())),
                                  true,
                                  "Expected a QoS DATA frame sent by the AP");
            const auto recipient = psduIt->psduMap.cbegin()->second->GetHeader(0).GetAddr1();
            ++psduIt;

            NS_TEST_EXPECT_MSG_EQ((psduIt->psduMap.cbegin()->second->GetHeader(0).IsAck() &&
                                   (recipient == m_staMacs.at(i)->GetAddress())),
                                  true,
                                  "Expected a ACK response sent by STA " << i + 1);
            ++psduIt;
        }

        if (psduIt != m_txPsdus.cend())
        {
            // Check the CF-END sent by the AP
            NS_TEST_EXPECT_MSG_EQ((psduIt->psduMap.cbegin()->second->GetHeader(0).IsCfEnd() &&
                                   (psduIt->psduMap.cbegin()->second->GetHeader(0).GetAddr2() ==
                                    m_apMac->GetAddress())),
                                  true,
                                  "The last frame of the TXOP is expected to be a CF-END");

            // per STA, we expect numDlMuPpdus or numUlMuPpdus during DSO TXOP and 1 packet in a
            // following (non-DSO) TXOP.
            const auto expectedRxPackets =
                (m_params.numDlMuPpdus + m_params.numUlMuPpdus + 1) * m_nDsoStas;
            NS_TEST_EXPECT_MSG_EQ(m_receivedPackets,
                                  expectedRxPackets,
                                  "Unexpected amount of received packets");
        }
    }

    // check DSO TXOP events
    for (std::size_t i = 0; i < m_nDsoStas; ++i)
    {
        const auto numDsoTxopsWithEvents =
            m_params.corruptIcf ||
                    (isDsoContinuedIfIcrNotReceived && !m_params.corruptedIcfResponses.contains(i))
                ? 1
                : numDsoTxops;
        const auto it = m_dsoTxopEventInfos.find(i);
        NS_TEST_ASSERT_MSG_EQ((it != m_dsoTxopEventInfos.cend()),
                              true,
                              "No DSO TXOP termination event for STA " << i + 1);
        // per DSO TXOP and per STA, 3 events are expected: reception of ICF, transmission of ICR
        // and TXOP termination
        NS_TEST_ASSERT_MSG_EQ(it->second.size(),
                              3 * numDsoTxopsWithEvents,
                              "Unexpected number of DSO TXOP events for STA " << i + 1);
        NS_TEST_EXPECT_MSG_EQ(it->second.at(0),
                              DsoTxopEvent::RX_ICF,
                              "DSO ICF reception event has not been "
                              "received for STA "
                                  << i + 1);
        NS_TEST_EXPECT_MSG_EQ(it->second.at(1),
                              DsoTxopEvent::TX_ICR,
                              "DSO ICR transmission event has not been "
                              "received for STA "
                                  << i + 1);
        const auto expectedTxopEndEvent = (i == m_idxStaInDsoSubband)
                                              ? m_params.expectedTxopEndEventInDsoSubband
                                              : DsoTxopEvent::TIMEOUT;
        NS_TEST_EXPECT_MSG_EQ(it->second.at(2),
                              expectedTxopEndEvent,
                              "Unexpected DSO TXOP termination event for STA " << i + 1);
    }
}

void
DsoTxopTest::DoSetup()
{
    Config::SetDefault("ns3::QosFrameExchangeManager::ProtectSingleExchange",
                       BooleanValue(m_params.protectSingleExchange));
    Config::SetDefault("ns3::EhtFrameExchangeManager::EarlyTxopEndDetect", BooleanValue(false));

    DsoTestBase::DoSetup();

    for (std::size_t i = 0; i < m_nDsoStas; ++i)
    {
        auto staMac = m_staMacs.at(i);
        auto dsoManager = staMac->GetDsoManager();
        dsoManager->TraceConnectWithoutContext(
            "DsoTxopEvent",
            MakeCallback(&DsoTxopTest::DsoTxopEventCallback, this).Bind(i));
    }

    auto obssNode = CreateObject<Node>();
    auto obssDev = CreateObject<WifiNetDevice>();
    m_obssPhy = CreateObjectWithAttributes<SpectrumWifiPhy>("ChannelSwitchDelay",
                                                            TimeValue(Time(0)),
                                                            "TxMaskInnerBandMinimumRejection",
                                                            DoubleValue(-100.0),
                                                            "TxMaskOuterBandMinimumRejection",
                                                            DoubleValue(-100.0),
                                                            "TxMaskOuterBandMaximumRejection",
                                                            DoubleValue(-100.0));
    auto interferenceHelper = CreateObject<InterferenceHelper>();
    m_obssPhy->SetInterferenceHelper(interferenceHelper);
    auto error = CreateObject<NistErrorRateModel>();
    m_obssPhy->SetErrorRateModel(error);
    m_obssPhy->SetDevice(obssDev);
    m_obssPhy->AddChannel(m_channel);
    m_obssPhy->ConfigureStandard(WIFI_STANDARD_80211be);
    obssDev->SetStandard(WIFI_STANDARD_80211be);
    obssDev->SetPhy(m_obssPhy);
    obssNode->AddDevice(obssDev);
    auto txEhtConfiguration = CreateObject<EhtConfiguration>();
    obssDev->SetEhtConfiguration(txEhtConfiguration);

    auto interfererNode = CreateObject<Node>();
    auto interfererDev = CreateObject<NonCommunicatingNetDevice>();
    m_interferer = CreateObject<WaveformGenerator>();
    m_interferer->SetDevice(interfererDev);
    m_interferer->SetChannel(m_channel);
    m_interferer->SetDutyCycle(1);
    interfererNode->AddDevice(interfererDev);

    m_apErrorModel = CreateObject<ListErrorModel>();
    m_apMac->GetDevice()->GetPhy(SINGLE_LINK_OP_ID)->SetPostReceptionErrorModel(m_apErrorModel);

    m_apMac->GetQosTxop(AC_BE)->SetTxopLimit(MicroSeconds(1600));
    for (std::size_t i = 0; i < m_nDsoStas; ++i)
    {
        NS_ASSERT_MSG(i < m_params.channelSwitchBackDelays.size(),
                      "Not enough switch back delay values provided");
        m_staMacs.at(i)
            ->GetWifiPhy(SINGLE_LINK_OP_ID)
            ->SetAttribute("ChannelSwitchDelay", TimeValue(m_params.channelSwitchBackDelays.at(i)));
        auto dsoManager = m_staMacs.at(i)->GetDsoManager();
        dsoManager->SetAttribute("DsoSwitchBackDelay",
                                 TimeValue(m_params.channelSwitchBackDelays.at(i)));
        dsoManager->SetAttribute("DsoPaddingDelay", TimeValue(m_params.paddingDelays.at(i)));
        dsoManager->SetAttribute("ChSwitchToDsoBandDelay", TimeValue(m_params.switchingDelayToDso));

        auto errorModel = CreateObject<ListErrorModel>();
        m_staMacs.at(i)
            ->GetDevice()
            ->GetPhy(SINGLE_LINK_OP_ID)
            ->SetPostReceptionErrorModel(errorModel);
        m_staErrorModels.push_back(errorModel);
    }
}

void
DsoTxopTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    CheckResults();

    Simulator::Destroy();
}

DsoSchedulerTest::DsoSchedulerTest(const Params& params)
    : DsoTestBase(params.testName),
      m_params{params},
      m_enableUlOfdma{std::any_of(m_params.numGeneratedUlPackets.cbegin(),
                                  m_params.numGeneratedUlPackets.cend(),
                                  [](const auto& num) { return num > 0; })}
{
    m_nDsoStas = params.dsoStasOpChannel.size();
    m_apOpChannel = params.apOpChannel;
    m_stasOpChannel = params.dsoStasOpChannel;
}

void
DsoSchedulerTest::StartTraffic()
{
    NS_LOG_FUNCTION(this);

    DsoTestBase::StartTraffic();

    NS_ASSERT_MSG(m_params.numGeneratedDlPackets.size() ==
                      (m_nDsoStas + m_nNonDsoStas + m_nNonUhrStas),
                  "Number of DL packets to generate per STA is not set correctly");
    NS_ASSERT_MSG(m_params.numGeneratedUlPackets.size() ==
                      (m_nDsoStas + m_nNonDsoStas + m_nNonUhrStas),
                  "Number of UL packets to generate per STA is not set correctly");

    for (std::size_t i = 0; i < (m_nDsoStas + m_nNonDsoStas + m_nNonUhrStas); ++i)
    {
        if (m_params.numGeneratedDlPackets.at(i) > 0)
        {
            NS_LOG_DEBUG("Generate one DL packet for STA " << i + 1);
            m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(DOWNLINK, i, 1, 1000));
            m_dlTxPackets.push_back(1);
        }
        else
        {
            m_dlTxPackets.push_back(0);
        }
    }

    if (m_enableUlOfdma)
    {
        auto muScheduler =
            DynamicCast<TestDsoMultiUserScheduler>(m_apMac->GetObject<MultiUserScheduler>());
        // Keep polling interval short to ensure DSO ICF is sent as soon as possible after the
        // previous TXOP to trigger UL OFDMA transmissions, since this test is not interested in
        // SU UL transmissions. Hence, interval has to be greater than AIFSN * slottime, where AIFSN
        // has been set to 15 in DoSetup().
        muScheduler->SetAccessReqInterval(MicroSeconds(100));
    }
}

void
DsoSchedulerTest::Transmit(WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW)
{
    DsoTestBase::Transmit(psduMap, txVector, txPowerW);

    switch (auto psdu = psduMap.begin()->second; psdu->GetHeader(0).GetType())
    {
    case WIFI_MAC_CTL_TRIGGER: {
        Simulator::Schedule(TimeStep(1), [this]() {
            for (std::size_t i = 0; i < (m_nDsoStas + m_nNonDsoStas + m_nNonUhrStas); ++i)
            {
                if (m_staMacs.at(i)->GetQosTxop(AC_BE)->GetWifiMacQueue()->GetNPackets() > 0)
                {
                    continue;
                }
                bool first{false};
                if (m_ulTxPackets.size() <= i)
                {
                    first = true;
                    m_ulTxPackets.push_back(0);
                }
                if (m_params.numGeneratedUlPackets.at(i) > m_ulTxPackets.at(i))
                {
                    NS_LOG_DEBUG("Generate one " << (first ? "" : "more ") << "UL packet for STA "
                                                 << i + 1);
                    m_staMacs.at(i)->GetDevice()->GetNode()->AddApplication(
                        GetApplication(UPLINK, i, 1, 1000));
                    ++m_ulTxPackets.at(i);
                }
            }
        });
        CheckTrigger(psduMap, txVector);
        break;
    }
    case WIFI_MAC_QOSDATA:
        if (txVector.IsMu())
        {
            CheckMuPpdu(psduMap, txVector);
        }
        break;
    case WIFI_MAC_CTL_BACKRESP: {
        const auto ta = psduMap.cbegin()->second->GetAddr2();
        for (std::size_t i = 0; i < (m_nDsoStas + m_nNonDsoStas + m_nNonUhrStas); ++i)
        {
            if (m_staMacs.at(i)->GetLinkIdByAddress(ta) &&
                m_params.numGeneratedDlPackets.at(i) > m_dlTxPackets.at(i))
            {
                NS_LOG_DEBUG("Generate one more DL packet for STA " << i + 1);
                m_apMac->GetDevice()->GetNode()->AddApplication(
                    GetApplication(DOWNLINK, i, 1, 1000));
                ++m_dlTxPackets.at(i);
            }
        }
    }
    break;
    case WIFI_MAC_CTL_END: {
        ++m_txopId;
        break;
    }
    default:
        break;
    }
}

void
DsoSchedulerTest::CheckTrigger(const WifiConstPsduMap& psduMap, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psduMap << txVector);

    if (m_txopId >= m_params.expectedRuAllocations.size())
    {
        if (m_enableUlOfdma)
        {
            // all UL MU PPDUs are expected to be transmitted, disable polling to speed up test
            // completion
            auto muScheduler =
                DynamicCast<TestDsoMultiUserScheduler>(m_apMac->GetObject<MultiUserScheduler>());
            muScheduler->SetAccessReqInterval(Time());
        }
        return;
    }

    CtrlTriggerHeader trigger;
    psduMap.cbegin()->second->GetPayload(0)->PeekHeader(trigger);
    for (std::size_t i = 0; i < (m_nDsoStas + m_nNonDsoStas + m_nNonUhrStas); ++i)
    {
        const auto it = trigger.FindUserInfoWithAid(m_staMacs.at(i)->GetAssociationId());
        if (it == trigger.end())
        {
            NS_LOG_DEBUG("No user info for STA " << i + 1);
            if (trigger.IsBsrp())
            {
                NS_TEST_EXPECT_MSG_EQ(m_params.expectedRuAllocations.at(m_txopId).contains(i),
                                      false,
                                      "Expected no RU allocated in TF for STA "
                                          << i + 1 << " during TXOP #" << m_txopId + 1);
            }
            continue;
        }
        NS_TEST_ASSERT_MSG_EQ(m_params.expectedRuAllocations.at(m_txopId).contains(i),
                              true,
                              "Expected RU allocated in TF for STA " << i + 1 << " during TXOP #"
                                                                     << m_txopId + 1);
        const auto ru = std::get<EhtRu::RuSpec>(it->GetRuAllocation());
        NS_TEST_EXPECT_MSG_EQ(ru,
                              m_params.expectedRuAllocations.at(m_txopId).at(i),
                              "Unexpected RU allocation in TF for STA " << i + 1 << " during TXOP #"
                                                                        << m_txopId + 1);
    }
}

void
DsoSchedulerTest::CheckMuPpdu(const WifiConstPsduMap& psduMap, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psduMap << txVector);

    NS_TEST_ASSERT_MSG_LT(m_txopId,
                          m_params.expectedRuAllocations.size(),
                          "Unexpected number of MU PPDUS");

    const auto expectedNumDlMuPpdus = *std::max_element(m_params.numGeneratedDlPackets.cbegin(),
                                                        m_params.numGeneratedDlPackets.cend());
    const std::size_t countPrevDlMuPpdus = std::accumulate(
        m_txPsdus.cbegin(),
        m_txPsdus.cend(),
        0,
        [](std::size_t sum, const auto& frameInfo) {
            return (frameInfo.startTx < Simulator::Now()) && (frameInfo.txVector.IsDlMu())
                       ? (sum + 1)
                       : sum;
        });
    const auto prevMuPpdu =
        std::find_if(m_txPsdus.crbegin(), m_txPsdus.crend(), [](const auto& frameInfo) {
            return (frameInfo.startTx < Simulator::Now()) && frameInfo.txVector.IsMu() &&
                   frameInfo.psduMap.cbegin()->second->GetHeader(0).IsQosData() &&
                   frameInfo.psduMap.cbegin()->second->GetHeader(0).HasData();
        });
    if (m_enableUlOfdma && ((countPrevDlMuPpdus == expectedNumDlMuPpdus) ||
                            ((prevMuPpdu != m_txPsdus.crend()) && prevMuPpdu->txVector.IsDlMu())))
    {
        NS_TEST_EXPECT_MSG_EQ(txVector.IsUlMu(), true, "Expected a UL MU PPDU");
    }
    else
    {
        NS_TEST_EXPECT_MSG_EQ(txVector.IsDlMu(), true, "Expected a DL MU PPDU");
    }

    const auto& userInfoMap = txVector.GetHeMuUserInfoMap();
    for (const auto& [staId, userInfo] : userInfoMap)
    {
        NS_TEST_ASSERT_MSG_EQ(m_params.expectedRuAllocations.at(m_txopId).contains(staId - 1),
                              true,
                              "Expected RU allocated in MU PPDU for STA "
                                  << staId << " during TXOP #" << m_txopId + 1);
        const auto ru = std::get<EhtRu::RuSpec>(userInfo.ru);
        NS_TEST_EXPECT_MSG_EQ(ru,
                              m_params.expectedRuAllocations.at(m_txopId).at(staId - 1),
                              "Unexpected RU allocation in MU PPDU for STA "
                                  << staId << " during TXOP #" << m_txopId + 1);
    }
}

void
DsoSchedulerTest::CheckResults()
{
    NS_LOG_FUNCTION(this);
    for (const auto& pair : m_txPsdus)
    {
        if (pair.psduMap.cbegin()->second->GetHeader(0).IsQosData() &&
            pair.psduMap.cbegin()->second->GetHeader(0).HasData())
        {
            NS_LOG_DEBUG("TX PSDU: " << pair.psduMap.cbegin()->second->GetHeader(0) << " at "
                                     << pair.startTx << " with TX vector " << pair.txVector);
        }
    }
    const auto countDlMuPpdus = std::accumulate(
        m_txPsdus.cbegin(),
        m_txPsdus.cend(),
        0,
        [](std::size_t sum, const auto& frameInfo) {
            return (frameInfo.txVector.IsDlMu())
                       ? (sum + std::count_if(frameInfo.psduMap.cbegin(),
                                              frameInfo.psduMap.cend(),
                                              [](const auto& psdu) {
                                                  return psdu.second->GetHeader(0).IsQosData() &&
                                                         psdu.second->GetHeader(0).HasData();
                                              }))
                       : sum;
        });
    const auto countUlMuPpdus =
        std::accumulate(m_txPsdus.cbegin(),
                        m_txPsdus.cend(),
                        0,
                        [](std::size_t sum, const auto& frameInfo) {
                            return (frameInfo.txVector.IsUlMu() &&
                                    frameInfo.psduMap.cbegin()->second->GetHeader(0).IsQosData() &&
                                    frameInfo.psduMap.cbegin()->second->GetHeader(0).HasData())
                                       ? (sum + frameInfo.psduMap.size())
                                       : sum;
                        });
    const auto expectedDlTxPackets =
        std::accumulate(m_params.numGeneratedDlPackets.begin(),
                        m_params.numGeneratedDlPackets.end(),
                        0,
                        [](std::size_t sum, const auto numPackets) { return sum + numPackets; });
    const auto expectedUlTxPackets =
        std::accumulate(m_params.numGeneratedUlPackets.begin(),
                        m_params.numGeneratedUlPackets.end(),
                        0,
                        [](std::size_t sum, const auto numPackets) { return sum + numPackets; });
    NS_TEST_EXPECT_MSG_EQ(countDlMuPpdus,
                          expectedDlTxPackets,
                          "Unexpected number of transmitted DL packets");
    NS_TEST_EXPECT_MSG_EQ(countUlMuPpdus,
                          expectedUlTxPackets,
                          "Unexpected number of transmitted UL packets");
}

void
DsoSchedulerTest::DoSetup()
{
    // MU EDCA timers only prioritize trigger-based for STAs that have participated to the previous
    // TXOP, but this test also involves multiple TXOPs with different DSO STAs participating in
    // each of these TXOPs. Hence, in order to prioritize trigger-based access needed to test DSO MU
    // scheduler, we set AIFSN as high as allowed by the specs.
    Config::SetDefault("ns3::ApWifiMac::AifsnsForSta", StringValue(std::string("BE 15")));

    DsoTestBase::DoSetup();
    auto muScheduler =
        DynamicCast<TestDsoMultiUserScheduler>(m_apMac->GetObject<MultiUserScheduler>());
    muScheduler->SetAttribute("NStations", UintegerValue(m_params.maxServedStas));
    // Enable UL-OFDMA in case of UL traffic
    muScheduler->SetAttribute("EnableUlOfdma", BooleanValue(m_enableUlOfdma));

    m_apMac->GetQosTxop(AC_BE)->SetTxopLimit(MicroSeconds(1600));
}

void
DsoSchedulerTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    CheckResults();

    Simulator::Destroy();
}

WifiDsoTestSuite::WifiDsoTestSuite()
    : TestSuite("wifi-dso", Type::UNIT)
{
    WifiPhyOperatingChannel expectedChannel;

    /**
     * AP operating on 160 MHz in 5 GHz band with P80 being the lower 80 MHz, DSO STA supports up to
     * 80 MHz:
     *
     *
     *                ┌───────────┬───────────┐
     *  AP            │CH 106(P80)│CH 122(S80)│
     *                └───────────┴───────────┘
     *
     *                ┌───────────┬───────────┐
     *  STA           │  PRIMARY  │    DSO    │
     *                └───────────┴───────────┘
     */
    expectedChannel.Set({{122, MHz_t{5610}, MHz_t{80}, WIFI_PHY_BAND_5GHZ}}, WIFI_STANDARD_80211bn);
    AddTestCase(new DsoSubbandsTest("{114, 0, BAND_5GHZ, 0}",
                                    "{106, 0, BAND_5GHZ, 0}",
                                    {{EhtRu::SECONDARY_80_FLAGS, expectedChannel}}),
                TestCase::Duration::QUICK);

    /**
     * AP operating on 160 MHz in 5 GHz band with P80 being the higher 80 MHz, DSO STA supports up
     * to 80 MHz:
     *
     *
     *                ┌───────────┬───────────┐
     *  AP            │CH 155(S80)│CH 171(P80)│
     *                └───────────┴───────────┘
     *
     *                ┌───────────┬───────────┐
     *  STA           │   DSO     │  PRIMARY  │
     *                └───────────┴───────────┘
     */
    expectedChannel.Set({{155, MHz_t{5775}, MHz_t{80}, WIFI_PHY_BAND_5GHZ}}, WIFI_STANDARD_80211bn);
    AddTestCase(new DsoSubbandsTest("{163, 0, BAND_5GHZ, 7}",
                                    "{171, 0, BAND_5GHZ, 3}",
                                    {{EhtRu::SECONDARY_80_FLAGS, expectedChannel}}),
                TestCase::Duration::QUICK);

    /**
     * AP operating on 160 MHz in 6 GHz band with P80 being the lower 80 MHz, DSO STA supports up to
     * 80 MHz:
     *
     *
     *                ┌───────────┬───────────┐
     *  AP            │CH 103(P80)│CH 119(S80)│
     *                └───────────┴───────────┘
     *
     *                ┌───────────┬───────────┐
     *  STA           │  PRIMARY  │    DSO    │
     *                └───────────┴───────────┘
     */
    expectedChannel.Set({{119, MHz_t{6545}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    AddTestCase(new DsoSubbandsTest("{111, 0, BAND_6GHZ, 2}",
                                    "{103, 0, BAND_6GHZ, 2}",
                                    {{EhtRu::SECONDARY_80_FLAGS, expectedChannel}}),
                TestCase::Duration::QUICK);

    /**
     * AP operating on 160 MHz in 6 GHz band with P80 being the higher 80 MHz, DSO STA supports up
     * to 80 MHz:
     *
     *
     *                ┌───────────┬───────────┐
     *  AP            │CH 103(S80)│CH 119(P80)│
     *                └───────────┴───────────┘
     *
     *                ┌───────────┬───────────┐
     *  STA           │   DSO     │  PRIMARY  │
     *                └───────────┴───────────┘
     */
    expectedChannel.Set({{103, MHz_t{6465}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    AddTestCase(new DsoSubbandsTest("{111, 0, BAND_6GHZ, 7}",
                                    "{119, 0, BAND_6GHZ, 3}",
                                    {{EhtRu::SECONDARY_80_FLAGS, expectedChannel}}),
                TestCase::Duration::QUICK);

    /**
     * AP operating on 320 MHz with P160 being the lower 160 MHz, DSO STA supports up to 160 MHz:
     *
     *
     *                ┌───────────────────────┬───────────────────────┐
     *  AP            │       CH 15(P160)     │      CH 47(S160)      │
     *                └───────────────────────┴───────────────────────┘
     *
     *                ┌───────────────────────┬───────────────────────┐
     *  STA           │       PRIMARY         │          DSO          │
     *                └───────────────────────┴───────────────────────┘
     */
    expectedChannel.Set({{47, MHz_t{6185}, MHz_t{160}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    AddTestCase(new DsoSubbandsTest("{31, 0, BAND_6GHZ, 6}",
                                    "{15, 0, BAND_6GHZ, 6}",
                                    {{EhtRu::SECONDARY_160_FLAGS, expectedChannel}}),
                TestCase::Duration::QUICK);

    /**
     * AP operating on 320 MHz with P160 being the higher 160 MHz, DSO STA supports up to 160 MHz:
     *
     *
     *                ┌───────────────────────┬───────────────────────┐
     *  AP            │       CH 15(S160)     │      CH 47(P160)      │
     *                └───────────────────────┴───────────────────────┘
     *
     *                ┌───────────────────────┬───────────────────────┐
     *  STA           │          DSO          │       PRIMARY         │
     *                └───────────────────────┴───────────────────────┘
     */
    expectedChannel.Set({{15, MHz_t{6025}, MHz_t{160}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    AddTestCase(new DsoSubbandsTest("{31, 0, BAND_6GHZ, 9}",
                                    "{47, 0, BAND_6GHZ, 1}",
                                    {{EhtRu::SECONDARY_160_FLAGS, expectedChannel}}),
                TestCase::Duration::QUICK);

    /**
     * AP operating on 320 MHz with P160 being the lower 160 MHz and P80 being the lower 80 MHz
     * within the P160, DSO STA supports up to 80 MHz:
     *
     *
     *                ┌───────────┬───────────┬───────────┬───────────┐
     *  AP            │CH 167(P80)│CH 183(S80)│CH 199(L80)│CH 215(H80)│
     *                └───────────┴───────────┴───────────┴───────────┘
     *
     *                ┌───────────┬───────────┬───────────┬───────────┐
     *  STA           │  PRIMARY  │    DSO    │    DSO    │    DSO    │
     *                └───────────┴───────────┴───────────┴───────────┘
     */
    expectedChannel.Set({{183, MHz_t{6865}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    std::map<EhtRu::PrimaryFlags, WifiPhyOperatingChannel> expectedDsoSubbands;
    expectedDsoSubbands[EhtRu::SECONDARY_80_FLAGS] = expectedChannel;
    expectedChannel.Set({{199, MHz_t{6945}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_160_LOW_FLAGS] = expectedChannel;
    expectedChannel.Set({{215, MHz_t{7025}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_160_HIGH_FLAGS] = expectedChannel;
    AddTestCase(new DsoSubbandsTest("{191, 0, BAND_6GHZ, 3}",
                                    "{167, 0, BAND_6GHZ, 3}",
                                    expectedDsoSubbands),
                TestCase::Duration::QUICK);
    expectedDsoSubbands.clear();

    /**
     * AP operating on 320 MHz with P160 being the lower 160 MHz and P80 being the higher 80 MHz
     * within the P160, DSO STA supports up to 80 MHz:
     *
     *
     *                ┌───────────┬───────────┬───────────┬───────────┐
     *  AP            │CH 167(S80)│CH 183(P80)│CH 199(L80)│CH 215(H80)│
     *                └───────────┴───────────┴───────────┴───────────┘
     *
     *                ┌───────────┬───────────┬───────────┬───────────┐
     *  STA           │    DSO    │  PRIMARY  │   DSO     │   DSO     │
     *                └───────────┴───────────┴───────────┴───────────┘
     */
    expectedChannel.Set({{167, MHz_t{6785}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_80_FLAGS] = expectedChannel;
    expectedChannel.Set({{199, MHz_t{6945}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_160_LOW_FLAGS] = expectedChannel;
    expectedChannel.Set({{215, MHz_t{7025}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_160_HIGH_FLAGS] = expectedChannel;
    AddTestCase(new DsoSubbandsTest("{191, 0, BAND_6GHZ, 5}",
                                    "{183, 0, BAND_6GHZ, 1}",
                                    expectedDsoSubbands),
                TestCase::Duration::QUICK);
    expectedDsoSubbands.clear();

    /**
     * AP operating on 320 MHz with P160 being the higher 160 MHz and P80 being the lower 80 MHz
     * within the P160, DSO STA supports up to 80 MHz:
     *
     *
     *                ┌───────────┬───────────┬───────────┬───────────┐
     *  AP            │CH 167(L80)│CH 183(H80)│CH 199(P80)│CH 215(S80)│
     *                └───────────┴───────────┴───────────┴───────────┘
     *
     *                ┌───────────┬───────────┬───────────┬───────────┐
     *  STA           │    DSO    │    DSO    │  PRIMARY  │    DSO    │
     *                └───────────┴───────────┴───────────┴───────────┘
     */
    expectedChannel.Set({{215, MHz_t{7025}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_80_FLAGS] = expectedChannel;
    expectedChannel.Set({{167, MHz_t{6785}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_160_LOW_FLAGS] = expectedChannel;
    expectedChannel.Set({{183, MHz_t{6865}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_160_HIGH_FLAGS] = expectedChannel;
    AddTestCase(new DsoSubbandsTest("{191, 0, BAND_6GHZ, 10}",
                                    "{199, 0, BAND_6GHZ, 2}",
                                    expectedDsoSubbands),
                TestCase::Duration::QUICK);
    expectedDsoSubbands.clear();

    /**
     * AP operating on 320 MHz with P160 being the higher 160 MHz and P80 being the higher 80 MHz
     * within the P160, DSO STA supports up to 80 MHz:
     *
     *
     *                ┌───────────┬───────────┬───────────┬───────────┐
     *  AP            │CH 167(L80)│CH 183(H80)│CH 199(S80)│CH 215(P80)│
     *                └───────────┴───────────┴───────────┴───────────┘
     *
     *                ┌───────────┬───────────┬───────────┬───────────┐
     *  STA           │    DSO    │    DSO    │    DSO    │  PRIMARY  │
     *                └───────────┴───────────┴───────────┴───────────┘
     */
    expectedChannel.Set({{199, MHz_t{6945}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_80_FLAGS] = expectedChannel;
    expectedChannel.Set({{167, MHz_t{6785}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_160_LOW_FLAGS] = expectedChannel;
    expectedChannel.Set({{183, MHz_t{6865}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_160_HIGH_FLAGS] = expectedChannel;
    AddTestCase(new DsoSubbandsTest("{191, 0, BAND_6GHZ, 12}",
                                    "{215, 0, BAND_6GHZ, 0}",
                                    expectedDsoSubbands),
                TestCase::Duration::QUICK);
    expectedDsoSubbands.clear();

    auto printDelays = [](const std::vector<Time>& delays) {
        std::ostringstream oss;
        for (const auto& delay : delays)
        {
            oss << delay.As(Time::US) << " ";
        }
        return oss.str();
    };

    for (const auto nextTxopIsDso : {false, true})
    {
        for (const auto& channelSwitchBackDelays :
             std::initializer_list<std::vector<Time>>{{MicroSeconds(250), MicroSeconds(250)},
                                                      {MicroSeconds(100), MicroSeconds(300)},
                                                      {MicroSeconds(400), MicroSeconds(200)}})
        {
            for (const auto& paddingDelays :
                 std::initializer_list<std::vector<Time>>{{MicroSeconds(0), MicroSeconds(0)},
                                                          {MicroSeconds(256), MicroSeconds(256)},
                                                          {MicroSeconds(64), MicroSeconds(128)},
                                                          {MicroSeconds(1024), MicroSeconds(512)}})
            {
                /**
                 *             ┌──────┐          ┌────────┐   switch back
                 *             │ BSRP │          │QoS DATA│   to primary
                 *  [AP]       │  TF  │          ├────────┤    |
                 *             │(ICF) │          │QoS DATA│    |
                 *  ───────────┴──────▼┬────────┬┴────────┴┬─┬─▼─ ...
                 *  [DSO STA]         |│QoS Null│          │B│
                 *                    |│(ICR)   │          │A│
                 *                    |├────────┤          ├─┤
                 *  [UHR STA]         |│QoS Null│          │B│
                 *                    |│(ICR)   │          │A│
                 *                    |└────────┘          └─┘
                 *                    |
                 *                  switch
                 *                  to DSO
                 */
                AddTestCase(
                    new DsoTxopTest(
                        {.testName = "Check DSO basic frame exchange sequence with "
                                     "single protection enabled (nextTxopIsDso=" +
                                     std::to_string(nextTxopIsDso) + ", channelSwitchBackDelays=" +
                                     printDelays(channelSwitchBackDelays) +
                                     ", paddingDelays=" + printDelays(paddingDelays) + ")",
                         .numDlMuPpdus = 1,
                         .channelSwitchBackDelays = channelSwitchBackDelays,
                         .paddingDelays = paddingDelays,
                         .nextTxopIsDso = nextTxopIsDso,
                         .protectSingleExchange = true,
                         .expectedTxopEndEventInDsoSubband = DsoTxopEvent::TIMEOUT}),
                    TestCase::Duration::QUICK);
            }

            /**
             *                                                 switch back
             *                                                 to primary
             *                                                    |
             *             ┌──────┐          ┌────────┐    ┌──────┐
             *             │ BSRP │          │QoS DATA│    │      │
             *  [AP]       │  TF  │          ├────────┤    │CF-END│
             *             │(ICF) │          │QoS DATA│    │      │
             *  ───────────┴──────▼┬────────┬┴────────┴┬─┬─┴──────▼── ...
             *  [DSO STA]         |│QoS Null│          │B│
             *                    |│(ICR)   │          │A│
             *                    |├────────┤          ├─┤
             *  [UHR STA]         |│QoS Null│          │B│
             *                    |│(ICR)   │          │A│
             *                    |└────────┘          └─┘
             *                    |
             *                  switch
             *                  to DSO
             */
            AddTestCase(
                new DsoTxopTest(
                    {.testName = "Check DSO basic frame exchange sequence with CF-END to "
                                 "indicate end of TXOP (nextTxopIsDso=" +
                                 std::to_string(nextTxopIsDso) + ", channelSwitchBackDelays=" +
                                 printDelays(channelSwitchBackDelays) + ")",
                     .numDlMuPpdus = 1,
                     .channelSwitchBackDelays = channelSwitchBackDelays,
                     .nextTxopIsDso = nextTxopIsDso,
                     .expectedTxopEndEventInDsoSubband = DsoTxopEvent::RX_CF_END}),
                TestCase::Duration::QUICK);

            /**
             *                                                               switch back
             *                                                               to primary
             *                                                                  |
             *             ┌──────┐          ┌────────┐    ┌────────┐    ┌──────┐
             *             │ BSRP │          │QoS DATA│    │QoS DATA│    │      │
             *  [AP]       │  TF  │          ├────────┤    ├────────┤    │CF-END│
             *             │(ICF) │          │QoS DATA│    │QoS DATA│    │      │
             *  ───────────┴──────▼┬────────┬┴────────┴┬─┬─┴────────┴┬─┬─┴──────▼─ ...
             *  [DSO STA]         |│QoS Null│          │B│           │B│
             *                    |│(ICR)   │          │A│           │A│
             *                    |├────────┤          ├─┤           ├─┤
             *  [UHR STA]         |│QoS Null│          │B│           │B│
             *                    |│(ICR)   │          │A│           │A│
             *                    |└────────┘          └─┘           └─┘
             *                  switch
             *                  to DSO
             */
            AddTestCase(
                new DsoTxopTest({.testName = "Check DSO operations with multiple downlink "
                                             "transmissions (nextTxopIsDso=" +
                                             std::to_string(nextTxopIsDso) +
                                             ", channelSwitchBackDelays=" +
                                             printDelays(channelSwitchBackDelays) + ")",
                                 .numDlMuPpdus = 2,
                                 .channelSwitchBackDelays = channelSwitchBackDelays,
                                 .nextTxopIsDso = nextTxopIsDso,
                                 .expectedTxopEndEventInDsoSubband = DsoTxopEvent::RX_CF_END}),
                TestCase::Duration::QUICK);

            /**
             *                                                       switch back
             *                                                       to primary
             *                                                           |
             *             ┌──────┐          ┌────────┐    ┌────────┐    |
             *             │ BSRP │          │QoS DATA│    │QoS DATA│    |
             *  [AP]       │  TF  │          ├────────┤    ├────────┤    |
             *             │(ICF) │          │QoS DATA│    │QoS DATA│    |
             *  ───────────┴──────▼┬────────┬┴────────┴┬─┬─┴────────┴┬─┬─▼─ ...
             *  [DSO STA]         |│QoS Null│          │B│           │B│
             *                    |│(ICR)   │          │A│           │A│
             *                    |├────────┤          ├─┤           ├─┤
             *  [UHR STA]         |│QoS Null│          │B│           │B│
             *                    |│(ICR)   │          │A│           │A│
             *                    |└────────┘          └─┘           └─┘
             *                  switch
             *                  to DSO
             */
            AddTestCase(
                new DsoTxopTest({
                    .testName = "Check DSO operations with SIFS bursting (nextTxopIsDso=" +
                                std::to_string(nextTxopIsDso) + ", channelSwitchBackDelays=" +
                                printDelays(channelSwitchBackDelays) + ")",
                    .numDlMuPpdus = 2,
                    .channelSwitchBackDelays = channelSwitchBackDelays,
                    .nextTxopIsDso = nextTxopIsDso,
                    .protectSingleExchange = true,
                    .expectedTxopEndEventInDsoSubband = DsoTxopEvent::TIMEOUT,
                }),
                TestCase::Duration::QUICK);

            /**
             *             ┌──────┐        switch back
             *             │ BSRP │        to primary
             *  [AP]       │  TF  │           |
             *             │(ICF) │           |
             *  ───────────┴──────▼┬────────┬─▼─ ...
             *  [DSO STA]         |│QoS Null│
             *                    |│(ICR)   │
             *                    |├────────┤
             *  [UHR STA]         |│QoS Null│
             *                    |│(ICR)   │
             *                    |└────────┘
             *                  switch
             *                  to DSO
             */
            AddTestCase(
                new DsoTxopTest({.testName = "Check DSO operations with no DL MU PPDU after "
                                             "ICF response (nextTxopIsDso=" +
                                             std::to_string(nextTxopIsDso) +
                                             ", channelSwitchBackDelays=" +
                                             printDelays(channelSwitchBackDelays) + ")",
                                 .numDlMuPpdus = 0,
                                 .channelSwitchBackDelays = channelSwitchBackDelays,
                                 .nextTxopIsDso = nextTxopIsDso,
                                 .protectSingleExchange = true,
                                 .expectedTxopEndEventInDsoSubband = DsoTxopEvent::TIMEOUT}),
                TestCase::Duration::QUICK);

            /**
             *             ┌──────┐          ┌──────┐
             *             │ BSRP │          │      │
             *  [AP]       │  TF  │          │CF-END│
             *             │(ICF) │          │      │
             *  ───────────┴──────▼┬────────┬┴──────▼─ ...
             *  [DSO STA]         |│QoS Null│       |
             *                    |│(ICR)   │       |
             *                    |├────────┤       │
             *  [UHR STA]         |│QoS Null│       │
             *                    |│(ICR)   │       │
             *                    |└────────┘       │
             *                    |                 |
             *                  switch          switch back
             *                  to DSO          to primary
             */
            AddTestCase(
                new DsoTxopTest(
                    {.testName = "Check DSO operations with truncated TXOP (nextTxopIsDso=" +
                                 std::to_string(nextTxopIsDso) + ", channelSwitchBackDelays=" +
                                 printDelays(channelSwitchBackDelays) + ")",
                     .numDlMuPpdus = 0,
                     .channelSwitchBackDelays = channelSwitchBackDelays,
                     .nextTxopIsDso = nextTxopIsDso,
                     .expectedTxopEndEventInDsoSubband = DsoTxopEvent::RX_CF_END}),
                TestCase::Duration::QUICK);

            /**
             *             ┌──────┐          ┌────────┐        ┌────────┐  switch back
             *             │ BSRP │          │QoS DATA│        │QoS DATA│  to primary
             *  [AP]       │  TF  │          ├────────┤        ├────────┤    |
             *             │(ICF) │          │QoS DATA│        │QoS DATA│    │
             *  ───────────┴──────▼┬────────┬┴────────┴┬─┬─...─┴────────┴┬─┬─▼── ...
             *  [DSO STA]         ││QoS Null│          │B│               │B│
             *                    ││(ICR)   │          │A│               │A│
             *                    │├────────┤          ├─┤               ├─┤
             *  [UHR STA]         ││QoS Null│          │B│               │B│
             *                    ││(ICR)   │          │A│               │A│
             *                    │└────────┘          └─┘               └─┘
             *                    |
             *                  switch
             *                  to DSO
             */
            AddTestCase(
                new DsoTxopTest({.testName = "Check DSO operations with downlink transmissions "
                                             "covering the whole TXOP duration (nextTxopIsDso=" +
                                             std::to_string(nextTxopIsDso) +
                                             ", channelSwitchBackDelays=" +
                                             printDelays(channelSwitchBackDelays) + ")",
                                 .numDlMuPpdus = 8,
                                 .channelSwitchBackDelays = channelSwitchBackDelays,
                                 .nextTxopIsDso = nextTxopIsDso,
                                 .expectedTxopEndEventInDsoSubband = DsoTxopEvent::TIMEOUT}),
                TestCase::Duration::QUICK);
        }
    }

    /**
     *             ┌──────┐          ┌──────┐          ┌──┐┌──────┐          ┌──┐┌──────┐
     *             │ BSRP │          │Basic │          │BA││Basic │          │BA││      │
     *  [AP]       │  TF  │          │  TF  │          ├──┤│  TF  │          ├──┤│CF-END│
     *             │(ICF) │          │      │          │BA││      │          │BA││      │
     *  ───────────┴──────▼┬────────┬┴──────┴┬────────┬┴──┴┴──────┴┬────────┬┴──┴┴──────▼─ ...
     *  [DSO STA]         ││QoS Null│        │QoS DATA│            │QoS DATA│           │
     *                    ││(ICR)   │        │        │            │        │           │
     *                    │├────────┤        ├────────┤            ├────────┤           │
     *  [UHR STA]         ││QoS Null│        │QoS DATA│            │QoS DATA│           │
     *                    ││(ICR)   │        │        │            │        │           │
     *                    │└────────┘        └────────┘            └────────┘           │
     *                    │                                                             │
     *                  switch                                                    switch back
     *                  to DSO                                                    to primary
     *
     */
    AddTestCase(new DsoTxopTest({.testName = "Check DSO operations with trigger-based uplink "
                                             "transmission",
                                 .numUlMuPpdus = 2,
                                 .expectedTxopEndEventInDsoSubband = DsoTxopEvent::RX_CF_END}),
                TestCase::Duration::QUICK);

    /**
     *                  start
     *               interference
     *                    |
     *                    │─────────────────────────...
     * [DSO subband]      │      non-wifi signal
     *                    │─────────────────────────...
     *             ┌──────▼          ┌────────┐
     *             │ BSRP │          │QoS DATA│
     *  [AP]       │  TF  │          ├────────┤
     *             │(ICF) │          │QoS DATA│
     *  ───────────┴──────▼┬────────┬┴────────┴───▼─...
     *  [DSO STA]         |│QoS Null│             |
     *                    |│(ICR)   │             |
     *                    |├────────┤           ┌─┐
     *  [UHR STA]         |│QoS Null│           │B│
     *                    |│(ICR)   │           │A│
     *                    |└────────┘           └─┘
     *                    |                       |
     *                  switch                 switch back
     *                  to DSO                 to primary
     *
     *
     * The interference is generated on the DSO subband right before the ICF is processed by the
     * DSO STA, hence before the STA switches to the DSO subband. That way, the test can verify
     * interferences on the inactive PHY interface is correctly tracked.
     *
     * Note: Currently DSO STA when sending ICF reply does not do CCA. ICF response is sent at
     * lowest modulation and increased power to ensure it can still be successfully decoded
     */
    AddTestCase(new DsoTxopTest({.testName = "Check DL MU PPDU is not received by DSO STA in "
                                             "presence of interference on DSO subband",
                                 .numDlMuPpdus = 1,
                                 .generateInterferenceAfterIcf = true,
                                 .expectedTxopEndEventInDsoSubband = DsoTxopEvent::TIMEOUT}),
                TestCase::Duration::QUICK);

    /**
     *                                            ┌──────┐
     * [DSO subband]                              │ OBSS │
     *                                            └──────┘
     *                                            |
     *             ┌──────┐          ┌────────┐   |┌────────┐
     *             │ BSRP │          │QoS DATA│   |│QoS DATA│
     *  [AP]       │  TF  │          ├────────┤   |├────────┤
     *             │(ICF) │          │QoS DATA│   |│QoS DATA│
     *  ───────────┴──────▼┬────────┬┴────────┴┬─┬▼┴────────┴──▼─...
     *  [DSO STA]         |│QoS Null│          │B│             |
     *                    |│(ICR)   │          │A│             |
     *                    |├────────┤          ├─┤           ┌─┐
     *  [UHR STA]         |│QoS Null│          │B│           │B│
     *                    |│(ICR)   │          │A│           │A│
     *                    |└────────┘          └─┘           └─┘
     *                    |                                    |
     *                  switch                                switch back
     *                  to DSO                                to primary
     */
    AddTestCase(new DsoTxopTest({
                    .testName = "Check DSO STA switches back to primary subband "
                                "if it latches on an OBSS frame",
                    .numDlMuPpdus = 2,
                    .generateObssDuringDsoTxop = true,
                    .expectedTxopEndEventInDsoSubband = DsoTxopEvent::RX_OBSS,
                }),
                TestCase::Duration::QUICK);

    /**
     *                                        switch back
     *                                        to primary
     *             ┌──────┐                    |
     *             │ BSRP │                    |
     *  [AP]       │  TF  │          ┌────────┐|
     *             │(ICF) │          │QoS DATA│|
     *  ───────────┴──────▼┬────────┬┴────────┴▼───── ...
     *  [DSO STA]         |│QoS Null│
     *                    |│(ICR)   x
     *                    |├────────┤           ┌─┐
     *  [UHR STA]         |│QoS Null│           │B│
     *                    |│(ICR)   │           │A│
     *                    |└────────┘           └─┘
     *                    |
     *                  switch
     *                  to DSO
     */
    AddTestCase(new DsoTxopTest({.testName = "Check DSO frame exchange sequence can continue if "
                                             "one ICF response received on primary",
                                 .numDlMuPpdus = 1,
                                 .protectSingleExchange = true,
                                 .corruptedIcfResponses = {1},
                                 .expectedTxopEndEventInDsoSubband = DsoTxopEvent::TIMEOUT}),
                TestCase::Duration::QUICK);

    /**
     *                           switch back
     *                           to primary
     *             ┌──────┐          |
     *             │ BSRP │          |
     *  [AP]       │  TF  │          |
     *             │(ICF) │          |
     *  ───────────┴──────▼┬────────┬▼─ ...
     *  [DSO STA]         |│QoS Null│
     *                    |│(ICR)   │
     *                    |├────────┤
     *  [UHR STA]         |│QoS Null│
     *                    |│(ICR)   x
     *                    |└────────┘
     *                    |
     *                  switch
     *                  to DSO
     */
    AddTestCase(
        new DsoTxopTest(
            {.testName =
                 "Check DSO TXOP is released if no ICF responses in the primary 20 MHz subband",
             .numDlMuPpdus = 1,
             .protectSingleExchange = true,
             .corruptedIcfResponses = {0},
             .expectedTxopEndEventInDsoSubband = DsoTxopEvent::TIMEOUT}),
        TestCase::Duration::QUICK);

    /**
     *                           switch back
     *                           to primary
     *             ┌──────┐          |
     *             │ BSRP │          |
     *  [AP]       │  TF  │          |
     *             │(ICF) │          |
     *  ───────────┴──────▼┬────────┬▼─ ...
     *  [DSO STA]         |│QoS Null│
     *                    |│(ICR)   x
     *                    |├────────┤
     *  [UHR STA]         |│QoS Null│
     *                    |│(ICR)   x
     *                    |└────────┘
     *                    |
     *                  switch
     *                  to DSO
     */
    AddTestCase(
        new DsoTxopTest(
            {.testName = "Check DSO TXOP is released if no ICF responses because all ICF responses "
                         "are corrupted",
             .numDlMuPpdus = 1,
             .protectSingleExchange = true,
             .corruptedIcfResponses = {0, 1},
             .expectedTxopEndEventInDsoSubband = DsoTxopEvent::TIMEOUT}),
        TestCase::Duration::QUICK);

    /**
     *                           expected switch
     *                           back to primary
     *             ┌──────┐          |
     *             │ BSRP │          |
     *  [AP]       │  TF  │          |
     *             │(ICF) x          |
     *  ───────────┴──────▼──────────▼─ ...
     */
    AddTestCase(
        new DsoTxopTest(
            {.testName = "Check DSO TXOP is released if no ICF responses because ICF is corrupted",
             .numDlMuPpdus = 1,
             .protectSingleExchange = true,
             .corruptIcf = true,
             .expectedTxopEndEventInDsoSubband = DsoTxopEvent::TIMEOUT}),
        TestCase::Duration::QUICK);

    using TrafficParams = std::initializer_list<
        std::tuple<std::string, std::vector<std::size_t>, std::vector<std::size_t>>>;
    for (const auto& [trafficStr, dlTraffic, ulTraffic] : TrafficParams{
             {"DL traffic with equal traffic per STA in DSO TXOP", {1, 1}, {0, 0}},
             {"DL traffic with unequal traffic per STA in DSO TXOP", {3, 2}, {0, 0}},
             {"UL traffic with equal traffic per STA in DSO TXOP", {0, 0}, {1, 1}},
             {"UL traffic with unequal traffic per STA in DSO TXOP", {0, 0}, {2, 3}},
             {"DL+UL traffic with equal traffic per STA in DSO TXOP", {1, 1}, {1, 1}},
             {"DL+UL traffic with unequal traffic per STA in DSO TXOP (same for DL and UL)",
              {2, 3},
              {2, 3}},
             {"DL+UL traffic with unequal traffic per STA in DSO TXOP (different for DL and UL)",
              {2, 3},
              {3, 2}}})
    {
        AddTestCase(
            new DsoSchedulerTest(
                {.testName =
                     "Check DSO scheduler with 320 MHz AP, two 160 MHz DSO STAs: " + trafficStr,
                 .apOpChannel = "{31, 0, BAND_6GHZ, 0}",
                 .dsoStasOpChannel = {"{15, 0, BAND_6GHZ, 0}", "{15, 0, BAND_6GHZ, 0}"},
                 .numGeneratedDlPackets = dlTraffic,
                 .numGeneratedUlPackets = ulTraffic,
                 .expectedRuAllocations = {{{0,
                                             EhtRu::RuSpec{RuType::RU_2x996_TONE,
                                                           1,
                                                           EhtRu::PRIMARY_160_FLAGS.first,
                                                           EhtRu::PRIMARY_160_FLAGS.second}},
                                            {1,
                                             EhtRu::RuSpec{RuType::RU_2x996_TONE,
                                                           1,
                                                           EhtRu::SECONDARY_160_FLAGS.first,
                                                           EhtRu::SECONDARY_160_FLAGS.second}}}}}),
            TestCase::Duration::QUICK);

        AddTestCase(
            new DsoSchedulerTest(
                {.testName =
                     "Check DSO scheduler with 320 MHz AP, two 80 MHz DSO STAs: " + trafficStr,
                 .apOpChannel = "{31, 0, BAND_6GHZ, 0}",
                 .dsoStasOpChannel = {"{7, 0, BAND_6GHZ, 0}", "{7, 0, BAND_6GHZ, 0}"},
                 .numGeneratedDlPackets = dlTraffic,
                 .numGeneratedUlPackets = ulTraffic,
                 .expectedRuAllocations = {{{0,
                                             EhtRu::RuSpec{RuType::RU_996_TONE,
                                                           1,
                                                           EhtRu::PRIMARY_80_FLAGS.first,
                                                           EhtRu::PRIMARY_80_FLAGS.second}},
                                            {1,
                                             EhtRu::RuSpec{RuType::RU_996_TONE,
                                                           1,
                                                           EhtRu::SECONDARY_80_FLAGS.first,
                                                           EhtRu::SECONDARY_80_FLAGS.second}}}}}),
            TestCase::Duration::QUICK);

        AddTestCase(
            new DsoSchedulerTest(
                {.testName =
                     "Check DSO scheduler with 160 MHz AP, two 80 MHz DSO STAs: " + trafficStr,
                 .apOpChannel = "{15, 0, BAND_6GHZ, 0}",
                 .dsoStasOpChannel = {"{7, 0, BAND_6GHZ, 0}", "{7, 0, BAND_6GHZ, 0}"},
                 .numGeneratedDlPackets = dlTraffic,
                 .numGeneratedUlPackets = ulTraffic,
                 .expectedRuAllocations = {{{0,
                                             EhtRu::RuSpec{RuType::RU_996_TONE,
                                                           1,
                                                           EhtRu::PRIMARY_80_FLAGS.first,
                                                           EhtRu::PRIMARY_80_FLAGS.second}},
                                            {1,
                                             EhtRu::RuSpec{RuType::RU_996_TONE,
                                                           1,
                                                           EhtRu::SECONDARY_80_FLAGS.first,
                                                           EhtRu::SECONDARY_80_FLAGS.second}}}}}),
            TestCase::Duration::QUICK);
    }

    for (const auto& [trafficStr, dlTraffic, ulTraffic] :
         TrafficParams{{"DL traffic", {1, 1, 1, 1}, {0, 0, 0, 0}},
                       {"UL traffic", {0, 0, 0, 0}, {1, 1, 1, 1}}})
    {
        AddTestCase(
            new DsoSchedulerTest(
                {.testName = "Check DSO scheduler with 160 MHz AP, four 80 MHz DSO STAs (" +
                             trafficStr + ")",
                 .apOpChannel = "{15, 0, BAND_6GHZ, 0}",
                 .dsoStasOpChannel = {"{7, 0, BAND_6GHZ, 0}",
                                      "{7, 0, BAND_6GHZ, 0}",
                                      "{7, 0, BAND_6GHZ, 0}",
                                      "{7, 0, BAND_6GHZ, 0}"},
                 .numGeneratedDlPackets = dlTraffic,
                 .numGeneratedUlPackets = ulTraffic,
                 .expectedRuAllocations = {{{0,
                                             EhtRu::RuSpec{RuType::RU_484_TONE,
                                                           1,
                                                           EhtRu::PRIMARY_80_FLAGS.first,
                                                           EhtRu::PRIMARY_80_FLAGS.second}},
                                            {1,
                                             EhtRu::RuSpec{RuType::RU_484_TONE,
                                                           2,
                                                           EhtRu::PRIMARY_80_FLAGS.first,
                                                           EhtRu::PRIMARY_80_FLAGS.second}},
                                            {2,
                                             EhtRu::RuSpec{RuType::RU_484_TONE,
                                                           1,
                                                           EhtRu::SECONDARY_80_FLAGS.first,
                                                           EhtRu::SECONDARY_80_FLAGS.second}},
                                            {3,
                                             EhtRu::RuSpec{RuType::RU_484_TONE,
                                                           2,
                                                           EhtRu::SECONDARY_80_FLAGS.first,
                                                           EhtRu::SECONDARY_80_FLAGS.second}}}}}),
            TestCase::Duration::QUICK);
    }

    AddTestCase(
        new DsoSchedulerTest(
            {.testName = "Check DSO scheduler with 320 MHz AP, six 80 MHz DSO STAs (DL traffic)",
             .apOpChannel = "{31, 0, BAND_6GHZ, 0}",
             .dsoStasOpChannel = {"{7, 0, BAND_6GHZ, 0}",
                                  "{7, 0, BAND_6GHZ, 0}",
                                  "{7, 0, BAND_6GHZ, 0}",
                                  "{7, 0, BAND_6GHZ, 0}",
                                  "{7, 0, BAND_6GHZ, 0}",
                                  "{7, 0, BAND_6GHZ, 0}"},
             .numGeneratedDlPackets = {1, 1, 1, 1, 1, 1},
             .numGeneratedUlPackets = {0, 0, 0, 0, 0, 0},
             .expectedRuAllocations = {{{0,
                                         EhtRu::RuSpec{RuType::RU_996_TONE,
                                                       1,
                                                       EhtRu::PRIMARY_80_FLAGS.first,
                                                       EhtRu::PRIMARY_80_FLAGS.second}},
                                        {1,
                                         EhtRu::RuSpec{RuType::RU_996_TONE,
                                                       1,
                                                       EhtRu::SECONDARY_80_FLAGS.first,
                                                       EhtRu::SECONDARY_80_FLAGS.second}},
                                        {2,
                                         EhtRu::RuSpec{RuType::RU_996_TONE,
                                                       1,
                                                       EhtRu::SECONDARY_160_LOW_FLAGS.first,
                                                       EhtRu::SECONDARY_160_LOW_FLAGS.second}},
                                        {3,
                                         EhtRu::RuSpec{RuType::RU_996_TONE,
                                                       1,
                                                       EhtRu::SECONDARY_160_HIGH_FLAGS.first,
                                                       EhtRu::SECONDARY_160_HIGH_FLAGS.second}}},
                                       {{4,
                                         EhtRu::RuSpec{RuType::RU_996_TONE,
                                                       1,
                                                       EhtRu::PRIMARY_80_FLAGS.first,
                                                       EhtRu::PRIMARY_80_FLAGS.second}},
                                        {5,
                                         EhtRu::RuSpec{RuType::RU_996_TONE,
                                                       1,
                                                       EhtRu::SECONDARY_80_FLAGS.first,
                                                       EhtRu::SECONDARY_80_FLAGS.second}}}}}),
        TestCase::Duration::QUICK);

    AddTestCase(
        new DsoSchedulerTest(
            {.testName = "Check DSO scheduler with 320 MHz AP, six 80 MHz DSO STAs (UL traffic)",
             .apOpChannel = "{31, 0, BAND_6GHZ, 0}",
             .dsoStasOpChannel = {"{7, 0, BAND_6GHZ, 0}",
                                  "{7, 0, BAND_6GHZ, 0}",
                                  "{7, 0, BAND_6GHZ, 0}",
                                  "{7, 0, BAND_6GHZ, 0}",
                                  "{7, 0, BAND_6GHZ, 0}",
                                  "{7, 0, BAND_6GHZ, 0}"},
             .numGeneratedDlPackets = {0, 0, 0, 0, 0, 0},
             .numGeneratedUlPackets = {1, 1, 1, 1, 1, 1},
             .expectedRuAllocations = {{{0,
                                         EhtRu::RuSpec{RuType::RU_996_TONE,
                                                       1,
                                                       EhtRu::PRIMARY_80_FLAGS.first,
                                                       EhtRu::PRIMARY_80_FLAGS.second}},
                                        {1,
                                         EhtRu::RuSpec{RuType::RU_996_TONE,
                                                       1,
                                                       EhtRu::SECONDARY_80_FLAGS.first,
                                                       EhtRu::SECONDARY_80_FLAGS.second}},
                                        {2,
                                         EhtRu::RuSpec{RuType::RU_996_TONE,
                                                       1,
                                                       EhtRu::SECONDARY_160_LOW_FLAGS.first,
                                                       EhtRu::SECONDARY_160_LOW_FLAGS.second}},
                                        {3,
                                         EhtRu::RuSpec{RuType::RU_996_TONE,
                                                       1,
                                                       EhtRu::SECONDARY_160_HIGH_FLAGS.first,
                                                       EhtRu::SECONDARY_160_HIGH_FLAGS.second}}},
                                       {{4,
                                         EhtRu::RuSpec{RuType::RU_996_TONE,
                                                       1,
                                                       EhtRu::PRIMARY_80_FLAGS.first,
                                                       EhtRu::PRIMARY_80_FLAGS.second}},
                                        {5,
                                         EhtRu::RuSpec{RuType::RU_996_TONE,
                                                       1,
                                                       EhtRu::SECONDARY_80_FLAGS.first,
                                                       EhtRu::SECONDARY_80_FLAGS.second}}}}}),
        TestCase::Duration::QUICK);
}

static WifiDsoTestSuite g_wifiDsoTestSuite; ///< the test suite
