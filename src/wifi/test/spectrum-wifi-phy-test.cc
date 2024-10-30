/*
 * Copyright (c) 2015 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/boolean.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/he-phy.h" //includes OFDM PHY
#include "ns3/interference-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/non-communicating-net-device.h"
#include "ns3/ofdm-ppdu.h"
#include "ns3/pointer.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/waveform-generator.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy-listener.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-spectrum-phy-interface.h"
#include "ns3/wifi-spectrum-signal-parameters.h"
#include "ns3/wifi-spectrum-value-helper.h"
#include "ns3/wifi-utils.h"

#include <memory>
#include <optional>
#include <tuple>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SpectrumWifiPhyTest");

static const uint8_t CHANNEL_NUMBER = 36;
static const MHz_u CHANNEL_WIDTH{20};
static const MHz_u GUARD_WIDTH = CHANNEL_WIDTH; // expanded to channel width to model spectrum mask

/**
 * Extended SpectrumWifiPhy class for the purpose of the tests.
 */
class ExtSpectrumWifiPhy : public SpectrumWifiPhy
{
  public:
    using WifiPhy::GetBand;
};

/**
 * Extended InterferenceHelper class for the purpose of the tests.
 */
class ExtInterferenceHelper : public InterferenceHelper
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::ExtInterferenceHelper")
                                .SetParent<InterferenceHelper>()
                                .SetGroupName("Wifi")
                                .AddConstructor<ExtInterferenceHelper>();
        return tid;
    }

    /**
     * Indicate whether the interference helper is in receiving state
     *
     * @return true if the interference helper is in receiving state, false otherwise
     */
    bool IsRxing() const
    {
        return std::any_of(m_rxing.cbegin(), m_rxing.cend(), [](const auto& rxing) {
            return rxing.second;
        });
    }

    /**
     * Indicate whether a given band is tracked by the interference helper
     *
     * @param startStopFreqs the start and stop frequencies per segment of the band
     *
     * @return true if the specified band is tracked by the interference helper, false otherwise
     */
    bool IsBandTracked(const std::vector<WifiSpectrumBandFrequencies>& startStopFreqs) const
    {
        for (const auto& [band, nis] : m_niChanges)
        {
            if (band.frequencies == startStopFreqs)
            {
                return true;
            }
        }
        return false;
    }
};

NS_OBJECT_ENSURE_REGISTERED(ExtInterferenceHelper);

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Spectrum Wifi Phy Basic Test
 */
class SpectrumWifiPhyBasicTest : public TestCase
{
  public:
    SpectrumWifiPhyBasicTest();
    /**
     * Constructor
     *
     * @param name reference name
     */
    SpectrumWifiPhyBasicTest(std::string name);
    ~SpectrumWifiPhyBasicTest() override;

  protected:
    void DoSetup() override;
    void DoTeardown() override;
    Ptr<SpectrumWifiPhy> m_phy; ///< Phy
    /**
     * Make signal function
     * @param txPower the transmit power
     * @param channel the operating channel of the PHY used for the transmission
     * @returns Ptr<SpectrumSignalParameters>
     */
    Ptr<SpectrumSignalParameters> MakeSignal(Watt_u txPower,
                                             const WifiPhyOperatingChannel& channel);
    /**
     * Send signal function
     * @param txPower the transmit power
     */
    void SendSignal(Watt_u txPower);
    /**
     * Spectrum wifi receive success function
     * @param psdu the PSDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector the transmit vector
     * @param statusPerMpdu reception status per MPDU
     */
    void SpectrumWifiPhyRxSuccess(Ptr<const WifiPsdu> psdu,
                                  RxSignalInfo rxSignalInfo,
                                  const WifiTxVector& txVector,
                                  const std::vector<bool>& statusPerMpdu);
    /**
     * Spectrum wifi receive failure function
     * @param psdu the PSDU
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
SpectrumWifiPhyBasicTest::MakeSignal(Watt_u txPower, const WifiPhyOperatingChannel& channel)
{
    WifiTxVector txVector{OfdmPhy::GetOfdmRate6Mbps(),
                          0,
                          WIFI_PREAMBLE_LONG,
                          NanoSeconds(800),
                          1,
                          1,
                          0,
                          CHANNEL_WIDTH,
                          false};

    auto pkt = Create<Packet>(1000);
    WifiMacHeader hdr;

    hdr.SetType(WIFI_MAC_QOSDATA);
    hdr.SetQosTid(0);

    auto psdu = Create<WifiPsdu>(pkt, hdr);
    const auto txDuration =
        SpectrumWifiPhy::CalculateTxDuration(psdu->GetSize(), txVector, m_phy->GetPhyBand());

    auto ppdu = Create<OfdmPpdu>(psdu, txVector, channel, m_uid++);

    auto txPowerSpectrum = WifiSpectrumValueHelper::CreateOfdmTxPowerSpectralDensity(
        channel.GetPrimaryChannelCenterFrequency(CHANNEL_WIDTH),
        CHANNEL_WIDTH,
        txPower,
        GUARD_WIDTH);
    auto txParams = Create<WifiSpectrumSignalParameters>();
    txParams->psd = txPowerSpectrum;
    txParams->txPhy = nullptr;
    txParams->duration = txDuration;
    txParams->ppdu = ppdu;

    return txParams;
}

// Make a Wi-Fi signal to inject directly to the StartRx() method
void
SpectrumWifiPhyBasicTest::SendSignal(Watt_u txPower)
{
    m_phy->StartRx(MakeSignal(txPower, m_phy->GetOperatingChannel()), nullptr);
}

void
SpectrumWifiPhyBasicTest::SpectrumWifiPhyRxSuccess(Ptr<const WifiPsdu> psdu,
                                                   RxSignalInfo rxSignalInfo,
                                                   const WifiTxVector& txVector,
                                                   const std::vector<bool>& statusPerMpdu)
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
    auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    auto node = CreateObject<Node>();
    auto dev = CreateObject<WifiNetDevice>();
    m_phy = CreateObject<SpectrumWifiPhy>();
    auto interferenceHelper = CreateObject<InterferenceHelper>();
    m_phy->SetInterferenceHelper(interferenceHelper);
    auto error = CreateObject<NistErrorRateModel>();
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
    Watt_u txPower{0.01};
    // Send packets spaced 1 second apart; all should be received
    Simulator::Schedule(Seconds(1), &SpectrumWifiPhyBasicTest::SendSignal, this, txPower);
    Simulator::Schedule(Seconds(2), &SpectrumWifiPhyBasicTest::SendSignal, this, txPower);
    Simulator::Schedule(Seconds(3), &SpectrumWifiPhyBasicTest::SendSignal, this, txPower);
    // Send packets spaced 1 microsecond second apart; none should be received (PHY header reception
    // failure)
    Simulator::Schedule(MicroSeconds(4000000),
                        &SpectrumWifiPhyBasicTest::SendSignal,
                        this,
                        txPower);
    Simulator::Schedule(MicroSeconds(4000001),
                        &SpectrumWifiPhyBasicTest::SendSignal,
                        this,
                        txPower);
    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ(m_count, 3, "Didn't receive right number of packets");
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test Phy Listener
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

    void NotifyTxStart(Time duration, dBm_u txPower) override
    {
        NS_LOG_FUNCTION(this << duration << txPower);
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
    Time m_ccaBusyStart{};                 ///< CCA_BUSY start time
    Time m_ccaBusyEnd{};                   ///< CCA_BUSY end time
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Spectrum Wifi Phy Listener Test
 */
class SpectrumWifiPhyListenerTest : public SpectrumWifiPhyBasicTest
{
  public:
    SpectrumWifiPhyListenerTest();

