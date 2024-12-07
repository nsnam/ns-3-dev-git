/*
 * Copyright (c) 2022
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/constant-obss-pd-algorithm.h"
#include "ns3/he-phy.h"
#include "ns3/he-ppdu.h"
#include "ns3/ht-ppdu.h"
#include "ns3/interference-helper.h"
#include "ns3/log.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/non-communicating-net-device.h"
#include "ns3/ofdm-ppdu.h"
#include "ns3/pointer.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/test.h"
#include "ns3/threshold-preamble-detection-model.h"
#include "ns3/vht-configuration.h"
#include "ns3/vht-ppdu.h"
#include "ns3/waveform-generator.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy-listener.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-spectrum-value-helper.h"
#include "ns3/wifi-utils.h"

#include <memory>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiPhyCcaTest");

constexpr MHz_u P20_CENTER_FREQUENCY{5180};
constexpr MHz_u S20_CENTER_FREQUENCY = P20_CENTER_FREQUENCY + MHz_u{20};
constexpr MHz_u P40_CENTER_FREQUENCY = P20_CENTER_FREQUENCY + MHz_u{10};
constexpr MHz_u S40_CENTER_FREQUENCY = P40_CENTER_FREQUENCY + MHz_u{40};
constexpr MHz_u P80_CENTER_FREQUENCY = P40_CENTER_FREQUENCY + MHz_u{20};
constexpr MHz_u S80_CENTER_FREQUENCY = P80_CENTER_FREQUENCY + MHz_u{80};
constexpr MHz_u P160_CENTER_FREQUENCY = P80_CENTER_FREQUENCY + MHz_u{40};
const Time smallDelta = NanoSeconds(1);
// add small delta to be right after aCCATime, since test checks are scheduled before wifi events
const Time aCcaTime = MicroSeconds(4) + smallDelta;
const std::map<MHz_u, Time> PpduDurations = {
    {20, NanoSeconds(1009600)},
    {40, NanoSeconds(533600)},
    {80, NanoSeconds(275200)},
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief PHY CCA thresholds test
 */
class WifiPhyCcaThresholdsTest : public TestCase
{
  public:
    WifiPhyCcaThresholdsTest();

  private:
    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     * Run tests for given CCA attributes
     */
    void RunOne();

    /**
     * Create a dummy PSDU whose payload is 1000 bytes
     * @return a dummy PSDU whose payload is 1000 bytes
     */
    Ptr<WifiPsdu> CreateDummyPsdu();
    /**
     * Create a non-HT PPDU
     * @param channel the operating channel of the PHY used for the transmission
     * @return a non-HT PPDU
     */
    Ptr<OfdmPpdu> CreateDummyNonHtPpdu(const WifiPhyOperatingChannel& channel);
    /**
     * Create a HT PPDU
     * @param bandwidth the bandwidth used for the transmission the PPDU
     * @param channel the operating channel of the PHY used for the transmission
     * @return a HT PPDU
     */
    Ptr<HtPpdu> CreateDummyHtPpdu(MHz_u bandwidth, const WifiPhyOperatingChannel& channel);
    /**
     * Create a VHT PPDU
     * @param bandwidth the bandwidth used for the transmission the PPDU
     * @param channel the operating channel of the PHY used for the transmission
     * @return a VHT PPDU
     */
    Ptr<VhtPpdu> CreateDummyVhtPpdu(MHz_u bandwidth, const WifiPhyOperatingChannel& channel);
    /**
     * Create a HE PPDU
     * @param bandwidth the bandwidth used for the transmission the PPDU
     * @param channel the operating channel of the PHY used for the transmission
     * @return a HE PPDU
     */
    Ptr<HePpdu> CreateDummyHePpdu(MHz_u bandwidth, const WifiPhyOperatingChannel& channel);

    /**
     * Function to verify the CCA threshold that is being reported by a given PHY entity upon
     * reception of a signal or a PPDU
     * @param phy the PHY entity to verify
     * @param ppdu the incoming PPDU or signal (if nullptr)
     * @param channelType the channel list type that indicates which channel the PPDU or the
     * signal occupies
     * @param expectedCcaThreshold the CCA threshold that is expected to be reported
     */
    void VerifyCcaThreshold(const Ptr<PhyEntity> phy,
                            const Ptr<const WifiPpdu> ppdu,
                            WifiChannelListType channelType,
                            dBm_u expectedCcaThreshold);

    Ptr<WifiNetDevice> m_device;              ///< The WifiNetDevice
    Ptr<SpectrumWifiPhy> m_phy;               ///< The spectrum PHY
    Ptr<ObssPdAlgorithm> m_obssPdAlgorithm;   ///< The OBSS-PD algorithm
    Ptr<VhtConfiguration> m_vhtConfiguration; ///< The VHT configuration

    dBm_u m_CcaEdThreshold; ///< The current CCA-ED threshold for a 20 MHz subchannel
    dBm_u m_CcaSensitivity; ///< The current CCA sensitivity threshold for signals that occupy the
                            ///< primary 20 MHz channel

    VhtConfiguration::SecondaryCcaSensitivityThresholds
        m_secondaryCcaSensitivityThresholds; ///< The current CCA sensitivity thresholds for signals
                                             ///< that do not occupy the primary 20 MHz channel

    dBm_u m_obssPdLevel; ///< The current OBSS-PD level
};

WifiPhyCcaThresholdsTest::WifiPhyCcaThresholdsTest()
    : TestCase("Wi-Fi PHY CCA thresholds test"),
      m_CcaEdThreshold{-62.0},
      m_CcaSensitivity{-82.0},
      m_secondaryCcaSensitivityThresholds{dBm_u{-72}, dBm_u{-72}, dBm_u{-69}},
      m_obssPdLevel{-82.0}
{
}

Ptr<WifiPsdu>
WifiPhyCcaThresholdsTest::CreateDummyPsdu()
{
    Ptr<Packet> pkt = Create<Packet>(1000);
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_QOSDATA);
    hdr.SetQosTid(0);
    return Create<WifiPsdu>(pkt, hdr);
}

Ptr<OfdmPpdu>
WifiPhyCcaThresholdsTest::CreateDummyNonHtPpdu(const WifiPhyOperatingChannel& channel)
{
    WifiTxVector txVector = WifiTxVector(OfdmPhy::GetOfdmRate6Mbps(),
                                         0,
                                         WIFI_PREAMBLE_LONG,
                                         NanoSeconds(800),
                                         1,
                                         1,
                                         0,
                                         MHz_u{20},
                                         false);
    Ptr<WifiPsdu> psdu = CreateDummyPsdu();
    return Create<OfdmPpdu>(psdu, txVector, channel, 0);
}

Ptr<HtPpdu>
WifiPhyCcaThresholdsTest::CreateDummyHtPpdu(MHz_u bandwidth, const WifiPhyOperatingChannel& channel)
{
    WifiTxVector txVector = WifiTxVector(HtPhy::GetHtMcs0(),
                                         0,
                                         WIFI_PREAMBLE_HT_MF,
                                         NanoSeconds(800),
                                         1,
                                         1,
                                         0,
                                         bandwidth,
                                         false);
    Ptr<WifiPsdu> psdu = CreateDummyPsdu();
    return Create<HtPpdu>(psdu, txVector, channel, MicroSeconds(100), 0);
}

Ptr<VhtPpdu>
WifiPhyCcaThresholdsTest::CreateDummyVhtPpdu(MHz_u bandwidth,
                                             const WifiPhyOperatingChannel& channel)
{
    WifiTxVector txVector = WifiTxVector(VhtPhy::GetVhtMcs0(),
                                         0,
                                         WIFI_PREAMBLE_VHT_SU,
                                         NanoSeconds(800),
                                         1,
                                         1,
                                         0,
                                         bandwidth,
                                         false);
    Ptr<WifiPsdu> psdu = CreateDummyPsdu();
    return Create<VhtPpdu>(psdu, txVector, channel, MicroSeconds(100), 0);
}

Ptr<HePpdu>
WifiPhyCcaThresholdsTest::CreateDummyHePpdu(MHz_u bandwidth, const WifiPhyOperatingChannel& channel)
{
    WifiTxVector txVector = WifiTxVector(HePhy::GetHeMcs0(),
                                         0,
                                         WIFI_PREAMBLE_HE_SU,
                                         NanoSeconds(800),
                                         1,
                                         1,
                                         0,
                                         bandwidth,
                                         false);
    Ptr<WifiPsdu> psdu = CreateDummyPsdu();
    return Create<HePpdu>(psdu, txVector, channel, MicroSeconds(100), 0);
}

void
WifiPhyCcaThresholdsTest::VerifyCcaThreshold(const Ptr<PhyEntity> phy,
                                             const Ptr<const WifiPpdu> ppdu,
                                             WifiChannelListType channelType,
                                             dBm_u expectedCcaThreshold)
{
    NS_LOG_FUNCTION(this << phy << channelType << expectedCcaThreshold);
    const auto actualThreshold = phy->GetCcaThreshold(ppdu, channelType);
    NS_LOG_INFO((ppdu == nullptr ? "any signal" : "a PPDU")
                << " in " << channelType << " channel: " << actualThreshold << "dBm");
    NS_TEST_EXPECT_MSG_EQ_TOL(actualThreshold,
                              expectedCcaThreshold,
                              dB_u{1e-6},
                              "Actual CCA threshold for "
                                  << (ppdu == nullptr ? "any signal" : "a PPDU") << " in "
                                  << channelType << " channel " << actualThreshold
                                  << "dBm does not match expected threshold "
                                  << expectedCcaThreshold << "dBm");
}

void
WifiPhyCcaThresholdsTest::DoSetup()
{
    // WifiHelper::EnableLogComponents ();
    // LogComponentEnable ("WifiPhyCcaTest", LOG_LEVEL_ALL);

    m_device = CreateObject<WifiNetDevice>();
    m_device->SetStandard(WIFI_STANDARD_80211ax);
    m_vhtConfiguration = CreateObject<VhtConfiguration>();
    m_device->SetVhtConfiguration(m_vhtConfiguration);

    m_phy = CreateObject<SpectrumWifiPhy>();
    m_phy->SetDevice(m_device);
    m_device->SetPhy(m_phy);
    m_phy->SetInterferenceHelper(CreateObject<InterferenceHelper>());
    m_phy->AddChannel(CreateObject<MultiModelSpectrumChannel>());

    auto channelNum = WifiPhyOperatingChannel::FindFirst(0,
                                                         MHz_u{0},
                                                         MHz_u{160},
                                                         WIFI_STANDARD_80211ax,
                                                         WIFI_PHY_BAND_5GHZ)
                          ->number;
    m_phy->SetOperatingChannel(WifiPhy::ChannelTuple{channelNum, 160, WIFI_PHY_BAND_5GHZ, 0});
    m_phy->ConfigureStandard(WIFI_STANDARD_80211ax);

    m_obssPdAlgorithm = CreateObject<ConstantObssPdAlgorithm>();
    m_device->AggregateObject(m_obssPdAlgorithm);
    m_obssPdAlgorithm->ConnectWifiNetDevice(m_device);
}

void
WifiPhyCcaThresholdsTest::DoTeardown()
{
    m_device->Dispose();
    m_device = nullptr;
}

