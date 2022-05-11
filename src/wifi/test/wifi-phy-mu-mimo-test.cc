/*
 * Copyright (c) 2022 DERONNE SOFTWARE ENGINEERING
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

#include "ns3/he-phy.h"
#include "ns3/he-ppdu.h"
#include "ns3/interference-helper.h"
#include "ns3/log.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/node.h"
#include "ns3/pointer.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/test.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-spectrum-signal-parameters.h"
#include "ns3/wifi-spectrum-value-helper.h"
#include "ns3/wifi-utils.h"

#include <list>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiPhyMuMimoTest");

constexpr uint32_t DEFAULT_FREQUENCY = 5180;   // MHz
constexpr uint16_t DEFAULT_CHANNEL_WIDTH = 20; // MHz

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief DL MU TX-VECTOR test
 */
class TestDlMuTxVector : public TestCase
{
  public:
    TestDlMuTxVector();

  private:
    void DoRun() override;

    /**
     * Build a TXVECTOR for DL MU with the given bandwidth and user information.
     *
     * \param bw the channel width of the PPDU in MHz
     * \param userInfos the list of HE MU specific user transmission parameters
     *
     * \return the configured MU TXVECTOR
     */
    static WifiTxVector BuildTxVector(uint16_t bw, const std::list<HeMuUserInfo>& userInfos);
};

TestDlMuTxVector::TestDlMuTxVector()
    : TestCase("Check for valid combinations of MU TX-VECTOR")
{
}

WifiTxVector
TestDlMuTxVector::BuildTxVector(uint16_t bw, const std::list<HeMuUserInfo>& userInfos)
{
    WifiTxVector txVector;
    txVector.SetPreambleType(WIFI_PREAMBLE_HE_MU);
    txVector.SetChannelWidth(bw);
    std::list<uint16_t> staIds;
    uint16_t staId = 1;
    for (const auto& userInfo : userInfos)
    {
        txVector.SetHeMuUserInfo(staId, userInfo);
        staIds.push_back(staId++);
    }
    return txVector;
}

void
TestDlMuTxVector::DoRun()
{
    // Verify TxVector is OFDMA
    std::list<HeMuUserInfo> userInfos;
    userInfos.push_back({{HeRu::RU_106_TONE, 1, true}, 11, 1});
    userInfos.push_back({{HeRu::RU_106_TONE, 2, true}, 10, 2});
    WifiTxVector txVector = BuildTxVector(20, userInfos);
    NS_TEST_EXPECT_MSG_EQ(txVector.IsDlOfdma(),
                          true,
                          "TX-VECTOR should indicate an OFDMA transmission");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsDlMuMimo(),
                          false,
                          "TX-VECTOR should not indicate a MU-MIMO transmission");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsSigBCompression(),
                          false,
                          "TX-VECTOR should not indicate a SIG-B compression");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsValid(),
                          true,
                          "TX-VECTOR should indicate all checks are passed");
    userInfos.clear();

    // Verify TxVector is a full BW MU-MIMO
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 11, 1});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 10, 2});
    txVector = BuildTxVector(20, userInfos);
    NS_TEST_EXPECT_MSG_EQ(txVector.IsDlOfdma(),
                          false,
                          "TX-VECTOR should indicate a MU-MIMO transmission");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsDlMuMimo(),
                          true,
                          "TX-VECTOR should not indicate an OFDMA transmission");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsSigBCompression(),
                          true,
                          "TX-VECTOR should indicate a SIG-B compression");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsValid(),
                          true,
                          "TX-VECTOR should indicate all checks are passed");
    userInfos.clear();

    // Verify TxVector is not valid if there are more than 8 STAs using the same RU
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 11, 1});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 10, 1});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 9, 1});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 8, 1});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 7, 1});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 6, 1});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 5, 1});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 4, 1});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 3, 1});
    txVector = BuildTxVector(20, userInfos);
    NS_TEST_EXPECT_MSG_EQ(txVector.IsDlOfdma(),
                          false,
                          "TX-VECTOR should indicate a MU-MIMO transmission");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsDlMuMimo(),
                          true,
                          "TX-VECTOR should not indicate an OFDMA transmission");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsSigBCompression(),
                          true,
                          "TX-VECTOR should indicate a SIG-B compression");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsValid(),
                          false,
                          "TX-VECTOR should not indicate all checks are passed");

    // Verify TxVector is not valid if the total number of antennas in a full BW MU-MIMO is above 8
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 11, 2});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 10, 2});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 9, 3});
    userInfos.push_back({{HeRu::RU_242_TONE, 1, true}, 8, 3});
    txVector = BuildTxVector(20, userInfos);
    NS_TEST_EXPECT_MSG_EQ(txVector.IsDlOfdma(),
                          false,
                          "TX-VECTOR should indicate a MU-MIMO transmission");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsDlMuMimo(),
                          true,
                          "TX-VECTOR should not indicate an OFDMA transmission");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsSigBCompression(),
                          true,
                          "TX-VECTOR should indicate a SIG-B compression");
    NS_TEST_EXPECT_MSG_EQ(txVector.IsValid(),
                          false,
                          "TX-VECTOR should not indicate all checks are passed");
}