  private:
    void DoSetup() override;
    void DoRun() override;
    std::shared_ptr<TestPhyListener> m_listener; ///< listener
};

SpectrumWifiPhyListenerTest::SpectrumWifiPhyListenerTest()
    : SpectrumWifiPhyBasicTest("SpectrumWifiPhy test operation of WifiPhyListener")
{
}

void
SpectrumWifiPhyListenerTest::DoSetup()
{
    SpectrumWifiPhyBasicTest::DoSetup();
    m_listener = std::make_shared<TestPhyListener>();
    m_phy->RegisterListener(m_listener);
}

void
SpectrumWifiPhyListenerTest::DoRun()
{
    Watt_u txPower{0.01};
    Simulator::Schedule(Seconds(1), &SpectrumWifiPhyListenerTest::SendSignal, this, txPower);
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
    m_listener.reset();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Spectrum Wifi Phy Filter Test
 */
class SpectrumWifiPhyFilterTest : public TestCase
{
  public:
    SpectrumWifiPhyFilterTest();
    /**
     * Constructor
     *
     * @param name reference name
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
     * @param p the received packet
     * @param rxPowersW the received power per channel band in watts
     */
    void RxCallback(Ptr<const Packet> p, RxPowerWattPerChannelBand rxPowersW);

    Ptr<ExtSpectrumWifiPhy> m_txPhy; ///< TX PHY
    Ptr<ExtSpectrumWifiPhy> m_rxPhy; ///< RX PHY

    MHz_u m_txChannelWidth; ///< TX channel width
    MHz_u m_rxChannelWidth; ///< RX channel width
};

SpectrumWifiPhyFilterTest::SpectrumWifiPhyFilterTest()
    : TestCase("SpectrumWifiPhy test RX filters"),
      m_txChannelWidth(MHz_u{20}),
      m_rxChannelWidth(MHz_u{20})
{
}

SpectrumWifiPhyFilterTest::SpectrumWifiPhyFilterTest(std::string name)
    : TestCase(name)
{
}

void
SpectrumWifiPhyFilterTest::SendPpdu()
{
    WifiTxVector txVector{HePhy::GetHeMcs0(),
                          0,
                          WIFI_PREAMBLE_HE_SU,
                          NanoSeconds(800),
                          1,
                          1,
                          0,
                          m_txChannelWidth,
                          false,
                          false};
    auto pkt = Create<Packet>(1000);
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_QOSDATA);
    hdr.SetQosTid(0);
    hdr.SetAddr1(Mac48Address("00:00:00:00:00:01"));
    hdr.SetSequenceNumber(1);
    auto psdu = Create<WifiPsdu>(pkt, hdr);
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
    for (const auto& [band, powerW] : rxPowersW)
    {
        NS_LOG_INFO(
            "band: (" << band << ") -> powerW=" << powerW
                      << (powerW > 0.0 ? " (" + std::to_string(WToDbm(powerW)) + " dBm)" : ""));
    }

    size_t numBands = rxPowersW.size();
    auto expectedNumBands = std::max<std::size_t>(1, m_rxChannelWidth / MHz_u{20});
    expectedNumBands += (m_rxChannelWidth / MHz_u{40});
    expectedNumBands += (m_rxChannelWidth / MHz_u{80});
    expectedNumBands += (m_rxChannelWidth / MHz_u{160});
    expectedNumBands += m_rxPhy
                            ->GetHeRuBands(m_rxPhy->GetCurrentInterface(),
                                           m_rxPhy->GetGuardBandwidth(
                                               m_rxPhy->GetCurrentInterface()->GetChannelWidth()))
                            .size();

    NS_TEST_ASSERT_MSG_EQ(numBands,
                          expectedNumBands,
                          "Total number of bands handled by the receiver is incorrect");

    MHz_u channelWidth = std::min(m_txChannelWidth, m_rxChannelWidth);
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

    if ((m_txChannelWidth <= m_rxChannelWidth) && (channelWidth >= MHz_u{20}))
    {
        band = m_rxPhy->GetBand(MHz_u{20}, 0); // primary 20 MHz
        it = rxPowersW.find(band);
        NS_LOG_INFO("powerW in primary 20 MHz channel: " << it->second << " (" << WToDbm(it->second)
                                                         << " dBm)");
        const auto rxPowerPrimaryChannel20 = static_cast<int>(WToDbm(it->second) + 0.5);
        const auto expectedRxPowerPrimaryChannel20 =
            16 - static_cast<int>(RatioToDb(Count20MHzSubchannels(channelWidth)));
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

    auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    auto lossModel = CreateObject<FriisPropagationLossModel>();
    lossModel->SetFrequency(5.180e9);
    spectrumChannel->AddPropagationLossModel(lossModel);
    auto delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);

    auto txNode = CreateObject<Node>();
    auto txDev = CreateObject<WifiNetDevice>();
    m_txPhy = CreateObject<ExtSpectrumWifiPhy>();
    auto txInterferenceHelper = CreateObject<InterferenceHelper>();
    m_txPhy->SetInterferenceHelper(txInterferenceHelper);
    auto txErrorModel = CreateObject<NistErrorRateModel>();
    m_txPhy->SetErrorRateModel(txErrorModel);
    m_txPhy->SetDevice(txDev);
    m_txPhy->AddChannel(spectrumChannel);
    m_txPhy->ConfigureStandard(WIFI_STANDARD_80211ax);
    auto apMobility = CreateObject<ConstantPositionMobilityModel>();
    m_txPhy->SetMobility(apMobility);
    txDev->SetPhy(m_txPhy);
    txNode->AggregateObject(apMobility);
    txNode->AddDevice(txDev);

    auto rxNode = CreateObject<Node>();
    auto rxDev = CreateObject<WifiNetDevice>();
    m_rxPhy = CreateObject<ExtSpectrumWifiPhy>();
    auto rxInterferenceHelper = CreateObject<InterferenceHelper>();
    m_rxPhy->SetInterferenceHelper(rxInterferenceHelper);
    auto rxErrorModel = CreateObject<NistErrorRateModel>();
    m_rxPhy->SetErrorRateModel(rxErrorModel);
    m_rxPhy->AddChannel(spectrumChannel);
    m_rxPhy->ConfigureStandard(WIFI_STANDARD_80211ax);
    auto sta1Mobility = CreateObject<ConstantPositionMobilityModel>();
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
    MHz_u txFrequency;
    switch (static_cast<uint16_t>(m_txChannelWidth))
    {
    case 20:
    default:
        txFrequency = MHz_u{5180};
        break;
    case 40:
        txFrequency = MHz_u{5190};
        break;
    case 80:
        txFrequency = MHz_u{5210};
        break;
    case 160:
        txFrequency = MHz_u{5250};
        break;
    }
    auto txChannelNum = WifiPhyOperatingChannel::FindFirst(0,
                                                           txFrequency,
                                                           m_txChannelWidth,
                                                           WIFI_STANDARD_80211ax,
                                                           WIFI_PHY_BAND_5GHZ)
                            ->number;
    m_txPhy->SetOperatingChannel(
        WifiPhy::ChannelTuple{txChannelNum, m_txChannelWidth, WIFI_PHY_BAND_5GHZ, 0});

    MHz_u rxFrequency;
    switch (static_cast<uint16_t>(m_rxChannelWidth))
    {
    case 20:
    default:
        rxFrequency = MHz_u{5180};
        break;
    case 40:
        rxFrequency = MHz_u{5190};
        break;
    case 80:
        rxFrequency = MHz_u{5210};
        break;
    case 160:
        rxFrequency = MHz_u{5250};
        break;
    }
    auto rxChannelNum = WifiPhyOperatingChannel::FindFirst(0,
                                                           rxFrequency,
                                                           m_rxChannelWidth,
                                                           WIFI_STANDARD_80211ax,
                                                           WIFI_PHY_BAND_5GHZ)
                            ->number;
    m_rxPhy->SetOperatingChannel(
        WifiPhy::ChannelTuple{rxChannelNum, m_rxChannelWidth, WIFI_PHY_BAND_5GHZ, 0});

    Simulator::Schedule(Seconds(1), &SpectrumWifiPhyFilterTest::SendPpdu, this);

    Simulator::Run();
}

void
SpectrumWifiPhyFilterTest::DoRun()
{
    m_txChannelWidth = MHz_u{20};
    m_rxChannelWidth = MHz_u{20};
    RunOne();

    m_txChannelWidth = MHz_u{40};
    m_rxChannelWidth = MHz_u{40};
    RunOne();

    m_txChannelWidth = MHz_u{80};
    m_rxChannelWidth = MHz_u{80};
    RunOne();

    m_txChannelWidth = MHz_u{160};
    m_rxChannelWidth = MHz_u{160};
    RunOne();

    m_txChannelWidth = MHz_u{20};
    m_rxChannelWidth = MHz_u{40};
    RunOne();

    m_txChannelWidth = MHz_u{20};
    m_rxChannelWidth = MHz_u{80};
    RunOne();

    m_txChannelWidth = MHz_u{40};
    m_rxChannelWidth = MHz_u{80};
    RunOne();

    m_txChannelWidth = MHz_u{20};
    m_rxChannelWidth = MHz_u{160};
    RunOne();

    m_txChannelWidth = MHz_u{40};
    m_rxChannelWidth = MHz_u{160};
    RunOne();

    m_txChannelWidth = MHz_u{80};
    m_rxChannelWidth = MHz_u{160};
    RunOne();

    m_txChannelWidth = MHz_u{40};
    m_rxChannelWidth = MHz_u{20};
    RunOne();

    m_txChannelWidth = MHz_u{80};
    m_rxChannelWidth = MHz_u{20};
    RunOne();

    m_txChannelWidth = MHz_u{80};
    m_rxChannelWidth = MHz_u{40};
    RunOne();

    m_txChannelWidth = MHz_u{160};
    m_rxChannelWidth = MHz_u{20};
    RunOne();

    m_txChannelWidth = MHz_u{160};
    m_rxChannelWidth = MHz_u{40};
    RunOne();

    m_txChannelWidth = MHz_u{160};
    m_rxChannelWidth = MHz_u{80};
    RunOne();

    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Spectrum Wifi Phy Bands Calculations Test
 *
 * This test verifies SpectrumWifiPhy::GetBand produces the expected results, for both contiguous
 * (160 MHz) and non-contiguous (80+80MHz) operating channel
 */
class SpectrumWifiPhyGetBandTest : public TestCase
{
  public:
    SpectrumWifiPhyGetBandTest();

  private:
    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     * Run one function
     * @param channelNumberPerSegment the channel number for each segment of the operating channel
     * @param bandWidth the width of the band to test
     * @param bandIndex the index of the band to test
     * @param expectedIndices the expected start and stop indices returned by
     * SpectrumWifiPhy::GetBand \param expectedFrequencies the expected start and stop frequencies
     * returned by SpectrumWifiPhy::GetBand
     */
    void RunOne(const std::vector<uint8_t>& channelNumberPerSegment,
                MHz_u bandWidth,
                uint8_t bandIndex,
                const std::vector<WifiSpectrumBandIndices>& expectedIndices,
                const std::vector<WifiSpectrumBandFrequencies>& expectedFrequencies);

    Ptr<SpectrumWifiPhy> m_phy; ///< PHY
};

SpectrumWifiPhyGetBandTest::SpectrumWifiPhyGetBandTest()
    : TestCase("SpectrumWifiPhy test bands calculations")
{
}

void
SpectrumWifiPhyGetBandTest::DoSetup()
{
    // WifiHelper::EnableLogComponents();
    // LogComponentEnable("SpectrumWifiPhyTest", LOG_LEVEL_ALL);

    auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    auto lossModel = CreateObject<FriisPropagationLossModel>();
    lossModel->SetFrequency(5.180e9);
    spectrumChannel->AddPropagationLossModel(lossModel);
    auto delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);

    auto node = CreateObject<Node>();
    auto dev = CreateObject<WifiNetDevice>();
    m_phy = CreateObject<SpectrumWifiPhy>();
    auto interferenceHelper = CreateObject<InterferenceHelper>();
    m_phy->SetInterferenceHelper(interferenceHelper);
    auto errorModel = CreateObject<NistErrorRateModel>();
    m_phy->SetErrorRateModel(errorModel);
    m_phy->SetDevice(dev);
    m_phy->AddChannel(spectrumChannel);
    m_phy->ConfigureStandard(WIFI_STANDARD_80211ax);
    dev->SetPhy(m_phy);
    node->AddDevice(dev);
}

void
SpectrumWifiPhyGetBandTest::DoTeardown()
{
    m_phy->Dispose();
    m_phy = nullptr;
}

void
SpectrumWifiPhyGetBandTest::RunOne(
    const std::vector<uint8_t>& channelNumberPerSegment,
    MHz_u bandWidth,
    uint8_t bandIndex,
    const std::vector<WifiSpectrumBandIndices>& expectedIndices,
    const std::vector<WifiSpectrumBandFrequencies>& expectedFrequencies)
{
    WifiPhy::ChannelSegments channelSegments;
    for (auto channelNumber : channelNumberPerSegment)
    {
        const auto& channelInfo = WifiPhyOperatingChannel::FindFirst(channelNumber,
                                                                     MHz_u{0},
                                                                     MHz_u{0},
                                                                     WIFI_STANDARD_80211ax,
                                                                     WIFI_PHY_BAND_5GHZ);
        channelSegments.emplace_back(channelInfo->number, channelInfo->width, channelInfo->band, 0);
    }
    m_phy->SetOperatingChannel(channelSegments);

    const auto& bandInfo = m_phy->GetBand(bandWidth, bandIndex);
    NS_ASSERT(expectedIndices.size() == expectedFrequencies.size());
    NS_ASSERT(expectedIndices.size() == bandInfo.indices.size());
    NS_ASSERT(expectedFrequencies.size() == bandInfo.frequencies.size());
    for (std::size_t i = 0; i < expectedIndices.size(); ++i)
    {
        NS_ASSERT(bandInfo.indices.at(i).first == expectedIndices.at(i).first);
        NS_TEST_ASSERT_MSG_EQ(bandInfo.indices.at(i).first,
                              expectedIndices.at(i).first,
                              "Incorrect start indice for segment " << i);
        NS_TEST_ASSERT_MSG_EQ(bandInfo.indices.at(i).second,
                              expectedIndices.at(i).second,
                              "Incorrect stop indice for segment " << i);
        NS_TEST_ASSERT_MSG_EQ(bandInfo.frequencies.at(i).first,
                              expectedFrequencies.at(i).first,
                              "Incorrect start frequency for segment " << i);
        NS_TEST_ASSERT_MSG_EQ(bandInfo.frequencies.at(i).second,
                              expectedFrequencies.at(i).second,
                              "Incorrect stop frequency for segment " << i);
    }
}

void
SpectrumWifiPhyGetBandTest::DoRun()
{
    const uint32_t indicesPer20MhzBand = 256; // based on 802.11ax carrier spacing
    const MHz_u channelWidth{160};            // we consider the largest channel width
    const uint8_t channelNumberContiguous160Mhz =
        50; // channel number of the first 160 MHz band in 5 GHz band
    const std::vector<uint8_t> channelNumberPerSegment = {42,
                                                          106}; // channel numbers used for 80+80MHz
    // separation between segment at channel number 42 and segment at channel number 106
    const MHz_u separationWidth{240};
    for (bool contiguous160Mhz : {true /* 160 MHz */, false /* 80+80MHz */})
    {
        const auto guardWidth = contiguous160Mhz ? channelWidth : (channelWidth / 2);
        uint32_t guardStopIndice = (indicesPer20MhzBand * Count20MHzSubchannels(guardWidth)) - 1;
        std::vector<WifiSpectrumBandIndices> previousExpectedIndices{};
        std::vector<WifiSpectrumBandFrequencies> previousExpectedFrequencies{};
        for (auto bandWidth : {MHz_u{20}, MHz_u{40}, MHz_u{80}, MHz_u{160}})
        {
            const uint32_t expectedStartIndice = guardStopIndice + 1;
            const uint32_t expectedStopIndice =
                expectedStartIndice + (indicesPer20MhzBand * Count20MHzSubchannels(bandWidth)) - 1;
            std::vector<WifiSpectrumBandIndices> expectedIndices{
                {expectedStartIndice, expectedStopIndice}};
            const Hz_u expectedStartFrequency = MHzToHz(5170);
            const Hz_u expectedStopFrequency = MHzToHz(5170 + bandWidth);
            std::vector<WifiSpectrumBandFrequencies> expectedFrequencies{
                {expectedStartFrequency, expectedStopFrequency}};
            const std::size_t numBands = (channelWidth / bandWidth);
            for (std::size_t i = 0; i < numBands; ++i)
            {
                if ((bandWidth != channelWidth) && (i >= (numBands / 2)))
                {
                    // skip DC
                    expectedIndices.at(0).first++;
                }
                if ((bandWidth == channelWidth) && !contiguous160Mhz)
                {
                    // For contiguous 160 MHz, band is made of the two 80 MHz segments (previous run
                    // in the loop)
                    expectedIndices = previousExpectedIndices;
                    expectedFrequencies = previousExpectedFrequencies;
                }
                else if ((i == (numBands / 2)) && !contiguous160Mhz)
                {
                    expectedIndices.at(0).first +=
                        (indicesPer20MhzBand * Count20MHzSubchannels(separationWidth));
                    expectedIndices.at(0).second +=
                        (indicesPer20MhzBand * Count20MHzSubchannels(separationWidth));
                    expectedFrequencies.at(0).first += MHzToHz(separationWidth);
                    expectedFrequencies.at(0).second += MHzToHz(separationWidth);
                }
                RunOne(contiguous160Mhz ? std::vector<uint8_t>{channelNumberContiguous160Mhz}
                                        : channelNumberPerSegment,
                       bandWidth,
                       i,
                       expectedIndices,
                       expectedFrequencies);
                if (!contiguous160Mhz && (bandWidth == (channelWidth / 2)))
                {
                    previousExpectedIndices.emplace_back(expectedIndices.front());
                    previousExpectedFrequencies.emplace_back(expectedFrequencies.front());
                }
                expectedIndices.at(0).first = expectedIndices.at(0).second + 1;
                expectedIndices.at(0).second =
                    expectedIndices.at(0).first +
                    (indicesPer20MhzBand * Count20MHzSubchannels(bandWidth)) - 1;
                expectedFrequencies.at(0).first += MHzToHz(bandWidth);
                expectedFrequencies.at(0).second += MHzToHz(bandWidth);
            }
        }
    }

    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test tracked bands in interference helper upon channel switching
 *
 * The test is verifying that the correct bands are tracked by the interference helper upon channel
 * switching. It focuses on 80 and 160 MHz bands while considering 160 MHz operating channels, for
 * both contiguous and non-contiguous cases.
 */
class SpectrumWifiPhyTrackedBandsTest : public TestCase
{
  public:
    SpectrumWifiPhyTrackedBandsTest();

  private:
    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     * Switch channel function
     *
     * @param channelNumberPerSegment the channel number for each segment of the operating channel
     * to switch to
     */
    void SwitchChannel(const std::vector<uint8_t>& channelNumberPerSegment);

    /**
     * Verify the bands tracked by the interference helper
     *
     * @param expectedTrackedBands the bands that are expected to be tracked by the interference
     * helper
     * @param expectedUntrackedBands the bands that are expected to be untracked by the
     * interference helper
     */
    void VerifyTrackedBands(
        const std::vector<std::vector<WifiSpectrumBandFrequencies>>& expectedTrackedBands,
        const std::vector<std::vector<WifiSpectrumBandFrequencies>>& expectedUntrackedBands);

    /**
     * Run one function
     * @param channelNumberPerSegmentBeforeSwitching the channel number for each segment of the
     * operating channel to switch from \param channelNumberPerSegmentAfterSwitching the channel
     * number for each segment of the operating channel to switch to \param expectedTrackedBands the
     * bands that are expected to be tracked by the interference helper \param expectedUntrackedBand
     * the bands that are expected to be untracked by the interference helper
     */
    void RunOne(const std::vector<uint8_t>& channelNumberPerSegmentBeforeSwitching,
                const std::vector<uint8_t>& channelNumberPerSegmentAfterSwitching,
                const std::vector<std::vector<WifiSpectrumBandFrequencies>>& expectedTrackedBands,
                const std::vector<std::vector<WifiSpectrumBandFrequencies>>& expectedUntrackedBand);

    Ptr<ExtSpectrumWifiPhy> m_phy; ///< PHY
};

SpectrumWifiPhyTrackedBandsTest::SpectrumWifiPhyTrackedBandsTest()
    : TestCase("SpectrumWifiPhy test channel switching for non-contiguous operating channels")
{
}

void
SpectrumWifiPhyTrackedBandsTest::DoSetup()
{
    // WifiHelper::EnableLogComponents();
    // LogComponentEnable("SpectrumWifiPhyTest", LOG_LEVEL_ALL);

    auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    auto lossModel = CreateObject<FriisPropagationLossModel>();
    lossModel->SetFrequency(5.180e9);
    spectrumChannel->AddPropagationLossModel(lossModel);
    auto delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);

    auto node = CreateObject<Node>();
    auto dev = CreateObject<WifiNetDevice>();
    m_phy = CreateObject<ExtSpectrumWifiPhy>();
    auto interferenceHelper = CreateObject<ExtInterferenceHelper>();
    m_phy->SetInterferenceHelper(interferenceHelper);
    auto errorModel = CreateObject<NistErrorRateModel>();
    m_phy->SetErrorRateModel(errorModel);
    m_phy->SetDevice(dev);
    m_phy->AddChannel(spectrumChannel);
    m_phy->ConfigureStandard(WIFI_STANDARD_80211ax);
    dev->SetPhy(m_phy);
    node->AddDevice(dev);
}

void
SpectrumWifiPhyTrackedBandsTest::DoTeardown()
{
    m_phy->Dispose();
    m_phy = nullptr;
}

void
SpectrumWifiPhyTrackedBandsTest::SwitchChannel(const std::vector<uint8_t>& channelNumberPerSegment)
{
    NS_LOG_FUNCTION(this);
    WifiPhy::ChannelSegments channelSegments;
    for (auto channelNumber : channelNumberPerSegment)
    {
        const auto& channelInfo = WifiPhyOperatingChannel::FindFirst(channelNumber,
                                                                     MHz_u{0},
                                                                     MHz_u{0},
                                                                     WIFI_STANDARD_80211ax,
                                                                     WIFI_PHY_BAND_5GHZ);
        channelSegments.emplace_back(channelInfo->number, channelInfo->width, channelInfo->band, 0);
    }
    m_phy->SetOperatingChannel(channelSegments);
}

void
SpectrumWifiPhyTrackedBandsTest::VerifyTrackedBands(
    const std::vector<std::vector<WifiSpectrumBandFrequencies>>& expectedTrackedBands,
    const std::vector<std::vector<WifiSpectrumBandFrequencies>>& expectedUntrackedBands)
{
    NS_LOG_FUNCTION(this);
    PointerValue ptr;
    m_phy->GetAttribute("InterferenceHelper", ptr);
    auto interferenceHelper = DynamicCast<ExtInterferenceHelper>(ptr.Get<ExtInterferenceHelper>());
    NS_ASSERT(interferenceHelper);
    auto printBand = [](const std::vector<WifiSpectrumBandFrequencies>& v) {
        std::stringstream ss;
        for (const auto& [start, stop] : v)
        {
            ss << "[" << start << "-" << stop << "] ";
        }
        return ss.str();
    };
    for (const auto& expectedTrackedBand : expectedTrackedBands)
    {
        auto bandTracked = interferenceHelper->IsBandTracked(expectedTrackedBand);
        NS_TEST_ASSERT_MSG_EQ(bandTracked,
                              true,
                              "Band " << printBand(expectedTrackedBand) << " is not tracked");
    }
    for (const auto& expectedUntrackedBand : expectedUntrackedBands)
    {
        auto bandTracked = interferenceHelper->IsBandTracked(expectedUntrackedBand);
        NS_TEST_ASSERT_MSG_EQ(bandTracked,
                              false,
                              "Band " << printBand(expectedUntrackedBand)
                                      << " is unexpectedly tracked");
    }
}

void
SpectrumWifiPhyTrackedBandsTest::RunOne(
    const std::vector<uint8_t>& channelNumberPerSegmentBeforeSwitching,
    const std::vector<uint8_t>& channelNumberPerSegmentAfterSwitching,
    const std::vector<std::vector<WifiSpectrumBandFrequencies>>& expectedTrackedBands,
    const std::vector<std::vector<WifiSpectrumBandFrequencies>>& expectedUntrackedBands)
{
    NS_LOG_FUNCTION(this);

    Simulator::Schedule(Seconds(0),
                        &SpectrumWifiPhyTrackedBandsTest::SwitchChannel,
                        this,
                        channelNumberPerSegmentBeforeSwitching);

    Simulator::Schedule(Seconds(1),
                        &SpectrumWifiPhyTrackedBandsTest::SwitchChannel,
                        this,
                        channelNumberPerSegmentAfterSwitching);

    Simulator::Schedule(Seconds(2),
                        &SpectrumWifiPhyTrackedBandsTest::VerifyTrackedBands,
                        this,
                        expectedTrackedBands,
                        expectedUntrackedBands);

    Simulator::Run();
}

void
SpectrumWifiPhyTrackedBandsTest::DoRun()
{
    // switch from 160 MHz to 80+80 MHz
    RunOne({50},
           {42, 106},
           {{{MHzToHz(5170), MHzToHz(5250)}} /* first 80 MHz segment */,
            {{MHzToHz(5490), MHzToHz(5570)}} /* second 80 MHz segment */,
            {{MHzToHz(5170), MHzToHz(5250)},
             {MHzToHz(5490),
              MHzToHz(5570)}} /* non-contiguous 160 MHz band made of the two segments */},
           {{{MHzToHz(5170), MHzToHz(5330)}} /* full 160 MHz band should have been removed */});

    // switch from 80+80 MHz to 160 MHz
    RunOne(
        {42, 106},
        {50},
        {{{MHzToHz(5170), MHzToHz(5330)}} /* full 160 MHz band */,
         {{MHzToHz(5170), MHzToHz(5250)}} /* first 80 MHz segment is part of the 160 MHz channel*/},
        {{{MHzToHz(5490), MHzToHz(5570)}} /* second 80 MHz segment should have been removed */,
         {{MHzToHz(5170), MHzToHz(5250)},
          {MHzToHz(5490),
           MHzToHz(5570)}} /* non-contiguous 160 MHz band should have been removed */});

    // switch from 80+80 MHz to 80+80 MHz with segment swap
    RunOne({42, 106},
           {106, 42},
           {{{MHzToHz(5490), MHzToHz(5570)}} /* first 80 MHz segment */,
            {{MHzToHz(5490), MHzToHz(5570)}} /* second 80 MHz segment */,
            {{MHzToHz(5170), MHzToHz(5250)},
             {MHzToHz(5490),
              MHzToHz(5570)}} /* non-contiguous 160 MHz band made of the two segments */},
           {});

    // switch from 80+80 MHz to another 80+80 MHz with one common segment
    RunOne({42, 106},
           {106, 138},
           {{{MHzToHz(5490), MHzToHz(5570)}} /* first 80 MHz segment */,
            {{MHzToHz(5650), MHzToHz(5730)}} /* second 80 MHz segment */,
            {{MHzToHz(5490), MHzToHz(5570)},
             {MHzToHz(5650),
              MHzToHz(5730)}} /* non-contiguous 160 MHz band made of the two segments */},
           {{{MHzToHz(5170),
              MHzToHz(5250)}} /* 80 MHz segment at channel 42 should have been removed */,
            {{MHzToHz(5170), MHzToHz(5250)},
             {MHzToHz(5490),
              MHzToHz(5570)}} /* previous non-contiguous 160 MHz band should have been removed */});

    // switch from 80+80 MHz to another 80+80 MHz with no common segment
    RunOne({42, 106},
           {122, 155},
           {{{MHzToHz(5570), MHzToHz(5650)}} /* first 80 MHz segment */,
            {{MHzToHz(5735), MHzToHz(5815)}} /* second 80 MHz segment */,
            {{MHzToHz(5570), MHzToHz(5650)},
             {MHzToHz(5735),
              MHzToHz(5815)}} /* non-contiguous 160 MHz band made of the two segments */},
           {{{MHzToHz(5170),
              MHzToHz(5250)}} /* previous first 80 MHz segment should have been removed */,
            {{MHzToHz(5490),
              MHzToHz(5570)}} /* previous second 80 MHz segment should have been removed */,
            {{MHzToHz(5170), MHzToHz(5250)},
             {MHzToHz(5490),
              MHzToHz(5570)}} /* previous non-contiguous 160 MHz band should have been removed */});

    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test 80+80MHz transmission
 *
 * The test verifies that two non-contiguous segments are handled by the spectrum PHY
 * to transmit 160 MHz PPDUs when the operating channel is configured as 80+80MHz.
 *
 * The test first considers a contiguous 160 MHz segment and generate interference on the second
 * 80 MHz band to verify reception fails in this scenario. Then, a similar interference
 * is generated when a 80+80MHz operating channel is configured, where the first frequency segment
 * occupies the first 80 MHz band of the previous 160 MHz operating channel. The reception should
 * succeed in that scenario, which demonstrates the second 80 MHz band of the operating channel is
 * no longer occupying that spectrum portion (the interference is hence is the gap between the two
 * frequency segments). Finally, the test also generates interference on each of the frequency
 * segments when the operating channel is 80+80MHz, to demonstrate the frequency segments are
 * positioned as expected.
 */
class SpectrumWifiPhy80Plus80Test : public TestCase
{
  public:
    SpectrumWifiPhy80Plus80Test();

  private:
    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     * Run one function
     * @param channelNumbers the channel number for each segment of the operating channel
     * @param interferenceCenterFrequency the center frequency of the interference signal to
     * generate
     * @param interferenceBandWidth the band width of the interference signal to generate
     * @param expectSuccess flag to indicate whether reception is expected to be successful
     */
    void RunOne(const std::vector<uint8_t>& channelNumbers,
                MHz_u interferenceCenterFrequency,
                MHz_u interferenceBandWidth,
                bool expectSuccess);

    /**
     * Switch channel function
     *
     * @param channelNumbers the channel number for each segment of the operating channel
     * to switch to
     */
    void SwitchChannel(const std::vector<uint8_t>& channelNumbers);

    /**
     * Send 160MHz PPDU function
     */
    void Send160MhzPpdu();

    /**
     * Generate interference function
     * @param interferencePsd the PSD of the interference to be generated
     * @param duration the duration of the interference
     */
    void GenerateInterference(Ptr<SpectrumValue> interferencePsd, Time duration);

    /**
     * Stop interference function
     */
    void StopInterference();

    /**
     * Receive success function for STA
     * @param psdu the PSDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector the transmit vector
     * @param statusPerMpdu reception status per MPDU
     */
    void RxSuccessSta(Ptr<const WifiPsdu> psdu,
                      RxSignalInfo rxSignalInfo,
                      const WifiTxVector& txVector,
                      const std::vector<bool>& statusPerMpdu);

    /**
     * Receive failure function for STA
     * @param psdu the PSDU
     */
    void RxFailureSta(Ptr<const WifiPsdu> psdu);

    /**
     * Verify results
     *
     * @param expectSuccess flag to indicate whether reception is expected to be successful
     */
    void CheckResults(bool expectSuccess);

    Ptr<SpectrumWifiPhy> m_phyAp;           ///< PHY of AP
    Ptr<SpectrumWifiPhy> m_phySta;          ///< PHY of STA
    Ptr<WaveformGenerator> m_phyInterferer; ///< PHY of interferer

    uint32_t m_countRxSuccessSta; ///< count RX success for STA
    uint32_t m_countRxFailureSta; ///< count RX failure for STA
};

SpectrumWifiPhy80Plus80Test::SpectrumWifiPhy80Plus80Test()
    : TestCase("SpectrumWifiPhy test 80+80MHz transmission"),
      m_countRxSuccessSta(0),
      m_countRxFailureSta(0)
{
}

void
SpectrumWifiPhy80Plus80Test::SwitchChannel(const std::vector<uint8_t>& channelNumbers)
{
    NS_LOG_FUNCTION(this);
    WifiPhy::ChannelSegments channelSegments;
    for (auto channelNumber : channelNumbers)
    {
        const auto& channelInfo = WifiPhyOperatingChannel::FindFirst(channelNumber,
                                                                     MHz_u{0},
                                                                     MHz_u{0},
                                                                     WIFI_STANDARD_80211ax,
                                                                     WIFI_PHY_BAND_5GHZ);
        channelSegments.emplace_back(channelInfo->number, channelInfo->width, channelInfo->band, 0);
    }
    m_phyAp->SetOperatingChannel(channelSegments);
    m_phySta->SetOperatingChannel(channelSegments);
}

void
SpectrumWifiPhy80Plus80Test::Send160MhzPpdu()
{
    NS_LOG_FUNCTION(this);

    WifiTxVector txVector{HePhy::GetHeMcs7(),
                          0,
                          WIFI_PREAMBLE_HE_SU,
                          NanoSeconds(800),
                          1,
                          1,
                          0,
                          MHz_u{160},
                          false,
                          false,
                          false};

    auto pkt = Create<Packet>(1000);
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_QOSDATA);
    auto psdu = Create<WifiPsdu>(pkt, hdr);

    m_phyAp->Send(psdu, txVector);
}

void
SpectrumWifiPhy80Plus80Test::GenerateInterference(Ptr<SpectrumValue> interferencePsd, Time duration)
{
    m_phyInterferer->SetTxPowerSpectralDensity(interferencePsd);
    m_phyInterferer->SetPeriod(duration);
    m_phyInterferer->Start();
    Simulator::Schedule(duration, &SpectrumWifiPhy80Plus80Test::StopInterference, this);
}

void
SpectrumWifiPhy80Plus80Test::StopInterference()
{
    m_phyInterferer->Stop();
}

void
SpectrumWifiPhy80Plus80Test::RxSuccessSta(Ptr<const WifiPsdu> psdu,
                                          RxSignalInfo rxSignalInfo,
                                          const WifiTxVector& txVector,
                                          const std::vector<bool>& /*statusPerMpdu*/)
{
    NS_LOG_FUNCTION(this << *psdu << rxSignalInfo << txVector);
    m_countRxSuccessSta++;
}

void
SpectrumWifiPhy80Plus80Test::RxFailureSta(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << *psdu);
    m_countRxFailureSta++;
}