void
WifiPhyCcaThresholdsTest::RunOne()
{
    m_phy->SetCcaEdThreshold(m_CcaEdThreshold);
    m_phy->SetCcaSensitivityThreshold(m_CcaSensitivity);
    m_vhtConfiguration->SetSecondaryCcaSensitivityThresholds(m_secondaryCcaSensitivityThresholds);
    m_obssPdAlgorithm->SetObssPdLevel(m_obssPdLevel);

    // OFDM PHY: any signal in primary channel (20 MHz) if power above CCA-ED threshold
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_OFDM),
                       nullptr,
                       WIFI_CHANLIST_PRIMARY,
                       m_CcaEdThreshold);

    //-----------------------------------------------------------------------------------------------------------------------------------

    // OFDM PHY: 20 MHz non-HT PPDU in primary channel (20 MHz) if power above CCA sensitivity
    // threshold
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_OFDM),
                       CreateDummyNonHtPpdu(m_phy->GetOperatingChannel()),
                       WIFI_CHANLIST_PRIMARY,
                       m_CcaSensitivity);

    //-----------------------------------------------------------------------------------------------------------------------------------

    // HT PHY: any signal in primary channel (20 MHz) if power above CCA-ED threshold
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_HT),
                       nullptr,
                       WIFI_CHANLIST_PRIMARY,
                       m_CcaEdThreshold);

    // HT PHY: any signal in primary channel (20 MHz) if power above CCA-ED threshold
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_HT),
                       nullptr,
                       WIFI_CHANLIST_SECONDARY,
                       m_CcaEdThreshold);

    //-----------------------------------------------------------------------------------------------------------------------------------

    // HT PHY: 20 MHz HT PPDU in primary channel (20 MHz) if power in primary above CCA sensitivity
    // threshold
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_HT),
                       CreateDummyHtPpdu(MHz_u{20}, m_phy->GetOperatingChannel()),
                       WIFI_CHANLIST_PRIMARY,
                       m_CcaSensitivity);

    // HT PHY: 40 MHz HT PPDU in primary channel (20 MHz) if power in primary above CCA sensitivity
    // threshold
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_HT),
                       CreateDummyHtPpdu(MHz_u{40}, m_phy->GetOperatingChannel()),
                       WIFI_CHANLIST_PRIMARY,
                       m_CcaSensitivity);

    //-----------------------------------------------------------------------------------------------------------------------------------

    // VHT PHY: any signal in primary channel (20 MHz) if power above CCA-ED threshold
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_VHT),
                       nullptr,
                       WIFI_CHANLIST_PRIMARY,
                       m_CcaEdThreshold);

    // VHT PHY: any signal in secondary channel (20 MHz) if power above CCA-ED threshold
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_VHT),
                       nullptr,
                       WIFI_CHANLIST_SECONDARY,
                       m_CcaEdThreshold);

    // VHT PHY: any signal in secondary40 channel (40 MHz) if power above CCA-ED threshold + 3dB
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_VHT),
                       nullptr,
                       WIFI_CHANLIST_SECONDARY40,
                       m_CcaEdThreshold + dB_u{3.0});

    // VHT PHY: any signal in secondary80 channel (80 MHz) if power above CCA-ED threshold + 6dB
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_VHT),
                       nullptr,
                       WIFI_CHANLIST_SECONDARY80,
                       m_CcaEdThreshold + dB_u{6.0});

    //-----------------------------------------------------------------------------------------------------------------------------------

    // VHT PHY: 20 MHz VHT PPDU in primary channel (20 MHz) if power in primary above CCA
    // sensitivity threshold
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_VHT),
                       CreateDummyVhtPpdu(MHz_u{20}, m_phy->GetOperatingChannel()),
                       WIFI_CHANLIST_PRIMARY,
                       m_CcaSensitivity);

    // VHT PHY: 40 MHz VHT PPDU in primary channel (20 MHz) if power in primary above CCA
    // sensitivity threshold
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_VHT),
                       CreateDummyVhtPpdu(MHz_u{40}, m_phy->GetOperatingChannel()),
                       WIFI_CHANLIST_PRIMARY,
                       m_CcaSensitivity);

    // VHT PHY: 80 MHz VHT PPDU in primary channel (20 MHz) if power in primary above CCA
    // sensitivity threshold
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_VHT),
                       CreateDummyVhtPpdu(MHz_u{80}, m_phy->GetOperatingChannel()),
                       WIFI_CHANLIST_PRIMARY,
                       m_CcaSensitivity);

    // VHT PHY: 160 MHz VHT PPDU in primary channel (20 MHz) if power in primary above CCA
    // sensitivity threshold
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_VHT),
                       CreateDummyVhtPpdu(MHz_u{160}, m_phy->GetOperatingChannel()),
                       WIFI_CHANLIST_PRIMARY,
                       m_CcaSensitivity);

    //-----------------------------------------------------------------------------------------------------------------------------------

    // VHT PHY: 20 MHz VHT PPDU in secondary channel (20 MHz) if power above the CCA sensitivity
    // threshold corresponding to a 20 MHz PPDU that does not occupy the primary 20 MHz
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_VHT),
                       CreateDummyVhtPpdu(MHz_u{20}, m_phy->GetOperatingChannel()),
                       WIFI_CHANLIST_SECONDARY,
                       std::get<0>(m_secondaryCcaSensitivityThresholds));

    // VHT PHY: 20 MHz VHT PPDU in secondary40 channel (40 MHz) if power above the CCA sensitivity
    // threshold corresponding to a 20 MHz PPDU that does not occupy the primary 20 MHz
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_VHT),
                       CreateDummyVhtPpdu(MHz_u{20}, m_phy->GetOperatingChannel()),
                       WIFI_CHANLIST_SECONDARY40,
                       std::get<0>(m_secondaryCcaSensitivityThresholds));

    // VHT PHY: 40 MHz VHT PPDU in secondary40 channel (40 MHz) if power above the CCA sensitivity
    // threshold corresponding to a 40 MHz PPDU that does not occupy the primary 20 MHz
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_VHT),
                       CreateDummyVhtPpdu(MHz_u{40}, m_phy->GetOperatingChannel()),
                       WIFI_CHANLIST_SECONDARY40,
                       std::get<1>(m_secondaryCcaSensitivityThresholds));

    // VHT PHY: 20 MHz VHT PPDU in secondary80 channel (80 MHz) if power above the CCA sensitivity
    // threshold corresponding to a 20 MHz PPDU that does not occupy the primary 20 MHz
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_VHT),
                       CreateDummyVhtPpdu(MHz_u{20}, m_phy->GetOperatingChannel()),
                       WIFI_CHANLIST_SECONDARY80,
                       std::get<0>(m_secondaryCcaSensitivityThresholds));

    // VHT PHY: 40 MHz VHT PPDU in secondary80 channel (80 MHz) if power above the CCA sensitivity
    // threshold corresponding to a 40 MHz PPDU that does not occupy the primary 20 MHz
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_VHT),
                       CreateDummyVhtPpdu(MHz_u{40}, m_phy->GetOperatingChannel()),
                       WIFI_CHANLIST_SECONDARY80,
                       std::get<1>(m_secondaryCcaSensitivityThresholds));

    // VHT PHY: 80 MHz VHT PPDU in secondary80 channel (80 MHz) if power above the CCA sensitivity
    // threshold corresponding to a 80 MHz PPDU that does not occupy the primary 20 MHz
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_VHT),
                       CreateDummyVhtPpdu(MHz_u{80}, m_phy->GetOperatingChannel()),
                       WIFI_CHANLIST_SECONDARY80,
                       std::get<2>(m_secondaryCcaSensitivityThresholds));

    //-----------------------------------------------------------------------------------------------------------------------------------

    // HE PHY: any signal in primary channel (20 MHz) if power above CCA-ED threshold
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_HE),
                       nullptr,
                       WIFI_CHANLIST_PRIMARY,
                       m_CcaEdThreshold);

    // HE PHY: any signal in secondary channel (20 MHz) if power above CCA-ED threshold
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_HE),
                       nullptr,
                       WIFI_CHANLIST_SECONDARY,
                       m_CcaEdThreshold);

    // HE PHY: any signal in secondary40 channel (40 MHz) if power above CCA-ED threshold + 3dB
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_HE),
                       nullptr,
                       WIFI_CHANLIST_SECONDARY40,
                       m_CcaEdThreshold + dB_u{3.0});

    // HE PHY: any signal in secondary80 channel (80 MHz) if power above CCA-ED threshold + 6dB
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_HE),
                       nullptr,
                       WIFI_CHANLIST_SECONDARY80,
                       m_CcaEdThreshold + dB_u{6.0});

    //-----------------------------------------------------------------------------------------------------------------------------------

    // HE PHY: 20 MHz HE PPDU in primary channel (20 MHz) if power in primary above CCA sensitivity
    // threshold
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_HE),
                       CreateDummyHePpdu(MHz_u{20}, m_phy->GetOperatingChannel()),
                       WIFI_CHANLIST_PRIMARY,
                       m_CcaSensitivity);

    // HE PHY: 40 MHz HE PPDU in primary channel (20 MHz) if power in primary above CCA sensitivity
    // threshold
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_HE),
                       CreateDummyHePpdu(MHz_u{40}, m_phy->GetOperatingChannel()),
                       WIFI_CHANLIST_PRIMARY,
                       m_CcaSensitivity);

    // HE PHY: 80 MHz HE PPDU in primary channel (20 MHz) if power in primary above CCA sensitivity
    // threshold
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_HE),
                       CreateDummyHePpdu(MHz_u{80}, m_phy->GetOperatingChannel()),
                       WIFI_CHANLIST_PRIMARY,
                       m_CcaSensitivity);

    // HE PHY: 160 MHz HE PPDU in primary channel (20 MHz) if power in primary above CCA sensitivity
    // threshold
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_HE),
                       CreateDummyHePpdu(MHz_u{160}, m_phy->GetOperatingChannel()),
                       WIFI_CHANLIST_PRIMARY,
                       m_CcaSensitivity);

    //-----------------------------------------------------------------------------------------------------------------------------------

    // HE PHY: 20 MHz HE PPDU in secondary channel (20 MHz) if power above the max between the CCA
    // sensitivity threshold corresponding to a 20 MHz PPDU that does not occupy the primary 20 MHz
    // and the OBSS-PD level
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_HE),
                       CreateDummyHePpdu(MHz_u{20}, m_phy->GetOperatingChannel()),
                       WIFI_CHANLIST_SECONDARY,
                       std::max(m_obssPdLevel, std::get<0>(m_secondaryCcaSensitivityThresholds)));

    // HE PHY: 20 MHz HE PPDU in secondary40 channel (40 MHz) if power above the max between the CCA
    // sensitivity threshold corresponding to a 20 MHz PPDU that does not occupy the primary 20 MHz
    // and the OBSS-PD level
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_HE),
                       CreateDummyHePpdu(MHz_u{20}, m_phy->GetOperatingChannel()),
                       WIFI_CHANLIST_SECONDARY40,
                       std::max(m_obssPdLevel, std::get<0>(m_secondaryCcaSensitivityThresholds)));

    // HE PHY: 40 MHz HE PPDU in secondary40 channel (40 MHz) if power above the max between the CCA
    // sensitivity threshold corresponding to a 40 MHz PPDU that does not occupy the primary 20 MHz
    // and the OBSS-PD level plus 3 dB
    VerifyCcaThreshold(
        m_phy->GetPhyEntity(WIFI_MOD_CLASS_HE),
        CreateDummyHePpdu(MHz_u{40}, m_phy->GetOperatingChannel()),
        WIFI_CHANLIST_SECONDARY40,
        std::max(m_obssPdLevel + dB_u{3.0}, std::get<1>(m_secondaryCcaSensitivityThresholds)));

    // HE PHY: 20 MHz HE PPDU in secondary80 channel (80 MHz) if power above the max between the CCA
    // sensitivity threshold corresponding to a 20 MHz PPDU that does not occupy the primary 20 MHz
    // and the OBSS-PD level
    VerifyCcaThreshold(m_phy->GetPhyEntity(WIFI_MOD_CLASS_HE),
                       CreateDummyHePpdu(MHz_u{20}, m_phy->GetOperatingChannel()),
                       WIFI_CHANLIST_SECONDARY80,
                       std::max(m_obssPdLevel, std::get<0>(m_secondaryCcaSensitivityThresholds)));

    // HE PHY: 40 MHz HE PPDU in secondary80 channel (80 MHz) if power above the max between the CCA
    // sensitivity threshold corresponding to a 40 MHz PPDU that does not occupy the primary 20 MHz
    // and the OBSS-PD level plus 3 dB
    VerifyCcaThreshold(
        m_phy->GetPhyEntity(WIFI_MOD_CLASS_HE),
        CreateDummyHePpdu(MHz_u{40}, m_phy->GetOperatingChannel()),
        WIFI_CHANLIST_SECONDARY80,
        std::max(m_obssPdLevel + dB_u{3.0}, std::get<1>(m_secondaryCcaSensitivityThresholds)));

    // HE PHY: 80 MHz HE PPDU in secondary80 channel (80 MHz) if power above the max between the CCA
    // sensitivity threshold corresponding to a 80 MHz PPDU that does not occupy the primary 20 MHz
    // and the OBSS-PD level plus 6 dB
    VerifyCcaThreshold(
        m_phy->GetPhyEntity(WIFI_MOD_CLASS_HE),
        CreateDummyHePpdu(MHz_u{80}, m_phy->GetOperatingChannel()),
        WIFI_CHANLIST_SECONDARY80,
        std::max(m_obssPdLevel + dB_u{6.0}, std::get<2>(m_secondaryCcaSensitivityThresholds)));
}

