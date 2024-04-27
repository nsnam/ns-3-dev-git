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
#include "ns3/boolean.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/he-configuration.h"
#include "ns3/he-phy.h"
#include "ns3/interference-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/node.h"
#include "ns3/non-communicating-net-device.h"
#include "ns3/pointer.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/txop.h"
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

constexpr uint32_t DEFAULT_FREQUENCY = 5180; // MHz

/**
 * HE PHY used for testing MU-RTS/CTS.
 */
class MuRtsCtsHePhy : public HePhy
{
  public:
    MuRtsCtsHePhy();
    ~MuRtsCtsHePhy() override;

    /**
     * Set the previous TX PPDU UID counter.
     *
     * \param uid the value to which the previous TX PPDU UID counter should be set
     */
    void SetPreviousTxPpduUid(uint64_t uid);

    /**
     * Set the TXVECTOR of the previously transmitted MU-RTS.
     *
     * \param muRtsTxVector the TXVECTOR used to transmit MU-RTS trigger frame
     */
    void SetMuRtsTxVector(const WifiTxVector& muRtsTxVector);
}; // class MuRtsCtsHePhy

MuRtsCtsHePhy::MuRtsCtsHePhy()
    : HePhy()
{
    NS_LOG_FUNCTION(this);
}

MuRtsCtsHePhy::~MuRtsCtsHePhy()
{
    NS_LOG_FUNCTION(this);
}

void
MuRtsCtsHePhy::SetPreviousTxPpduUid(uint64_t uid)
{
    NS_LOG_FUNCTION(this << uid);
    m_previouslyTxPpduUid = uid;
}

void
MuRtsCtsHePhy::SetMuRtsTxVector(const WifiTxVector& muRtsTxVector)
{
    NS_LOG_FUNCTION(this << muRtsTxVector);
    m_currentTxVector = muRtsTxVector;
}

/**
 * Spectrum PHY used for testing MU-RTS/CTS.
 */
class MuRtsCtsSpectrumWifiPhy : public SpectrumWifiPhy
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    MuRtsCtsSpectrumWifiPhy();
    ~MuRtsCtsSpectrumWifiPhy() override;

    void DoInitialize() override;
    void DoDispose() override;

    /**
     * Set the global PPDU UID counter.
     *
     * \param uid the value to which the global PPDU UID counter should be set
     */
    void SetPpduUid(uint64_t uid);

    /**
     * Set the TXVECTOR of the previously transmitted MU-RTS.
     *
     * \param muRtsTxVector the TXVECTOR used to transmit MU-RTS trigger frame
     */
    void SetMuRtsTxVector(const WifiTxVector& muRtsTxVector);

  private:
    Ptr<MuRtsCtsHePhy> m_muRtsCtsHePhy; ///< Pointer to HE PHY instance used for MU-RTS/CTS PHY test
};                                      // class MuRtsCtsSpectrumWifiPhy

TypeId
MuRtsCtsSpectrumWifiPhy::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::MuRtsCtsSpectrumWifiPhy").SetParent<SpectrumWifiPhy>().SetGroupName("Wifi");
    return tid;
}

MuRtsCtsSpectrumWifiPhy::MuRtsCtsSpectrumWifiPhy()
    : SpectrumWifiPhy()
{
    NS_LOG_FUNCTION(this);
    m_muRtsCtsHePhy = Create<MuRtsCtsHePhy>();
    m_muRtsCtsHePhy->SetOwner(this);
}

MuRtsCtsSpectrumWifiPhy::~MuRtsCtsSpectrumWifiPhy()
{
    NS_LOG_FUNCTION(this);
}

void
MuRtsCtsSpectrumWifiPhy::DoInitialize()
{
    // Replace HE PHY instance with test instance
    m_phyEntities[WIFI_MOD_CLASS_HE] = m_muRtsCtsHePhy;
    SpectrumWifiPhy::DoInitialize();
}