/**
 * HE PHY slightly modified so as to return a given
 * STA-ID in case of DL MU for MuMimoSpectrumWifiPhy.
 */
class MuMimoTestHePhy : public HePhy
{
  public:
    /**
     * Constructor
     *
     * \param staId the ID of the STA to which this PHY belongs to
     */
    MuMimoTestHePhy(uint16_t staId);

    /**
     * Return the STA ID that has been assigned to the station this PHY belongs to.
     * This is typically called for MU PPDUs, in order to pick the correct PSDU.
     *
     * \param ppdu the PPDU for which the STA ID is requested
     * \return the STA ID
     */
    uint16_t GetStaId(const Ptr<const WifiPpdu> ppdu) const override;

  private:
    uint16_t m_staId; ///< ID of the STA to which this PHY belongs to
};                    // class MuMimoTestHePhy

MuMimoTestHePhy::MuMimoTestHePhy(uint16_t staId)
    : HePhy(),
      m_staId(staId)
{
}

uint16_t
MuMimoTestHePhy::GetStaId(const Ptr<const WifiPpdu> ppdu) const
{
    if (ppdu->GetType() == WIFI_PPDU_TYPE_DL_MU)
    {
        return m_staId;
    }
    return HePhy::GetStaId(ppdu);
}

/**
 * SpectrumWifiPhy used for testing MU-MIMO.
 */
class MuMimoSpectrumWifiPhy : public SpectrumWifiPhy
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    /**
     * Constructor
     *
     * \param staId the ID of the STA to which this PHY belongs to
     */
    MuMimoSpectrumWifiPhy(uint16_t staId);
    ~MuMimoSpectrumWifiPhy() override;

    void DoInitialize() override;
    void DoDispose() override;

  private:
    Ptr<MuMimoTestHePhy> m_ofdmTestHePhy; ///< Pointer to HE PHY instance used for MU-MIMO test
};                                        // class MuMimoSpectrumWifiPhy

TypeId
MuMimoSpectrumWifiPhy::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::MuMimoSpectrumWifiPhy").SetParent<SpectrumWifiPhy>().SetGroupName("Wifi");
    return tid;
}

MuMimoSpectrumWifiPhy::MuMimoSpectrumWifiPhy(uint16_t staId)
    : SpectrumWifiPhy()
{
    m_ofdmTestHePhy = Create<MuMimoTestHePhy>(staId);
    m_ofdmTestHePhy->SetOwner(this);
}

MuMimoSpectrumWifiPhy::~MuMimoSpectrumWifiPhy()
{
}

void
MuMimoSpectrumWifiPhy::DoInitialize()
{
    // Replace HE PHY instance with test instance
    m_phyEntities[WIFI_MOD_CLASS_HE] = m_ofdmTestHePhy;
    SpectrumWifiPhy::DoInitialize();
}