void
SpectrumWifiPhy80Plus80Test::CheckResults(bool expectSuccess)
{
    NS_LOG_FUNCTION(this << expectSuccess);
    NS_TEST_ASSERT_MSG_EQ(((m_countRxSuccessSta > 0) && (m_countRxFailureSta == 0)),
                          expectSuccess,
                          "Reception should be "
                              << (expectSuccess ? "successful" : "unsuccessful"));
}

void
SpectrumWifiPhy80Plus80Test::DoSetup()
{
    // WifiHelper::EnableLogComponents();
    // LogComponentEnable("SpectrumWifiPhyTest", LOG_LEVEL_ALL);

    auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    auto lossModel = CreateObject<FriisPropagationLossModel>();
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

    auto staNode = CreateObject<Node>();
    auto staDev = CreateObject<WifiNetDevice>();
    m_phySta = CreateObject<SpectrumWifiPhy>();
    auto staInterferenceHelper = CreateObject<InterferenceHelper>();
    m_phySta->SetInterferenceHelper(staInterferenceHelper);
    auto staErrorModel = CreateObject<NistErrorRateModel>();
    m_phySta->SetErrorRateModel(staErrorModel);
    m_phySta->SetDevice(staDev);
    m_phySta->AddChannel(spectrumChannel);
    m_phySta->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phySta->SetReceiveOkCallback(MakeCallback(&SpectrumWifiPhy80Plus80Test::RxSuccessSta, this));
    m_phySta->SetReceiveErrorCallback(
        MakeCallback(&SpectrumWifiPhy80Plus80Test::RxFailureSta, this));
    auto staMobility = CreateObject<ConstantPositionMobilityModel>();
    m_phySta->SetMobility(staMobility);
    staDev->SetPhy(m_phySta);
    staNode->AggregateObject(staMobility);
    staNode->AddDevice(staDev);

    auto interfererNode = CreateObject<Node>();
    auto interfererDev = CreateObject<NonCommunicatingNetDevice>();
    m_phyInterferer = CreateObject<WaveformGenerator>();
    m_phyInterferer->SetDevice(interfererDev);
    m_phyInterferer->SetChannel(spectrumChannel);
    m_phyInterferer->SetDutyCycle(1);
    interfererNode->AddDevice(interfererDev);
}