void
WifiPhyCcaThresholdsTest::DoRun()
{
    // default attributes
    m_CcaEdThreshold = dBm_u{-62};
    m_CcaSensitivity = dBm_u{-82};
    m_secondaryCcaSensitivityThresholds = std::make_tuple(dBm_u{-72}, dBm_u{-72}, dBm_u{-69});
    m_obssPdLevel = dBm_u{-82};
    RunOne();

    // default attributes with OBSS-PD level set to -80 dBm
    m_CcaEdThreshold = dBm_u{-62};
    m_CcaSensitivity = dBm_u{-82};
    m_secondaryCcaSensitivityThresholds = std::make_tuple(dBm_u{-72}, dBm_u{-72}, dBm_u{-69});
    m_obssPdLevel = dBm_u{-80};
    RunOne();

    // default attributes with OBSS-PD level set to -70 dBm
    m_CcaEdThreshold = dBm_u{-62};
    m_CcaSensitivity = dBm_u{-82};
    m_secondaryCcaSensitivityThresholds = std::make_tuple(dBm_u{-72}, dBm_u{-72}, dBm_u{-69});
    m_obssPdLevel = dBm_u{-70};
    RunOne();

    // CCA-ED set to -65 dBm
    m_CcaEdThreshold = dBm_u{-65};
    m_CcaSensitivity = dBm_u{-82};
    m_secondaryCcaSensitivityThresholds = std::make_tuple(dBm_u{-72}, dBm_u{-72}, dBm_u{-69});
    m_obssPdLevel = dBm_u{-82};
    RunOne();

    // CCA sensitivity for signals in primary set to -75 dBm
    m_CcaEdThreshold = dBm_u{-62};
    m_CcaSensitivity = dBm_u{-75};
    m_secondaryCcaSensitivityThresholds = std::make_tuple(dBm_u{-72}, dBm_u{-72}, dBm_u{-69});
    m_obssPdLevel = dBm_u{-82};
    RunOne();

    // custom CCA sensitivities for signals not in primary
    m_CcaEdThreshold = dBm_u{-62};
    m_CcaSensitivity = dBm_u{-72};
    m_secondaryCcaSensitivityThresholds = std::make_tuple(dBm_u{-70}, dBm_u{-70}, dBm_u{-70});
    m_obssPdLevel = dBm_u{-82};
    RunOne();

    // custom CCA sensitivities for signals not in primary with OBSS-PD level set to -80 dBm
    m_CcaEdThreshold = dBm_u{-62};
    m_CcaSensitivity = dBm_u{-72};
    m_secondaryCcaSensitivityThresholds = std::make_tuple(dBm_u{-70}, dBm_u{-70}, dBm_u{-70});
    m_obssPdLevel = dBm_u{-80};
    RunOne();

    // custom CCA sensitivities for signals not in primary with OBSS-PD level set to -70 dBm
    m_CcaEdThreshold = dBm_u{-62};
    m_CcaSensitivity = dBm_u{-72};
    m_secondaryCcaSensitivityThresholds = std::make_tuple(dBm_u{-70}, dBm_u{-70}, dBm_u{-70});
    m_obssPdLevel = dBm_u{-70};
    RunOne();

    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief PHY listener for CCA tests
 */
class CcaTestPhyListener : public ns3::WifiPhyListener
{
  public:
    CcaTestPhyListener() = default;

    void NotifyRxStart(Time duration) override
    {
        NS_LOG_FUNCTION(this << duration);
    }

    void NotifyRxEndOk() override
    {
        NS_LOG_FUNCTION(this);
    }

    void NotifyRxEndError() override
    {
        NS_LOG_FUNCTION(this);
    }

    void NotifyTxStart(Time duration, dBm_u txPower) override
    {
        NS_LOG_FUNCTION(this << duration << txPower);
    }

    void NotifyCcaBusyStart(Time duration,
                            WifiChannelListType channelType,
                            const std::vector<Time>& per20MhzDurations) override
    {
        NS_LOG_FUNCTION(this << duration << channelType << per20MhzDurations.size());
        m_endCcaBusy = Simulator::Now() + duration;
        m_lastCcaBusyChannelType = channelType;
        m_lastPer20MhzCcaBusyDurations = per20MhzDurations;
        m_notifications++;
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
        m_notifications = 0;
        m_endCcaBusy = Seconds(0);
        m_lastCcaBusyChannelType = WIFI_CHANLIST_PRIMARY;
        m_lastPer20MhzCcaBusyDurations.clear();
    }

    std::size_t m_notifications{0}; ///< Number of CCA notifications
    Time m_endCcaBusy{Seconds(0)};  ///< End of the CCA-BUSY duration
    WifiChannelListType m_lastCcaBusyChannelType{
        WIFI_CHANLIST_PRIMARY}; ///< Channel type indication for the last CCA-BUSY notification
    std::vector<Time>
        m_lastPer20MhzCcaBusyDurations{}; ///< End of the CCA-BUSY durations per 20 MHz
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Wifi Phy Threshold Test base class
 */
class WifiPhyCcaIndicationTest : public TestCase
{
  public:
    WifiPhyCcaIndicationTest();

  private:
    void DoSetup() override;
    void DoRun() override;
    void DoTeardown() override;

    /**
     * Send an HE SU PPDU
     * @param txPower the transmit power
     * @param frequency the center frequency the transmitter is operating on
     * @param bandwidth the bandwidth to use for the transmission
     */
    void SendHeSuPpdu(dBm_u txPower, MHz_u frequency, MHz_u bandwidth);

    /**
     * Start to generate a signal
     * @param signalGenerator the signal generator to use
     * @param txPower the transmit power
     * @param frequency the center frequency of the signal to send
     * @param bandwidth the bandwidth of the signal to send
     * @param duration the duration of the signal
     */
    void StartSignal(Ptr<WaveformGenerator> signalGenerator,
                     dBm_u txPower,
                     MHz_u frequency,
                     MHz_u bandwidth,
                     Time duration);
    /**
     * Stop to generate a signal
     * @param signalGenerator the signal generator to use
     */
    void StopSignal(Ptr<WaveformGenerator> signalGenerator);

    /**
     * Check the PHY state
     * @param expectedState the expected state of the PHY
     */
    void CheckPhyState(WifiPhyState expectedState);
    /// @copydoc CheckPhyState
    void DoCheckPhyState(WifiPhyState expectedState);

    /**
     * Check the last CCA-BUSY notification
     * @param expectedEndTime the expected CCA-BUSY end time
     * @param expectedChannelType the expected channel type
     * @param expectedPer20MhzDurations the expected per-20 MHz CCA-BUSY durations
     */
    void CheckLastCcaBusyNotification(Time expectedEndTime,
                                      WifiChannelListType expectedChannelType,
                                      const std::vector<Time>& expectedPer20MhzDurations);

    /**
     * Log scenario description
     *
     * @param log the scenario description to add to log
     */
    void LogScenario(const std::string& log) const;

    /**
     * structure that holds information to generate signals
     */
    struct TxSignalInfo
    {
        dBm_u power{0.0};           //!< transmit power to use
        Time startTime{Seconds(0)}; //!< time at which transmission will be started
        Time duration{Seconds(0)};  //!< the duration of the transmission
        MHz_u centerFreq{0};        //!< center frequency to use
        MHz_u bandwidth{0};         //!< bandwidth to use
    };

    /**
     * structure that holds information to generate PPDUs
     */
    struct TxPpduInfo
    {
        dBm_u power{0.0};           //!< transmit power to use
        Time startTime{Seconds(0)}; //!< time at which transmission will be started
        MHz_u centerFreq{0};        //!< center frequency to use
        MHz_u bandwidth{0};         //!< bandwidth to use
    };

    /**
     * structure that holds information to perform PHY state check
     */
    struct StateCheckPoint
    {
        Time timePoint{Seconds(0)}; //!< time at which the check will performed
        WifiPhyState expectedPhyState{WifiPhyState::IDLE}; //!< expected PHY state
    };

    /**
     * structure that holds information to perform CCA check
     */
    struct CcaCheckPoint
    {
        Time timePoint{Seconds(0)};          //!< time at which the check will performed
        Time expectedCcaEndTime{Seconds(0)}; //!< expected CCA_BUSY end time
        WifiChannelListType expectedChannelListType{
            WIFI_CHANLIST_PRIMARY};                    //!< expected channel list type
        std::vector<Time> expectedPer20MhzDurations{}; //!< expected per-20 MHz CCA duration
    };

    /**
     * Schedule test to perform.
     * @param delay the reference delay to schedule the events
     * @param generatedSignals the vector of signals to be generated
     * @param generatedPpdus the vector of PPDUs to be generated
     * @param stateCheckpoints the vector of PHY state checks
     * @param ccaCheckpoints the vector of PHY CCA checks
     */
    void ScheduleTest(Time delay,
                      const std::vector<TxSignalInfo>& generatedSignals,
                      const std::vector<TxPpduInfo>& generatedPpdus,
                      const std::vector<StateCheckPoint>& stateCheckpoints,
                      const std::vector<CcaCheckPoint>& ccaCheckpoints);

    /**
     * Reset function
     */
    void Reset();

    /**
     * Run one function
     */
    void RunOne();

    Ptr<SpectrumWifiPhy> m_rxPhy; ///< PHY object of the receiver
    Ptr<SpectrumWifiPhy> m_txPhy; ///< PHY object of the transmitter

    std::vector<Ptr<WaveformGenerator>> m_signalGenerators; ///< Generators of non-wifi signals
    std::size_t
        m_numSignalGenerators; ///< The number of non-wifi signals generators needed for the test

    std::shared_ptr<CcaTestPhyListener>
        m_rxPhyStateListener; ///< Listener for PHY state transitions

    MHz_u m_frequency;    ///< Operating frequency
    MHz_u m_channelWidth; ///< Operating channel width
};

WifiPhyCcaIndicationTest::WifiPhyCcaIndicationTest()
    : TestCase("Wi-Fi PHY CCA indication test"),
      m_numSignalGenerators(2),
      m_frequency(P20_CENTER_FREQUENCY),
      m_channelWidth(MHz_u{20})
{
}

void
WifiPhyCcaIndicationTest::StartSignal(Ptr<WaveformGenerator> signalGenerator,
                                      dBm_u txPower,
                                      MHz_u frequency,
                                      MHz_u bandwidth,
                                      Time duration)
{
    NS_LOG_FUNCTION(this << signalGenerator << txPower << frequency << bandwidth << duration);

    BandInfo bandInfo;
    bandInfo.fc = MHzToHz(frequency);
    bandInfo.fl = bandInfo.fc - MHzToHz(bandwidth / 2);
    bandInfo.fh = bandInfo.fc + MHzToHz(bandwidth / 2);
    Bands bands;
    bands.push_back(bandInfo);

    Ptr<SpectrumModel> spectrumSignal = Create<SpectrumModel>(bands);
    Ptr<SpectrumValue> signalPsd = Create<SpectrumValue>(spectrumSignal);
    *signalPsd = DbmToW(txPower) / MHzToHz(bandwidth);

    signalGenerator->SetTxPowerSpectralDensity(signalPsd);
    signalGenerator->SetPeriod(duration);
    signalGenerator->Start();
    Simulator::Schedule(duration, &WifiPhyCcaIndicationTest::StopSignal, this, signalGenerator);
}

void
WifiPhyCcaIndicationTest::StopSignal(Ptr<WaveformGenerator> signalGenerator)
{
    NS_LOG_FUNCTION(this << signalGenerator);
    signalGenerator->Stop();
}

void
WifiPhyCcaIndicationTest::SendHeSuPpdu(dBm_u txPower, MHz_u frequency, MHz_u bandwidth)
{
    NS_LOG_FUNCTION(this << txPower);

    auto channelNum = WifiPhyOperatingChannel::FindFirst(0,
                                                         frequency,
                                                         bandwidth,
                                                         WIFI_STANDARD_80211ax,
                                                         WIFI_PHY_BAND_5GHZ)
                          ->number;
    m_txPhy->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, bandwidth, WIFI_PHY_BAND_5GHZ, 0});

    WifiTxVector txVector = WifiTxVector(HePhy::GetHeMcs0(),
                                         0,
                                         WIFI_PREAMBLE_HE_SU,
                                         NanoSeconds(800),
                                         1,
                                         1,
                                         0,
                                         bandwidth,
                                         false);

    Ptr<Packet> pkt = Create<Packet>(1000);
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_QOSDATA);
    hdr.SetQosTid(0);
    Ptr<WifiPsdu> psdu = Create<WifiPsdu>(pkt, hdr);

    m_txPhy->SetTxPowerStart(txPower);
    m_txPhy->SetTxPowerEnd(txPower);

    m_txPhy->Send(psdu, txVector);
}

void
WifiPhyCcaIndicationTest::CheckPhyState(WifiPhyState expectedState)
{
    // This is needed to make sure PHY state will be checked as the last event if a state change
    // occurred at the exact same time as the check
    Simulator::ScheduleNow(&WifiPhyCcaIndicationTest::DoCheckPhyState, this, expectedState);
}

void
WifiPhyCcaIndicationTest::DoCheckPhyState(WifiPhyState expectedState)
{
    WifiPhyState currentState;
    PointerValue ptr;
    m_rxPhy->GetAttribute("State", ptr);
    Ptr<WifiPhyStateHelper> state = DynamicCast<WifiPhyStateHelper>(ptr.Get<WifiPhyStateHelper>());
    currentState = state->GetState();
    NS_TEST_ASSERT_MSG_EQ(currentState,
                          expectedState,
                          "PHY State " << currentState << " does not match expected state "
                                       << expectedState << " at " << Simulator::Now());
}

void
WifiPhyCcaIndicationTest::CheckLastCcaBusyNotification(
    Time expectedEndTime,
    WifiChannelListType expectedChannelType,
    const std::vector<Time>& expectedPer20MhzDurations)
{
    NS_TEST_ASSERT_MSG_EQ(m_rxPhyStateListener->m_endCcaBusy,
                          expectedEndTime,
                          "PHY CCA end time " << m_rxPhyStateListener->m_endCcaBusy
                                              << " does not match expected time " << expectedEndTime
                                              << " at " << Simulator::Now());
    NS_TEST_ASSERT_MSG_EQ(m_rxPhyStateListener->m_lastCcaBusyChannelType,
                          expectedChannelType,
                          "PHY CCA-BUSY for " << m_rxPhyStateListener->m_lastCcaBusyChannelType
                                              << " does not match expected channel type "
                                              << expectedChannelType << " at " << Simulator::Now());
    NS_TEST_ASSERT_MSG_EQ(m_rxPhyStateListener->m_lastPer20MhzCcaBusyDurations.size(),
                          expectedPer20MhzDurations.size(),
                          "PHY CCA-BUSY per-20 MHz durations does not match expected vector"
                              << " at " << Simulator::Now());
    for (std::size_t i = 0; i < expectedPer20MhzDurations.size(); ++i)
    {
        NS_TEST_ASSERT_MSG_EQ(m_rxPhyStateListener->m_lastPer20MhzCcaBusyDurations.at(i),
                              expectedPer20MhzDurations.at(i),
                              "PHY CCA-BUSY per-20 MHz duration at index "
                                  << i << " does not match expected duration at "
                                  << Simulator::Now());
    }
}

void
WifiPhyCcaIndicationTest::LogScenario(const std::string& log) const
{
    NS_LOG_INFO(log);
}

void
WifiPhyCcaIndicationTest::ScheduleTest(Time delay,
                                       const std::vector<TxSignalInfo>& generatedSignals,
                                       const std::vector<TxPpduInfo>& generatedPpdus,
                                       const std::vector<StateCheckPoint>& stateCheckpoints,
                                       const std::vector<CcaCheckPoint>& ccaCheckpoints)
{
    for (const auto& generatedPpdu : generatedPpdus)
    {
        Simulator::Schedule(delay + generatedPpdu.startTime,
                            &WifiPhyCcaIndicationTest::SendHeSuPpdu,
                            this,
                            generatedPpdu.power,
                            generatedPpdu.centerFreq,
                            generatedPpdu.bandwidth);
    }

    std::size_t index = 0;
    for (const auto& generatedSignal : generatedSignals)
    {
        Simulator::Schedule(delay + generatedSignal.startTime,
                            &WifiPhyCcaIndicationTest::StartSignal,
                            this,
                            m_signalGenerators.at(index++),
                            generatedSignal.power,
                            generatedSignal.centerFreq,
                            generatedSignal.bandwidth,
                            generatedSignal.duration);
    }

    for (const auto& checkpoint : ccaCheckpoints)
    {
        Simulator::Schedule(delay + checkpoint.timePoint,
                            &WifiPhyCcaIndicationTest::CheckLastCcaBusyNotification,
                            this,
                            Simulator::Now() + delay + checkpoint.expectedCcaEndTime,
                            checkpoint.expectedChannelListType,
                            checkpoint.expectedPer20MhzDurations);
    }

    for (const auto& checkpoint : stateCheckpoints)
    {
        Simulator::Schedule(delay + checkpoint.timePoint,
                            &WifiPhyCcaIndicationTest::CheckPhyState,
                            this,
                            checkpoint.expectedPhyState);
    }

    Simulator::Schedule(delay + Seconds(0.5), &WifiPhyCcaIndicationTest::Reset, this);
}

void
WifiPhyCcaIndicationTest::Reset()
{
    m_rxPhyStateListener->Reset();
}

