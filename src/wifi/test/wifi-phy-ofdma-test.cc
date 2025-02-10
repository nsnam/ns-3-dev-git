/*
 * Copyright (c) 2019 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/ap-wifi-mac.h"
#include "ns3/boolean.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/ctrl-headers.h"
#include "ns3/double.h"
#include "ns3/he-configuration.h"
#include "ns3/he-phy.h"
#include "ns3/he-ppdu.h"
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
#include "ns3/threshold-preamble-detection-model.h"
#include "ns3/txop.h"
#include "ns3/waveform-generator.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy-listener.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-spectrum-phy-interface.h"
#include "ns3/wifi-spectrum-signal-parameters.h"
#include "ns3/wifi-spectrum-value-helper.h"
#include "ns3/wifi-utils.h"

#include <algorithm>
#include <iterator>
#include <memory>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiPhyOfdmaTest");

static const uint8_t DEFAULT_CHANNEL_NUMBER = 36;
static const MHz_u DEFAULT_FREQUENCY{5180};
static const WifiPhyBand DEFAULT_WIFI_BAND = WIFI_PHY_BAND_5GHZ;
static const MHz_u DEFAULT_CHANNEL_WIDTH{20};
static const MHz_u DEFAULT_GUARD_WIDTH =
    DEFAULT_CHANNEL_WIDTH; // expanded to channel width to model spectrum mask

/**
 * HE PHY slightly modified so as to return a given
 * STA-ID in case of DL MU for OfdmaSpectrumWifiPhy.
 */
class OfdmaTestHePhy : public HePhy
{
  public:
    /**
     * Constructor
     *
     * @param staId the ID of the STA to which this PHY belongs to
     */
    OfdmaTestHePhy(uint16_t staId);
    ~OfdmaTestHePhy() override;

    /**
     * Return the STA ID that has been assigned to the station this PHY belongs to.
     * This is typically called for MU PPDUs, in order to pick the correct PSDU.
     *
     * @param ppdu the PPDU for which the STA ID is requested
     * @return the STA ID
     */
    uint16_t GetStaId(const Ptr<const WifiPpdu> ppdu) const override;

    /**
     * Set the global PPDU UID counter.
     *
     * @param uid the value to which the global PPDU UID counter should be set
     */
    void SetGlobalPpduUid(uint64_t uid);

  private:
    uint16_t m_staId; ///< ID of the STA to which this PHY belongs to

    // end of class OfdmaTestHePhy
};

OfdmaTestHePhy::OfdmaTestHePhy(uint16_t staId)
    : HePhy(),
      m_staId(staId)
{
}

OfdmaTestHePhy::~OfdmaTestHePhy()
{
}

uint16_t
OfdmaTestHePhy::GetStaId(const Ptr<const WifiPpdu> ppdu) const
{
    if (ppdu->GetType() == WIFI_PPDU_TYPE_DL_MU)
    {
        return m_staId;
    }
    return HePhy::GetStaId(ppdu);
}

void
OfdmaTestHePhy::SetGlobalPpduUid(uint64_t uid)
{
    m_globalPpduUid = uid;
}

/**
 * SpectrumWifiPhy used for testing OFDMA.
 */
class OfdmaSpectrumWifiPhy : public SpectrumWifiPhy
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    /**
     * Constructor
     *
     * @param staId the ID of the STA to which this PHY belongs to
     */
    OfdmaSpectrumWifiPhy(uint16_t staId);
    ~OfdmaSpectrumWifiPhy() override;

    void DoInitialize() override;
    void DoDispose() override;

    using WifiPhy::Reset;
    void StartTx(Ptr<const WifiPpdu> ppdu) override;

    /**
     * TracedCallback signature for UID of transmitted PPDU.
     *
     * @param uid the UID of the transmitted PPDU
     */
    typedef void (*TxPpduUidCallback)(uint64_t uid);

    /**
     * Set the global PPDU UID counter.
     *
     * @param uid the value to which the global PPDU UID counter should be set
     */
    void SetPpduUid(uint64_t uid);

    /**
     * Since we assume trigger frame was previously received from AP, this is used to set its UID
     *
     * @param uid the PPDU UID of the trigger frame
     */
    void SetTriggerFrameUid(uint64_t uid);

    /**
     * @return the current preamble events map
     */
    std::map<std::pair<uint64_t, WifiPreamble>, Ptr<Event>>& GetCurrentPreambleEvents();
    /**
     * @return the current event
     */
    Ptr<Event> GetCurrentEvent();

    /**
     * Wrapper to InterferenceHelper method.
     *
     * @param energy the minimum energy requested
     * @param band identify the requested band
     *
     * @returns the expected amount of time the observed
     *          energy on the medium for a given band will
     *          be higher than the requested threshold.
     */
    Time GetEnergyDuration(Watt_u energy, WifiSpectrumBandInfo band);

    /**
     * @return a const pointer to the HE PHY instance
     */
    Ptr<const HePhy> GetHePhy() const;

  private:
    /// Pointer to HE PHY instance used for OFDMA test
    Ptr<OfdmaTestHePhy> m_ofdmTestHePhy;

    /// Callback providing UID of the PPDU that is about to be transmitted
    TracedCallback<uint64_t> m_phyTxPpduUidTrace;

    // end of class OfdmaSpectrumWifiPhy
};

TypeId
OfdmaSpectrumWifiPhy::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::OfdmaSpectrumWifiPhy")
            .SetParent<SpectrumWifiPhy>()
            .SetGroupName("Wifi")
            .AddTraceSource("TxPpduUid",
                            "UID of the PPDU to be transmitted",
                            MakeTraceSourceAccessor(&OfdmaSpectrumWifiPhy::m_phyTxPpduUidTrace),
                            "ns3::OfdmaSpectrumWifiPhy::TxPpduUidCallback");
    return tid;
}

OfdmaSpectrumWifiPhy::OfdmaSpectrumWifiPhy(uint16_t staId)
    : SpectrumWifiPhy()
{
    m_ofdmTestHePhy = Create<OfdmaTestHePhy>(staId);
    m_ofdmTestHePhy->SetOwner(this);
}

OfdmaSpectrumWifiPhy::~OfdmaSpectrumWifiPhy()
{
}

void
OfdmaSpectrumWifiPhy::DoInitialize()
{
    // Replace HE PHY instance with test instance
    m_phyEntities[WIFI_MOD_CLASS_HE] = m_ofdmTestHePhy;
    SpectrumWifiPhy::DoInitialize();
}

void
OfdmaSpectrumWifiPhy::DoDispose()
{
    m_ofdmTestHePhy = nullptr;
    SpectrumWifiPhy::DoDispose();
}

void
OfdmaSpectrumWifiPhy::SetPpduUid(uint64_t uid)
{
    m_ofdmTestHePhy->SetGlobalPpduUid(uid);
    m_previouslyRxPpduUid = uid;
}

void
OfdmaSpectrumWifiPhy::SetTriggerFrameUid(uint64_t uid)
{
    m_previouslyRxPpduUid = uid;
}

void
OfdmaSpectrumWifiPhy::StartTx(Ptr<const WifiPpdu> ppdu)
{
    m_phyTxPpduUidTrace(ppdu->GetUid());
    SpectrumWifiPhy::StartTx(ppdu);
}

std::map<std::pair<uint64_t, WifiPreamble>, Ptr<Event>>&
OfdmaSpectrumWifiPhy::GetCurrentPreambleEvents()
{
    return m_currentPreambleEvents;
}

Ptr<Event>
OfdmaSpectrumWifiPhy::GetCurrentEvent()
{
    return m_currentEvent;
}

Time
OfdmaSpectrumWifiPhy::GetEnergyDuration(Watt_u energy, WifiSpectrumBandInfo band)
{
    return m_interference->GetEnergyDuration(energy, band);
}

Ptr<const HePhy>
OfdmaSpectrumWifiPhy::GetHePhy() const
{
    return DynamicCast<const HePhy>(GetLatestPhyEntity());
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief DL-OFDMA PHY test
 */
class TestDlOfdmaPhyTransmission : public TestCase
{
  public:
    TestDlOfdmaPhyTransmission();
    ~TestDlOfdmaPhyTransmission() override;

  private:
    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     * Receive success function for STA 1
     * @param psdu the PSDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector the transmit vector
     * @param statusPerMpdu reception status per MPDU
     */
    void RxSuccessSta1(Ptr<const WifiPsdu> psdu,
                       RxSignalInfo rxSignalInfo,
                       const WifiTxVector& txVector,
                       const std::vector<bool>& statusPerMpdu);
    /**
     * Receive success function for STA 2
     * @param psdu the PSDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector the transmit vector
     * @param statusPerMpdu reception status per MPDU
     */
    void RxSuccessSta2(Ptr<const WifiPsdu> psdu,
                       RxSignalInfo rxSignalInfo,
                       const WifiTxVector& txVector,
                       const std::vector<bool>& statusPerMpdu);
    /**
     * Receive success function for STA 3
     * @param psdu the PSDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector the transmit vector
     * @param statusPerMpdu reception status per MPDU
     */
    void RxSuccessSta3(Ptr<const WifiPsdu> psdu,
                       RxSignalInfo rxSignalInfo,
                       const WifiTxVector& txVector,
                       const std::vector<bool>& statusPerMpdu);

    /**
     * Receive failure function for STA 1
     * @param psdu the PSDU
     */
    void RxFailureSta1(Ptr<const WifiPsdu> psdu);
    /**
     * Receive failure function for STA 2
     * @param psdu the PSDU
     */
    void RxFailureSta2(Ptr<const WifiPsdu> psdu);
    /**
     * Receive failure function for STA 3
     * @param psdu the PSDU
     */
    void RxFailureSta3(Ptr<const WifiPsdu> psdu);

    /**
     * Check the results for STA 1
     * @param expectedRxSuccess the expected number of RX success
     * @param expectedRxFailure the expected number of RX failures
     * @param expectedRxBytes the expected number of RX bytes
     */
    void CheckResultsSta1(uint32_t expectedRxSuccess,
                          uint32_t expectedRxFailure,
                          uint32_t expectedRxBytes);
    /**
     * Check the results for STA 2
     * @param expectedRxSuccess the expected number of RX success
     * @param expectedRxFailure the expected number of RX failures
     * @param expectedRxBytes the expected number of RX bytes
     */
    void CheckResultsSta2(uint32_t expectedRxSuccess,
                          uint32_t expectedRxFailure,
                          uint32_t expectedRxBytes);
    /**
     * Check the results for STA 3
     * @param expectedRxSuccess the expected number of RX success
     * @param expectedRxFailure the expected number of RX failures
     * @param expectedRxBytes the expected number of RX bytes
     */
    void CheckResultsSta3(uint32_t expectedRxSuccess,
                          uint32_t expectedRxFailure,
                          uint32_t expectedRxBytes);

    /**
     * Reset the results
     */
    void ResetResults();

    /**
     * Send MU-PPDU function
     * @param rxStaId1 the ID of the recipient STA for the first PSDU
     * @param rxStaId2 the ID of the recipient STA for the second PSDU
     */
    void SendMuPpdu(uint16_t rxStaId1, uint16_t rxStaId2);

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
     * Run one function
     */
    void RunOne();

    /**
     * Schedule now to check  the PHY state
     * @param phy the PHY
     * @param expectedState the expected state of the PHY
     */
    void CheckPhyState(Ptr<OfdmaSpectrumWifiPhy> phy, WifiPhyState expectedState);
    /**
     * Check the PHY state now
     * @param phy the PHY
     * @param expectedState the expected state of the PHY
     */
    void DoCheckPhyState(Ptr<OfdmaSpectrumWifiPhy> phy, WifiPhyState expectedState);

    uint32_t m_countRxSuccessSta1; ///< count RX success for STA 1
    uint32_t m_countRxSuccessSta2; ///< count RX success for STA 2
    uint32_t m_countRxSuccessSta3; ///< count RX success for STA 3
    uint32_t m_countRxFailureSta1; ///< count RX failure for STA 1
    uint32_t m_countRxFailureSta2; ///< count RX failure for STA 2
    uint32_t m_countRxFailureSta3; ///< count RX failure for STA 3
    uint32_t m_countRxBytesSta1;   ///< count RX bytes for STA 1
    uint32_t m_countRxBytesSta2;   ///< count RX bytes for STA 2
    uint32_t m_countRxBytesSta3;   ///< count RX bytes for STA 3

    Ptr<SpectrumWifiPhy> m_phyAp;           ///< PHY of AP
    Ptr<OfdmaSpectrumWifiPhy> m_phySta1;    ///< PHY of STA 1
    Ptr<OfdmaSpectrumWifiPhy> m_phySta2;    ///< PHY of STA 2
    Ptr<OfdmaSpectrumWifiPhy> m_phySta3;    ///< PHY of STA 3
    Ptr<WaveformGenerator> m_phyInterferer; ///< PHY of interferer

    MHz_u m_frequency;           ///< frequency
    MHz_u m_channelWidth;        ///< channel width
    Time m_expectedPpduDuration; ///< expected duration to send MU PPDU
};

TestDlOfdmaPhyTransmission::TestDlOfdmaPhyTransmission()
    : TestCase("DL-OFDMA PHY test"),
      m_countRxSuccessSta1(0),
      m_countRxSuccessSta2(0),
      m_countRxSuccessSta3(0),
      m_countRxFailureSta1(0),
      m_countRxFailureSta2(0),
      m_countRxFailureSta3(0),
      m_countRxBytesSta1(0),
      m_countRxBytesSta2(0),
      m_countRxBytesSta3(0),
      m_frequency(DEFAULT_FREQUENCY),
      m_channelWidth(DEFAULT_CHANNEL_WIDTH),
      m_expectedPpduDuration(NanoSeconds(306400))
{
}

void
TestDlOfdmaPhyTransmission::ResetResults()
{
    m_countRxSuccessSta1 = 0;
    m_countRxSuccessSta2 = 0;
    m_countRxSuccessSta3 = 0;
    m_countRxFailureSta1 = 0;
    m_countRxFailureSta2 = 0;
    m_countRxFailureSta3 = 0;
    m_countRxBytesSta1 = 0;
    m_countRxBytesSta2 = 0;
    m_countRxBytesSta3 = 0;
}

void
TestDlOfdmaPhyTransmission::SendMuPpdu(uint16_t rxStaId1, uint16_t rxStaId2)
{
    NS_LOG_FUNCTION(this << rxStaId1 << rxStaId2);
    WifiConstPsduMap psdus;
    WifiTxVector txVector{HePhy::GetHeMcs7(),
                          0,
                          WIFI_PREAMBLE_HE_MU,
                          NanoSeconds(800),
                          1,
                          1,
                          0,
                          m_channelWidth,
                          false,
                          false};
    HeRu::RuType ruType = HeRu::RU_106_TONE;
    if (m_channelWidth == MHz_u{20})
    {
        ruType = HeRu::RU_106_TONE;
        txVector.SetRuAllocation({96}, 0);
    }
    else if (m_channelWidth == MHz_u{40})
    {
        ruType = HeRu::RU_242_TONE;
        txVector.SetRuAllocation({192, 192}, 0);
    }
    else if (m_channelWidth == MHz_u{80})
    {
        ruType = HeRu::RU_484_TONE;
        txVector.SetRuAllocation({200, 200, 200, 200}, 0);
    }
    else if (m_channelWidth == MHz_u{160})
    {
        ruType = HeRu::RU_996_TONE;
        txVector.SetRuAllocation({208, 208, 208, 208, 208, 208, 208, 208}, 0);
    }
    else
    {
        NS_ASSERT_MSG(false, "Unsupported channel width");
    }

    txVector.SetSigBMode(VhtPhy::GetVhtMcs5());

    HeRu::RuSpec ru1(ruType, 1, true);
    txVector.SetRu(ru1, rxStaId1);
    txVector.SetMode(HePhy::GetHeMcs7(), rxStaId1);
    txVector.SetNss(1, rxStaId1);

    HeRu::RuSpec ru2(ruType, (m_channelWidth == MHz_u{160} ? 1 : 2), m_channelWidth != MHz_u{160});
    txVector.SetRu(ru2, rxStaId2);
    txVector.SetMode(HePhy::GetHeMcs9(), rxStaId2);
    txVector.SetNss(1, rxStaId2);

    Ptr<Packet> pkt1 = Create<Packet>(1000);
    WifiMacHeader hdr1;
    hdr1.SetType(WIFI_MAC_QOSDATA);
    hdr1.SetQosTid(0);
    hdr1.SetAddr1(Mac48Address("00:00:00:00:00:01"));
    hdr1.SetSequenceNumber(1);
    Ptr<WifiPsdu> psdu1 = Create<WifiPsdu>(pkt1, hdr1);
    psdus.insert(std::make_pair(rxStaId1, psdu1));

    Ptr<Packet> pkt2 = Create<Packet>(1500);
    WifiMacHeader hdr2;
    hdr2.SetType(WIFI_MAC_QOSDATA);
    hdr2.SetQosTid(0);
    hdr2.SetAddr1(Mac48Address("00:00:00:00:00:02"));
    hdr2.SetSequenceNumber(2);
    Ptr<WifiPsdu> psdu2 = Create<WifiPsdu>(pkt2, hdr2);
    psdus.insert(std::make_pair(rxStaId2, psdu2));

    m_phyAp->Send(psdus, txVector);
}

void
TestDlOfdmaPhyTransmission::GenerateInterference(Ptr<SpectrumValue> interferencePsd, Time duration)
{
    m_phyInterferer->SetTxPowerSpectralDensity(interferencePsd);
    m_phyInterferer->SetPeriod(duration);
    m_phyInterferer->Start();
    Simulator::Schedule(duration, &TestDlOfdmaPhyTransmission::StopInterference, this);
}

void
TestDlOfdmaPhyTransmission::StopInterference()
{
    m_phyInterferer->Stop();
}

TestDlOfdmaPhyTransmission::~TestDlOfdmaPhyTransmission()
{
}

void
TestDlOfdmaPhyTransmission::RxSuccessSta1(Ptr<const WifiPsdu> psdu,
                                          RxSignalInfo rxSignalInfo,
                                          const WifiTxVector& txVector,
                                          const std::vector<bool>& /*statusPerMpdu*/)
{
    NS_LOG_FUNCTION(this << *psdu << rxSignalInfo << txVector);
    m_countRxSuccessSta1++;
    m_countRxBytesSta1 += (psdu->GetSize() - 30);
}

void
TestDlOfdmaPhyTransmission::RxSuccessSta2(Ptr<const WifiPsdu> psdu,
                                          RxSignalInfo rxSignalInfo,
                                          const WifiTxVector& txVector,
                                          const std::vector<bool>& /*statusPerMpdu*/)
{
    NS_LOG_FUNCTION(this << *psdu << rxSignalInfo << txVector);
    m_countRxSuccessSta2++;
    m_countRxBytesSta2 += (psdu->GetSize() - 30);
}

void
TestDlOfdmaPhyTransmission::RxSuccessSta3(Ptr<const WifiPsdu> psdu,
                                          RxSignalInfo rxSignalInfo,
                                          const WifiTxVector& txVector,
                                          const std::vector<bool>& /*statusPerMpdu*/)
{
    NS_LOG_FUNCTION(this << *psdu << rxSignalInfo << txVector);
    m_countRxSuccessSta3++;
    m_countRxBytesSta3 += (psdu->GetSize() - 30);
}

void
TestDlOfdmaPhyTransmission::RxFailureSta1(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << *psdu);
    m_countRxFailureSta1++;
}

void
TestDlOfdmaPhyTransmission::RxFailureSta2(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << *psdu);
    m_countRxFailureSta2++;
}

void
TestDlOfdmaPhyTransmission::RxFailureSta3(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << *psdu);
    m_countRxFailureSta3++;
}

void
TestDlOfdmaPhyTransmission::CheckResultsSta1(uint32_t expectedRxSuccess,
                                             uint32_t expectedRxFailure,
                                             uint32_t expectedRxBytes)
{
    NS_TEST_ASSERT_MSG_EQ(m_countRxSuccessSta1,
                          expectedRxSuccess,
                          "The number of successfully received packets by STA 1 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxFailureSta1,
                          expectedRxFailure,
                          "The number of unsuccessfuly received packets by STA 1 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxBytesSta1,
                          expectedRxBytes,
                          "The number of bytes received by STA 1 is not correct!");
}

void
TestDlOfdmaPhyTransmission::CheckResultsSta2(uint32_t expectedRxSuccess,
                                             uint32_t expectedRxFailure,
                                             uint32_t expectedRxBytes)
{
    NS_TEST_ASSERT_MSG_EQ(m_countRxSuccessSta2,
                          expectedRxSuccess,
                          "The number of successfully received packets by STA 2 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxFailureSta2,
                          expectedRxFailure,
                          "The number of unsuccessfuly received packets by STA 2 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxBytesSta2,
                          expectedRxBytes,
                          "The number of bytes received by STA 2 is not correct!");
}

void
TestDlOfdmaPhyTransmission::CheckResultsSta3(uint32_t expectedRxSuccess,
                                             uint32_t expectedRxFailure,
                                             uint32_t expectedRxBytes)
{
    NS_TEST_ASSERT_MSG_EQ(m_countRxSuccessSta3,
                          expectedRxSuccess,
                          "The number of successfully received packets by STA 3 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxFailureSta3,
                          expectedRxFailure,
                          "The number of unsuccessfuly received packets by STA 3 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxBytesSta3,
                          expectedRxBytes,
                          "The number of bytes received by STA 3 is not correct!");
}

void
TestDlOfdmaPhyTransmission::CheckPhyState(Ptr<OfdmaSpectrumWifiPhy> phy, WifiPhyState expectedState)
{
    // This is needed to make sure PHY state will be checked as the last event if a state change
    // occurred at the exact same time as the check
    Simulator::ScheduleNow(&TestDlOfdmaPhyTransmission::DoCheckPhyState, this, phy, expectedState);
}

void
TestDlOfdmaPhyTransmission::DoCheckPhyState(Ptr<OfdmaSpectrumWifiPhy> phy,
                                            WifiPhyState expectedState)
{
    WifiPhyState currentState;
    PointerValue ptr;
    phy->GetAttribute("State", ptr);
    Ptr<WifiPhyStateHelper> state = DynamicCast<WifiPhyStateHelper>(ptr.Get<WifiPhyStateHelper>());
    currentState = state->GetState();
    NS_LOG_FUNCTION(this << currentState);
    NS_TEST_ASSERT_MSG_EQ(currentState,
                          expectedState,
                          "PHY State " << currentState << " does not match expected state "
                                       << expectedState << " at " << Simulator::Now());
}

void
TestDlOfdmaPhyTransmission::DoSetup()
{
    Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel>();
    lossModel->SetFrequency(MHzToHz(m_frequency));
    spectrumChannel->AddPropagationLossModel(lossModel);
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);

    Ptr<Node> apNode = CreateObject<Node>();
    Ptr<WifiNetDevice> apDev = CreateObject<WifiNetDevice>();
    m_phyAp = CreateObject<SpectrumWifiPhy>();
    Ptr<InterferenceHelper> apInterferenceHelper = CreateObject<InterferenceHelper>();
    m_phyAp->SetInterferenceHelper(apInterferenceHelper);
    Ptr<ErrorRateModel> apErrorModel = CreateObject<NistErrorRateModel>();
    m_phyAp->SetErrorRateModel(apErrorModel);
    m_phyAp->SetDevice(apDev);
    m_phyAp->AddChannel(spectrumChannel);
    m_phyAp->ConfigureStandard(WIFI_STANDARD_80211ax);
    Ptr<ConstantPositionMobilityModel> apMobility = CreateObject<ConstantPositionMobilityModel>();
    m_phyAp->SetMobility(apMobility);
    apDev->SetPhy(m_phyAp);
    apNode->AggregateObject(apMobility);
    apNode->AddDevice(apDev);

    Ptr<Node> sta1Node = CreateObject<Node>();
    Ptr<WifiNetDevice> sta1Dev = CreateObject<WifiNetDevice>();
    m_phySta1 = CreateObject<OfdmaSpectrumWifiPhy>(1);
    Ptr<InterferenceHelper> sta1InterferenceHelper = CreateObject<InterferenceHelper>();
    m_phySta1->SetInterferenceHelper(sta1InterferenceHelper);
    Ptr<ErrorRateModel> sta1ErrorModel = CreateObject<NistErrorRateModel>();
    m_phySta1->SetErrorRateModel(sta1ErrorModel);
    m_phySta1->SetDevice(sta1Dev);
    m_phySta1->AddChannel(spectrumChannel);
    m_phySta1->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phySta1->SetReceiveOkCallback(MakeCallback(&TestDlOfdmaPhyTransmission::RxSuccessSta1, this));
    m_phySta1->SetReceiveErrorCallback(
        MakeCallback(&TestDlOfdmaPhyTransmission::RxFailureSta1, this));
    Ptr<ConstantPositionMobilityModel> sta1Mobility = CreateObject<ConstantPositionMobilityModel>();
    m_phySta1->SetMobility(sta1Mobility);
    sta1Dev->SetPhy(m_phySta1);
    sta1Node->AggregateObject(sta1Mobility);
    sta1Node->AddDevice(sta1Dev);

    Ptr<Node> sta2Node = CreateObject<Node>();
    Ptr<WifiNetDevice> sta2Dev = CreateObject<WifiNetDevice>();
    m_phySta2 = CreateObject<OfdmaSpectrumWifiPhy>(2);
    Ptr<InterferenceHelper> sta2InterferenceHelper = CreateObject<InterferenceHelper>();
    m_phySta2->SetInterferenceHelper(sta2InterferenceHelper);
    Ptr<ErrorRateModel> sta2ErrorModel = CreateObject<NistErrorRateModel>();
    m_phySta2->SetErrorRateModel(sta2ErrorModel);
    m_phySta2->SetDevice(sta2Dev);
    m_phySta2->AddChannel(spectrumChannel);
    m_phySta2->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phySta2->SetReceiveOkCallback(MakeCallback(&TestDlOfdmaPhyTransmission::RxSuccessSta2, this));
    m_phySta2->SetReceiveErrorCallback(
        MakeCallback(&TestDlOfdmaPhyTransmission::RxFailureSta2, this));
    Ptr<ConstantPositionMobilityModel> sta2Mobility = CreateObject<ConstantPositionMobilityModel>();
    m_phySta2->SetMobility(sta2Mobility);
    sta2Dev->SetPhy(m_phySta2);
    sta2Node->AggregateObject(sta2Mobility);
    sta2Node->AddDevice(sta2Dev);

    Ptr<Node> sta3Node = CreateObject<Node>();
    Ptr<WifiNetDevice> sta3Dev = CreateObject<WifiNetDevice>();
    m_phySta3 = CreateObject<OfdmaSpectrumWifiPhy>(3);
    Ptr<InterferenceHelper> sta3InterferenceHelper = CreateObject<InterferenceHelper>();
    m_phySta3->SetInterferenceHelper(sta3InterferenceHelper);
    Ptr<ErrorRateModel> sta3ErrorModel = CreateObject<NistErrorRateModel>();
    m_phySta3->SetErrorRateModel(sta3ErrorModel);
    m_phySta3->SetDevice(sta3Dev);
    m_phySta3->AddChannel(spectrumChannel);
    m_phySta3->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phySta3->SetReceiveOkCallback(MakeCallback(&TestDlOfdmaPhyTransmission::RxSuccessSta3, this));
    m_phySta3->SetReceiveErrorCallback(
        MakeCallback(&TestDlOfdmaPhyTransmission::RxFailureSta3, this));
    Ptr<ConstantPositionMobilityModel> sta3Mobility = CreateObject<ConstantPositionMobilityModel>();
    m_phySta3->SetMobility(sta3Mobility);
    sta3Dev->SetPhy(m_phySta3);
    sta3Node->AggregateObject(sta3Mobility);
    sta3Node->AddDevice(sta3Dev);

    Ptr<Node> interfererNode = CreateObject<Node>();
    Ptr<NonCommunicatingNetDevice> interfererDev = CreateObject<NonCommunicatingNetDevice>();
    m_phyInterferer = CreateObject<WaveformGenerator>();
    m_phyInterferer->SetDevice(interfererDev);
    m_phyInterferer->SetChannel(spectrumChannel);
    m_phyInterferer->SetDutyCycle(1);
    interfererNode->AddDevice(interfererDev);
}

