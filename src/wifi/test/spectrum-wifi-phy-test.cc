/*
 * Copyright (c) 2015 University of Washington
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
 */

#include "ns3/boolean.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/he-phy.h" //includes OFDM PHY
#include "ns3/interference-helper.h"
#include "ns3/log.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/ofdm-ppdu.h"
#include "ns3/pointer.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/test.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy-listener.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-spectrum-phy-interface.h"
#include "ns3/wifi-spectrum-signal-parameters.h"
#include "ns3/wifi-spectrum-value-helper.h"
#include "ns3/wifi-utils.h"

#include <memory>
#include <tuple>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SpectrumWifiPhyTest");

static const uint8_t CHANNEL_NUMBER = 36;
static const uint16_t CHANNEL_WIDTH = 20; // MHz
static const uint16_t GUARD_WIDTH =
    CHANNEL_WIDTH; // MHz (expanded to channel width to model spectrum mask)

/**
 * Extended SpectrumWifiPhy class for the purpose of the tests.
 */
class ExtSpectrumWifiPhy : public SpectrumWifiPhy
{
  public:
    using SpectrumWifiPhy::SpectrumWifiPhy;
    using WifiPhy::GetBand;

    /**
     * Get the spectrum PHY interfaces
     *
     * \return the spectrum PHY interfaces
     */
    const std::map<FrequencyRange, Ptr<WifiSpectrumPhyInterface>>& GetSpectrumPhyInterfaces() const
    {
        return m_spectrumPhyInterfaces;
    }

    /**
     * \return the current spectrum PHY interface
     */
    Ptr<WifiSpectrumPhyInterface> GetCurrentSpectrumPhyInterface() const
    {
        return m_currentSpectrumPhyInterface;
    }

    /**
     * Get the info of a given band for a given spectrum PHY interface
     *
     * \param bandWidth the width of the band to be returned (MHz)
     * \param bandIndex the index of the band to be returned
     * \param freqRange the frequency range identifying the spectrum PHY interface
     * \param channelWidth the channel width currently used by the spectrum PHY interface
     *
     * \return the info that defines the band
     */
    WifiSpectrumBandInfo GetBandForInterface(uint16_t bandWidth,
                                             uint8_t bandIndex,
                                             const FrequencyRange& freqRange,
                                             uint16_t channelWidth)
    {
        auto subcarrierSpacing = GetSubcarrierSpacing();
        auto numBandsInChannel = static_cast<size_t>(channelWidth * 1e6 / subcarrierSpacing);
        auto numBandsInBand = static_cast<size_t>(bandWidth * 1e6 / subcarrierSpacing);
        auto rxSpectrumModel = m_spectrumPhyInterfaces.at(freqRange)->GetRxSpectrumModel();
        size_t totalNumBands = rxSpectrumModel->GetNumBands();
        auto startIndex = ((totalNumBands - numBandsInChannel) / 2) + (bandIndex * numBandsInBand);
        auto stopIndex = startIndex + numBandsInBand - 1;
        auto startGuardBand = rxSpectrumModel->Begin();
        auto startChannel = startGuardBand + startIndex;
        auto endChannel = startGuardBand + stopIndex + 1;
        auto lowFreq = static_cast<uint64_t>(startChannel->fc);
        auto highFreq = static_cast<uint64_t>(endChannel->fc);
        return {{startIndex, stopIndex}, {lowFreq, highFreq}};
    }
};

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Spectrum Wifi Phy Basic Test
 */
class SpectrumWifiPhyBasicTest : public TestCase
{
  public:
    SpectrumWifiPhyBasicTest();
    /**
     * Constructor
     *
     * \param name reference name
     */
    SpectrumWifiPhyBasicTest(std::string name);
    ~SpectrumWifiPhyBasicTest() override;

  protected:
    void DoSetup() override;
    void DoTeardown() override;
    Ptr<SpectrumWifiPhy> m_phy; ///< Phy
    /**
     * Make signal function
     * \param txPowerWatts the transmit power in watts
     * \param channel the operating channel of the PHY used for the transmission
     * \returns Ptr<SpectrumSignalParameters>
     */
    Ptr<SpectrumSignalParameters> MakeSignal(double txPowerWatts,
                                             const WifiPhyOperatingChannel& channel);
    /**
     * Send signal function
     * \param txPowerWatts the transmit power in watts
     */
    void SendSignal(double txPowerWatts);
    /**
     * Spectrum wifi receive success function
     * \param psdu the PSDU
     * \param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * \param txVector the transmit vector
     * \param statusPerMpdu reception status per MPDU
     */
    void SpectrumWifiPhyRxSuccess(Ptr<const WifiPsdu> psdu,
                                  RxSignalInfo rxSignalInfo,
                                  WifiTxVector txVector,
                                  std::vector<bool> statusPerMpdu);
    /**
     * Spectrum wifi receive failure function
     * \param psdu the PSDU
     */
    void SpectrumWifiPhyRxFailure(Ptr<const WifiPsdu> psdu);
    uint32_t m_count; ///< count

  private:
    void DoRun() override;

    uint64_t m_uid; //!< the UID to use for the PPDU
};

SpectrumWifiPhyBasicTest::SpectrumWifiPhyBasicTest()
    : SpectrumWifiPhyBasicTest("SpectrumWifiPhy test case receives one packet")
{
}

SpectrumWifiPhyBasicTest::SpectrumWifiPhyBasicTest(std::string name)
    : TestCase(name),
      m_count(0),
      m_uid(0)
{
}

// Make a Wi-Fi signal to inject directly to the StartRx() method
Ptr<SpectrumSignalParameters>
SpectrumWifiPhyBasicTest::MakeSignal(double txPowerWatts, const WifiPhyOperatingChannel& channel)
{
    WifiTxVector txVector = WifiTxVector(OfdmPhy::GetOfdmRate6Mbps(),
                                         0,
                                         WIFI_PREAMBLE_LONG,
                                         800,
                                         1,
                                         1,
                                         0,
                                         CHANNEL_WIDTH,
                                         false);

    Ptr<Packet> pkt = Create<Packet>(1000);
    WifiMacHeader hdr;

    hdr.SetType(WIFI_MAC_QOSDATA);
    hdr.SetQosTid(0);

    Ptr<WifiPsdu> psdu = Create<WifiPsdu>(pkt, hdr);
    Time txDuration = m_phy->CalculateTxDuration(psdu->GetSize(), txVector, m_phy->GetPhyBand());

    Ptr<WifiPpdu> ppdu = Create<OfdmPpdu>(psdu, txVector, channel, m_uid++);

    Ptr<SpectrumValue> txPowerSpectrum = WifiSpectrumValueHelper::CreateOfdmTxPowerSpectralDensity(
        channel.GetPrimaryChannelCenterFrequency(CHANNEL_WIDTH),
        CHANNEL_WIDTH,
        txPowerWatts,
        GUARD_WIDTH);
    Ptr<WifiSpectrumSignalParameters> txParams = Create<WifiSpectrumSignalParameters>();
    txParams->psd = txPowerSpectrum;
    txParams->txPhy = nullptr;
    txParams->duration = txDuration;
    txParams->ppdu = ppdu;
    txParams->txWidth = CHANNEL_WIDTH;

    return txParams;
}

