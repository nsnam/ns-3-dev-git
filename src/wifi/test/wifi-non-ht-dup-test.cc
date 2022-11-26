/*
 * Copyright (c) 2022
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/ap-wifi-mac.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/he-phy.h"
#include "ns3/interference-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/node.h"
#include "ns3/non-communicating-net-device.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/test.h"
#include "ns3/waveform-generator.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-spectrum-signal-parameters.h"
#include "ns3/wifi-spectrum-value-helper.h"
#include "ns3/wifi-utils.h"

#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiNonHtDuplicateTest");

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief non-HT duplicate PHY reception test
 * The test consists in an AP sending a single non-HT duplicate PPDU
 * of a given channel width (multiple of 20 MHz) over a spectrum
 * channel and it checks whether the STAs attached to the channel
 * receive the PPDU. If an interference is injected on a given 20 MHz
 * subchannel, the payload reception should fail, otherwise it should succeed.
 */
class TestNonHtDuplicatePhyReception : public TestCase
{
  public:
    /// A vector containing parameters per STA: the standard, the center frequency and the P20 index
    using StasParams = std::vector<std::tuple<WifiStandard, uint16_t, uint8_t>>;

    /**
     * Constructor
     * \param apStandard the standard to use for the AP
     * \param apFrequency the center frequency of the AP (in MHz)
     * \param apP20Index the index of the primary 20 MHz channel of the AP
     * \param stasParams the parameters of the STAs (\see StasParams)
     * \param per20MhzInterference flags per 20 MHz subchannel whether an interference should be
     * generated on that subchannel. An empty vector means that the test will not generate any
     * interference.
     */
    TestNonHtDuplicatePhyReception(WifiStandard apStandard,
                                   uint16_t apFrequency,
                                   uint8_t apP20Index,
                                   StasParams stasParams,
                                   std::vector<bool> per20MhzInterference = {});

  private:
    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     * Receive success function
     * \param index index of the RX STA
     * \param psdu the PSDU
     * \param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * \param txVector the transmit vector
     * \param statusPerMpdu reception status per MPDU
     */
    void RxSuccess(std::size_t index,
                   Ptr<const WifiPsdu> psdu,
                   RxSignalInfo rxSignalInfo,
                   WifiTxVector txVector,
                   std::vector<bool> statusPerMpdu);

    /**
     * Receive failure function
     * \param index index of the RX STA
     * \param psdu the PSDU
     */
    void RxFailure(std::size_t index, Ptr<const WifiPsdu> psdu);

    /**
     * Check the results
     * \param index index of the RX STA
     * \param expectedRxSuccess the expected number of RX success
     * \param expectedRxFailure the expected number of RX failures
     */
    void CheckResults(std::size_t index, uint32_t expectedRxSuccess, uint32_t expectedRxFailure);

    /**
     * Reset the results
     */
    void ResetResults();

    /**
     * Send non-HT duplicate PPDU function
     * \param channelWidth the channel width to use to transmit the non-HT PPDU (in MHz)
     */
    void SendNonHtDuplicatePpdu(uint16_t channelWidth);

    /**
     * Generate interference function
     * \param interferer the PHY of the interferer to use to generate the signal
     * \param interferencePsd the PSD of the interference to be generated
     * \param duration the duration of the interference
     */
    void GenerateInterference(Ptr<WaveformGenerator> interferer,
                              Ptr<SpectrumValue> interferencePsd,
                              Time duration);
    /**
     * Stop interference function
     * \param interferer the PHY of the interferer that was used to generate the signal
     */
    void StopInterference(Ptr<WaveformGenerator> interferer);

    WifiStandard m_apStandard; ///< the standard to use for the AP
    uint16_t m_apFrequency;    ///< the center frequency of the AP (in MHz)
    uint8_t m_apP20Index;      ///< the index of the primary 20 MHz channel of the AP
    StasParams m_stasParams;   ///< the parameters of the STAs
    std::vector<bool>
        m_per20MhzInterference; ///< flags per 20 MHz subchannel whether an interference should be
                                ///< generated on that subchannel