void
WifiPhyCcaIndicationTest::DoSetup()
{
    // WifiHelper::EnableLogComponents ();
    // LogComponentEnable ("WifiPhyCcaTest", LOG_LEVEL_ALL);

    Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel>();

    Ptr<Node> rxNode = CreateObject<Node>();
    Ptr<WifiNetDevice> rxDev = CreateObject<WifiNetDevice>();
    rxDev->SetStandard(WIFI_STANDARD_80211ax);
    Ptr<VhtConfiguration> vhtConfiguration = CreateObject<VhtConfiguration>();
    rxDev->SetVhtConfiguration(vhtConfiguration);
    m_rxPhy = CreateObject<SpectrumWifiPhy>();
    m_rxPhyStateListener = std::make_unique<CcaTestPhyListener>();
    m_rxPhy->RegisterListener(m_rxPhyStateListener);
    Ptr<InterferenceHelper> rxInterferenceHelper = CreateObject<InterferenceHelper>();
    m_rxPhy->SetInterferenceHelper(rxInterferenceHelper);
    Ptr<ErrorRateModel> rxErrorModel = CreateObject<NistErrorRateModel>();
    m_rxPhy->SetErrorRateModel(rxErrorModel);
    Ptr<ThresholdPreambleDetectionModel> preambleDetectionModel =
        CreateObject<ThresholdPreambleDetectionModel>();
    m_rxPhy->SetPreambleDetectionModel(preambleDetectionModel);
    m_rxPhy->AddChannel(spectrumChannel);
    m_rxPhy->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_rxPhy->SetDevice(rxDev);
    rxDev->SetPhy(m_rxPhy);
    rxNode->AddDevice(rxDev);

    Ptr<Node> txNode = CreateObject<Node>();
    Ptr<WifiNetDevice> txDev = CreateObject<WifiNetDevice>();
    m_txPhy = CreateObject<SpectrumWifiPhy>();
    m_txPhy->SetAttribute("ChannelSwitchDelay", TimeValue(Seconds(0)));
    Ptr<InterferenceHelper> txInterferenceHelper = CreateObject<InterferenceHelper>();
    m_txPhy->SetInterferenceHelper(txInterferenceHelper);
    Ptr<ErrorRateModel> txErrorModel = CreateObject<NistErrorRateModel>();
    m_txPhy->SetErrorRateModel(txErrorModel);
    m_txPhy->AddChannel(spectrumChannel);
    m_txPhy->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_txPhy->SetDevice(txDev);
    txDev->SetPhy(m_txPhy);
    txNode->AddDevice(txDev);

    for (std::size_t i = 0; i < m_numSignalGenerators; ++i)
    {
        Ptr<Node> signalGeneratorNode = CreateObject<Node>();
        Ptr<NonCommunicatingNetDevice> signalGeneratorDev =
            CreateObject<NonCommunicatingNetDevice>();
        Ptr<WaveformGenerator> signalGenerator = CreateObject<WaveformGenerator>();
        signalGenerator->SetDevice(signalGeneratorDev);
        signalGenerator->SetChannel(spectrumChannel);
        signalGenerator->SetDutyCycle(1);
        signalGeneratorNode->AddDevice(signalGeneratorDev);
        m_signalGenerators.push_back(signalGenerator);
    }
}

