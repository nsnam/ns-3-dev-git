/*
 * Copyright (c) 2018
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/interference-helper.h"
#include "ns3/log.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/ofdm-phy.h"
#include "ns3/ofdm-ppdu.h"
#include "ns3/spectrum-phy.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/test.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-spectrum-phy-interface.h"
#include "ns3/wifi-spectrum-signal-parameters.h"
#include "ns3/wifi-spectrum-value-helper.h"
#include "ns3/wifi-utils.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiPhyThresholdsTest");

static const uint8_t CHANNEL_NUMBER = 36;
static const MHz_u FREQUENCY{5180};
static const MHz_u CHANNEL_WIDTH{20};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Wifi Phy Threshold Test base class
 */
class WifiPhyThresholdsTest : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param test_name the test name
     */
    WifiPhyThresholdsTest(std::string test_name);

  protected:
    /**
     * Make wifi signal function
     * @param txPower the transmit power
     * @param channel the operating channel of the PHY used for the transmission
     * @returns Ptr<SpectrumSignalParameters>
     */
    virtual Ptr<SpectrumSignalParameters> MakeWifiSignal(Watt_u txPower,
                                                         const WifiPhyOperatingChannel& channel);
    /**
     * Make foreign signal function
     * @param txPower the transmit power
     * @returns Ptr<SpectrumSignalParameters>
     */
    virtual Ptr<SpectrumSignalParameters> MakeForeignSignal(Watt_u txPower);
    /**
     * Send signal function
     * @param txPower the transmit power
     * @param wifiSignal whether the signal is a wifi signal or not
     */
    virtual void SendSignal(Watt_u txPower, bool wifiSignal);
    /**
     * PHY receive success callback function
     * @param psdu the PSDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector the transmit vector
     * @param statusPerMpdu reception status per MPDU
     */
    virtual void RxSuccess(Ptr<const WifiPsdu> psdu,
                           RxSignalInfo rxSignalInfo,
                           const WifiTxVector& txVector,
                           const std::vector<bool>& statusPerMpdu);
    /**
     * PHY receive failure callback function
     * @param psdu the PSDU
     */
    virtual void RxFailure(Ptr<const WifiPsdu> psdu);
    /**
     * PHY dropped packet callback function
     * @param p the packet
     * @param reason the reason
     */
    void RxDropped(Ptr<const Packet> p, WifiPhyRxfailureReason reason);
    /**
     * PHY state changed callback function
     * @param start the start time of the new state
     * @param duration the duration of the new state
     * @param newState the new state
     */
    virtual void PhyStateChanged(Time start, Time duration, WifiPhyState newState);

    Ptr<SpectrumWifiPhy> m_phy;   ///< PHY object
    uint32_t m_rxSuccess;         ///< count number of successfully received packets
    uint32_t m_rxFailure;         ///< count number of unsuccessfuly received packets
    uint32_t m_rxDropped;         ///< count number of dropped packets
    uint32_t m_stateChanged;      ///< count number of PHY state change
    uint32_t m_rxStateCount;      ///< count number of PHY state change to RX state
    uint32_t m_idleStateCount;    ///< count number of PHY state change to IDLE state
    uint32_t m_ccabusyStateCount; ///< count number of PHY state change to CCA_BUSY state

  private:
    void DoSetup() override;
    void DoTeardown() override;
};

WifiPhyThresholdsTest::WifiPhyThresholdsTest(std::string test_name)
    : TestCase(test_name),
      m_rxSuccess(0),
      m_rxFailure(0),
      m_rxDropped(0),
      m_stateChanged(0),
      m_rxStateCount(0),
      m_idleStateCount(0),
      m_ccabusyStateCount(0)
{
}

Ptr<SpectrumSignalParameters>
WifiPhyThresholdsTest::MakeWifiSignal(Watt_u txPower, const WifiPhyOperatingChannel& channel)
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

    auto ppdu = Create<OfdmPpdu>(psdu, txVector, channel, 0);

    auto txPowerSpectrum = WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity(
        channel.GetPrimaryChannelCenterFrequency(CHANNEL_WIDTH),
        CHANNEL_WIDTH,
        txPower,
        CHANNEL_WIDTH);
    auto txParams = Create<WifiSpectrumSignalParameters>();
    txParams->psd = txPowerSpectrum;
    txParams->txPhy = nullptr;
    txParams->duration = txDuration;
    txParams->ppdu = ppdu;
    return txParams;
}

Ptr<SpectrumSignalParameters>
WifiPhyThresholdsTest::MakeForeignSignal(Watt_u txPower)
{
    auto txPowerSpectrum =
        WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity(FREQUENCY,
                                                                    CHANNEL_WIDTH,
                                                                    txPower,
                                                                    CHANNEL_WIDTH);
    auto txParams = Create<SpectrumSignalParameters>();
    txParams->psd = txPowerSpectrum;
    txParams->txPhy = nullptr;
    txParams->duration = Seconds(0.5);
    return txParams;
}