void
TestDlOfdmaPhyTransmission::DoTeardown()
{
    m_phyAp->Dispose();
    m_phyAp = nullptr;
    m_phySta1->Dispose();
    m_phySta1 = nullptr;
    m_phySta2->Dispose();
    m_phySta2 = nullptr;
    m_phySta3->Dispose();
    m_phySta3 = nullptr;
    m_phyInterferer->Dispose();
    m_phyInterferer = nullptr;
}

void
TestDlOfdmaPhyTransmission::RunOne()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 0;
    m_phyAp->AssignStreams(streamNumber);
    m_phySta1->AssignStreams(streamNumber);
    m_phySta2->AssignStreams(streamNumber);
    m_phySta3->AssignStreams(streamNumber);

    auto channelNum = WifiPhyOperatingChannel::FindFirst(0,
                                                         m_frequency,
                                                         m_channelWidth,
                                                         WIFI_STANDARD_80211ax,
                                                         WIFI_PHY_BAND_5GHZ)
                          ->number;

    m_phyAp->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, m_channelWidth, WIFI_PHY_BAND_5GHZ, 0});
    m_phySta1->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, m_channelWidth, WIFI_PHY_BAND_5GHZ, 0});
    m_phySta2->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, m_channelWidth, WIFI_PHY_BAND_5GHZ, 0});
    m_phySta3->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, m_channelWidth, WIFI_PHY_BAND_5GHZ, 0});

    Simulator::Schedule(Seconds(0.5), &TestDlOfdmaPhyTransmission::ResetResults, this);

    // Send MU PPDU with two PSDUs addressed to STA 1 and STA 2:
    // Each STA should receive its PSDU.
    Simulator::Schedule(Seconds(1), &TestDlOfdmaPhyTransmission::SendMuPpdu, this, 1, 2);

    // Since it takes m_expectedPpduDuration to transmit the PPDU,
    // all 3 PHYs should be back to IDLE at the same time,
    // even the PHY that has no PSDU addressed to it.
    Simulator::Schedule(Seconds(1) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta1,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(1) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta2,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(1) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta3,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(1) + m_expectedPpduDuration,
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta1,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(1) + m_expectedPpduDuration,
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta2,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(1) + m_expectedPpduDuration,
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta3,
                        WifiPhyState::IDLE);

    // One PSDU of 1000 bytes should have been successfully received by STA 1
    Simulator::Schedule(Seconds(1.1),
                        &TestDlOfdmaPhyTransmission::CheckResultsSta1,
                        this,
                        1,
                        0,
                        1000);
    // One PSDU of 1500 bytes should have been successfully received by STA 2
    Simulator::Schedule(Seconds(1.1),
                        &TestDlOfdmaPhyTransmission::CheckResultsSta2,
                        this,
                        1,
                        0,
                        1500);
    // No PSDU should have been received by STA 3
    Simulator::Schedule(Seconds(1.1), &TestDlOfdmaPhyTransmission::CheckResultsSta3, this, 0, 0, 0);

    Simulator::Schedule(Seconds(1.5), &TestDlOfdmaPhyTransmission::ResetResults, this);

    // Send MU PPDU with two PSDUs addressed to STA 1 and STA 3:
    // STA 1 should receive its PSDU, whereas STA 2 should not receive any PSDU
    // but should keep its PHY busy during all PPDU duration.
    Simulator::Schedule(Seconds(2), &TestDlOfdmaPhyTransmission::SendMuPpdu, this, 1, 3);

    // Since it takes m_expectedPpduDuration to transmit the PPDU,
    // all 3 PHYs should be back to IDLE at the same time,
    // even the PHY that has no PSDU addressed to it.
    Simulator::Schedule(Seconds(2) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta1,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(2) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta2,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(2) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta3,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(2) + m_expectedPpduDuration,
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta1,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(2) + m_expectedPpduDuration,
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta2,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(2) + m_expectedPpduDuration,
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta3,
                        WifiPhyState::IDLE);

    // One PSDU of 1000 bytes should have been successfully received by STA 1
    Simulator::Schedule(Seconds(2.1),
                        &TestDlOfdmaPhyTransmission::CheckResultsSta1,
                        this,
                        1,
                        0,
                        1000);
    // No PSDU should have been received by STA 2
    Simulator::Schedule(Seconds(2.1), &TestDlOfdmaPhyTransmission::CheckResultsSta2, this, 0, 0, 0);
    // One PSDU of 1500 bytes should have been successfully received by STA 3
    Simulator::Schedule(Seconds(2.1),
                        &TestDlOfdmaPhyTransmission::CheckResultsSta3,
                        this,
                        1,
                        0,
                        1500);

    Simulator::Schedule(Seconds(2.5), &TestDlOfdmaPhyTransmission::ResetResults, this);

    // Send MU PPDU with two PSDUs addressed to STA 1 and STA 2:
    Simulator::Schedule(Seconds(3), &TestDlOfdmaPhyTransmission::SendMuPpdu, this, 1, 2);

    // A strong non-wifi interference is generated on RU 1 during PSDU reception
    BandInfo bandInfo;
    bandInfo.fc = MHzToHz(m_frequency - (m_channelWidth / 4));
    bandInfo.fl = bandInfo.fc - MHzToHz(m_channelWidth / 4);
    bandInfo.fh = bandInfo.fc + MHzToHz(m_channelWidth / 4);
    Bands bands;
    bands.push_back(bandInfo);

    auto SpectrumInterferenceRu1 = Create<SpectrumModel>(bands);
    auto interferencePsdRu1 = Create<SpectrumValue>(SpectrumInterferenceRu1);
    Watt_u interferencePower{0.1};
    *interferencePsdRu1 = interferencePower / (MHzToHz(m_channelWidth / 2) * 20);

    Simulator::Schedule(Seconds(3) + MicroSeconds(50),
                        &TestDlOfdmaPhyTransmission::GenerateInterference,
                        this,
                        interferencePsdRu1,
                        MilliSeconds(100));

    // Since it takes m_expectedPpduDuration to transmit the PPDU,
    // both PHYs should be back to CCA_BUSY (due to the interference) at the same time,
    // even the PHY that has no PSDU addressed to it.
    Simulator::Schedule(Seconds(3) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta1,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(3) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta2,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(3) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta3,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(3) + m_expectedPpduDuration,
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta1,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(3) + m_expectedPpduDuration,
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta2,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(3) + m_expectedPpduDuration,
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta3,
                        WifiPhyState::CCA_BUSY);

    // One PSDU of 1000 bytes should have been unsuccessfuly received by STA 1 (since interference
    // occupies RU 1)
    Simulator::Schedule(Seconds(3.1), &TestDlOfdmaPhyTransmission::CheckResultsSta1, this, 0, 1, 0);
    // One PSDU of 1500 bytes should have been successfully received by STA 2
    Simulator::Schedule(Seconds(3.1),
                        &TestDlOfdmaPhyTransmission::CheckResultsSta2,
                        this,
                        1,
                        0,
                        1500);
    // No PSDU should have been received by STA3
    Simulator::Schedule(Seconds(3.1), &TestDlOfdmaPhyTransmission::CheckResultsSta3, this, 0, 0, 0);

    Simulator::Schedule(Seconds(3.5), &TestDlOfdmaPhyTransmission::ResetResults, this);

    // Send MU PPDU with two PSDUs addressed to STA 1 and STA 2:
    Simulator::Schedule(Seconds(4), &TestDlOfdmaPhyTransmission::SendMuPpdu, this, 1, 2);

    // A strong non-wifi interference is generated on RU 2 during PSDU reception
    bandInfo.fc = MHzToHz(m_frequency + (m_channelWidth / 4));
    bandInfo.fl = bandInfo.fc - MHzToHz(m_channelWidth / 4);
    bandInfo.fh = bandInfo.fc + MHzToHz(m_channelWidth / 4);
    bands.clear();
    bands.push_back(bandInfo);

    Ptr<SpectrumModel> SpectrumInterferenceRu2 = Create<SpectrumModel>(bands);
    Ptr<SpectrumValue> interferencePsdRu2 = Create<SpectrumValue>(SpectrumInterferenceRu2);
    *interferencePsdRu2 = interferencePower / (MHzToHz(m_channelWidth / 2) * 20);

    Simulator::Schedule(Seconds(4) + MicroSeconds(50),
                        &TestDlOfdmaPhyTransmission::GenerateInterference,
                        this,
                        interferencePsdRu2,
                        MilliSeconds(100));

    // Since it takes m_expectedPpduDuration to transmit the PPDU,
    // both PHYs should be back to IDLE (or CCA_BUSY if interference on the primary 20 MHz) at the
    // same time, even the PHY that has no PSDU addressed to it.
    Simulator::Schedule(Seconds(4) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta1,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(4) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta2,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(4) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta3,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(4) + m_expectedPpduDuration,
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta1,
                        (m_channelWidth >= MHz_u{40}) ? WifiPhyState::IDLE
                                                      : WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(4) + m_expectedPpduDuration,
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta2,
                        (m_channelWidth >= MHz_u{40}) ? WifiPhyState::IDLE
                                                      : WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(4) + m_expectedPpduDuration,
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta3,
                        (m_channelWidth >= MHz_u{40}) ? WifiPhyState::IDLE
                                                      : WifiPhyState::CCA_BUSY);

    // One PSDU of 1000 bytes should have been successfully received by STA 1
    Simulator::Schedule(Seconds(4.1),
                        &TestDlOfdmaPhyTransmission::CheckResultsSta1,
                        this,
                        1,
                        0,
                        1000);
    // One PSDU of 1500 bytes should have been unsuccessfuly received by STA 2 (since interference
    // occupies RU 2)
    Simulator::Schedule(Seconds(4.1), &TestDlOfdmaPhyTransmission::CheckResultsSta2, this, 0, 1, 0);
    // No PSDU should have been received by STA3
    Simulator::Schedule(Seconds(4.1), &TestDlOfdmaPhyTransmission::CheckResultsSta3, this, 0, 0, 0);

    Simulator::Schedule(Seconds(4.5), &TestDlOfdmaPhyTransmission::ResetResults, this);

    // Send MU PPDU with two PSDUs addressed to STA 1 and STA 2:
    Simulator::Schedule(Seconds(5), &TestDlOfdmaPhyTransmission::SendMuPpdu, this, 1, 2);

    // A strong non-wifi interference is generated on the full band during PSDU reception
    bandInfo.fc = MHzToHz(m_frequency);
    bandInfo.fl = bandInfo.fc - MHzToHz(m_channelWidth / 2);
    bandInfo.fh = bandInfo.fc + MHzToHz(m_channelWidth / 2);
    bands.clear();
    bands.push_back(bandInfo);

    Ptr<SpectrumModel> SpectrumInterferenceAll = Create<SpectrumModel>(bands);
    Ptr<SpectrumValue> interferencePsdAll = Create<SpectrumValue>(SpectrumInterferenceAll);
    *interferencePsdAll = interferencePower / (MHzToHz(m_channelWidth) * 20);

    Simulator::Schedule(Seconds(5) + MicroSeconds(50),
                        &TestDlOfdmaPhyTransmission::GenerateInterference,
                        this,
                        interferencePsdAll,
                        MilliSeconds(100));

    // Since it takes m_expectedPpduDuration to transmit the PPDU,
    // both PHYs should be back to CCA_BUSY (due to the interference) at the same time,
    // even the PHY that has no PSDU addressed to it.
    Simulator::Schedule(Seconds(5) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta1,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(5) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta2,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(5) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta3,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(5) + m_expectedPpduDuration,
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta1,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(5) + m_expectedPpduDuration,
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta2,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(5) + m_expectedPpduDuration,
                        &TestDlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta3,
                        WifiPhyState::CCA_BUSY);

    // One PSDU of 1000 bytes should have been unsuccessfuly received by STA 1 (since interference
    // occupies RU 1)
    Simulator::Schedule(Seconds(5.1), &TestDlOfdmaPhyTransmission::CheckResultsSta1, this, 0, 1, 0);
    // One PSDU of 1500 bytes should have been unsuccessfuly received by STA 2 (since interference
    // occupies RU 2)
    Simulator::Schedule(Seconds(5.1), &TestDlOfdmaPhyTransmission::CheckResultsSta2, this, 0, 1, 0);
    // No PSDU should have been received by STA3
    Simulator::Schedule(Seconds(5.1), &TestDlOfdmaPhyTransmission::CheckResultsSta3, this, 0, 0, 0);

    Simulator::Schedule(Seconds(5.5), &TestDlOfdmaPhyTransmission::ResetResults, this);

    Simulator::Run();
}

void
TestDlOfdmaPhyTransmission::DoRun()
{
    m_frequency = MHz_u{5180};
    m_channelWidth = MHz_u{20};
    m_expectedPpduDuration = NanoSeconds(306400);
    RunOne();

    m_frequency = MHz_u{5190};
    m_channelWidth = MHz_u{40};
    m_expectedPpduDuration = NanoSeconds(156800);
    RunOne();

    m_frequency = MHz_u{5210};
    m_channelWidth = MHz_u{80};
    m_expectedPpduDuration = NanoSeconds(102400);
    RunOne();

    m_frequency = MHz_u{5250};
    m_channelWidth = MHz_u{160};
    m_expectedPpduDuration = NanoSeconds(75200);
    RunOne();

    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief DL-OFDMA PHY puncturing test
 */
class TestDlOfdmaPhyPuncturing : public TestCase
{
  public:
    TestDlOfdmaPhyPuncturing();

  private:
    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     * Receive success function for STA 1
     * @param psdu the PSDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector the transmit vector
     * @param statusPerMpdu reception status per MPDU
     */
    void RxSuccessSta1(Ptr<const WifiPsdu> psdu,
                       RxSignalInfo rxSignalInfo,
                       const WifiTxVector& txVector,
                       const std::vector<bool>& statusPerMpdu);

    /**
     * Receive success function for STA 2
     * @param psdu the PSDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector the transmit vector
     * @param statusPerMpdu reception status per MPDU
     */
    void RxSuccessSta2(Ptr<const WifiPsdu> psdu,
                       RxSignalInfo rxSignalInfo,
                       const WifiTxVector& txVector,
                       const std::vector<bool>& statusPerMpdu);

    /**
     * Receive failure function for STA 1
     * @param psdu the PSDU
     */
    void RxFailureSta1(Ptr<const WifiPsdu> psdu);

    /**
     * Receive failure function for STA 2
     * @param psdu the PSDU
     */
    void RxFailureSta2(Ptr<const WifiPsdu> psdu);

    /**
     * Check the results for STA 1
     * @param expectedRxSuccess the expected number of RX success
     * @param expectedRxFailure the expected number of RX failures
     * @param expectedRxBytes the expected number of RX bytes
     */
    void CheckResultsSta1(uint32_t expectedRxSuccess,
                          uint32_t expectedRxFailure,
                          uint32_t expectedRxBytes);

    /**
     * Check the results for STA 2
     * @param expectedRxSuccess the expected number of RX success
     * @param expectedRxFailure the expected number of RX failures
     * @param expectedRxBytes the expected number of RX bytes
     */
    void CheckResultsSta2(uint32_t expectedRxSuccess,
                          uint32_t expectedRxFailure,
                          uint32_t expectedRxBytes);

    /**
     * Reset the results
     */
    void ResetResults();

    /**
     * Send MU-PPDU function
     * @param rxStaId1 the ID of the recipient STA for the first PSDU
     * @param rxStaId2 the ID of the recipient STA for the second PSDU
     * @param puncturedSubchannels indicates for each subchannel whether it is punctured or not. if
     * empty, preamble puncturing is not used.
     */
    void SendMuPpdu(uint16_t rxStaId1,
                    uint16_t rxStaId2,
                    const std::vector<bool>& puncturedSubchannels);

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
     * Run one function
     */
    void RunOne();

    /**
     * Schedule now to check  the PHY state
     * @param phy the PHY
     * @param expectedState the expected state of the PHY
     */
    void CheckPhyState(Ptr<OfdmaSpectrumWifiPhy> phy, WifiPhyState expectedState);

    /**
     * Check the PHY state now
     * @param phy the PHY
     * @param expectedState the expected state of the PHY
     */
    void DoCheckPhyState(Ptr<OfdmaSpectrumWifiPhy> phy, WifiPhyState expectedState);

    uint32_t m_countRxSuccessSta1; ///< count RX success for STA 1
    uint32_t m_countRxSuccessSta2; ///< count RX success for STA 2
    uint32_t m_countRxFailureSta1; ///< count RX failure for STA 1
    uint32_t m_countRxFailureSta2; ///< count RX failure for STA 2
    uint32_t m_countRxBytesSta1;   ///< count RX bytes for STA 1
    uint32_t m_countRxBytesSta2;   ///< count RX bytes for STA 2

    Ptr<SpectrumWifiPhy> m_phyAp;           ///< PHY of AP
    Ptr<OfdmaSpectrumWifiPhy> m_phySta1;    ///< PHY of STA 1
    Ptr<OfdmaSpectrumWifiPhy> m_phySta2;    ///< PHY of STA 2
    Ptr<WaveformGenerator> m_phyInterferer; ///< PHY of interferer

    MHz_u m_frequency;    ///< frequency
    MHz_u m_channelWidth; ///< channel width

    uint8_t m_indexSubchannel; ///< Index of the subchannel (starting from 0) that should contain an
                               ///< interference and be punctured during the test run

    Time m_expectedPpduDuration20Mhz; ///< expected duration to send MU PPDU on 20 MHz RU
    Time m_expectedPpduDuration40Mhz; ///< expected duration to send MU PPDU on 40 MHz RU
};

TestDlOfdmaPhyPuncturing::TestDlOfdmaPhyPuncturing()
    : TestCase("DL-OFDMA PHY puncturing test"),
      m_countRxSuccessSta1(0),
      m_countRxSuccessSta2(0),
      m_countRxFailureSta1(0),
      m_countRxFailureSta2(0),
      m_countRxBytesSta1(0),
      m_countRxBytesSta2(0),
      m_frequency(MHz_u{5210}),
      m_channelWidth(MHz_u{80}),
      m_indexSubchannel(0),
      m_expectedPpduDuration20Mhz(NanoSeconds(156800)),
      m_expectedPpduDuration40Mhz(NanoSeconds(102400))
{
}

void
TestDlOfdmaPhyPuncturing::ResetResults()
{
    m_countRxSuccessSta1 = 0;
    m_countRxSuccessSta2 = 0;
    m_countRxFailureSta1 = 0;
    m_countRxFailureSta2 = 0;
    m_countRxBytesSta1 = 0;
    m_countRxBytesSta2 = 0;
}

void
TestDlOfdmaPhyPuncturing::SendMuPpdu(uint16_t rxStaId1,
                                     uint16_t rxStaId2,
                                     const std::vector<bool>& puncturedSubchannels)
{
    NS_LOG_FUNCTION(this << rxStaId1 << rxStaId2);
    WifiConstPsduMap psdus;
    WifiTxVector txVector{HePhy::GetHeMcs7(),
                          0,
                          WIFI_PREAMBLE_HE_MU,
                          NanoSeconds(800),
                          1,
                          1,
                          0,
                          m_channelWidth,
                          false,
                          false};

    HeRu::RuType ruType =
        puncturedSubchannels.empty()
            ? HeRu::RU_484_TONE
            : (puncturedSubchannels.at(1) ? HeRu::RU_242_TONE : HeRu::RU_484_TONE);
    HeRu::RuSpec ru1(ruType, 1, true);
    txVector.SetRu(ru1, rxStaId1);
    txVector.SetMode(HePhy::GetHeMcs7(), rxStaId1);
    txVector.SetNss(1, rxStaId1);

    ruType = puncturedSubchannels.empty()
                 ? HeRu::RU_484_TONE
                 : (puncturedSubchannels.at(1) ? HeRu::RU_484_TONE : HeRu::RU_242_TONE);
    HeRu::RuSpec ru2(ruType,
                     ruType == HeRu::RU_484_TONE ? 2 : (puncturedSubchannels.at(3) ? 3 : 4),
                     true);
    txVector.SetRu(ru2, rxStaId2);
    txVector.SetMode(HePhy::GetHeMcs9(), rxStaId2);
    txVector.SetNss(1, rxStaId2);

    RuAllocation ruAlloc;
    if (puncturedSubchannels.empty())
    {
        std::fill_n(std::back_inserter(ruAlloc), 4, 200);
    }
    else
    {
        ruAlloc.push_back(puncturedSubchannels.at(1) ? 192 : 200);
        ruAlloc.push_back(puncturedSubchannels.at(1) ? 113 : 200);
        ruAlloc.push_back(puncturedSubchannels.at(2) ? 113
                                                     : (puncturedSubchannels.at(3) ? 192 : 200));
        ruAlloc.push_back(puncturedSubchannels.at(2) ? 192
                                                     : (puncturedSubchannels.at(3) ? 113 : 200));
    }

    txVector.SetRuAllocation(ruAlloc, 0);
    txVector.SetSigBMode(VhtPhy::GetVhtMcs5());

    Ptr<Packet> pkt1 = Create<Packet>(1000);
    WifiMacHeader hdr1;
    hdr1.SetType(WIFI_MAC_QOSDATA);
    hdr1.SetQosTid(0);
    hdr1.SetAddr1(Mac48Address("00:00:00:00:00:01"));
    hdr1.SetSequenceNumber(1);
    Ptr<WifiPsdu> psdu1 = Create<WifiPsdu>(pkt1, hdr1);
    psdus.insert(std::make_pair(rxStaId1, psdu1));

    Ptr<Packet> pkt2 = Create<Packet>(1500);
    WifiMacHeader hdr2;
    hdr2.SetType(WIFI_MAC_QOSDATA);
    hdr2.SetQosTid(0);
    hdr2.SetAddr1(Mac48Address("00:00:00:00:00:02"));
    hdr2.SetSequenceNumber(2);
    Ptr<WifiPsdu> psdu2 = Create<WifiPsdu>(pkt2, hdr2);
    psdus.insert(std::make_pair(rxStaId2, psdu2));

    if (!puncturedSubchannels.empty())
    {
        txVector.SetInactiveSubchannels(puncturedSubchannels);
    }

    m_phyAp->Send(psdus, txVector);
}

void
TestDlOfdmaPhyPuncturing::GenerateInterference(Ptr<SpectrumValue> interferencePsd, Time duration)
{
    NS_LOG_FUNCTION(this << duration);
    m_phyInterferer->SetTxPowerSpectralDensity(interferencePsd);
    m_phyInterferer->SetPeriod(duration);
    m_phyInterferer->Start();
    Simulator::Schedule(duration, &TestDlOfdmaPhyPuncturing::StopInterference, this);
}

void
TestDlOfdmaPhyPuncturing::StopInterference()
{
    NS_LOG_FUNCTION(this);
    m_phyInterferer->Stop();
}

void
TestDlOfdmaPhyPuncturing::RxSuccessSta1(Ptr<const WifiPsdu> psdu,
                                        RxSignalInfo rxSignalInfo,
                                        const WifiTxVector& txVector,
                                        const std::vector<bool>& /*statusPerMpdu*/)
{
    NS_LOG_FUNCTION(this << *psdu << rxSignalInfo << txVector);
    m_countRxSuccessSta1++;
    m_countRxBytesSta1 += (psdu->GetSize() - 30);
}

void
TestDlOfdmaPhyPuncturing::RxSuccessSta2(Ptr<const WifiPsdu> psdu,
                                        RxSignalInfo rxSignalInfo,
                                        const WifiTxVector& txVector,
                                        const std::vector<bool>& /*statusPerMpdu*/)
{
    NS_LOG_FUNCTION(this << *psdu << rxSignalInfo << txVector);
    m_countRxSuccessSta2++;
    m_countRxBytesSta2 += (psdu->GetSize() - 30);
}

void
TestDlOfdmaPhyPuncturing::RxFailureSta1(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << *psdu);
    m_countRxFailureSta1++;
}

void
TestDlOfdmaPhyPuncturing::RxFailureSta2(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << *psdu);
    m_countRxFailureSta2++;
}

void
TestDlOfdmaPhyPuncturing::CheckResultsSta1(uint32_t expectedRxSuccess,
                                           uint32_t expectedRxFailure,
                                           uint32_t expectedRxBytes)
{
    NS_TEST_ASSERT_MSG_EQ(m_countRxSuccessSta1,
                          expectedRxSuccess,
                          "The number of successfully received packets by STA 1 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxFailureSta1,
                          expectedRxFailure,
                          "The number of unsuccessfuly received packets by STA 1 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxBytesSta1,
                          expectedRxBytes,
                          "The number of bytes received by STA 1 is not correct!");
}

void
TestDlOfdmaPhyPuncturing::CheckResultsSta2(uint32_t expectedRxSuccess,
                                           uint32_t expectedRxFailure,
                                           uint32_t expectedRxBytes)
{
    NS_TEST_ASSERT_MSG_EQ(m_countRxSuccessSta2,
                          expectedRxSuccess,
                          "The number of successfully received packets by STA 2 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxFailureSta2,
                          expectedRxFailure,
                          "The number of unsuccessfuly received packets by STA 2 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxBytesSta2,
                          expectedRxBytes,
                          "The number of bytes received by STA 2 is not correct!");
}

void
TestDlOfdmaPhyPuncturing::CheckPhyState(Ptr<OfdmaSpectrumWifiPhy> phy, WifiPhyState expectedState)
{
    // This is needed to make sure PHY state will be checked as the last event if a state change
    // occurred at the exact same time as the check
    Simulator::ScheduleNow(&TestDlOfdmaPhyPuncturing::DoCheckPhyState, this, phy, expectedState);
}

void
TestDlOfdmaPhyPuncturing::DoCheckPhyState(Ptr<OfdmaSpectrumWifiPhy> phy, WifiPhyState expectedState)
{
    WifiPhyState currentState;
    PointerValue ptr;
    phy->GetAttribute("State", ptr);
    Ptr<WifiPhyStateHelper> state = DynamicCast<WifiPhyStateHelper>(ptr.Get<WifiPhyStateHelper>());
    currentState = state->GetState();
    NS_LOG_FUNCTION(this << currentState);
    NS_TEST_ASSERT_MSG_EQ(currentState,
                          expectedState,
                          "PHY State " << currentState << " does not match expected state "
                                       << expectedState << " at " << Simulator::Now());
}