// Make a Wi-Fi signal to inject directly to the StartRx() method
void
SpectrumWifiPhyBasicTest::SendSignal(double txPowerWatts)
{
    m_phy->StartRx(MakeSignal(txPowerWatts, m_phy->GetOperatingChannel()), nullptr);
}

void
SpectrumWifiPhyBasicTest::SpectrumWifiPhyRxSuccess(Ptr<const WifiPsdu> psdu,
                                                   RxSignalInfo rxSignalInfo,
                                                   WifiTxVector txVector,
                                                   std::vector<bool> statusPerMpdu)
{
    NS_LOG_FUNCTION(this << *psdu << rxSignalInfo << txVector);
    m_count++;
}

void
SpectrumWifiPhyBasicTest::SpectrumWifiPhyRxFailure(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << *psdu);
    m_count++;
}

SpectrumWifiPhyBasicTest::~SpectrumWifiPhyBasicTest()
{
}

// Create necessary objects, and inject signals.  Test that the expected
// number of packet receptions occur.
void
SpectrumWifiPhyBasicTest::DoSetup()
{
    Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<Node> node = CreateObject<Node>();
    Ptr<WifiNetDevice> dev = CreateObject<WifiNetDevice>();
    m_phy = CreateObject<SpectrumWifiPhy>();
    Ptr<InterferenceHelper> interferenceHelper = CreateObject<InterferenceHelper>();
    m_phy->SetInterferenceHelper(interferenceHelper);
    Ptr<ErrorRateModel> error = CreateObject<NistErrorRateModel>();
    m_phy->SetErrorRateModel(error);
    m_phy->SetDevice(dev);
    m_phy->AddChannel(spectrumChannel);
    m_phy->SetOperatingChannel(WifiPhy::ChannelTuple{CHANNEL_NUMBER, 0, WIFI_PHY_BAND_5GHZ, 0});
    m_phy->ConfigureStandard(WIFI_STANDARD_80211n);
    m_phy->SetReceiveOkCallback(
        MakeCallback(&SpectrumWifiPhyBasicTest::SpectrumWifiPhyRxSuccess, this));
    m_phy->SetReceiveErrorCallback(
        MakeCallback(&SpectrumWifiPhyBasicTest::SpectrumWifiPhyRxFailure, this));
    dev->SetPhy(m_phy);
    node->AddDevice(dev);
}

void
SpectrumWifiPhyBasicTest::DoTeardown()
{
    m_phy->Dispose();
    m_phy = nullptr;
}