void
MuRtsCtsSpectrumWifiPhy::DoDispose()
{
    m_muRtsCtsHePhy = nullptr;
    SpectrumWifiPhy::DoDispose();
}

void
MuRtsCtsSpectrumWifiPhy::SetPpduUid(uint64_t uid)
{
    NS_LOG_FUNCTION(this << uid);
    m_muRtsCtsHePhy->SetPreviousTxPpduUid(uid);
    m_previouslyRxPpduUid = uid;
}

void
MuRtsCtsSpectrumWifiPhy::SetMuRtsTxVector(const WifiTxVector& muRtsTxVector)
{
    NS_LOG_FUNCTION(this << muRtsTxVector);
    m_muRtsCtsHePhy->SetMuRtsTxVector(muRtsTxVector);
}

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
    const auto expectedWidth =
        std::min(m_phyAp->GetChannelWidth(), m_phyStas.at(index)->GetChannelWidth());
    NS_TEST_ASSERT_MSG_EQ(txVector.GetChannelWidth(),
                          expectedWidth,
                          "Incorrect channel width in TXVECTOR");
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
    auto apInterferenceHelper = CreateObject<InterferenceHelper>();
    m_phyAp->SetInterferenceHelper(apInterferenceHelper);
    auto apErrorModel = CreateObject<NistErrorRateModel>();
    m_phyAp->SetErrorRateModel(apErrorModel);
    m_phyAp->SetDevice(apDev);
    m_phyAp->AddChannel(spectrumChannel);
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
        auto sta1InterferenceHelper = CreateObject<InterferenceHelper>();
        staPhy->SetInterferenceHelper(sta1InterferenceHelper);
        auto sta1ErrorModel = CreateObject<NistErrorRateModel>();
        staPhy->SetErrorRateModel(sta1ErrorModel);
        staPhy->SetDevice(staDev);
        staPhy->AddChannel(spectrumChannel);
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
 * \brief test PHY reception of multiple CTS frames as a response to a MU-RTS frame.
 * The test is checking whether the reception of multiple identical CTS frames as a response to a
 * MU-RTS frame is successfully received by the AP PHY and that only a single CTS frame is forwarded
 * up to the MAC. Since the test is focusing on the PHY reception of multiple CTS response, the
 * transmission of the MU-RTS frame is faked. The test also checks the correct channel width is
 * passed to the MAC layer through the TXVECTOR. The test also consider the case some STAs do not
 * respond to verify the largest channel width of the successfully CTS responses is reported to the
 * MAC.
 */
class TestMultipleCtsResponsesFromMuRts : public TestCase
{
  public:
    /// Information about CTS responses to expect in the test
    struct CtsTxInfos
    {
        uint16_t bw{20};     ///< the width in MHz of the CTS response
        bool discard{false}; ///< flag whether the CTS response shall be discarded
    };

    /**
     * Constructor
     * \param ctsTxInfosPerSta the information about CTS responses to generate
     */
    TestMultipleCtsResponsesFromMuRts(const std::vector<CtsTxInfos>& ctsTxInfosPerSta);

  private:
    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     * Function called to fake the transmission of a MU-RTS.
     */
    void FakePreviousMuRts();

    /**
     * Function called to trigger a CTS frame sent by a STA using non-HT duplicate.
     *
     * \param phyIndex the index of the TX PHY
     */
    void TxNonHtDuplicateCts(std::size_t phyIndex);

    /**
     * CTS RX success function
     * \param phyIndex the index of the PHY (0 for AP)
     * \param psdu the PSDU
     * \param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * \param txVector the transmit vector
     * \param statusPerMpdu reception status per MPDU
     */
    void RxCtsSuccess(std::size_t phyIndex,
                      Ptr<const WifiPsdu> psdu,
                      RxSignalInfo rxSignalInfo,
                      WifiTxVector txVector,
                      std::vector<bool> statusPerMpdu);