void
SpectrumWifiPhy80Plus80Test::DoTeardown()
{
    m_phyAp->Dispose();
    m_phyAp = nullptr;
    m_phySta->Dispose();
    m_phySta = nullptr;
    m_phyInterferer->Dispose();
    m_phyInterferer = nullptr;
}

void
SpectrumWifiPhy80Plus80Test::RunOne(const std::vector<uint8_t>& channelNumbers,
                                    MHz_u interferenceCenterFrequency,
                                    MHz_u interferenceBandWidth,
                                    bool expectSuccess)
{
    // reset counters
    m_countRxSuccessSta = 0;
    m_countRxFailureSta = 0;

    Simulator::Schedule(Seconds(0),
                        &SpectrumWifiPhy80Plus80Test::SwitchChannel,
                        this,
                        channelNumbers);

    // create info about interference to generate
    BandInfo bandInfo{.fl = MHzToHz(interferenceCenterFrequency - (interferenceBandWidth / 2)),
                      .fc = MHzToHz(interferenceCenterFrequency),
                      .fh = MHzToHz(interferenceCenterFrequency + (interferenceBandWidth / 2))};
    auto spectrumInterference = Create<SpectrumModel>(Bands{bandInfo});
    auto interferencePsd = Create<SpectrumValue>(spectrumInterference);
    Watt_u interferencePower{0.1};
    *interferencePsd = interferencePower / (interferenceBandWidth * 20e6);

    Simulator::Schedule(Seconds(1),
                        &SpectrumWifiPhy80Plus80Test::GenerateInterference,
                        this,
                        interferencePsd,
                        MilliSeconds(100));

    Simulator::Schedule(Seconds(1), &SpectrumWifiPhy80Plus80Test::Send160MhzPpdu, this);

    Simulator::Schedule(Seconds(2),
                        &SpectrumWifiPhy80Plus80Test::CheckResults,
                        this,
                        expectSuccess);

    Simulator::Run();
}