void
TestDlOfdmaPhyPuncturing::DoSetup()
{
    Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel>();
    lossModel->SetFrequency(MHzToHz(m_frequency));
    spectrumChannel->AddPropagationLossModel(lossModel);
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);

    Ptr<Node> apNode = CreateObject<Node>();
    Ptr<WifiNetDevice> apDev = CreateObject<WifiNetDevice>();
    m_phyAp = CreateObject<SpectrumWifiPhy>();
    Ptr<InterferenceHelper> apInterferenceHelper = CreateObject<InterferenceHelper>();
    m_phyAp->SetInterferenceHelper(apInterferenceHelper);
    Ptr<ErrorRateModel> apErrorModel = CreateObject<NistErrorRateModel>();
    m_phyAp->SetErrorRateModel(apErrorModel);
    m_phyAp->SetDevice(apDev);
    m_phyAp->AddChannel(spectrumChannel);
    m_phyAp->ConfigureStandard(WIFI_STANDARD_80211ax);
    Ptr<ConstantPositionMobilityModel> apMobility = CreateObject<ConstantPositionMobilityModel>();
    m_phyAp->SetMobility(apMobility);
    apDev->SetPhy(m_phyAp);
    apNode->AggregateObject(apMobility);
    apNode->AddDevice(apDev);

    Ptr<Node> sta1Node = CreateObject<Node>();
    Ptr<WifiNetDevice> sta1Dev = CreateObject<WifiNetDevice>();
    m_phySta1 = CreateObject<OfdmaSpectrumWifiPhy>(1);
    Ptr<InterferenceHelper> sta1InterferenceHelper = CreateObject<InterferenceHelper>();
    m_phySta1->SetInterferenceHelper(sta1InterferenceHelper);
    Ptr<ErrorRateModel> sta1ErrorModel = CreateObject<NistErrorRateModel>();
    m_phySta1->SetErrorRateModel(sta1ErrorModel);
    m_phySta1->SetDevice(sta1Dev);
    m_phySta1->AddChannel(spectrumChannel);
    m_phySta1->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phySta1->SetReceiveOkCallback(MakeCallback(&TestDlOfdmaPhyPuncturing::RxSuccessSta1, this));
    m_phySta1->SetReceiveErrorCallback(
        MakeCallback(&TestDlOfdmaPhyPuncturing::RxFailureSta1, this));
    Ptr<ConstantPositionMobilityModel> sta1Mobility = CreateObject<ConstantPositionMobilityModel>();
    m_phySta1->SetMobility(sta1Mobility);
    sta1Dev->SetPhy(m_phySta1);
    sta1Node->AggregateObject(sta1Mobility);
    sta1Node->AddDevice(sta1Dev);

    Ptr<Node> sta2Node = CreateObject<Node>();
    Ptr<WifiNetDevice> sta2Dev = CreateObject<WifiNetDevice>();
    m_phySta2 = CreateObject<OfdmaSpectrumWifiPhy>(2);
    Ptr<InterferenceHelper> sta2InterferenceHelper = CreateObject<InterferenceHelper>();
    m_phySta2->SetInterferenceHelper(sta2InterferenceHelper);
    Ptr<ErrorRateModel> sta2ErrorModel = CreateObject<NistErrorRateModel>();
    m_phySta2->SetErrorRateModel(sta2ErrorModel);
    m_phySta2->SetDevice(sta2Dev);
    m_phySta2->AddChannel(spectrumChannel);
    m_phySta2->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phySta2->SetReceiveOkCallback(MakeCallback(&TestDlOfdmaPhyPuncturing::RxSuccessSta2, this));
    m_phySta2->SetReceiveErrorCallback(
        MakeCallback(&TestDlOfdmaPhyPuncturing::RxFailureSta2, this));
    Ptr<ConstantPositionMobilityModel> sta2Mobility = CreateObject<ConstantPositionMobilityModel>();
    m_phySta2->SetMobility(sta2Mobility);
    sta2Dev->SetPhy(m_phySta2);
    sta2Node->AggregateObject(sta2Mobility);
    sta2Node->AddDevice(sta2Dev);

    Ptr<Node> interfererNode = CreateObject<Node>();
    Ptr<NonCommunicatingNetDevice> interfererDev = CreateObject<NonCommunicatingNetDevice>();
    m_phyInterferer = CreateObject<WaveformGenerator>();
    m_phyInterferer->SetDevice(interfererDev);
    m_phyInterferer->SetChannel(spectrumChannel);
    m_phyInterferer->SetDutyCycle(1);
    interfererNode->AddDevice(interfererDev);
}

void
TestDlOfdmaPhyPuncturing::DoTeardown()
{
    m_phyAp->Dispose();
    m_phyAp = nullptr;
    m_phySta1->Dispose();
    m_phySta1 = nullptr;
    m_phySta2->Dispose();
    m_phySta2 = nullptr;
    m_phyInterferer->Dispose();
    m_phyInterferer = nullptr;
}

void
TestDlOfdmaPhyPuncturing::RunOne()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 0;
    m_phyAp->AssignStreams(streamNumber);
    m_phySta1->AssignStreams(streamNumber);
    m_phySta2->AssignStreams(streamNumber);

    auto channelNum = WifiPhyOperatingChannel::FindFirst(0,
                                                         m_frequency,
                                                         m_channelWidth,
                                                         WIFI_STANDARD_80211ax,
                                                         WIFI_PHY_BAND_5GHZ)
                          ->number;

    m_phyAp->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, m_channelWidth, WIFI_PHY_BAND_5GHZ, 0});
    m_phySta1->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, m_channelWidth, WIFI_PHY_BAND_5GHZ, 0});
    m_phySta2->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, m_channelWidth, WIFI_PHY_BAND_5GHZ, 0});

    // A strong non-wifi interference is generated on selected 20 MHz subchannel for the whole
    // duration of the test run
    BandInfo bandInfo;
    bandInfo.fc = MHzToHz(m_frequency - (m_channelWidth / 2) + 10 + (m_indexSubchannel * 20));
    // Occupy half of the RU to make sure we do not have some power allocated to the subcarriers on
    // the border of another RU
    bandInfo.fl = bandInfo.fc - MHzToHz(5);
    bandInfo.fh = bandInfo.fc + MHzToHz(5);
    Bands bands;
    bands.push_back(bandInfo);

    auto spectrumInterference = Create<SpectrumModel>(bands);
    auto interferencePsd = Create<SpectrumValue>(spectrumInterference);
    Watt_u interferencePower{0.1};
    *interferencePsd = interferencePower / 10e6;

    Simulator::Schedule(Seconds(0),
                        &TestDlOfdmaPhyPuncturing::GenerateInterference,
                        this,
                        interferencePsd,
                        Seconds(3));

    //---------------------------------------------------------------------------
    // Send MU PPDU with two PSDUs addressed to STA 1 and STA 2 without preamble puncturing:
    Simulator::Schedule(Seconds(1),
                        &TestDlOfdmaPhyPuncturing::SendMuPpdu,
                        this,
                        1,
                        2,
                        std::vector<bool>{});

    // Since it takes m_expectedPpduDuration to transmit the PPDU,
    // both PHYs should be back to IDLE at the same time.
    Simulator::Schedule(Seconds(1) + m_expectedPpduDuration40Mhz - NanoSeconds(1),
                        &TestDlOfdmaPhyPuncturing::CheckPhyState,
                        this,
                        m_phySta1,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(1) + m_expectedPpduDuration40Mhz - NanoSeconds(1),
                        &TestDlOfdmaPhyPuncturing::CheckPhyState,
                        this,
                        m_phySta2,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(1) + m_expectedPpduDuration40Mhz,
                        &TestDlOfdmaPhyPuncturing::CheckPhyState,
                        this,
                        m_phySta1,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(1) + m_expectedPpduDuration40Mhz,
                        &TestDlOfdmaPhyPuncturing::CheckPhyState,
                        this,
                        m_phySta2,
                        WifiPhyState::IDLE);

    if (m_indexSubchannel < 2) // interference in RU 1
    {
        // One PSDU of 1000 bytes should have been unsuccessfuly received by STA 1
        Simulator::Schedule(Seconds(1.1),
                            &TestDlOfdmaPhyPuncturing::CheckResultsSta1,
                            this,
                            0,
                            1,
                            0);
        // One PSDU of 1500 bytes should have been successfully received by STA 2
        Simulator::Schedule(Seconds(1.1),
                            &TestDlOfdmaPhyPuncturing::CheckResultsSta2,
                            this,
                            1,
                            0,
                            1500);
    }
    else // interference in RU 2
    {
        // One PSDU of 1000 bytes should have been successfully received by STA 1
        Simulator::Schedule(Seconds(1.1),
                            &TestDlOfdmaPhyPuncturing::CheckResultsSta1,
                            this,
                            1,
                            0,
                            1000);
        // One PSDU of 1500 bytes should have been unsuccessfuly received by STA 2
        Simulator::Schedule(Seconds(1.1),
                            &TestDlOfdmaPhyPuncturing::CheckResultsSta2,
                            this,
                            0,
                            1,
                            0);
    }

    Simulator::Schedule(Seconds(1.5), &TestDlOfdmaPhyPuncturing::ResetResults, this);

    //---------------------------------------------------------------------------
    // Send MU PPDU with two PSDUs addressed to STA 1 and STA 2 with preamble puncturing:
    // the punctured 20 MHz subchannel is the one that has interference
    std::vector<bool> puncturedSubchannels;
    const auto num20MhzSubchannels = Count20MHzSubchannels(m_channelWidth);
    for (std::size_t i = 0; i < num20MhzSubchannels; ++i)
    {
        if (i == m_indexSubchannel)
        {
            puncturedSubchannels.push_back(true);
        }
        else
        {
            puncturedSubchannels.push_back(false);
        }
    }
    Simulator::Schedule(Seconds(2),
                        &TestDlOfdmaPhyPuncturing::SendMuPpdu,
                        this,
                        1,
                        2,
                        puncturedSubchannels);

    // Since it takes m_expectedPpduDuration to transmit the PPDU,
    // both PHYs should be back to IDLE at the same time.
    Simulator::Schedule(Seconds(2) + m_expectedPpduDuration20Mhz - NanoSeconds(1),
                        &TestDlOfdmaPhyPuncturing::CheckPhyState,
                        this,
                        m_phySta1,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(2) + m_expectedPpduDuration20Mhz - NanoSeconds(1),
                        &TestDlOfdmaPhyPuncturing::CheckPhyState,
                        this,
                        m_phySta2,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(2) + m_expectedPpduDuration20Mhz,
                        &TestDlOfdmaPhyPuncturing::CheckPhyState,
                        this,
                        m_phySta1,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(2) + m_expectedPpduDuration20Mhz,
                        &TestDlOfdmaPhyPuncturing::CheckPhyState,
                        this,
                        m_phySta2,
                        WifiPhyState::IDLE);

    // One PSDU of 1000 bytes should have been successfully received by STA 1
    Simulator::Schedule(Seconds(2.1),
                        &TestDlOfdmaPhyPuncturing::CheckResultsSta1,
                        this,
                        1,
                        0,
                        1000);
    // One PSDU of 1500 bytes should have been successfully received by STA 2
    Simulator::Schedule(Seconds(2.1),
                        &TestDlOfdmaPhyPuncturing::CheckResultsSta2,
                        this,
                        1,
                        0,
                        1500);

    Simulator::Schedule(Seconds(2.5), &TestDlOfdmaPhyPuncturing::ResetResults, this);

    Simulator::Run();
}

void
TestDlOfdmaPhyPuncturing::DoRun()
{
    // test all 20 MHz subchannels in the 80 MHz operation channel except the primary one which
    // cannot be punctured
    for (auto index : {1, 2, 3})
    {
        m_indexSubchannel = index;
        RunOne();
    }
    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief UL-OFDMA PPDU UID attribution test
 */
class TestUlOfdmaPpduUid : public TestCase
{
  public:
    TestUlOfdmaPpduUid();
    ~TestUlOfdmaPpduUid() override;

  private:
    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     * Transmitted PPDU information function for AP
     * @param uid the UID of the transmitted PPDU
     */
    void TxPpduAp(uint64_t uid);
    /**
     * Transmitted PPDU information function for STA 1
     * @param uid the UID of the transmitted PPDU
     */
    void TxPpduSta1(uint64_t uid);
    /**
     * Transmitted PPDU information function for STA 2
     * @param uid the UID of the transmitted PPDU
     */
    void TxPpduSta2(uint64_t uid);
    /**
     * Reset the global PPDU UID counter in WifiPhy
     */
    void ResetPpduUid();

    /**
     * Send MU-PPDU toward both STAs.
     */
    void SendMuPpdu();
    /**
     * Send TB-PPDU from both STAs.
     */
    void SendTbPpdu();
    /**
     * Send SU-PPDU function
     * @param txStaId the ID of the sending STA
     */
    void SendSuPpdu(uint16_t txStaId);

    /**
     * Check the UID of the transmitted PPDU
     * @param staId the STA-ID of the PHY (0 for AP)
     * @param expectedUid the expected UID
     */
    void CheckUid(uint16_t staId, uint64_t expectedUid);

    Ptr<OfdmaSpectrumWifiPhy> m_phyAp;   ///< PHY of AP
    Ptr<OfdmaSpectrumWifiPhy> m_phySta1; ///< PHY of STA 1
    Ptr<OfdmaSpectrumWifiPhy> m_phySta2; ///< PHY of STA 2

    uint64_t m_ppduUidAp;   ///< UID of PPDU transmitted by AP
    uint64_t m_ppduUidSta1; ///< UID of PPDU transmitted by STA1
    uint64_t m_ppduUidSta2; ///< UID of PPDU transmitted by STA2
};

TestUlOfdmaPpduUid::TestUlOfdmaPpduUid()
    : TestCase("UL-OFDMA PPDU UID attribution test"),
      m_ppduUidAp(UINT64_MAX),
      m_ppduUidSta1(UINT64_MAX),
      m_ppduUidSta2(UINT64_MAX)
{
}

TestUlOfdmaPpduUid::~TestUlOfdmaPpduUid()
{
}

void
TestUlOfdmaPpduUid::DoSetup()
{
    Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel>();
    lossModel->SetFrequency(DEFAULT_FREQUENCY);
    spectrumChannel->AddPropagationLossModel(lossModel);
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);

    Ptr<Node> apNode = CreateObject<Node>();
    Ptr<WifiNetDevice> apDev = CreateObject<WifiNetDevice>();
    m_phyAp = CreateObject<OfdmaSpectrumWifiPhy>(0);
    Ptr<InterferenceHelper> apInterferenceHelper = CreateObject<InterferenceHelper>();
    m_phyAp->SetInterferenceHelper(apInterferenceHelper);
    Ptr<ErrorRateModel> apErrorModel = CreateObject<NistErrorRateModel>();
    m_phyAp->SetErrorRateModel(apErrorModel);
    m_phyAp->AddChannel(spectrumChannel);
    m_phyAp->ConfigureStandard(WIFI_STANDARD_80211ax);
    auto channelNum = WifiPhyOperatingChannel::FindFirst(0,
                                                         DEFAULT_FREQUENCY,
                                                         DEFAULT_CHANNEL_WIDTH,
                                                         WIFI_STANDARD_80211ax,
                                                         WIFI_PHY_BAND_5GHZ)
                          ->number;
    m_phyAp->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, DEFAULT_CHANNEL_WIDTH, WIFI_PHY_BAND_5GHZ, 0});
    m_phyAp->SetDevice(apDev);
    m_phyAp->TraceConnectWithoutContext("TxPpduUid",
                                        MakeCallback(&TestUlOfdmaPpduUid::TxPpduAp, this));
    Ptr<ConstantPositionMobilityModel> apMobility = CreateObject<ConstantPositionMobilityModel>();
    m_phyAp->SetMobility(apMobility);
    apDev->SetPhy(m_phyAp);
    apNode->AggregateObject(apMobility);
    apNode->AddDevice(apDev);
    apDev->SetStandard(WIFI_STANDARD_80211ax);
    apDev->SetHeConfiguration(CreateObject<HeConfiguration>());

    Ptr<Node> sta1Node = CreateObject<Node>();
    Ptr<WifiNetDevice> sta1Dev = CreateObject<WifiNetDevice>();
    m_phySta1 = CreateObject<OfdmaSpectrumWifiPhy>(1);
    Ptr<InterferenceHelper> sta1InterferenceHelper = CreateObject<InterferenceHelper>();
    m_phySta1->SetInterferenceHelper(sta1InterferenceHelper);
    Ptr<ErrorRateModel> sta1ErrorModel = CreateObject<NistErrorRateModel>();
    m_phySta1->SetErrorRateModel(sta1ErrorModel);
    m_phySta1->AddChannel(spectrumChannel);
    m_phySta1->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phySta1->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, DEFAULT_CHANNEL_WIDTH, WIFI_PHY_BAND_5GHZ, 0});
    m_phySta1->SetDevice(sta1Dev);
    m_phySta1->TraceConnectWithoutContext("TxPpduUid",
                                          MakeCallback(&TestUlOfdmaPpduUid::TxPpduSta1, this));
    Ptr<ConstantPositionMobilityModel> sta1Mobility = CreateObject<ConstantPositionMobilityModel>();
    m_phySta1->SetMobility(sta1Mobility);
    sta1Dev->SetPhy(m_phySta1);
    sta1Node->AggregateObject(sta1Mobility);
    sta1Node->AddDevice(sta1Dev);

    Ptr<Node> sta2Node = CreateObject<Node>();
    Ptr<WifiNetDevice> sta2Dev = CreateObject<WifiNetDevice>();
    m_phySta2 = CreateObject<OfdmaSpectrumWifiPhy>(2);
    Ptr<InterferenceHelper> sta2InterferenceHelper = CreateObject<InterferenceHelper>();
    m_phySta2->SetInterferenceHelper(sta2InterferenceHelper);
    Ptr<ErrorRateModel> sta2ErrorModel = CreateObject<NistErrorRateModel>();
    m_phySta2->SetErrorRateModel(sta2ErrorModel);
    m_phySta2->AddChannel(spectrumChannel);
    m_phySta2->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phySta2->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, DEFAULT_CHANNEL_WIDTH, WIFI_PHY_BAND_5GHZ, 0});
    m_phySta2->SetDevice(sta2Dev);
    m_phySta2->TraceConnectWithoutContext("TxPpduUid",
                                          MakeCallback(&TestUlOfdmaPpduUid::TxPpduSta2, this));
    Ptr<ConstantPositionMobilityModel> sta2Mobility = CreateObject<ConstantPositionMobilityModel>();
    m_phySta2->SetMobility(sta2Mobility);
    sta2Dev->SetPhy(m_phySta2);
    sta2Node->AggregateObject(sta2Mobility);
    sta2Node->AddDevice(sta2Dev);
}

void
TestUlOfdmaPpduUid::DoTeardown()
{
    m_phyAp->Dispose();
    m_phyAp = nullptr;
    m_phySta1->Dispose();
    m_phySta1 = nullptr;
    m_phySta2->Dispose();
    m_phySta2 = nullptr;
}

void
TestUlOfdmaPpduUid::CheckUid(uint16_t staId, uint64_t expectedUid)
{
    uint64_t uid;
    std::string device;
    switch (staId)
    {
    case 0:
        uid = m_ppduUidAp;
        device = "AP";
        break;
    case 1:
        uid = m_ppduUidSta1;
        device = "STA1";
        break;
    case 2:
        uid = m_ppduUidSta2;
        device = "STA2";
        break;
    default:
        NS_ABORT_MSG("Unexpected STA-ID");
    }
    NS_TEST_ASSERT_MSG_EQ(uid,
                          expectedUid,
                          "UID " << uid << " does not match expected one " << expectedUid << " for "
                                 << device << " at " << Simulator::Now());
}

void
TestUlOfdmaPpduUid::TxPpduAp(uint64_t uid)
{
    NS_LOG_FUNCTION(this << uid);
    m_ppduUidAp = uid;
}

void
TestUlOfdmaPpduUid::TxPpduSta1(uint64_t uid)
{
    NS_LOG_FUNCTION(this << uid);
    m_ppduUidSta1 = uid;
}

void
TestUlOfdmaPpduUid::TxPpduSta2(uint64_t uid)
{
    NS_LOG_FUNCTION(this << uid);
    m_ppduUidSta2 = uid;
}

void
TestUlOfdmaPpduUid::ResetPpduUid()
{
    NS_LOG_FUNCTION(this);
    m_phyAp->SetPpduUid(0); // one is enough since it's a global attribute
}

void
TestUlOfdmaPpduUid::SendMuPpdu()
{
    WifiConstPsduMap psdus;
    WifiTxVector txVector{HePhy::GetHeMcs7(),
                          0,
                          WIFI_PREAMBLE_HE_MU,
                          NanoSeconds(800),
                          1,
                          1,
                          0,
                          DEFAULT_CHANNEL_WIDTH,
                          false,
                          false};

    uint16_t rxStaId1 = 1;
    HeRu::RuSpec ru1(HeRu::RU_106_TONE, 1, true);
    txVector.SetRu(ru1, rxStaId1);
    txVector.SetMode(HePhy::GetHeMcs7(), rxStaId1);
    txVector.SetNss(1, rxStaId1);

    uint16_t rxStaId2 = 2;
    HeRu::RuSpec ru2(HeRu::RU_106_TONE, 2, true);
    txVector.SetRu(ru2, rxStaId2);
    txVector.SetMode(HePhy::GetHeMcs9(), rxStaId2);
    txVector.SetNss(1, rxStaId2);
    txVector.SetSigBMode(VhtPhy::GetVhtMcs5());
    txVector.SetRuAllocation({96}, 0);

    Ptr<Packet> pkt1 = Create<Packet>(1000);
    WifiMacHeader hdr1;
    hdr1.SetType(WIFI_MAC_QOSDATA);
    hdr1.SetQosTid(0);
    hdr1.SetAddr1(Mac48Address("00:00:00:00:00:01"));
    hdr1.SetSequenceNumber(1);
    Ptr<WifiPsdu> psdu1 = Create<WifiPsdu>(pkt1, hdr1);
    psdus.insert(std::make_pair(rxStaId1, psdu1));

    Ptr<Packet> pkt2 = Create<Packet>(1500);
    WifiMacHeader hdr2;
    hdr2.SetType(WIFI_MAC_QOSDATA);
    hdr2.SetQosTid(0);
    hdr2.SetAddr1(Mac48Address("00:00:00:00:00:02"));
    hdr2.SetSequenceNumber(2);
    Ptr<WifiPsdu> psdu2 = Create<WifiPsdu>(pkt2, hdr2);
    psdus.insert(std::make_pair(rxStaId2, psdu2));

    m_phyAp->Send(psdus, txVector);
}

void
TestUlOfdmaPpduUid::SendTbPpdu()
{
    WifiConstPsduMap psdus1;
    WifiConstPsduMap psdus2;

    WifiTxVector txVector1{HePhy::GetHeMcs7(),
                           0,
                           WIFI_PREAMBLE_HE_TB,
                           NanoSeconds(1600),
                           1,
                           1,
                           0,
                           DEFAULT_CHANNEL_WIDTH,
                           false,
                           false};
    WifiTxVector txVector2{txVector1};
    WifiTxVector trigVector{txVector2};

    uint16_t rxStaId1 = 1;
    HeRu::RuSpec ru1(HeRu::RU_106_TONE, 1, false);
    txVector1.SetRu(ru1, rxStaId1);
    txVector1.SetMode(HePhy::GetHeMcs7(), rxStaId1);
    txVector1.SetNss(1, rxStaId1);
    trigVector.SetRu(ru1, rxStaId1);
    trigVector.SetMode(HePhy::GetHeMcs7(), rxStaId1);
    trigVector.SetNss(1, rxStaId1);

    auto pkt1 = Create<Packet>(1000);
    WifiMacHeader hdr1;
    hdr1.SetType(WIFI_MAC_QOSDATA);
    hdr1.SetQosTid(0);
    hdr1.SetAddr1(Mac48Address("00:00:00:00:00:00"));
    hdr1.SetSequenceNumber(1);
    auto psdu1 = Create<WifiPsdu>(pkt1, hdr1);
    psdus1.insert(std::make_pair(rxStaId1, psdu1));

    uint16_t rxStaId2 = 2;
    HeRu::RuSpec ru2(HeRu::RU_106_TONE, 2, false);
    txVector2.SetRu(ru2, rxStaId2);
    txVector2.SetMode(HePhy::GetHeMcs9(), rxStaId2);
    txVector2.SetNss(1, rxStaId2);
    trigVector.SetRu(ru2, rxStaId2);
    trigVector.SetMode(HePhy::GetHeMcs9(), rxStaId2);
    trigVector.SetNss(1, rxStaId2);

    auto pkt2 = Create<Packet>(1500);
    WifiMacHeader hdr2;
    hdr2.SetType(WIFI_MAC_QOSDATA);
    hdr2.SetQosTid(0);
    hdr2.SetAddr1(Mac48Address("00:00:00:00:00:00"));
    hdr2.SetSequenceNumber(2);
    auto psdu2 = Create<WifiPsdu>(pkt2, hdr2);
    psdus2.insert(std::make_pair(rxStaId2, psdu2));

    const auto txDuration1 = OfdmaSpectrumWifiPhy::CalculateTxDuration(psdu1->GetSize(),
                                                                       txVector1,
                                                                       m_phySta1->GetPhyBand(),
                                                                       rxStaId1);
    const auto txDuration2 = OfdmaSpectrumWifiPhy::CalculateTxDuration(psdu2->GetSize(),
                                                                       txVector2,
                                                                       m_phySta1->GetPhyBand(),
                                                                       rxStaId2);
    const auto txDuration = std::max(txDuration1, txDuration2);

    txVector1.SetLength(
        HePhy::ConvertHeTbPpduDurationToLSigLength(txDuration, txVector1, m_phySta1->GetPhyBand())
            .first);
    txVector2.SetLength(
        HePhy::ConvertHeTbPpduDurationToLSigLength(txDuration, txVector2, m_phySta2->GetPhyBand())
            .first);

    auto hePhyAp = DynamicCast<HePhy>(m_phyAp->GetPhyEntity(WIFI_MOD_CLASS_HE));
    hePhyAp->SetTrigVector(trigVector, txDuration);

    m_phySta1->Send(psdus1, txVector1);
    m_phySta2->Send(psdus2, txVector2);
}

void
TestUlOfdmaPpduUid::SendSuPpdu(uint16_t txStaId)
{
    WifiConstPsduMap psdus;
    WifiTxVector txVector{HePhy::GetHeMcs7(),
                          0,
                          WIFI_PREAMBLE_HE_SU,
                          NanoSeconds(800),
                          1,
                          1,
                          0,
                          DEFAULT_CHANNEL_WIDTH,
                          false,
                          false};

    auto pkt = Create<Packet>(1000);
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_QOSDATA);
    hdr.SetQosTid(0);
    hdr.SetAddr1(Mac48Address::GetBroadcast());
    hdr.SetSequenceNumber(1);
    auto psdu = Create<WifiPsdu>(pkt, hdr);
    psdus.insert(std::make_pair(SU_STA_ID, psdu));

    switch (txStaId)
    {
    case 0:
        m_phyAp->Send(psdus, txVector);
        break;
    case 1:
        m_phySta1->Send(psdus, txVector);
        break;
    case 2:
        m_phySta2->Send(psdus, txVector);
        break;
    default:
        NS_ABORT_MSG("Unexpected STA-ID");
    }
}

void
TestUlOfdmaPpduUid::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 0;
    m_phyAp->AssignStreams(streamNumber);
    m_phySta1->AssignStreams(streamNumber);
    m_phySta2->AssignStreams(streamNumber);

    // Reset PPDU UID so as not to be dependent on previously executed test cases,
    // since global attribute will be changed).
    ResetPpduUid();

    // Send HE MU PPDU with two PSDUs addressed to STA 1 and STA 2.
    // PPDU UID should be equal to 0 (the first counter value).
    Simulator::Schedule(Seconds(1), &TestUlOfdmaPpduUid::SendMuPpdu, this);
    Simulator::Schedule(Seconds(1), &TestUlOfdmaPpduUid::CheckUid, this, 0, 0);

    // Send HE SU PPDU from AP.
    // PPDU UID should be incremented since this is a new PPDU.
    Simulator::Schedule(Seconds(1.1), &TestUlOfdmaPpduUid::SendSuPpdu, this, 0);
    Simulator::Schedule(Seconds(1.1), &TestUlOfdmaPpduUid::CheckUid, this, 0, 1);

    // Send HE TB PPDU from STAs to AP.
    // PPDU UID should NOT be incremented since HE TB PPDUs reuse the UID of the immediately
    // preceding correctly received PPDU (which normally contains the trigger frame).
    Simulator::Schedule(Seconds(1.15), &TestUlOfdmaPpduUid::SendTbPpdu, this);
    Simulator::Schedule(Seconds(1.15), &TestUlOfdmaPpduUid::CheckUid, this, 1, 1);
    Simulator::Schedule(Seconds(1.15), &TestUlOfdmaPpduUid::CheckUid, this, 2, 1);

    // Send HE SU PPDU from STA1.
    // PPDU UID should be incremented since this is a new PPDU.
    Simulator::Schedule(Seconds(1.2), &TestUlOfdmaPpduUid::SendSuPpdu, this, 1);
    Simulator::Schedule(Seconds(1.2), &TestUlOfdmaPpduUid::CheckUid, this, 1, 2);

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief UL-OFDMA multiple RX events test
 */