    /**
     * CTS RX failure function
     * \param phyIndex the index of the PHY (0 for AP)
     * \param psdu the PSDU
     */
    void RxCtsFailure(std::size_t phyIndex, Ptr<const WifiPsdu> psdu);

    /**
     * Check the results
     */
    void CheckResults();

    Ptr<MuRtsCtsSpectrumWifiPhy> m_phyAp;                ///< AP PHY
    std::vector<Ptr<MuRtsCtsSpectrumWifiPhy>> m_phyStas; ///< STAs PHYs

    std::vector<CtsTxInfos> m_ctsTxInfosPerSta; ///< information about CTS responses

    std::size_t
        m_countApRxCtsSuccess; ///< count the number of successfully received CTS frames by the AP
    std::size_t
        m_countApRxCtsFailure; ///< count the number of unsuccessfully received CTS frames by the AP
    std::size_t m_countStaRxCtsSuccess; ///< count the number of successfully received CTS frames by
                                        ///< the non-participating STA
    std::size_t m_countStaRxCtsFailure; ///< count the number of unsuccessfully received CTS frames
                                        ///< by the non-participating STA

    double m_stasTxPowerDbm; ///< TX power in dBm configured for the STAs
};

TestMultipleCtsResponsesFromMuRts::TestMultipleCtsResponsesFromMuRts(
    const std::vector<CtsTxInfos>& ctsTxInfosPerSta)
    : TestCase{"test PHY reception of multiple CTS frames following a MU-RTS frame"},
      m_ctsTxInfosPerSta{ctsTxInfosPerSta},
      m_countApRxCtsSuccess{0},
      m_countApRxCtsFailure{0},
      m_countStaRxCtsSuccess{0},
      m_countStaRxCtsFailure{0},
      m_stasTxPowerDbm(10.0)
{
}

void
TestMultipleCtsResponsesFromMuRts::FakePreviousMuRts()
{
    NS_LOG_FUNCTION(this);

    const auto bw =
        std::max_element(m_ctsTxInfosPerSta.cbegin(),
                         m_ctsTxInfosPerSta.cend(),
                         [](const auto& lhs, const auto& rhs) { return lhs.bw < rhs.bw; })
            ->bw;
    WifiTxVector txVector;
    txVector.SetChannelWidth(bw); // only the channel width matters for this test

    // set the TXVECTOR and the UID of the previously transmitted MU-RTS in the AP PHY
    m_phyAp->SetMuRtsTxVector(txVector);
    m_phyAp->SetPpduUid(0);

    // set the UID of the previously received MU-RTS in the STAs PHYs
    for (auto& phySta : m_phyStas)
    {
        phySta->SetPpduUid(0);
    }
}

void
TestMultipleCtsResponsesFromMuRts::TxNonHtDuplicateCts(std::size_t phyIndex)
{
    const auto bw = m_ctsTxInfosPerSta.at(phyIndex).bw;
    const auto discarded = m_ctsTxInfosPerSta.at(phyIndex).discard;
    NS_LOG_FUNCTION(this << phyIndex << bw << discarded);

    if (discarded)
    {
        return;
    }

    WifiTxVector txVector =
        WifiTxVector(OfdmPhy::GetOfdmRate54Mbps(), // use less robust modulation for test purpose
                     0,
                     WIFI_PREAMBLE_LONG,
                     800,
                     1,
                     1,
                     0,
                     bw,
                     false,
                     false);
    txVector.SetTriggerResponding(true);

    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_CTL_CTS);
    hdr.SetDsNotFrom();
    hdr.SetDsNotTo();
    hdr.SetNoMoreFragments();
    hdr.SetNoRetry();

    auto pkt = Create<Packet>();
    auto mpdu = Create<WifiMpdu>(pkt, hdr);
    auto psdu = Create<WifiPsdu>(mpdu, false);

    m_phyStas.at(phyIndex)->Send(psdu, txVector);
}