    std::vector<uint32_t> m_countRxSuccessStas; ///< count RX success for STAs
    std::vector<uint32_t> m_countRxFailureStas; ///< count RX failure for STAs

    Ptr<SpectrumWifiPhy> m_phyAp;                ///< PHY of AP
    std::vector<Ptr<SpectrumWifiPhy>> m_phyStas; ///< PHYs of STAs

    std::vector<Ptr<WaveformGenerator>>
        m_phyInterferers; ///< PHYs of interferers (1 interferer per 20 MHz subchannel)
};

TestNonHtDuplicatePhyReception::TestNonHtDuplicatePhyReception(
    WifiStandard apStandard,
    uint16_t apFrequency,
    uint8_t apP20Index,
    StasParams stasParams,
    std::vector<bool> per20MhzInterference)
    : TestCase{"non-HT duplicate PHY reception test"},
      m_apStandard{apStandard},
      m_apFrequency{apFrequency},
      m_apP20Index{apP20Index},
      m_stasParams{stasParams},
      m_per20MhzInterference{per20MhzInterference},
      m_countRxSuccessStas{},
      m_countRxFailureStas{},
      m_phyStas{}
{
}

void
TestNonHtDuplicatePhyReception::ResetResults()
{
    for (auto& countRxSuccess : m_countRxSuccessStas)
    {
        countRxSuccess = 0;
    }
    for (auto& countRxFailure : m_countRxFailureStas)
    {
        countRxFailure = 0;
    }
}

void
TestNonHtDuplicatePhyReception::SendNonHtDuplicatePpdu(uint16_t channelWidth)
{
    NS_LOG_FUNCTION(this << channelWidth);
    WifiTxVector txVector = WifiTxVector(OfdmPhy::GetOfdmRate24Mbps(),
                                         0,
                                         WIFI_PREAMBLE_LONG,
                                         800,
                                         1,
                                         1,
                                         0,
                                         channelWidth,
                                         false);

    Ptr<Packet> pkt = Create<Packet>(1000);
    WifiMacHeader hdr;

    hdr.SetType(WIFI_MAC_QOSDATA);
    hdr.SetQosTid(0);

    Ptr<WifiPsdu> psdu = Create<WifiPsdu>(pkt, hdr);
    Time txDuration =
        m_phyAp->CalculateTxDuration(psdu->GetSize(), txVector, m_phyAp->GetPhyBand());

    m_phyAp->Send(WifiConstPsduMap({std::make_pair(SU_STA_ID, psdu)}), txVector);
}

void
TestNonHtDuplicatePhyReception::GenerateInterference(Ptr<WaveformGenerator> interferer,
                                                     Ptr<SpectrumValue> interferencePsd,
                                                     Time duration)
{
    NS_LOG_FUNCTION(this << interferer << duration);
    interferer->SetTxPowerSpectralDensity(interferencePsd);
    interferer->SetPeriod(duration);
    interferer->Start();
    Simulator::Schedule(duration,
                        &TestNonHtDuplicatePhyReception::StopInterference,
                        this,
                        interferer);
}

void
TestNonHtDuplicatePhyReception::StopInterference(Ptr<WaveformGenerator> interferer)
{
    NS_LOG_FUNCTION(this << interferer);
    interferer->Stop();
}

void
TestNonHtDuplicatePhyReception::RxSuccess(std::size_t index,
                                          Ptr<const WifiPsdu> psdu,
                                          RxSignalInfo rxSignalInfo,
                                          WifiTxVector txVector,
                                          std::vector<bool> /*statusPerMpdu*/)
{
    NS_LOG_FUNCTION(this << index << *psdu << rxSignalInfo << txVector);
    m_countRxSuccessStas.at(index)++;
}

void
TestNonHtDuplicatePhyReception::RxFailure(std::size_t index, Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << index << *psdu);
    m_countRxFailureStas.at(index)++;
}