class TestMultipleHeTbPreambles : public TestCase
{
  public:
    TestMultipleHeTbPreambles();
    ~TestMultipleHeTbPreambles() override;

  private:
    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     * Receive HE TB PPDU function.
     *
     * @param uid the UID used to identify a set of HE TB PPDUs belonging to the same UL-MU
     * transmission
     * @param staId the STA ID
     * @param txPower the TX power
     * @param payloadSize the size of the payload in bytes
     */
    void RxHeTbPpdu(uint64_t uid, uint16_t staId, Watt_u txPower, size_t payloadSize);

    /**
     * Receive OFDMA part of HE TB PPDU function.
     * Immediately schedules DoRxHeTbPpduOfdmaPart.
     *
     * @param rxParamsOfdma the spectrum signal parameters to send for OFDMA part
     */
    void RxHeTbPpduOfdmaPart(Ptr<WifiSpectrumSignalParameters> rxParamsOfdma);
    /**
     * Receive OFDMA part of HE TB PPDU function.
     * Actual reception call.
     *
     * @param rxParamsOfdma the spectrum signal parameters to send for OFDMA part
     */
    void DoRxHeTbPpduOfdmaPart(Ptr<WifiSpectrumSignalParameters> rxParamsOfdma);

    /**
     * RX dropped function
     * @param p the packet
     * @param reason the reason
     */
    void RxDropped(Ptr<const Packet> p, WifiPhyRxfailureReason reason);

    /**
     * Reset function
     */
    void Reset();

    /**
     * Check the received HE TB preambles
     * @param nEvents the number of events created by the PHY
     * @param uids the vector of expected UIDs
     */
    void CheckHeTbPreambles(size_t nEvents, std::vector<uint64_t> uids);

    /**
     * Check the number of bytes dropped
     * @param expectedBytesDropped the expected number of bytes dropped
     */
    void CheckBytesDropped(size_t expectedBytesDropped);

    Ptr<OfdmaSpectrumWifiPhy> m_phy; ///< Phy

    uint64_t m_totalBytesDropped; ///< total number of dropped bytes
    WifiTxVector m_trigVector;    ///< TRIGVECTOR
};

TestMultipleHeTbPreambles::TestMultipleHeTbPreambles()
    : TestCase("UL-OFDMA multiple RX events test"),
      m_totalBytesDropped(0),
      m_trigVector(HePhy::GetHeMcs7(),
                   0,
                   WIFI_PREAMBLE_HE_TB,
                   NanoSeconds(1600),
                   1,
                   1,
                   0,
                   DEFAULT_CHANNEL_WIDTH,
                   false,
                   false)
{
}

TestMultipleHeTbPreambles::~TestMultipleHeTbPreambles()
{
}

void
TestMultipleHeTbPreambles::Reset()
{
    NS_LOG_FUNCTION(this);
    m_totalBytesDropped = 0;
    // We have to reset PHY here since we do not trigger OFDMA payload RX event in this test
    m_phy->Reset();
    m_trigVector.GetHeMuUserInfoMap().clear();
}

void
TestMultipleHeTbPreambles::RxDropped(Ptr<const Packet> p, WifiPhyRxfailureReason reason)
{
    NS_LOG_FUNCTION(this << p << reason);
    m_totalBytesDropped += (p->GetSize() - 30);
}

void
TestMultipleHeTbPreambles::CheckHeTbPreambles(size_t nEvents, std::vector<uint64_t> uids)
{
    auto events = m_phy->GetCurrentPreambleEvents();
    NS_TEST_ASSERT_MSG_EQ(events.size(), nEvents, "The number of UL MU events is not correct!");
    for (const auto& uid : uids)
    {
        auto pair = std::make_pair(uid, WIFI_PREAMBLE_HE_TB);
        auto it = events.find(pair);
        bool found = (it != events.end());
        NS_TEST_ASSERT_MSG_EQ(found,
                              true,
                              "HE TB PPDU with UID " << uid << " has not been received!");
    }
}

void
TestMultipleHeTbPreambles::CheckBytesDropped(size_t expectedBytesDropped)
{
    NS_TEST_ASSERT_MSG_EQ(m_totalBytesDropped,
                          expectedBytesDropped,
                          "The number of dropped bytes is not correct!");
}

void
TestMultipleHeTbPreambles::RxHeTbPpdu(uint64_t uid,
                                      uint16_t staId,
                                      Watt_u txPower,
                                      size_t payloadSize)
{
    WifiConstPsduMap psdus;
    WifiTxVector txVector{HePhy::GetHeMcs7(),
                          0,
                          WIFI_PREAMBLE_HE_TB,
                          NanoSeconds(1600),
                          1,
                          1,
                          0,
                          DEFAULT_CHANNEL_WIDTH,
                          false,
                          false};

    HeRu::RuSpec ru(HeRu::RU_106_TONE, staId, false);
    txVector.SetRu(ru, staId);
    txVector.SetMode(HePhy::GetHeMcs7(), staId);
    txVector.SetNss(1, staId);

    m_trigVector.SetHeMuUserInfo(staId, {ru, 7, 1});

    auto pkt = Create<Packet>(payloadSize);
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_QOSDATA);
    hdr.SetQosTid(0);
    hdr.SetAddr1(Mac48Address("00:00:00:00:00:00"));
    hdr.SetSequenceNumber(1);
    auto psdu = Create<WifiPsdu>(pkt, hdr);
    psdus.insert(std::make_pair(staId, psdu));

    auto ppduDuration = OfdmaSpectrumWifiPhy::CalculateTxDuration(psdu->GetSize(),
                                                                  txVector,
                                                                  m_phy->GetPhyBand(),
                                                                  staId);
    auto ppdu = Create<HePpdu>(psdus,
                               txVector,
                               m_phy->GetOperatingChannel(),
                               ppduDuration,
                               uid,
                               HePpdu::PSD_NON_HE_PORTION);

    // Send non-OFDMA part
    const auto nonOfdmaDuration = m_phy->GetHePhy()->CalculateNonHeDurationForHeTb(txVector);
    const auto centerFrequency =
        m_phy->GetHePhy()->GetCenterFrequenciesForNonHePart(ppdu, staId).front();
    MHz_u ruWidth = HeRu::GetBandwidth(txVector.GetRu(staId).GetRuType());
    MHz_u channelWidth = ruWidth < MHz_u{20} ? MHz_u{20} : ruWidth;
    Ptr<SpectrumValue> rxPsd = WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity(
        centerFrequency,
        channelWidth,
        txPower,
        m_phy->GetGuardBandwidth(channelWidth));
    auto rxParams = Create<WifiSpectrumSignalParameters>();
    rxParams->psd = rxPsd;
    rxParams->txPhy = nullptr;
    rxParams->duration = nonOfdmaDuration;
    rxParams->ppdu = ppdu;

    uint16_t length;
    std::tie(length, ppduDuration) =
        HePhy::ConvertHeTbPpduDurationToLSigLength(ppduDuration, txVector, m_phy->GetPhyBand());
    txVector.SetLength(length);
    m_trigVector.SetLength(length);
    auto hePhy = DynamicCast<HePhy>(m_phy->GetLatestPhyEntity());
    hePhy->SetTrigVector(m_trigVector, ppduDuration);
    ppdu->ResetTxVector();
    m_phy->StartRx(rxParams, nullptr);

    // Schedule OFDMA part
    auto ppduOfdma = DynamicCast<HePpdu>(ppdu->Copy()); // since flag will be modified
    ppduOfdma->SetTxPsdFlag(HePpdu::PSD_HE_PORTION);
    const auto band = m_phy->GetHePhy()->GetRuBandForRx(txVector, staId);
    auto rxPsdOfdma =
        WifiSpectrumValueHelper::CreateHeMuOfdmTxPowerSpectralDensity({DEFAULT_FREQUENCY},
                                                                      DEFAULT_CHANNEL_WIDTH,
                                                                      txPower,
                                                                      DEFAULT_GUARD_WIDTH,
                                                                      band.indices);
    auto rxParamsOfdma = Create<WifiSpectrumSignalParameters>();
    rxParamsOfdma->psd = rxPsd;
    rxParamsOfdma->txPhy = nullptr;
    rxParamsOfdma->duration = ppduDuration - nonOfdmaDuration;
    rxParamsOfdma->ppdu = ppduOfdma;
    Simulator::Schedule(nonOfdmaDuration,
                        &TestMultipleHeTbPreambles::RxHeTbPpduOfdmaPart,
                        this,
                        rxParamsOfdma);
}

void
TestMultipleHeTbPreambles::RxHeTbPpduOfdmaPart(Ptr<WifiSpectrumSignalParameters> rxParamsOfdma)
{
    Simulator::ScheduleNow(&TestMultipleHeTbPreambles::DoRxHeTbPpduOfdmaPart, this, rxParamsOfdma);
}

void
TestMultipleHeTbPreambles::DoRxHeTbPpduOfdmaPart(Ptr<WifiSpectrumSignalParameters> rxParamsOfdma)
{
    // This is needed to make sure the OFDMA part is started as the last event since HE-SIG-A should
    // end at the exact same time as the start For normal WifiNetDevices, this the reception of the
    // OFDMA part is scheduled after end of HE-SIG-A decoding.
    m_phy->StartRx(rxParamsOfdma, nullptr);
}

void
TestMultipleHeTbPreambles::DoSetup()
{
    Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<Node> node = CreateObject<Node>();
    Ptr<WifiNetDevice> dev = CreateObject<WifiNetDevice>();
    dev->SetStandard(WIFI_STANDARD_80211ax);
    m_phy = CreateObject<OfdmaSpectrumWifiPhy>(0);
    Ptr<InterferenceHelper> interferenceHelper = CreateObject<InterferenceHelper>();
    Ptr<ErrorRateModel> error = CreateObject<NistErrorRateModel>();
    auto mac = CreateObjectWithAttributes<ApWifiMac>(
        "Txop",
        PointerValue(CreateObjectWithAttributes<Txop>("AcIndex", StringValue("AC_BE_NQOS"))));
    mac->SetAttribute("BeaconGeneration", BooleanValue(false));
    dev->SetMac(mac);
    m_phy->SetInterferenceHelper(interferenceHelper);
    m_phy->SetErrorRateModel(error);
    m_phy->AddChannel(spectrumChannel);
    m_phy->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phy->SetOperatingChannel(WifiPhy::ChannelTuple{DEFAULT_CHANNEL_NUMBER,
                                                     DEFAULT_CHANNEL_WIDTH,
                                                     WIFI_PHY_BAND_5GHZ,
                                                     0});
    m_phy->TraceConnectWithoutContext("PhyRxDrop",
                                      MakeCallback(&TestMultipleHeTbPreambles::RxDropped, this));
    m_phy->SetDevice(dev);
    Ptr<ThresholdPreambleDetectionModel> preambleDetectionModel =
        CreateObject<ThresholdPreambleDetectionModel>();
    preambleDetectionModel->SetAttribute("Threshold", DoubleValue(4));
    preambleDetectionModel->SetAttribute("MinimumRssi", DoubleValue(-82));
    m_phy->SetPreambleDetectionModel(preambleDetectionModel);
    Ptr<HeConfiguration> heConfiguration = CreateObject<HeConfiguration>();
    heConfiguration->m_maxTbPpduDelay = NanoSeconds(400);
    dev->SetHeConfiguration(heConfiguration);
    dev->SetPhy(m_phy);
    node->AddDevice(dev);
}

void
TestMultipleHeTbPreambles::DoTeardown()
{
    m_phy->Dispose();
    m_phy = nullptr;
}

void
TestMultipleHeTbPreambles::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 0;
    m_phy->AssignStreams(streamNumber);

    Watt_u txPower{0.01};

    {
        // Verify a single UL MU transmission with two stations belonging to the same BSS
        std::vector<uint64_t> uids{0};
        Simulator::Schedule(Seconds(1),
                            &TestMultipleHeTbPreambles::RxHeTbPpdu,
                            this,
                            uids[0],
                            1,
                            txPower,
                            1001);
        Simulator::Schedule(Seconds(1) + NanoSeconds(100),
                            &TestMultipleHeTbPreambles::RxHeTbPpdu,
                            this,
                            uids[0],
                            2,
                            txPower,
                            1002);
        // Check that we received a single UL MU transmission with the corresponding UID
        Simulator::Schedule(Seconds(1) + MicroSeconds(1),
                            &TestMultipleHeTbPreambles::CheckHeTbPreambles,
                            this,
                            1,
                            uids);
        Simulator::Schedule(Seconds(1.5), &TestMultipleHeTbPreambles::Reset, this);
    }

    {
        // Verify the correct reception of 2 UL MU transmissions with two stations per BSS, where
        // the second transmission arrives during the preamble detection window and with half the
        // power of the first transmission.
        std::vector<uint64_t> uids{1, 2};
        Simulator::Schedule(Seconds(2),
                            &TestMultipleHeTbPreambles::RxHeTbPpdu,
                            this,
                            uids[0],
                            1,
                            txPower,
                            1001);
        Simulator::Schedule(Seconds(2) + NanoSeconds(100),
                            &TestMultipleHeTbPreambles::RxHeTbPpdu,
                            this,
                            uids[0],
                            2,
                            txPower,
                            1002);
        Simulator::Schedule(Seconds(2) + NanoSeconds(200),
                            &TestMultipleHeTbPreambles::RxHeTbPpdu,
                            this,
                            uids[1],
                            1,
                            txPower / 2,
                            1003);
        Simulator::Schedule(Seconds(2) + NanoSeconds(300),
                            &TestMultipleHeTbPreambles::RxHeTbPpdu,
                            this,
                            uids[1],
                            2,
                            txPower / 2,
                            1004);
        // Check that we received the correct reception of 2 UL MU transmissions with the
        // corresponding UIDs
        Simulator::Schedule(Seconds(2) + MicroSeconds(1),
                            &TestMultipleHeTbPreambles::CheckHeTbPreambles,
                            this,
                            2,
                            uids);
        Simulator::Schedule(Seconds(2.5), &TestMultipleHeTbPreambles::Reset, this);
        // TODO: verify PPDUs from second UL MU transmission are dropped
    }

    {
        // Verify the correct reception of 2 UL MU transmissions with two stations per BSS, where
        // the second transmission arrives during the preamble detection window and with twice the
        // power of the first transmission.
        std::vector<uint64_t> uids{3, 4};
        Simulator::Schedule(Seconds(3),
                            &TestMultipleHeTbPreambles::RxHeTbPpdu,
                            this,
                            uids[0],
                            1,
                            txPower / 2,
                            1001);
        Simulator::Schedule(Seconds(3) + NanoSeconds(100),
                            &TestMultipleHeTbPreambles::RxHeTbPpdu,
                            this,
                            uids[0],
                            2,
                            txPower / 2,
                            1002);
        Simulator::Schedule(Seconds(3) + NanoSeconds(200),
                            &TestMultipleHeTbPreambles::RxHeTbPpdu,
                            this,
                            uids[1],
                            1,
                            txPower,
                            1003);
        Simulator::Schedule(Seconds(3) + NanoSeconds(300),
                            &TestMultipleHeTbPreambles::RxHeTbPpdu,
                            this,
                            uids[1],
                            2,
                            txPower,
                            1004);
        // Check that we received the correct reception of 2 UL MU transmissions with the
        // corresponding UIDs
        Simulator::Schedule(Seconds(3) + MicroSeconds(1),
                            &TestMultipleHeTbPreambles::CheckHeTbPreambles,
                            this,
                            2,
                            uids);
        Simulator::Schedule(Seconds(3.5), &TestMultipleHeTbPreambles::Reset, this);
        // TODO: verify PPDUs from first UL MU transmission are dropped
    }

    {
        // Verify the correct reception of 2 UL MU transmissions with two stations per BSS, where
        // the second transmission arrives during PHY header reception and with the same power as
        // the first transmission.
        std::vector<uint64_t> uids{5, 6};
        Simulator::Schedule(Seconds(4),
                            &TestMultipleHeTbPreambles::RxHeTbPpdu,
                            this,
                            uids[0],
                            1,
                            txPower,
                            1001);
        Simulator::Schedule(Seconds(4) + NanoSeconds(100),
                            &TestMultipleHeTbPreambles::RxHeTbPpdu,
                            this,
                            uids[0],
                            2,
                            txPower,
                            1002);
        Simulator::Schedule(Seconds(4) + MicroSeconds(5),
                            &TestMultipleHeTbPreambles::RxHeTbPpdu,
                            this,
                            uids[1],
                            1,
                            txPower,
                            1003);
        Simulator::Schedule(Seconds(4) + MicroSeconds(5) + NanoSeconds(100),
                            &TestMultipleHeTbPreambles::RxHeTbPpdu,
                            this,
                            uids[1],
                            2,
                            txPower,
                            1004);
        // Check that we received the correct reception of the first UL MU transmission with the
        // corresponding UID (second one dropped)
        Simulator::Schedule(Seconds(4) + MicroSeconds(10),
                            &TestMultipleHeTbPreambles::CheckHeTbPreambles,
                            this,
                            1,
                            std::vector<uint64_t>{uids[0]});
        // The packets of the second UL MU transmission should have been dropped
        Simulator::Schedule(Seconds(4) + MicroSeconds(10),
                            &TestMultipleHeTbPreambles::CheckBytesDropped,
                            this,
                            1003 + 1004);
        Simulator::Schedule(Seconds(4.5), &TestMultipleHeTbPreambles::Reset, this);
    }

    {
        // Verify the correct reception of one UL MU transmission out of 2 with two stations per
        // BSS, where the second transmission arrives during payload reception and with the same
        // power as the first transmission.
        std::vector<uint64_t> uids{7, 8};
        Simulator::Schedule(Seconds(5),
                            &TestMultipleHeTbPreambles::RxHeTbPpdu,
                            this,
                            uids[0],
                            1,
                            txPower,
                            1001);
        Simulator::Schedule(Seconds(5) + NanoSeconds(100),
                            &TestMultipleHeTbPreambles::RxHeTbPpdu,
                            this,
                            uids[0],
                            2,
                            txPower,
                            1002);
        Simulator::Schedule(Seconds(5) + MicroSeconds(50),
                            &TestMultipleHeTbPreambles::RxHeTbPpdu,
                            this,
                            uids[1],
                            1,
                            txPower,
                            1003);
        Simulator::Schedule(Seconds(5) + MicroSeconds(50) + NanoSeconds(100),
                            &TestMultipleHeTbPreambles::RxHeTbPpdu,
                            this,
                            uids[1],
                            2,
                            txPower,
                            1004);
        // Check that we received the correct reception of the first UL MU transmission with the
        // corresponding UID (second one dropped)
        Simulator::Schedule(Seconds(5) + MicroSeconds(100),
                            &TestMultipleHeTbPreambles::CheckHeTbPreambles,
                            this,
                            1,
                            std::vector<uint64_t>{uids[0]});
        // The packets of the second UL MU transmission should have been dropped
        Simulator::Schedule(Seconds(5) + MicroSeconds(100),
                            &TestMultipleHeTbPreambles::CheckBytesDropped,
                            this,
                            1003 + 1004);
        Simulator::Schedule(Seconds(5.5), &TestMultipleHeTbPreambles::Reset, this);
    }

    {
        // Verify the correct reception of a single UL MU transmission with two stations belonging
        // to the same BSS, and the second PPDU arrives 500ns after the first PPDU, i.e. it exceeds
        // the configured delay spread of 400ns
        std::vector<uint64_t> uids{9};
        Simulator::Schedule(Seconds(6),
                            &TestMultipleHeTbPreambles::RxHeTbPpdu,
                            this,
                            uids[0],
                            1,
                            txPower,
                            1001);
        Simulator::Schedule(Seconds(6) + NanoSeconds(500),
                            &TestMultipleHeTbPreambles::RxHeTbPpdu,
                            this,
                            uids[0],
                            2,
                            txPower,
                            1002);
        // Check that we received a single UL MU transmission with the corresponding UID
        Simulator::Schedule(Seconds(6) + MicroSeconds(1),
                            &TestMultipleHeTbPreambles::CheckHeTbPreambles,
                            this,
                            1,
                            uids);
        // The first packet of 1001 bytes should be dropped because preamble is not detected after
        // 4us (because the PPDU that arrived at 500ns is interfering): the second HE TB PPDU is
        // acting as interference since it arrived after the maximum allowed 400ns. Obviously, that
        // second packet of 1002 bytes is dropped as well.
        Simulator::Schedule(Seconds(6) + MicroSeconds(5),
                            &TestMultipleHeTbPreambles::CheckBytesDropped,
                            this,
                            1001 + 1002);
        Simulator::Schedule(Seconds(6.5), &TestMultipleHeTbPreambles::Reset, this);
    }

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief PHY listener for OFDMA tests
 */
class OfdmaTestPhyListener : public ns3::WifiPhyListener
{
  public:
    OfdmaTestPhyListener() = default;

    void NotifyRxStart(Time duration) override
    {
        NS_LOG_FUNCTION(this << duration);
        m_lastRxStart = Simulator::Now();
        ++m_notifyRxStart;
        m_lastRxSuccess = false;
    }

    void NotifyRxEndOk() override
    {
        NS_LOG_FUNCTION(this);
        m_lastRxEnd = Simulator::Now();
        ++m_notifyRxEnd;
        m_lastRxSuccess = true;
    }