void
WifiPhyThresholdsTest::SendSignal(Watt_u txPower, bool wifiSignal)
{
    if (wifiSignal)
    {
        m_phy->StartRx(MakeWifiSignal(txPower, m_phy->GetOperatingChannel()), nullptr);
    }
    else
    {
        m_phy->StartRx(MakeForeignSignal(txPower), nullptr);
    }
}

void
WifiPhyThresholdsTest::RxSuccess(Ptr<const WifiPsdu> psdu,
                                 RxSignalInfo rxSignalInfo,
                                 const WifiTxVector& txVector,
                                 const std::vector<bool>& statusPerMpdu)
{
    NS_LOG_FUNCTION(this << *psdu << rxSignalInfo << txVector);
    m_rxSuccess++;
}

void
WifiPhyThresholdsTest::RxFailure(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << *psdu);
    m_rxFailure++;
}

void
WifiPhyThresholdsTest::RxDropped(Ptr<const Packet> p, WifiPhyRxfailureReason reason)
{
    NS_LOG_FUNCTION(this << p << reason);
    m_rxDropped++;
}

void
WifiPhyThresholdsTest::PhyStateChanged(Time start, Time duration, WifiPhyState newState)
{
    NS_LOG_FUNCTION(this << start << duration << newState);
    m_stateChanged++;
    if (newState == WifiPhyState::IDLE)
    {
        m_idleStateCount++;
    }
    else if (newState == WifiPhyState::RX)
    {
        m_rxStateCount++;
    }
    else if (newState == WifiPhyState::CCA_BUSY)
    {
        m_ccabusyStateCount++;
    }
}

void
WifiPhyThresholdsTest::DoSetup()
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
    m_phy->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phy->SetReceiveOkCallback(MakeCallback(&WifiPhyThresholdsTest::RxSuccess, this));
    m_phy->SetReceiveErrorCallback(MakeCallback(&WifiPhyThresholdsTest::RxFailure, this));
    m_phy->TraceConnectWithoutContext("PhyRxDrop",
                                      MakeCallback(&WifiPhyThresholdsTest::RxDropped, this));
    m_phy->GetState()->TraceConnectWithoutContext(
        "State",
        MakeCallback(&WifiPhyThresholdsTest::PhyStateChanged, this));
    dev->SetPhy(m_phy);
    node->AddDevice(dev);
}