// Test that the expected number of packet receptions occur.
void
SpectrumWifiPhyBasicTest::DoRun()
{
    double txPowerWatts = 0.010;
    // Send packets spaced 1 second apart; all should be received
    Simulator::Schedule(Seconds(1), &SpectrumWifiPhyBasicTest::SendSignal, this, txPowerWatts);
    Simulator::Schedule(Seconds(2), &SpectrumWifiPhyBasicTest::SendSignal, this, txPowerWatts);
    Simulator::Schedule(Seconds(3), &SpectrumWifiPhyBasicTest::SendSignal, this, txPowerWatts);
    // Send packets spaced 1 microsecond second apart; none should be received (PHY header reception
    // failure)
    Simulator::Schedule(MicroSeconds(4000000),
                        &SpectrumWifiPhyBasicTest::SendSignal,
                        this,
                        txPowerWatts);
    Simulator::Schedule(MicroSeconds(4000001),
                        &SpectrumWifiPhyBasicTest::SendSignal,
                        this,
                        txPowerWatts);
    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ(m_count, 3, "Didn't receive right number of packets");
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test Phy Listener
 */
class TestPhyListener : public ns3::WifiPhyListener
{
  public:
    /**
     * Create a test PhyListener
     */
    TestPhyListener() = default;
    ~TestPhyListener() override = default;

    void NotifyRxStart(Time duration) override
    {
        NS_LOG_FUNCTION(this << duration);
        ++m_notifyRxStart;
    }

    void NotifyRxEndOk() override
    {
        NS_LOG_FUNCTION(this);
        ++m_notifyRxEndOk;
    }

    void NotifyRxEndError() override
    {
        NS_LOG_FUNCTION(this);
        ++m_notifyRxEndError;
    }

    void NotifyTxStart(Time duration, double txPowerDbm) override
    {
        NS_LOG_FUNCTION(this << duration << txPowerDbm);
    }

    void NotifyCcaBusyStart(Time duration,
                            WifiChannelListType channelType,
                            const std::vector<Time>& /*per20MhzDurations*/) override
    {
        NS_LOG_FUNCTION(this << duration << channelType);
        if (duration.IsStrictlyPositive())
        {
            ++m_notifyMaybeCcaBusyStart;
            if (!m_ccaBusyStart.IsStrictlyPositive())
            {
                m_ccaBusyStart = Simulator::Now();
            }
            m_ccaBusyEnd = std::max(m_ccaBusyEnd, Simulator::Now() + duration);
        }
    }

    void NotifySwitchingStart(Time duration) override
    {
    }

    void NotifySleep() override
    {
    }

    void NotifyOff() override
    {
    }

    void NotifyWakeup() override
    {
    }

    void NotifyOn() override
    {
    }

    /**
     * Reset function
     */
    void Reset()
    {
        NS_LOG_FUNCTION(this);
        m_notifyRxStart = 0;
        m_notifyRxEndOk = 0;
        m_notifyRxEndError = 0;
        m_notifyMaybeCcaBusyStart = 0;
        m_ccaBusyStart = Seconds(0);
        m_ccaBusyEnd = Seconds(0);
    }

    uint32_t m_notifyRxStart{0};           ///< notify receive start
    uint32_t m_notifyRxEndOk{0};           ///< notify receive end OK
    uint32_t m_notifyRxEndError{0};        ///< notify receive end error
    uint32_t m_notifyMaybeCcaBusyStart{0}; ///< notify maybe CCA busy start
    Time m_ccaBusyStart{0};                ///< CCA_BUSY start time
    Time m_ccaBusyEnd{0};                  ///< CCA_BUSY end time
};

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Spectrum Wifi Phy Listener Test
 */
class SpectrumWifiPhyListenerTest : public SpectrumWifiPhyBasicTest
{
  public:
    SpectrumWifiPhyListenerTest();
    ~SpectrumWifiPhyListenerTest() override;

  private:
    void DoSetup() override;
    void DoRun() override;
    TestPhyListener* m_listener; ///< listener
};

SpectrumWifiPhyListenerTest::SpectrumWifiPhyListenerTest()
    : SpectrumWifiPhyBasicTest("SpectrumWifiPhy test operation of WifiPhyListener")
{
}

SpectrumWifiPhyListenerTest::~SpectrumWifiPhyListenerTest()
{
}

void
SpectrumWifiPhyListenerTest::DoSetup()
{
    SpectrumWifiPhyBasicTest::DoSetup();
    m_listener = new TestPhyListener;
    m_phy->RegisterListener(m_listener);
}

void
SpectrumWifiPhyListenerTest::DoRun()
{
    double txPowerWatts = 0.010;
    Simulator::Schedule(Seconds(1), &SpectrumWifiPhyListenerTest::SendSignal, this, txPowerWatts);
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(m_count, 1, "Didn't receive right number of packets");
    NS_TEST_ASSERT_MSG_EQ(
        m_listener->m_notifyMaybeCcaBusyStart,
        2,
        "Didn't receive NotifyCcaBusyStart (once preamble is detected + prolonged by L-SIG "
        "reception, then switched to Rx by at the beginning of data)");
    NS_TEST_ASSERT_MSG_EQ(m_listener->m_notifyRxStart, 1, "Didn't receive NotifyRxStart");
    NS_TEST_ASSERT_MSG_EQ(m_listener->m_notifyRxEndOk, 1, "Didn't receive NotifyRxEnd");

    Simulator::Destroy();
    delete m_listener;
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Spectrum Wifi Phy Filter Test
 */
class SpectrumWifiPhyFilterTest : public TestCase
{
  public:
    SpectrumWifiPhyFilterTest();
    /**
     * Constructor
     *
     * \param name reference name
     */
    SpectrumWifiPhyFilterTest(std::string name);
    ~SpectrumWifiPhyFilterTest() override;

  private:
    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     * Run one function
     */
    void RunOne();

    /**
     * Send PPDU function
     */
    void SendPpdu();

    /**
     * Callback triggered when a packet is received by the PHYs
     * \param p the received packet
     * \param rxPowersW the received power per channel band in watts
     */
    void RxCallback(Ptr<const Packet> p, RxPowerWattPerChannelBand rxPowersW);

    Ptr<ExtSpectrumWifiPhy> m_txPhy; ///< TX PHY
    Ptr<ExtSpectrumWifiPhy> m_rxPhy; ///< RX PHY

    uint16_t m_txChannelWidth; ///< TX channel width (MHz)
    uint16_t m_rxChannelWidth; ///< RX channel width (MHz)

    std::set<WifiSpectrumBandIndices> m_ruBands; ///< spectrum bands associated to all the RUs
};

SpectrumWifiPhyFilterTest::SpectrumWifiPhyFilterTest()
    : TestCase("SpectrumWifiPhy test RX filters"),
      m_txChannelWidth(20),
      m_rxChannelWidth(20)
{
}

SpectrumWifiPhyFilterTest::SpectrumWifiPhyFilterTest(std::string name)
    : TestCase(name)
{
}

void
SpectrumWifiPhyFilterTest::SendPpdu()
{
    WifiTxVector txVector = WifiTxVector(HePhy::GetHeMcs0(),
                                         0,
                                         WIFI_PREAMBLE_HE_SU,
                                         800,
                                         1,
                                         1,
                                         0,
                                         m_txChannelWidth,
                                         false,
                                         false);
    Ptr<Packet> pkt = Create<Packet>(1000);
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_QOSDATA);
    hdr.SetQosTid(0);
    hdr.SetAddr1(Mac48Address("00:00:00:00:00:01"));
    hdr.SetSequenceNumber(1);
    Ptr<WifiPsdu> psdu = Create<WifiPsdu>(pkt, hdr);
    m_txPhy->Send(WifiConstPsduMap({std::make_pair(SU_STA_ID, psdu)}), txVector);
}

SpectrumWifiPhyFilterTest::~SpectrumWifiPhyFilterTest()
{
    m_txPhy = nullptr;
    m_rxPhy = nullptr;
}

void
SpectrumWifiPhyFilterTest::RxCallback(Ptr<const Packet> p, RxPowerWattPerChannelBand rxPowersW)
{
    for (const auto& pair : rxPowersW)
    {
        NS_LOG_INFO("band: (" << pair.first << ") -> powerW=" << pair.second << " ("
                              << WToDbm(pair.second) << " dBm)");
    }

    size_t numBands = rxPowersW.size();
    size_t expectedNumBands = std::max(1, (m_rxChannelWidth / 20));
    expectedNumBands += (m_rxChannelWidth / 40);
    expectedNumBands += (m_rxChannelWidth / 80);
    expectedNumBands += (m_rxChannelWidth / 160);
    expectedNumBands += m_ruBands.size();

    NS_TEST_ASSERT_MSG_EQ(numBands,
                          expectedNumBands,
                          "Total number of bands handled by the receiver is incorrect");

    uint16_t channelWidth = std::min(m_txChannelWidth, m_rxChannelWidth);
    auto band = m_rxPhy->GetBand(channelWidth, 0);
    auto it = rxPowersW.find(band);
    NS_LOG_INFO("powerW total band: " << it->second << " (" << WToDbm(it->second) << " dBm)");
    int totalRxPower = static_cast<int>(WToDbm(it->second) + 0.5);
    int expectedTotalRxPower;
    if (m_txChannelWidth <= m_rxChannelWidth)
    {
        // PHY sends at 16 dBm, and since there is no loss, this should be the total power at the
        // receiver.
        expectedTotalRxPower = 16;
    }
    else
    {
        // Only a part of the transmitted power is received
        expectedTotalRxPower =
            16 - static_cast<int>(RatioToDb(m_txChannelWidth / m_rxChannelWidth));
    }
    NS_TEST_ASSERT_MSG_EQ(totalRxPower,
                          expectedTotalRxPower,
                          "Total received power is not correct");

    if ((m_txChannelWidth <= m_rxChannelWidth) && (channelWidth >= 20))
    {
        band = m_rxPhy->GetBand(20, 0); // primary 20 MHz
        it = rxPowersW.find(band);
        NS_LOG_INFO("powerW in primary 20 MHz channel: " << it->second << " (" << WToDbm(it->second)
                                                         << " dBm)");
        int rxPowerPrimaryChannel20 = static_cast<int>(WToDbm(it->second) + 0.5);
        int expectedRxPowerPrimaryChannel20 = 16 - static_cast<int>(RatioToDb(channelWidth / 20));
        NS_TEST_ASSERT_MSG_EQ(rxPowerPrimaryChannel20,
                              expectedRxPowerPrimaryChannel20,
                              "Received power in the primary 20 MHz band is not correct");
    }
}

void
SpectrumWifiPhyFilterTest::DoSetup()
{
    // WifiHelper::EnableLogComponents();
    // LogComponentEnable("SpectrumWifiPhyTest", LOG_LEVEL_ALL);

    Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel>();
    lossModel->SetFrequency(5.180e9);
    spectrumChannel->AddPropagationLossModel(lossModel);
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);

    Ptr<Node> txNode = CreateObject<Node>();
    Ptr<WifiNetDevice> txDev = CreateObject<WifiNetDevice>();
    m_txPhy = CreateObject<ExtSpectrumWifiPhy>();
    Ptr<InterferenceHelper> txInterferenceHelper = CreateObject<InterferenceHelper>();
    m_txPhy->SetInterferenceHelper(txInterferenceHelper);
    Ptr<ErrorRateModel> txErrorModel = CreateObject<NistErrorRateModel>();
    m_txPhy->SetErrorRateModel(txErrorModel);
    m_txPhy->SetDevice(txDev);
    m_txPhy->AddChannel(spectrumChannel);
    m_txPhy->ConfigureStandard(WIFI_STANDARD_80211ax);
    Ptr<ConstantPositionMobilityModel> apMobility = CreateObject<ConstantPositionMobilityModel>();
    m_txPhy->SetMobility(apMobility);
    txDev->SetPhy(m_txPhy);
    txNode->AggregateObject(apMobility);
    txNode->AddDevice(txDev);

    Ptr<Node> rxNode = CreateObject<Node>();
    Ptr<WifiNetDevice> rxDev = CreateObject<WifiNetDevice>();
    m_rxPhy = CreateObject<ExtSpectrumWifiPhy>();
    Ptr<InterferenceHelper> rxInterferenceHelper = CreateObject<InterferenceHelper>();
    m_rxPhy->SetInterferenceHelper(rxInterferenceHelper);
    Ptr<ErrorRateModel> rxErrorModel = CreateObject<NistErrorRateModel>();
    m_rxPhy->SetErrorRateModel(rxErrorModel);
    m_rxPhy->AddChannel(spectrumChannel);
    m_rxPhy->ConfigureStandard(WIFI_STANDARD_80211ax);
    Ptr<ConstantPositionMobilityModel> sta1Mobility = CreateObject<ConstantPositionMobilityModel>();
    m_rxPhy->SetMobility(sta1Mobility);
    rxDev->SetPhy(m_rxPhy);
    rxNode->AggregateObject(sta1Mobility);
    rxNode->AddDevice(rxDev);
    m_rxPhy->TraceConnectWithoutContext("PhyRxBegin",
                                        MakeCallback(&SpectrumWifiPhyFilterTest::RxCallback, this));
}