void
MuMimoSpectrumWifiPhy::DoDispose()
{
    m_ofdmTestHePhy = nullptr;
    SpectrumWifiPhy::DoDispose();
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief DL MU-MIMO PHY test
 */
class TestDlMuMimoPhyTransmission : public TestCase
{
  public:
    TestDlMuMimoPhyTransmission();

  private:
    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     * Receive success function for STA 1
     * \param psdu the PSDU
     * \param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * \param txVector the transmit vector
     * \param statusPerMpdu reception status per MPDU
     */
    void RxSuccessSta1(Ptr<const WifiPsdu> psdu,
                       RxSignalInfo rxSignalInfo,
                       WifiTxVector txVector,
                       std::vector<bool> statusPerMpdu);
    /**
     * Receive success function for STA 2
     * \param psdu the PSDU
     * \param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * \param txVector the transmit vector
     * \param statusPerMpdu reception status per MPDU
     */
    void RxSuccessSta2(Ptr<const WifiPsdu> psdu,
                       RxSignalInfo rxSignalInfo,
                       WifiTxVector txVector,
                       std::vector<bool> statusPerMpdu);
    /**
     * Receive success function for STA 3
     * \param psdu the PSDU
     * \param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * \param txVector the transmit vector
     * \param statusPerMpdu reception status per MPDU
     */
    void RxSuccessSta3(Ptr<const WifiPsdu> psdu,
                       RxSignalInfo rxSignalInfo,
                       WifiTxVector txVector,
                       std::vector<bool> statusPerMpdu);

    /**
     * Receive failure function for STA 1
     * \param psdu the PSDU
     */
    void RxFailureSta1(Ptr<const WifiPsdu> psdu);
    /**
     * Receive failure function for STA 2
     * \param psdu the PSDU
     */
    void RxFailureSta2(Ptr<const WifiPsdu> psdu);
    /**
     * Receive failure function for STA 3
     * \param psdu the PSDU
     */
    void RxFailureSta3(Ptr<const WifiPsdu> psdu);

    /**
     * Check the results for STA 1
     * \param expectedRxSuccess the expected number of RX success
     * \param expectedRxFailure the expected number of RX failures
     * \param expectedRxBytes the expected number of RX bytes
     */
    void CheckResultsSta1(uint32_t expectedRxSuccess,
                          uint32_t expectedRxFailure,
                          uint32_t expectedRxBytes);
    /**
     * Check the results for STA 2
     * \param expectedRxSuccess the expected number of RX success
     * \param expectedRxFailure the expected number of RX failures
     * \param expectedRxBytes the expected number of RX bytes
     */
    void CheckResultsSta2(uint32_t expectedRxSuccess,
                          uint32_t expectedRxFailure,
                          uint32_t expectedRxBytes);
    /**
     * Check the results for STA 3
     * \param expectedRxSuccess the expected number of RX success
     * \param expectedRxFailure the expected number of RX failures
     * \param expectedRxBytes the expected number of RX bytes
     */
    void CheckResultsSta3(uint32_t expectedRxSuccess,
                          uint32_t expectedRxFailure,
                          uint32_t expectedRxBytes);

    /**
     * Reset the results
     */
    void ResetResults();

    /**
     * STA info
     */
    struct StaInfo
    {
        uint16_t staId; //!< STA ID
        uint8_t staNss; //!< Number of spatial streams used for the STA
    };

    /**
     * Send DL MU-MIMO PPDU function
     * \param staInfos the STAs infos
     */
    void SendMuPpdu(const std::vector<StaInfo>& staInfos);

    /**
     * Generate interference function
     * \param interferencePsd the PSD of the interference to be generated
     * \param duration the duration of the interference
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
     * \param phy the PHY
     * \param expectedState the expected state of the PHY
     */
    void CheckPhyState(Ptr<MuMimoSpectrumWifiPhy> phy, WifiPhyState expectedState);
    /**
     * Check the PHY state now
     * \param phy the PHY
     * \param expectedState the expected state of the PHY
     */
    void DoCheckPhyState(Ptr<MuMimoSpectrumWifiPhy> phy, WifiPhyState expectedState);

    uint32_t m_countRxSuccessSta1; ///< count RX success for STA 1
    uint32_t m_countRxSuccessSta2; ///< count RX success for STA 2
    uint32_t m_countRxSuccessSta3; ///< count RX success for STA 3
    uint32_t m_countRxFailureSta1; ///< count RX failure for STA 1
    uint32_t m_countRxFailureSta2; ///< count RX failure for STA 2
    uint32_t m_countRxFailureSta3; ///< count RX failure for STA 3
    uint32_t m_countRxBytesSta1;   ///< count RX bytes for STA 1
    uint32_t m_countRxBytesSta2;   ///< count RX bytes for STA 2
    uint32_t m_countRxBytesSta3;   ///< count RX bytes for STA 3

    Ptr<SpectrumWifiPhy> m_phyAp;         ///< PHY of AP
    Ptr<MuMimoSpectrumWifiPhy> m_phySta1; ///< PHY of STA 1
    Ptr<MuMimoSpectrumWifiPhy> m_phySta2; ///< PHY of STA 2
    Ptr<MuMimoSpectrumWifiPhy> m_phySta3; ///< PHY of STA 3

    uint8_t m_nss;               ///< number of spatial streams per STA
    uint16_t m_frequency;        ///< frequency in MHz
    uint16_t m_channelWidth;     ///< channel width in MHz
    Time m_expectedPpduDuration; ///< expected duration to send MU PPDU
};

TestDlMuMimoPhyTransmission::TestDlMuMimoPhyTransmission()
    : TestCase("DL MU-MIMO PHY test"),
      m_countRxSuccessSta1{0},
      m_countRxSuccessSta2{0},
      m_countRxSuccessSta3{0},
      m_countRxFailureSta1{0},
      m_countRxFailureSta2{0},
      m_countRxFailureSta3{0},
      m_countRxBytesSta1{0},
      m_countRxBytesSta2{0},
      m_countRxBytesSta3{0},
      m_nss{1},
      m_frequency{DEFAULT_FREQUENCY},
      m_channelWidth{DEFAULT_CHANNEL_WIDTH},
      m_expectedPpduDuration{NanoSeconds(306400)}
{
}

void
TestDlMuMimoPhyTransmission::ResetResults()
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
TestDlMuMimoPhyTransmission::SendMuPpdu(const std::vector<StaInfo>& staInfos)
{
    NS_LOG_FUNCTION(this << staInfos.size());
    NS_ASSERT(staInfos.size() > 1);

    WifiTxVector txVector = WifiTxVector(HePhy::GetHeMcs7(),
                                         0,
                                         WIFI_PREAMBLE_HE_MU,
                                         800,
                                         1,
                                         1,
                                         0,
                                         m_channelWidth,
                                         false,
                                         false);

    WifiConstPsduMap psdus;
    HeRu::RuSpec ru(HeRu::GetRuType(m_channelWidth), 1, true); // full BW MU-MIMO
    for (const auto& staInfo : staInfos)
    {
        txVector.SetRu(ru, staInfo.staId);
        txVector.SetMode(HePhy::GetHeMcs7(), staInfo.staId);
        txVector.SetNss(staInfo.staNss, staInfo.staId);

        Ptr<Packet> pkt = Create<Packet>(1000 + (8 * staInfo.staId));
        WifiMacHeader hdr;
        hdr.SetType(WIFI_MAC_QOSDATA);
        hdr.SetQosTid(0);
        std::ostringstream addr;
        addr << "00:00:00:00:00:0" << staInfo.staId;
        hdr.SetAddr1(Mac48Address(addr.str().c_str()));
        hdr.SetSequenceNumber(1 + staInfo.staId);
        Ptr<WifiPsdu> psdu = Create<WifiPsdu>(pkt, hdr);
        psdus.insert(std::make_pair(staInfo.staId, psdu));
    }

    txVector.SetSigBMode(VhtPhy::GetVhtMcs5());

    NS_ASSERT(txVector.IsDlMuMimo());
    NS_ASSERT(!txVector.IsDlOfdma());

    m_phyAp->Send(psdus, txVector);
}

void
TestDlMuMimoPhyTransmission::RxSuccessSta1(Ptr<const WifiPsdu> psdu,
                                           RxSignalInfo rxSignalInfo,
                                           WifiTxVector txVector,
                                           std::vector<bool> /*statusPerMpdu*/)
{
    NS_LOG_FUNCTION(this << *psdu << rxSignalInfo << txVector);
    m_countRxSuccessSta1++;
    m_countRxBytesSta1 += (psdu->GetSize() - 30);
}

void
TestDlMuMimoPhyTransmission::RxSuccessSta2(Ptr<const WifiPsdu> psdu,
                                           RxSignalInfo rxSignalInfo,
                                           WifiTxVector txVector,
                                           std::vector<bool> /*statusPerMpdu*/)
{
    NS_LOG_FUNCTION(this << *psdu << rxSignalInfo << txVector);
    m_countRxSuccessSta2++;
    m_countRxBytesSta2 += (psdu->GetSize() - 30);
}

void
TestDlMuMimoPhyTransmission::RxSuccessSta3(Ptr<const WifiPsdu> psdu,
                                           RxSignalInfo rxSignalInfo,
                                           WifiTxVector txVector,
                                           std::vector<bool> /*statusPerMpdu*/)
{
    NS_LOG_FUNCTION(this << *psdu << rxSignalInfo << txVector);
    m_countRxSuccessSta3++;
    m_countRxBytesSta3 += (psdu->GetSize() - 30);
}

void
TestDlMuMimoPhyTransmission::RxFailureSta1(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << *psdu);
    m_countRxFailureSta1++;
}