void
SpectrumWifiPhy80Plus80Test::DoRun()
{
    // Test transmission over contiguous 160 MHz (channel 50) and interference generated in
    // the second half of the channel width (channel 58, i.e. center frequency 5290 and bandwidth 80
    // MHz). The reception should fail because the interference occupies half the channel width used
    // for the transmission.
    //                                       ┌──────────────────┐
    // Interference                          │    channel 58    │
    //                                       │ 5290 MHz, 80 MHz │
    //                                       └──────────────────┘
    //
    //                   ┌──────────────────────────────────────┐
    // Operating Channel │             channel 50               │
    //                   │          5250 MHz, 160 MHz           │
    //                   └──────────────────────────────────────┘
    //
    RunOne({50}, MHz_u{5290}, MHz_u{80}, false);

    // Test transmission over non-contiguous 160 MHz (i.e. 80+80MHz) and same interference as in
    // previous run. The reception should succeed because the interference is located between the
    // two segments.
    //                                       ┌──────────────────┐
    // Interference                          │    channel 58    │
    //                                       │ 5290 MHz, 80 MHz │
    //                                       └──────────────────┘
    //
    //                   ┌──────────────────┐                             ┌──────────────────┐
    // Operating Channel │    channel 42    │                             │   channel 106    │
    //                   │80+80MHz segment 0│                             │80+80MHz segment 1│
    //                   └──────────────────┘                             └──────────────────┘
    //
    RunOne({42, 106}, MHz_u{5290}, MHz_u{80}, true);

    // Test transmission over non-contiguous 160 MHz (i.e. 80+80MHz) and interference generated on
    // the first segment of the channel width (channel 42, i.e. center frequency 5210 and bandwidth
    // 80 MHz). The reception should fail because the interference occupies half the channel width
    // used for the transmission.
    //                   ┌──────────────────┐
    // Interference      │    channel 42    │
    //                   │ 5210 MHz, 80 MHz │
    //                   └──────────────────┘
    //
    //                   ┌──────────────────┐                             ┌──────────────────┐
    // Operating Channel │    channel 42    │                             │   channel 106    │
    //                   │80+80MHz segment 0│                             │80+80MHz segment 1│
    //                   └──────────────────┘                             └──────────────────┘
    //
    RunOne({42, 106}, MHz_u{5210}, MHz_u{80}, false);

    // Test transmission over non-contiguous 160 MHz (i.e. 80+80MHz) and interference generated on
    // the second segment of the channel width (channel 42, i.e. center frequency 5210 and bandwidth
    // 80 MHz). The reception should fail because the interference occupies half the channel width
    // used for the transmission.
    //                                                                    ┌──────────────────┐
    // Interference                                                       │    channel 106    │
    //                                                                    │ 5530 MHz, 80 MHz │
    //                                                                    └──────────────────┘
    //
    //                   ┌──────────────────┐                             ┌──────────────────┐
    // Operating Channel │    channel 42    │                             │   channel 106    │
    //                   │80+80MHz segment 0│                             │80+80MHz segment 1│
    //                   └──────────────────┘                             └──────────────────┘
    //
    RunOne({42, 106}, MHz_u{5530}, MHz_u{80}, false);

    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Spectrum Wifi Phy Multiple Spectrum Test
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
    /// Enumeration for channel switching scenarios
    enum class ChannelSwitchScenario : uint8_t
    {
        BEFORE_TX = 0, //!< start TX after channel switch is completed
        BETWEEN_TX_RX  //!< perform channel switch during propagation delay (after TX and before RX)
    };

    /**
     * Constructor
     *
     * @param trackSignalsInactiveInterfaces flag to indicate whether signals coming from inactive
     * spectrum PHY interfaces shall be tracked during the test
     * @param chanSwitchScenario the channel switching scenario to consider for the test
     */
    SpectrumWifiPhyMultipleInterfacesTest(bool trackSignalsInactiveInterfaces,
                                          ChannelSwitchScenario chanSwitchScenario);

  private:
    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     * Switch channel function
     *
     * @param phy the PHY to switch
     * @param band the PHY band to use
     * @param channelNumber number the channel number to use
     * @param channelWidth the channel width to use
     * @param listenerIndex index of the listener for that PHY, if PHY is a RX PHY
     */
    void SwitchChannel(Ptr<SpectrumWifiPhy> phy,
                       WifiPhyBand band,
                       uint8_t channelNumber,
                       MHz_u channelWidth,
                       std::optional<std::size_t> listenerIndex);

    /**
     * Send PPDU function
     *
     * @param phy the PHY to transmit the signal
     * @param txPower the power to transmit the signal (this is also the received power since we do
     * not have propagation loss to simplify) \param payloadSize the payload size in bytes
     */
    void SendPpdu(Ptr<SpectrumWifiPhy> phy, dBm_u txPower, uint32_t payloadSize);

    /**
     * Callback triggered when a packet is received by a PHY
     * @param index the index to identify the RX PHY
     * @param packet the received packet
     * @param rxPowersW the received power per channel band in watts
     */
    void RxCallback(std::size_t index,
                    Ptr<const Packet> packet,
                    RxPowerWattPerChannelBand rxPowersW);

    /**
     * Receive success function
     * @param index index of the RX STA
     * @param psdu the PSDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector the transmit vector
     * @param statusPerMpdu reception status per MPDU
     */
    void RxSuccess(std::size_t index,
                   Ptr<const WifiPsdu> psdu,
                   RxSignalInfo rxSignalInfo,
                   const WifiTxVector& txVector,
                   const std::vector<bool>& statusPerMpdu);

    /**
     * Receive failure function
     * @param index index of the RX STA
     * @param psdu the PSDU
     */
    void RxFailure(std::size_t index, Ptr<const WifiPsdu> psdu);

    /**
     * Schedule now to check the interferences
     * @param phy the PHY for which the check has to be executed
     * @param freqRange the frequency range for which the check has to be executed
     * @param band the band for which the check has to be executed
     * @param interferencesExpected flag whether interferences are expected to have been tracked
     */
    void CheckInterferences(Ptr<SpectrumWifiPhy> phy,
                            const FrequencyRange& freqRange,
                            const WifiSpectrumBandInfo& band,
                            bool interferencesExpected);

    /**
     * Check the interferences
     * @param phy the PHY for which the check has to be executed
     * @param band the band for which the check has to be executed
     * @param interferencesExpected flag whether interferences are expected to have been tracked
     */
    void DoCheckInterferences(Ptr<SpectrumWifiPhy> phy,
                              const WifiSpectrumBandInfo& band,
                              bool interferencesExpected);

    /**
     * Verify results
     *
     * @param index the index to identify the RX PHY to check
     * @param expectedNumRx the expected number of RX events for that PHY
     * @param expectedNumRxSuccess the expected amount of successfully received packets
     * @param expectedRxBytes the expected amount of received bytes
     * @param expectedFrequencyRangeActiveRfInterface the expected frequency range (in MHz) of the
     * active RF interface
     * @param expectedConnectedPhysPerChannel the expected number of PHYs attached for each spectrum
     * channel
     */
    void CheckResults(std::size_t index,
                      uint32_t expectedNumRx,
                      uint32_t expectedNumRxSuccess,
                      uint32_t expectedRxBytes,
                      FrequencyRange expectedFrequencyRangeActiveRfInterface,
                      const std::vector<std::size_t>& expectedConnectedPhysPerChannel);

    /**
     * Verify CCA indication reported by a given PHY
     *
     * @param index the index to identify the RX PHY to check
     * @param expectedCcaBusyIndication flag to indicate whether a CCA BUSY notification is expected
     * @param switchingDelay delay between the TX has started and the time RX switched to the TX
     * channel
     * @param propagationDelay the propagation delay
     */
    void CheckCcaIndication(std::size_t index,
                            bool expectedCcaBusyIndication,
                            Time switchingDelay,
                            Time propagationDelay);

    /**
     * Verify rxing state of the interference helper
     *
     * @param phy the PHY to which the interference helper instance is attached
     * @param rxingExpected flag whether the interference helper is expected to be in rxing state or
     * not
     */
    void CheckRxingState(Ptr<SpectrumWifiPhy> phy, bool rxingExpected);

    /**
     * Reset function
     */
    void Reset();

    bool
        m_trackSignalsInactiveInterfaces; //!< flag to indicate whether signals coming from inactive
                                          //!< spectrum PHY interfaces are tracked during the test
    ChannelSwitchScenario
        m_chanSwitchScenario; //!< the channel switch scenario to consider for the test
    std::vector<Ptr<MultiModelSpectrumChannel>> m_spectrumChannels; //!< Spectrum channels
    std::vector<Ptr<SpectrumWifiPhy>> m_txPhys{};                   //!< TX PHYs
    std::vector<Ptr<SpectrumWifiPhy>> m_rxPhys{};                   //!< RX PHYs
    std::vector<std::shared_ptr<TestPhyListener>> m_listeners{};    //!< listeners

    std::vector<uint32_t> m_counts; //!< count number of packets received by PHYs
    std::vector<uint32_t>
        m_countRxSuccess; //!< count number of packets successfully received by PHYs
    std::vector<uint32_t>
        m_countRxFailure;            //!< count number of packets unsuccessfully received by PHYs
    std::vector<uint32_t> m_rxBytes; //!< count number of received bytes

    Time m_lastTxStart{0}; //!< hold the time at which the last transmission started
    Time m_lastTxEnd{0};   //!< hold the time at which the last transmission ended
};

SpectrumWifiPhyMultipleInterfacesTest::SpectrumWifiPhyMultipleInterfacesTest(
    bool trackSignalsInactiveInterfaces,
    ChannelSwitchScenario chanSwitchScenario)
    : TestCase{"SpectrumWifiPhy test operation with multiple RF interfaces"},
      m_trackSignalsInactiveInterfaces{trackSignalsInactiveInterfaces},
      m_chanSwitchScenario{chanSwitchScenario}
{
}

void
SpectrumWifiPhyMultipleInterfacesTest::SwitchChannel(Ptr<SpectrumWifiPhy> phy,
                                                     WifiPhyBand band,
                                                     uint8_t channelNumber,
                                                     MHz_u channelWidth,
                                                     std::optional<std::size_t> listenerIndex)
{
    NS_LOG_FUNCTION(this << phy << band << +channelNumber << channelWidth);
    if (listenerIndex)
    {
        auto& listener = m_listeners.at(*listenerIndex);
        listener->m_notifyMaybeCcaBusyStart = 0;
        listener->m_ccaBusyStart = Seconds(0);
        listener->m_ccaBusyEnd = Seconds(0);
    }
    phy->SetOperatingChannel(WifiPhy::ChannelTuple{channelNumber, channelWidth, band, 0});
    // verify rxing state of interference helper is reset after channel switch
    Simulator::ScheduleNow(&SpectrumWifiPhyMultipleInterfacesTest::CheckRxingState,
                           this,
                           phy,
                           false);
}

void
SpectrumWifiPhyMultipleInterfacesTest::SendPpdu(Ptr<SpectrumWifiPhy> phy,
                                                dBm_u txPower,
                                                uint32_t payloadSize)
{
    NS_LOG_FUNCTION(this << phy << txPower << payloadSize << phy->GetCurrentFrequencyRange()
                         << phy->GetChannelWidth() << phy->GetChannelNumber());

    WifiTxVector txVector{HePhy::GetHeMcs11(),
                          0,
                          WIFI_PREAMBLE_HE_SU,
                          NanoSeconds(800),
                          1,
                          1,
                          0,
                          MHz_u{20},
                          false,
                          false};
    Ptr<Packet> pkt = Create<Packet>(payloadSize);
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
    phy->SetTxPowerStart(txPower);
    phy->SetTxPowerEnd(txPower);
    phy->Send(WifiConstPsduMap({std::make_pair(SU_STA_ID, psdu)}), txVector);
}

void
SpectrumWifiPhyMultipleInterfacesTest::RxCallback(std::size_t index,
                                                  Ptr<const Packet> packet,
                                                  RxPowerWattPerChannelBand /*rxPowersW*/)
{
    auto phy = m_rxPhys.at(index);
    const auto payloadBytes = packet->GetSize() - 30;
    NS_LOG_FUNCTION(this << index << payloadBytes << phy->GetCurrentFrequencyRange()
                         << phy->GetChannelWidth() << phy->GetChannelNumber());
    m_counts.at(index)++;
    m_rxBytes.at(index) += payloadBytes;
}

void
SpectrumWifiPhyMultipleInterfacesTest::RxSuccess(std::size_t index,
                                                 Ptr<const WifiPsdu> psdu,
                                                 RxSignalInfo rxSignalInfo,
                                                 const WifiTxVector& txVector,
                                                 const std::vector<bool>& /*statusPerMpdu*/)
{
    NS_LOG_FUNCTION(this << index << *psdu << rxSignalInfo << txVector);
    m_countRxSuccess.at(index)++;
}

void
SpectrumWifiPhyMultipleInterfacesTest::RxFailure(std::size_t index, Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << index << *psdu);
    m_countRxFailure.at(index)++;
}

void
SpectrumWifiPhyMultipleInterfacesTest::CheckInterferences(Ptr<SpectrumWifiPhy> phy,
                                                          const FrequencyRange& freqRange,
                                                          const WifiSpectrumBandInfo& band,
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
                           band,
                           interferencesExpected);
}

void
SpectrumWifiPhyMultipleInterfacesTest::DoCheckInterferences(Ptr<SpectrumWifiPhy> phy,
                                                            const WifiSpectrumBandInfo& band,
                                                            bool interferencesExpected)
{
    NS_LOG_FUNCTION(this << phy << band << interferencesExpected);
    PointerValue ptr;
    phy->GetAttribute("InterferenceHelper", ptr);
    auto interferenceHelper = DynamicCast<InterferenceHelper>(ptr.Get<InterferenceHelper>());
    NS_ASSERT(interferenceHelper);
    const auto energyDuration = interferenceHelper->GetEnergyDuration(Watt_u{0}, band);
    NS_TEST_ASSERT_MSG_EQ(energyDuration.IsStrictlyPositive(),
                          interferencesExpected,
                          "Incorrect interferences detection");
}