void
SpectrumWifiPhyFilterTest::DoTeardown()
{
    m_txPhy->Dispose();
    m_txPhy = nullptr;
    m_rxPhy->Dispose();
    m_rxPhy = nullptr;
}

void
SpectrumWifiPhyFilterTest::RunOne()
{
    uint16_t txFrequency;
    switch (m_txChannelWidth)
    {
    case 20:
    default:
        txFrequency = 5180;
        break;
    case 40:
        txFrequency = 5190;
        break;
    case 80:
        txFrequency = 5210;
        break;
    case 160:
        txFrequency = 5250;
        break;
    }
    auto txChannelNum = std::get<0>(*WifiPhyOperatingChannel::FindFirst(0,
                                                                        txFrequency,
                                                                        m_txChannelWidth,
                                                                        WIFI_STANDARD_80211ax,
                                                                        WIFI_PHY_BAND_5GHZ));
    m_txPhy->SetOperatingChannel(
        WifiPhy::ChannelTuple{txChannelNum, m_txChannelWidth, (int)(WIFI_PHY_BAND_5GHZ), 0});

    uint16_t rxFrequency;
    switch (m_rxChannelWidth)
    {
    case 20:
    default:
        rxFrequency = 5180;
        break;
    case 40:
        rxFrequency = 5190;
        break;
    case 80:
        rxFrequency = 5210;
        break;
    case 160:
        rxFrequency = 5250;
        break;
    }
    auto rxChannelNum = std::get<0>(*WifiPhyOperatingChannel::FindFirst(0,
                                                                        rxFrequency,
                                                                        m_rxChannelWidth,
                                                                        WIFI_STANDARD_80211ax,
                                                                        WIFI_PHY_BAND_5GHZ));
    m_rxPhy->SetOperatingChannel(
        WifiPhy::ChannelTuple{rxChannelNum, m_rxChannelWidth, (int)(WIFI_PHY_BAND_5GHZ), 0});

    m_ruBands.clear();
    for (uint16_t bw = 160; bw >= 20; bw = bw / 2)
    {
        for (uint16_t i = 0; i < (m_rxChannelWidth / bw); ++i)
        {
            for (unsigned int type = 0; type < 7; type++)
            {
                HeRu::RuType ruType = static_cast<HeRu::RuType>(type);
                for (std::size_t index = 1; index <= HeRu::GetNRus(bw, ruType); index++)
                {
                    HeRu::SubcarrierGroup subcarrierGroup =
                        HeRu::GetSubcarrierGroup(bw, ruType, index);
                    HeRu::SubcarrierRange subcarrierRange =
                        std::make_pair(subcarrierGroup.front().first,
                                       subcarrierGroup.back().second);
                    const auto band =
                        HePhy::ConvertHeRuSubcarriers(bw,
                                                      m_rxPhy->GetGuardBandwidth(m_rxChannelWidth),
                                                      m_rxPhy->GetSubcarrierSpacing(),
                                                      subcarrierRange,
                                                      i);
                    m_ruBands.insert(band);
                }
            }
        }
    }

    Simulator::Schedule(Seconds(1), &SpectrumWifiPhyFilterTest::SendPpdu, this);

    Simulator::Run();
}