void
TestDlMuMimoPhyTransmission::RxFailureSta2(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << *psdu);
    m_countRxFailureSta2++;
}

void
TestDlMuMimoPhyTransmission::RxFailureSta3(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << *psdu);
    m_countRxFailureSta3++;
}

void
TestDlMuMimoPhyTransmission::CheckResultsSta1(uint32_t expectedRxSuccess,
                                              uint32_t expectedRxFailure,
                                              uint32_t expectedRxBytes)
{
    NS_TEST_ASSERT_MSG_EQ(m_countRxSuccessSta1,
                          expectedRxSuccess,
                          "The number of successfully received packets by STA 1 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxFailureSta1,
                          expectedRxFailure,
                          "The number of unsuccessfully received packets by STA 1 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxBytesSta1,
                          expectedRxBytes,
                          "The number of bytes received by STA 1 is not correct!");
}

void
TestDlMuMimoPhyTransmission::CheckResultsSta2(uint32_t expectedRxSuccess,
                                              uint32_t expectedRxFailure,
                                              uint32_t expectedRxBytes)
{
    NS_TEST_ASSERT_MSG_EQ(m_countRxSuccessSta2,
                          expectedRxSuccess,
                          "The number of successfully received packets by STA 2 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxFailureSta2,
                          expectedRxFailure,
                          "The number of unsuccessfully received packets by STA 2 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxBytesSta2,
                          expectedRxBytes,
                          "The number of bytes received by STA 2 is not correct!");
}