void
TestNonHtDuplicatePhyReception::CheckResults(std::size_t index,
                                             uint32_t expectedRxSuccess,
                                             uint32_t expectedRxFailure)
{
    NS_LOG_FUNCTION(this << index << expectedRxSuccess << expectedRxFailure);
    NS_TEST_ASSERT_MSG_EQ(m_countRxSuccessStas.at(index),
                          expectedRxSuccess,
                          "The number of successfully received packets by STA "
                              << index << " is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxFailureStas.at(index),
                          expectedRxFailure,
                          "The number of unsuccessfully received packets by STA "
                              << index << " is not correct!");
}

void
TestNonHtDuplicatePhyReception::DoSetup()
{
    auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    auto lossModel = CreateObject<FriisPropagationLossModel>();
    lossModel->SetFrequency(m_apFrequency * 1e6);
    spectrumChannel->AddPropagationLossModel(lossModel);
    auto delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);

    auto apNode = CreateObject<Node>();
    auto apDev = CreateObject<WifiNetDevice>();
    m_phyAp = CreateObject<SpectrumWifiPhy>();
    m_phyAp->CreateWifiSpectrumPhyInterface(apDev);
    auto apInterferenceHelper = CreateObject<InterferenceHelper>();
    m_phyAp->SetInterferenceHelper(apInterferenceHelper);
    auto apErrorModel = CreateObject<NistErrorRateModel>();
    m_phyAp->SetErrorRateModel(apErrorModel);
    m_phyAp->SetDevice(apDev);
    m_phyAp->SetChannel(spectrumChannel);
    m_phyAp->ConfigureStandard(WIFI_STANDARD_80211ax);
    auto apMobility = CreateObject<ConstantPositionMobilityModel>();
    m_phyAp->SetMobility(apMobility);
    apDev->SetPhy(m_phyAp);
    apNode->AggregateObject(apMobility);
    apNode->AddDevice(apDev);

    for (const auto& staParams : m_stasParams)
    {
        auto staNode = CreateObject<Node>();
        auto staDev = CreateObject<WifiNetDevice>();
        auto staPhy = CreateObject<SpectrumWifiPhy>();
        staPhy->CreateWifiSpectrumPhyInterface(staDev);
        auto sta1InterferenceHelper = CreateObject<InterferenceHelper>();
        staPhy->SetInterferenceHelper(sta1InterferenceHelper);
        auto sta1ErrorModel = CreateObject<NistErrorRateModel>();
        staPhy->SetErrorRateModel(sta1ErrorModel);
        staPhy->SetDevice(staDev);
        staPhy->SetChannel(spectrumChannel);
        staPhy->ConfigureStandard(std::get<0>(staParams));
        staPhy->SetReceiveOkCallback(
            MakeCallback(&TestNonHtDuplicatePhyReception::RxSuccess, this).Bind(m_phyStas.size()));
        staPhy->SetReceiveErrorCallback(
            MakeCallback(&TestNonHtDuplicatePhyReception::RxFailure, this).Bind(m_phyStas.size()));
        auto staMobility = CreateObject<ConstantPositionMobilityModel>();
        staPhy->SetMobility(staMobility);
        staDev->SetPhy(staPhy);
        staNode->AggregateObject(staMobility);
        staNode->AddDevice(staDev);
        m_phyStas.push_back(staPhy);
        m_countRxSuccessStas.push_back(0);
        m_countRxFailureStas.push_back(0);
    }

    if (!m_per20MhzInterference.empty())
    {
        [[maybe_unused]] auto [channelNum, centerFreq, apChannelWidth, type, phyBand] =
            (*WifiPhyOperatingChannel::FindFirst(0,
                                                 m_apFrequency,
                                                 0,
                                                 m_apStandard,
                                                 WIFI_PHY_BAND_5GHZ));
        NS_ASSERT(m_per20MhzInterference.size() == (apChannelWidth / 20));
        for (std::size_t i = 0; i < m_per20MhzInterference.size(); ++i)
        {
            auto interfererNode = CreateObject<Node>();
            auto interfererDev = CreateObject<NonCommunicatingNetDevice>();
            auto phyInterferer = CreateObject<WaveformGenerator>();
            phyInterferer->SetDevice(interfererDev);
            phyInterferer->SetChannel(spectrumChannel);
            phyInterferer->SetDutyCycle(1);
            interfererNode->AddDevice(interfererDev);
            m_phyInterferers.push_back(phyInterferer);
        }
    }
}