void
TestMultipleCtsResponsesFromMuRts::RxCtsSuccess(std::size_t phyIndex,
                                                Ptr<const WifiPsdu> psdu,
                                                RxSignalInfo rxSignalInfo,
                                                WifiTxVector txVector,
                                                std::vector<bool> /*statusPerMpdu*/)
{
    NS_LOG_FUNCTION(this << phyIndex << *psdu << rxSignalInfo << txVector);
    std::vector<CtsTxInfos> successfulCtsInfos{};
    std::copy_if(m_ctsTxInfosPerSta.cbegin(),
                 m_ctsTxInfosPerSta.cend(),
                 std::back_inserter(successfulCtsInfos),
                 [](const auto& info) { return !info.discard; });
    const auto isAp = (phyIndex == 0);
    if (isAp)
    {
        NS_TEST_EXPECT_MSG_EQ_TOL(rxSignalInfo.rssi,
                                  WToDbm(DbmToW(m_stasTxPowerDbm) * successfulCtsInfos.size()),
                                  0.1,
                                  "RX power is not correct!");
    }
    auto expectedWidth =
        std::max_element(successfulCtsInfos.cbegin(),
                         successfulCtsInfos.cend(),
                         [](const auto& lhs, const auto& rhs) { return lhs.bw < rhs.bw; })
            ->bw;
    if (!isAp)
    {
        expectedWidth = std::min(m_ctsTxInfosPerSta.at(phyIndex - 1).bw, expectedWidth);
    }
    NS_TEST_ASSERT_MSG_EQ(txVector.GetChannelWidth(),
                          expectedWidth,
                          "Incorrect channel width in TXVECTOR");
    if (isAp)
    {
        m_countApRxCtsSuccess++;
    }
    else
    {
        m_countStaRxCtsSuccess++;
    }
}

void
TestMultipleCtsResponsesFromMuRts::RxCtsFailure(std::size_t phyIndex, Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << phyIndex << *psdu);
    const auto isAp = (phyIndex == 0);
    if (isAp)
    {
        m_countApRxCtsFailure++;
    }
    else
    {
        m_countStaRxCtsFailure++;
    }
}

void
TestMultipleCtsResponsesFromMuRts::CheckResults()
{
    NS_TEST_ASSERT_MSG_EQ(m_countApRxCtsSuccess,
                          1,
                          "The number of successfully received CTS frames by AP is not correct!");
    NS_TEST_ASSERT_MSG_EQ(
        m_countStaRxCtsSuccess,
        m_ctsTxInfosPerSta.size(),
        "The number of successfully received CTS frames by non-participating STAs is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countApRxCtsFailure,
                          0,
                          "The number of unsuccessfully received CTS frames by AP is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countStaRxCtsFailure,
                          0,
                          "The number of unsuccessfully received CTS frames by non-participating "
                          "STAs is not correct!");
}

