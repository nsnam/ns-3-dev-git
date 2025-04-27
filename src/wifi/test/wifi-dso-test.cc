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
#include "ns3/wifi-protection.h"
#include "ns3/wifi-psdu.h"

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
    stasPhyHelper.Set("ChannelSettings", StringValue(m_stasOpChannel));

    WifiMacHelper mac;
    mac.SetType("ns3::StaWifiMac",
                "Ssid",
                SsidValue(Ssid("wrong-ssid")),
                "MaxMissedBeacons",
                UintegerValue(1e6), // do not deassociate (beacons are stopped once all STAs are
                                    // associated to simplify)
                "ActiveProbing",
                BooleanValue(false));
    mac.SetDsoManager("ns3::TestDsoManager");

    NetDeviceContainer staDevices;
    for (std::size_t i = 0; i < m_nDsoStas + m_nNonDsoStas + m_nNonUhrStas; ++i)
    {
        wifi.ConfigUhrOptions("DsoActivated", BooleanValue(i < m_nDsoStas));
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
                BooleanValue(true));
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

    m_startAid = m_apMac->GetNextAssociationId();

    // schedule ML setup for one station at a time
    m_apMac->TraceConnectWithoutContext("AssociatedSta",
                                        MakeCallback(&DsoTestBase::StaAssociated, this));
    m_apMac->GetQosTxop(AC_BE)->TraceConnectWithoutContext(
        "BaEstablished",
        MakeCallback(&DsoTestBase::BaEstablishedDl, this));
    for (std::size_t i = 0; i < m_nDsoStas + m_nNonDsoStas + m_nNonUhrStas; ++i)
    {
        m_staMacs.at(i)->GetQosTxop(AC_BE)->TraceConnectWithoutContext(
            "BaEstablished",
            MakeCallback(&DsoTestBase::BaEstablishedUl, this).Bind(i));
    }
    Simulator::Schedule(Seconds(0), [&]() { m_staMacs.front()->SetSsid(Ssid("ns-3-ssid")); });
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
DsoTestBase::StaAssociated(uint16_t aid, Mac48Address /*addr*/)
{
    NS_LOG_FUNCTION(this << aid);

    NS_ASSERT(m_lastAid < aid);
    m_lastAid = aid;

    // wait some time (5ms) to allow the completion of association
    const auto delay = MilliSeconds(5);

    if (!m_establishBaDl.empty())
    {
        // trigger establishment of BA agreement with AP as originator
        Simulator::Schedule(delay, [=, this]() {
            m_apMac->GetDevice()->GetNode()->AddApplication(
                GetApplication(DOWNLINK, aid - m_startAid, 1, 1000, m_establishBaDl.front()));
        });
    }
    else if (!m_establishBaUl.empty())
    {
        // trigger establishment of BA agreement with AP as recipient
        Simulator::Schedule(delay, [=, this]() {
            m_staMacs.at(aid - m_startAid)
                ->GetDevice()
                ->GetNode()
                ->AddApplication(
                    GetApplication(UPLINK, aid - m_startAid, 1, 1000, m_establishBaUl.front()));
        });
    }
    else
    {
        Simulator::Schedule(delay, [=, this]() { SetSsid(aid - m_startAid + 1); });
    }
}

void
DsoTestBase::BaEstablishedDl(Mac48Address recipient,
                             uint8_t tid,
                             std::optional<Mac48Address> /* gcrGroup */)
{
    NS_LOG_FUNCTION(this << recipient << tid);

    // wait some time (5ms) to allow the exchange of the data frame that triggered the Block Ack
    const auto delay = MilliSeconds(5);

    auto linkId = m_apMac->IsAssociated(recipient);
    NS_TEST_ASSERT_MSG_EQ(linkId.has_value(), true, "No link for association of " << recipient);
    auto aid = m_apMac->GetWifiRemoteStationManager(*linkId)->GetAssociationId(recipient);

    if (auto it = std::find(m_establishBaDl.cbegin(), m_establishBaDl.cend(), tid);
        it != m_establishBaDl.cend() && std::next(it) != m_establishBaDl.cend())
    {
        // trigger establishment of BA agreement with AP as originator
        Simulator::Schedule(delay, [=, this]() {
            m_apMac->GetDevice()->GetNode()->AddApplication(
                GetApplication(DOWNLINK, aid - m_startAid, 1, 1000, *std::next(it)));
        });
    }
    else if (!m_establishBaUl.empty())
    {
        // trigger establishment of BA agreement with AP as recipient
        Simulator::Schedule(delay, [=, this]() {
            m_staMacs.at(aid - m_startAid)
                ->GetDevice()
                ->GetNode()
                ->AddApplication(
                    GetApplication(UPLINK, aid - m_startAid, 1, 1000, m_establishBaUl.front()));
        });
    }
    else
    {
        Simulator::Schedule(delay, [=, this]() { SetSsid(aid - m_startAid + 1); });
    }
}