    void NotifyRxEndError() override
    {
        NS_LOG_FUNCTION(this);
        m_lastRxEnd = Simulator::Now();
        ++m_notifyRxEnd;
        m_lastRxSuccess = false;
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
     * Reset function.
     */
    void Reset()
    {
        m_notifyRxStart = 0;
        m_notifyRxEnd = 0;
        m_lastRxStart = Seconds(0);
        m_lastRxEnd = Seconds(0);
        m_lastRxSuccess = false;
    }

    /**
     * Return the number of RX start notifications that has been received since the last reset.
     * @return the number of RX start notifications that has been received
     */
    uint32_t GetNumRxStartNotifications() const
    {
        return m_notifyRxStart;
    }

    /**
     * Return the number of RX end notifications that has been received since the last reset.
     * @return the number of RX end notifications that has been received
     */
    uint32_t GetNumRxEndNotifications() const
    {
        return m_notifyRxEnd;
    }

    /**
     * Return the time at which the last RX start notification has been received.
     * @return the time at which the last RX start notification has been received
     */
    Time GetLastRxStartNotification() const
    {
        return m_lastRxStart;
    }

    /**
     * Return the time at which the last RX end notification has been received.
     * @return the time at which the last RX end notification has been received
     */
    Time GetLastRxEndNotification() const
    {
        return m_lastRxEnd;
    }

    /**
     * Return whether last RX has been successful.
     * @return true if last RX has been successful, false otherwise
     */
    bool IsLastRxSuccess() const
    {
        return m_lastRxSuccess;
    }

  private:
    uint32_t m_notifyRxStart{0};    ///< count number of RX start notifications
    uint32_t m_notifyRxEnd{0};      ///< count number of RX end notifications
    Time m_lastRxStart{Seconds(0)}; ///< last time a RX start notification has been received
    Time m_lastRxEnd{Seconds(0)};   ///< last time a RX end notification has been received
    bool m_lastRxSuccess{false};    ///< flag whether last RX has been successful
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief UL-OFDMA PHY test
 */
class TestUlOfdmaPhyTransmission : public TestCase
{
  public:
    /**
     * Erroneous info included in a TRIGVECTOR
     */
    enum TrigVectorInfo
    {
        NONE = 0,
        CHANNEL_WIDTH,
        UL_LENGTH,
        AID,
    };

    TestUlOfdmaPhyTransmission();
    ~TestUlOfdmaPhyTransmission() override;

  private:
    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     * Get TXVECTOR for HE TB PPDU.
     * @param txStaId the ID of the TX STA
     * @param index the RU index used for the transmission
     * @param bssColor the BSS color of the TX STA
     * @return the TXVECTOR for HE TB PPDU
     */
    WifiTxVector GetTxVectorForHeTbPpdu(uint16_t txStaId,
                                        std::size_t index,
                                        uint8_t bssColor) const;
    /**
     * Set TRIGVECTOR for HE TB PPDU
     *
     * @param bssColor the BSS color of the TX STA
     * @param error the erroneous info (if any) in the TRIGVECTOR to set
     */
    void SetTrigVector(uint8_t bssColor, TrigVectorInfo error);
    /**
     * Send HE TB PPDU function
     * @param txStaId the ID of the TX STA
     * @param index the RU index used for the transmission
     * @param payloadSize the size of the payload in bytes
     * @param uid the UID of the trigger frame that is initiating this transmission
     * @param bssColor the BSS color of the TX STA
     * @param incrementUid whether UID shall be incremented
     */
    void SendHeTbPpdu(uint16_t txStaId,
                      std::size_t index,
                      std::size_t payloadSize,
                      uint64_t uid,
                      uint8_t bssColor,
                      bool incrementUid);

    /**
     * Send HE SU PPDU function
     * @param txStaId the ID of the TX STA
     * @param payloadSize the size of the payload in bytes
     * @param uid the UID of the trigger frame that is initiating this transmission
     * @param bssColor the BSS color of the TX STA
     */
    void SendHeSuPpdu(uint16_t txStaId, std::size_t payloadSize, uint64_t uid, uint8_t bssColor);

    /**
     * Set the BSS color
     * @param phy the PHY
     * @param bssColor the BSS color
     */
    void SetBssColor(Ptr<WifiPhy> phy, uint8_t bssColor);

    /**
     * Set the PSD limit
     * @param phy the PHY
     * @param psdLimit the PSD limit
     */
    void SetPsdLimit(Ptr<WifiPhy> phy, dBm_per_MHz_u psdLimit);

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
     * Run one function
     */
    void RunOne();

    /**
     * Check the received PSDUs from STA1
     * @param expectedSuccess the expected number of success
     * @param expectedFailures the expected number of failures
     * @param expectedBytes the expected number of bytes
     */
    void CheckRxFromSta1(uint32_t expectedSuccess,
                         uint32_t expectedFailures,
                         uint32_t expectedBytes);

    /**
     * Check the received PSDUs from STA2
     * @param expectedSuccess the expected number of success
     * @param expectedFailures the expected number of failures
     * @param expectedBytes the expected number of bytes
     */
    void CheckRxFromSta2(uint32_t expectedSuccess,
                         uint32_t expectedFailures,
                         uint32_t expectedBytes);

    /**
     * Check the received power for the non-OFDMA of the HE TB PPDUs over the given band
     * @param phy the PHY
     * @param band the indices of the band over which the power is measured
     * @param expectedRxPower the expected received power
     */
    void CheckNonOfdmaRxPower(Ptr<OfdmaSpectrumWifiPhy> phy,
                              WifiSpectrumBandInfo band,
                              Watt_u expectedRxPower);
    /**
     * Check the received power for the OFDMA part of the HE TB PPDUs over the given band
     * @param phy the PHY
     * @param band the indices of the band over which the power is measured
     * @param expectedRxPower the expected received power
     */
    void CheckOfdmaRxPower(Ptr<OfdmaSpectrumWifiPhy> phy,
                           WifiSpectrumBandInfo band,
                           Watt_u expectedRxPower);

    /**
     * Verify all events are cleared at end of TX or RX
     */
    void VerifyEventsCleared();

    /**
     * Check the PHY state
     * @param phy the PHY
     * @param expectedState the expected state of the PHY
     */
    void CheckPhyState(Ptr<OfdmaSpectrumWifiPhy> phy, WifiPhyState expectedState);
    /// @copydoc CheckPhyState
    void DoCheckPhyState(Ptr<OfdmaSpectrumWifiPhy> phy, WifiPhyState expectedState);

    /**
     * Check the the number of RX start notifications at the AP as well as the last time a RX start
     * has been notified
     * @param expectedNotifications the expected number of RX start notifications at the AP
     * @param expectedLastNotification the expected time of the last RX start notification at the AP
     */
    void CheckApRxStart(uint32_t expectedNotifications, Time expectedLastNotification);
    /**
     * Check the the number of RX end notifications at the AP as well as the last time a RX end has
     * been notified
     * @param expectedNotifications the expected number of RX end notifications at the AP
     * @param expectedLastNotification the expected time of the last RX end notification at the AP
     * @param expectedSuccess true if the last RX notification indicates a success, false otherwise
     */
    void CheckApRxEnd(uint32_t expectedNotifications,
                      Time expectedLastNotification,
                      bool expectedSuccess);

    /**
     * Reset function
     */
    void Reset();

    /**
     * Receive success function
     * @param psdu the PSDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector the transmit vector
     * @param statusPerMpdu reception status per MPDU
     */
    void RxSuccess(Ptr<const WifiPsdu> psdu,
                   RxSignalInfo rxSignalInfo,
                   const WifiTxVector& txVector,
                   const std::vector<bool>& statusPerMpdu);

    /**
     * Receive failure function
     * @param psdu the PSDU
     */
    void RxFailure(Ptr<const WifiPsdu> psdu);

    /**
     * Schedule test to perform.
     * The interference generation should be scheduled apart.
     *
     * @param delay the reference delay to schedule the events
     * @param solicited flag indicating if HE TB PPDUs were solicited by the AP
     * @param expectedStateAtEnd the expected state of the PHY at the end of the reception
     * @param expectedSuccessFromSta1 the expected number of success from STA 1
     * @param expectedFailuresFromSta1 the expected number of failures from STA 1
     * @param expectedBytesFromSta1 the expected number of bytes from STA 1
     * @param expectedSuccessFromSta2 the expected number of success from STA 2
     * @param expectedFailuresFromSta2 the expected number of failures from STA 2
     * @param expectedBytesFromSta2 the expected number of bytes from STA 2
     * @param scheduleTxSta1 flag indicating to schedule a HE TB PPDU from STA 1
     * @param ulTimeDifference delay between HE TB PPDU from STA 1 and HE TB PPDU from STA 2
     * are received
     * @param expectedStateBeforeEnd the expected state of the PHY before the end of the
     * transmission
     * @param error the erroneous info (if any) in the TRIGVECTOR to set
     */
    void ScheduleTest(Time delay,
                      bool solicited,
                      WifiPhyState expectedStateAtEnd,
                      uint32_t expectedSuccessFromSta1,
                      uint32_t expectedFailuresFromSta1,
                      uint32_t expectedBytesFromSta1,
                      uint32_t expectedSuccessFromSta2,
                      uint32_t expectedFailuresFromSta2,
                      uint32_t expectedBytesFromSta2,
                      bool scheduleTxSta1 = true,
                      Time ulTimeDifference = Seconds(0),
                      WifiPhyState expectedStateBeforeEnd = WifiPhyState::RX,
                      TrigVectorInfo error = NONE);

    /**
     * Schedule power measurement related checks.
     *
     * @param delay the reference delay used to schedule the events
     * @param rxPowerNonOfdmaRu1 the received power on the non-OFDMA part of RU1
     * @param rxPowerNonOfdmaRu2 the received power on the non-OFDMA part of RU2
     * @param rxPowerOfdmaRu1 the received power on RU1
     * @param rxPowerOfdmaRu2 the received power on RU2
     */
    void SchedulePowerMeasurementChecks(Time delay,
                                        Watt_u rxPowerNonOfdmaRu1,
                                        Watt_u rxPowerNonOfdmaRu2,
                                        Watt_u rxPowerOfdmaRu1,
                                        Watt_u rxPowerOfdmaRu2);
    /**
     * Log scenario description
     *
     * @param log the scenario description to add to log
     */
    void LogScenario(std::string log) const;

    Ptr<OfdmaSpectrumWifiPhy> m_phyAp;   ///< PHY of AP
    Ptr<OfdmaSpectrumWifiPhy> m_phySta1; ///< PHY of STA 1
    Ptr<OfdmaSpectrumWifiPhy> m_phySta2; ///< PHY of STA 2
    Ptr<OfdmaSpectrumWifiPhy> m_phySta3; ///< PHY of STA 3

    std::shared_ptr<OfdmaTestPhyListener>
        m_apPhyStateListener; ///< listener for AP PHY state transitions

    Ptr<WaveformGenerator> m_phyInterferer; ///< PHY of interferer

    uint32_t m_countRxSuccessFromSta1; ///< count RX success from STA 1
    uint32_t m_countRxSuccessFromSta2; ///< count RX success from STA 2
    uint32_t m_countRxFailureFromSta1; ///< count RX failure from STA 1
    uint32_t m_countRxFailureFromSta2; ///< count RX failure from STA 2
    uint32_t m_countRxBytesFromSta1;   ///< count RX bytes from STA 1
    uint32_t m_countRxBytesFromSta2;   ///< count RX bytes from STA 2

    MHz_u m_frequency;           ///< frequency
    MHz_u m_channelWidth;        ///< channel width
    Time m_expectedPpduDuration; ///< expected duration to send MU PPDU
};

TestUlOfdmaPhyTransmission::TestUlOfdmaPhyTransmission()
    : TestCase("UL-OFDMA PHY test"),
      m_countRxSuccessFromSta1(0),
      m_countRxSuccessFromSta2(0),
      m_countRxFailureFromSta1(0),
      m_countRxFailureFromSta2(0),
      m_countRxBytesFromSta1(0),
      m_countRxBytesFromSta2(0),
      m_frequency(DEFAULT_FREQUENCY),
      m_channelWidth(DEFAULT_CHANNEL_WIDTH),
      m_expectedPpduDuration(NanoSeconds(271200))
{
}

void
TestUlOfdmaPhyTransmission::SendHeSuPpdu(uint16_t txStaId,
                                         std::size_t payloadSize,
                                         uint64_t uid,
                                         uint8_t bssColor)
{
    NS_LOG_FUNCTION(this << txStaId << payloadSize << uid << +bssColor);
    WifiConstPsduMap psdus;

    WifiTxVector txVector{HePhy::GetHeMcs7(),
                          0,
                          WIFI_PREAMBLE_HE_SU,
                          NanoSeconds(800),
                          1,
                          1,
                          0,
                          m_channelWidth,
                          false,
                          false,
                          false,
                          bssColor};

    auto pkt = Create<Packet>(payloadSize);
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_QOSDATA);
    hdr.SetQosTid(0);
    hdr.SetAddr1(Mac48Address("00:00:00:00:00:00"));
    std::ostringstream addr;
    addr << "00:00:00:00:00:0" << txStaId;
    hdr.SetAddr2(Mac48Address(addr.str().c_str()));
    hdr.SetSequenceNumber(1);
    auto psdu = Create<WifiPsdu>(pkt, hdr);
    psdus.insert(std::make_pair(SU_STA_ID, psdu));

    Ptr<OfdmaSpectrumWifiPhy> phy;
    if (txStaId == 1)
    {
        phy = m_phySta1;
    }
    else if (txStaId == 2)
    {
        phy = m_phySta2;
    }
    else if (txStaId == 3)
    {
        phy = m_phySta3;
    }
    else if (txStaId == 0)
    {
        phy = m_phyAp;
    }
    phy->SetPpduUid(uid);
    phy->Send(psdus, txVector);
}

WifiTxVector
TestUlOfdmaPhyTransmission::GetTxVectorForHeTbPpdu(uint16_t txStaId,
                                                   std::size_t index,
                                                   uint8_t bssColor) const
{
    WifiTxVector txVector{HePhy::GetHeMcs7(),
                          0,
                          WIFI_PREAMBLE_HE_TB,
                          NanoSeconds(1600),
                          1,
                          1,
                          0,
                          m_channelWidth,
                          false,
                          false,
                          false,
                          bssColor};

    auto ruType = HeRu::RU_106_TONE;
    if (m_channelWidth == MHz_u{20})
    {
        ruType = HeRu::RU_106_TONE;
    }
    else if (m_channelWidth == MHz_u{40})
    {
        ruType = HeRu::RU_242_TONE;
    }
    else if (m_channelWidth == MHz_u{80})
    {
        ruType = HeRu::RU_484_TONE;
    }
    else if (m_channelWidth == MHz_u{160})
    {
        ruType = HeRu::RU_996_TONE;
    }
    else
    {
        NS_ASSERT_MSG(false, "Unsupported channel width");
    }

    auto primary80MHz = true;
    if (m_channelWidth == MHz_u{160} && index == 2)
    {
        primary80MHz = false;
        index = 1;
    }
    HeRu::RuSpec ru(ruType, index, primary80MHz);
    txVector.SetRu(ru, txStaId);
    txVector.SetMode(HePhy::GetHeMcs7(), txStaId);
    txVector.SetNss(1, txStaId);
    return txVector;
}

void
TestUlOfdmaPhyTransmission::SetTrigVector(uint8_t bssColor, TrigVectorInfo error)
{
    auto channelWidth = m_channelWidth;
    if (error == CHANNEL_WIDTH)
    {
        channelWidth = (channelWidth == MHz_u{160} ? MHz_u{20} : channelWidth * 2);
    }

    WifiTxVector txVector(HePhy::GetHeMcs7(),
                          0,
                          WIFI_PREAMBLE_HE_TB,
                          NanoSeconds(1600),
                          1,
                          1,
                          0,
                          channelWidth,
                          false,
                          false,
                          false,
                          bssColor);

    HeRu::RuType ruType = HeRu::RU_106_TONE;
    if (channelWidth == MHz_u{20})
    {
        ruType = HeRu::RU_106_TONE;
    }
    else if (channelWidth == MHz_u{40})
    {
        ruType = HeRu::RU_242_TONE;
    }
    else if (channelWidth == MHz_u{80})
    {
        ruType = HeRu::RU_484_TONE;
    }
    else if (channelWidth == MHz_u{160})
    {
        ruType = HeRu::RU_996_TONE;
    }
    else
    {
        NS_ASSERT_MSG(false, "Unsupported channel width");
    }

    uint16_t aid1 = (error == AID ? 3 : 1);
    uint16_t aid2 = (error == AID ? 4 : 2);

    HeRu::RuSpec ru1(ruType, 1, true);
    txVector.SetRu(ru1, aid1);
    txVector.SetMode(HePhy::GetHeMcs7(), aid1);
    txVector.SetNss(1, aid1);

    HeRu::RuSpec ru2(ruType, (channelWidth == MHz_u{160} ? 1 : 2), (channelWidth != MHz_u{160}));
    txVector.SetRu(ru2, aid2);
    txVector.SetMode(HePhy::GetHeMcs7(), aid2);
    txVector.SetNss(1, aid2);

    uint16_t length;
    std::tie(length, m_expectedPpduDuration) =
        HePhy::ConvertHeTbPpduDurationToLSigLength(m_expectedPpduDuration,
                                                   txVector,
                                                   m_phyAp->GetPhyBand());
    if (error == UL_LENGTH)
    {
        ++length;
    }
    txVector.SetLength(length);
    auto hePhyAp = DynamicCast<HePhy>(m_phyAp->GetLatestPhyEntity());
    hePhyAp->SetTrigVector(txVector, m_expectedPpduDuration);
}

void
TestUlOfdmaPhyTransmission::SendHeTbPpdu(uint16_t txStaId,
                                         std::size_t index,
                                         std::size_t payloadSize,
                                         uint64_t uid,
                                         uint8_t bssColor,
                                         bool incrementUid)
{
    NS_LOG_FUNCTION(this << txStaId << index << payloadSize << uid << +bssColor << (incrementUid));
    WifiConstPsduMap psdus;

    if (incrementUid)
    {
        ++uid;
    }

    WifiTxVector txVector = GetTxVectorForHeTbPpdu(txStaId, index, bssColor);
    Ptr<Packet> pkt = Create<Packet>(payloadSize);
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_QOSDATA);
    hdr.SetQosTid(0);
    hdr.SetAddr1(Mac48Address("00:00:00:00:00:00"));
    std::ostringstream addr;
    addr << "00:00:00:00:00:0" << txStaId;
    hdr.SetAddr2(Mac48Address(addr.str().c_str()));
    hdr.SetSequenceNumber(1);
    Ptr<WifiPsdu> psdu = Create<WifiPsdu>(pkt, hdr);
    psdus.insert(std::make_pair(txStaId, psdu));

    Ptr<OfdmaSpectrumWifiPhy> phy;
    if (txStaId == 1)
    {
        phy = m_phySta1;
    }
    else if (txStaId == 2)
    {
        phy = m_phySta2;
    }
    else if (txStaId == 3)
    {
        phy = m_phySta3;
    }

    Time txDuration = OfdmaSpectrumWifiPhy::CalculateTxDuration(psdu->GetSize(),
                                                                txVector,
                                                                phy->GetPhyBand(),
                                                                txStaId);
    txVector.SetLength(
        HePhy::ConvertHeTbPpduDurationToLSigLength(txDuration, txVector, phy->GetPhyBand()).first);

    phy->SetPpduUid(uid);
    phy->Send(psdus, txVector);
}

void
TestUlOfdmaPhyTransmission::GenerateInterference(Ptr<SpectrumValue> interferencePsd, Time duration)
{
    NS_LOG_FUNCTION(this << duration);
    m_phyInterferer->SetTxPowerSpectralDensity(interferencePsd);
    m_phyInterferer->SetPeriod(duration);
    m_phyInterferer->Start();
    Simulator::Schedule(duration, &TestUlOfdmaPhyTransmission::StopInterference, this);
}

void
TestUlOfdmaPhyTransmission::StopInterference()
{
    m_phyInterferer->Stop();
}

TestUlOfdmaPhyTransmission::~TestUlOfdmaPhyTransmission()
{
}

void
TestUlOfdmaPhyTransmission::RxSuccess(Ptr<const WifiPsdu> psdu,
                                      RxSignalInfo rxSignalInfo,
                                      const WifiTxVector& txVector,
                                      const std::vector<bool>& /*statusPerMpdu*/)
{
    NS_LOG_FUNCTION(this << *psdu << psdu->GetAddr2() << rxSignalInfo << txVector);
    if (psdu->GetAddr2() == Mac48Address("00:00:00:00:00:01"))
    {
        m_countRxSuccessFromSta1++;
        m_countRxBytesFromSta1 += (psdu->GetSize() - 30);
    }
    else if (psdu->GetAddr2() == Mac48Address("00:00:00:00:00:02"))
    {
        m_countRxSuccessFromSta2++;
        m_countRxBytesFromSta2 += (psdu->GetSize() - 30);
    }
}

void
TestUlOfdmaPhyTransmission::RxFailure(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << *psdu << psdu->GetAddr2());
    if (psdu->GetAddr2() == Mac48Address("00:00:00:00:00:01"))
    {
        m_countRxFailureFromSta1++;
    }
    else if (psdu->GetAddr2() == Mac48Address("00:00:00:00:00:02"))
    {
        m_countRxFailureFromSta2++;
    }
}

void
TestUlOfdmaPhyTransmission::CheckRxFromSta1(uint32_t expectedSuccess,
                                            uint32_t expectedFailures,
                                            uint32_t expectedBytes)
{
    NS_TEST_ASSERT_MSG_EQ(m_countRxSuccessFromSta1,
                          expectedSuccess,
                          "The number of successfully received packets from STA 1 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(
        m_countRxFailureFromSta1,
        expectedFailures,
        "The number of unsuccessfuly received packets from STA 1 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxBytesFromSta1,
                          expectedBytes,
                          "The number of bytes received from STA 1 is not correct!");
}

void
TestUlOfdmaPhyTransmission::CheckRxFromSta2(uint32_t expectedSuccess,
                                            uint32_t expectedFailures,
                                            uint32_t expectedBytes)
{
    NS_TEST_ASSERT_MSG_EQ(m_countRxSuccessFromSta2,
                          expectedSuccess,
                          "The number of successfully received packets from STA 2 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(
        m_countRxFailureFromSta2,
        expectedFailures,
        "The number of unsuccessfuly received packets from STA 2 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxBytesFromSta2,
                          expectedBytes,
                          "The number of bytes received from STA 2 is not correct!");
}

void
TestUlOfdmaPhyTransmission::CheckNonOfdmaRxPower(Ptr<OfdmaSpectrumWifiPhy> phy,
                                                 WifiSpectrumBandInfo band,
                                                 Watt_u expectedRxPower)
{
    auto event = phy->GetCurrentEvent();
    NS_ASSERT(event);
    auto rxPower = event->GetRxPower(band);
    NS_LOG_FUNCTION(this << band << expectedRxPower << rxPower);
    // Since there is out of band emission due to spectrum mask, the tolerance cannot be very low
    NS_TEST_ASSERT_MSG_EQ_TOL(rxPower,
                              expectedRxPower,
                              Watt_u{5e-3},
                              "RX power " << rxPower << " over (" << band
                                          << ") does not match expected power " << expectedRxPower
                                          << " at " << Simulator::Now());
}

void
TestUlOfdmaPhyTransmission::CheckOfdmaRxPower(Ptr<OfdmaSpectrumWifiPhy> phy,
                                              WifiSpectrumBandInfo band,
                                              Watt_u expectedRxPower)
{
    /**
     * The current event cannot be used since it points to the preamble part of the HE TB PPDU.
     * We will have to check if the expected power is indeed the max power returning a positive
     * duration when calling GetEnergyDuration.
     */
    NS_LOG_FUNCTION(this << band << expectedRxPower);
    Watt_u step{5e-3};
    if (expectedRxPower > Watt_u{0.0})
    {
        NS_TEST_ASSERT_MSG_EQ(
            phy->GetEnergyDuration(expectedRxPower - step, band).IsStrictlyPositive(),
            true,
            "At least " << expectedRxPower << " W expected for OFDMA part over (" << band << ") at "
                        << Simulator::Now());
        NS_TEST_ASSERT_MSG_EQ(
            phy->GetEnergyDuration(expectedRxPower + step, band).IsStrictlyPositive(),
            false,
            "At most " << expectedRxPower << " W expected for OFDMA part over (" << band << ") at "
                       << Simulator::Now());
    }
    else
    {
        NS_TEST_ASSERT_MSG_EQ(
            phy->GetEnergyDuration(expectedRxPower + step, band).IsStrictlyPositive(),
            false,
            "At most " << expectedRxPower << " W expected for OFDMA part over (" << band << ") at "
                       << Simulator::Now());
    }
}

void
TestUlOfdmaPhyTransmission::VerifyEventsCleared()
{
    NS_TEST_ASSERT_MSG_EQ(m_phyAp->GetCurrentEvent(),
                          nullptr,
                          "m_currentEvent for AP was not cleared");
    NS_TEST_ASSERT_MSG_EQ(m_phySta1->GetCurrentEvent(),
                          nullptr,
                          "m_currentEvent for STA 1 was not cleared");
    NS_TEST_ASSERT_MSG_EQ(m_phySta2->GetCurrentEvent(),
                          nullptr,
                          "m_currentEvent for STA 2 was not cleared");
}

void
TestUlOfdmaPhyTransmission::CheckPhyState(Ptr<OfdmaSpectrumWifiPhy> phy, WifiPhyState expectedState)
{
    // This is needed to make sure PHY state will be checked as the last event if a state change
    // occurred at the exact same time as the check
    Simulator::ScheduleNow(&TestUlOfdmaPhyTransmission::DoCheckPhyState, this, phy, expectedState);
}

void
TestUlOfdmaPhyTransmission::DoCheckPhyState(Ptr<OfdmaSpectrumWifiPhy> phy,
                                            WifiPhyState expectedState)
{
    WifiPhyState currentState;
    PointerValue ptr;
    phy->GetAttribute("State", ptr);
    Ptr<WifiPhyStateHelper> state = DynamicCast<WifiPhyStateHelper>(ptr.Get<WifiPhyStateHelper>());
    currentState = state->GetState();
    NS_LOG_FUNCTION(this << currentState);
    NS_TEST_ASSERT_MSG_EQ(currentState,
                          expectedState,
                          "PHY State " << currentState << " does not match expected state "
                                       << expectedState << " at " << Simulator::Now());
}

void
TestUlOfdmaPhyTransmission::CheckApRxStart(uint32_t expectedNotifications,
                                           Time expectedLastNotification)
{
    NS_TEST_ASSERT_MSG_EQ(m_apPhyStateListener->GetNumRxStartNotifications(),
                          expectedNotifications,
                          "Number of RX start notifications "
                              << m_apPhyStateListener->GetNumRxStartNotifications()
                              << " does not match expected count " << expectedNotifications
                              << " for AP at " << Simulator::Now());
    NS_TEST_ASSERT_MSG_EQ(m_apPhyStateListener->GetLastRxStartNotification(),
                          expectedLastNotification,
                          "Last time RX start notification has been received "
                              << m_apPhyStateListener->GetLastRxStartNotification()
                              << " does not match expected time " << expectedLastNotification
                              << " for AP at " << Simulator::Now());
}

void
TestUlOfdmaPhyTransmission::CheckApRxEnd(uint32_t expectedNotifications,
                                         Time expectedLastNotification,
                                         bool expectedSuccess)
{
    NS_TEST_ASSERT_MSG_EQ(m_apPhyStateListener->GetNumRxEndNotifications(),
                          expectedNotifications,
                          "Number of RX end notifications "
                              << m_apPhyStateListener->GetNumRxEndNotifications()
                              << " does not match expected count " << expectedNotifications
                              << " for AP at " << Simulator::Now());
    NS_TEST_ASSERT_MSG_EQ(m_apPhyStateListener->GetLastRxEndNotification(),
                          expectedLastNotification,
                          "Last time RX end notification has been received "
                              << m_apPhyStateListener->GetLastRxEndNotification()
                              << " does not match expected time " << expectedLastNotification
                              << " for AP at " << Simulator::Now());
    NS_TEST_ASSERT_MSG_EQ(m_apPhyStateListener->IsLastRxSuccess(),
                          expectedSuccess,
                          "Last time RX end notification indicated a "
                              << (m_apPhyStateListener->IsLastRxSuccess() ? "success" : "failure")
                              << " but expected a " << (expectedSuccess ? "success" : "failure")
                              << " for AP at " << Simulator::Now());
}

void
TestUlOfdmaPhyTransmission::Reset()
{
    m_countRxSuccessFromSta1 = 0;
    m_countRxSuccessFromSta2 = 0;
    m_countRxFailureFromSta1 = 0;
    m_countRxFailureFromSta2 = 0;
    m_countRxBytesFromSta1 = 0;
    m_countRxBytesFromSta2 = 0;
    m_phySta1->SetPpduUid(0);
    m_phySta1->SetTriggerFrameUid(0);
    m_phySta2->SetTriggerFrameUid(0);
    SetBssColor(m_phyAp, 0);
    m_apPhyStateListener->Reset();
}

void
TestUlOfdmaPhyTransmission::SetBssColor(Ptr<WifiPhy> phy, uint8_t bssColor)
{
    Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice>(phy->GetDevice());
    Ptr<HeConfiguration> heConfiguration = device->GetHeConfiguration();
    heConfiguration->SetAttribute("BssColor", UintegerValue(bssColor));
}

void
TestUlOfdmaPhyTransmission::SetPsdLimit(Ptr<WifiPhy> phy, dBm_per_MHz_u psdLimit)
{
    NS_LOG_FUNCTION(this << phy << psdLimit);
    phy->SetAttribute("PowerDensityLimit", DoubleValue(psdLimit));
}

void
TestUlOfdmaPhyTransmission::DoSetup()
{
    Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel>();
    lossModel->SetFrequency(m_frequency);
    spectrumChannel->AddPropagationLossModel(lossModel);
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);

    Ptr<ThresholdPreambleDetectionModel> preambleDetectionModel =
        CreateObject<ThresholdPreambleDetectionModel>();
    preambleDetectionModel->SetAttribute(
        "MinimumRssi",
        DoubleValue(
            -8)); // to ensure that transmission in neighboring channel is ignored (16 dBm baseline)
    preambleDetectionModel->SetAttribute("Threshold", DoubleValue(-100)); // no limit on SNR

    Ptr<Node> apNode = CreateObject<Node>();
    Ptr<WifiNetDevice> apDev = CreateObject<WifiNetDevice>();
    apDev->SetStandard(WIFI_STANDARD_80211ax);
    auto apMac = CreateObjectWithAttributes<ApWifiMac>(
        "Txop",
        PointerValue(CreateObjectWithAttributes<Txop>("AcIndex", StringValue("AC_BE_NQOS"))));
    apMac->SetAttribute("BeaconGeneration", BooleanValue(false));
    apDev->SetMac(apMac);
    m_phyAp = CreateObject<OfdmaSpectrumWifiPhy>(0);
    Ptr<HeConfiguration> heConfiguration = CreateObject<HeConfiguration>();
    apDev->SetHeConfiguration(heConfiguration);
    Ptr<InterferenceHelper> apInterferenceHelper = CreateObject<InterferenceHelper>();
    m_phyAp->SetInterferenceHelper(apInterferenceHelper);
    Ptr<ErrorRateModel> apErrorModel = CreateObject<NistErrorRateModel>();
    m_phyAp->SetErrorRateModel(apErrorModel);
    m_phyAp->SetDevice(apDev);
    m_phyAp->AddChannel(spectrumChannel);
    m_phyAp->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phyAp->SetReceiveOkCallback(MakeCallback(&TestUlOfdmaPhyTransmission::RxSuccess, this));
    m_phyAp->SetReceiveErrorCallback(MakeCallback(&TestUlOfdmaPhyTransmission::RxFailure, this));
    m_phyAp->SetPreambleDetectionModel(preambleDetectionModel);
    Ptr<ConstantPositionMobilityModel> apMobility = CreateObject<ConstantPositionMobilityModel>();
    m_phyAp->SetMobility(apMobility);
    m_apPhyStateListener = std::make_unique<OfdmaTestPhyListener>();
    m_phyAp->RegisterListener(m_apPhyStateListener);
    apDev->SetPhy(m_phyAp);
    apMac->SetWifiPhys({m_phyAp});
    apNode->AggregateObject(apMobility);
    apNode->AddDevice(apDev);

    Ptr<Node> sta1Node = CreateObject<Node>();
    Ptr<WifiNetDevice> sta1Dev = CreateObject<WifiNetDevice>();
    sta1Dev->SetStandard(WIFI_STANDARD_80211ax);
    sta1Dev->SetHeConfiguration(CreateObject<HeConfiguration>());
    m_phySta1 = CreateObject<OfdmaSpectrumWifiPhy>(1);
    Ptr<InterferenceHelper> sta1InterferenceHelper = CreateObject<InterferenceHelper>();
    m_phySta1->SetInterferenceHelper(sta1InterferenceHelper);
    Ptr<ErrorRateModel> sta1ErrorModel = CreateObject<NistErrorRateModel>();
    m_phySta1->SetErrorRateModel(sta1ErrorModel);
    m_phySta1->SetDevice(sta1Dev);
    m_phySta1->AddChannel(spectrumChannel);
    m_phySta1->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phySta1->SetPreambleDetectionModel(preambleDetectionModel);
    Ptr<ConstantPositionMobilityModel> sta1Mobility = CreateObject<ConstantPositionMobilityModel>();
    m_phySta1->SetMobility(sta1Mobility);
    sta1Dev->SetPhy(m_phySta1);
    sta1Node->AggregateObject(sta1Mobility);
    sta1Node->AddDevice(sta1Dev);

    Ptr<Node> sta2Node = CreateObject<Node>();
    Ptr<WifiNetDevice> sta2Dev = CreateObject<WifiNetDevice>();
    sta2Dev->SetStandard(WIFI_STANDARD_80211ax);
    sta2Dev->SetHeConfiguration(CreateObject<HeConfiguration>());
    m_phySta2 = CreateObject<OfdmaSpectrumWifiPhy>(2);
    Ptr<InterferenceHelper> sta2InterferenceHelper = CreateObject<InterferenceHelper>();
    m_phySta2->SetInterferenceHelper(sta2InterferenceHelper);
    Ptr<ErrorRateModel> sta2ErrorModel = CreateObject<NistErrorRateModel>();
    m_phySta2->SetErrorRateModel(sta2ErrorModel);
    m_phySta2->SetDevice(sta2Dev);
    m_phySta2->AddChannel(spectrumChannel);
    m_phySta2->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phySta2->SetPreambleDetectionModel(preambleDetectionModel);
    Ptr<ConstantPositionMobilityModel> sta2Mobility = CreateObject<ConstantPositionMobilityModel>();
    m_phySta2->SetMobility(sta2Mobility);
    sta2Dev->SetPhy(m_phySta2);
    sta2Node->AggregateObject(sta2Mobility);
    sta2Node->AddDevice(sta2Dev);

    Ptr<Node> sta3Node = CreateObject<Node>();
    Ptr<WifiNetDevice> sta3Dev = CreateObject<WifiNetDevice>();
    sta3Dev->SetStandard(WIFI_STANDARD_80211ax);
    sta3Dev->SetHeConfiguration(CreateObject<HeConfiguration>());
    m_phySta3 = CreateObject<OfdmaSpectrumWifiPhy>(3);
    Ptr<InterferenceHelper> sta3InterferenceHelper = CreateObject<InterferenceHelper>();
    m_phySta3->SetInterferenceHelper(sta3InterferenceHelper);
    Ptr<ErrorRateModel> sta3ErrorModel = CreateObject<NistErrorRateModel>();
    m_phySta3->SetErrorRateModel(sta3ErrorModel);
    m_phySta3->SetDevice(sta3Dev);
    m_phySta3->AddChannel(spectrumChannel);
    m_phySta3->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phySta3->SetPreambleDetectionModel(preambleDetectionModel);
    Ptr<ConstantPositionMobilityModel> sta3Mobility = CreateObject<ConstantPositionMobilityModel>();
    m_phySta3->SetMobility(sta3Mobility);
    sta3Dev->SetPhy(m_phySta3);
    sta3Node->AggregateObject(sta3Mobility);
    sta3Node->AddDevice(sta3Dev);

    Ptr<Node> interfererNode = CreateObject<Node>();
    Ptr<NonCommunicatingNetDevice> interfererDev = CreateObject<NonCommunicatingNetDevice>();
    m_phyInterferer = CreateObject<WaveformGenerator>();
    m_phyInterferer->SetDevice(interfererDev);
    m_phyInterferer->SetChannel(spectrumChannel);
    m_phyInterferer->SetDutyCycle(1);
    interfererNode->AddDevice(interfererDev);

    // Configure power attributes of all wifi devices
    std::list<Ptr<WifiPhy>> phys{m_phyAp, m_phySta1, m_phySta2, m_phySta3};
    for (auto& phy : phys)
    {
        phy->SetAttribute("TxGain", DoubleValue(1.0));
        phy->SetAttribute("TxPowerStart", DoubleValue(16.0));
        phy->SetAttribute("TxPowerEnd", DoubleValue(16.0));
        phy->SetAttribute("PowerDensityLimit", DoubleValue(100.0)); // no impact by default
        phy->SetAttribute("RxGain", DoubleValue(2.0));
        // test assumes no rejection power for simplicity
        phy->SetAttribute("TxMaskInnerBandMinimumRejection", DoubleValue(-100.0));
        phy->SetAttribute("TxMaskOuterBandMinimumRejection", DoubleValue(-100.0));
        phy->SetAttribute("TxMaskOuterBandMaximumRejection", DoubleValue(-100.0));
    }
}