void
TestNonHtDuplicatePhyReception::DoTeardown()
{
    m_phyAp->Dispose();
    m_phyAp = nullptr;
    for (auto phySta : m_phyStas)
    {
        phySta->Dispose();
        phySta = nullptr;
    }
    for (auto phyInterferer : m_phyInterferers)
    {
        phyInterferer->Dispose();
        phyInterferer = nullptr;
    }
}

void
TestNonHtDuplicatePhyReception::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 0;
    m_phyAp->AssignStreams(streamNumber);
    for (auto phySta : m_phyStas)
    {
        phySta->AssignStreams(streamNumber);
    }

    [[maybe_unused]] auto [apChannelNum, centerFreq, apChannelWidth, type, phyBand] =
        (*WifiPhyOperatingChannel::FindFirst(0,
                                             m_apFrequency,
                                             0,
                                             m_apStandard,
                                             WIFI_PHY_BAND_5GHZ));
    m_phyAp->SetOperatingChannel(
        WifiPhy::ChannelTuple{apChannelNum, apChannelWidth, WIFI_PHY_BAND_5GHZ, m_apP20Index});

    auto index = 0;
    for (const auto& [staStandard, staFrequency, staP20Index] : m_stasParams)
    {
        [[maybe_unused]] auto [staChannelNum, centerFreq, staChannelWidth, type, phyBand] =
            (*WifiPhyOperatingChannel::FindFirst(0,
                                                 staFrequency,
                                                 0,
                                                 staStandard,
                                                 WIFI_PHY_BAND_5GHZ));
        m_phyStas.at(index++)->SetOperatingChannel(
            WifiPhy::ChannelTuple{staChannelNum, staChannelWidth, WIFI_PHY_BAND_5GHZ, staP20Index});
    }

    index = 0;
    const auto minApCenterFrequency =
        m_phyAp->GetFrequency() - (m_phyAp->GetChannelWidth() / 2) + (20 / 2);
    for (auto channelWidth = 20; channelWidth <= apChannelWidth; channelWidth *= 2, ++index)
    {
        if (!m_phyInterferers.empty())
        {
            for (std::size_t i = 0; i < m_phyInterferers.size(); ++i)
            {
                if (!m_per20MhzInterference.at(i))
                {
                    continue;
                }
                BandInfo bandInfo;
                bandInfo.fc = (minApCenterFrequency + (i * 20)) * 1e6;
                bandInfo.fl = bandInfo.fc - (5 * 1e6);
                bandInfo.fh = bandInfo.fc + (5 * 1e6);
                Bands bands;
                bands.push_back(bandInfo);
                auto spectrumInterference = Create<SpectrumModel>(bands);
                auto interferencePsd = Create<SpectrumValue>(spectrumInterference);
                auto interferencePower = 0.005; // in watts (designed to make PHY headers reception
                                                // successful but payload reception fail)
                *interferencePsd = interferencePower / 10e6;
                Simulator::Schedule(Seconds(index),
                                    &TestNonHtDuplicatePhyReception::GenerateInterference,
                                    this,
                                    m_phyInterferers.at(i),
                                    interferencePsd,
                                    Seconds(0.5));
            }
        }
        const auto apCenterFreq =
            m_phyAp->GetOperatingChannel().GetPrimaryChannelCenterFrequency(channelWidth);
        const auto apMinFreq = apCenterFreq - (channelWidth / 2);
        const auto apMaxFreq = apCenterFreq + (channelWidth / 2);
        Simulator::Schedule(Seconds(index + 0.1),
                            &TestNonHtDuplicatePhyReception::SendNonHtDuplicatePpdu,
                            this,
                            channelWidth);
        for (std::size_t i = 0; i < m_stasParams.size(); ++i)
        {
            const auto p20Width = 20;
            const auto staP20Freq =
                m_phyStas.at(i)->GetOperatingChannel().GetPrimaryChannelCenterFrequency(p20Width);
            const auto staP20MinFreq = staP20Freq - (p20Width / 2);
            const auto staP20MaxFreq = staP20Freq + (p20Width / 2);
            bool expectRx = (staP20MinFreq >= apMinFreq && staP20MaxFreq <= apMaxFreq);
            bool expectSuccess = true;
            if (!m_per20MhzInterference.empty())
            {
                const auto index20MhzSubBand = ((staP20Freq - minApCenterFrequency) / 20);
                expectSuccess = !m_per20MhzInterference.at(index20MhzSubBand);
            }
            Simulator::Schedule(Seconds(index + 0.5),
                                &TestNonHtDuplicatePhyReception::CheckResults,
                                this,
                                i,
                                expectRx ? expectSuccess : 0,
                                expectRx ? !expectSuccess : 0);
        }
        Simulator::Schedule(Seconds(index + 0.5),
                            &TestNonHtDuplicatePhyReception::ResetResults,
                            this);
    }

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief wifi non-HT duplicate Test Suite
 */