void
TestDlMuMimoPhyTransmission::CheckResultsSta3(uint32_t expectedRxSuccess,
                                              uint32_t expectedRxFailure,
                                              uint32_t expectedRxBytes)
{
    NS_TEST_ASSERT_MSG_EQ(m_countRxSuccessSta3,
                          expectedRxSuccess,
                          "The number of successfully received packets by STA 3 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxFailureSta3,
                          expectedRxFailure,
                          "The number of unsuccessfully received packets by STA 3 is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxBytesSta3,
                          expectedRxBytes,
                          "The number of bytes received by STA 3 is not correct!");
}

void
TestDlMuMimoPhyTransmission::CheckPhyState(Ptr<MuMimoSpectrumWifiPhy> phy,
                                           WifiPhyState expectedState)
{
    // This is needed to make sure PHY state will be checked as the last event if a state change
    // occurred at the exact same time as the check
    Simulator::ScheduleNow(&TestDlMuMimoPhyTransmission::DoCheckPhyState, this, phy, expectedState);
}

void
TestDlMuMimoPhyTransmission::DoCheckPhyState(Ptr<MuMimoSpectrumWifiPhy> phy,
                                             WifiPhyState expectedState)
{
    WifiPhyState currentState;
    PointerValue ptr;
    phy->GetAttribute("State", ptr);
    Ptr<WifiPhyStateHelper> state = DynamicCast<WifiPhyStateHelper>(ptr.Get<WifiPhyStateHelper>());
    currentState = state->GetState();
    NS_LOG_FUNCTION(this << currentState << expectedState);
    NS_TEST_ASSERT_MSG_EQ(currentState,
                          expectedState,
                          "PHY State " << currentState << " does not match expected state "
                                       << expectedState << " at " << Simulator::Now());
}