void
TestUlOfdmaPhyTransmission::DoTeardown()
{
    m_phyAp->Dispose();
    m_phyAp = nullptr;
    m_phySta1->Dispose();
    m_phySta1 = nullptr;
    m_phySta2->Dispose();
    m_phySta2 = nullptr;
    m_phySta3->Dispose();
    m_phySta3 = nullptr;
    m_phyInterferer->Dispose();
    m_phyInterferer = nullptr;
}

void
TestUlOfdmaPhyTransmission::LogScenario(std::string log) const
{
    NS_LOG_INFO(log);
}

void
TestUlOfdmaPhyTransmission::ScheduleTest(Time delay,
                                         bool solicited,
                                         WifiPhyState expectedStateAtEnd,
                                         uint32_t expectedSuccessFromSta1,
                                         uint32_t expectedFailuresFromSta1,
                                         uint32_t expectedBytesFromSta1,
                                         uint32_t expectedSuccessFromSta2,
                                         uint32_t expectedFailuresFromSta2,
                                         uint32_t expectedBytesFromSta2,
                                         bool scheduleTxSta1,
                                         Time ulTimeDifference,
                                         WifiPhyState expectedStateBeforeEnd,
                                         TrigVectorInfo error)
{
    static uint64_t uid = 0;

    // AP sends an SU packet preceding HE TB PPDUs
    Simulator::Schedule(delay - MilliSeconds(10),
                        &TestUlOfdmaPhyTransmission::SendHeSuPpdu,
                        this,
                        0,
                        50,
                        ++uid,
                        0);
    if (!solicited)
    {
        // UID of TB PPDUs will be different than the one of the preceding frame
        ++uid;
    }
    else
    {
        Simulator::Schedule(delay, &TestUlOfdmaPhyTransmission::SetTrigVector, this, 0, error);
    }
    // STA1 and STA2 send MU UL PPDUs addressed to AP
    Simulator::Schedule(delay - MilliSeconds(1),
                        &OfdmaTestPhyListener::Reset,
                        m_apPhyStateListener.get());
    if (scheduleTxSta1)
    {
        Simulator::Schedule(delay,
                            &TestUlOfdmaPhyTransmission::SendHeTbPpdu,
                            this,
                            1,
                            1,
                            1000,
                            uid,
                            0,
                            false);
    }
    Simulator::Schedule(delay + ulTimeDifference,
                        &TestUlOfdmaPhyTransmission::SendHeTbPpdu,
                        this,
                        2,
                        2,
                        1001,
                        uid,
                        0,
                        false);

    // Verify it takes m_expectedPpduDuration to transmit the PPDUs
    Simulator::Schedule(delay + m_expectedPpduDuration - NanoSeconds(1),
                        &TestUlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phyAp,
                        expectedStateBeforeEnd);
    Simulator::Schedule(delay + m_expectedPpduDuration + ulTimeDifference,
                        &TestUlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phyAp,
                        expectedStateAtEnd);
    // TODO: add checks on TX stop for STAs

    if (expectedSuccessFromSta1 + expectedFailuresFromSta1 + expectedSuccessFromSta2 +
            expectedFailuresFromSta2 >
        0)
    {
        // RxEndOk if at least one HE TB PPDU has been successfully received, RxEndError otherwise
        const bool isSuccess = (expectedSuccessFromSta1 > 0) || (expectedSuccessFromSta2 > 0);
        // The expected time at which the reception is started corresponds to the time at which the
        // test is started, plus the time to transmit the PHY preamble and the PHY headers.
        const Time expectedPayloadStart = delay + MicroSeconds(48);
        // The expected time at which the reception is terminated corresponds to the time at which
        // the test is started, plus the time to transmit the PPDU, plus the delay between the first
        // received HE TB PPDU and the last received HE TB PPDU.
        const Time expectedPayloadEnd = delay + m_expectedPpduDuration + ulTimeDifference;
        // At the end of the transmission, verify that a single RX start notification shall have
        // been notified when the reception of the first HE RB PPDU starts.
        Simulator::Schedule(expectedPayloadEnd,
                            &TestUlOfdmaPhyTransmission::CheckApRxStart,
                            this,
                            1,
                            Simulator::Now() + expectedPayloadStart);
        // After the reception (hence we add 1ns to expectedPayloadEnd), a single RX end
        // notification shall have been notified when the reception of the last HE RB PPDU ends
        Simulator::Schedule(expectedPayloadEnd + NanoSeconds(1),
                            &TestUlOfdmaPhyTransmission::CheckApRxEnd,
                            this,
                            1,
                            Simulator::Now() + expectedPayloadEnd,
                            isSuccess);
    }

    delay += MilliSeconds(100);
    // Check reception state from STA 1
    Simulator::Schedule(delay,
                        &TestUlOfdmaPhyTransmission::CheckRxFromSta1,
                        this,
                        expectedSuccessFromSta1,
                        expectedFailuresFromSta1,
                        expectedBytesFromSta1);
    // Check reception state from STA 2
    Simulator::Schedule(delay,
                        &TestUlOfdmaPhyTransmission::CheckRxFromSta2,
                        this,
                        expectedSuccessFromSta2,
                        expectedFailuresFromSta2,
                        expectedBytesFromSta2);
    // Verify events data have been cleared
    Simulator::Schedule(delay, &TestUlOfdmaPhyTransmission::VerifyEventsCleared, this);

    delay += MilliSeconds(100);
    Simulator::Schedule(delay, &TestUlOfdmaPhyTransmission::Reset, this);
}

void
TestUlOfdmaPhyTransmission::SchedulePowerMeasurementChecks(Time delay,
                                                           Watt_u rxPowerNonOfdmaRu1,
                                                           Watt_u rxPowerNonOfdmaRu2,
                                                           Watt_u rxPowerOfdmaRu1,
                                                           Watt_u rxPowerOfdmaRu2)
{
    const auto detectionDuration = WifiPhy::GetPreambleDetectionDuration();
    const auto txVectorSta1 = GetTxVectorForHeTbPpdu(1, 1, 0);
    const auto txVectorSta2 = GetTxVectorForHeTbPpdu(2, 2, 0);
    const auto hePhy = m_phyAp->GetHePhy();
    const auto nonOfdmaDuration = hePhy->CalculateNonHeDurationForHeTb(txVectorSta2);
    NS_ASSERT(nonOfdmaDuration == hePhy->CalculateNonHeDurationForHeTb(txVectorSta1));

    std::vector<Watt_u> rxPowerNonOfdma{rxPowerNonOfdmaRu1, rxPowerNonOfdmaRu2};
    std::vector<WifiSpectrumBandInfo> nonOfdmaBand{hePhy->GetNonOfdmaBand(txVectorSta1, 1),
                                                   hePhy->GetNonOfdmaBand(txVectorSta2, 2)};
    std::vector<Watt_u> rxPowerOfdma{rxPowerOfdmaRu1, rxPowerOfdmaRu2};
    std::vector<WifiSpectrumBandInfo> ofdmaBand{hePhy->GetRuBandForRx(txVectorSta1, 1),
                                                hePhy->GetRuBandForRx(txVectorSta2, 2)};

    for (uint8_t i = 0; i < 2; ++i)
    {
        /**
         * Perform checks at AP
         */
        // Check received power on non-OFDMA portion
        Simulator::Schedule(
            delay + detectionDuration +
                NanoSeconds(1), // just after beginning of portion (once event is stored)
            &TestUlOfdmaPhyTransmission::CheckNonOfdmaRxPower,
            this,
            m_phyAp,
            nonOfdmaBand[i],
            rxPowerNonOfdma[i]);
        Simulator::Schedule(delay + nonOfdmaDuration - NanoSeconds(1), // just before end of portion
                            &TestUlOfdmaPhyTransmission::CheckNonOfdmaRxPower,
                            this,
                            m_phyAp,
                            nonOfdmaBand[i],
                            rxPowerNonOfdma[i]);
        // Check received power on OFDMA portion
        Simulator::Schedule(delay + nonOfdmaDuration +
                                NanoSeconds(1), // just after beginning of portion
                            &TestUlOfdmaPhyTransmission::CheckOfdmaRxPower,
                            this,
                            m_phyAp,
                            ofdmaBand[i],
                            rxPowerOfdma[i]);
        Simulator::Schedule(delay + m_expectedPpduDuration -
                                NanoSeconds(1), // just before end of portion
                            &TestUlOfdmaPhyTransmission::CheckOfdmaRxPower,
                            this,
                            m_phyAp,
                            ofdmaBand[i],
                            rxPowerOfdma[i]);

        /**
         * Perform checks for non-transmitting STA (STA 3).
         * Cannot use CheckNonOfdmaRxPower method since current event may be reset if
         * preamble not detected (e.g. not on primary).
         */
        // Check received power on non-OFDMA portion
        Simulator::Schedule(
            delay + detectionDuration +
                NanoSeconds(1), // just after beginning of portion (once event is stored)
            &TestUlOfdmaPhyTransmission::CheckOfdmaRxPower,
            this,
            m_phySta3,
            nonOfdmaBand[i],
            rxPowerNonOfdma[i]);
        Simulator::Schedule(delay + nonOfdmaDuration - NanoSeconds(1), // just before end of portion
                            &TestUlOfdmaPhyTransmission::CheckOfdmaRxPower,
                            this,
                            m_phySta3,
                            nonOfdmaBand[i],
                            rxPowerNonOfdma[i]);
        // Check received power on OFDMA portion
        Simulator::Schedule(delay + nonOfdmaDuration +
                                NanoSeconds(1), // just after beginning of portion
                            &TestUlOfdmaPhyTransmission::CheckOfdmaRxPower,
                            this,
                            m_phySta3,
                            ofdmaBand[i],
                            rxPowerOfdma[i]);
        Simulator::Schedule(delay + m_expectedPpduDuration -
                                NanoSeconds(1), // just before end of portion
                            &TestUlOfdmaPhyTransmission::CheckOfdmaRxPower,
                            this,
                            m_phySta3,
                            ofdmaBand[i],
                            rxPowerOfdma[i]);
    }

    if (rxPowerOfdmaRu1 != Watt_u{0.0})
    {
        /**
         * Perform checks for transmitting STA (STA 2) to ensure it has correctly logged
         * power received from other transmitting STA (STA 1).
         * Cannot use CheckNonOfdmaRxPower method since current event not set.
         */
        const auto rxPowerNonOfdmaSta1Only =
            (m_channelWidth >= MHz_u{40})
                ? rxPowerNonOfdma[0]
                : rxPowerNonOfdma[0] / 2; // both STAs transmit over the same 20 MHz channel
        // Check received power on non-OFDMA portion
        Simulator::Schedule(
            delay + detectionDuration +
                NanoSeconds(1), // just after beginning of portion (once event is stored)
            &TestUlOfdmaPhyTransmission::CheckOfdmaRxPower,
            this,
            m_phySta2,
            nonOfdmaBand[0],
            rxPowerNonOfdmaSta1Only);
        Simulator::Schedule(delay + nonOfdmaDuration - NanoSeconds(1), // just before end of portion
                            &TestUlOfdmaPhyTransmission::CheckOfdmaRxPower,
                            this,
                            m_phySta2,
                            nonOfdmaBand[0],
                            rxPowerNonOfdmaSta1Only);
        // Check received power on OFDMA portion
        Simulator::Schedule(delay + nonOfdmaDuration +
                                NanoSeconds(1), // just after beginning of portion
                            &TestUlOfdmaPhyTransmission::CheckOfdmaRxPower,
                            this,
                            m_phySta2,
                            ofdmaBand[0],
                            rxPowerOfdma[0]);
        Simulator::Schedule(delay + m_expectedPpduDuration -
                                NanoSeconds(1), // just before end of portion
                            &TestUlOfdmaPhyTransmission::CheckOfdmaRxPower,
                            this,
                            m_phySta2,
                            ofdmaBand[0],
                            rxPowerOfdma[0]);
    }
}

void
TestUlOfdmaPhyTransmission::RunOne()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 0;
    m_phyAp->AssignStreams(streamNumber);
    m_phySta1->AssignStreams(streamNumber);
    m_phySta2->AssignStreams(streamNumber);
    m_phySta3->AssignStreams(streamNumber);

    auto channelNum = WifiPhyOperatingChannel::FindFirst(0,
                                                         m_frequency,
                                                         m_channelWidth,
                                                         WIFI_STANDARD_80211ax,
                                                         WIFI_PHY_BAND_5GHZ)
                          ->number;

    m_phyAp->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, m_channelWidth, WIFI_PHY_BAND_5GHZ, 0});
    m_phySta1->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, m_channelWidth, WIFI_PHY_BAND_5GHZ, 0});
    m_phySta2->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, m_channelWidth, WIFI_PHY_BAND_5GHZ, 0});
    m_phySta3->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, m_channelWidth, WIFI_PHY_BAND_5GHZ, 0});

    Time delay;
    Simulator::Schedule(delay, &TestUlOfdmaPhyTransmission::Reset, this);
    delay += Seconds(1);

    /**
     * In all the following tests, 2 HE TB PPDUs of the same UL MU transmission
     * are sent on RU 1 for STA 1 and RU 2 for STA 2.
     * The difference between solicited and unsolicited lies in that their PPDU
     * ID correspond to the one of the immediately preceding HE SU PPDU (thus
     * mimicking trigger frame reception).
     */

    //---------------------------------------------------------------------------
    // Verify that both solicited HE TB PPDUs have been corrected received
    Simulator::Schedule(delay,
                        &TestUlOfdmaPhyTransmission::LogScenario,
                        this,
                        "Reception of solicited HE TB PPDUs");
    ScheduleTest(delay,
                 true,
                 WifiPhyState::IDLE,
                 1,
                 0,
                 1000, // One PSDU of 1000 bytes should have been successfully received from STA 1
                 1,
                 0,
                 1001); // One PSDU of 1001 bytes should have been successfully received from STA 2
    delay += Seconds(1);

    //---------------------------------------------------------------------------
    // Verify that two solicited HE TB PPDUs with delay (< 400ns) between the two signals have been
    // corrected received
    Simulator::Schedule(
        delay,
        &TestUlOfdmaPhyTransmission::LogScenario,
        this,
        "Reception of solicited HE TB PPDUs with delay (< 400ns) between the two signals");
    ScheduleTest(delay,
                 true,
                 WifiPhyState::IDLE,
                 1,
                 0,
                 1000, // One PSDU of 1000 bytes should have been successfully received from STA 1
                 1,
                 0,
                 1001, // One PSDU of 1001 bytes should have been successfully received from STA 2
                 true,
                 NanoSeconds(100));
    delay += Seconds(1);

    //---------------------------------------------------------------------------
    // Verify that no unsolicited HE TB PPDU is received
    Simulator::Schedule(delay,
                        &TestUlOfdmaPhyTransmission::LogScenario,
                        this,
                        "Dropping of unsolicited HE TB PPDUs");
    ScheduleTest(delay,
                 false,
                 WifiPhyState::IDLE,
                 0,
                 0,
                 0, // PSDU from STA 1 is not received (no TRIGVECTOR)
                 0,
                 0,
                 0, // PSDU from STA 2 is not received (no TRIGVECTOR)
                 true,
                 Seconds(0),
                 WifiPhyState::CCA_BUSY);
    delay += Seconds(1);

    //---------------------------------------------------------------------------
    // Verify that HE TB PPDUs with channel width differing from TRIGVECTOR are discarded
    Simulator::Schedule(delay,
                        &TestUlOfdmaPhyTransmission::LogScenario,
                        this,
                        "Dropping of HE TB PPDUs with channel width differing from TRIGVECTOR");
    ScheduleTest(delay,
                 true,
                 WifiPhyState::IDLE,
                 0,
                 0,
                 0, // PSDU from STA 1 is not received (no TRIGVECTOR)
                 0,
                 0,
                 0, // PSDU from STA 2 is not received (no TRIGVECTOR)
                 true,
                 Seconds(0),
                 WifiPhyState::CCA_BUSY,
                 CHANNEL_WIDTH);
    delay += Seconds(1);

    //---------------------------------------------------------------------------
    // Verify that HE TB PPDUs with UL Length differing from TRIGVECTOR are discarded
    Simulator::Schedule(delay,
                        &TestUlOfdmaPhyTransmission::LogScenario,
                        this,
                        "Dropping of HE TB PPDUs with UL Length differing from TRIGVECTOR");
    ScheduleTest(delay,
                 true,
                 WifiPhyState::IDLE,
                 0,
                 0,
                 0, // PSDU from STA 1 is not received (no TRIGVECTOR)
                 0,
                 0,
                 0, // PSDU from STA 2 is not received (no TRIGVECTOR)
                 true,
                 Seconds(0),
                 WifiPhyState::CCA_BUSY,
                 UL_LENGTH);
    delay += Seconds(1);

    //---------------------------------------------------------------------------
    // Verify that HE TB PPDUs with AIDs differing from TRIGVECTOR are discarded
    Simulator::Schedule(delay,
                        &TestUlOfdmaPhyTransmission::LogScenario,
                        this,
                        "Dropping of HE TB PPDUs with AIDs differing from TRIGVECTOR");
    ScheduleTest(delay,
                 true,
                 WifiPhyState::IDLE,
                 0,
                 0,
                 0, // PSDU from STA 1 is not received (no TRIGVECTOR)
                 0,
                 0,
                 0, // PSDU from STA 2 is not received (no TRIGVECTOR)
                 true,
                 Seconds(0),
                 WifiPhyState::CCA_BUSY,
                 AID);
    delay += Seconds(1);

    //---------------------------------------------------------------------------
    // Generate an interference on RU 1 and verify that only STA 1's solicited HE TB PPDU has been
    // impacted
    Simulator::Schedule(
        delay,
        &TestUlOfdmaPhyTransmission::LogScenario,
        this,
        "Reception of solicited HE TB PPDUs with interference on RU 1 during PSDU reception");
    // A strong non-wifi interference is generated on RU 1 during PSDU reception
    BandInfo bandInfo;
    bandInfo.fc = MHzToHz(m_frequency - (m_channelWidth / 4));
    bandInfo.fl = bandInfo.fc - MHzToHz(m_channelWidth / 4);
    bandInfo.fh = bandInfo.fc + MHzToHz(m_channelWidth / 4);
    Bands bands;
    bands.push_back(bandInfo);

    Ptr<SpectrumModel> SpectrumInterferenceRu1 = Create<SpectrumModel>(bands);
    Ptr<SpectrumValue> interferencePsdRu1 = Create<SpectrumValue>(SpectrumInterferenceRu1);
    Watt_u interferencePower{0.1};
    *interferencePsdRu1 = interferencePower / (MHzToHz(m_channelWidth / 2) * 20);

    Simulator::Schedule(delay + MicroSeconds(50),
                        &TestUlOfdmaPhyTransmission::GenerateInterference,
                        this,
                        interferencePsdRu1,
                        MilliSeconds(100));
    ScheduleTest(
        delay,
        true,
        WifiPhyState::CCA_BUSY, // PHY should move to CCA_BUSY instead of IDLE due to the
                                // interference
        0,
        1,
        0, // Reception of the PSDU from STA 1 should have failed (since interference occupies RU 1)
        1,
        0,
        1001); // One PSDU of 1001 bytes should have been successfully received from STA 2
    delay += Seconds(1);

    //---------------------------------------------------------------------------
    // Generate an interference on RU 2 and verify that only STA 2's solicited HE TB PPDU has been
    // impacted
    Simulator::Schedule(
        delay,
        &TestUlOfdmaPhyTransmission::LogScenario,
        this,
        "Reception of solicited HE TB PPDUs with interference on RU 2 during PSDU reception");
    // A strong non-wifi interference is generated on RU 2 during PSDU reception
    bandInfo.fc = MHzToHz(m_frequency + (m_channelWidth / 4));
    bandInfo.fl = bandInfo.fc - MHzToHz(m_channelWidth / 4);
    bandInfo.fh = bandInfo.fc + MHzToHz(m_channelWidth / 4);
    bands.clear();
    bands.push_back(bandInfo);

    Ptr<SpectrumModel> SpectrumInterferenceRu2 = Create<SpectrumModel>(bands);
    Ptr<SpectrumValue> interferencePsdRu2 = Create<SpectrumValue>(SpectrumInterferenceRu2);
    *interferencePsdRu2 = interferencePower / (MHzToHz(m_channelWidth / 2) * 20);

    Simulator::Schedule(delay + MicroSeconds(50),
                        &TestUlOfdmaPhyTransmission::GenerateInterference,
                        this,
                        interferencePsdRu2,
                        MilliSeconds(100));
    ScheduleTest(delay,
                 true,
                 (m_channelWidth >= MHz_u{40})
                     ? WifiPhyState::IDLE
                     : WifiPhyState::CCA_BUSY, // PHY should move to CCA_BUSY if interference is
                                               // generated in its primary channel
                 1,
                 0,
                 1000, // One PSDU of 1000 bytes should have been successfully received from STA 1
                 0,
                 1,
                 0); // Reception of the PSDU from STA 2 should have failed (since interference
                     // occupies RU 2)
    delay += Seconds(1);

    //---------------------------------------------------------------------------
    // Generate an interference on the full band and verify that both solicited HE TB PPDUs have
    // been impacted
    Simulator::Schedule(delay,
                        &TestUlOfdmaPhyTransmission::LogScenario,
                        this,
                        "Reception of solicited HE TB PPDUs with interference on the full band "
                        "during PSDU reception");
    // A strong non-wifi interference is generated on the full band during PSDU reception
    bandInfo.fc = MHzToHz(m_frequency);
    bandInfo.fl = bandInfo.fc - MHzToHz(m_channelWidth / 2);
    bandInfo.fh = bandInfo.fc + MHzToHz(m_channelWidth / 2);
    bands.clear();
    bands.push_back(bandInfo);

    Ptr<SpectrumModel> SpectrumInterferenceAll = Create<SpectrumModel>(bands);
    Ptr<SpectrumValue> interferencePsdAll = Create<SpectrumValue>(SpectrumInterferenceAll);
    *interferencePsdAll = interferencePower / (MHzToHz(m_channelWidth) * 20);

    Simulator::Schedule(delay + MicroSeconds(50),
                        &TestUlOfdmaPhyTransmission::GenerateInterference,
                        this,
                        interferencePsdAll,
                        MilliSeconds(100));
    ScheduleTest(
        delay,
        true,
        WifiPhyState::CCA_BUSY, // PHY should move to CCA_BUSY instead of IDLE due to the
                                // interference
        0,
        1,
        0, // Reception of the PSDU from STA 1 should have failed (since interference occupies RU 1)
        0,
        1,
        0); // Reception of the PSDU from STA 2 should have failed (since interference occupies RU
            // 2)
    delay += Seconds(1);

    //---------------------------------------------------------------------------
    // Send another HE TB PPDU (of another UL MU transmission) on RU 1 and verify that both
    // solicited HE TB PPDUs have been impacted if they are on the same
    // 20 MHz channel. Only STA 1's solicited HE TB PPDU is impacted otherwise.
    Simulator::Schedule(delay,
                        &TestUlOfdmaPhyTransmission::LogScenario,
                        this,
                        "Reception of solicited HE TB PPDUs with another HE TB PPDU arriving on RU "
                        "1 during PSDU reception");
    // Another HE TB PPDU arrives at AP on the same RU as STA 1 during PSDU reception
    Simulator::Schedule(delay + MicroSeconds(50),
                        &TestUlOfdmaPhyTransmission::SendHeTbPpdu,
                        this,
                        3,
                        1,
                        1002,
                        1,
                        0,
                        false);
    // Expected figures from STA 2
    uint32_t succ;
    uint32_t fail;
    uint32_t bytes;
    if (m_channelWidth > MHz_u{20})
    {
        // One PSDU of 1001 bytes should have been successfully received from STA 2 (since
        // interference from STA 3 on distinct 20 MHz channel)
        succ = 1;
        fail = 0;
        bytes = 1001;
    }
    else
    {
        // Reception of the PSDU from STA 2 should have failed (since interference from STA 3 on
        // same 20 MHz channel)
        succ = 0;
        fail = 1;
        bytes = 0;
    }
    ScheduleTest(delay,
                 true,
                 WifiPhyState::CCA_BUSY, // PHY should move to CCA_BUSY instead of IDLE due to the
                                         // interference on measurement channel width
                 0,
                 1,
                 0, // Reception of the PSDU from STA 1 should have failed (since interference from
                    // STA 3 on same 20 MHz channel)
                 succ,
                 fail,
                 bytes);
    delay += Seconds(1);

    //---------------------------------------------------------------------------
    // Send another HE TB PPDU (of another UL MU transmission) on RU 2 and verify that both
    // solicited HE TB PPDUs have been impacted if they are on the same
    // 20 MHz channel. Only STA 2's solicited HE TB PPDU is impacted otherwise.
    Simulator::Schedule(delay,
                        &TestUlOfdmaPhyTransmission::LogScenario,
                        this,
                        "Reception of solicited HE TB PPDUs with another HE TB PPDU arriving on RU "
                        "2 during PSDU reception");
    // Another HE TB PPDU arrives at AP on the same RU as STA 2 during PSDU reception
    Simulator::Schedule(delay + MicroSeconds(50),
                        &TestUlOfdmaPhyTransmission::SendHeTbPpdu,
                        this,
                        3,
                        2,
                        1002,
                        1,
                        0,
                        false);
    // Expected figures from STA 1
    if (m_channelWidth > MHz_u{20})
    {
        // One PSDU of 1000 bytes should have been successfully received from STA 1 (since
        // interference from STA 3 on distinct 20 MHz channel)
        succ = 1;
        fail = 0;
        bytes = 1000;
    }
    else
    {
        // Reception of the PSDU from STA 1 should have failed (since interference from STA 3 on
        // same 20 MHz channel)
        succ = 0;
        fail = 1;
        bytes = 0;
    }
    ScheduleTest(delay,
                 true,
                 (m_channelWidth >= MHz_u{40})
                     ? WifiPhyState::IDLE
                     : WifiPhyState::CCA_BUSY, // PHY should move to CCA_BUSY instead of IDLE if HE
                                               // TB PPDU on primary channel
                 succ,
                 fail,
                 bytes,
                 0,
                 1,
                 0); // Reception of the PSDU from STA 2 should have failed (since interference from
                     // STA 3 on same 20 MHz channel)
    delay += Seconds(1);

    //---------------------------------------------------------------------------
    // Send an HE SU PPDU during 400 ns window and verify that both solicited HE TB PPDUs have been
    // impacted
    Simulator::Schedule(
        delay,
        &TestUlOfdmaPhyTransmission::LogScenario,
        this,
        "Reception of solicited HE TB PPDUs with an HE SU PPDU arriving during the 400 ns window");
    // One HE SU arrives at AP during the 400ns window
    Simulator::Schedule(delay + NanoSeconds(300),
                        &TestUlOfdmaPhyTransmission::SendHeSuPpdu,
                        this,
                        3,
                        1002,
                        1,
                        0);
    ScheduleTest(
        delay,
        true,
        WifiPhyState::IDLE,
        0,
        1,
        0, // Reception of the PSDU from STA 1 should have failed (since interference from STA 3)
        0,
        1,
        0); // Reception of the PSDU from STA 2 should have failed (since interference from STA 3)
    delay += Seconds(1);

    //---------------------------------------------------------------------------
    // Only send a solicited HE TB PPDU from STA 2 on RU 2 and verify that it has been correctly
    // received
    Simulator::Schedule(delay,
                        &TestUlOfdmaPhyTransmission::LogScenario,
                        this,
                        "Reception of solicited HE TB PPDU only on RU 2");
    // Check that STA3 will correctly set its state to CCA_BUSY if in measurement channel or IDLE
    // otherwise
    Simulator::Schedule(delay + m_expectedPpduDuration - NanoSeconds(1),
                        &TestUlOfdmaPhyTransmission::CheckPhyState,
                        this,
                        m_phySta3,
                        (m_channelWidth >= MHz_u{40})
                            ? WifiPhyState::IDLE
                            : WifiPhyState::CCA_BUSY); // PHY should move to CCA_BUSY instead of
                                                       // IDLE if HE TB PPDU on primary channel
    ScheduleTest(delay,
                 true,
                 WifiPhyState::IDLE,
                 0,
                 0,
                 0, // No transmission scheduled for STA 1
                 1,
                 0,
                 1001, // One PSDU of 1001 bytes should have been successfully received from STA 2
                 false,
                 Seconds(0),
                 WifiPhyState::RX); // Measurement channel is total channel width
    delay += Seconds(1);

    //---------------------------------------------------------------------------
    // Measure the power of a solicited HE TB PPDU from STA 2 on RU 2
    Simulator::Schedule(delay,
                        &TestUlOfdmaPhyTransmission::LogScenario,
                        this,
                        "Measure power for reception of HE TB PPDU only on RU 2");
    auto rxPower = DbmToW(
        dBm_u{19}); // 16+1 dBm at STAs and +2 at AP (no loss since all devices are colocated)
    SchedulePowerMeasurementChecks(delay,
                                   (m_channelWidth >= MHz_u{40}) ? Watt_u{0.0} : rxPower,
                                   rxPower, // power detected on RU1 only if same 20 MHz as RU 2
                                   Watt_u{0.0},
                                   rxPower);
    ScheduleTest(delay,
                 true,
                 WifiPhyState::IDLE,
                 0,
                 0,
                 0, // No transmission scheduled for STA 1
                 1,
                 0,
                 1001, // One PSDU of 1001 bytes should have been successfully received from STA 2
                 false,
                 Seconds(0),
                 WifiPhyState::RX); // Measurement channel is total channel width
    delay += Seconds(1);

    //---------------------------------------------------------------------------
    // Measure the power of a solicited HE TB PPDU from STA 2 on RU 2 with power spectrum density
    // limitation enforced
    Simulator::Schedule(
        delay,
        &TestUlOfdmaPhyTransmission::LogScenario,
        this,
        "Measure power for reception of HE TB PPDU only on RU 2 with PSD limitation");
    // Configure PSD limitation at 3 dBm/MHz -> 3+13.0103=16.0103 dBm max for 20 MHz,
    // 3+9.0309=12.0309 dBm max for 106-tone RU, no impact for 40 MHz and above
    Simulator::Schedule(delay - NanoSeconds(1), // just before sending HE TB
                        &TestUlOfdmaPhyTransmission::SetPsdLimit,
                        this,
                        m_phySta2,
                        dBm_per_MHz_u{3});

    rxPower = (m_channelWidth > MHz_u{40})
                  ? DbmToW(dBm_u{19})
                  : DbmToW(dBm_u{18.0103}); // 15.0103+1 dBm at STA 2 and +2 at AP for non-OFDMA
                                            // transmitted only on one 20 MHz channel
    auto rxPowerOfdma = rxPower;
    if (m_channelWidth <= MHz_u{40})
    {
        rxPowerOfdma =
            (m_channelWidth == MHz_u{20})
                ? DbmToW(dBm_u{14.0309})  // 11.0309+1 dBm at STA and +2 at AP if 106-tone RU
                : DbmToW(dBm_u{18.0103}); // 15.0103+1 dBm at STA 2 and +2 at AP if 242-tone RU
    }
    SchedulePowerMeasurementChecks(delay,
                                   (m_channelWidth >= MHz_u{40}) ? Watt_u{0.0} : rxPower,
                                   rxPower, // power detected on RU1 only if same 20 MHz as RU 2
                                   Watt_u{0.0},
                                   rxPowerOfdma);

    // Reset PSD limitation once HE TB has been sent
    Simulator::Schedule(delay + m_expectedPpduDuration,
                        &TestUlOfdmaPhyTransmission::SetPsdLimit,
                        this,
                        m_phySta2,
                        dBm_per_MHz_u{100});
    ScheduleTest(delay,
                 true,
                 WifiPhyState::IDLE,
                 0,
                 0,
                 0, // No transmission scheduled for STA 1
                 1,
                 0,
                 1001, // One PSDU of 1001 bytes should have been successfully received from STA 2
                 false,
                 Seconds(0),
                 WifiPhyState::RX); // Measurement channel is total channel width
    delay += Seconds(1);

    //---------------------------------------------------------------------------
    // Measure the power of 2 solicited HE TB PPDU from both STAs
    Simulator::Schedule(delay,
                        &TestUlOfdmaPhyTransmission::LogScenario,
                        this,
                        "Measure power for reception of HE TB PPDU on both RUs");
    rxPower = DbmToW(
        dBm_u{19}); // 16+1 dBm at STAs and +2 at AP (no loss since all devices are colocated)
    const auto rxPowerNonOfdma =
        (m_channelWidth >= MHz_u{40})
            ? rxPower
            : rxPower * 2; // both STAs transmit over the same 20 MHz channel
    SchedulePowerMeasurementChecks(delay, rxPowerNonOfdma, rxPowerNonOfdma, rxPower, rxPower);
    ScheduleTest(delay,
                 true,
                 WifiPhyState::IDLE,
                 1,
                 0,
                 1000, // One PSDU of 1000 bytes should have been successfully received from STA 1
                 1,
                 0,
                 1001); // One PSDU of 1001 bytes should have been successfully received from STA 2
    delay += Seconds(1);

    //---------------------------------------------------------------------------
    // Verify that an HE TB PPDU from another BSS has been correctly received (no UL MU transmission
    // ongoing)
    Simulator::Schedule(delay,
                        &TestUlOfdmaPhyTransmission::LogScenario,
                        this,
                        "Reception of an HE TB PPDU from another BSS");
    // One HE TB from another BSS (BSS color 2) arrives at AP (BSS color 1)
    Simulator::Schedule(delay, &TestUlOfdmaPhyTransmission::SetBssColor, this, m_phyAp, 1);
    Simulator::Schedule(delay + MilliSeconds(100),
                        &TestUlOfdmaPhyTransmission::SendHeTbPpdu,
                        this,
                        3,
                        1,
                        1002,
                        1,
                        2,
                        false);

    // Verify events data have been cleared
    Simulator::Schedule(delay + MilliSeconds(200),
                        &TestUlOfdmaPhyTransmission::VerifyEventsCleared,
                        this);

    Simulator::Schedule(delay + MilliSeconds(500), &TestUlOfdmaPhyTransmission::Reset, this);
    delay += Seconds(1);

    //---------------------------------------------------------------------------
    // Verify that two solicited HE TB PPDUs with delay (< 400ns) between the two signals have been
    // corrected received
    Simulator::Schedule(
        delay,
        &TestUlOfdmaPhyTransmission::LogScenario,
        this,
        "Reception of solicited HE TB PPDUs with delay (< 400ns) between the two signals and "
        "reception of an HE TB PPDU from another BSS between the ends of the two HE TB PPDUs");
    Simulator::Schedule(delay, &TestUlOfdmaPhyTransmission::SetBssColor, this, m_phyAp, 1);
    Simulator::Schedule(delay + m_expectedPpduDuration + NanoSeconds(100),
                        &TestUlOfdmaPhyTransmission::SendHeTbPpdu,
                        this,
                        3,
                        1,
                        1002,
                        1,
                        2,
                        true);
    ScheduleTest(delay,
                 true,
                 WifiPhyState::CCA_BUSY,
                 1,
                 0,
                 1000, // One PSDU of 1000 bytes should have been successfully received from STA 1
                 1,
                 0,
                 1001, // One PSDU of 1001 bytes should have been successfully received from STA 2
                 true,
                 NanoSeconds(200));
    delay += Seconds(1);

    Simulator::Run();
}