class WifiNonHtDuplicateTestSuite : public TestSuite
{
  public:
    WifiNonHtDuplicateTestSuite();
};

WifiNonHtDuplicateTestSuite::WifiNonHtDuplicateTestSuite()
    : TestSuite("wifi-non-ht-dup", UNIT)
{
    /**
     * Channel map:
     *
     *                | 20MHz  | 20MHz  | 20MHz  | 20MHz  |
     *
     *                ┌────────┬────────┬────────┬────────┐
     *  AP 802.11ax   │CH 36(P)│ CH 40  │ CH 44  │ CH 48  │
     *                └────────┴────────┴────────┴────────┘
     *
     *                ┌────────┐
     *  STA1 802.11a  │ CH 36  │
     *                └────────┘
     *
     *                         ┌────────┐
     *  STA2 802.11n           │ CH 40  │
     *                         └────────┘
     *
     *                                  ┌────────┬────────┐
     *  STA3 802.11ac                   │CH 44(P)│ CH 48  │
     *                                  └────────┴────────┘
     *
     * Test scenario:
     *                ┌────────┐       ┌──────────────────────┐
     *                │        │       │RX non-HT PPDU @ STA 1│
     *                │ 80 MHz │       └──────────────────────┘
     *                │ non-HT │       ┌──────────────────────┐
     *                │  PPDU  │       │RX non-HT PPDU @ STA 2│
     *                │  sent  │       └──────────────────────┘
     *                │  from  │       ┌──────────────────────┐
     *                │   AP   │       │                      │
     *                │        │       │RX non-HT PPDU @ STA 3│
     *                │        │       │                      │
     *                └────────┘       └──────────────────────┘
     */
    AddTestCase(new TestNonHtDuplicatePhyReception(WIFI_STANDARD_80211ax,
                                                   5210,
                                                   0,
                                                   {{WIFI_STANDARD_80211a, 5180, 0},
                                                    {WIFI_STANDARD_80211n, 5200, 0},
                                                    {WIFI_STANDARD_80211ac, 5230, 0}}),
                TestCase::QUICK);
    /* same channel map and test scenario as previously but inject interference on channel 40 */
    AddTestCase(new TestNonHtDuplicatePhyReception(WIFI_STANDARD_80211ax,
                                                   5210,
                                                   0,
                                                   {{WIFI_STANDARD_80211a, 5180, 0},
                                                    {WIFI_STANDARD_80211n, 5200, 0},
                                                    {WIFI_STANDARD_80211ac, 5230, 0}},
                                                   {false, true, false, false}),
                TestCase::QUICK);
}

static WifiNonHtDuplicateTestSuite wifiNonHtDuplicateTestSuite; ///< the test suite