void
TestDlMuMimoPhyTransmission::DoSetup()
{
    Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
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
    apDev->SetPhy(m_phyAp);
    apNode->AddDevice(apDev);

    Ptr<Node> sta1Node = CreateObject<Node>();
    Ptr<WifiNetDevice> sta1Dev = CreateObject<WifiNetDevice>();
    m_phySta1 = CreateObject<MuMimoSpectrumWifiPhy>(1);
    Ptr<InterferenceHelper> sta1InterferenceHelper = CreateObject<InterferenceHelper>();
    m_phySta1->SetInterferenceHelper(sta1InterferenceHelper);
    Ptr<ErrorRateModel> sta1ErrorModel = CreateObject<NistErrorRateModel>();
    m_phySta1->SetErrorRateModel(sta1ErrorModel);
    m_phySta1->SetDevice(sta1Dev);
    m_phySta1->AddChannel(spectrumChannel);
    m_phySta1->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phySta1->SetReceiveOkCallback(
        MakeCallback(&TestDlMuMimoPhyTransmission::RxSuccessSta1, this));
    m_phySta1->SetReceiveErrorCallback(
        MakeCallback(&TestDlMuMimoPhyTransmission::RxFailureSta1, this));
    sta1Dev->SetPhy(m_phySta1);
    sta1Node->AddDevice(sta1Dev);

    Ptr<Node> sta2Node = CreateObject<Node>();
    Ptr<WifiNetDevice> sta2Dev = CreateObject<WifiNetDevice>();
    m_phySta2 = CreateObject<MuMimoSpectrumWifiPhy>(2);
    Ptr<InterferenceHelper> sta2InterferenceHelper = CreateObject<InterferenceHelper>();
    m_phySta2->SetInterferenceHelper(sta2InterferenceHelper);
    Ptr<ErrorRateModel> sta2ErrorModel = CreateObject<NistErrorRateModel>();
    m_phySta2->SetErrorRateModel(sta2ErrorModel);
    m_phySta2->SetDevice(sta2Dev);
    m_phySta2->AddChannel(spectrumChannel);
    m_phySta2->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phySta2->SetReceiveOkCallback(
        MakeCallback(&TestDlMuMimoPhyTransmission::RxSuccessSta2, this));
    m_phySta2->SetReceiveErrorCallback(
        MakeCallback(&TestDlMuMimoPhyTransmission::RxFailureSta2, this));
    sta2Dev->SetPhy(m_phySta2);
    sta2Node->AddDevice(sta2Dev);

    Ptr<Node> sta3Node = CreateObject<Node>();
    Ptr<WifiNetDevice> sta3Dev = CreateObject<WifiNetDevice>();
    m_phySta3 = CreateObject<MuMimoSpectrumWifiPhy>(3);
    Ptr<InterferenceHelper> sta3InterferenceHelper = CreateObject<InterferenceHelper>();
    m_phySta3->SetInterferenceHelper(sta3InterferenceHelper);
    Ptr<ErrorRateModel> sta3ErrorModel = CreateObject<NistErrorRateModel>();
    m_phySta3->SetErrorRateModel(sta3ErrorModel);
    m_phySta3->SetDevice(sta3Dev);
    m_phySta3->AddChannel(spectrumChannel);
    m_phySta3->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phySta3->SetReceiveOkCallback(
        MakeCallback(&TestDlMuMimoPhyTransmission::RxSuccessSta3, this));
    m_phySta3->SetReceiveErrorCallback(
        MakeCallback(&TestDlMuMimoPhyTransmission::RxFailureSta3, this));
    sta3Dev->SetPhy(m_phySta3);
    sta3Node->AddDevice(sta3Dev);
}

void
TestDlMuMimoPhyTransmission::DoTeardown()
{
    m_phyAp->Dispose();
    m_phyAp = nullptr;
    m_phySta1->Dispose();
    m_phySta1 = nullptr;
    m_phySta2->Dispose();
    m_phySta2 = nullptr;
    m_phySta3->Dispose();
    m_phySta3 = nullptr;
}