void
SpectrumWifiPhyMultipleInterfacesTest::CheckResults(
    std::size_t index,
    uint32_t expectedNumRx,
    uint32_t expectedNumRxSuccess,
    uint32_t expectedRxBytes,
    FrequencyRange expectedFrequencyRangeActiveRfInterface,
    const std::vector<std::size_t>& expectedConnectedPhysPerChannel)
{
    NS_LOG_FUNCTION(this << index << expectedNumRx << expectedNumRxSuccess << expectedRxBytes
                         << expectedFrequencyRangeActiveRfInterface);
    const auto phy = m_rxPhys.at(index);
    std::size_t numActiveInterfaces = 0;
    for (const auto& [freqRange, interface] : phy->GetSpectrumPhyInterfaces())
    {
        const auto expectedActive = (freqRange == expectedFrequencyRangeActiveRfInterface);
        const auto isActive = (interface == phy->GetCurrentInterface());
        NS_TEST_ASSERT_MSG_EQ(isActive, expectedActive, "Incorrect active interface");
        if (isActive)
        {
            numActiveInterfaces++;
        }
    }
    NS_TEST_ASSERT_MSG_EQ(numActiveInterfaces, 1, "There should always be one active interface");
    NS_ASSERT(expectedConnectedPhysPerChannel.size() == m_spectrumChannels.size());
    for (std::size_t i = 0; i < m_spectrumChannels.size(); ++i)
    {
        NS_TEST_ASSERT_MSG_EQ(m_spectrumChannels.at(i)->GetNDevices(),
                              expectedConnectedPhysPerChannel.at(i),
                              "Incorrect number of PHYs attached to the spectrum channel");
    }
    NS_TEST_ASSERT_MSG_EQ(m_counts.at(index), expectedNumRx, "Unexpected amount of RX events");
    NS_TEST_ASSERT_MSG_EQ(m_countRxSuccess.at(index),
                          expectedNumRxSuccess,
                          "Unexpected amount of successfully received packets");
    NS_TEST_ASSERT_MSG_EQ(m_countRxFailure.at(index),
                          0,
                          "Unexpected amount of unsuccessfully received packets");
    NS_TEST_ASSERT_MSG_EQ(m_listeners.at(index)->m_notifyRxStart,
                          expectedNumRxSuccess,
                          "Unexpected amount of RX payload start indication");
}

void
SpectrumWifiPhyMultipleInterfacesTest::CheckCcaIndication(std::size_t index,
                                                          bool expectedCcaBusyIndication,
                                                          Time switchingDelay,
                                                          Time propagationDelay)
{
    const auto expectedCcaBusyStart =
        expectedCcaBusyIndication ? m_lastTxStart + switchingDelay : Seconds(0);
    const auto expectedCcaBusyEnd =
        expectedCcaBusyIndication ? m_lastTxEnd + propagationDelay : Seconds(0);
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
SpectrumWifiPhyMultipleInterfacesTest::CheckRxingState(Ptr<SpectrumWifiPhy> phy, bool rxingExpected)
{
    NS_LOG_FUNCTION(this << phy << rxingExpected);
    PointerValue ptr;
    phy->GetAttribute("InterferenceHelper", ptr);
    auto interferenceHelper = DynamicCast<ExtInterferenceHelper>(ptr.Get<ExtInterferenceHelper>());
    NS_ASSERT(interferenceHelper);
    NS_TEST_ASSERT_MSG_EQ(interferenceHelper->IsRxing(), rxingExpected, "Incorrect rxing state");
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
        SwitchChannel(m_rxPhys.at(rxPhyIndex),
                      txPhy->GetPhyBand(),
                      txPhy->GetChannelNumber(),
                      txPhy->GetChannelWidth(),
                      rxPhyIndex);
    }
    // reset counters
    for (auto& countRxSuccess : m_countRxSuccess)
    {
        countRxSuccess = 0;
    }
    for (auto& countRxFailure : m_countRxFailure)
    {
        countRxFailure = 0;
    }
    for (auto& rxBytes : m_rxBytes)
    {
        rxBytes = 0;
    }
}