void
WifiPhyCcaIndicationTest::RunOne()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 0;
    m_rxPhy->AssignStreams(streamNumber);
    m_txPhy->AssignStreams(streamNumber);

    auto channelNum = WifiPhyOperatingChannel::FindFirst(0,
                                                         m_frequency,
                                                         m_channelWidth,
                                                         WIFI_STANDARD_80211ax,
                                                         WIFI_PHY_BAND_5GHZ)
                          ->number;

    m_rxPhy->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, m_channelWidth, WIFI_PHY_BAND_5GHZ, 0});
    m_txPhy->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, m_channelWidth, WIFI_PHY_BAND_5GHZ, 0});

    std::vector<Time> expectedPer20MhzCcaBusyDurations{};
    Time delay;
    Simulator::Schedule(delay, &WifiPhyCcaIndicationTest::Reset, this);
    delay += Seconds(1);

    //----------------------------------------------------------------------------------------------------------------------------------
    // Verify PHY state stays IDLE and no CCA-BUSY indication is reported when a signal below the
    // energy detection threshold occupies P20
    Simulator::Schedule(delay,
                        &WifiPhyCcaIndicationTest::LogScenario,
                        this,
                        "Reception of a signal that occupies P20 below ED threshold");
    ScheduleTest(
        delay,
        {{dBm_u{-65}, MicroSeconds(0), MicroSeconds(100), P20_CENTER_FREQUENCY, MHz_u{20}}},
        {},
        {
            {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
            {MicroSeconds(100) - smallDelta,
             WifiPhyState::IDLE}, // IDLE just before the transmission ends
            {MicroSeconds(100) + smallDelta,
             WifiPhyState::IDLE} // IDLE just after the transmission ends
        },
        {});
    delay += Seconds(1);

    //----------------------------------------------------------------------------------------------------------------------------------
    // Verify PHY state is CCA-BUSY as long as a 20 MHz signal above the energy detection threshold
    // occupies P20
    Simulator::Schedule(delay,
                        &WifiPhyCcaIndicationTest::LogScenario,
                        this,
                        "Reception of signal that occupies P20 above ED threshold");
    ScheduleTest(
        delay,
        {{dBm_u{-60.0}, MicroSeconds(0), MicroSeconds(100), P20_CENTER_FREQUENCY, MHz_u{20}}},
        {},
        {
            {aCcaTime, WifiPhyState::CCA_BUSY}, // CCA-BUSY after aCCATime
            {MicroSeconds(100) - smallDelta,
             WifiPhyState::CCA_BUSY}, // CCA-BUSY just before the transmission ends
            {MicroSeconds(100) + smallDelta,
             WifiPhyState::IDLE} // IDLE just after the transmission ends
        },
        {{MicroSeconds(100) - smallDelta,
          MicroSeconds(100),
          WIFI_CHANLIST_PRIMARY,
          ((m_channelWidth > MHz_u{20})
               ? ((m_channelWidth > MHz_u{40})
                      ? ((m_channelWidth > MHz_u{80}) ? std::vector<Time>{MicroSeconds(100),
                                                                          MicroSeconds(0),
                                                                          MicroSeconds(0),
                                                                          MicroSeconds(0),
                                                                          MicroSeconds(0),
                                                                          MicroSeconds(0),
                                                                          MicroSeconds(0),
                                                                          MicroSeconds(0)}
                                                      : std::vector<Time>{MicroSeconds(100),
                                                                          MicroSeconds(0),
                                                                          MicroSeconds(0),
                                                                          MicroSeconds(0)})
                      : std::vector<Time>{MicroSeconds(100), MicroSeconds(0)})
               : std::vector<Time>{})}});
    delay += Seconds(1);

    //----------------------------------------------------------------------------------------------------------------------------------
    // Verify PHY state is CCA-BUSY as long as the sum of 20 MHz signals occupying P20 is above the
    // energy detection threshold
    Simulator::Schedule(delay,
                        &WifiPhyCcaIndicationTest::LogScenario,
                        this,
                        "Reception of two 20 MHz signals that occupies P20 below ED threshold with "
                        "sum above ED threshold");
    ScheduleTest(
        delay,
        {{dBm_u{-64.0}, MicroSeconds(0), MicroSeconds(100), P20_CENTER_FREQUENCY, MHz_u{20}},
         {dBm_u{-65.0}, MicroSeconds(50), MicroSeconds(200), P20_CENTER_FREQUENCY, MHz_u{20}}},
        {},
        {
            {MicroSeconds(50) + aCcaTime, WifiPhyState::CCA_BUSY}, // CCA-BUSY after aCCATime
            {MicroSeconds(100) - smallDelta,
             WifiPhyState::CCA_BUSY}, // CCA-BUSY just before the transmission ends
            {MicroSeconds(100) + smallDelta,
             WifiPhyState::IDLE} // IDLE just after the transmission ends
        },
        {{MicroSeconds(100) - smallDelta,
          MicroSeconds(100),
          WIFI_CHANLIST_PRIMARY,
          ((m_channelWidth > MHz_u{20})
               ? ((m_channelWidth > MHz_u{40})
                      ? ((m_channelWidth > MHz_u{80}) ? std::vector<Time>{MicroSeconds(50),
                                                                          MicroSeconds(0),
                                                                          MicroSeconds(0),
                                                                          MicroSeconds(0),
                                                                          MicroSeconds(0),
                                                                          MicroSeconds(0),
                                                                          MicroSeconds(0),
                                                                          MicroSeconds(0)}
                                                      : std::vector<Time>{MicroSeconds(50),
                                                                          MicroSeconds(0),
                                                                          MicroSeconds(0),
                                                                          MicroSeconds(0)})
                      : std::vector<Time>{MicroSeconds(50), MicroSeconds(0)})
               : std::vector<Time>{})}});
    delay += Seconds(1);

    //----------------------------------------------------------------------------------------------------------------------------------
    // Verify PHY state stays IDLE when a 20 MHz HE SU PPDU with received power below the
    // corresponding CCA sensitivity threshold occupies P20
    Simulator::Schedule(
        delay,
        &WifiPhyCcaIndicationTest::LogScenario,
        this,
        "Reception of a 20 MHz HE PPDU that occupies P20 below CCA sensitivity threshold");
    ScheduleTest(delay,
                 {},
                 {{dBm_u{-85}, MicroSeconds(0), P20_CENTER_FREQUENCY, MHz_u{20}}},
                 {
                     {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                     {PpduDurations.at(MHz_u{20}) - smallDelta,
                      WifiPhyState::IDLE}, // IDLE just before the transmission ends
                     {PpduDurations.at(MHz_u{20}) + smallDelta,
                      WifiPhyState::IDLE} // IDLE just after the transmission ends
                 },
                 {});
    delay += Seconds(1);

    //----------------------------------------------------------------------------------------------------------------------------------
    // Verify PHY state transitions to CCA-BUSY when an HE SU PPDU with received power above the CCA
    // sensitivity threshold occupies P20. The per20Bitmap should indicate idle on the primary 20
    // MHz subchannel because received power is below -72 dBm (27.3.20.6.5).
    Simulator::Schedule(
        delay,
        &WifiPhyCcaIndicationTest::LogScenario,
        this,
        "Reception of a 20 MHz HE PPDU that occupies P20 above CCA sensitivity threshold");
    ScheduleTest(
        delay,
        {},
        {{dBm_u{-80}, MicroSeconds(0), P20_CENTER_FREQUENCY, MHz_u{20}}},
        {
            {aCcaTime, WifiPhyState::CCA_BUSY}, // CCA-BUSY after aCCATime
            {PpduDurations.at(MHz_u{20}) - smallDelta,
             WifiPhyState::RX}, // RX just before the transmission ends
            {PpduDurations.at(MHz_u{20}) + smallDelta,
             WifiPhyState::IDLE} // IDLE just after the transmission ends
        },
        {{aCcaTime,
          MicroSeconds(16),
          WIFI_CHANLIST_PRIMARY,
          ((m_channelWidth > 20)
               ? ((m_channelWidth > 40)
                      ? ((m_channelWidth > 80)
                             ? std::vector<Time>{Seconds(0),
                                                 Seconds(0),
                                                 Seconds(0),
                                                 Seconds(0),
                                                 Seconds(0),
                                                 Seconds(0),
                                                 Seconds(0),
                                                 Seconds(0)}
                             : std::vector<Time>{Seconds(0), Seconds(0), Seconds(0), Seconds(0)})
                      : std::vector<Time>{Seconds(0), Seconds(0)})
               : std::vector<Time>{})}});
    delay += Seconds(1);

    //----------------------------------------------------------------------------------------------------------------------------------
    // Verify PHY state stays IDLE when a 40 MHz HE SU PPDU with received power below the CCA
    // sensitivity threshold occupies P40
    Simulator::Schedule(
        delay,
        &WifiPhyCcaIndicationTest::LogScenario,
        this,
        "Reception of a 40 MHz HE PPDU that occupies P20 below CCA sensitivity threshold");
    ScheduleTest(delay,
                 {},
                 {{dBm_u{-80}, MicroSeconds(0), P40_CENTER_FREQUENCY, MHz_u{40}}},
                 {
                     {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                     {PpduDurations.at(MHz_u{40}) - smallDelta,
                      WifiPhyState::IDLE}, // IDLE just before the transmission ends
                     {PpduDurations.at(MHz_u{40}) + smallDelta,
                      WifiPhyState::IDLE} // IDLE just after the transmission ends
                 },
                 {});
    delay += Seconds(1);

    //----------------------------------------------------------------------------------------------------------------------------------
    // Verify PHY state transitions to CCA-BUSY when an HE SU PPDU with received power above the CCA
    // sensitivity threshold occupies P40. The per20Bitmap should indicate idle on the primary 20
    // MHz subchannel because received power is below -72 dBm (27.3.20.6.5).
    Simulator::Schedule(
        delay,
        &WifiPhyCcaIndicationTest::LogScenario,
        this,
        "Reception of a 40 MHz HE PPDU that occupies P40 above CCA sensitivity threshold");
    ScheduleTest(
        delay,
        {},
        {{dBm_u{-75}, MicroSeconds(0), P40_CENTER_FREQUENCY, MHz_u{40}}},
        {
            {aCcaTime, WifiPhyState::CCA_BUSY}, // CCA-BUSY after aCCATime
            {PpduDurations.at(MHz_u{40}) - smallDelta,
             (m_channelWidth > MHz_u{20})
                 ? WifiPhyState::RX
                 : WifiPhyState::CCA_BUSY}, // RX or IDLE just before the transmission ends
            {PpduDurations.at(MHz_u{40}) + smallDelta,
             WifiPhyState::IDLE} // IDLE just after the transmission ends
        },
        {{aCcaTime,
          MicroSeconds(16),
          WIFI_CHANLIST_PRIMARY,
          ((m_channelWidth > 20)
               ? ((m_channelWidth > 40)
                      ? ((m_channelWidth > 80)
                             ? std::vector<Time>{Seconds(0),
                                                 Seconds(0),
                                                 Seconds(0),
                                                 Seconds(0),
                                                 Seconds(0),
                                                 Seconds(0),
                                                 Seconds(0),
                                                 Seconds(0)}
                             : std::vector<Time>{Seconds(0), Seconds(0), Seconds(0), Seconds(0)})
                      : std::vector<Time>{Seconds(0), Seconds(0)})
               : std::vector<Time>{})}});
    delay += Seconds(1);

    if (m_channelWidth > MHz_u{20})
    {
        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE and no CCA-BUSY indication is reported when a 20 MHz signal
        // below the energy detection threshold occupies S20
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 20 MHz signal that occupies S20 below ED threshold");
        ScheduleTest(
            delay,
            {{dBm_u{-65}, MicroSeconds(0), MicroSeconds(100), S20_CENTER_FREQUENCY, MHz_u{20}}},
            {},
            {
                {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                {MicroSeconds(100) - smallDelta,
                 WifiPhyState::IDLE}, // IDLE just before the transmission ends
                {MicroSeconds(100) + smallDelta,
                 WifiPhyState::IDLE} // IDLE just after the transmission ends
            },
            {});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE but CCA-BUSY indication is reported when a 20 MHz signal
        // above the energy detection threshold occupies S20
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 20 MHz signal that occupies S20 above ED threshold");
        ScheduleTest(
            delay,
            {{dBm_u{-60}, MicroSeconds(0), MicroSeconds(100), S20_CENTER_FREQUENCY, MHz_u{20}}},
            {},
            {
                {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                {MicroSeconds(100) - smallDelta,
                 WifiPhyState::IDLE}, // IDLE just before the transmission ends
                {MicroSeconds(100) + smallDelta,
                 WifiPhyState::IDLE} // IDLE just after the transmission ends
            },
            {{MicroSeconds(100) - smallDelta,
              MicroSeconds(100),
              WIFI_CHANLIST_SECONDARY,
              ((m_channelWidth > MHz_u{40})
                   ? ((m_channelWidth > MHz_u{80}) ? std::vector<Time>{MicroSeconds(0),
                                                                       MicroSeconds(100),
                                                                       MicroSeconds(0),
                                                                       MicroSeconds(0),
                                                                       MicroSeconds(0),
                                                                       MicroSeconds(0),
                                                                       MicroSeconds(0),
                                                                       MicroSeconds(0)}
                                                   : std::vector<Time>{MicroSeconds(0),
                                                                       MicroSeconds(100),
                                                                       MicroSeconds(0),
                                                                       MicroSeconds(0)})
                   : std::vector<Time>{MicroSeconds(0), MicroSeconds(100)})}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state is CCA-BUSY as long as a 40 MHz signal above the energy detection
        // threshold occupies P40
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 40 MHz signal that occupies P40 above ED threshold");
        ScheduleTest(
            delay,
            {{dBm_u{-55}, MicroSeconds(0), MicroSeconds(100), P40_CENTER_FREQUENCY, MHz_u{40}}},
            {},
            {
                {aCcaTime, WifiPhyState::CCA_BUSY}, // CCA-BUSY after aCCATime
                {MicroSeconds(100) - smallDelta,
                 WifiPhyState::CCA_BUSY}, // CCA-BUSY just before the transmission ends
                {MicroSeconds(100) + smallDelta,
                 WifiPhyState::IDLE} // IDLE just after the transmission ends
            },
            {{MicroSeconds(100) - smallDelta,
              MicroSeconds(100),
              WIFI_CHANLIST_PRIMARY,
              ((m_channelWidth > 40) ? ((m_channelWidth > 80) ? std::vector<Time>{MicroSeconds(100),
                                                                                  MicroSeconds(100),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0)}
                                                              : std::vector<Time>{MicroSeconds(100),
                                                                                  MicroSeconds(100),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0)})
                                     : std::vector<Time>{MicroSeconds(100), MicroSeconds(100)})}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY notifies CCA-BUSY for the primary channel while the secondary channel was
        // already in CCA-BUSY state
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a signal that occupies S20 followed by the reception of "
                            "another signal that occupies P20");
        ScheduleTest(
            delay,
            {{dBm_u{-60}, MicroSeconds(0), MicroSeconds(100), S20_CENTER_FREQUENCY, MHz_u{20}},
             {dBm_u{-60}, MicroSeconds(50), MicroSeconds(100), P20_CENTER_FREQUENCY, MHz_u{20}}},
            {},
            {
                {aCcaTime, WifiPhyState::IDLE}, // state of primary stays idle after aCCATime
                {MicroSeconds(50) + aCcaTime,
                 WifiPhyState::CCA_BUSY}, // state of primary is CCA-BUSY after aCCATime that
                                          // followed the second transmission
                {MicroSeconds(50) + MicroSeconds(100) - smallDelta,
                 WifiPhyState::CCA_BUSY}, // CCA-BUSY just before the transmission ends
                {MicroSeconds(50) + MicroSeconds(100) + smallDelta,
                 WifiPhyState::IDLE} // IDLE just after the transmission ends
            },
            {{aCcaTime, // notification upon reception of the first signal
              MicroSeconds(100),
              WIFI_CHANLIST_SECONDARY,
              ((m_channelWidth > 40) ? ((m_channelWidth > 80) ? std::vector<Time>{MicroSeconds(0),
                                                                                  MicroSeconds(100),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0)}
                                                              : std::vector<Time>{MicroSeconds(0),
                                                                                  MicroSeconds(100),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0)})
                                     : std::vector<Time>{MicroSeconds(0), MicroSeconds(100)})},
             {MicroSeconds(50) + aCcaTime, // notification upon reception of the second signal
              MicroSeconds(50) + MicroSeconds(100),
              WIFI_CHANLIST_PRIMARY,
              ((m_channelWidth > 40) ? ((m_channelWidth > 80) ? std::vector<Time>{MicroSeconds(100),
                                                                                  MicroSeconds(50),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0)}
                                                              : std::vector<Time>{MicroSeconds(100),
                                                                                  MicroSeconds(50),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0)})
                                     : std::vector<Time>{MicroSeconds(100), MicroSeconds(50)})}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY updates per-20 MHz CCA durations if a signal arrives on the secondary channel
        // while primary is CCA-BUSY
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a signal that occupies P20 followed by the reception of "
                            "another signal that occupies S20");
        ScheduleTest(
            delay,
            {{dBm_u{-60}, MicroSeconds(0), MicroSeconds(100), P20_CENTER_FREQUENCY, MHz_u{20}},
             {dBm_u{-60}, MicroSeconds(50), MicroSeconds(100), S20_CENTER_FREQUENCY, MHz_u{20}}},
            {},
            {
                {aCcaTime, WifiPhyState::CCA_BUSY}, // CCA-BUSY after aCCATime
                {MicroSeconds(50) + aCcaTime,
                 WifiPhyState::CCA_BUSY}, // state of primary is still CCA-BUSY after aCCATime that
                                          // followed the second transmission
                {MicroSeconds(100) - smallDelta,
                 WifiPhyState::CCA_BUSY}, // CCA-BUSY just before the first transmission ends
                {MicroSeconds(100) + smallDelta,
                 WifiPhyState::IDLE} // IDLE just after the first transmission ends
            },
            {{aCcaTime, // notification upon reception of the first signal
              MicroSeconds(100),
              WIFI_CHANLIST_PRIMARY,
              ((m_channelWidth > 40) ? ((m_channelWidth > 80) ? std::vector<Time>{MicroSeconds(100),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0)}
                                                              : std::vector<Time>{MicroSeconds(100),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0)})
                                     : std::vector<Time>{MicroSeconds(100), MicroSeconds(0)})},
             {MicroSeconds(50) + aCcaTime, // notification upon reception of the second signal
              MicroSeconds(100),
              WIFI_CHANLIST_PRIMARY,
              ((m_channelWidth > 40) ? ((m_channelWidth > 80) ? std::vector<Time>{MicroSeconds(50),
                                                                                  MicroSeconds(100),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0)}
                                                              : std::vector<Time>{MicroSeconds(50),
                                                                                  MicroSeconds(100),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0)})
                                     : std::vector<Time>{MicroSeconds(50), MicroSeconds(100)})}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE when a 20 MHz HE SU PPDU with received power below the CCA
        // sensitivity threshold occupies S40
        Simulator::Schedule(
            delay,
            &WifiPhyCcaIndicationTest::LogScenario,
            this,
            "Reception of a 20 MHz HE PPDU that occupies S20 below CCA sensitivity threshold");
        ScheduleTest(delay,
                     {},
                     {{dBm_u{-75}, MicroSeconds(0), S20_CENTER_FREQUENCY, MHz_u{20}}},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {PpduDurations.at(MHz_u{20}) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {PpduDurations.at(MHz_u{20}) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE but CCA-BUSY indication is reported when a 20 MHz HE SU PPDU
        // with received power above the CCA sensitivity threshold occupies S20
        Simulator::Schedule(
            delay,
            &WifiPhyCcaIndicationTest::LogScenario,
            this,
            "Reception of a 20 MHz HE PPDU that occupies S20 above CCA sensitivity threshold");
        ScheduleTest(
            delay,
            {},
            {{dBm_u{-70}, MicroSeconds(0), S20_CENTER_FREQUENCY, MHz_u{20}}},
            {
                {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                {PpduDurations.at(MHz_u{20}) - smallDelta,
                 WifiPhyState::IDLE}, // IDLE just before the transmission ends
                {PpduDurations.at(MHz_u{20}) + smallDelta,
                 WifiPhyState::IDLE} // IDLE just after the transmission ends
            },
            {{aCcaTime,
              PpduDurations.at(MHz_u{20}),
              WIFI_CHANLIST_SECONDARY,
              ((m_channelWidth > MHz_u{40})
                   ? ((m_channelWidth > MHz_u{80}) ? std::vector<Time>{NanoSeconds(0),
                                                                       PpduDurations.at(MHz_u{20}),
                                                                       NanoSeconds(0),
                                                                       NanoSeconds(0),
                                                                       NanoSeconds(0),
                                                                       NanoSeconds(0),
                                                                       NanoSeconds(0),
                                                                       NanoSeconds(0)}
                                                   : std::vector<Time>{NanoSeconds(0),
                                                                       PpduDurations.at(MHz_u{20}),
                                                                       NanoSeconds(0),
                                                                       NanoSeconds(0)})
                   : std::vector<Time>{NanoSeconds(0), PpduDurations.at(MHz_u{20})})}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE but CCA-BUSY indication is still reported as long as a signal
        // above the energy detection threshold occupies the S20 while a 40 MHz PPDU below the CCA
        // sensitivity threshold is received on P40.
        Simulator::Schedule(
            delay,
            &WifiPhyCcaIndicationTest::LogScenario,
            this,
            "Reception of a 20 MHz signal that occupies S20 above ED threshold followed by a 40 "
            "MHz HE PPDU that occupies P40 below CCA sensitivity threshold");
        ScheduleTest(
            delay,
            {{dBm_u{-60},
              MicroSeconds(0),
              MicroSeconds(100),
              S20_CENTER_FREQUENCY,
              MHz_u{20}}}, // signal on S20 above threshold
            {{dBm_u{-80},
              MicroSeconds(50),
              P40_CENTER_FREQUENCY,
              MHz_u{40}}}, // PPDU on P40 below threshold
            {
                {MicroSeconds(50) + aCcaTime, WifiPhyState::IDLE}, // PHY state stays IDLE
            },
            {{MicroSeconds(50) - smallDelta,
              MicroSeconds(100),
              WIFI_CHANLIST_SECONDARY,
              ((m_channelWidth > 20)
                   ? ((m_channelWidth > 40)
                          ? ((m_channelWidth > 80) ? std::vector<Time>{MicroSeconds(0),
                                                                       MicroSeconds(100),
                                                                       MicroSeconds(0),
                                                                       MicroSeconds(0),
                                                                       MicroSeconds(0),
                                                                       MicroSeconds(0),
                                                                       MicroSeconds(0),
                                                                       MicroSeconds(0)}
                                                   : std::vector<Time>{MicroSeconds(0),
                                                                       MicroSeconds(100),
                                                                       MicroSeconds(0),
                                                                       MicroSeconds(0)})
                          : std::vector<Time>{MicroSeconds(0), MicroSeconds(100)})
                   : std::vector<Time>{})},
             {MicroSeconds(100) - smallDelta,
              MicroSeconds(100),
              WIFI_CHANLIST_SECONDARY,
              ((m_channelWidth > 40) ? ((m_channelWidth > 80) ? std::vector<Time>{MicroSeconds(0),
                                                                                  MicroSeconds(46),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0)}
                                                              : std::vector<Time>{MicroSeconds(0),
                                                                                  MicroSeconds(46),
                                                                                  MicroSeconds(0),
                                                                                  MicroSeconds(0)})
                                     : std::vector<Time>{MicroSeconds(0), MicroSeconds(46)})}});
        delay += Seconds(1);
    }

    if (m_channelWidth > MHz_u{40})
    {
        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE and no CCA-BUSY indication is reported when a signal below
        // the energy detection threshold occupies S40
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 20 MHz signal that occupies the first subchannel of "
                            "S40 below ED threshold");
        ScheduleTest(delay,
                     {{dBm_u{-65},
                       MicroSeconds(0),
                       MicroSeconds(100),
                       S40_CENTER_FREQUENCY - MHz_u{10},
                       MHz_u{20}}},
                     {},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {MicroSeconds(100) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {MicroSeconds(100) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY notifies CCA-BUSY for the S40 as long as a signal above the energy detection
        // threshold occupies the first 20 MHz subchannel of the S40: 27.3.20.6.4: Any signal within
        // the secondary 40 MHz channel at or above a threshold of –59 dBm within a period of
        // aCCATime after the signal arrives at the receiver's antenna(s).
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 20 MHz signal that occupies the first subchannel of "
                            "S40 above ED threshold");
        ScheduleTest(delay,
                     {{dBm_u{-55},
                       MicroSeconds(0),
                       MicroSeconds(100),
                       S40_CENTER_FREQUENCY - MHz_u{10},
                       MHz_u{20}}},
                     {},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {MicroSeconds(100) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {MicroSeconds(100) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {{MicroSeconds(100) - smallDelta,
                       MicroSeconds(100),
                       WIFI_CHANLIST_SECONDARY40,
                       ((m_channelWidth > 80) ? std::vector<Time>{MicroSeconds(0),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(100),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(0)}
                                              : std::vector<Time>{MicroSeconds(0),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(100),
                                                                  MicroSeconds(0)})}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE for the S40 if a signal below the energy detection threshold
        // occupies the second 20 MHz subchannel of the S40
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 20 MHz signal that occupies the second subchannel of "
                            "S40 below ED threshold");
        ScheduleTest(delay,
                     {{dBm_u{-65},
                       MicroSeconds(0),
                       MicroSeconds(100),
                       S40_CENTER_FREQUENCY + MHz_u{10},
                       MHz_u{20}}},
                     {},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {MicroSeconds(100) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {MicroSeconds(100) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY notifies CCA-BUSY for the S40 as long as a signal above the energy detection
        // threshold occupies the second 20 MHz subchannel of the S40: 27.3.20.6.4: Any signal
        // within the secondary 40 MHz channel at or above a threshold of –59 dBm within a period of
        // aCCATime after the signal arrives at the receiver's antenna(s).
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 20 MHz signal that occupies the second subchannel of "
                            "S40 above ED threshold");
        ScheduleTest(delay,
                     {{dBm_u{-55},
                       MicroSeconds(0),
                       MicroSeconds(100),
                       S40_CENTER_FREQUENCY + MHz_u{10},
                       MHz_u{20}}},
                     {},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {MicroSeconds(100) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {MicroSeconds(100) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {{MicroSeconds(100) - smallDelta,
                       MicroSeconds(100),
                       WIFI_CHANLIST_SECONDARY40,
                       ((m_channelWidth > 80) ? std::vector<Time>{MicroSeconds(0),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(100),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(0)}
                                              : std::vector<Time>{MicroSeconds(0),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(100)})}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE for the S40 if a signal below the energy detection threshold
        // occupies S40
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 40 MHz signal that occupies S40 below ED threshold");
        ScheduleTest(
            delay,
            {{dBm_u{-60}, MicroSeconds(0), MicroSeconds(100), S40_CENTER_FREQUENCY, MHz_u{40}}},
            {},
            {
                {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                {MicroSeconds(100) - smallDelta,
                 WifiPhyState::IDLE}, // IDLE just before the transmission ends
                {MicroSeconds(100) + smallDelta,
                 WifiPhyState::IDLE} // IDLE just after the transmission ends
            },
            {});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY notifies CCA-BUSY for the S40 as long as a signal above the energy detection
        // threshold occupies S40: 27.3.20.6.4: Any signal within the secondary 40 MHz channel at or
        // above a threshold of –59 dBm within a period of aCCATime after the signal arrives at the
        // receiver's antenna(s).
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 20 MHz signal that occupies the second subchannel of "
                            "S40 above ED threshold");
        ScheduleTest(
            delay,
            {{dBm_u{-55}, MicroSeconds(0), MicroSeconds(100), S40_CENTER_FREQUENCY, MHz_u{40}}},
            {},
            {
                {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                {MicroSeconds(100) - smallDelta,
                 WifiPhyState::IDLE}, // IDLE just before the transmission ends
                {MicroSeconds(100) + smallDelta,
                 WifiPhyState::IDLE} // IDLE just after the transmission ends
            },
            {{MicroSeconds(100) - smallDelta,
              MicroSeconds(100),
              WIFI_CHANLIST_SECONDARY40,
              ((m_channelWidth > MHz_u{80}) ? std::vector<Time>{MicroSeconds(0),
                                                                MicroSeconds(0),
                                                                MicroSeconds(100),
                                                                MicroSeconds(100),
                                                                MicroSeconds(0),
                                                                MicroSeconds(0),
                                                                MicroSeconds(0),
                                                                MicroSeconds(0)}
                                            : std::vector<Time>{MicroSeconds(0),
                                                                MicroSeconds(0),
                                                                MicroSeconds(100),
                                                                MicroSeconds(100)})}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state is CCA-BUSY as long as a 80 MHz signal above the energy detection
        // threshold occupies P80
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 80 MHz signal that occupies P80 above ED threshold");
        ScheduleTest(
            delay,
            {{dBm_u{-55}, MicroSeconds(0), MicroSeconds(100), P80_CENTER_FREQUENCY, MHz_u{80}}},
            {},
            {
                {aCcaTime, WifiPhyState::CCA_BUSY}, // CCA-BUSY after aCCATime
                {MicroSeconds(100) - smallDelta,
                 WifiPhyState::CCA_BUSY}, // CCA-BUSY just before the transmission ends
                {MicroSeconds(100) + smallDelta,
                 WifiPhyState::IDLE} // IDLE just after the transmission ends
            },
            {{MicroSeconds(100) - smallDelta,
              MicroSeconds(100),
              WIFI_CHANLIST_PRIMARY,
              ((m_channelWidth > MHz_u{80}) ? std::vector<Time>{MicroSeconds(100),
                                                                MicroSeconds(100),
                                                                MicroSeconds(100),
                                                                MicroSeconds(100),
                                                                MicroSeconds(0),
                                                                MicroSeconds(0),
                                                                MicroSeconds(0),
                                                                MicroSeconds(0)}
                                            : std::vector<Time>{MicroSeconds(100),
                                                                MicroSeconds(100),
                                                                MicroSeconds(100),
                                                                MicroSeconds(100)})}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY notifies CCA-BUSY for the P20 channel while the S40 channel was already in
        // CCA-BUSY state
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 20 MHz signal that occupies S40 followed by the "
                            "reception of another 20 MHz signal that occupies P20");
        ScheduleTest(
            delay,
            {{dBm_u{-55},
              MicroSeconds(0),
              MicroSeconds(100),
              S40_CENTER_FREQUENCY - MHz_u{10},
              MHz_u{20}},
             {dBm_u{-55}, MicroSeconds(50), MicroSeconds(100), P20_CENTER_FREQUENCY, MHz_u{20}}},
            {},
            {
                {aCcaTime, WifiPhyState::IDLE}, // state of primary stays idle after aCCATime
                {MicroSeconds(50) + aCcaTime,
                 WifiPhyState::CCA_BUSY}, // state of primary is CCA-BUSY after aCCATime that
                                          // followed the second transmission
                {MicroSeconds(50) + MicroSeconds(100) - smallDelta,
                 WifiPhyState::CCA_BUSY}, // CCA-BUSY just before the transmission ends
                {MicroSeconds(50) + MicroSeconds(100) + smallDelta,
                 WifiPhyState::IDLE} // IDLE just after the transmission ends
            },
            {{aCcaTime, // notification upon reception of the first signal
              MicroSeconds(100),
              WIFI_CHANLIST_SECONDARY40,
              ((m_channelWidth > 80) ? std::vector<Time>{MicroSeconds(0),
                                                         MicroSeconds(0),
                                                         MicroSeconds(100),
                                                         MicroSeconds(0),
                                                         MicroSeconds(0),
                                                         MicroSeconds(0),
                                                         MicroSeconds(0),
                                                         MicroSeconds(0)}
                                     : std::vector<Time>{MicroSeconds(0),
                                                         MicroSeconds(0),
                                                         MicroSeconds(100),
                                                         MicroSeconds(0)})},
             {MicroSeconds(50) + aCcaTime, // notification upon reception of the second signal
              MicroSeconds(50) + MicroSeconds(100),
              WIFI_CHANLIST_PRIMARY,
              ((m_channelWidth > 80) ? std::vector<Time>{MicroSeconds(100),
                                                         MicroSeconds(0),
                                                         MicroSeconds(50),
                                                         MicroSeconds(0),
                                                         MicroSeconds(0),
                                                         MicroSeconds(0),
                                                         MicroSeconds(0),
                                                         MicroSeconds(0)}
                                     : std::vector<Time>{MicroSeconds(100),
                                                         MicroSeconds(0),
                                                         MicroSeconds(50),
                                                         MicroSeconds(0)})}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE but notifies CCA-BUSY for the S20 channel while the S40
        // channel was already in CCA-BUSY state
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a signal that occupies S40 followed by the reception of "
                            "another signal that occupies S20");
        ScheduleTest(
            delay,
            {{dBm_u{-55},
              MicroSeconds(0),
              MicroSeconds(100),
              S40_CENTER_FREQUENCY - MHz_u{10},
              MHz_u{20}},
             {dBm_u{-55}, MicroSeconds(50), MicroSeconds(100), S20_CENTER_FREQUENCY, MHz_u{20}}},
            {},
            {
                {aCcaTime, WifiPhyState::IDLE}, // state of primary stays idle after aCCATime
                {MicroSeconds(50) + aCcaTime, WifiPhyState::IDLE}, // state of primary stays IDLE
                {MicroSeconds(50) + MicroSeconds(100) - smallDelta,
                 WifiPhyState::IDLE}, // IDLE just before the transmission ends
                {MicroSeconds(50) + MicroSeconds(100) + smallDelta,
                 WifiPhyState::IDLE} // IDLE just after the transmission ends
            },
            {{aCcaTime, // notification upon reception of the first signal
              MicroSeconds(100),
              WIFI_CHANLIST_SECONDARY40,
              ((m_channelWidth > 80) ? std::vector<Time>{MicroSeconds(0),
                                                         MicroSeconds(0),
                                                         MicroSeconds(100),
                                                         MicroSeconds(0),
                                                         MicroSeconds(0),
                                                         MicroSeconds(0),
                                                         MicroSeconds(0),
                                                         MicroSeconds(0)}
                                     : std::vector<Time>{MicroSeconds(0),
                                                         MicroSeconds(0),
                                                         MicroSeconds(100),
                                                         MicroSeconds(0)})},
             {MicroSeconds(50) + aCcaTime, // notification upon reception of the second signal
              MicroSeconds(50) + MicroSeconds(100),
              WIFI_CHANLIST_SECONDARY,
              ((m_channelWidth > 80) ? std::vector<Time>{MicroSeconds(0),
                                                         MicroSeconds(100),
                                                         MicroSeconds(50),
                                                         MicroSeconds(0),
                                                         MicroSeconds(0),
                                                         MicroSeconds(0),
                                                         MicroSeconds(0),
                                                         MicroSeconds(0)}
                                     : std::vector<Time>{MicroSeconds(0),
                                                         MicroSeconds(100),
                                                         MicroSeconds(50),
                                                         MicroSeconds(0)})}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE when a 40 MHz HE SU PPDU with received power below the CCA
        // sensitivity threshold occupies S40
        Simulator::Schedule(
            delay,
            &WifiPhyCcaIndicationTest::LogScenario,
            this,
            "Reception of a 40 MHz HE PPDU that occupies S40 below CCA sensitivity threshold");
        ScheduleTest(delay,
                     {},
                     {{dBm_u{-75}, MicroSeconds(0), S40_CENTER_FREQUENCY, MHz_u{40}}},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {PpduDurations.at(MHz_u{20}) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {PpduDurations.at(MHz_u{20}) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE but CCA-BUSY indication is reported when a 40 MHz HE SU PPDU
        // with received power above the CCA sensitivity threshold occupies S40
        Simulator::Schedule(
            delay,
            &WifiPhyCcaIndicationTest::LogScenario,
            this,
            "Reception of a 40 MHz HE PPDU that occupies S40 above CCA sensitivity threshold");
        ScheduleTest(
            delay,
            {},
            {{dBm_u{-70.0}, MicroSeconds(0), S40_CENTER_FREQUENCY, MHz_u{40}}},
            {
                {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                {PpduDurations.at(MHz_u{40}) - smallDelta,
                 WifiPhyState::IDLE}, // IDLE just before the transmission ends
                {PpduDurations.at(MHz_u{40}) + smallDelta,
                 WifiPhyState::IDLE} // IDLE just after the transmission ends
            },
            {{aCcaTime,
              PpduDurations.at(MHz_u{40}),
              WIFI_CHANLIST_SECONDARY40,
              ((m_channelWidth > MHz_u{80}) ? std::vector<Time>{NanoSeconds(0),
                                                                NanoSeconds(0),
                                                                PpduDurations.at(MHz_u{40}),
                                                                PpduDurations.at(MHz_u{40}),
                                                                NanoSeconds(0),
                                                                NanoSeconds(0),
                                                                NanoSeconds(0),
                                                                NanoSeconds(0)}
                                            : std::vector<Time>{NanoSeconds(0),
                                                                NanoSeconds(0),
                                                                PpduDurations.at(MHz_u{40}),
                                                                PpduDurations.at(MHz_u{40})})}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE but CCA-BUSY indication is still reported as long as a signal
        // above the energy detection threshold occupies the S40 while a 80 MHz PPDU below the CCA
        // sensitivity threshold is received on P80.
        Simulator::Schedule(
            delay,
            &WifiPhyCcaIndicationTest::LogScenario,
            this,
            "Reception of a 40 MHz signal that occupies S40 above ED threshold followed by a 80 "
            "MHz HE PPDU that occupies P80 below CCA sensitivity threshold");
        ScheduleTest(delay,
                     {{dBm_u{-55},
                       MicroSeconds(0),
                       MicroSeconds(100),
                       S40_CENTER_FREQUENCY,
                       MHz_u{40}}}, // signal on S40 above threshold
                     {{dBm_u{-80},
                       MicroSeconds(50),
                       P80_CENTER_FREQUENCY,
                       MHz_u{80}}}, // PPDU on P80 below threshold
                     {
                         {MicroSeconds(50) + aCcaTime, WifiPhyState::IDLE}, // PHY state stays IDLE
                     },
                     {{MicroSeconds(50) - smallDelta,
                       MicroSeconds(100),
                       WIFI_CHANLIST_SECONDARY40,
                       ((m_channelWidth > 80) ? std::vector<Time>{MicroSeconds(0),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(100),
                                                                  MicroSeconds(100),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(0)}
                                              : std::vector<Time>{MicroSeconds(0),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(100),
                                                                  MicroSeconds(100)})},
                      {MicroSeconds(100) - smallDelta,
                       MicroSeconds(100),
                       WIFI_CHANLIST_SECONDARY40,
                       ((m_channelWidth > 80) ? std::vector<Time>{MicroSeconds(0),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(46),
                                                                  MicroSeconds(46),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(0)}
                                              : std::vector<Time>{MicroSeconds(0),
                                                                  MicroSeconds(0),
                                                                  MicroSeconds(46),
                                                                  MicroSeconds(46)})}});
        delay += Seconds(1);
    }
    else // 20 or 40 MHz receiver
    {
        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY notifies CCA-BUSY when a 80 MHz HE SU PPDU with received power above the CCA
        // sensitivity threshold occupies P40 The per20Bitmap should indicate idle for all
        // subchannels because received power is below -62 dBm (27.3.20.6.5).
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 80 MHz HE PPDU that occupies the 40 MHz band above CCA "
                            "sensitivity threshold");
        ScheduleTest(delay,
                     {},
                     {{dBm_u{-70}, MicroSeconds(0), P80_CENTER_FREQUENCY, MHz_u{80}}},
                     {
                         {aCcaTime, WifiPhyState::CCA_BUSY}, // CCA_BUSY after aCCATime
                         {PpduDurations.at(MHz_u{80}) - smallDelta,
                          WifiPhyState::CCA_BUSY}, // CCA_BUSY just before the transmission ends
                         {PpduDurations.at(MHz_u{80}) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {{aCcaTime,
                       MicroSeconds(16),
                       WIFI_CHANLIST_PRIMARY,
                       ((m_channelWidth > MHz_u{20})
                            ? ((m_channelWidth > MHz_u{40})
                                   ? ((m_channelWidth > MHz_u{80}) ? std::vector<Time>{Seconds(0),
                                                                                       Seconds(0),
                                                                                       Seconds(0),
                                                                                       Seconds(0),
                                                                                       Seconds(0),
                                                                                       Seconds(0),
                                                                                       Seconds(0),
                                                                                       Seconds(0)}
                                                                   : std::vector<Time>{Seconds(0),
                                                                                       Seconds(0),
                                                                                       Seconds(0),
                                                                                       Seconds(0)})
                                   : std::vector<Time>{Seconds(0), Seconds(0)})
                            : std::vector<Time>{})},
                      {PpduDurations.at(MHz_u{80}) - smallDelta,
                       PpduDurations.at(MHz_u{80}),
                       WIFI_CHANLIST_PRIMARY,
                       ((m_channelWidth > MHz_u{20})
                            ? ((m_channelWidth > MHz_u{40})
                                   ? ((m_channelWidth > MHz_u{80}) ? std::vector<Time>{Seconds(0),
                                                                                       Seconds(0),
                                                                                       Seconds(0),
                                                                                       Seconds(0),
                                                                                       Seconds(0),
                                                                                       Seconds(0),
                                                                                       Seconds(0),
                                                                                       Seconds(0)}
                                                                   : std::vector<Time>{Seconds(0),
                                                                                       Seconds(0),
                                                                                       Seconds(0),
                                                                                       Seconds(0)})
                                   : std::vector<Time>{Seconds(0), Seconds(0)})
                            : std::vector<Time>{})}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY notifies CCA-BUSY when a 80 MHz HE SU PPDU with received power above the CCA
        // sensitivity threshold occupies P40 The per20Bitmap should indicate CCA_BUSY for all
        // subchannels because received power is above -62 dBm (27.3.20.6.5).
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 80 MHz HE PPDU that occupies the 40 MHz band above CCA "
                            "sensitivity threshold");
        ScheduleTest(
            delay,
            {},
            {{dBm_u{-55}, MicroSeconds(0), P80_CENTER_FREQUENCY, MHz_u{80}}},
            {
                {aCcaTime, WifiPhyState::CCA_BUSY}, // CCA_BUSY after aCCATime
                {PpduDurations.at(MHz_u{80}) - smallDelta,
                 WifiPhyState::CCA_BUSY}, // CCA_BUSY just before the transmission ends
                {PpduDurations.at(MHz_u{80}) + smallDelta,
                 WifiPhyState::IDLE} // IDLE just after the transmission ends
            },
            {{aCcaTime,
              MicroSeconds(16),
              WIFI_CHANLIST_PRIMARY,
              ((m_channelWidth > MHz_u{20})
                   ? ((m_channelWidth > MHz_u{40})
                          ? ((m_channelWidth > MHz_u{80}) ? std::vector<Time>{NanoSeconds(271200),
                                                                              NanoSeconds(271200),
                                                                              NanoSeconds(271200),
                                                                              NanoSeconds(271200),
                                                                              NanoSeconds(0),
                                                                              NanoSeconds(0),
                                                                              NanoSeconds(0),
                                                                              NanoSeconds(0)}
                                                          : std::vector<Time>{NanoSeconds(271200),
                                                                              NanoSeconds(271200),
                                                                              NanoSeconds(271200),
                                                                              NanoSeconds(271200)})
                          : std::vector<Time>{NanoSeconds(271200), NanoSeconds(271200)})
                   : std::vector<Time>{})},
             {PpduDurations.at(MHz_u{80}) - smallDelta,
              PpduDurations.at(MHz_u{80}),
              WIFI_CHANLIST_PRIMARY,
              ((m_channelWidth > 20)
                   ? ((m_channelWidth > 40)
                          ? ((m_channelWidth > 80) ? std::vector<Time>{NanoSeconds(243200),
                                                                       NanoSeconds(243200),
                                                                       NanoSeconds(243200),
                                                                       NanoSeconds(243200),
                                                                       NanoSeconds(0),
                                                                       NanoSeconds(0),
                                                                       NanoSeconds(0),
                                                                       NanoSeconds(0)}
                                                   : std::vector<Time>{NanoSeconds(243200),
                                                                       NanoSeconds(243200),
                                                                       NanoSeconds(243200),
                                                                       NanoSeconds(243200)})
                          : std::vector<Time>{NanoSeconds(243200), NanoSeconds(243200)})
                   : std::vector<Time>{})}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE and no CCA-BUSY indication is reported when a signal not
        // occupying the operational channel is being received
        Simulator::Schedule(
            delay,
            &WifiPhyCcaIndicationTest::LogScenario,
            this,
            "Reception of a 40 MHz HE PPDU that does not occupy the operational channel");
        ScheduleTest(delay,
                     {},
                     {{dBm_u{-50}, MicroSeconds(0), S40_CENTER_FREQUENCY, MHz_u{40}}},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {PpduDurations.at(MHz_u{20}) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {PpduDurations.at(MHz_u{20}) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {});
        delay += Seconds(1);
    }

    if (m_channelWidth > MHz_u{80})
    {
        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE and no CCA-BUSY indication is reported if a signal below the
        // energy detection threshold occupies the first 20 MHz subchannel of the S80
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 20 MHz signal that occupies the first subchannel of "
                            "S80 below ED threshold");
        ScheduleTest(delay,
                     {{dBm_u{-65},
                       MicroSeconds(0),
                       MicroSeconds(100),
                       S80_CENTER_FREQUENCY - MHz_u{30},
                       MHz_u{20}}},
                     {},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {MicroSeconds(100) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {MicroSeconds(100) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE and CCA-BUSY indication is reported if a signal above the
        // energy detection threshold occupies the first 20 MHz subchannel of the S80 27.3.20.6.4:
        // Any signal within the secondary 80 MHz channel at or above –56 dBm.
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 20 MHz signal that occupies the first subchannel of "
                            "S80 above ED threshold");
        ScheduleTest(delay,
                     {{dBm_u{-55},
                       MicroSeconds(0),
                       MicroSeconds(100),
                       S80_CENTER_FREQUENCY - MHz_u{30},
                       MHz_u{20}}},
                     {},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {MicroSeconds(100) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {MicroSeconds(100) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {{MicroSeconds(100) - smallDelta,
                       MicroSeconds(100),
                       WIFI_CHANLIST_SECONDARY80,
                       std::vector<Time>{MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(100),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0)}}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE and no CCA-BUSY indication is reported if a signal below the
        // energy detection threshold occupies the second 20 MHz subchannel of the S80
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 20 MHz signal that occupies the second subchannel of "
                            "S80 below ED threshold");
        ScheduleTest(delay,
                     {{dBm_u{-65},
                       MicroSeconds(0),
                       MicroSeconds(100),
                       S80_CENTER_FREQUENCY - MHz_u{10},
                       MHz_u{20}}},
                     {},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {MicroSeconds(100) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {MicroSeconds(100) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE and CCA-BUSY indication is reported if a signal above the
        // energy detection threshold occupies the second 20 MHz subchannel of the S80 27.3.20.6.4:
        // Any signal within the secondary 80 MHz channel at or above –56 dBm.
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 20 MHz signal that occupies the second subchannel of "
                            "S80 above ED threshold");
        ScheduleTest(delay,
                     {{dBm_u{-55},
                       MicroSeconds(0),
                       MicroSeconds(100),
                       S80_CENTER_FREQUENCY - MHz_u{10},
                       MHz_u{20}}},
                     {},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {MicroSeconds(100) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {MicroSeconds(100) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {{MicroSeconds(100) - smallDelta,
                       MicroSeconds(100),
                       WIFI_CHANLIST_SECONDARY80,
                       std::vector<Time>{MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(100),
                                         MicroSeconds(0),
                                         MicroSeconds(0)}}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE and no CCA-BUSY indication is reported if a signal below the
        // energy detection threshold occupies the third 20 MHz subchannel of the S80
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 20 MHz signal that occupies the third subchannel of "
                            "S80 below ED threshold");
        ScheduleTest(delay,
                     {{dBm_u{-65},
                       MicroSeconds(0),
                       MicroSeconds(100),
                       S80_CENTER_FREQUENCY + MHz_u{10},
                       MHz_u{20}}},
                     {},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {MicroSeconds(100) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {MicroSeconds(100) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE and CCA-BUSY indication is reported if a signal above the
        // energy detection threshold occupies the third 20 MHz subchannel of the S80 27.3.20.6.4:
        // Any signal within the secondary 80 MHz channel at or above –56 dBm.
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 20 MHz signal that occupies the third subchannel of "
                            "S80 above ED threshold");
        ScheduleTest(delay,
                     {{dBm_u{-55},
                       MicroSeconds(0),
                       MicroSeconds(100),
                       S80_CENTER_FREQUENCY + MHz_u{10},
                       MHz_u{20}}},
                     {},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {MicroSeconds(100) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {MicroSeconds(100) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {{MicroSeconds(100) - smallDelta,
                       MicroSeconds(100),
                       WIFI_CHANLIST_SECONDARY80,
                       std::vector<Time>{MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(100),
                                         MicroSeconds(0)}}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE and no CCA-BUSY indication is reported if a signal below the
        // energy detection threshold occupies the fourth 20 MHz subchannel of the S80
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 20 MHz signal that occupies the fourth subchannel of "
                            "S80 below ED threshold");
        ScheduleTest(delay,
                     {{dBm_u{-65},
                       MicroSeconds(0),
                       MicroSeconds(100),
                       S80_CENTER_FREQUENCY + MHz_u{30},
                       MHz_u{20}}},
                     {},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {MicroSeconds(100) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {MicroSeconds(100) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE and CCA-BUSY indication is reported if a signal above the
        // energy detection threshold occupies the fourth 20 MHz subchannel of the S80 27.3.20.6.4:
        // Any signal within the secondary 80 MHz channel at or above –56 dBm.
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 20 MHz signal that occupies the fourth subchannel of "
                            "S80 above ED threshold");
        ScheduleTest(delay,
                     {{dBm_u{-55},
                       MicroSeconds(0),
                       MicroSeconds(100),
                       S80_CENTER_FREQUENCY + MHz_u{30},
                       MHz_u{20}}},
                     {},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {MicroSeconds(100) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {MicroSeconds(100) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {{MicroSeconds(100) - smallDelta,
                       MicroSeconds(100),
                       WIFI_CHANLIST_SECONDARY80,
                       std::vector<Time>{MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(100)}}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE and no CCA-BUSY indication is reported if a signal below the
        // energy detection threshold occupies the first and second 20 MHz subchannels of the S80
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 40 MHz signal that occupies the first and second "
                            "subchannels of S80 below ED threshold");
        ScheduleTest(delay,
                     {{dBm_u{-65},
                       MicroSeconds(0),
                       MicroSeconds(100),
                       S80_CENTER_FREQUENCY - MHz_u{20},
                       MHz_u{40}}},
                     {},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {MicroSeconds(100) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {MicroSeconds(100) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE and CCA-BUSY indication is reported if a signal above the
        // energy detection threshold occupies the first and second 20 MHz subchannels of the S80
        // 27.3.20.6.4: Any signal within the secondary 80 MHz channel at or above –56 dBm.
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 40 MHz signal that occupies the first and second "
                            "subchannels of S80 above ED threshold");
        ScheduleTest(delay,
                     {{dBm_u{-55},
                       MicroSeconds(0),
                       MicroSeconds(100),
                       S80_CENTER_FREQUENCY - MHz_u{20},
                       MHz_u{40}}},
                     {},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {MicroSeconds(100) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {MicroSeconds(100) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {{MicroSeconds(100) - smallDelta,
                       MicroSeconds(100),
                       WIFI_CHANLIST_SECONDARY80,
                       std::vector<Time>{MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(100),
                                         MicroSeconds(100),
                                         MicroSeconds(0),
                                         MicroSeconds(0)}}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE and no CCA-BUSY indication is reported if a signal below the
        // energy detection threshold occupies the third and fourth 20 MHz subchannels of the S80
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 40 MHz signal that occupies the third and fourth "
                            "subchannels of S80 below ED threshold");
        ScheduleTest(delay,
                     {{dBm_u{-65},
                       MicroSeconds(0),
                       MicroSeconds(100),
                       S80_CENTER_FREQUENCY + MHz_u{20},
                       MHz_u{40}}},
                     {},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {MicroSeconds(100) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {MicroSeconds(100) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE and CCA-BUSY indication is reported if a signal above the
        // energy detection threshold occupies the third and fourth 20 MHz subchannels of the S80
        // 27.3.20.6.4: Any signal within the secondary 80 MHz channel at or above –56 dBm.
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 40 MHz signal that occupies the third and fourth "
                            "subchannels of S80 above ED threshold");
        ScheduleTest(delay,
                     {{dBm_u{-55},
                       MicroSeconds(0),
                       MicroSeconds(100),
                       S80_CENTER_FREQUENCY + MHz_u{20},
                       MHz_u{40}}},
                     {},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {MicroSeconds(100) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {MicroSeconds(100) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {{MicroSeconds(100) - smallDelta,
                       MicroSeconds(100),
                       WIFI_CHANLIST_SECONDARY80,
                       std::vector<Time>{MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(100),
                                         MicroSeconds(100)}}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE and no CCA-BUSY indication is reported if a signal below the
        // energy detection threshold occupies the S80
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 80 MHz signal that occupies S80 below ED threshold");
        ScheduleTest(
            delay,
            {{dBm_u{-65}, MicroSeconds(0), MicroSeconds(100), S80_CENTER_FREQUENCY, MHz_u{80}}},
            {},
            {
                {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                {MicroSeconds(100) - smallDelta,
                 WifiPhyState::IDLE}, // IDLE just before the transmission ends
                {MicroSeconds(100) + smallDelta,
                 WifiPhyState::IDLE} // IDLE just after the transmission ends
            },
            {});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE and CCA-BUSY indication is reported if a signal above the
        // energy detection threshold occupies the S80 27.3.20.6.4: Any signal within the secondary
        // 80 MHz channel at or above –56 dBm.
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 80 MHz signal that occupies S80 above ED threshold");
        ScheduleTest(
            delay,
            {{dBm_u{-55}, MicroSeconds(0), MicroSeconds(100), S80_CENTER_FREQUENCY, MHz_u{80}}},
            {},
            {
                {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                {MicroSeconds(100) - smallDelta,
                 WifiPhyState::IDLE}, // IDLE just before the transmission ends
                {MicroSeconds(100) + smallDelta,
                 WifiPhyState::IDLE} // IDLE just after the transmission ends
            },
            {{MicroSeconds(100) - smallDelta,
              MicroSeconds(100),
              WIFI_CHANLIST_SECONDARY80,
              std::vector<Time>{MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(100),
                                MicroSeconds(100),
                                MicroSeconds(100),
                                MicroSeconds(100)}}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE as long as a 160 MHz signal below the energy detection
        // threshold occupies the whole band
        Simulator::Schedule(
            delay,
            &WifiPhyCcaIndicationTest::LogScenario,
            this,
            "Reception of a 160 MHz signal that occupies the whole band below ED threshold");
        ScheduleTest(
            delay,
            {{dBm_u{-55}, MicroSeconds(0), MicroSeconds(100), P160_CENTER_FREQUENCY, MHz_u{160}}},
            {},
            {
                {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                {MicroSeconds(100) - smallDelta,
                 WifiPhyState::IDLE}, // IDLE just before the transmission ends
                {MicroSeconds(100) + smallDelta,
                 WifiPhyState::IDLE} // IDLE just after the transmission ends
            },
            {});

        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state is CCA-BUSY as long as a 160 MHz signal above the energy detection
        // threshold occupies the whole band
        Simulator::Schedule(
            delay,
            &WifiPhyCcaIndicationTest::LogScenario,
            this,
            "Reception of a 160 MHz signal that occupies the whole band above ED threshold");
        ScheduleTest(
            delay,
            {{dBm_u{-50}, MicroSeconds(0), MicroSeconds(100), P160_CENTER_FREQUENCY, MHz_u{160}}},
            {},
            {
                {aCcaTime, WifiPhyState::CCA_BUSY}, // CCA-BUSY after aCCATime
                {MicroSeconds(100) - smallDelta,
                 WifiPhyState::CCA_BUSY}, // CCA-BUSY just before the transmission ends
                {MicroSeconds(100) + smallDelta,
                 WifiPhyState::IDLE} // IDLE just after the transmission ends
            },
            {{MicroSeconds(100) - smallDelta,
              MicroSeconds(100),
              WIFI_CHANLIST_PRIMARY,
              std::vector<Time>{MicroSeconds(100),
                                MicroSeconds(100),
                                MicroSeconds(100),
                                MicroSeconds(100),
                                MicroSeconds(100),
                                MicroSeconds(100),
                                MicroSeconds(100),
                                MicroSeconds(100)}}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY notifies CCA-BUSY for the P20 channel while the S80 channel was already in
        // CCA-BUSY state
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 20 MHz signal that occupies S80 followed by the "
                            "reception of another 20 MHz signal that occupies P20");
        ScheduleTest(
            delay,
            {{dBm_u{-55},
              MicroSeconds(0),
              MicroSeconds(100),
              S80_CENTER_FREQUENCY + MHz_u{10},
              MHz_u{20}},
             {dBm_u{-55}, MicroSeconds(50), MicroSeconds(100), P20_CENTER_FREQUENCY, MHz_u{20}}},
            {},
            {
                {aCcaTime, WifiPhyState::IDLE}, // state of primary stays idle after aCCATime
                {MicroSeconds(50) + aCcaTime,
                 WifiPhyState::CCA_BUSY}, // state of primary is CCA-BUSY after aCCATime that
                                          // followed the second transmission
                {MicroSeconds(50) + MicroSeconds(100) - smallDelta,
                 WifiPhyState::CCA_BUSY}, // CCA-BUSY just before the transmission ends
                {MicroSeconds(50) + MicroSeconds(100) + smallDelta,
                 WifiPhyState::IDLE} // IDLE just after the transmission ends
            },
            {{aCcaTime, // notification upon reception of the first signal
              MicroSeconds(100),
              WIFI_CHANLIST_SECONDARY80,
              std::vector<Time>{MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(100),
                                MicroSeconds(0)}},
             {MicroSeconds(50) + aCcaTime, // notification upon reception of the second signal
              MicroSeconds(50) + MicroSeconds(100),
              WIFI_CHANLIST_PRIMARY,
              std::vector<Time>{MicroSeconds(100),
                                MicroSeconds(0),
                                MicroSeconds(00),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(50),
                                MicroSeconds(0)}}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE but notifies CCA-BUSY for the S40 channel while the S80
        // channel was already in CCA-BUSY state
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a signal that occupies S80 followed by the reception of "
                            "another signal that occupies S40");
        ScheduleTest(
            delay,
            {{dBm_u{-55},
              MicroSeconds(0),
              MicroSeconds(100),
              S80_CENTER_FREQUENCY + MHz_u{30},
              MHz_u{20}},
             {dBm_u{-55},
              MicroSeconds(50),
              MicroSeconds(100),
              S40_CENTER_FREQUENCY - MHz_u{10},
              MHz_u{20}}},
            {},
            {
                {aCcaTime, WifiPhyState::IDLE}, // state of primary stays idle after aCCATime
                {MicroSeconds(50) + aCcaTime, WifiPhyState::IDLE}, // state of primary stays IDLE
                {MicroSeconds(50) + MicroSeconds(100) - smallDelta,
                 WifiPhyState::IDLE}, // IDLE just before the transmission ends
                {MicroSeconds(50) + MicroSeconds(100) + smallDelta,
                 WifiPhyState::IDLE} // IDLE just after the transmission ends
            },
            {{aCcaTime, // notification upon reception of the first signal
              MicroSeconds(100),
              WIFI_CHANLIST_SECONDARY80,
              std::vector<Time>{MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(100)}},
             {MicroSeconds(50) + aCcaTime, // notification upon reception of the second signal
              MicroSeconds(50) + MicroSeconds(100),
              WIFI_CHANLIST_SECONDARY40,
              std::vector<Time>{MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(100),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(50)}}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE but notifies CCA-BUSY for the S20 channel while the S80
        // channel was already in CCA-BUSY state
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a signal that occupies S80 followed by the reception of "
                            "another signal that occupies S20");
        ScheduleTest(
            delay,
            {{dBm_u{-55},
              MicroSeconds(0),
              MicroSeconds(100),
              S80_CENTER_FREQUENCY - MHz_u{30},
              MHz_u{20}},
             {dBm_u{-55}, MicroSeconds(50), MicroSeconds(100), S20_CENTER_FREQUENCY, MHz_u{20}}},
            {},
            {
                {aCcaTime, WifiPhyState::IDLE}, // state of primary stays idle after aCCATime
                {MicroSeconds(50) + aCcaTime, WifiPhyState::IDLE}, // state of primary stays IDLE
                {MicroSeconds(50) + MicroSeconds(100) - smallDelta,
                 WifiPhyState::IDLE}, // IDLE just before the transmission ends
                {MicroSeconds(50) + MicroSeconds(100) + smallDelta,
                 WifiPhyState::IDLE} // IDLE just after the transmission ends
            },
            {{aCcaTime, // notification upon reception of the first signal
              MicroSeconds(100),
              WIFI_CHANLIST_SECONDARY80,
              std::vector<Time>{MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(100),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(0)}},
             {MicroSeconds(50) + aCcaTime, // notification upon reception of the second signal
              MicroSeconds(50) + MicroSeconds(100),
              WIFI_CHANLIST_SECONDARY,
              std::vector<Time>{MicroSeconds(0),
                                MicroSeconds(100),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(50),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(0)}}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE when a 80 MHz HE SU PPDU with received power below the CCA
        // sensitivity threshold occupies S80
        Simulator::Schedule(
            delay,
            &WifiPhyCcaIndicationTest::LogScenario,
            this,
            "Reception of a 40 MHz HE PPDU that occupies S40 below CCA sensitivity threshold");
        ScheduleTest(delay,
                     {},
                     {{dBm_u{-70}, MicroSeconds(0), S80_CENTER_FREQUENCY, MHz_u{80}}},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {PpduDurations.at(MHz_u{20}) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {PpduDurations.at(MHz_u{20}) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE but CCA-BUSY indication is reported when a 80 MHz HE SU PPDU
        // with received power above the CCA sensitivity threshold occupies S80
        Simulator::Schedule(
            delay,
            &WifiPhyCcaIndicationTest::LogScenario,
            this,
            "Reception of a 80 MHz HE PPDU that occupies S80 above CCA sensitivity threshold");
        ScheduleTest(delay,
                     {},
                     {{dBm_u{-65}, MicroSeconds(0), S80_CENTER_FREQUENCY, MHz_u{80}}},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {PpduDurations.at(MHz_u{80}) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {PpduDurations.at(MHz_u{80}) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {{aCcaTime,
                       PpduDurations.at(MHz_u{80}),
                       WIFI_CHANLIST_SECONDARY80,
                       std::vector<Time>{NanoSeconds(0),
                                         NanoSeconds(0),
                                         NanoSeconds(0),
                                         NanoSeconds(0),
                                         PpduDurations.at(MHz_u{80}),
                                         PpduDurations.at(MHz_u{80}),
                                         PpduDurations.at(MHz_u{80}),
                                         PpduDurations.at(MHz_u{80})}}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays IDLE and CCA-BUSY indication is reported if only the per20bitmap
        // parameter changes
        Simulator::Schedule(delay,
                            &WifiPhyCcaIndicationTest::LogScenario,
                            this,
                            "Reception of a 20 MHz signal that generates a per20bitmap parameter "
                            "change when previous CCA indication reports IDLE");
        ScheduleTest(delay,
                     {{dBm_u{-60},
                       MicroSeconds(0),
                       MicroSeconds(100),
                       S80_CENTER_FREQUENCY + MHz_u{30},
                       MHz_u{20}}},
                     {},
                     {
                         {aCcaTime, WifiPhyState::IDLE}, // IDLE after aCCATime
                         {MicroSeconds(100) - smallDelta,
                          WifiPhyState::IDLE}, // IDLE just before the transmission ends
                         {MicroSeconds(100) + smallDelta,
                          WifiPhyState::IDLE} // IDLE just after the transmission ends
                     },
                     {{aCcaTime,
                       Seconds(0),
                       WIFI_CHANLIST_PRIMARY,
                       std::vector<Time>{MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(0),
                                         MicroSeconds(100)}}});
        delay += Seconds(1);

        //----------------------------------------------------------------------------------------------------------------------------------
        // Verify PHY state stays CCA_BUSY and CCA-BUSY indication is reported if only the
        // per20bitmap parameter changes
        Simulator::Schedule(
            delay,
            &WifiPhyCcaIndicationTest::LogScenario,
            this,
            "Reception of a 20 MHz signal that generates a per20bitmap parameter change when "
            "previous CCA indication reports BUSY for the primary channel");
        ScheduleTest(
            delay,
            {{dBm_u{-50.0}, MicroSeconds(0), MicroSeconds(100), P80_CENTER_FREQUENCY, MHz_u{80}},
             {dBm_u{-60.0},
              MicroSeconds(50),
              MicroSeconds(200),
              S80_CENTER_FREQUENCY + MHz_u{30},
              MHz_u{20}}},
            {},
            {
                {aCcaTime, WifiPhyState::CCA_BUSY}, // CCA-BUSY after aCCATime
                {MicroSeconds(100) - smallDelta,
                 WifiPhyState::CCA_BUSY}, // CCA-BUSY just before the transmission ends
                {MicroSeconds(100) + smallDelta,
                 WifiPhyState::IDLE} // IDLE just after the transmission ends
            },
            {{aCcaTime,
              MicroSeconds(100),
              WIFI_CHANLIST_PRIMARY,
              std::vector<Time>{MicroSeconds(100),
                                MicroSeconds(100),
                                MicroSeconds(100),
                                MicroSeconds(100),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(0)}},
             {MicroSeconds(50) + aCcaTime, // notification upon reception of the second signal
              MicroSeconds(100),
              WIFI_CHANLIST_PRIMARY,
              std::vector<Time>{MicroSeconds(50),
                                MicroSeconds(50),
                                MicroSeconds(50),
                                MicroSeconds(50),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(0),
                                MicroSeconds(200)}}});
        delay += Seconds(1);
    }

    Simulator::Run();
}

void
WifiPhyCcaIndicationTest::DoRun()
{
    m_frequency = MHz_u{5180};
    m_channelWidth = MHz_u{20};
    RunOne();

    m_frequency = MHz_u{5190};
    m_channelWidth = MHz_u{40};
    RunOne();

    m_frequency = MHz_u{5210};
    m_channelWidth = MHz_u{80};
    RunOne();

    m_frequency = MHz_u{5250};
    m_channelWidth = MHz_u{160};
    RunOne();

    Simulator::Destroy();
}

void
WifiPhyCcaIndicationTest::DoTeardown()
{
    m_rxPhy->Dispose();
    m_rxPhy = nullptr;
    m_txPhy->Dispose();
    m_txPhy = nullptr;
    for (auto& signalGenerator : m_signalGenerators)
    {
        signalGenerator->Dispose();
        signalGenerator = nullptr;
    }
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Wi-Fi PHY CCA Test Suite
 */
class WifiPhyCcaTestSuite : public TestSuite
{
  public:
    WifiPhyCcaTestSuite();
};

WifiPhyCcaTestSuite::WifiPhyCcaTestSuite()
    : TestSuite("wifi-phy-cca", Type::UNIT)
{
    AddTestCase(new WifiPhyCcaThresholdsTest, TestCase::Duration::QUICK);
    AddTestCase(new WifiPhyCcaIndicationTest, TestCase::Duration::QUICK);
}

static WifiPhyCcaTestSuite WifiPhyCcaTestSuite; ///< the test suite