void
TestUlOfdmaPhyTransmission::DoRun()
{
    m_frequency = MHz_u{5180};
    m_channelWidth = MHz_u{20};
    m_expectedPpduDuration = NanoSeconds(292800);
    NS_LOG_DEBUG("Run UL OFDMA PHY transmission test for " << m_channelWidth << " MHz");
    RunOne();

    m_frequency = MHz_u{5190};
    m_channelWidth = MHz_u{40};
    m_expectedPpduDuration = NanoSeconds(163200);
    NS_LOG_DEBUG("Run UL OFDMA PHY transmission test for " << m_channelWidth << " MHz");
    RunOne();

    m_frequency = MHz_u{5210};
    m_channelWidth = MHz_u{80};
    m_expectedPpduDuration = NanoSeconds(105600);
    NS_LOG_DEBUG("Run UL OFDMA PHY transmission test for " << m_channelWidth << " MHz");
    RunOne();

    m_frequency = MHz_u{5250};
    m_channelWidth = MHz_u{160};
    m_expectedPpduDuration = NanoSeconds(76800);
    NS_LOG_DEBUG("Run UL OFDMA PHY transmission test for " << m_channelWidth << " MHz");
    RunOne();

    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief PHY padding exclusion test
 */
class TestPhyPaddingExclusion : public TestCase
{
  public:
    TestPhyPaddingExclusion();
    ~TestPhyPaddingExclusion() override;

  private:
    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     * Send HE TB PPDU function
     * @param txStaId the ID of the TX STA
     * @param index the RU index used for the transmission
     * @param payloadSize the size of the payload in bytes
     * @param txDuration the duration of the PPDU
     */
    void SendHeTbPpdu(uint16_t txStaId,
                      std::size_t index,
                      std::size_t payloadSize,
                      Time txDuration);
    /**
     * Set TRIGVECTOR for HE TB PPDU
     *
     * @param ppduDuration the duration of the HE TB PPDU
     */
    void SetTrigVector(Time ppduDuration);

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
     * Run one function
     */
    void RunOne();

    /**
     * Check the received PSDUs from STA1
     * @param expectedSuccess the expected number of success
     * @param expectedFailures the expected number of failures
     * @param expectedBytes the expected number of bytes
     */
    void CheckRxFromSta1(uint32_t expectedSuccess,
                         uint32_t expectedFailures,
                         uint32_t expectedBytes);

    /**
     * Check the received PSDUs from STA2
     * @param expectedSuccess the expected number of success
     * @param expectedFailures the expected number of failures
     * @param expectedBytes the expected number of bytes
     */
    void CheckRxFromSta2(uint32_t expectedSuccess,
                         uint32_t expectedFailures,
                         uint32_t expectedBytes);

    /**
     * Verify all events are cleared at end of TX or RX
     */
    void VerifyEventsCleared();

    /**
     * Check the PHY state
     * @param phy the PHY
     * @param expectedState the expected state of the PHY
     */
    void CheckPhyState(Ptr<OfdmaSpectrumWifiPhy> phy, WifiPhyState expectedState);
    /// @copydoc CheckPhyState
    void DoCheckPhyState(Ptr<OfdmaSpectrumWifiPhy> phy, WifiPhyState expectedState);

    /**
     * Reset function
     */
    void Reset();

    /**
     * Receive success function
     * @param psdu the PSDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector the transmit vector
     * @param statusPerMpdu reception status per MPDU
     */
    void RxSuccess(Ptr<const WifiPsdu> psdu,
                   RxSignalInfo rxSignalInfo,
                   const WifiTxVector& txVector,
                   const std::vector<bool>& statusPerMpdu);

    /**
     * Receive failure function
     * @param psdu the PSDU
     */
    void RxFailure(Ptr<const WifiPsdu> psdu);

    Ptr<OfdmaSpectrumWifiPhy> m_phyAp;   ///< PHY of AP
    Ptr<OfdmaSpectrumWifiPhy> m_phySta1; ///< PHY of STA 1
    Ptr<OfdmaSpectrumWifiPhy> m_phySta2; ///< PHY of STA 2

    Ptr<WaveformGenerator> m_phyInterferer; ///< PHY of interferer

    uint32_t m_countRxSuccessFromSta1; ///< count RX success from STA 1
    uint32_t m_countRxSuccessFromSta2; ///< count RX success from STA 2
    uint32_t m_countRxFailureFromSta1; ///< count RX failure from STA 1
    uint32_t m_countRxFailureFromSta2; ///< count RX failure from STA 2
    uint32_t m_countRxBytesFromSta1;   ///< count RX bytes from STA 1
    uint32_t m_countRxBytesFromSta2;   ///< count RX bytes from STA 2
};

TestPhyPaddingExclusion::TestPhyPaddingExclusion()
    : TestCase("PHY padding exclusion test"),
      m_countRxSuccessFromSta1(0),
      m_countRxSuccessFromSta2(0),
      m_countRxFailureFromSta1(0),
      m_countRxFailureFromSta2(0),
      m_countRxBytesFromSta1(0),
      m_countRxBytesFromSta2(0)
{
}

void
TestPhyPaddingExclusion::SendHeTbPpdu(uint16_t txStaId,
                                      std::size_t index,
                                      std::size_t payloadSize,
                                      Time txDuration)
{
    WifiConstPsduMap psdus;

    WifiTxVector txVector{HePhy::GetHeMcs7(),
                          0,
                          WIFI_PREAMBLE_HE_TB,
                          NanoSeconds(1600),
                          1,
                          1,
                          0,
                          DEFAULT_CHANNEL_WIDTH,
                          false,
                          false,
                          true};

    HeRu::RuSpec ru(HeRu::RU_106_TONE, index, false);
    txVector.SetRu(ru, txStaId);
    txVector.SetMode(HePhy::GetHeMcs7(), txStaId);
    txVector.SetNss(1, txStaId);

    auto pkt = Create<Packet>(payloadSize);
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_QOSDATA);
    hdr.SetQosTid(0);
    hdr.SetAddr1(Mac48Address("00:00:00:00:00:00"));
    std::ostringstream addr;
    addr << "00:00:00:00:00:0" << txStaId;
    hdr.SetAddr2(Mac48Address(addr.str().c_str()));
    hdr.SetSequenceNumber(1);
    auto psdu = Create<WifiPsdu>(pkt, hdr);
    psdus.insert(std::make_pair(txStaId, psdu));

    Ptr<OfdmaSpectrumWifiPhy> phy;
    if (txStaId == 1)
    {
        phy = m_phySta1;
    }
    else if (txStaId == 2)
    {
        phy = m_phySta2;
    }

    txVector.SetLength(
        HePhy::ConvertHeTbPpduDurationToLSigLength(txDuration, txVector, phy->GetPhyBand()).first);

    phy->SetPpduUid(0);
    phy->Send(psdus, txVector);
}

void
TestPhyPaddingExclusion::GenerateInterference(Ptr<SpectrumValue> interferencePsd, Time duration)
{
    m_phyInterferer->SetTxPowerSpectralDensity(interferencePsd);
    m_phyInterferer->SetPeriod(duration);
    m_phyInterferer->Start();
    Simulator::Schedule(duration, &TestPhyPaddingExclusion::StopInterference, this);
}

void
TestPhyPaddingExclusion::StopInterference()
{
    m_phyInterferer->Stop();
}

TestPhyPaddingExclusion::~TestPhyPaddingExclusion()
{
}

void
TestPhyPaddingExclusion::RxSuccess(Ptr<const WifiPsdu> psdu,
                                   RxSignalInfo rxSignalInfo,
                                   const WifiTxVector& txVector,
                                   const std::vector<bool>& /*statusPerMpdu*/)
{
    NS_LOG_FUNCTION(this << *psdu << psdu->GetAddr2() << rxSignalInfo << txVector);
    if (psdu->GetAddr2() == Mac48Address("00:00:00:00:00:01"))
    {
        m_countRxSuccessFromSta1++;
        m_countRxBytesFromSta1 += (psdu->GetSize() - 30);
    }
    else if (psdu->GetAddr2() == Mac48Address("00:00:00:00:00:02"))
    {
        m_countRxSuccessFromSta2++;
        m_countRxBytesFromSta2 += (psdu->GetSize() - 30);
    }
}

void
TestPhyPaddingExclusion::RxFailure(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << *psdu << psdu->GetAddr2());
    if (psdu->GetAddr2() == Mac48Address("00:00:00:00:00:01"))
    {
        m_countRxFailureFromSta1++;
    }
    else if (psdu->GetAddr2() == Mac48Address("00:00:00:00:00:02"))
    {
        m_countRxFailureFromSta2++;
    }
}

void
TestPhyPaddingExclusion::CheckRxFromSta1(uint32_t expectedSuccess,
                                         uint32_t expectedFailures,
                                         uint32_t expectedBytes)
{
    NS_TEST_ASSERT_MSG_EQ(m_countRxSuccessFromSta1,
                          expectedSuccess,
                          "The number of successfully received packets from STA 1 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(
        m_countRxFailureFromSta1,
        expectedFailures,
        "The number of unsuccessfuly received packets from STA 1 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxBytesFromSta1,
                          expectedBytes,
                          "The number of bytes received from STA 1 is not correct!");
}

void
TestPhyPaddingExclusion::CheckRxFromSta2(uint32_t expectedSuccess,
                                         uint32_t expectedFailures,
                                         uint32_t expectedBytes)
{
    NS_TEST_ASSERT_MSG_EQ(m_countRxSuccessFromSta2,
                          expectedSuccess,
                          "The number of successfully received packets from STA 2 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(
        m_countRxFailureFromSta2,
        expectedFailures,
        "The number of unsuccessfuly received packets from STA 2 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxBytesFromSta2,
                          expectedBytes,
                          "The number of bytes received from STA 2 is not correct!");
}

void
TestPhyPaddingExclusion::VerifyEventsCleared()
{
    NS_TEST_ASSERT_MSG_EQ(m_phyAp->GetCurrentEvent(),
                          nullptr,
                          "m_currentEvent for AP was not cleared");
    NS_TEST_ASSERT_MSG_EQ(m_phySta1->GetCurrentEvent(),
                          nullptr,
                          "m_currentEvent for STA 1 was not cleared");
    NS_TEST_ASSERT_MSG_EQ(m_phySta2->GetCurrentEvent(),
                          nullptr,
                          "m_currentEvent for STA 2 was not cleared");
}

void
TestPhyPaddingExclusion::CheckPhyState(Ptr<OfdmaSpectrumWifiPhy> phy, WifiPhyState expectedState)
{
    // This is needed to make sure PHY state will be checked as the last event if a state change
    // occurred at the exact same time as the check
    Simulator::ScheduleNow(&TestPhyPaddingExclusion::DoCheckPhyState, this, phy, expectedState);
}

void
TestPhyPaddingExclusion::DoCheckPhyState(Ptr<OfdmaSpectrumWifiPhy> phy, WifiPhyState expectedState)
{
    WifiPhyState currentState = phy->GetState()->GetState();
    NS_LOG_FUNCTION(this << currentState);
    NS_TEST_ASSERT_MSG_EQ(currentState,
                          expectedState,
                          "PHY State " << currentState << " does not match expected state "
                                       << expectedState << " at " << Simulator::Now());
}

void
TestPhyPaddingExclusion::Reset()
{
    m_countRxSuccessFromSta1 = 0;
    m_countRxSuccessFromSta2 = 0;
    m_countRxFailureFromSta1 = 0;
    m_countRxFailureFromSta2 = 0;
    m_countRxBytesFromSta1 = 0;
    m_countRxBytesFromSta2 = 0;
    m_phySta1->SetPpduUid(0);
    m_phySta1->SetTriggerFrameUid(0);
    m_phySta2->SetTriggerFrameUid(0);
}

void
TestPhyPaddingExclusion::DoSetup()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 0;

    Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel>();
    lossModel->SetFrequency(MHzToHz(DEFAULT_FREQUENCY));
    spectrumChannel->AddPropagationLossModel(lossModel);
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);

    Ptr<Node> apNode = CreateObject<Node>();
    Ptr<WifiNetDevice> apDev = CreateObject<WifiNetDevice>();
    auto apMac = CreateObjectWithAttributes<ApWifiMac>(
        "Txop",
        PointerValue(CreateObjectWithAttributes<Txop>("AcIndex", StringValue("AC_BE_NQOS"))));
    apMac->SetAttribute("BeaconGeneration", BooleanValue(false));
    apDev->SetMac(apMac);
    m_phyAp = CreateObject<OfdmaSpectrumWifiPhy>(0);
    Ptr<HeConfiguration> heConfiguration = CreateObject<HeConfiguration>();
    apDev->SetHeConfiguration(heConfiguration);
    Ptr<InterferenceHelper> apInterferenceHelper = CreateObject<InterferenceHelper>();
    m_phyAp->SetInterferenceHelper(apInterferenceHelper);
    Ptr<ErrorRateModel> apErrorModel = CreateObject<NistErrorRateModel>();
    m_phyAp->SetErrorRateModel(apErrorModel);
    m_phyAp->SetDevice(apDev);
    m_phyAp->AddChannel(spectrumChannel);
    m_phyAp->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phyAp->AssignStreams(streamNumber);
    auto channelNum = WifiPhyOperatingChannel::FindFirst(0,
                                                         DEFAULT_FREQUENCY,
                                                         DEFAULT_CHANNEL_WIDTH,
                                                         WIFI_STANDARD_80211ax,
                                                         WIFI_PHY_BAND_5GHZ)
                          ->number;

    m_phyAp->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, DEFAULT_CHANNEL_WIDTH, WIFI_PHY_BAND_5GHZ, 0});
    m_phyAp->SetReceiveOkCallback(MakeCallback(&TestPhyPaddingExclusion::RxSuccess, this));
    m_phyAp->SetReceiveErrorCallback(MakeCallback(&TestPhyPaddingExclusion::RxFailure, this));
    Ptr<ConstantPositionMobilityModel> apMobility = CreateObject<ConstantPositionMobilityModel>();
    m_phyAp->SetMobility(apMobility);
    apDev->SetPhy(m_phyAp);
    apDev->SetStandard(WIFI_STANDARD_80211ax);
    apDev->SetHeConfiguration(CreateObject<HeConfiguration>());
    apMac->SetWifiPhys({m_phyAp});
    apNode->AggregateObject(apMobility);
    apNode->AddDevice(apDev);

    Ptr<Node> sta1Node = CreateObject<Node>();
    Ptr<WifiNetDevice> sta1Dev = CreateObject<WifiNetDevice>();
    m_phySta1 = CreateObject<OfdmaSpectrumWifiPhy>(1);
    Ptr<InterferenceHelper> sta1InterferenceHelper = CreateObject<InterferenceHelper>();
    m_phySta1->SetInterferenceHelper(sta1InterferenceHelper);
    Ptr<ErrorRateModel> sta1ErrorModel = CreateObject<NistErrorRateModel>();
    m_phySta1->SetErrorRateModel(sta1ErrorModel);
    m_phySta1->SetDevice(sta1Dev);
    m_phySta1->AddChannel(spectrumChannel);
    m_phySta1->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phySta1->AssignStreams(streamNumber);
    m_phySta1->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, DEFAULT_CHANNEL_WIDTH, WIFI_PHY_BAND_5GHZ, 0});
    Ptr<ConstantPositionMobilityModel> sta1Mobility = CreateObject<ConstantPositionMobilityModel>();
    m_phySta1->SetMobility(sta1Mobility);
    sta1Dev->SetPhy(m_phySta1);
    sta1Dev->SetStandard(WIFI_STANDARD_80211ax);
    sta1Dev->SetHeConfiguration(CreateObject<HeConfiguration>());
    sta1Node->AggregateObject(sta1Mobility);
    sta1Node->AddDevice(sta1Dev);

    Ptr<Node> sta2Node = CreateObject<Node>();
    Ptr<WifiNetDevice> sta2Dev = CreateObject<WifiNetDevice>();
    m_phySta2 = CreateObject<OfdmaSpectrumWifiPhy>(2);
    Ptr<InterferenceHelper> sta2InterferenceHelper = CreateObject<InterferenceHelper>();
    m_phySta2->SetInterferenceHelper(sta2InterferenceHelper);
    Ptr<ErrorRateModel> sta2ErrorModel = CreateObject<NistErrorRateModel>();
    m_phySta2->SetErrorRateModel(sta2ErrorModel);
    m_phySta2->SetDevice(sta2Dev);
    m_phySta2->AddChannel(spectrumChannel);
    m_phySta2->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phySta2->AssignStreams(streamNumber);
    m_phySta2->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, DEFAULT_CHANNEL_WIDTH, WIFI_PHY_BAND_5GHZ, 0});
    Ptr<ConstantPositionMobilityModel> sta2Mobility = CreateObject<ConstantPositionMobilityModel>();
    m_phySta2->SetMobility(sta2Mobility);
    sta2Dev->SetPhy(m_phySta2);
    sta2Dev->SetStandard(WIFI_STANDARD_80211ax);
    sta2Dev->SetHeConfiguration(CreateObject<HeConfiguration>());
    sta2Node->AggregateObject(sta2Mobility);
    sta2Node->AddDevice(sta2Dev);

    Ptr<Node> interfererNode = CreateObject<Node>();
    Ptr<NonCommunicatingNetDevice> interfererDev = CreateObject<NonCommunicatingNetDevice>();
    m_phyInterferer = CreateObject<WaveformGenerator>();
    m_phyInterferer->SetDevice(interfererDev);
    m_phyInterferer->SetChannel(spectrumChannel);
    m_phyInterferer->SetDutyCycle(1);
    interfererNode->AddDevice(interfererDev);
}