void
SpectrumWifiPhyMultipleInterfacesTest::DoSetup()
{
    NS_LOG_FUNCTION(this);

    // WifiHelper::EnableLogComponents();
    // LogComponentEnable("SpectrumWifiPhyTest", LOG_LEVEL_ALL);

    NodeContainer wifiApNode(1);
    NodeContainer wifiStaNode(1);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211be);

    SpectrumWifiPhyHelper phyHelper(4);
    phyHelper.SetInterferenceHelper("ns3::ExtInterferenceHelper");
    phyHelper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

    struct SpectrumPhyInterfaceInfo
    {
        FrequencyRange range; ///< frequency range covered by the interface
        uint8_t number;       ///< channel number the interface operates on
        WifiPhyBand band;     ///< PHY band the interface operates on
        std::string bandName; ///< name of the PHY band the interface operates on
    };

    const FrequencyRange WIFI_SPECTRUM_5_GHZ_LOW{
        WIFI_SPECTRUM_5_GHZ.minFrequency,
        WIFI_SPECTRUM_5_GHZ.minFrequency +
            ((WIFI_SPECTRUM_5_GHZ.maxFrequency - WIFI_SPECTRUM_5_GHZ.minFrequency) / 2)};
    const FrequencyRange WIFI_SPECTRUM_5_GHZ_HIGH{
        WIFI_SPECTRUM_5_GHZ.minFrequency +
            ((WIFI_SPECTRUM_5_GHZ.maxFrequency - WIFI_SPECTRUM_5_GHZ.minFrequency) / 2),
        WIFI_SPECTRUM_5_GHZ.maxFrequency};

    const std::vector<SpectrumPhyInterfaceInfo> interfaces{
        {WIFI_SPECTRUM_2_4_GHZ, 2, WIFI_PHY_BAND_2_4GHZ, "BAND_2_4GHZ"},
        {WIFI_SPECTRUM_5_GHZ_LOW, 42, WIFI_PHY_BAND_5GHZ, "BAND_5GHZ"},
        {WIFI_SPECTRUM_5_GHZ_HIGH, 163, WIFI_PHY_BAND_5GHZ, "BAND_5GHZ"},
        {WIFI_SPECTRUM_6_GHZ, 215, WIFI_PHY_BAND_6GHZ, "BAND_6GHZ"}};

    for (std::size_t i = 0; i < interfaces.size(); ++i)
    {
        auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
        auto delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
        spectrumChannel->SetPropagationDelayModel(delayModel);
        std::ostringstream oss;
        oss << "{" << +interfaces.at(i).number << ", 0, " << interfaces.at(i).bandName << ", 0}";
        phyHelper.Set(i, "ChannelSettings", StringValue(oss.str()));
        phyHelper.AddChannel(spectrumChannel, interfaces.at(i).range);
        m_spectrumChannels.emplace_back(spectrumChannel);
    }

    WifiMacHelper mac;
    mac.SetType("ns3::ApWifiMac", "BeaconGeneration", BooleanValue(false));
    phyHelper.Set("TrackSignalsFromInactiveInterfaces", BooleanValue(false));
    auto apDevice = wifi.Install(phyHelper, mac, wifiApNode.Get(0));

    mac.SetType("ns3::StaWifiMac", "ActiveProbing", BooleanValue(false));
    phyHelper.Set("TrackSignalsFromInactiveInterfaces",
                  BooleanValue(m_trackSignalsInactiveInterfaces));
    auto staDevice = wifi.Install(phyHelper, mac, wifiStaNode.Get(0));

    MobilityHelper mobility;
    auto positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(10.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNode);

    for (std::size_t i = 0; i < interfaces.size(); ++i)
    {
        auto txPhy =
            DynamicCast<SpectrumWifiPhy>(DynamicCast<WifiNetDevice>(apDevice.Get(0))->GetPhy(i));
        if (m_chanSwitchScenario == ChannelSwitchScenario::BETWEEN_TX_RX)
        {
            txPhy->SetAttribute("ChannelSwitchDelay", TimeValue(NanoSeconds(1)));
        }
        m_txPhys.push_back(txPhy);

        const auto index = m_rxPhys.size();
        auto rxPhy =
            DynamicCast<SpectrumWifiPhy>(DynamicCast<WifiNetDevice>(staDevice.Get(0))->GetPhy(i));
        if (m_chanSwitchScenario == ChannelSwitchScenario::BETWEEN_TX_RX)
        {
            rxPhy->SetAttribute("ChannelSwitchDelay", TimeValue(NanoSeconds(1)));
        }
        rxPhy->TraceConnectWithoutContext(
            "PhyRxBegin",
            MakeCallback(&SpectrumWifiPhyMultipleInterfacesTest::RxCallback, this).Bind(index));

        rxPhy->SetReceiveOkCallback(
            MakeCallback(&SpectrumWifiPhyMultipleInterfacesTest::RxSuccess, this).Bind(index));
        rxPhy->SetReceiveErrorCallback(
            MakeCallback(&SpectrumWifiPhyMultipleInterfacesTest::RxFailure, this).Bind(index));

        auto listener = std::make_shared<TestPhyListener>();
        rxPhy->RegisterListener(listener);
        m_listeners.push_back(std::move(listener));

        m_rxPhys.push_back(rxPhy);
        m_counts.push_back(0);
        m_countRxSuccess.push_back(0);
        m_countRxFailure.push_back(0);
        m_rxBytes.push_back(0);
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

    const dBm_u ccaEdThreshold{-62.0}; ///< CCA-ED threshold
    const auto txAfterChannelSwitchDelay =
        MicroSeconds((m_chanSwitchScenario == ChannelSwitchScenario::BEFORE_TX)
                         ? 250
                         : 0); ///< delay in seconds between channel switch is triggered and a
                               ///< transmission gets started
    const auto checkResultsDelay =
        Seconds(0.5); ///< delay in seconds between start of test and moment results are verified
    const auto flushResultsDelay =
        Seconds(0.9); ///< delay in seconds between start of test and moment results are flushed
    const auto txOngoingAfterTxStartedDelay =
        MicroSeconds(50); ///< delay in microseconds between a transmission has started and a point
                          ///< in time the transmission is ongoing
    const auto propagationDelay = NanoSeconds(33); // propagation delay for the test scenario

    Time delay{0};

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
                            dBm_u{0},
                            1000);
        for (std::size_t j = 0; j < 4; ++j)
        {
            auto txPhy = m_txPhys.at(j);
            auto rxPhy = m_rxPhys.at(j);
            const auto& expectedFreqRange = txPhy->GetCurrentFrequencyRange();
            Simulator::Schedule(delay + txOngoingAfterTxStartedDelay,
                                &SpectrumWifiPhyMultipleInterfacesTest::CheckInterferences,
                                this,
                                rxPhy,
                                txPpduPhy->GetCurrentFrequencyRange(),
                                txPpduPhy->GetBand(txPpduPhy->GetChannelWidth(), 0),
                                true);
            Simulator::Schedule(delay + checkResultsDelay,
                                &SpectrumWifiPhyMultipleInterfacesTest::CheckResults,
                                this,
                                j,
                                (i == j) ? 1 : 0,
                                (i == j) ? 1 : 0,
                                (i == j) ? 1000 : 0,
                                expectedFreqRange,
                                expectedConnectedPhysPerChannel);
        }
        Simulator::Schedule(delay + flushResultsDelay,
                            &SpectrumWifiPhyMultipleInterfacesTest::Reset,
                            this);
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
                            dBm_u{0},
                            1000);
        const auto& expectedFreqRange = txPpduPhy->GetCurrentFrequencyRange();
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
                                rxPhy,
                                txPpduPhy->GetPhyBand(),
                                txPpduPhy->GetChannelNumber(),
                                txPpduPhy->GetChannelWidth(),
                                j);
            Simulator::Schedule(delay + txAfterChannelSwitchDelay + txOngoingAfterTxStartedDelay,
                                &SpectrumWifiPhyMultipleInterfacesTest::CheckInterferences,
                                this,
                                rxPhy,
                                txPpduPhy->GetCurrentFrequencyRange(),
                                txPpduPhy->GetBand(txPpduPhy->GetChannelWidth(), 0),
                                true);
            Simulator::Schedule(delay + checkResultsDelay,
                                &SpectrumWifiPhyMultipleInterfacesTest::CheckResults,
                                this,
                                j,
                                1,
                                1,
                                1000,
                                expectedFreqRange,
                                expectedConnectedPhysPerChannel);
        }
        Simulator::Schedule(delay + flushResultsDelay,
                            &SpectrumWifiPhyMultipleInterfacesTest::Reset,
                            this);
    }

    // Switch all PHYs to channel 36: all PHYs switch to the second spectrum channel
    // since second spectrum channel is 42 (80 MHz) and hence covers channel 36 (20 MHz)
    const auto secondSpectrumChannelIndex = 1;
    auto channel36TxPhy = m_txPhys.at(secondSpectrumChannelIndex);
    const auto& expectedFreqRange = channel36TxPhy->GetCurrentFrequencyRange();
    for (std::size_t i = 0; i < 4; ++i)
    {
        delay += Seconds(1);
        auto txPpduPhy = m_txPhys.at(i);
        Simulator::Schedule(delay + txAfterChannelSwitchDelay,
                            &SpectrumWifiPhyMultipleInterfacesTest::SendPpdu,
                            this,
                            txPpduPhy,
                            dBm_u{0},
                            1000);
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
                                m_rxPhys.at(j),
                                WIFI_PHY_BAND_5GHZ,
                                CHANNEL_NUMBER,
                                CHANNEL_WIDTH,
                                j);
            Simulator::Schedule(delay + checkResultsDelay,
                                &SpectrumWifiPhyMultipleInterfacesTest::CheckResults,
                                this,
                                j,
                                (i == secondSpectrumChannelIndex) ? 1 : 0,
                                (i == secondSpectrumChannelIndex) ? 1 : 0,
                                (i == secondSpectrumChannelIndex) ? 1000 : 0,
                                expectedFreqRange,
                                expectedConnectedPhysPerChannel);
        }
        Simulator::Schedule(delay + flushResultsDelay,
                            &SpectrumWifiPhyMultipleInterfacesTest::Reset,
                            this);
    }

    // verify CCA indication when switching to a channel with an ongoing transmission
    for (const auto txPower : {dBm_u{-60} /* above CCA-ED */, dBm_u{-70} /* below CCA-ED */})
    {
        for (std::size_t i = 0; i < 4; ++i)
        {
            for (std::size_t j = 0; j < 4; ++j)
            {
                auto txPpduPhy = m_txPhys.at(i);
                const auto startChannel = WifiPhyOperatingChannel::FindFirst(
                    txPpduPhy->GetPrimaryChannelNumber(MHz_u{20}),
                    MHz_u{0},
                    MHz_u{20},
                    WIFI_STANDARD_80211ax,
                    txPpduPhy->GetPhyBand());
                for (auto bw = txPpduPhy->GetChannelWidth(); bw >= MHz_u{20}; bw /= 2)
                {
                    const auto& channelInfo =
                        (*WifiPhyOperatingChannel::FindFirst(0,
                                                             MHz_u{0},
                                                             bw,
                                                             WIFI_STANDARD_80211ax,
                                                             txPpduPhy->GetPhyBand(),
                                                             startChannel));
                    delay += Seconds(1);
                    Simulator::Schedule(delay,
                                        &SpectrumWifiPhyMultipleInterfacesTest::SendPpdu,
                                        this,
                                        txPpduPhy,
                                        txPower,
                                        1000);
                    Simulator::Schedule(delay + txOngoingAfterTxStartedDelay,
                                        &SpectrumWifiPhyMultipleInterfacesTest::SwitchChannel,
                                        this,
                                        m_rxPhys.at(j),
                                        channelInfo.band,
                                        channelInfo.number,
                                        channelInfo.width,
                                        j);
                    for (std::size_t k = 0; k < 4; ++k)
                    {
                        if ((i != j) && (k == i))
                        {
                            continue;
                        }
                        const auto expectCcaBusyIndication =
                            (k == i) ? (txPower >= ccaEdThreshold)
                                     : (m_trackSignalsInactiveInterfaces
                                            ? ((txPower >= ccaEdThreshold) ? (j == k) : false)
                                            : false);
                        Simulator::Schedule(
                            delay + checkResultsDelay,
                            &SpectrumWifiPhyMultipleInterfacesTest::CheckCcaIndication,
                            this,
                            k,
                            expectCcaBusyIndication,
                            txOngoingAfterTxStartedDelay,
                            propagationDelay);
                    }
                    Simulator::Schedule(delay + flushResultsDelay,
                                        &SpectrumWifiPhyMultipleInterfacesTest::Reset,
                                        this);
                }
            }
        }
    }

    if (m_trackSignalsInactiveInterfaces)
    {
        /* Reproduce an EMLSR scenario where a PHY is on an initial band and receives a packet.
         * Then, the PHY switches to another band where it starts receiving another packet.
         * During reception of the PHY header, the PHY switches back to the initial band and starts
         * receiving yet another packet. In this case, first and last packets should be successfully
         * received (no interference), the second packet reception has been interrupted (before the
         * payload reception does start, hence it does not reach the RX state). */
        {
            // first TX on initial band
            auto txPpduPhy = m_txPhys.at(0);
            delay += Seconds(1);
            Simulator::Schedule(delay,
                                &SpectrumWifiPhyMultipleInterfacesTest::SendPpdu,
                                this,
                                txPpduPhy,
                                dBm_u{20},
                                500);

            // switch channel to other band
            delay += Seconds(1);
            txPpduPhy = m_txPhys.at(1);
            Simulator::Schedule(delay,
                                &SpectrumWifiPhyMultipleInterfacesTest::SwitchChannel,
                                this,
                                m_rxPhys.at(0),
                                txPpduPhy->GetPhyBand(),
                                txPpduPhy->GetChannelNumber(),
                                txPpduPhy->GetChannelWidth(),
                                0);

            // TX on other band
            delay += Seconds(1);
            Simulator::Schedule(delay,
                                &SpectrumWifiPhyMultipleInterfacesTest::SendPpdu,
                                this,
                                txPpduPhy,
                                dBm_u{0},
                                1000);

            // switch back to initial band during PHY header reception
            txPpduPhy = m_txPhys.at(0);
            delay += MicroSeconds(20); // right after legacy PHY header reception
            Simulator::Schedule(delay,
                                &SpectrumWifiPhyMultipleInterfacesTest::SwitchChannel,
                                this,
                                m_rxPhys.at(0),
                                txPpduPhy->GetPhyBand(),
                                txPpduPhy->GetChannelNumber(),
                                txPpduPhy->GetChannelWidth(),
                                0);

            // TX once more on the initial band
            delay += Seconds(1);
            Simulator::Schedule(delay,
                                &SpectrumWifiPhyMultipleInterfacesTest::SendPpdu,
                                this,
                                txPpduPhy,
                                dBm_u{0},
                                1500);

            // check results
            Simulator::Schedule(
                delay + checkResultsDelay,
                &SpectrumWifiPhyMultipleInterfacesTest::CheckResults,
                this,
                0,
                3,    // 3 RX events
                2,    // 2 packets should have been successfully received, 1 packet should
                      // have been interrupted (switch during PHY header reception)
                2000, // 500 bytes (firstpacket) and 1500 bytes (third packet)
                txPpduPhy->GetCurrentFrequencyRange(),
                expectedConnectedPhysPerChannel);

            // reset
            Simulator::Schedule(delay + flushResultsDelay,
                                &SpectrumWifiPhyMultipleInterfacesTest::Reset,
                                this);
        }

        /* Reproduce an EMLSR scenario where a PHY is on an initial band and receives a packet
         * but switches to another band during preamble detection period. Then, it starts receiving
         * two packets which interfere with each other. Afterwards, the PHY goes back to its initial
         * band and starts receiving yet another packet. In this case, only the last packet should
         * be successfully received (no interference). */
        {
            // switch channel of PHY index 0 to 5 GHz low band (operating channel of TX PHY index 1)
            auto txPpduPhy = m_txPhys.at(1);
            delay += Seconds(1);
            Simulator::Schedule(delay,
                                &SpectrumWifiPhyMultipleInterfacesTest::SwitchChannel,
                                this,
                                m_rxPhys.at(0),
                                txPpduPhy->GetPhyBand(),
                                txPpduPhy->GetChannelNumber(),
                                txPpduPhy->GetChannelWidth(),
                                0);

            // start transmission on 5 GHz low band
            delay += Seconds(1);
            Simulator::Schedule(delay,
                                &SpectrumWifiPhyMultipleInterfacesTest::SendPpdu,
                                this,
                                txPpduPhy,
                                dBm_u{20},
                                500);

            // switch channel back to previous channel before preamble detection is finished:
            // this is needed to verify interference helper rxing state is properly reset
            // since ongoing reception is aborted when switching operating channel
            Simulator::Schedule(delay + MicroSeconds(2),
                                &SpectrumWifiPhyMultipleInterfacesTest::SwitchChannel,
                                this,
                                m_rxPhys.at(0),
                                m_txPhys.at(0)->GetPhyBand(),
                                m_txPhys.at(0)->GetChannelNumber(),
                                m_txPhys.at(0)->GetChannelWidth(),
                                0);

            delay += Seconds(1);
            // we need 2 TX PHYs on the 5 GHz low band to have simultaneous transmissions
            // switch operating channel of TX PHY index 2 to the 5 GHz low band
            Simulator::Schedule(delay,
                                &SpectrumWifiPhyMultipleInterfacesTest::SwitchChannel,
                                this,
                                m_txPhys.at(2),
                                txPpduPhy->GetPhyBand(),
                                txPpduPhy->GetChannelNumber(),
                                txPpduPhy->GetChannelWidth(),
                                std::nullopt);

            // first transmission on 5 GHz low band with high power
            delay += Seconds(1);
            Simulator::Schedule(delay,
                                &SpectrumWifiPhyMultipleInterfacesTest::SendPpdu,
                                this,
                                txPpduPhy,
                                dBm_u{20},
                                1000);

            // second transmission on 5 GHz low band  with high power a bit later:
            // first powers get updated updated in the corresponding bands
            txPpduPhy = m_txPhys.at(2);
            Simulator::Schedule(delay + NanoSeconds(10),
                                &SpectrumWifiPhyMultipleInterfacesTest::SendPpdu,
                                this,
                                txPpduPhy,
                                dBm_u{20},
                                1000);

            // restore channel for TX PHY index 2
            delay += Seconds(1);
            Simulator::Schedule(delay,
                                &SpectrumWifiPhyMultipleInterfacesTest::SwitchChannel,
                                this,
                                m_txPhys.at(2),
                                m_rxPhys.at(2)->GetPhyBand(),
                                m_rxPhys.at(2)->GetChannelNumber(),
                                m_rxPhys.at(2)->GetChannelWidth(),
                                std::nullopt);

            // switch channel of PHY index 0 to 5 GHz low band again
            delay += Seconds(1);
            txPpduPhy = m_txPhys.at(1);
            Simulator::Schedule(delay,
                                &SpectrumWifiPhyMultipleInterfacesTest::SwitchChannel,
                                this,
                                m_rxPhys.at(0),
                                txPpduPhy->GetPhyBand(),
                                txPpduPhy->GetChannelNumber(),
                                txPpduPhy->GetChannelWidth(),
                                0);

            // transmit PPDU on 5 GHz low band (no interference)
            delay += Seconds(1);
            Simulator::Schedule(delay,
                                &SpectrumWifiPhyMultipleInterfacesTest::SendPpdu,
                                this,
                                txPpduPhy,
                                dBm_u{0},
                                1500);

            // check results
            Simulator::Schedule(delay + checkResultsDelay,
                                &SpectrumWifiPhyMultipleInterfacesTest::CheckResults,
                                this,
                                0,
                                1, // 1 RX event
                                1, // last transmitted packet should have been successfully received
                                1500, // 1500 bytes (payload of last transmitted packet)
                                txPpduPhy->GetCurrentFrequencyRange(),
                                expectedConnectedPhysPerChannel);

            // reset
            Simulator::Schedule(delay + flushResultsDelay,
                                &SpectrumWifiPhyMultipleInterfacesTest::Reset,
                                this);
        }
    }

    delay += Seconds(1);
    Simulator::Stop(delay);
    Simulator::Run();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Spectrum Wifi Phy Interfaces Helper Test
 *
 * This test checks the expected interfaces are added to the spectrum PHY instances
 * created by the helper.
 */
class SpectrumWifiPhyInterfacesHelperTest : public TestCase
{
  public:
    SpectrumWifiPhyInterfacesHelperTest();
    ~SpectrumWifiPhyInterfacesHelperTest() override = default;

  private:
    void DoRun() override;
};

SpectrumWifiPhyInterfacesHelperTest::SpectrumWifiPhyInterfacesHelperTest()
    : TestCase("Check PHY interfaces added to PHY instances using helper")
{
}