void
DsoTestBase::BaEstablishedUl(std::size_t index,
                             Mac48Address recipient,
                             uint8_t tid,
                             std::optional<Mac48Address> /* gcrGroup */)
{
    NS_LOG_FUNCTION(this << index << recipient << tid);

    // wait some time (5ms) to allow the exchange of the data frame that triggered the Block Ack
    const auto delay = MilliSeconds(5);

    if (auto it = std::find(m_establishBaUl.cbegin(), m_establishBaUl.cend(), tid);
        it != m_establishBaUl.cend() && std::next(it) != m_establishBaUl.cend())
    {
        // trigger establishment of BA agreement with AP as recipient
        Simulator::Schedule(delay, [=, this]() {
            m_staMacs.at(index)->GetDevice()->GetNode()->AddApplication(
                GetApplication(UPLINK, index, 1, 1000, *std::next(it)));
        });
    }
    else
    {
        Simulator::Schedule(delay, [=, this]() { SetSsid(index + 1); });
    }
}

void
DsoTestBase::SetSsid(std::size_t staIdx)
{
    NS_LOG_FUNCTION(this << staIdx);

    if (staIdx < m_nDsoStas + m_nNonDsoStas + m_nNonUhrStas)
    {
        // make the next STA start association
        m_staMacs.at(staIdx)->SetSsid(Ssid("ns-3-ssid"));
        return;
    }

    // all stations associated: traffic if needed
    StartTraffic();

    // stop generation of beacon frames in order to avoid interference
    m_apMac->SetAttribute("BeaconGeneration", BooleanValue(false));

    // disconnect callbacks
    m_apMac->TraceDisconnectWithoutContext("AssociatedSta",
                                           MakeCallback(&DsoTestBase::StaAssociated, this));
    m_apMac->GetQosTxop(AC_BE)->TraceDisconnectWithoutContext(
        "BaEstablished",
        MakeCallback(&DsoTestBase::BaEstablishedDl, this));
    for (std::size_t i = 0; i < m_nDsoStas + m_nNonDsoStas + m_nNonUhrStas; ++i)
    {
        m_staMacs.at(i)->GetQosTxop(AC_BE)->TraceDisconnectWithoutContext(
            "BaEstablished",
            MakeCallback(&DsoTestBase::BaEstablishedUl, this).Bind(i));
    }
}

void
DsoTestBase::StartTraffic()
{
    NS_LOG_FUNCTION(this);
    m_started = true;
}

void
DsoTestBase::Transmit(WifiConstPsduMap psduMap, WifiTxVector txVector, double /*txPowerW*/)
{
    NS_LOG_FUNCTION(this << psduMap << txVector);
    if (!m_started)
    {
        return;
    }
    m_txPsdus.push_back({Simulator::Now(), psduMap, txVector});
}