void
TestMultipleCtsResponsesFromMuRts::DoSetup()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 0;

    auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    auto lossModel = CreateObject<FriisPropagationLossModel>();
    lossModel->SetFrequency(DEFAULT_FREQUENCY * 1e6);
    spectrumChannel->AddPropagationLossModel(lossModel);
    auto delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);

    auto apNode = CreateObject<Node>();
    auto apDev = CreateObject<WifiNetDevice>();
    auto apMac = CreateObjectWithAttributes<ApWifiMac>(
        "Txop",
        PointerValue(CreateObjectWithAttributes<Txop>("AcIndex", StringValue("AC_BE_NQOS"))));
    apMac->SetAttribute("BeaconGeneration", BooleanValue(false));
    apDev->SetMac(apMac);
    m_phyAp = CreateObject<MuRtsCtsSpectrumWifiPhy>();
    apDev->SetHeConfiguration(CreateObject<HeConfiguration>());
    auto apInterferenceHelper = CreateObject<InterferenceHelper>();
    m_phyAp->SetInterferenceHelper(apInterferenceHelper);
    auto apErrorModel = CreateObject<NistErrorRateModel>();
    m_phyAp->SetErrorRateModel(apErrorModel);
    m_phyAp->SetDevice(apDev);
    m_phyAp->AddChannel(spectrumChannel);
    m_phyAp->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phyAp->AssignStreams(streamNumber);

    m_phyAp->SetReceiveOkCallback(
        MakeCallback(&TestMultipleCtsResponsesFromMuRts::RxCtsSuccess, this).Bind(0));
    m_phyAp->SetReceiveErrorCallback(
        MakeCallback(&TestMultipleCtsResponsesFromMuRts::RxCtsFailure, this).Bind(0));

    const auto apBw =
        std::max_element(m_ctsTxInfosPerSta.cbegin(),
                         m_ctsTxInfosPerSta.cend(),
                         [](const auto& lhs, const auto& rhs) { return lhs.bw < rhs.bw; })
            ->bw;
    auto apChannelNum = std::get<0>(
        *WifiPhyOperatingChannel::FindFirst(0, 0, apBw, WIFI_STANDARD_80211ac, WIFI_PHY_BAND_5GHZ));

    m_phyAp->SetOperatingChannel(WifiPhy::ChannelTuple{apChannelNum, apBw, WIFI_PHY_BAND_5GHZ, 0});

    auto apMobility = CreateObject<ConstantPositionMobilityModel>();
    m_phyAp->SetMobility(apMobility);
    apDev->SetPhy(m_phyAp);
    apDev->SetStandard(WIFI_STANDARD_80211ax);
    apDev->SetHeConfiguration(CreateObject<HeConfiguration>());
    apMac->SetWifiPhys({m_phyAp});
    apNode->AggregateObject(apMobility);
    apNode->AddDevice(apDev);

    for (std::size_t i = 0; i < m_ctsTxInfosPerSta.size(); ++i)
    {
        auto staNode = CreateObject<Node>();
        auto staDev = CreateObject<WifiNetDevice>();
        auto phySta = CreateObject<MuRtsCtsSpectrumWifiPhy>();
        auto staInterferenceHelper = CreateObject<InterferenceHelper>();
        phySta->SetInterferenceHelper(staInterferenceHelper);
        auto staErrorModel = CreateObject<NistErrorRateModel>();
        phySta->SetErrorRateModel(staErrorModel);
        phySta->SetDevice(staDev);
        phySta->AddChannel(spectrumChannel);
        phySta->ConfigureStandard(WIFI_STANDARD_80211ax);
        phySta->AssignStreams(streamNumber);
        phySta->SetTxPowerStart(m_stasTxPowerDbm);
        phySta->SetTxPowerEnd(m_stasTxPowerDbm);

        auto channelNum =
            std::get<0>(*WifiPhyOperatingChannel::FindFirst(0,
                                                            0,
                                                            m_ctsTxInfosPerSta.at(i).bw,
                                                            WIFI_STANDARD_80211ac,
                                                            WIFI_PHY_BAND_5GHZ));

        phySta->SetOperatingChannel(
            WifiPhy::ChannelTuple{channelNum, m_ctsTxInfosPerSta.at(i).bw, WIFI_PHY_BAND_5GHZ, 0});

        auto staMobility = CreateObject<ConstantPositionMobilityModel>();
        phySta->SetMobility(staMobility);
        staDev->SetPhy(phySta);
        staDev->SetStandard(WIFI_STANDARD_80211ax);
        staDev->SetHeConfiguration(CreateObject<HeConfiguration>());
        staNode->AggregateObject(staMobility);
        staNode->AddDevice(staDev);
        m_phyStas.push_back(phySta);

        // non-participating HE STA
        auto nonParticipatingHeStaNode = CreateObject<Node>();
        auto nonParticipatingHeStaDev = CreateObject<WifiNetDevice>();
        auto nonParticipatingHePhySta = CreateObject<SpectrumWifiPhy>();
        auto nonParticipatingHeStaInterferenceHelper = CreateObject<InterferenceHelper>();
        nonParticipatingHePhySta->SetInterferenceHelper(nonParticipatingHeStaInterferenceHelper);
        auto nonParticipatingHeStaErrorModel = CreateObject<NistErrorRateModel>();
        nonParticipatingHePhySta->SetErrorRateModel(nonParticipatingHeStaErrorModel);
        nonParticipatingHePhySta->SetDevice(nonParticipatingHeStaDev);
        nonParticipatingHePhySta->AddChannel(spectrumChannel);
        nonParticipatingHePhySta->ConfigureStandard(WIFI_STANDARD_80211ax);

        nonParticipatingHePhySta->SetOperatingChannel(
            WifiPhy::ChannelTuple{channelNum, m_ctsTxInfosPerSta.at(i).bw, WIFI_PHY_BAND_5GHZ, 0});

        auto nonParticipatingHeStaMobility = CreateObject<ConstantPositionMobilityModel>();
        nonParticipatingHePhySta->SetMobility(nonParticipatingHeStaMobility);
        nonParticipatingHeStaDev->SetPhy(nonParticipatingHePhySta);
        nonParticipatingHeStaDev->SetStandard(WIFI_STANDARD_80211ax);
        nonParticipatingHeStaDev->SetHeConfiguration(CreateObject<HeConfiguration>());
        nonParticipatingHePhySta->AssignStreams(streamNumber);
        nonParticipatingHeStaNode->AggregateObject(nonParticipatingHeStaMobility);
        nonParticipatingHeStaNode->AddDevice(nonParticipatingHeStaDev);

        nonParticipatingHePhySta->SetReceiveOkCallback(
            MakeCallback(&TestMultipleCtsResponsesFromMuRts::RxCtsSuccess, this).Bind(i + 1));
        nonParticipatingHePhySta->SetReceiveErrorCallback(
            MakeCallback(&TestMultipleCtsResponsesFromMuRts::RxCtsFailure, this).Bind(i + 1));
    }

    // non-HE STA
    auto nonHeStaNode = CreateObject<Node>();
    auto nonHeStaDev = CreateObject<WifiNetDevice>();
    auto nonHePhySta = CreateObject<SpectrumWifiPhy>();
    auto nonHeStaInterferenceHelper = CreateObject<InterferenceHelper>();
    nonHePhySta->SetInterferenceHelper(nonHeStaInterferenceHelper);
    auto nonHeStaErrorModel = CreateObject<NistErrorRateModel>();
    nonHePhySta->SetErrorRateModel(nonHeStaErrorModel);
    nonHePhySta->SetDevice(nonHeStaDev);
    nonHePhySta->AddChannel(spectrumChannel);
    nonHePhySta->ConfigureStandard(WIFI_STANDARD_80211ac);
    nonHePhySta->SetOperatingChannel(
        WifiPhy::ChannelTuple{apChannelNum, apBw, WIFI_PHY_BAND_5GHZ, 0});
    auto nonHeStaMobility = CreateObject<ConstantPositionMobilityModel>();
    nonHePhySta->SetMobility(nonHeStaMobility);
    nonHeStaDev->SetPhy(nonHePhySta);
    nonHeStaDev->SetStandard(WIFI_STANDARD_80211ac);
    nonHePhySta->AssignStreams(streamNumber);
    nonHeStaNode->AggregateObject(nonHeStaMobility);
    nonHeStaNode->AddDevice(nonHeStaDev);
}