void
SpectrumWifiPhyFilterTest::DoRun()
{
    m_txChannelWidth = 20;
    m_rxChannelWidth = 20;
    RunOne();

    m_txChannelWidth = 40;
    m_rxChannelWidth = 40;
    RunOne();

    m_txChannelWidth = 80;
    m_rxChannelWidth = 80;
    RunOne();

    m_txChannelWidth = 160;
    m_rxChannelWidth = 160;
    RunOne();

    m_txChannelWidth = 20;
    m_rxChannelWidth = 40;
    RunOne();

    m_txChannelWidth = 20;
    m_rxChannelWidth = 80;
    RunOne();

    m_txChannelWidth = 40;
    m_rxChannelWidth = 80;
    RunOne();

    m_txChannelWidth = 20;
    m_rxChannelWidth = 160;
    RunOne();

    m_txChannelWidth = 40;
    m_rxChannelWidth = 160;
    RunOne();

    m_txChannelWidth = 80;
    m_rxChannelWidth = 160;
    RunOne();

    m_txChannelWidth = 40;
    m_rxChannelWidth = 20;
    RunOne();

    m_txChannelWidth = 80;
    m_rxChannelWidth = 20;
    RunOne();

    m_txChannelWidth = 80;
    m_rxChannelWidth = 40;
    RunOne();

    m_txChannelWidth = 160;
    m_rxChannelWidth = 20;
    RunOne();

    m_txChannelWidth = 160;
    m_rxChannelWidth = 40;
    RunOne();

    m_txChannelWidth = 160;
    m_rxChannelWidth = 80;
    RunOne();

    Simulator::Destroy();
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Spectrum Wifi Phy Multiple Spectrum Test
 *
 * This test is testing the ability to plug multiple spectrum channels to the spectrum wifi PHY.
 * It considers 4 TX-RX PHY pairs that are independent from each others and are plugged to different
 * spectrum channels that are covering different frequency range. Each RX PHY is also attached to
 * each of the other 3 spectrum channels it can switch to.
 *
 * In the first scenario, we consider the default case where each TX-RX PHY pairs are operating on
 * different frequency ranges and hence using independent spectrum channels. We validate that no
 * packets is received from other TX PHYs attached to different spectrum channels and we also verify
 * the amount of connected PHYs to each spectrum channel is exactly 2. The test also makes sure each
 * PHY has only one active spectrum channel and that the active one is operating at the expected
 * frequency range.
 *
 * In the second scenario, we consecutively switch the channel of all RX PHYs to the one of each TX
 * PHY. We validate that packets are received by all PHYs and we also verify the amount of connected
 * PHYs to each spectrum channels is either 5 (1 TX PHY and 4 RX PHYs) or 1 (the TX PHY left alone).
 */
class SpectrumWifiPhyMultipleInterfacesTest : public TestCase
{
  public:
    /**
     * Constructor
     *
     * \param trackSignalsInactiveInterfaces flag to indicate whether signals coming from inactive
     * spectrum PHY interfaces shall be tracked during the test
     */
    SpectrumWifiPhyMultipleInterfacesTest(bool trackSignalsInactiveInterfaces);

  private:
    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     * Switch channel function
     *
     * \param index the index to identify the RX PHY
     * \param band the PHY band to use
     * \param channelNumber number the channel number to use
     * \param channelWidth the channel width to use
     */
    void SwitchChannel(std::size_t index,
                       WifiPhyBand band,
                       uint8_t channelNumber,
                       uint16_t channelWidth);

    /**
     * Send PPDU function
     *
     * \param phy the PHY to transmit the signal
     * \param txPowerDbm the power in dBm to transmit the signal (this is also the received power
     * since we do not have propagation loss to simplify)
     */
    void SendPpdu(Ptr<SpectrumWifiPhy> phy, double txPowerDbm);

    /**
     * Callback triggered when a packet is received by a PHY
     * \param index the index to identify the RX PHY
     * \param packet the received packet
     * \param rxPowersW the received power per channel band in watts
     */
    void RxCallback(std::size_t index,
                    Ptr<const Packet> packet,
                    RxPowerWattPerChannelBand rxPowersW);

    /**
     * Schedule now to check the interferences
     * \param phy the PHY for which the check has to be executed
     * \param freqRange the frequency range for which the check has to be executed
     * \param channelWidth the channel width for which the check has to be executed
     * \param interferencesExpected flag whether interferences are expected to have been tracked
     */
    void CheckInterferences(Ptr<ExtSpectrumWifiPhy> phy,
                            const FrequencyRange& freqRange,
                            uint16_t channelWidth,
                            bool interferencesExpected);

    /**
     * Check the interferences
     * \param phy the PHY for which the check has to be executed
     * \param freqRange the frequency range for which the check has to be executed
     * \param channelWidth the channel width for which the check has to be executed
     * \param interferencesExpected flag whether interferences are expected to have been tracked
     */
    void DoCheckInterferences(Ptr<ExtSpectrumWifiPhy> phy,
                              const FrequencyRange& freqRange,
                              uint16_t channelWidth,
                              bool interferencesExpected);

    /**
     * Verify results
     *
     * \param index the index to identify the RX PHY to check
     * \param expectedNumRx the expected number of RX events for that PHY
     * \param expectedFrequencyRangeActiveRfInterface the expected frequency range (in MHz) of the
     * active RF interface
     * \param expectedConnectedPhysPerChannel the expected
     * number of PHYs attached for each spectrum channel
     */
    void CheckResults(std::size_t index,
                      uint32_t expectedNumRx,
                      FrequencyRange expectedFrequencyRangeActiveRfInterface,
                      const std::vector<std::size_t>& expectedConnectedPhysPerChannel);

    /**
     * Verify CCA indication reported by a given PHY
     *
     * \param index the index to identify the RX PHY to check
     * \param expectedCcaBusyIndication flag to indicate whether a CCA BUSY notification is expected
     * \param switchingDelay delay between the TX has started and the time RX switched to the TX
     * channel
     */
    void CheckCcaIndication(std::size_t index, bool expectedCcaBusyIndication, Time switchingDelay);

    /**
     * Reset function
     */
    void Reset();

    /// typedef for spectrum channel infos
    using SpectrumChannelInfos = std::tuple<Ptr<MultiModelSpectrumChannel>, uint16_t, uint16_t>;

    bool
        m_trackSignalsInactiveInterfaces; //!< flag to indicate whether signals coming from inactive
                                          //!< spectrum PHY interfaces are tracked during the test

    std::vector<SpectrumChannelInfos> m_spectrumChannelInfos;    //!< Spectrum channels infos
    std::vector<Ptr<ExtSpectrumWifiPhy>> m_txPhys{};             //!< TX PHYs
    std::vector<Ptr<ExtSpectrumWifiPhy>> m_rxPhys{};             //!< RX PHYs
    std::vector<std::unique_ptr<TestPhyListener>> m_listeners{}; //!< listeners

    std::vector<uint32_t> m_counts{0}; //!< count number of packets received by PHYs

    Time m_lastTxStart{0}; //!< hold the time at which the last transmission started
    Time m_lastTxEnd{0};   //!< hold the time at which the last transmission ended
};

SpectrumWifiPhyMultipleInterfacesTest::SpectrumWifiPhyMultipleInterfacesTest(
    bool trackSignalsInactiveInterfaces)
    : TestCase{"SpectrumWifiPhy test operation with multiple RF interfaces"},
      m_trackSignalsInactiveInterfaces{trackSignalsInactiveInterfaces}
{
}

void
SpectrumWifiPhyMultipleInterfacesTest::SwitchChannel(std::size_t index,
                                                     WifiPhyBand band,
                                                     uint8_t channelNumber,
                                                     uint16_t channelWidth)
{
    NS_LOG_FUNCTION(this << index << band << +channelNumber << channelWidth);
    auto& listener = m_listeners.at(index);
    listener->m_notifyMaybeCcaBusyStart = 0;
    listener->m_ccaBusyStart = Seconds(0);
    listener->m_ccaBusyEnd = Seconds(0);
    auto phy = m_rxPhys.at(index);
    phy->SetOperatingChannel(WifiPhy::ChannelTuple{channelNumber, channelWidth, band, 0});
}

void
SpectrumWifiPhyMultipleInterfacesTest::SendPpdu(Ptr<SpectrumWifiPhy> phy, double txPowerDbm)
{
    NS_LOG_FUNCTION(this << phy << txPowerDbm << phy->GetCurrentFrequencyRange()
                         << phy->GetChannelWidth() << phy->GetChannelNumber());

    WifiTxVector txVector =
        WifiTxVector(HePhy::GetHeMcs0(), 0, WIFI_PREAMBLE_HE_SU, 800, 1, 1, 0, 20, false, false);
    Ptr<Packet> pkt = Create<Packet>(1000);
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_QOSDATA);
    hdr.SetQosTid(0);
    hdr.SetAddr1(Mac48Address("00:00:00:00:00:01"));
    hdr.SetSequenceNumber(1);
    Ptr<WifiPsdu> psdu = Create<WifiPsdu>(pkt, hdr);

    m_lastTxStart = Simulator::Now();
    m_lastTxEnd = m_lastTxStart + WifiPhy::CalculateTxDuration({std::make_pair(SU_STA_ID, psdu)},
                                                               txVector,
                                                               phy->GetPhyBand());
    phy->SetTxPowerStart(txPowerDbm);
    phy->SetTxPowerEnd(txPowerDbm);
    phy->Send(WifiConstPsduMap({std::make_pair(SU_STA_ID, psdu)}), txVector);
}

void
SpectrumWifiPhyMultipleInterfacesTest::RxCallback(std::size_t index,
                                                  Ptr<const Packet> /*packet*/,
                                                  RxPowerWattPerChannelBand /*rxPowersW*/)
{
    auto phy = m_rxPhys.at(index);
    NS_LOG_FUNCTION(this << index << phy->GetCurrentFrequencyRange() << phy->GetChannelWidth()
                         << phy->GetChannelNumber());
    m_counts.at(index)++;
}

void
SpectrumWifiPhyMultipleInterfacesTest::CheckInterferences(Ptr<ExtSpectrumWifiPhy> phy,
                                                          const FrequencyRange& freqRange,
                                                          uint16_t channelWidth,
                                                          bool interferencesExpected)
{
    if ((!m_trackSignalsInactiveInterfaces) && (phy->GetCurrentFrequencyRange() != freqRange))
    {
        // ignore since no bands for that range exists in interference helper in that case
        return;
    }
    // This is needed to make sure PHY state will be checked as the last event if a state change
    // occurred at the exact same time as the check
    Simulator::ScheduleNow(&SpectrumWifiPhyMultipleInterfacesTest::DoCheckInterferences,
                           this,
                           phy,
                           freqRange,
                           channelWidth,
                           interferencesExpected);
}