void
WifiPhyThresholdsTest::DoTeardown()
{
    m_phy->Dispose();
    m_phy = nullptr;
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Wifi Phy Threshold Weak Wifi Signal Test
 *
 * This test makes sure PHY ignores a Wi-Fi signal
 * if its received power lower than RxSensitivity.
 */
class WifiPhyThresholdsWeakWifiSignalTest : public WifiPhyThresholdsTest
{
  public:
    WifiPhyThresholdsWeakWifiSignalTest();
    void DoRun() override;
};

WifiPhyThresholdsWeakWifiSignalTest::WifiPhyThresholdsWeakWifiSignalTest()
    : WifiPhyThresholdsTest("WifiPhy reception thresholds: test weak wifi signal reception")
{
}

void
WifiPhyThresholdsWeakWifiSignalTest::DoRun()
{
    const auto txPower = DbmToW(dBm_u{-110});

    Simulator::Schedule(Seconds(1),
                        &WifiPhyThresholdsWeakWifiSignalTest::SendSignal,
                        this,
                        txPower,
                        true);

    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ(m_rxDropped + m_rxSuccess + m_rxFailure,
                          0,
                          "Reception should not have been triggered if packet is weaker than "
                          "RxSensitivity threshold");
    NS_TEST_ASSERT_MSG_EQ(m_stateChanged,
                          0,
                          "State should stay idle if reception involves a signal weaker than "
                          "RxSensitivity threshold");
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Wifi Phy Threshold Weak Foreign Signal Test
 *
 * This test makes sure PHY keeps the state as IDLE if reception involves
 * a foreign signal with a received power lower than CcaEdThreshold.
 */
class WifiPhyThresholdsWeakForeignSignalTest : public WifiPhyThresholdsTest
{
  public:
    WifiPhyThresholdsWeakForeignSignalTest();
    ~WifiPhyThresholdsWeakForeignSignalTest() override;
    void DoRun() override;
};

WifiPhyThresholdsWeakForeignSignalTest::WifiPhyThresholdsWeakForeignSignalTest()
    : WifiPhyThresholdsTest("WifiPhy reception thresholds: test weak foreign signal reception")
{
}

WifiPhyThresholdsWeakForeignSignalTest::~WifiPhyThresholdsWeakForeignSignalTest()
{
}

void
WifiPhyThresholdsWeakForeignSignalTest::DoRun()
{
    const auto txPower = DbmToW(dBm_u{-90});

    Simulator::Schedule(Seconds(1),
                        &WifiPhyThresholdsWeakForeignSignalTest::SendSignal,
                        this,
                        txPower,
                        false);

    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ(m_rxDropped + m_rxSuccess + m_rxFailure,
                          0,
                          "Reception of non-wifi packet should not be triggered");
    NS_TEST_ASSERT_MSG_EQ(m_stateChanged,
                          0,
                          "State should stay idle if reception involves a signal weaker than "
                          "RxSensitivity threshold");
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Wifi Phy Threshold Strong Wifi Signal Test
 *
 * This test makes sure PHY processes a Wi-Fi signal
 * with a received power higher than RxSensitivity.
 */
class WifiPhyThresholdsStrongWifiSignalTest : public WifiPhyThresholdsTest
{
  public:
    WifiPhyThresholdsStrongWifiSignalTest();
    ~WifiPhyThresholdsStrongWifiSignalTest() override;
    void DoRun() override;
};

WifiPhyThresholdsStrongWifiSignalTest::WifiPhyThresholdsStrongWifiSignalTest()
    : WifiPhyThresholdsTest("WifiPhy reception thresholds: test strong wifi signal reception")
{
}

WifiPhyThresholdsStrongWifiSignalTest::~WifiPhyThresholdsStrongWifiSignalTest()
{
}

void
WifiPhyThresholdsStrongWifiSignalTest::DoRun()
{
    const auto txPower = DbmToW(dBm_u{-60});

    Simulator::Schedule(Seconds(1),
                        &WifiPhyThresholdsStrongWifiSignalTest::SendSignal,
                        this,
                        txPower,
                        true);

    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ(m_rxDropped + m_rxFailure,
                          0,
                          "Packet reception should have been successful");
    NS_TEST_ASSERT_MSG_EQ(m_rxSuccess, 1, "Packet should have been successfully received");
    NS_TEST_ASSERT_MSG_EQ(m_ccabusyStateCount, 2, "State should have moved to CCA_BUSY once");
    NS_TEST_ASSERT_MSG_EQ(
        m_stateChanged,
        4,
        "State should have moved to CCA_BUSY, then to RX and finally back to IDLE");
    NS_TEST_ASSERT_MSG_EQ(m_rxStateCount, 1, "State should have moved to RX once");
    NS_TEST_ASSERT_MSG_EQ(m_idleStateCount, 1, "State should have moved to IDLE once");
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Wifi Phy Threshold Strong Foreign Signal Test
 *
 * This test makes sure PHY declare the state as CCA_BUSY if reception involves
 * a foreign signal with a received power higher than CcaEdThreshold.
 */
class WifiPhyThresholdsStrongForeignSignalTest : public WifiPhyThresholdsTest
{
  public:
    WifiPhyThresholdsStrongForeignSignalTest();
    ~WifiPhyThresholdsStrongForeignSignalTest() override;
    void DoRun() override;
};

WifiPhyThresholdsStrongForeignSignalTest::WifiPhyThresholdsStrongForeignSignalTest()
    : WifiPhyThresholdsTest("WifiPhy reception thresholds: test weak foreign signal reception")
{
}

WifiPhyThresholdsStrongForeignSignalTest::~WifiPhyThresholdsStrongForeignSignalTest()
{
}

void
WifiPhyThresholdsStrongForeignSignalTest::DoRun()
{
    const auto txPower = DbmToW(dBm_u{-60});

    Simulator::Schedule(Seconds(1),
                        &WifiPhyThresholdsStrongForeignSignalTest::SendSignal,
                        this,
                        txPower,
                        false);

    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ(m_rxDropped + m_rxSuccess + m_rxFailure,
                          0,
                          "Reception of non-wifi packet should not be triggered");
    NS_TEST_ASSERT_MSG_EQ(m_idleStateCount,
                          1,
                          "State should have moved to CCA-BUSY then back to IDLE");
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Wifi Phy Thresholds Test Suite
 */
class WifiPhyThresholdsTestSuite : public TestSuite
{
  public:
    WifiPhyThresholdsTestSuite();
};

WifiPhyThresholdsTestSuite::WifiPhyThresholdsTestSuite()
    : TestSuite("wifi-phy-thresholds", Type::UNIT)
{
    AddTestCase(new WifiPhyThresholdsWeakWifiSignalTest, TestCase::Duration::QUICK);
    AddTestCase(new WifiPhyThresholdsWeakForeignSignalTest, TestCase::Duration::QUICK);
    AddTestCase(new WifiPhyThresholdsStrongWifiSignalTest, TestCase::Duration::QUICK);
    AddTestCase(new WifiPhyThresholdsStrongForeignSignalTest, TestCase::Duration::QUICK);
}

static WifiPhyThresholdsTestSuite wifiPhyThresholdsTestSuite; ///< the test suite