void
TestMultipleCtsResponsesFromMuRts::DoTeardown()
{
    m_phyAp->Dispose();
    m_phyAp = nullptr;
    for (auto& phySta : m_phyStas)
    {
        phySta->Dispose();
        phySta = nullptr;
    }
    m_phyStas.clear();
}

void
TestMultipleCtsResponsesFromMuRts::DoRun()
{
    // Fake transmission of a MU-RTS frame preceding the CTS responses
    Simulator::Schedule(Seconds(0.0), &TestMultipleCtsResponsesFromMuRts::FakePreviousMuRts, this);

    for (std::size_t index = 0; index < m_phyStas.size(); ++index)
    {
        // Transmit CTS responses over their operating bandwidth with 1 nanosecond delay between
        // each other
        const auto delay = (index + 1) * NanoSeconds(1.0);
        Simulator::Schedule(delay,
                            &TestMultipleCtsResponsesFromMuRts::TxNonHtDuplicateCts,
                            this,
                            index);
    }

    // Verify successful reception of the CTS frames: since multiple copies are sent
    // simultaneously, a single CTS frame should be forwarded up to the MAC.
    Simulator::Schedule(Seconds(1.0), &TestMultipleCtsResponsesFromMuRts::CheckResults, this);

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
    : TestSuite("wifi-non-ht-dup", Type::UNIT)
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
                TestCase::Duration::QUICK);
    /* same channel map and test scenario as previously but inject interference on channel 40 */
    AddTestCase(new TestNonHtDuplicatePhyReception(WIFI_STANDARD_80211ax,
                                                   5210,
                                                   0,
                                                   {{WIFI_STANDARD_80211a, 5180, 0},
                                                    {WIFI_STANDARD_80211n, 5200, 0},
                                                    {WIFI_STANDARD_80211ac, 5230, 0}},
                                                   {false, true, false, false}),
                TestCase::Duration::QUICK);
    /* test PHY reception of multiple CTS responses following a MU-RTS */
    /* 4 STAs operating on 20 MHz */
    AddTestCase(new TestMultipleCtsResponsesFromMuRts({{20}, {20}, {20}, {20}}),
                TestCase::Duration::QUICK);
    /* 4 STAs operating on 40 MHz */
    AddTestCase(new TestMultipleCtsResponsesFromMuRts({{40}, {40}, {40}, {40}}),
                TestCase::Duration::QUICK);
    /* 4 STAs operating on 80 MHz */
    AddTestCase(new TestMultipleCtsResponsesFromMuRts({{80}, {80}, {80}, {80}}),
                TestCase::Duration::QUICK);
    /* 4 STAs operating on 160 MHz */
    AddTestCase(new TestMultipleCtsResponsesFromMuRts({{160}, {160}, {160}, {160}}),
                TestCase::Duration::QUICK);
    /* 4 STAs operating on different bandwidths with PPDUs sent with decreasing BW: 160, 80, 40 and
     * 20 MHz */
    AddTestCase(new TestMultipleCtsResponsesFromMuRts({{160}, {80}, {40}, {20}}),
                TestCase::Duration::QUICK);
    /* 4 STAs operating on different bandwidths with PPDUs sent with increasing BW: 20, 40, 80 and
     * 160 MHz */
    AddTestCase(new TestMultipleCtsResponsesFromMuRts({{20}, {40}, {80}, {160}}),
                TestCase::Duration::QUICK);
    /* 2 STAs operating on different bandwidths with PPDUs sent with decreasing BW but the first STA
     * does not respond */
    AddTestCase(new TestMultipleCtsResponsesFromMuRts({{80, true}, {40, false}}),
                TestCase::Duration::QUICK);
    /* 2 STAs operating on different bandwidths with PPDUs sent with decreasing BW but the second
     * STA does not respond */
    AddTestCase(new TestMultipleCtsResponsesFromMuRts({{80, false}, {40, true}}),
                TestCase::Duration::QUICK);
    /* 2 STAs operating on different bandwidths with PPDUs sent with increasing BW but the first STA
     * does not respond */
    AddTestCase(new TestMultipleCtsResponsesFromMuRts({{40, true}, {80, false}}),
                TestCase::Duration::QUICK);
    /* 2 STAs operating on different bandwidths with PPDUs sent with increasing BW but the second
     * STA does not respond */
    AddTestCase(new TestMultipleCtsResponsesFromMuRts({{40, false}, {80, true}}),
                TestCase::Duration::QUICK);
}

static WifiNonHtDuplicateTestSuite wifiNonHtDuplicateTestSuite; ///< the test suite