void
SpectrumWifiPhyMultipleInterfacesTest::DoCheckInterferences(Ptr<ExtSpectrumWifiPhy> phy,
                                                            const FrequencyRange& freqRange,
                                                            uint16_t channelWidth,
                                                            bool interferencesExpected)
{
    NS_LOG_FUNCTION(this << phy << freqRange << interferencesExpected);
    PointerValue ptr;
    phy->GetAttribute("InterferenceHelper", ptr);
    auto interferenceHelper = DynamicCast<InterferenceHelper>(ptr.Get<InterferenceHelper>());
    NS_ASSERT(interferenceHelper);
    const auto band = phy->GetBandForInterface(channelWidth, 0, freqRange, channelWidth);
    const auto energyDuration = interferenceHelper->GetEnergyDuration(0, band);
    NS_TEST_ASSERT_MSG_EQ(energyDuration.IsStrictlyPositive(),
                          interferencesExpected,
                          "Incorrect interferences detection");
}

void
SpectrumWifiPhyMultipleInterfacesTest::CheckResults(
    std::size_t index,
    uint32_t expectedNumRx,
    FrequencyRange expectedFrequencyRangeActiveRfInterface,
    const std::vector<std::size_t>& expectedConnectedPhysPerChannel)
{
    NS_LOG_FUNCTION(this << index << expectedNumRx << expectedFrequencyRangeActiveRfInterface);
    const auto phy = m_rxPhys.at(index);
    std::size_t numActiveInterfaces = 0;
    for (const auto& [freqRange, interface] : phy->GetSpectrumPhyInterfaces())
    {
        const auto expectedActive = (freqRange == expectedFrequencyRangeActiveRfInterface);
        const auto isActive = (interface == phy->GetCurrentSpectrumPhyInterface());
        NS_TEST_ASSERT_MSG_EQ(isActive, expectedActive, "Incorrect active interface");
        if (isActive)
        {
            numActiveInterfaces++;
        }
    }
    NS_TEST_ASSERT_MSG_EQ(numActiveInterfaces, 1, "There should always be one active interface");
    NS_ASSERT(expectedConnectedPhysPerChannel.size() == m_spectrumChannelInfos.size());
    for (std::size_t i = 0; i < m_spectrumChannelInfos.size(); ++i)
    {
        const auto spectrumChannel = std::get<0>(m_spectrumChannelInfos.at(i));
        NS_TEST_ASSERT_MSG_EQ(spectrumChannel->GetNDevices(),
                              expectedConnectedPhysPerChannel.at(i),
                              "Incorrect number of PHYs attached to the spectrum channel");
    }
    NS_TEST_ASSERT_MSG_EQ(m_counts.at(index),
                          expectedNumRx,
                          "Didn't receive right number of packets");
    NS_TEST_ASSERT_MSG_EQ(m_listeners.at(index)->m_notifyRxStart,
                          expectedNumRx,
                          "Didn't receive NotifyRxStart");
}

void
SpectrumWifiPhyMultipleInterfacesTest::CheckCcaIndication(std::size_t index,
                                                          bool expectedCcaBusyIndication,
                                                          Time switchingDelay)
{
    const auto expectedCcaBusyStart =
        expectedCcaBusyIndication ? m_lastTxStart + switchingDelay : Seconds(0);
    const auto expectedCcaBusyEnd = expectedCcaBusyIndication ? m_lastTxEnd : Seconds(0);
    NS_LOG_FUNCTION(this << index << expectedCcaBusyIndication << expectedCcaBusyStart
                         << expectedCcaBusyEnd);
    auto& listener = m_listeners.at(index);
    const auto ccaBusyIndication = (listener->m_notifyMaybeCcaBusyStart > 0);
    const auto ccaBusyStart = listener->m_ccaBusyStart;
    const auto ccaBusyEnd = listener->m_ccaBusyEnd;
    NS_TEST_ASSERT_MSG_EQ(ccaBusyIndication,
                          expectedCcaBusyIndication,
                          "CCA busy indication check failed");
    NS_TEST_ASSERT_MSG_EQ(ccaBusyStart, expectedCcaBusyStart, "CCA busy start mismatch");
    NS_TEST_ASSERT_MSG_EQ(ccaBusyEnd, expectedCcaBusyEnd, "CCA busy end mismatch");
}

void
SpectrumWifiPhyMultipleInterfacesTest::Reset()
{
    NS_LOG_FUNCTION(this);
    for (auto& count : m_counts)
    {
        count = 0;
    }
    for (auto& listener : m_listeners)
    {
        listener->Reset();
    }
    // restore all RX PHYs to initial channels
    for (std::size_t rxPhyIndex = 0; rxPhyIndex < m_rxPhys.size(); ++rxPhyIndex)
    {
        auto txPhy = m_txPhys.at(rxPhyIndex);
        SwitchChannel(rxPhyIndex,
                      txPhy->GetPhyBand(),
                      txPhy->GetChannelNumber(),
                      txPhy->GetChannelWidth());
    }
}