void
SpectrumWifiPhyInterfacesHelperTest::DoRun()
{
    WifiHelper wifiHelper;
    wifiHelper.SetStandard(WIFI_STANDARD_80211be);

    SpectrumWifiPhyHelper phyHelper(3);
    phyHelper.Set(0, "ChannelSettings", StringValue("{2, 0, BAND_2_4GHZ, 0}"));
    phyHelper.Set(1, "ChannelSettings", StringValue("{36, 0, BAND_5GHZ, 0}"));
    phyHelper.Set(2, "ChannelSettings", StringValue("{1, 0, BAND_6GHZ, 0}"));

    phyHelper.AddChannel(CreateObject<MultiModelSpectrumChannel>(), WIFI_SPECTRUM_2_4_GHZ);
    phyHelper.AddChannel(CreateObject<MultiModelSpectrumChannel>(), WIFI_SPECTRUM_5_GHZ);
    phyHelper.AddChannel(CreateObject<MultiModelSpectrumChannel>(), WIFI_SPECTRUM_6_GHZ);

    WifiMacHelper macHelper;
    NodeContainer nodes(4);

    /* Default case: all interfaces are added to each link */
    auto device = wifiHelper.Install(phyHelper, macHelper, nodes.Get(0));

    // Verify each PHY has 3 interfaces
    auto phyLink0 =
        DynamicCast<SpectrumWifiPhy>(DynamicCast<WifiNetDevice>(device.Get(0))->GetPhy(0));
    NS_ASSERT(phyLink0);
    NS_TEST_ASSERT_MSG_EQ(phyLink0->GetSpectrumPhyInterfaces().size(),
                          3,
                          "Incorrect number of PHY interfaces added to PHY link ID 0");

    auto phyLink1 =
        DynamicCast<SpectrumWifiPhy>(DynamicCast<WifiNetDevice>(device.Get(0))->GetPhy(1));
    NS_ASSERT(phyLink1);
    NS_TEST_ASSERT_MSG_EQ(phyLink1->GetSpectrumPhyInterfaces().size(),
                          3,
                          "Incorrect number of PHY interfaces added to PHY link ID 1");

    auto phyLink2 =
        DynamicCast<SpectrumWifiPhy>(DynamicCast<WifiNetDevice>(device.Get(0))->GetPhy(2));
    NS_ASSERT(phyLink2);
    NS_TEST_ASSERT_MSG_EQ(phyLink1->GetSpectrumPhyInterfaces().size(),
                          3,
                          "Incorrect number of PHY interfaces added to PHY link ID 2");

    /* each PHY has a single interface */
    phyHelper.AddPhyToFreqRangeMapping(0, WIFI_SPECTRUM_2_4_GHZ);
    phyHelper.AddPhyToFreqRangeMapping(1, WIFI_SPECTRUM_5_GHZ);
    phyHelper.AddPhyToFreqRangeMapping(2, WIFI_SPECTRUM_6_GHZ);
    device = wifiHelper.Install(phyHelper, macHelper, nodes.Get(1));

    // Verify each PHY has a single interface
    phyLink0 = DynamicCast<SpectrumWifiPhy>(DynamicCast<WifiNetDevice>(device.Get(0))->GetPhy(0));
    NS_ASSERT(phyLink0);
    NS_TEST_ASSERT_MSG_EQ(phyLink0->GetSpectrumPhyInterfaces().size(),
                          1,
                          "Incorrect number of PHY interfaces added to PHY link ID 0");
    NS_TEST_ASSERT_MSG_EQ(phyLink0->GetSpectrumPhyInterfaces().count(WIFI_SPECTRUM_2_4_GHZ),
                          1,
                          "Incorrect PHY interfaces added to PHY link ID 0");

    phyLink1 = DynamicCast<SpectrumWifiPhy>(DynamicCast<WifiNetDevice>(device.Get(0))->GetPhy(1));
    NS_ASSERT(phyLink1);
    NS_TEST_ASSERT_MSG_EQ(phyLink1->GetSpectrumPhyInterfaces().size(),
                          1,
                          "Incorrect number of PHY interfaces added to PHY link ID 1");
    NS_TEST_ASSERT_MSG_EQ(phyLink1->GetSpectrumPhyInterfaces().count(WIFI_SPECTRUM_5_GHZ),
                          1,
                          "Incorrect PHY interfaces added to PHY link ID 1");

    phyLink2 = DynamicCast<SpectrumWifiPhy>(DynamicCast<WifiNetDevice>(device.Get(0))->GetPhy(2));
    NS_ASSERT(phyLink2);
    NS_TEST_ASSERT_MSG_EQ(phyLink2->GetSpectrumPhyInterfaces().size(),
                          1,
                          "Incorrect number of PHY interfaces added to PHY link ID 2");
    NS_TEST_ASSERT_MSG_EQ(phyLink2->GetSpectrumPhyInterfaces().count(WIFI_SPECTRUM_6_GHZ),
                          1,
                          "Incorrect PHY interfaces added to PHY link ID 2");

    /* add yet another interface to PHY 0 */
    phyHelper.AddPhyToFreqRangeMapping(0, WIFI_SPECTRUM_5_GHZ);
    device = wifiHelper.Install(phyHelper, macHelper, nodes.Get(2));

    // Verify each PHY has a single interface except PHY 0 that should have 2 interfaces
    phyLink0 = DynamicCast<SpectrumWifiPhy>(DynamicCast<WifiNetDevice>(device.Get(0))->GetPhy(0));
    NS_ASSERT(phyLink0);
    NS_TEST_ASSERT_MSG_EQ(phyLink0->GetSpectrumPhyInterfaces().size(),
                          2,
                          "Incorrect number of PHY interfaces added to PHY link ID 0");
    NS_TEST_ASSERT_MSG_EQ(phyLink0->GetSpectrumPhyInterfaces().count(WIFI_SPECTRUM_2_4_GHZ),
                          1,
                          "Incorrect PHY interfaces added to PHY link ID 0");
    NS_TEST_ASSERT_MSG_EQ(phyLink0->GetSpectrumPhyInterfaces().count(WIFI_SPECTRUM_5_GHZ),
                          1,
                          "Incorrect PHY interfaces added to PHY link ID 0");

    phyLink1 = DynamicCast<SpectrumWifiPhy>(DynamicCast<WifiNetDevice>(device.Get(0))->GetPhy(1));
    NS_ASSERT(phyLink1);
    NS_TEST_ASSERT_MSG_EQ(phyLink1->GetSpectrumPhyInterfaces().size(),
                          1,
                          "Incorrect number of PHY interfaces added to PHY link ID 1");
    NS_TEST_ASSERT_MSG_EQ(phyLink1->GetSpectrumPhyInterfaces().count(WIFI_SPECTRUM_5_GHZ),
                          1,
                          "Incorrect PHY interfaces added to PHY link ID 1");

    phyLink2 = DynamicCast<SpectrumWifiPhy>(DynamicCast<WifiNetDevice>(device.Get(0))->GetPhy(2));
    NS_ASSERT(phyLink2);
    NS_TEST_ASSERT_MSG_EQ(phyLink2->GetSpectrumPhyInterfaces().size(),
                          1,
                          "Incorrect number of PHY interfaces added to PHY link ID 2");
    NS_TEST_ASSERT_MSG_EQ(phyLink2->GetSpectrumPhyInterfaces().count(WIFI_SPECTRUM_6_GHZ),
                          1,
                          "Incorrect PHY interfaces added to PHY link ID 2");

    /* reset mapping previously configured to helper: back to default */
    phyHelper.ResetPhyToFreqRangeMapping();
    device = wifiHelper.Install(phyHelper, macHelper, nodes.Get(3));

    // Verify each PHY has 3 interfaces
    phyLink0 = DynamicCast<SpectrumWifiPhy>(DynamicCast<WifiNetDevice>(device.Get(0))->GetPhy(0));
    NS_ASSERT(phyLink0);
    NS_TEST_ASSERT_MSG_EQ(phyLink0->GetSpectrumPhyInterfaces().size(),
                          3,
                          "Incorrect number of PHY interfaces added to PHY link ID 0");
    NS_TEST_ASSERT_MSG_EQ(phyLink0->GetSpectrumPhyInterfaces().count(WIFI_SPECTRUM_2_4_GHZ),
                          1,
                          "Incorrect PHY interfaces added to PHY link ID 0");
    NS_TEST_ASSERT_MSG_EQ(phyLink0->GetSpectrumPhyInterfaces().count(WIFI_SPECTRUM_5_GHZ),
                          1,
                          "Incorrect PHY interfaces added to PHY link ID 0");
    NS_TEST_ASSERT_MSG_EQ(phyLink0->GetSpectrumPhyInterfaces().count(WIFI_SPECTRUM_6_GHZ),
                          1,
                          "Incorrect PHY interfaces added to PHY link ID 0");

    phyLink1 = DynamicCast<SpectrumWifiPhy>(DynamicCast<WifiNetDevice>(device.Get(0))->GetPhy(1));
    NS_ASSERT(phyLink1);
    NS_TEST_ASSERT_MSG_EQ(phyLink1->GetSpectrumPhyInterfaces().size(),
                          3,
                          "Incorrect number of PHY interfaces added to PHY link ID 1");
    NS_TEST_ASSERT_MSG_EQ(phyLink1->GetSpectrumPhyInterfaces().count(WIFI_SPECTRUM_2_4_GHZ),
                          1,
                          "Incorrect PHY interfaces added to PHY link ID 0");
    NS_TEST_ASSERT_MSG_EQ(phyLink1->GetSpectrumPhyInterfaces().count(WIFI_SPECTRUM_5_GHZ),
                          1,
                          "Incorrect PHY interfaces added to PHY link ID 0");
    NS_TEST_ASSERT_MSG_EQ(phyLink1->GetSpectrumPhyInterfaces().count(WIFI_SPECTRUM_6_GHZ),
                          1,
                          "Incorrect PHY interfaces added to PHY link ID 0");

    phyLink2 = DynamicCast<SpectrumWifiPhy>(DynamicCast<WifiNetDevice>(device.Get(0))->GetPhy(2));
    NS_ASSERT(phyLink2);
    NS_TEST_ASSERT_MSG_EQ(phyLink2->GetSpectrumPhyInterfaces().size(),
                          3,
                          "Incorrect number of PHY interfaces added to PHY link ID 2");
    NS_TEST_ASSERT_MSG_EQ(phyLink2->GetSpectrumPhyInterfaces().count(WIFI_SPECTRUM_2_4_GHZ),
                          1,
                          "Incorrect PHY interfaces added to PHY link ID 0");
    NS_TEST_ASSERT_MSG_EQ(phyLink2->GetSpectrumPhyInterfaces().count(WIFI_SPECTRUM_5_GHZ),
                          1,
                          "Incorrect PHY interfaces added to PHY link ID 0");
    NS_TEST_ASSERT_MSG_EQ(phyLink2->GetSpectrumPhyInterfaces().count(WIFI_SPECTRUM_6_GHZ),
                          1,
                          "Incorrect PHY interfaces added to PHY link ID 0");

    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Spectrum Wifi Phy Test Suite
 */
class SpectrumWifiPhyTestSuite : public TestSuite
{
  public:
    SpectrumWifiPhyTestSuite();
};

SpectrumWifiPhyTestSuite::SpectrumWifiPhyTestSuite()
    : TestSuite("wifi-spectrum-phy", Type::UNIT)
{
    AddTestCase(new SpectrumWifiPhyBasicTest, TestCase::Duration::QUICK);
    AddTestCase(new SpectrumWifiPhyListenerTest, TestCase::Duration::QUICK);
    AddTestCase(new SpectrumWifiPhyFilterTest, TestCase::Duration::QUICK);
    AddTestCase(new SpectrumWifiPhyGetBandTest, TestCase::Duration::QUICK);
    AddTestCase(new SpectrumWifiPhyTrackedBandsTest, TestCase::Duration::QUICK);
    AddTestCase(new SpectrumWifiPhy80Plus80Test, TestCase::Duration::QUICK);
    AddTestCase(new SpectrumWifiPhyMultipleInterfacesTest(
                    false,
                    SpectrumWifiPhyMultipleInterfacesTest::ChannelSwitchScenario::BEFORE_TX),
                TestCase::Duration::QUICK);
    AddTestCase(new SpectrumWifiPhyMultipleInterfacesTest(
                    true,
                    SpectrumWifiPhyMultipleInterfacesTest::ChannelSwitchScenario::BEFORE_TX),
                TestCase::Duration::QUICK);
    AddTestCase(new SpectrumWifiPhyMultipleInterfacesTest(
                    true,
                    SpectrumWifiPhyMultipleInterfacesTest::ChannelSwitchScenario::BETWEEN_TX_RX),
                TestCase::Duration::QUICK);
    AddTestCase(new SpectrumWifiPhyInterfacesHelperTest, TestCase::Duration::QUICK);
}

static SpectrumWifiPhyTestSuite spectrumWifiPhyTestSuite; ///< the test suite