void
TestDlMuMimoPhyTransmission::RunOne()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 0;
    m_phyAp->AssignStreams(streamNumber);
    m_phySta1->AssignStreams(streamNumber);
    m_phySta2->AssignStreams(streamNumber);

    auto channelNum = std::get<0>(*WifiPhyOperatingChannel::FindFirst(0,
                                                                      m_frequency,
                                                                      m_channelWidth,
                                                                      WIFI_STANDARD_80211ax,
                                                                      WIFI_PHY_BAND_5GHZ));

    m_phyAp->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, m_channelWidth, WIFI_PHY_BAND_5GHZ, 0});
    m_phySta1->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, m_channelWidth, WIFI_PHY_BAND_5GHZ, 0});
    m_phySta2->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, m_channelWidth, WIFI_PHY_BAND_5GHZ, 0});
    m_phySta3->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, m_channelWidth, WIFI_PHY_BAND_5GHZ, 0});

    m_phyAp->SetNumberOfAntennas(8);
    m_phyAp->SetMaxSupportedTxSpatialStreams(8);

    //----------------------------------------------------------------------------------------------------
    // Send MU PPDU with two PSDUs addressed to STA 1 and STA 2:
    // STA 1 and STA 2 should receive their PSDUs, whereas STA 3 should not receive any PSDU
    // but should keep its PHY busy during all PPDU duration.
    Simulator::Schedule(Seconds(1.0),
                        &TestDlMuMimoPhyTransmission::SendMuPpdu,
                        this,
                        std::vector<StaInfo>{{1, m_nss}, {2, m_nss}});

    // Since it takes m_expectedPpduDuration to transmit the PPDU,
    // all 3 PHYs should be back to IDLE at the same time,
    // even the PHY that has no PSDU addressed to it.
    Simulator::Schedule(Seconds(1.0) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta1,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(1.0) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta2,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(1.0) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta3,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(1.0) + m_expectedPpduDuration,
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta1,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(1.0) + m_expectedPpduDuration,
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta2,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(1.0) + m_expectedPpduDuration,
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta3,
                        WifiPhyState::IDLE);

    // One PSDU of 1008 bytes should have been successfully received by STA 1
    Simulator::Schedule(Seconds(1.1),
                        &TestDlMuMimoPhyTransmission::CheckResultsSta1,
                        this,
                        1,
                        0,
                        1008);
    // One PSDU of 1016 bytes should have been successfully received by STA 2
    Simulator::Schedule(Seconds(1.1),
                        &TestDlMuMimoPhyTransmission::CheckResultsSta2,
                        this,
                        1,
                        0,
                        1016);
    // No PSDU should have been received by STA 3
    Simulator::Schedule(Seconds(1.1),
                        &TestDlMuMimoPhyTransmission::CheckResultsSta3,
                        this,
                        0,
                        0,
                        0);

    Simulator::Schedule(Seconds(1.5), &TestDlMuMimoPhyTransmission::ResetResults, this);

    //----------------------------------------------------------------------------------------------------
    // Send MU PPDU with two PSDUs addressed to STA 1 and STA 3:
    // STA 1 and STA 3 should receive their PSDUs, whereas STA 2 should not receive any PSDU
    // but should keep its PHY busy during all PPDU duration.
    Simulator::Schedule(Seconds(2.0),
                        &TestDlMuMimoPhyTransmission::SendMuPpdu,
                        this,
                        std::vector<StaInfo>{{1, m_nss}, {3, m_nss}});

    // Since it takes m_expectedPpduDuration to transmit the PPDU,
    // all 3 PHYs should be back to IDLE at the same time,
    // even the PHY that has no PSDU addressed to it.
    Simulator::Schedule(Seconds(2.0) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta1,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(2.0) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta2,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(2.0) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta3,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(2.0) + m_expectedPpduDuration,
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta1,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(2.0) + m_expectedPpduDuration,
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta2,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(2.0) + m_expectedPpduDuration,
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta3,
                        WifiPhyState::IDLE);

    // One PSDU of 1008 bytes should have been successfully received by STA 1
    Simulator::Schedule(Seconds(2.1),
                        &TestDlMuMimoPhyTransmission::CheckResultsSta1,
                        this,
                        1,
                        0,
                        1008);
    // No PSDU should have been received by STA 2
    Simulator::Schedule(Seconds(2.1),
                        &TestDlMuMimoPhyTransmission::CheckResultsSta2,
                        this,
                        0,
                        0,
                        0);
    // One PSDU of 1024 bytes should have been successfully received by STA 3
    Simulator::Schedule(Seconds(2.1),
                        &TestDlMuMimoPhyTransmission::CheckResultsSta3,
                        this,
                        1,
                        0,
                        1024);

    Simulator::Schedule(Seconds(2.5), &TestDlMuMimoPhyTransmission::ResetResults, this);

    //----------------------------------------------------------------------------------------------------
    // Send MU PPDU with two PSDUs addressed to STA 2 and STA 3:
    // STA 2 and STA 3 should receive their PSDUs, whereas STA 1 should not receive any PSDU
    // but should keep its PHY busy during all PPDU duration.
    Simulator::Schedule(Seconds(3.0),
                        &TestDlMuMimoPhyTransmission::SendMuPpdu,
                        this,
                        std::vector<StaInfo>{{2, m_nss}, {3, m_nss}});

    // Since it takes m_expectedPpduDuration to transmit the PPDU,
    // all 3 PHYs should be back to IDLE at the same time,
    // even the PHY that has no PSDU addressed to it.
    Simulator::Schedule(Seconds(3.0) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta1,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(3.0) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta2,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(3.0) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta3,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(3.0) + m_expectedPpduDuration,
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta1,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(3.0) + m_expectedPpduDuration,
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta2,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(3.0) + m_expectedPpduDuration,
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta3,
                        WifiPhyState::IDLE);

    // No PSDU should have been received by STA 1
    Simulator::Schedule(Seconds(3.1),
                        &TestDlMuMimoPhyTransmission::CheckResultsSta1,
                        this,
                        0,
                        0,
                        0);
    // One PSDU of 1016 bytes should have been successfully received by STA 2
    Simulator::Schedule(Seconds(3.1),
                        &TestDlMuMimoPhyTransmission::CheckResultsSta2,
                        this,
                        1,
                        0,
                        1016);
    // One PSDU of 1024 bytes should have been successfully received by STA 3
    Simulator::Schedule(Seconds(3.1),
                        &TestDlMuMimoPhyTransmission::CheckResultsSta3,
                        this,
                        1,
                        0,
                        1024);

    Simulator::Schedule(Seconds(3.5), &TestDlMuMimoPhyTransmission::ResetResults, this);

    //----------------------------------------------------------------------------------------------------
    // Send MU PPDU with three PSDUs addressed to STA 1, STA 2 and STA 3:
    // All STAs should receive their PSDUs.
    Simulator::Schedule(Seconds(4.0),
                        &TestDlMuMimoPhyTransmission::SendMuPpdu,
                        this,
                        std::vector<StaInfo>{{1, m_nss}, {2, m_nss}, {3, m_nss}});

    // Since it takes m_expectedPpduDuration to transmit the PPDU,
    // all 3 PHYs should be back to IDLE at the same time.
    Simulator::Schedule(Seconds(4.0) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta1,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(4.0) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta2,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(4.0) + m_expectedPpduDuration - NanoSeconds(1),
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta3,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(4.0) + m_expectedPpduDuration,
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta1,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(4.0) + m_expectedPpduDuration,
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta2,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(4.0) + m_expectedPpduDuration,
                        &TestDlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phySta3,
                        WifiPhyState::IDLE);

    // One PSDU of 1008 bytes should have been successfully received by STA 1
    Simulator::Schedule(Seconds(4.1),
                        &TestDlMuMimoPhyTransmission::CheckResultsSta1,
                        this,
                        1,
                        0,
                        1008);
    // One PSDU of 1016 bytes should have been successfully received by STA 2
    Simulator::Schedule(Seconds(4.1),
                        &TestDlMuMimoPhyTransmission::CheckResultsSta2,
                        this,
                        1,
                        0,
                        1016);
    // One PSDU of 1024 bytes should have been successfully received by STA 3
    Simulator::Schedule(Seconds(4.1),
                        &TestDlMuMimoPhyTransmission::CheckResultsSta3,
                        this,
                        1,
                        0,
                        1024);

    Simulator::Schedule(Seconds(4.5), &TestDlMuMimoPhyTransmission::ResetResults, this);

    Simulator::Run();
}