void
SpectrumWifiPhyMultipleInterfacesTest::DoSetup()
{
    NS_LOG_FUNCTION(this);

    // WifiHelper::EnableLogComponents();
    // LogComponentEnable("SpectrumWifiPhyTest", LOG_LEVEL_ALL);

    const std::vector<std::pair<WifiPhyBand, uint8_t>> channels{{WIFI_PHY_BAND_2_4GHZ, 2},
                                                                {WIFI_PHY_BAND_5GHZ, 42},
                                                                {WIFI_PHY_BAND_5GHZ, 114},
                                                                {WIFI_PHY_BAND_6GHZ, 215}};

    // create spectrum channels
    for (std::size_t i = 0; i < channels.size(); ++i)
    {
        auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
        [[maybe_unused]] const auto [channel, frequency, channelWidth, type, band] =
            (*WifiPhyOperatingChannel::FindFirst(channels.at(i).second,
                                                 0,
                                                 0,
                                                 WIFI_STANDARD_80211ax,
                                                 channels.at(i).first));

        m_spectrumChannelInfos.emplace_back(spectrumChannel, frequency, channelWidth);
    }

    // create PHYs and add all channels to each of them
    for (std::size_t i = 0; i < channels.size(); ++i)
    {
        auto txNode = CreateObject<Node>();
        auto txDev = CreateObject<WifiNetDevice>();
        auto txPhy = CreateObject<ExtSpectrumWifiPhy>();
        txPhy->SetAttribute("ChannelSwitchDelay", TimeValue(Seconds(0)));
        txPhy->SetAttribute("TrackSignalsFromInactiveInterfaces",
                            BooleanValue(m_trackSignalsInactiveInterfaces));
        auto txInterferenceHelper = CreateObject<InterferenceHelper>();
        txPhy->SetInterferenceHelper(txInterferenceHelper);
        auto txError = CreateObject<NistErrorRateModel>();
        txPhy->SetErrorRateModel(txError);
        txPhy->SetDevice(txDev);
        for (const auto& [spectrumChannel, freq, width] : m_spectrumChannelInfos)
        {
            const uint16_t minFreq = freq - width / 2;
            const uint16_t maxFreq = freq + width / 2;
            txPhy->AddChannel(spectrumChannel, {minFreq, maxFreq});
        }
        txPhy->SetOperatingChannel(
            WifiPhy::ChannelTuple{channels.at(i).second, 0, channels.at(i).first, 0});
        txPhy->ConfigureStandard(WIFI_STANDARD_80211ax);
        txDev->SetPhy(txPhy);
        txNode->AddDevice(txDev);

        m_txPhys.push_back(txPhy);

        auto rxNode = CreateObject<Node>();
        auto rxDev = CreateObject<WifiNetDevice>();
        auto rxPhy = CreateObject<ExtSpectrumWifiPhy>();
        rxPhy->SetAttribute("ChannelSwitchDelay", TimeValue(Seconds(0)));
        rxPhy->SetAttribute("TrackSignalsFromInactiveInterfaces",
                            BooleanValue(m_trackSignalsInactiveInterfaces));
        auto rxInterferenceHelper = CreateObject<InterferenceHelper>();
        rxPhy->SetInterferenceHelper(rxInterferenceHelper);
        auto rxError = CreateObject<NistErrorRateModel>();
        rxPhy->SetErrorRateModel(rxError);
        rxPhy->SetDevice(rxDev);
        for (const auto& [spectrumChannel, freq, width] : m_spectrumChannelInfos)
        {
            const uint16_t minFreq = freq - width / 2;
            const uint16_t maxFreq = freq + width / 2;
            rxPhy->AddChannel(spectrumChannel, {minFreq, maxFreq});
        }
        rxPhy->SetOperatingChannel(
            WifiPhy::ChannelTuple{channels.at(i).second, 0, channels.at(i).first, 0});
        rxPhy->ConfigureStandard(WIFI_STANDARD_80211ax);
        rxDev->SetPhy(rxPhy);
        rxNode->AddDevice(rxDev);

        const auto index = m_rxPhys.size();
        rxPhy->TraceConnectWithoutContext(
            "PhyRxBegin",
            MakeCallback(&SpectrumWifiPhyMultipleInterfacesTest::RxCallback, this).Bind(index));

        auto listener = std::make_unique<TestPhyListener>();
        rxPhy->RegisterListener(listener.get());
        m_listeners.push_back(std::move(listener));

        m_rxPhys.push_back(rxPhy);
        m_counts.push_back(0);
    }
}

void
SpectrumWifiPhyMultipleInterfacesTest::DoTeardown()
{
    NS_LOG_FUNCTION(this);
    for (auto& phy : m_txPhys)
    {
        phy->Dispose();
        phy = nullptr;
    }
    for (auto& phy : m_rxPhys)
    {
        phy->Dispose();
        phy = nullptr;
    }
    Simulator::Destroy();
}