void
DsoTestBase::Receive(Ptr<const Packet> p, const Address& adr)
{
    NS_LOG_FUNCTION(this << p << adr);
    if (!m_started)
    {
        return;
    }
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
    m_stasOpChannel = stasChannel;
    m_duration = Seconds(0.5);
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
    m_stasOpChannel = "{106, 0, BAND_5GHZ, 0}";
    m_duration = Seconds(1.0);
    m_establishBaDl = {0};
    m_establishBaUl = {0};
    if (m_params.generateInterferenceAfterIcf)
    {
        // use more robust modulation if interference is generated
        m_mode = "UhrMcs0";
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
        if (const auto isStaOperatingOnDsoSubband = (i == m_idxStaInDsoSubband);
            isStaOperatingOnDsoSubband)
        {
            CheckSwitchBack(i, txDuration);
        }
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

    auto muScheduler =
        DynamicCast<TestDsoMultiUserScheduler>(m_apMac->GetObject<MultiUserScheduler>());
    muScheduler->SetForcedFormat(std::nullopt);

    DsoTestBase::StartTraffic();

    auto numDlMuPpdus{m_params.numDlMuPpdus};
    if ((m_params.numDlMuPpdus == 0) && (m_params.numUlMuPpdus == 0))
    {
        // we need to generate at least one packet to trigger the DSO frame exchange to be
        // initiated, but we ensure the packet gets dropped before it has a chance to be transmitted
        numDlMuPpdus = 1;
        m_apMac->GetQosTxop(AC_BE)->GetWifiMacQueue()->SetAttribute("MaxDelay",
                                                                    TimeValue(MicroSeconds(10)));
    }

    if (numDlMuPpdus > 0)
    {
        for (std::size_t i = 0; i < m_nDsoStas; ++i)
        {
            m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(DOWNLINK, i, 1, 1000));
        }
    }
    else if (m_params.numUlMuPpdus > 0)
    {
        muScheduler->SetAccessReqInterval(MilliSeconds(1));
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
    if (!m_started)
    {
        // traffic is not started yet
        return;
    }

    CtrlTriggerHeader trigger;
    psduMap.cbegin()->second->GetPayload(0)->PeekHeader(trigger);
    if (!trigger.IsBsrp())
    {
        // not a DSO ICF
        return;
    }

    NS_LOG_DEBUG("Send DSO ICF");

    NS_TEST_EXPECT_MSG_EQ(txVector.IsNonHtDuplicate(),
                          true,
                          "DSO ICF shall be sent in the non-HT duplicate PPDU format");

    const auto dataRate = txVector.GetMode().GetDataRate(MHz_t{20});
    NS_TEST_EXPECT_MSG_EQ(
        ((dataRate == 6'000'000) || (dataRate == 12'000'000) || (dataRate == 24'000'000)),
        true,
        "DSO ICF shall be sent at 6 Mb/s, 12 Mb/s, or 24 Mb/s");

    auto txDuration =
        WifiPhy::CalculateTxDuration(psduMap,
                                     txVector,
                                     m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetPhyBand());

    if (m_params.generateInterferenceAfterIcf)
    {
        // start interference right before the DSO STA is expected to switch to the DSO subband
        Simulator::Schedule(txDuration - TimeStep(1),
                            &DsoTxopTest::StartInterference,
                            this,
                            m_duration);
        // increase power of STAs to have the AP able to latch on their signals despite the
        // interference
        for (std::size_t i = 0; i < m_nDsoStas; ++i)
        {
            m_staMacs.at(i)->GetWifiPhy(SINGLE_LINK_OP_ID)->SetTxPowerStart(dBm_t{30});
            m_staMacs.at(i)->GetWifiPhy(SINGLE_LINK_OP_ID)->SetTxPowerEnd(dBm_t{30});
        }
    }

    Simulator::Schedule(txDuration - TimeStep(1), [=, this]() {
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
        Simulator::Schedule(txDuration + TimeStep(1), [=, this]() {
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

            NS_LOG_DEBUG("Check STA " << m_idxStaInDsoSubband + 1
                                      << " is switching to the DSO subband");
            dsoManager =
                DynamicCast<TestDsoManager>(m_staMacs.at(m_idxStaInDsoSubband)->GetDsoManager());
            phy = m_staMacs.at(m_idxStaInDsoSubband)->GetWifiPhy(SINGLE_LINK_OP_ID);
            NS_TEST_EXPECT_MSG_EQ(
                ((phy->IsStateSwitching() &&
                  (phy->GetOperatingChannel() ==
                   dsoManager->GetDsoSubbands(SINGLE_LINK_OP_ID).cbegin()->second))),
                true,
                "PHY of STA " << (m_idxStaInDsoSubband + 1)
                              << " should be switching to the DSO subband after the "
                                 "reception of the ICF");
        });
    }

    // check that STAs are operating on the expected subband after the reception of the ICF
    Simulator::Schedule(txDuration + m_params.switchingDelayToDso + TimeStep(1), [=, this]() {
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
                                  << " has switched to the DSO subband");
        dsoManager =
            DynamicCast<TestDsoManager>(m_staMacs.at(m_idxStaInDsoSubband)->GetDsoManager());
        phy = m_staMacs.at(m_idxStaInDsoSubband)->GetWifiPhy(SINGLE_LINK_OP_ID);
        NS_TEST_EXPECT_MSG_EQ(
            ((!phy->IsStateSwitching()) &&
             (phy->GetOperatingChannel() ==
              dsoManager->GetDsoSubbands(SINGLE_LINK_OP_ID).cbegin()->second)),
            true,
            "PHY of STA "
                << (m_idxStaInDsoSubband + 1)
                << " should have switched to the DSO subband after the reception of the ICF");
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
    if (!m_started)
    {
        // traffic is not started yet
        return;
    }

    NS_LOG_DEBUG("Send ICF response");

    if ((m_params.numDlMuPpdus == 0) && (m_params.numUlMuPpdus == 0))
    {
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
        if (const auto isStaOperatingOnDsoSubband = (clientId == m_idxStaInDsoSubband);
            isStaOperatingOnDsoSubband)
        {
            auto delay = GetDelayUntilTxopCompletion(clientId, psduMap, txVector);
            if (m_params.protectSingleExchange) // switch back immediately if TXOP is terminated by
                                                // CF-END
            {
                NS_LOG_DEBUG(
                    "Switch back to primary subband for STA "
                    << clientId + 1
                    << " is expected to be delayed by aSIFSTime + aSlotTime + aRxPHYStartDelay");
                const auto phy = m_staMacs.at(clientId)->GetWifiPhy(SINGLE_LINK_OP_ID);
                delay += phy->GetSifs() + phy->GetSlot() + EMLSR_OR_DSO_RX_PHY_START_DELAY;
            }
            CheckSwitchBack(clientId, delay);
        }
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
DsoTxopTest::CheckSwitchBack(std::size_t clientId, const Time& delay)
{
    const auto phy = m_staMacs.at(clientId)->GetWifiPhy(SINGLE_LINK_OP_ID);
    const auto dsoManager = DynamicCast<TestDsoManager>(m_staMacs.at(clientId)->GetDsoManager());

    NS_TEST_EXPECT_MSG_NE(phy->GetOperatingChannel(),
                          dsoManager->GetPrimarySubband(SINGLE_LINK_OP_ID),
                          "STA " << clientId + 1
                                 << " should not be operating on its primary subband");

    // check that STAs are operating on the expected channel before the end of the switch back
    // delay
    Simulator::Schedule(delay - TimeStep(1), [=, this]() {
        // the first STA is operating on its primary subband whereas the second STA is operating
        // on the DSO subband
        NS_LOG_DEBUG("Check STA " << clientId + 1 << " is still operating on the DSO subband");
        NS_TEST_EXPECT_MSG_EQ(
            m_staMacs.at(clientId)->GetWifiPhy(SINGLE_LINK_OP_ID)->GetOperatingChannel(),
            dsoManager->GetDsoSubbands(SINGLE_LINK_OP_ID).cbegin()->second,
            "PHY of STA " << std::to_string(clientId + 1)
                          << " should be operating on the DSO "
                             "subband before the switch back delay");
    });

    // check that STAs are operating on the expected channel after the end of the switch back
    // delay
    if (m_params.switchingDelayToPrimary.IsStrictlyPositive())
    {
        Simulator::Schedule(delay + TimeStep(2), [=, this]() {
            // both STAs should be operating on the primary subband
            NS_LOG_DEBUG("Check STA " << clientId + 1
                                      << " is switching back to the primary subband");
            auto dsoManager = DynamicCast<TestDsoManager>(m_staMacs.at(clientId)->GetDsoManager());
            NS_TEST_EXPECT_MSG_EQ(
                (phy->IsStateSwitching() &&
                 (phy->GetOperatingChannel() == dsoManager->GetPrimarySubband(SINGLE_LINK_OP_ID))),
                true,
                "PHY of STA " << std::to_string(clientId + 1)
                              << " should be switching back to the primary subband after the "
                                 "switch back delay");
        });
    }
    Simulator::Schedule(delay + m_params.switchingDelayToPrimary + TimeStep(1), [=, this]() {
        // both STAs should be operating on the primary subband
        NS_LOG_DEBUG("Check STA " << clientId + 1 << " has switched back to the primary subband");
        auto dsoManager = DynamicCast<TestDsoManager>(m_staMacs.at(clientId)->GetDsoManager());
        auto phy = m_staMacs.at(clientId)->GetWifiPhy(SINGLE_LINK_OP_ID);
        NS_TEST_EXPECT_MSG_EQ(
            ((!phy->IsStateSwitching()) &&
             (phy->GetOperatingChannel() == dsoManager->GetPrimarySubband(SINGLE_LINK_OP_ID))),
            true,
            "PHY of STA "
                << std::to_string(clientId + 1)
                << " should have switched back to the primary subband after the switch back delay");
    });
}

void
DsoTxopTest::CheckQosFrames(const WifiConstPsduMap& psduMap, const WifiTxVector& txVector)
{
    if (!m_started)
    {
        // traffic is not started yet
        return;
    }

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
    NS_TEST_EXPECT_MSG_EQ(userInfoMap.size(), 2, "DL MU PPDU should be addressed to both STAs");

    const auto ruStaPrimary = std::get<EhtRu::RuSpec>(
        userInfoMap.at(m_staMacs.at(m_idxStaInPrimarySubband)->GetAssociationId()).ru);
    NS_TEST_EXPECT_MSG_EQ(ruStaPrimary.GetPrimary160MHz() &&
                              ruStaPrimary.GetPrimary80MHzOrLower80MHz(),
                          true,
                          "PSDU for STA " << m_idxStaInPrimarySubband + 1
                                          << " should be sent over the primary 80 MHz");

    const auto ruStaDso = std::get<EhtRu::RuSpec>(
        userInfoMap.at(m_staMacs.at(m_idxStaInDsoSubband)->GetAssociationId()).ru);
    NS_TEST_EXPECT_MSG_EQ((ruStaDso.GetPrimary160MHz() && !ruStaDso.GetPrimary80MHzOrLower80MHz()),
                          true,
                          "PSDU for STA " << (m_idxStaInDsoSubband + 1)
                                          << " should be sent over the secondary 80 MHz");

    if (m_params.generateInterferenceAfterIcf)
    {
        const auto txDuration =
            WifiPhy::CalculateTxDuration(psduMap,
                                         txVector,
                                         m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetPhyBand());
        for (std::size_t i = 0; i < m_nDsoStas; ++i)
        {
            if (const auto isStaOperatingOnDsoSubband = (i == m_idxStaInDsoSubband);
                isStaOperatingOnDsoSubband)
            {
                CheckSwitchBack(i, txDuration);
            }
        }
    }
}

void
DsoTxopTest::CheckBlockAck(const WifiConstPsduMap& psduMap, const WifiTxVector& txVector)
{
    if (!m_started)
    {
        // traffic is not started yet
        return;
    }

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

    if (m_params.generateObssDuringDsoTxop && (m_countBlockAck == 1))
    {
        auto txDuration =
            WifiPhy::CalculateTxDuration(psduMap,
                                         txVector,
                                         m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetPhyBand());
        Simulator::Schedule(txDuration + TimeStep(1), &DsoTxopTest::GenerateObssFrame, this);
    }

    auto muScheduler =
        DynamicCast<TestDsoMultiUserScheduler>(m_apMac->GetObject<MultiUserScheduler>());
    if (m_params.numDlMuPpdus > m_countQoSframes)
    {
        NS_LOG_DEBUG("Generate one more packet for STA " << *clientId + 1);
        NS_ASSERT(clientId);
        m_apMac->GetDevice()->GetNode()->AddApplication(
            GetApplication(DOWNLINK, *clientId, 1, 1000));
    }
    else if (m_params.numDlMuPpdus == m_countQoSframes)
    {
        if (const auto isStaOperatingOnDsoSubband = (*clientId == m_idxStaInDsoSubband);
            isStaOperatingOnDsoSubband && !m_params.generateInterferenceAfterIcf &&
            !m_params.generateObssDuringDsoTxop)
        {
            NS_LOG_DEBUG("DSO frame exchange(s) completed, channel shall switch back for STA "
                         << *clientId + 1);
            const auto phy = m_staMacs.at(*clientId)->GetWifiPhy(SINGLE_LINK_OP_ID);
            auto delay = GetDelayUntilTxopCompletion(*clientId, psduMap, txVector);
            const auto txDuration =
                WifiPhy::CalculateTxDuration(psduMap, txVector, phy->GetPhyBand());
            const auto cfEndDuration = WifiPhy::CalculateTxDuration(
                Create<WifiPsdu>(Create<Packet>(), WifiMacHeader(WIFI_MAC_CTL_END)),
                m_apMac->GetWifiRemoteStationManager(SINGLE_LINK_OP_ID)
                    ->GetRtsTxVector(Mac48Address::GetBroadcast(), txVector.GetChannelWidth()),
                phy->GetPhyBand());
            if (delay - txDuration <
                cfEndDuration) // switch back immediately if TXOP is terminated by CF-END
            {
                NS_LOG_DEBUG(
                    "Switch back to primary subband for STA "
                    << *clientId + 1
                    << " is expected to be delayed by aSIFSTime + aSlotTime + aRxPHYStartDelay");
                const auto phy = m_staMacs.at(*clientId)->GetWifiPhy(SINGLE_LINK_OP_ID);
                delay =
                    txDuration + phy->GetSifs() + phy->GetSlot() + EMLSR_OR_DSO_RX_PHY_START_DELAY;
            }
            CheckSwitchBack(*clientId, delay);
        }

        if (m_countBlockAck == (m_nDsoStas * m_params.numDlMuPpdus))
        {
            // all STAs have sent all their Block ACKs, schedule one last packet per STA for next
            // (non-DSO) TXOP
            for (std::size_t i = 0; i < m_nDsoStas; ++i)
            {
                if (const auto mpdu =
                        m_apMac->GetQosTxop(AC_BE)->GetWifiMacQueue()->PeekByTidAndAddress(
                            0,
                            m_staMacs.at(i)->GetAddress());
                    !mpdu ||
                    mpdu->IsInFlight()) // otherwise it means there is already one packet enqueued
                                        // and hence it is not needed to schedule more
                {
                    NS_LOG_DEBUG("Generate one last packet for STA " << i + 1);
                    Simulator::Schedule(MicroSeconds(150) + m_params.switchingDelayToPrimary +
                                            TimeStep(i),
                                        [this, i]() {
                                            m_apMac->GetDevice()->GetNode()->AddApplication(
                                                GetApplication(DOWNLINK, i, 1, 1000));
                                        });
                }
            }
            muScheduler->SetForcedFormat(MultiUserScheduler::TxFormat::SU_TX);
        }
    }
    else if ((m_nDsoStas * m_params.numUlMuPpdus) == m_countQoSframes)
    {
        for (std::size_t i = 0; i < m_nDsoStas; ++i)
        {
            if (const auto isStaOperatingOnDsoSubband = (i == m_idxStaInDsoSubband);
                isStaOperatingOnDsoSubband)
            {
                NS_LOG_DEBUG("DSO frame exchange(s) completed, channel shall switch back for STA "
                             << i + 1);
                const auto delay = GetDelayUntilTxopCompletion(i, psduMap, txVector);
                CheckSwitchBack(i, delay);
            }

            // schedule one last packet to be transmitted in a following TXOP (non-DSO)
            NS_LOG_DEBUG("Generate one last packet for STA " << i + 1);
            Simulator::Schedule(MicroSeconds(150) + m_params.switchingDelayToPrimary + TimeStep(i),
                                [this, i]() {
                                    m_apMac->GetDevice()->GetNode()->AddApplication(
                                        GetApplication(DOWNLINK, i, 1, 1000));
                                });
        }
        muScheduler->SetForcedFormat(MultiUserScheduler::TxFormat::SU_TX);
        muScheduler->SetAccessReqInterval(Time());
    }
}

void
DsoTxopTest::DsoTxopEventCallback(std::size_t index, DsoTxopEvent event, uint8_t /*linkId*/)
{
    NS_LOG_DEBUG("DSO TXOP event for STA " << index + 1 << ": " << event);
    const auto& [it, inserted] = m_dsoTxopEventInfos.emplace(index, std::vector<DsoTxopEvent>{});
    it->second.emplace_back(event);
}

void
DsoTxopTest::CheckResults()
{
    NS_TEST_ASSERT_MSG_EQ(m_txPsdus.empty(), false, "No frame exchanged during the test");

    auto psduIt = m_txPsdus.cbegin();

    // Check the DSO ICF sent by the AP
    auto psdu = psduIt->psduMap.cbegin()->second;
    NS_TEST_ASSERT_MSG_EQ(psdu->GetHeader(0).IsTrigger(),
                          true,
                          "The first frame should be a trigger frame (ICF)");

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

    // Check ICR responses to DSO ICF sent by the non-AP STAs
    std::size_t qosNullCount{0};
    for (std::size_t i = 0; i < m_nDsoStas; ++i)
    {
        ++psduIt;
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
    }

    NS_TEST_EXPECT_MSG_EQ(qosNullCount, m_nDsoStas, "Unexpected number of QoS Null frames (ICR)");

    if ((m_params.numDlMuPpdus == 0) && (m_params.numUlMuPpdus == 0))
    {
        return;
    }

    const auto icrTxEnd =
        psduIt->startTx +
        WifiPhy::CalculateTxDuration(psduIt->psduMap,
                                     psduIt->txVector,
                                     m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetPhyBand());
    ++psduIt;

    auto endPrevious{icrTxEnd};
    for (std::size_t i = 0; i < m_params.numDlMuPpdus; ++i)
    {
        // Check the DL MU PPDU sent by the AP
        NS_TEST_EXPECT_MSG_EQ(psduIt->txVector.IsDlMu(),
                              true,
                              "A DL MU PPDU should be sent by the AP");
        NS_TEST_ASSERT_MSG_EQ(psduIt->psduMap.size(),
                              m_nDsoStas,
                              "Unexpected number of STAs addressed by the DL MU PPDU");
        NS_TEST_EXPECT_MSG_EQ(endPrevious + m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetSifs(),
                              psduIt->startTx,
                              "Expected the MU PPDU to start a SIFS after the previous frame");

        endPrevious = psduIt->startTx + WifiPhy::CalculateTxDuration(
                                            psduIt->psduMap,
                                            psduIt->txVector,
                                            m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetPhyBand());

        // Check Block ACK responses sent by the non-AP STAs
        std::size_t blockAckCount{0};
        for (std::size_t j = 0; j < m_nDsoStas; ++j)
        {
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
            NS_TEST_EXPECT_MSG_EQ(endPrevious + m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetSifs(),
                                  psduIt->startTx,
                                  "Expected the Block ACK to start a SIFS after the DL MU PPDU");
        }
        const auto expectedNumBlockAcks = ((i == 0) && m_params.generateInterferenceAfterIcf) ||
                                                  ((i == 1) && m_params.generateObssDuringDsoTxop)
                                              ? (m_nDsoStas - 1)
                                              : m_nDsoStas;
        NS_TEST_EXPECT_MSG_EQ(blockAckCount,
                              expectedNumBlockAcks,
                              "Unexpected number of Block ACK responses");
        endPrevious = psduIt->startTx + WifiPhy::CalculateTxDuration(
                                            psduIt->psduMap,
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

        endPrevious = psduIt->startTx + WifiPhy::CalculateTxDuration(
                                            psduIt->psduMap,
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
        endPrevious = psduIt->startTx + WifiPhy::CalculateTxDuration(
                                            psduIt->psduMap,
                                            psduIt->txVector,
                                            m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetPhyBand());

        ++psduIt;
    }

    if (m_params.generateInterferenceAfterIcf || m_params.generateObssDuringDsoTxop)
    {
        NS_TEST_ASSERT_MSG_EQ(
            (psduIt->psduMap.cbegin()->second->GetHeader(0).IsBlockAckReq() &&
             (psduIt->psduMap.cbegin()->second->GetHeader(0).GetAddr2() == m_apMac->GetAddress()) &&
             (psduIt->psduMap.cbegin()->second->GetHeader(0).GetAddr1() ==
              m_staMacs.at(m_idxStaInDsoSubband)->GetAddress())),
            true,
            "A BAR is expected to be sent to the DSO STA that did not reply with a Block ACK");
        ++psduIt;

        NS_TEST_ASSERT_MSG_EQ(
            (psduIt->psduMap.cbegin()->second->GetHeader(0).IsBlockAckReq() &&
             (psduIt->psduMap.cbegin()->second->GetHeader(0).GetAddr2() == m_apMac->GetAddress()) &&
             (psduIt->psduMap.cbegin()->second->GetHeader(0).GetAddr1() ==
              m_staMacs.at(m_idxStaInDsoSubband)->GetAddress()) &&
             psduIt->psduMap.cbegin()->second->GetHeader(0).IsRetry()),
            true,
            "A BAR is expected to be retransmitted to the DSO STA since it was "
            "switching back to its primary subband during the first BAR");
        endPrevious = psduIt->startTx + WifiPhy::CalculateTxDuration(
                                            psduIt->psduMap,
                                            psduIt->txVector,
                                            m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetPhyBand());

        ++psduIt;
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
        endPrevious = psduIt->startTx + WifiPhy::CalculateTxDuration(
                                            psduIt->psduMap,
                                            psduIt->txVector,
                                            m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetPhyBand());
        ++psduIt;
    }

    if (const auto numRemainingPsdus = std::distance(psduIt, m_txPsdus.cend());
        numRemainingPsdus >= 6)
    {
        // Check the CF-END sent by the AP
        NS_TEST_EXPECT_MSG_EQ(
            (psduIt->psduMap.cbegin()->second->GetHeader(0).IsCfEnd() &&
             (psduIt->psduMap.cbegin()->second->GetHeader(0).GetAddr2() == m_apMac->GetAddress())),
            true,
            "The last frame of the DSO TXOP is expected to be a CF-END");
        NS_TEST_EXPECT_MSG_EQ(endPrevious + m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetSifs(),
                              psduIt->startTx,
                              "Expected the CF-END to start a SIFS after the Block ACK");
        endPrevious = psduIt->startTx + WifiPhy::CalculateTxDuration(
                                            psduIt->psduMap,
                                            psduIt->txVector,
                                            m_apMac->GetWifiPhy(SINGLE_LINK_OP_ID)->GetPhyBand());
        ++psduIt;
    }

    for (std::size_t i = 0; i < m_nDsoStas; ++i)
    {
        NS_TEST_EXPECT_MSG_EQ(
            (psduIt->psduMap.cbegin()->second->GetHeader(0).IsQosData() &&
             (psduIt->psduMap.cbegin()->second->GetHeader(0).GetAddr2() == m_apMac->GetAddress())),
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
        NS_TEST_EXPECT_MSG_EQ(
            (psduIt->psduMap.cbegin()->second->GetHeader(0).IsCfEnd() &&
             (psduIt->psduMap.cbegin()->second->GetHeader(0).GetAddr2() == m_apMac->GetAddress())),
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

    for (std::size_t i = 0; i < m_nDsoStas; ++i)
    {
        const auto it = m_dsoTxopEventInfos.find(i);
        NS_TEST_ASSERT_MSG_EQ((it != m_dsoTxopEventInfos.cend()),
                              true,
                              "No DSO TXOP termination event for STA " << i + 1);
        // per STA, 3 events are expected: reception of ICF, transmission of ICR and TXOP
        // termination
        NS_TEST_ASSERT_MSG_EQ(it->second.size(),
                              3,
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

    // SU as long as steady conditions are not reached
    auto muScheduler =
        DynamicCast<TestDsoMultiUserScheduler>(m_apMac->GetObject<MultiUserScheduler>());
    muScheduler->SetForcedFormat(MultiUserScheduler::TxFormat::SU_TX);

    m_apMac->GetQosTxop(AC_BE)->SetTxopLimit(MicroSeconds(1600));
    for (std::size_t i = 0; i < m_nDsoStas; ++i)
    {
        m_staMacs.at(i)
            ->GetWifiPhy(SINGLE_LINK_OP_ID)
            ->SetAttribute("ChannelSwitchDelay", TimeValue(m_params.switchingDelayToPrimary));

        m_staMacs.at(i)->GetDsoManager()->SetAttribute("ChSwitchToDsoBandDelay",
                                                       TimeValue(m_params.switchingDelayToDso));
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
    AddTestCase(new DsoTxopTest({.testName = "Check DSO basic frame exchange sequence with "
                                             "single protection enabled",
                                 .numDlMuPpdus = 1,
                                 .protectSingleExchange = true,
                                 .expectedTxopEndEventInDsoSubband = DsoTxopEvent::TIMEOUT}),
                TestCase::Duration::QUICK);

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
        new DsoTxopTest({.testName = "Check DSO basic frame exchange sequence with CF-END to "
                                     "indicate end of TXOP",
                         .numDlMuPpdus = 1,
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
    AddTestCase(new DsoTxopTest({.testName = "Check DSO operations with multiple downlink "
                                             "transmissions",
                                 .numDlMuPpdus = 2,
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
    AddTestCase(new DsoTxopTest({.testName = "Check DSO operations with SIFS bursting",
                                 .numDlMuPpdus = 2,
                                 .protectSingleExchange = true,
                                 .expectedTxopEndEventInDsoSubband = DsoTxopEvent::TIMEOUT}),
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
    AddTestCase(new DsoTxopTest({.testName = "Check DSO operations with no DL MU PPDU after "
                                             "ICF response",
                                 .numDlMuPpdus = 0,
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
    AddTestCase(new DsoTxopTest({.testName = "Check DSO operations with truncated TXOP",
                                 .numDlMuPpdus = 0,
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
    AddTestCase(new DsoTxopTest({.testName = "Check DSO operations with downlink transmissions "
                                             "covering the whole TXOP duration",
                                 .numDlMuPpdus = 8,
                                 .expectedTxopEndEventInDsoSubband = DsoTxopEvent::TIMEOUT}),
                TestCase::Duration::QUICK);

    /**
     *             ┌──────┐          ┌──────┐          ┌──┐┌──────┐
     *             │ BSRP │          │Basic │          │BA││      │
     *  [AP]       │  TF  │          │  TF  │          ├──┤│CF-END│
     *             │(ICF) │          │      │          │BA││      │
     *  ───────────┴──────▼┬────────┬┴──────┴┬────────┬┴──┴┴──────▼─ ...
     *  [DSO STA]         ││QoS Null│        │QoS DATA│           │
     *                    ││(ICR)   │        │        │           │
     *                    │├────────┤        ├────────┤           │
     *  [UHR STA]         ││QoS Null│        │QoS DATA│           │
     *                    ││(ICR)   │        │        │           │
     *                    │└────────┘        └────────┘           │
     *                    │                                       │
     *                  switch                                switch back
     *                  to DSO                                to primary
     *
     */
    AddTestCase(new DsoTxopTest({.testName = "Check DSO operations with trigger-based uplink "
                                             "transmission",
                                 .numUlMuPpdus = 1,
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
}

static WifiDsoTestSuite g_wifiDsoTestSuite; ///< the test suite