void
TestDlMuMimoPhyTransmission::DoRun()
{
    std::vector<uint8_t> nssToTest{1, 2};
    for (auto nss : nssToTest)
    {
        m_nss = nss;
        m_frequency = 5180;
        m_channelWidth = 20;
        m_expectedPpduDuration = (nss > 1) ? NanoSeconds(110400) : NanoSeconds(156800);
        RunOne();

        m_frequency = 5190;
        m_channelWidth = 40;
        m_expectedPpduDuration = (nss > 1) ? NanoSeconds(83200) : NanoSeconds(102400);
        RunOne();

        m_frequency = 5210;
        m_channelWidth = 80;
        m_expectedPpduDuration = (nss > 1) ? NanoSeconds(69600) : NanoSeconds(75200);
        RunOne();

        m_frequency = 5250;
        m_channelWidth = 160;
        m_expectedPpduDuration = (nss > 1) ? NanoSeconds(69600) : NanoSeconds(61600);
        RunOne();
    }
    // FIXME: test also different nss over STAs once RX durations when receiving different PPDUs
    // with different nss over STAs are fixed

    Simulator::Destroy();
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief wifi PHY MU-MIMO Test Suite
 */
class WifiPhyMuMimoTestSuite : public TestSuite
{
  public:
    WifiPhyMuMimoTestSuite();
};

WifiPhyMuMimoTestSuite::WifiPhyMuMimoTestSuite()
    : TestSuite("wifi-phy-mu-mimo", UNIT)
{
    AddTestCase(new TestDlMuTxVector, TestCase::QUICK);
    AddTestCase(new TestDlMuMimoPhyTransmission, TestCase::QUICK);
}

static WifiPhyMuMimoTestSuite WifiPhyMuMimoTestSuite; ///< the test suite