void
TestPhyPaddingExclusion::DoTeardown()
{
    m_phyAp->Dispose();
    m_phyAp = nullptr;
    m_phySta1->Dispose();
    m_phySta1 = nullptr;
    m_phySta2->Dispose();
    m_phySta2 = nullptr;
    m_phyInterferer->Dispose();
    m_phyInterferer = nullptr;
}

void
TestPhyPaddingExclusion::SetTrigVector(Time ppduDuration)
{
    WifiTxVector trigVector{HePhy::GetHeMcs7(),
                            0,
                            WIFI_PREAMBLE_HE_TB,
                            NanoSeconds(1600),
                            1,
                            1,
                            0,
                            DEFAULT_CHANNEL_WIDTH,
                            false,
                            false,
                            true};
    trigVector.SetRu(HeRu::RuSpec(HeRu::RU_106_TONE, 1, false), 1);
    trigVector.SetMode(HePhy::GetHeMcs7(), 1);
    trigVector.SetNss(1, 1);
    trigVector.SetRu(HeRu::RuSpec(HeRu::RU_106_TONE, 2, false), 2);
    trigVector.SetMode(HePhy::GetHeMcs7(), 2);
    trigVector.SetNss(1, 2);
    uint16_t length;
    std::tie(length, ppduDuration) =
        HePhy::ConvertHeTbPpduDurationToLSigLength(ppduDuration, trigVector, m_phyAp->GetPhyBand());
    trigVector.SetLength(length);
    auto hePhyAp = DynamicCast<HePhy>(m_phyAp->GetLatestPhyEntity());
    hePhyAp->SetTrigVector(trigVector, ppduDuration);
}

void
TestPhyPaddingExclusion::DoRun()
{
    Time expectedPpduDuration = NanoSeconds(292800);
    Time ppduWithPaddingDuration =
        expectedPpduDuration + 10 * NanoSeconds(12800 + 1600 /* GI */); // add 10 extra OFDM symbols

    Simulator::Schedule(Seconds(0), &TestPhyPaddingExclusion::Reset, this);

    // STA1 and STA2 send MU UL PPDUs addressed to AP:
    Simulator::Schedule(Seconds(1),
                        &TestPhyPaddingExclusion::SendHeTbPpdu,
                        this,
                        1,
                        1,
                        1000,
                        ppduWithPaddingDuration);
    Simulator::Schedule(Seconds(1),
                        &TestPhyPaddingExclusion::SendHeTbPpdu,
                        this,
                        2,
                        2,
                        1001,
                        ppduWithPaddingDuration);

    // Set TRIGVECTOR on AP
    Simulator::Schedule(Seconds(1),
                        &TestPhyPaddingExclusion::SetTrigVector,
                        this,
                        ppduWithPaddingDuration);

    // Verify it takes expectedPpduDuration + padding to transmit the PPDUs
    Simulator::Schedule(Seconds(1) + ppduWithPaddingDuration - NanoSeconds(1),
                        &TestPhyPaddingExclusion::CheckPhyState,
                        this,
                        m_phyAp,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(1) + ppduWithPaddingDuration,
                        &TestPhyPaddingExclusion::CheckPhyState,
                        this,
                        m_phyAp,
                        WifiPhyState::IDLE);

    // One PSDU of 1000 bytes should have been successfully received from STA 1
    Simulator::Schedule(Seconds(1.1), &TestPhyPaddingExclusion::CheckRxFromSta1, this, 1, 0, 1000);
    // One PSDU of 1001 bytes should have been successfully received from STA 2
    Simulator::Schedule(Seconds(1.1), &TestPhyPaddingExclusion::CheckRxFromSta2, this, 1, 0, 1001);
    // Verify events data have been cleared
    Simulator::Schedule(Seconds(1.1), &TestPhyPaddingExclusion::VerifyEventsCleared, this);

    Simulator::Schedule(Seconds(1.5), &TestPhyPaddingExclusion::Reset, this);

    // STA1 and STA2 send MU UL PPDUs addressed to AP:
    Simulator::Schedule(Seconds(2),
                        &TestPhyPaddingExclusion::SendHeTbPpdu,
                        this,
                        1,
                        1,
                        1000,
                        ppduWithPaddingDuration);
    Simulator::Schedule(Seconds(2),
                        &TestPhyPaddingExclusion::SendHeTbPpdu,
                        this,
                        2,
                        2,
                        1001,
                        ppduWithPaddingDuration);

    // Set TRIGVECTOR on AP
    Simulator::Schedule(Seconds(2),
                        &TestPhyPaddingExclusion::SetTrigVector,
                        this,
                        ppduWithPaddingDuration);

    // A strong non-wifi interference is generated on RU 1 during padding reception
    BandInfo bandInfo;
    bandInfo.fc = MHzToHz(DEFAULT_FREQUENCY - (DEFAULT_CHANNEL_WIDTH / 4));
    bandInfo.fl = bandInfo.fc - MHzToHz(DEFAULT_CHANNEL_WIDTH / 4);
    bandInfo.fh = bandInfo.fc + MHzToHz(DEFAULT_CHANNEL_WIDTH / 4);
    Bands bands;
    bands.push_back(bandInfo);

    Ptr<SpectrumModel> SpectrumInterferenceRu1 = Create<SpectrumModel>(bands);
    Ptr<SpectrumValue> interferencePsdRu1 = Create<SpectrumValue>(SpectrumInterferenceRu1);
    Watt_u interferencePower{0.1};
    *interferencePsdRu1 = interferencePower / (MHzToHz(DEFAULT_CHANNEL_WIDTH / 2) * 20);

    Simulator::Schedule(Seconds(2) + MicroSeconds(50) + expectedPpduDuration,
                        &TestPhyPaddingExclusion::GenerateInterference,
                        this,
                        interferencePsdRu1,
                        MilliSeconds(100));

    // Verify it takes  expectedPpduDuration + padding to transmit the PPDUs (PHY should move to
    // CCA_BUSY instead of IDLE due to the interference)
    Simulator::Schedule(Seconds(2) + ppduWithPaddingDuration - NanoSeconds(1),
                        &TestPhyPaddingExclusion::CheckPhyState,
                        this,
                        m_phyAp,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(2) + ppduWithPaddingDuration,
                        &TestPhyPaddingExclusion::CheckPhyState,
                        this,
                        m_phyAp,
                        WifiPhyState::CCA_BUSY);

    // One PSDU of 1000 bytes should have been successfully received from STA 1 (since interference
    // occupies RU 1 after payload, during PHY padding)
    Simulator::Schedule(Seconds(2.1), &TestPhyPaddingExclusion::CheckRxFromSta1, this, 1, 0, 1000);
    // One PSDU of 1001 bytes should have been successfully received from STA 2
    Simulator::Schedule(Seconds(2.1), &TestPhyPaddingExclusion::CheckRxFromSta2, this, 1, 0, 1001);
    // Verify events data have been cleared
    Simulator::Schedule(Seconds(2.1), &TestPhyPaddingExclusion::VerifyEventsCleared, this);

    Simulator::Schedule(Seconds(2.5), &TestPhyPaddingExclusion::Reset, this);

    Simulator::Run();

    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief UL-OFDMA power control test
 */
class TestUlOfdmaPowerControl : public TestCase
{
  public:
    TestUlOfdmaPowerControl();
    ~TestUlOfdmaPowerControl() override;

  private:
    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     * Send a MU BAR through the AP to the STAs listed in the provided vector.
     *
     * @param staIds the vector of STA-IDs of STAs to address the MU-BAR to
     */
    void SendMuBar(std::vector<uint16_t> staIds);

    /**
     * Send a QoS Data packet to the destination station in order
     * to set up a block Ack session (so that the MU-BAR may have a reply).
     *
     * @param destination the address of the destination station
     */
    void SetupBa(Address destination);

    /**
     * Run one simulation with an optional BA session set up phase.
     *
     * @param setupBa true if BA session should be set up (i.e. upon first run),
     *                false otherwise
     */
    void RunOne(bool setupBa);

    /**
     * Replace the AP's callback on its PHY's ReceiveOkCallback
     * by the ReceiveOkCallbackAtAp method.
     */
    void ReplaceReceiveOkCallbackOfAp();

    /**
     * Receive OK callback function at AP.
     * This method will be plugged into the AP PHY's ReceiveOkCallback once the
     * block Ack session has been set up. This is done in the Reset function.
     * @param psdu the PSDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector the TXVECTOR used for the packet
     * @param statusPerMpdu reception status per MPDU
     */
    void ReceiveOkCallbackAtAp(Ptr<const WifiPsdu> psdu,
                               RxSignalInfo rxSignalInfo,
                               const WifiTxVector& txVector,
                               const std::vector<bool>& statusPerMpdu);

    uint8_t m_bssColor; ///< BSS color

    Ptr<WifiNetDevice> m_apDev;   ///< network device of AP
    Ptr<WifiNetDevice> m_sta1Dev; ///< network device of STA 1
    Ptr<WifiNetDevice> m_sta2Dev; ///< network device of STA 2

    Ptr<SpectrumWifiPhy> m_phyAp; ///< PHY of AP

    dBm_u m_txPowerAp;       ///< transmit power of AP
    dBm_u m_txPowerStart;    ///< minimum transmission power for STAs
    dBm_u m_txPowerEnd;      ///< maximum transmission power for STAs
    uint8_t m_txPowerLevels; ///< number of transmission power levels for STAs

    dBm_u m_requestedRssiSta1; ///< requested RSSI from STA 1 at AP for HE TB PPDUs
    dBm_u m_requestedRssiSta2; ///< requested RSSI from STA 2 at AP for HE TB PPDUs

    dBm_u m_rssiSta1; ///< expected RSSI from STA 1 at AP for HE TB PPDUs
    dBm_u m_rssiSta2; ///< expected RSSI from STA 2 at AP for HE TB PPDUs

    dB_u m_tol; ///< tolerance between received and expected RSSIs
};

TestUlOfdmaPowerControl::TestUlOfdmaPowerControl()
    : TestCase("UL-OFDMA power control test"),
      m_bssColor(1),
      m_txPowerAp(dBm_u{0}),
      m_txPowerStart(dBm_u{0}),
      m_txPowerEnd(dBm_u{0}),
      m_txPowerLevels(0),
      m_requestedRssiSta1(dBm_u{0}),
      m_requestedRssiSta2(dBm_u{0}),
      m_rssiSta1(dBm_u{0}),
      m_rssiSta2(dBm_u{0}),
      m_tol(dB_u{0.1})
{
}

TestUlOfdmaPowerControl::~TestUlOfdmaPowerControl()
{
    m_phyAp = nullptr;
    m_apDev = nullptr;
    m_sta1Dev = nullptr;
    m_sta2Dev = nullptr;
}

void
TestUlOfdmaPowerControl::SetupBa(Address destination)
{
    // Only one packet is sufficient to set up BA since AP and STAs are HE capable
    Ptr<Packet> pkt = Create<Packet>(100); // 100 dummy bytes of data
    m_apDev->Send(pkt, destination, 0);
}

void
TestUlOfdmaPowerControl::SendMuBar(std::vector<uint16_t> staIds)
{
    NS_ASSERT(!staIds.empty() && staIds.size() <= 2);

    // Build MU-BAR trigger frame
    CtrlTriggerHeader muBar;
    muBar.SetType(TriggerFrameType::MU_BAR_TRIGGER);
    muBar.SetMoreTF(true);
    muBar.SetCsRequired(true);
    muBar.SetUlBandwidth(DEFAULT_CHANNEL_WIDTH);
    muBar.SetGiAndLtfType(NanoSeconds(1600), 2);
    muBar.SetApTxPower(static_cast<int8_t>(m_txPowerAp));
    muBar.SetUlSpatialReuse(60500);

    HeRu::RuType ru = (staIds.size() == 1) ? HeRu::RU_242_TONE : HeRu::RU_106_TONE;
    std::size_t index = 1;
    int8_t ulTargetRssi = -40; // will be overwritten
    for (const auto& staId : staIds)
    {
        CtrlTriggerUserInfoField& ui = muBar.AddUserInfoField();
        ui.SetAid12(staId);
        ui.SetRuAllocation({ru, index, true});
        ui.SetUlFecCodingType(true);
        ui.SetUlMcs(7);
        ui.SetUlDcm(false);
        ui.SetSsAllocation(1, 1);
        if (staId == 1)
        {
            ulTargetRssi = m_requestedRssiSta1;
        }
        else if (staId == 2)
        {
            ulTargetRssi = m_requestedRssiSta2;
        }
        else
        {
            NS_ABORT_MSG("Unknown STA-ID (" << staId << ")");
        }
        ui.SetUlTargetRssi(ulTargetRssi);

        CtrlBAckRequestHeader bar;
        bar.SetType(BlockAckReqType::COMPRESSED);
        bar.SetTidInfo(0);
        bar.SetStartingSequence(4095);
        ui.SetMuBarTriggerDepUserInfo(bar);

        ++index;
    }

    WifiTxVector tbTxVector{muBar.GetHeTbTxVector(staIds.front())};
    muBar.SetUlLength(HePhy::ConvertHeTbPpduDurationToLSigLength(MicroSeconds(128),
                                                                 tbTxVector,
                                                                 WIFI_PHY_BAND_5GHZ)
                          .first);

    WifiConstPsduMap psdus;
    WifiTxVector txVector{HePhy::GetHeMcs7(),
                          0,
                          WIFI_PREAMBLE_HE_SU,
                          NanoSeconds(800),
                          1,
                          1,
                          0,
                          DEFAULT_CHANNEL_WIDTH,
                          false,
                          false,
                          false,
                          m_bssColor};

    auto bar = Create<Packet>();
    bar->AddHeader(muBar);

    auto receiver = Mac48Address::GetBroadcast();
    if (staIds.size() == 1)
    {
        const auto aidSta1 = DynamicCast<StaWifiMac>(m_sta1Dev->GetMac())->GetAssociationId();
        if (staIds.front() == aidSta1)
        {
            receiver = Mac48Address::ConvertFrom(m_sta1Dev->GetAddress());
        }
        else
        {
            NS_ASSERT(staIds.front() ==
                      DynamicCast<StaWifiMac>(m_sta2Dev->GetMac())->GetAssociationId());
            receiver = Mac48Address::ConvertFrom(m_sta2Dev->GetAddress());
        }
    }

    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_CTL_TRIGGER);
    hdr.SetAddr1(receiver);
    hdr.SetAddr2(Mac48Address::ConvertFrom(m_apDev->GetAddress()));
    hdr.SetAddr3(Mac48Address::ConvertFrom(m_apDev->GetAddress()));
    hdr.SetDsNotTo();
    hdr.SetDsFrom();
    hdr.SetNoRetry();
    hdr.SetNoMoreFragments();
    auto psdu = Create<WifiPsdu>(bar, hdr);

    auto nav = m_apDev->GetPhy()->GetSifs();
    const auto staId = staIds.front(); // either will do
    nav += SpectrumWifiPhy::CalculateTxDuration(GetBlockAckSize(BlockAckType::COMPRESSED),
                                                tbTxVector,
                                                DEFAULT_WIFI_BAND,
                                                staId);
    psdu->SetDuration(nav);
    psdus.insert(std::make_pair(SU_STA_ID, psdu));

    m_phyAp->Send(psdus, txVector);
}

void
TestUlOfdmaPowerControl::ReceiveOkCallbackAtAp(Ptr<const WifiPsdu> psdu,
                                               RxSignalInfo rxSignalInfo,
                                               const WifiTxVector& txVector,
                                               const std::vector<bool>& /*statusPerMpdu*/)
{
    NS_TEST_ASSERT_MSG_EQ(txVector.GetPreambleType(), WIFI_PREAMBLE_HE_TB, "HE TB PPDU expected");
    const auto rssi = rxSignalInfo.rssi;
    NS_ASSERT(psdu->GetNMpdus() == 1);
    const auto& hdr = psdu->GetHeader(0);
    NS_TEST_ASSERT_MSG_EQ(hdr.GetType(), WIFI_MAC_CTL_BACKRESP, "Block ACK expected");
    if (hdr.GetAddr2() == m_sta1Dev->GetAddress())
    {
        NS_TEST_ASSERT_MSG_EQ_TOL(
            rssi,
            m_rssiSta1,
            m_tol,
            "The obtained RSSI from STA 1 at AP is different from the expected one ("
                << rssi << " vs " << m_rssiSta1 << ", with tolerance of " << m_tol << ")");
    }
    else if (psdu->GetAddr2() == m_sta2Dev->GetAddress())
    {
        NS_TEST_ASSERT_MSG_EQ_TOL(
            rssi,
            m_rssiSta2,
            m_tol,
            "The obtained RSSI from STA 2 at AP is different from the expected one ("
                << rssi << " vs " << m_rssiSta2 << ", with tolerance of " << m_tol << ")");
    }
    else
    {
        NS_ABORT_MSG("The receiver address is unknown");
    }
}

void
TestUlOfdmaPowerControl::ReplaceReceiveOkCallbackOfAp()
{
    // Now that BA session has been established we can plug our method
    m_phyAp->SetReceiveOkCallback(
        MakeCallback(&TestUlOfdmaPowerControl::ReceiveOkCallbackAtAp, this));
}

void
TestUlOfdmaPowerControl::DoSetup()
{
    auto apNode = CreateObject<Node>();
    NodeContainer staNodes;
    staNodes.Create(2);

    auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    auto lossModel = CreateObject<MatrixPropagationLossModel>();
    spectrumChannel->AddPropagationLossModel(lossModel);
    auto delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);

    SpectrumWifiPhyHelper spectrumPhy;
    spectrumPhy.SetChannel(spectrumChannel);
    spectrumPhy.SetErrorRateModel("ns3::NistErrorRateModel");
    spectrumPhy.Set("ChannelSettings", StringValue("{0, 0, BAND_5GHZ, 0}"));

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ax);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("HeMcs7"),
                                 "ControlMode",
                                 StringValue("HeMcs7"));

    WifiMacHelper mac;
    mac.SetType("ns3::StaWifiMac");
    auto staDevs = wifi.Install(spectrumPhy, mac, staNodes);
    WifiHelper::AssignStreams(staDevs, 0);
    m_sta1Dev = DynamicCast<WifiNetDevice>(staDevs.Get(0));
    NS_ASSERT(m_sta1Dev);
    m_sta2Dev = DynamicCast<WifiNetDevice>(staDevs.Get(1));
    NS_ASSERT(m_sta2Dev);

    // Set the beacon interval long enough so that associated STAs may not consider link lost when
    // beacon generation is disabled during the actual tests. Having such a long interval also
    // avoids bloating logs with beacons during the set up phase.
    mac.SetType("ns3::ApWifiMac",
                "BeaconGeneration",
                BooleanValue(true),
                "BeaconInterval",
                TimeValue(MicroSeconds(1024 * 600)));
    m_apDev = DynamicCast<WifiNetDevice>(wifi.Install(spectrumPhy, mac, apNode).Get(0));
    NS_ASSERT(m_apDev);
    m_apDev->GetHeConfiguration()->SetAttribute("BssColor", UintegerValue(m_bssColor));
    m_phyAp = DynamicCast<SpectrumWifiPhy>(m_apDev->GetPhy());
    NS_ASSERT(m_phyAp);
    // ReceiveOkCallback of AP will be set to corresponding test's method once BA sessions have been
    // set up for both STAs

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    auto positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0)); // put close enough in order to use MCS
    positionAlloc->Add(
        Vector(2.0, 0.0, 0.0)); // STA 2 is a bit further away, but still in range of MCS
    mobility.SetPositionAllocator(positionAlloc);

    mobility.Install(apNode);
    mobility.Install(staNodes);

    lossModel->SetDefaultLoss(50.0);
    lossModel->SetLoss(apNode->GetObject<MobilityModel>(),
                       staNodes.Get(1)->GetObject<MobilityModel>(),
                       56.0,
                       true); //+6 dB between AP <-> STA 2 compared to AP <-> STA 1
}

void
TestUlOfdmaPowerControl::DoTeardown()
{
    m_phyAp->Dispose();
    m_phyAp = nullptr;
    m_apDev->Dispose();
    m_apDev = nullptr;
    m_sta1Dev->Dispose();
    m_sta1Dev = nullptr;
    m_sta2Dev->Dispose();
    m_sta2Dev = nullptr;
}

void
TestUlOfdmaPowerControl::RunOne(bool setupBa)
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 0;

    auto phySta1 = m_sta1Dev->GetPhy();
    auto phySta2 = m_sta2Dev->GetPhy();

    m_phyAp->AssignStreams(streamNumber);
    phySta1->AssignStreams(streamNumber);
    phySta2->AssignStreams(streamNumber);

    m_phyAp->SetAttribute("TxPowerStart", DoubleValue(m_txPowerAp));
    m_phyAp->SetAttribute("TxPowerEnd", DoubleValue(m_txPowerAp));
    m_phyAp->SetAttribute("TxPowerLevels", UintegerValue(1));

    phySta1->SetAttribute("TxPowerStart", DoubleValue(m_txPowerStart));
    phySta1->SetAttribute("TxPowerEnd", DoubleValue(m_txPowerEnd));
    phySta1->SetAttribute("TxPowerLevels", UintegerValue(m_txPowerLevels));

    phySta2->SetAttribute("TxPowerStart", DoubleValue(m_txPowerStart));
    phySta2->SetAttribute("TxPowerEnd", DoubleValue(m_txPowerEnd));
    phySta2->SetAttribute("TxPowerLevels", UintegerValue(m_txPowerLevels));

    Time relativeStart{};
    if (setupBa)
    {
        // Set up BA for each station once the association phase has ended
        // so that a BA session is established when the MU-BAR is received.
        Simulator::Schedule(MilliSeconds(800),
                            &TestUlOfdmaPowerControl::SetupBa,
                            this,
                            m_sta1Dev->GetAddress());
        Simulator::Schedule(MilliSeconds(850),
                            &TestUlOfdmaPowerControl::SetupBa,
                            this,
                            m_sta2Dev->GetAddress());
        relativeStart = MilliSeconds(1000);
    }
    else
    {
        auto apMac = DynamicCast<ApWifiMac>(m_apDev->GetMac());
        NS_ASSERT(apMac);
        apMac->SetAttribute("BeaconGeneration", BooleanValue(false));
    }

    Simulator::Schedule(relativeStart,
                        &TestUlOfdmaPowerControl::ReplaceReceiveOkCallbackOfAp,
                        this);

    {
        // Verify that the RSSI from STA 1 is consistent with what was requested
        std::vector<uint16_t> staIds{1};
        Simulator::Schedule(relativeStart, &TestUlOfdmaPowerControl::SendMuBar, this, staIds);
    }

    {
        // Verify that the RSSI from STA 2 is consistent with what was requested
        std::vector<uint16_t> staIds{2};
        Simulator::Schedule(relativeStart + MilliSeconds(20),
                            &TestUlOfdmaPowerControl::SendMuBar,
                            this,
                            staIds);
    }

    {
        // Verify that the RSSI from STA 1 and 2 is consistent with what was requested
        std::vector<uint16_t> staIds{1, 2};
        Simulator::Schedule(relativeStart + MilliSeconds(40),
                            &TestUlOfdmaPowerControl::SendMuBar,
                            this,
                            staIds);
    }

    Simulator::Stop(relativeStart + MilliSeconds(100));
    Simulator::Run();
}

void
TestUlOfdmaPowerControl::DoRun()
{
    // Power configurations
    m_txPowerAp = dBm_u{20}; // so as to have -30 and -36 dBm at STA 1 and STA 2 resp., since path
                             // loss = 50 dB for AP <-> STA 1 and 56 dB for AP <-> STA 2
    m_txPowerStart = dBm_u{15};

    // Requested UL RSSIs: should correspond to 20 dBm transmit power at STAs
    m_requestedRssiSta1 = dBm_u{-30};
    m_requestedRssiSta2 = dBm_u{-36};

    // Test single power level
    {
        // STA power configurations: 15 dBm only
        m_txPowerEnd = dBm_u{15};
        m_txPowerLevels = 1;

        // Expected UL RSSIs, considering that the provided power is 5 dB less than requested,
        // regardless of the estimated path loss.
        m_rssiSta1 = dBm_u{-35}; // 15 dBm - 50 dB
        m_rssiSta2 = dBm_u{-41}; // 15 dBm - 56 dB

        RunOne(true);
    }

    // Test 2 dBm granularity
    {
        // STA power configurations: [15:2:25] dBm
        m_txPowerEnd = dBm_u{25};
        m_txPowerLevels = 6;

        // Expected UL RSSIs, considering that the provided power (21 dBm) is 1 dB more than
        // requested
        m_rssiSta1 = dBm_u{-29}; // 21 dBm - 50 dB
        m_rssiSta2 = dBm_u{-35}; // 21 dBm - 50 dB

        RunOne(false);
    }

    // Test 1 dBm granularity
    {
        // STA power configurations: [15:1:25] dBm
        m_txPowerEnd = dBm_u{25};
        m_txPowerLevels = 11;

        // Expected UL RSSIs, considering that we can correctly tune the transmit power
        m_rssiSta1 = dBm_u{-30}; // 20 dBm - 50 dB
        m_rssiSta2 = dBm_u{-36}; // 20 dBm - 56 dB

        RunOne(false);
    }

    // Ask for different power levels (3 dB difference between HE_TB_PPDUs)
    {
        // STA power configurations: [15:1:25] dBm
        m_txPowerEnd = dBm_u{25};
        m_txPowerLevels = 11;

        // Requested UL RSSIs
        m_requestedRssiSta1 =
            dBm_u{-28}; // 2 dB higher than previously -> Tx power = 22 dBm at STA 1
        m_requestedRssiSta2 = dBm_u{-37}; // 1 dB less than previously -> Tx power = 19 dBm at STA 2

        // Expected UL RSSIs, considering that we can correctly tune the transmit power
        m_rssiSta1 = dBm_u{-28}; // 22 dBm - 50 dB
        m_rssiSta2 = dBm_u{-37}; // 19 dBm - 56 dB

        RunOne(false);
    }

    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi PHY OFDMA Test Suite
 */
class WifiPhyOfdmaTestSuite : public TestSuite
{
  public:
    WifiPhyOfdmaTestSuite();
};

WifiPhyOfdmaTestSuite::WifiPhyOfdmaTestSuite()
    : TestSuite("wifi-phy-ofdma", Type::UNIT)
{
    AddTestCase(new TestDlOfdmaPhyTransmission, TestCase::Duration::QUICK);
    AddTestCase(new TestDlOfdmaPhyPuncturing, TestCase::Duration::QUICK);
    AddTestCase(new TestUlOfdmaPpduUid, TestCase::Duration::QUICK);
    AddTestCase(new TestMultipleHeTbPreambles, TestCase::Duration::QUICK);
    AddTestCase(new TestUlOfdmaPhyTransmission, TestCase::Duration::QUICK);
    AddTestCase(new TestPhyPaddingExclusion, TestCase::Duration::QUICK);
    AddTestCase(new TestUlOfdmaPowerControl, TestCase::Duration::QUICK);
}

static WifiPhyOfdmaTestSuite wifiPhyOfdmaTestSuite; ///< the test suite