void
SpectrumWifiPhyMultipleInterfacesTest::DoRun()
{
    NS_LOG_FUNCTION(this);

    const auto ccaEdThresholdDbm = -62.0; ///< CCA-ED threshold in dBm
    const auto txAfterChannelSwitchDelay =
        Seconds(0.25); ///< delay in seconds between channel switch is triggered and a transmission
                       ///< gets started
    const auto checkResultsDelay =
        Seconds(0.5); ///< delay in seconds between start of test and moment results are verified
    const auto flushResultsDelay =
        Seconds(0.9); ///< delay in seconds between start of test and moment results are flushed
    const auto txOngoingAfterTxStartedDelay =
        MicroSeconds(50); ///< delay in microseconds between a transmission has started and a point
                          ///< in time the transmission is ongoing

    Time delay{0};
    // make sure all interfaces got active once for all RX PHYs to have them monitored
    for (std::size_t rxPhyIndex = 0; rxPhyIndex < m_rxPhys.size(); ++rxPhyIndex)
    {
        for (std::size_t txPhyIndex = 0; txPhyIndex < m_txPhys.size(); ++txPhyIndex)
        {
            if (txPhyIndex == rxPhyIndex)
            {
                // handled at end
                continue;
            }
            auto txPhy = m_txPhys.at(txPhyIndex);
            Simulator::Schedule(delay + Seconds((rxPhyIndex + rxPhyIndex) * 0.1),
                                &SpectrumWifiPhyMultipleInterfacesTest::SwitchChannel,
                                this,
                                rxPhyIndex,
                                txPhy->GetPhyBand(),
                                txPhy->GetChannelNumber(),
                                txPhy->GetChannelWidth());
        }
        Simulator::Schedule(delay + flushResultsDelay,
                            &SpectrumWifiPhyMultipleInterfacesTest::Reset,
                            this);
    }

    // default channels active for all PHYs: each PHY only receives from its associated TX
    std::vector<std::size_t> expectedConnectedPhysPerChannel =
        m_trackSignalsInactiveInterfaces ? std::vector<std::size_t>{5, 5, 5, 5}
                                         : // all RX PHYs keep all channels active when tracking
                                           // interferences on inactive interfaces
            std::vector<std::size_t>{2, 2, 2, 2}; // default channels active for all PHYs: each PHY
                                                  // only receives from its associated TX
    for (std::size_t i = 0; i < 4; ++i)
    {
        auto txPpduPhy = m_txPhys.at(i);
        delay += Seconds(1);
        Simulator::Schedule(delay,
                            &SpectrumWifiPhyMultipleInterfacesTest::SendPpdu,
                            this,
                            txPpduPhy,
                            0);
        for (std::size_t j = 0; j < 4; ++j)
        {
            auto txPhy = m_txPhys.at(j);
            auto rxPhy = m_rxPhys.at(j);
            const uint16_t minExpectedFreq = txPhy->GetFrequency() - (txPhy->GetChannelWidth() / 2);
            const uint16_t maxExpectedFreq = txPhy->GetFrequency() + (txPhy->GetChannelWidth() / 2);
            const FrequencyRange expectedFreqRange = {minExpectedFreq, maxExpectedFreq};
            Simulator::Schedule(delay + txOngoingAfterTxStartedDelay,
                                &SpectrumWifiPhyMultipleInterfacesTest::CheckInterferences,
                                this,
                                rxPhy,
                                txPpduPhy->GetCurrentFrequencyRange(),
                                txPpduPhy->GetChannelWidth(),
                                true);
            Simulator::Schedule(delay + checkResultsDelay,
                                &SpectrumWifiPhyMultipleInterfacesTest::CheckResults,
                                this,
                                j,
                                (i == j) ? 1 : 0,
                                expectedFreqRange,
                                expectedConnectedPhysPerChannel);
            Simulator::Schedule(delay + flushResultsDelay,
                                &SpectrumWifiPhyMultipleInterfacesTest::Reset,
                                this);
        }
    }

    // same channel active for all PHYs: all PHYs receive from TX
    for (std::size_t i = 0; i < 4; ++i)
    {
        delay += Seconds(1);
        auto txPpduPhy = m_txPhys.at(i);
        Simulator::Schedule(delay + txAfterChannelSwitchDelay,
                            &SpectrumWifiPhyMultipleInterfacesTest::SendPpdu,
                            this,
                            txPpduPhy,
                            0);
        const uint16_t minExpectedFreq =
            txPpduPhy->GetFrequency() - (txPpduPhy->GetChannelWidth() / 2);
        const uint16_t maxExpectedFreq =
            txPpduPhy->GetFrequency() + (txPpduPhy->GetChannelWidth() / 2);
        const FrequencyRange expectedFreqRange = {minExpectedFreq, maxExpectedFreq};
        for (std::size_t j = 0; j < 4; ++j)
        {
            if (!m_trackSignalsInactiveInterfaces)
            {
                for (std::size_t k = 0; k < expectedConnectedPhysPerChannel.size(); ++k)
                {
                    expectedConnectedPhysPerChannel.at(k) = (k == i) ? 5 : 1;
                }
            }
            auto rxPhy = m_rxPhys.at(j);
            Simulator::Schedule(delay,
                                &SpectrumWifiPhyMultipleInterfacesTest::SwitchChannel,
                                this,
                                j,
                                txPpduPhy->GetPhyBand(),
                                txPpduPhy->GetChannelNumber(),
                                txPpduPhy->GetChannelWidth());
            Simulator::Schedule(delay + txAfterChannelSwitchDelay + txOngoingAfterTxStartedDelay,
                                &SpectrumWifiPhyMultipleInterfacesTest::CheckInterferences,
                                this,
                                rxPhy,
                                txPpduPhy->GetCurrentFrequencyRange(),
                                txPpduPhy->GetChannelWidth(),
                                true);
            Simulator::Schedule(delay + checkResultsDelay,
                                &SpectrumWifiPhyMultipleInterfacesTest::CheckResults,
                                this,
                                j,
                                1,
                                expectedFreqRange,
                                expectedConnectedPhysPerChannel);
            Simulator::Schedule(delay + flushResultsDelay,
                                &SpectrumWifiPhyMultipleInterfacesTest::Reset,
                                this);
        }
    }

    // Switch all PHYs to channel 36: all PHYs switch to the second spectrum channel
    // since second spectrum channel is 42 (80 MHz) and hence covers channel 36 (20 MHz)
    const auto secondSpectrumChannelIndex = 1;
    auto channel36TxPhy = m_txPhys.at(secondSpectrumChannelIndex);
    const uint16_t minExpectedFreq =
        channel36TxPhy->GetFrequency() - (channel36TxPhy->GetChannelWidth() / 2);
    const uint16_t maxExpectedFreq =
        channel36TxPhy->GetFrequency() + (channel36TxPhy->GetChannelWidth() / 2);
    FrequencyRange expectedFreqRange = {minExpectedFreq, maxExpectedFreq};
    for (std::size_t i = 0; i < 4; ++i)
    {
        delay += Seconds(1);
        auto txPpduPhy = m_txPhys.at(i);
        Simulator::Schedule(delay + txAfterChannelSwitchDelay,
                            &SpectrumWifiPhyMultipleInterfacesTest::SendPpdu,
                            this,
                            txPpduPhy,
                            0);
        for (std::size_t j = 0; j < 4; ++j)
        {
            if (!m_trackSignalsInactiveInterfaces)
            {
                for (std::size_t k = 0; k < expectedConnectedPhysPerChannel.size(); ++k)
                {
                    expectedConnectedPhysPerChannel.at(k) =
                        (k == secondSpectrumChannelIndex) ? 5 : 1;
                }
            }
            Simulator::Schedule(delay,
                                &SpectrumWifiPhyMultipleInterfacesTest::SwitchChannel,
                                this,
                                j,
                                WIFI_PHY_BAND_5GHZ,
                                CHANNEL_NUMBER,
                                CHANNEL_WIDTH);
            Simulator::Schedule(delay + checkResultsDelay,
                                &SpectrumWifiPhyMultipleInterfacesTest::CheckResults,
                                this,
                                j,
                                (i == secondSpectrumChannelIndex) ? 1 : 0,
                                expectedFreqRange,
                                expectedConnectedPhysPerChannel);
            Simulator::Schedule(delay + flushResultsDelay,
                                &SpectrumWifiPhyMultipleInterfacesTest::Reset,
                                this);
        }
    }

    // verify CCA indication when switching to a channel with an ongoing transmission
    for (const auto txPowerDbm : {-60.0 /* above CCA-ED */, -70.0 /* below CCA-ED */})
    {
        for (std::size_t i = 0; i < 4; ++i)
        {
            for (std::size_t j = 0; j < 4; ++j)
            {
                auto txPpduPhy = m_txPhys.at(i);
                const auto startChannel =
                    WifiPhyOperatingChannel::FindFirst(txPpduPhy->GetPrimaryChannelNumber(20),
                                                       0,
                                                       20,
                                                       WIFI_STANDARD_80211ax,
                                                       txPpduPhy->GetPhyBand());
                for (uint16_t bw = txPpduPhy->GetChannelWidth(); bw >= 20; bw /= 2)
                {
                    [[maybe_unused]] const auto [channel, frequency, channelWidth, type, band] =
                        (*WifiPhyOperatingChannel::FindFirst(0,
                                                             0,
                                                             bw,
                                                             WIFI_STANDARD_80211ax,
                                                             txPpduPhy->GetPhyBand(),
                                                             startChannel));
                    delay += Seconds(1);
                    Simulator::Schedule(delay,
                                        &SpectrumWifiPhyMultipleInterfacesTest::SendPpdu,
                                        this,
                                        txPpduPhy,
                                        txPowerDbm);
                    Simulator::Schedule(delay + txOngoingAfterTxStartedDelay,
                                        &SpectrumWifiPhyMultipleInterfacesTest::SwitchChannel,
                                        this,
                                        j,
                                        band,
                                        channel,
                                        channelWidth);
                    for (std::size_t k = 0; k < 4; ++k)
                    {
                        Simulator::Schedule(delay + flushResultsDelay,
                                            &SpectrumWifiPhyMultipleInterfacesTest::Reset,
                                            this);
                        if ((i != j) && (k == i))
                        {
                            continue;
                        }
                        const auto expectCcaBusyIndication =
                            (k == i) ? (txPowerDbm >= ccaEdThresholdDbm)
                                     : (m_trackSignalsInactiveInterfaces
                                            ? ((txPowerDbm >= ccaEdThresholdDbm) ? (j == k) : false)
                                            : false);
                        Simulator::Schedule(
                            delay + checkResultsDelay,
                            &SpectrumWifiPhyMultipleInterfacesTest::CheckCcaIndication,
                            this,
                            k,
                            expectCcaBusyIndication,
                            txOngoingAfterTxStartedDelay);
                    }
                }
            }
        }
    }

    Simulator::Run();
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Spectrum Wifi Phy Test Suite
 */
class SpectrumWifiPhyTestSuite : public TestSuite
{
  public:
    SpectrumWifiPhyTestSuite();
};

SpectrumWifiPhyTestSuite::SpectrumWifiPhyTestSuite()
    : TestSuite("wifi-spectrum-wifi-phy", UNIT)
{
    AddTestCase(new SpectrumWifiPhyBasicTest, TestCase::QUICK);
    AddTestCase(new SpectrumWifiPhyListenerTest, TestCase::QUICK);
    AddTestCase(new SpectrumWifiPhyFilterTest, TestCase::QUICK);
    AddTestCase(new SpectrumWifiPhyMultipleInterfacesTest(false), TestCase::QUICK);
    AddTestCase(new SpectrumWifiPhyMultipleInterfacesTest(true), TestCase::QUICK);
}

static SpectrumWifiPhyTestSuite spectrumWifiPhyTestSuite; ///< the test suite
