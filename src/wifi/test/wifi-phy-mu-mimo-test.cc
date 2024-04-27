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

#include "ns3/ap-wifi-mac.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/he-configuration.h"
#include "ns3/he-phy.h"
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
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/txop.h"
#include "ns3/uinteger.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-spectrum-value-helper.h"
#include "ns3/wifi-utils.h"

#include <list>
#include <tuple>

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

    /**
     * Set the global PPDU UID counter.
     *
     * \param uid the value to which the global PPDU UID counter should be set
     */
    void SetGlobalPpduUid(uint64_t uid);

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

void
MuMimoTestHePhy::SetGlobalPpduUid(uint64_t uid)
{
    m_globalPpduUid = uid;
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

    /**
     * Set the global PPDU UID counter.
     *
     * \param uid the value to which the global PPDU UID counter should be set
     */
    void SetPpduUid(uint64_t uid);

    /**
     * Since we assume trigger frame was previously received from AP, this is used to set its UID
     *
     * \param uid the PPDU UID of the trigger frame
     */
    void SetTriggerFrameUid(uint64_t uid);

    /**
     * \return the current event
     */
    Ptr<Event> GetCurrentEvent();

  private:
    void DoInitialize() override;
    void DoDispose() override;

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

void
MuMimoSpectrumWifiPhy::SetPpduUid(uint64_t uid)
{
    m_ofdmTestHePhy->SetGlobalPpduUid(uid);
    m_previouslyRxPpduUid = uid;
}

void
MuMimoSpectrumWifiPhy::SetTriggerFrameUid(uint64_t uid)
{
    m_previouslyRxPpduUid = uid;
}

Ptr<Event>
MuMimoSpectrumWifiPhy::GetCurrentEvent()
{
    return m_currentEvent;
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
 * \brief UL MU-MIMO PHY test
 */
class TestUlMuMimoPhyTransmission : public TestCase
{
  public:
    TestUlMuMimoPhyTransmission();

  private:
    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     * Get TXVECTOR for HE TB PPDU.
     * \param txStaId the ID of the TX STA
     * \param nss the number of spatial streams used for the transmission
     * \param bssColor the BSS color of the TX STA
     * \return the TXVECTOR for HE TB PPDU
     */
    WifiTxVector GetTxVectorForHeTbPpdu(uint16_t txStaId, uint8_t nss, uint8_t bssColor) const;
    /**
     * Set TRIGVECTOR for HE TB PPDU
     *
     * \param staIds the IDs of the STAs sollicited for the HE TB transmission
     * \param bssColor the BSS color of the TX STA
     */
    void SetTrigVector(const std::vector<uint16_t>& staIds, uint8_t bssColor);
    /**
     * Send HE TB PPDU function
     * \param txStaId the ID of the TX STA
     * \param nss the number of spatial streams used for the transmission
     * \param payloadSize the size of the payload in bytes
     * \param uid the UID of the trigger frame that is initiating this transmission
     * \param bssColor the BSS color of the TX STA
     */
    void SendHeTbPpdu(uint16_t txStaId,
                      uint8_t nss,
                      std::size_t payloadSize,
                      uint64_t uid,
                      uint8_t bssColor);

    /**
     * Send HE SU PPDU function
     * \param txStaId the ID of the TX STA
     * \param payloadSize the size of the payload in bytes
     * \param uid the UID of the trigger frame that is initiating this transmission
     * \param bssColor the BSS color of the TX STA
     */
    void SendHeSuPpdu(uint16_t txStaId, std::size_t payloadSize, uint64_t uid, uint8_t bssColor);

    /**
     * Set the BSS color
     * \param phy the PHY
     * \param bssColor the BSS color
     */
    void SetBssColor(Ptr<WifiPhy> phy, uint8_t bssColor);

    /**
     * Run one function
     */
    void RunOne();

    /**
     * Check the received PSDUs from a given STA
     * \param staId the ID of the STA to check
     * \param expectedSuccess the expected number of success
     * \param expectedFailures the expected number of failures
     * \param expectedBytes the expected number of bytes
     */
    void CheckRxFromSta(uint16_t staId,
                        uint32_t expectedSuccess,
                        uint32_t expectedFailures,
                        uint32_t expectedBytes);

    /**
     * Verify all events are cleared at end of TX or RX
     */
    void VerifyEventsCleared();

    /**
     * Check the PHY state
     * \param phy the PHY
     * \param expectedState the expected state of the PHY
     */
    void CheckPhyState(Ptr<MuMimoSpectrumWifiPhy> phy, WifiPhyState expectedState);
    /// \copydoc CheckPhyState
    void DoCheckPhyState(Ptr<MuMimoSpectrumWifiPhy> phy, WifiPhyState expectedState);

    /**
     * Reset function
     */
    void Reset();

    /**
     * Receive success function
     * \param psdu the PSDU
     * \param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * \param txVector the transmit vector
     * \param statusPerMpdu reception status per MPDU
     */
    void RxSuccess(Ptr<const WifiPsdu> psdu,
                   RxSignalInfo rxSignalInfo,
                   WifiTxVector txVector,
                   std::vector<bool> statusPerMpdu);

    /**
     * Receive failure function
     * \param psdu the PSDU
     */
    void RxFailure(Ptr<const WifiPsdu> psdu);

    /**
     * Schedule test to perform.
     * The interference generation should be scheduled apart.
     *
     * \param delay the reference delay to schedule the events
     * \param txStaIds the IDs of the STAs planned to transmit an HE TB PPDU
     * \param expectedStateAtEnd the expected state of the PHY at the end of the reception
     * \param expectedCountersPerSta the expected counters per STA
     */
    void ScheduleTest(
        Time delay,
        const std::vector<uint16_t>& txStaIds,
        WifiPhyState expectedStateAtEnd,
        const std::vector<std::tuple<uint32_t, uint32_t, uint32_t>>& expectedCountersPerSta);

    /**
     * Log scenario description
     *
     * \param log the scenario description to add to log
     */
    void LogScenario(const std::string& log) const;

    Ptr<MuMimoSpectrumWifiPhy> m_phyAp;                ///< PHY of AP
    std::vector<Ptr<MuMimoSpectrumWifiPhy>> m_phyStas; ///< PHYs of STAs

    std::vector<uint32_t> m_countRxSuccessFromStas; ///< count RX success from STAs
    std::vector<uint32_t> m_countRxFailureFromStas; ///< count RX failure from STAs
    std::vector<uint32_t> m_countRxBytesFromStas;   ///< count RX bytes from STAs

    Time m_delayStart;           ///< delay between the start of each HE TB PPDUs
    uint16_t m_frequency;        ///< frequency in MHz
    uint16_t m_channelWidth;     ///< channel width in MHz
    Time m_expectedPpduDuration; ///< expected duration to send MU PPDU
};

TestUlMuMimoPhyTransmission::TestUlMuMimoPhyTransmission()
    : TestCase("UL MU-MIMO PHY test"),
      m_countRxSuccessFromStas{},
      m_countRxFailureFromStas{},
      m_countRxBytesFromStas{},
      m_delayStart{Seconds(0)},
      m_frequency{DEFAULT_FREQUENCY},
      m_channelWidth{DEFAULT_CHANNEL_WIDTH},
      m_expectedPpduDuration{NanoSeconds(271200)}
{
}

void
TestUlMuMimoPhyTransmission::SendHeSuPpdu(uint16_t txStaId,
                                          std::size_t payloadSize,
                                          uint64_t uid,
                                          uint8_t bssColor)
{
    NS_LOG_FUNCTION(this << txStaId << payloadSize << uid << +bssColor);
    WifiConstPsduMap psdus;

    WifiTxVector txVector = WifiTxVector(HePhy::GetHeMcs7(),
                                         0,
                                         WIFI_PREAMBLE_HE_SU,
                                         800,
                                         1,
                                         1,
                                         0,
                                         m_channelWidth,
                                         false,
                                         false,
                                         false,
                                         bssColor);

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
    psdus.insert(std::make_pair(SU_STA_ID, psdu));

    Ptr<MuMimoSpectrumWifiPhy> phy = (txStaId == 0) ? m_phyAp : m_phyStas.at(txStaId - 1);
    phy->SetPpduUid(uid);
    phy->Send(psdus, txVector);
}

WifiTxVector
TestUlMuMimoPhyTransmission::GetTxVectorForHeTbPpdu(uint16_t txStaId,
                                                    uint8_t nss,
                                                    uint8_t bssColor) const
{
    WifiTxVector txVector = WifiTxVector(HePhy::GetHeMcs7(),
                                         0,
                                         WIFI_PREAMBLE_HE_TB,
                                         1600,
                                         1,
                                         nss,
                                         0,
                                         m_channelWidth,
                                         false,
                                         false,
                                         false,
                                         bssColor);

    HeRu::RuSpec ru(HeRu::GetRuType(m_channelWidth), 1, true); // full BW MU-MIMO
    txVector.SetRu(ru, txStaId);
    txVector.SetMode(HePhy::GetHeMcs7(), txStaId);
    txVector.SetNss(nss, txStaId);

    return txVector;
}

void
TestUlMuMimoPhyTransmission::SetTrigVector(const std::vector<uint16_t>& staIds, uint8_t bssColor)
{
    WifiTxVector txVector(HePhy::GetHeMcs7(),
                          0,
                          WIFI_PREAMBLE_HE_TB,
                          1600,
                          1,
                          1,
                          0,
                          m_channelWidth,
                          false,
                          false,
                          false,
                          bssColor);

    HeRu::RuSpec ru(HeRu::GetRuType(m_channelWidth), 1, true); // full BW MU-MIMO
    for (auto staId : staIds)
    {
        txVector.SetRu(ru, staId);
        txVector.SetMode(HePhy::GetHeMcs7(), staId);
        txVector.SetNss(1, staId);
    }

    uint16_t length;
    std::tie(length, m_expectedPpduDuration) =
        HePhy::ConvertHeTbPpduDurationToLSigLength(m_expectedPpduDuration,
                                                   txVector,
                                                   m_phyAp->GetPhyBand());
    txVector.SetLength(length);
    auto hePhyAp = DynamicCast<HePhy>(m_phyAp->GetPhyEntity(WIFI_MOD_CLASS_HE));
    hePhyAp->SetTrigVector(txVector, m_expectedPpduDuration);
}

void
TestUlMuMimoPhyTransmission::SendHeTbPpdu(uint16_t txStaId,
                                          uint8_t nss,
                                          std::size_t payloadSize,
                                          uint64_t uid,
                                          uint8_t bssColor)
{
    NS_LOG_FUNCTION(this << txStaId << +nss << payloadSize << uid << +bssColor);
    WifiConstPsduMap psdus;

    WifiTxVector txVector = GetTxVectorForHeTbPpdu(txStaId, nss, bssColor);
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

    Ptr<MuMimoSpectrumWifiPhy> phy = m_phyStas.at(txStaId - 1);
    Time txDuration =
        phy->CalculateTxDuration(psdu->GetSize(), txVector, phy->GetPhyBand(), txStaId);
    txVector.SetLength(
        HePhy::ConvertHeTbPpduDurationToLSigLength(txDuration, txVector, phy->GetPhyBand()).first);

    phy->SetPpduUid(uid);
    phy->Send(psdus, txVector);
}

void
TestUlMuMimoPhyTransmission::RxSuccess(Ptr<const WifiPsdu> psdu,
                                       RxSignalInfo rxSignalInfo,
                                       WifiTxVector txVector,
                                       std::vector<bool> /*statusPerMpdu*/)
{
    NS_LOG_FUNCTION(this << *psdu << psdu->GetAddr2() << RatioToDb(rxSignalInfo.snr) << txVector);
    NS_TEST_ASSERT_MSG_EQ((RatioToDb(rxSignalInfo.snr) > 0), true, "Incorrect SNR value");
    for (std::size_t index = 0; index < m_countRxSuccessFromStas.size(); ++index)
    {
        std::ostringstream addr;
        addr << "00:00:00:00:00:0" << index + 1;
        if (psdu->GetAddr2() == Mac48Address(addr.str().c_str()))
        {
            m_countRxSuccessFromStas.at(index)++;
            m_countRxBytesFromStas.at(index) += (psdu->GetSize() - 30);
            break;
        }
    }
}

void
TestUlMuMimoPhyTransmission::RxFailure(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << *psdu << psdu->GetAddr2());
    for (std::size_t index = 0; index < m_countRxFailureFromStas.size(); ++index)
    {
        std::ostringstream addr;
        addr << "00:00:00:00:00:0" << index + 1;
        if (psdu->GetAddr2() == Mac48Address(addr.str().c_str()))
        {
            m_countRxFailureFromStas.at(index)++;
            break;
        }
    }
}

void
TestUlMuMimoPhyTransmission::CheckRxFromSta(uint16_t staId,
                                            uint32_t expectedSuccess,
                                            uint32_t expectedFailures,
                                            uint32_t expectedBytes)
{
    NS_LOG_FUNCTION(this << staId << expectedSuccess << expectedFailures << expectedBytes);
    NS_TEST_ASSERT_MSG_EQ(m_countRxSuccessFromStas[staId - 1],
                          expectedSuccess,
                          "The number of successfully received packets from STA "
                              << staId << " is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxFailureFromStas[staId - 1],
                          expectedFailures,
                          "The number of unsuccessfully received packets from STA "
                              << staId << " is not correct!");
    NS_TEST_ASSERT_MSG_EQ(m_countRxBytesFromStas[staId - 1],
                          expectedBytes,
                          "The number of bytes received from STA " << staId << " is not correct!");
}

void
TestUlMuMimoPhyTransmission::VerifyEventsCleared()
{
    NS_TEST_ASSERT_MSG_EQ(m_phyAp->GetCurrentEvent(),
                          nullptr,
                          "m_currentEvent for AP was not cleared");
    std::size_t sta = 1;
    for (auto& phy : m_phyStas)
    {
        NS_TEST_ASSERT_MSG_EQ(phy->GetCurrentEvent(),
                              nullptr,
                              "m_currentEvent for STA " << sta << " was not cleared");
        sta++;
    }
}

void
TestUlMuMimoPhyTransmission::CheckPhyState(Ptr<MuMimoSpectrumWifiPhy> phy,
                                           WifiPhyState expectedState)
{
    // This is needed to make sure PHY state will be checked as the last event if a state change
    // occurred at the exact same time as the check
    Simulator::ScheduleNow(&TestUlMuMimoPhyTransmission::DoCheckPhyState, this, phy, expectedState);
}

void
TestUlMuMimoPhyTransmission::DoCheckPhyState(Ptr<MuMimoSpectrumWifiPhy> phy,
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
TestUlMuMimoPhyTransmission::Reset()
{
    for (auto& counter : m_countRxSuccessFromStas)
    {
        counter = 0;
    }
    for (auto& counter : m_countRxFailureFromStas)
    {
        counter = 0;
    }
    for (auto& counter : m_countRxBytesFromStas)
    {
        counter = 0;
    }
    for (auto& phy : m_phyStas)
    {
        phy->SetPpduUid(0);
        phy->SetTriggerFrameUid(0);
    }
    SetBssColor(m_phyAp, 0);
}

void
TestUlMuMimoPhyTransmission::SetBssColor(Ptr<WifiPhy> phy, uint8_t bssColor)
{
    Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice>(phy->GetDevice());
    Ptr<HeConfiguration> heConfiguration = device->GetHeConfiguration();
    heConfiguration->SetAttribute("BssColor", UintegerValue(bssColor));
}

void
TestUlMuMimoPhyTransmission::DoSetup()
{
    // WifiHelper::EnableLogComponents();
    // LogComponentEnable("WifiPhyMuMimoTest", LOG_LEVEL_ALL);

    Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);

    Ptr<Node> apNode = CreateObject<Node>();
    Ptr<WifiNetDevice> apDev = CreateObject<WifiNetDevice>();
    apDev->SetStandard(WIFI_STANDARD_80211ax);
    auto apMac = CreateObjectWithAttributes<ApWifiMac>(
        "Txop",
        PointerValue(CreateObjectWithAttributes<Txop>("AcIndex", StringValue("AC_BE_NQOS"))));
    apMac->SetAttribute("BeaconGeneration", BooleanValue(false));
    apDev->SetMac(apMac);
    m_phyAp = CreateObject<MuMimoSpectrumWifiPhy>(0);
    Ptr<HeConfiguration> heConfiguration = CreateObject<HeConfiguration>();
    apDev->SetHeConfiguration(heConfiguration);
    Ptr<InterferenceHelper> apInterferenceHelper = CreateObject<InterferenceHelper>();
    m_phyAp->SetInterferenceHelper(apInterferenceHelper);
    Ptr<ErrorRateModel> apErrorModel = CreateObject<NistErrorRateModel>();
    m_phyAp->SetErrorRateModel(apErrorModel);
    m_phyAp->SetDevice(apDev);
    m_phyAp->AddChannel(spectrumChannel);
    m_phyAp->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phyAp->SetReceiveOkCallback(MakeCallback(&TestUlMuMimoPhyTransmission::RxSuccess, this));
    m_phyAp->SetReceiveErrorCallback(MakeCallback(&TestUlMuMimoPhyTransmission::RxFailure, this));
    apDev->SetPhy(m_phyAp);
    apNode->AddDevice(apDev);

    for (std::size_t i = 1; i <= 4; ++i)
    {
        Ptr<Node> staNode = CreateObject<Node>();
        Ptr<WifiNetDevice> staDev = CreateObject<WifiNetDevice>();
        staDev->SetStandard(WIFI_STANDARD_80211ax);
        Ptr<MuMimoSpectrumWifiPhy> phy = CreateObject<MuMimoSpectrumWifiPhy>(i);
        staDev->SetHeConfiguration(CreateObject<HeConfiguration>());
        Ptr<InterferenceHelper> staInterferenceHelper = CreateObject<InterferenceHelper>();
        phy->SetInterferenceHelper(staInterferenceHelper);
        Ptr<ErrorRateModel> staErrorModel = CreateObject<NistErrorRateModel>();
        phy->SetErrorRateModel(staErrorModel);
        phy->SetDevice(staDev);
        phy->AddChannel(spectrumChannel);
        phy->ConfigureStandard(WIFI_STANDARD_80211ax);
        phy->SetAttribute("TxGain", DoubleValue(1.0));
        phy->SetAttribute("TxPowerStart", DoubleValue(16.0));
        phy->SetAttribute("TxPowerEnd", DoubleValue(16.0));
        phy->SetAttribute("PowerDensityLimit", DoubleValue(100.0)); // no impact by default
        phy->SetAttribute("RxGain", DoubleValue(2.0));
        staDev->SetPhy(phy);
        staNode->AddDevice(staDev);
        m_phyStas.push_back(phy);
        m_countRxSuccessFromStas.push_back(0);
        m_countRxFailureFromStas.push_back(0);
        m_countRxBytesFromStas.push_back(0);
    }
}

void
TestUlMuMimoPhyTransmission::DoTeardown()
{
    for (auto& phy : m_phyStas)
    {
        phy->Dispose();
        phy = nullptr;
    }
}

void
TestUlMuMimoPhyTransmission::LogScenario(const std::string& log) const
{
    NS_LOG_INFO(log);
}

void
TestUlMuMimoPhyTransmission::ScheduleTest(
    Time delay,
    const std::vector<uint16_t>& txStaIds,
    WifiPhyState expectedStateAtEnd,
    const std::vector<std::tuple<uint32_t, uint32_t, uint32_t>>& expectedCountersPerSta)
{
    static uint64_t uid = 0;

    // AP sends an SU packet preceding HE TB PPDUs
    Simulator::Schedule(delay - MilliSeconds(10),
                        &TestUlMuMimoPhyTransmission::SendHeSuPpdu,
                        this,
                        0,
                        50,
                        ++uid,
                        0);

    Simulator::Schedule(delay, &TestUlMuMimoPhyTransmission::SetTrigVector, this, txStaIds, 0);

    // STAs send MU UL PPDUs addressed to AP
    uint16_t payloadSize = 1000;
    std::size_t index = 0;
    for (auto txStaId : txStaIds)
    {
        Simulator::Schedule(delay + (index * m_delayStart),
                            &TestUlMuMimoPhyTransmission::SendHeTbPpdu,
                            this,
                            txStaId,
                            1,
                            payloadSize,
                            uid,
                            0);
        payloadSize++;
        index++;
    }

    // Verify it takes m_expectedPpduDuration to transmit the PPDUs
    Simulator::Schedule(delay + m_expectedPpduDuration - NanoSeconds(1),
                        &TestUlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phyAp,
                        WifiPhyState::RX);
    Simulator::Schedule(delay + m_expectedPpduDuration +
                            (m_delayStart * expectedCountersPerSta.size()),
                        &TestUlMuMimoPhyTransmission::CheckPhyState,
                        this,
                        m_phyAp,
                        expectedStateAtEnd);

    delay += MilliSeconds(100);
    // Check reception state from STAs
    uint16_t staId = 1;
    for (const auto& expectedCounters : expectedCountersPerSta)
    {
        uint16_t expectedSuccessFromSta = std::get<0>(expectedCounters);
        uint16_t expectedFailuresFromSta = std::get<1>(expectedCounters);
        uint16_t expectedBytesFromSta = std::get<2>(expectedCounters);
        Simulator::Schedule(delay + (m_delayStart * (staId - 1)),
                            &TestUlMuMimoPhyTransmission::CheckRxFromSta,
                            this,
                            staId,
                            expectedSuccessFromSta,
                            expectedFailuresFromSta,
                            expectedBytesFromSta);
        staId++;
    }

    // Verify events data have been cleared
    Simulator::Schedule(delay, &TestUlMuMimoPhyTransmission::VerifyEventsCleared, this);

    delay += MilliSeconds(100);
    Simulator::Schedule(delay, &TestUlMuMimoPhyTransmission::Reset, this);
}

void
TestUlMuMimoPhyTransmission::RunOne()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 0;
    m_phyAp->AssignStreams(streamNumber);
    for (auto& phy : m_phyStas)
    {
        phy->AssignStreams(streamNumber);
    }

    auto channelNum = std::get<0>(*WifiPhyOperatingChannel::FindFirst(0,
                                                                      m_frequency,
                                                                      m_channelWidth,
                                                                      WIFI_STANDARD_80211ax,
                                                                      WIFI_PHY_BAND_5GHZ));

    m_phyAp->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelNum, m_channelWidth, WIFI_PHY_BAND_5GHZ, 0});
    for (auto& phy : m_phyStas)
    {
        phy->SetOperatingChannel(
            WifiPhy::ChannelTuple{channelNum, m_channelWidth, WIFI_PHY_BAND_5GHZ, 0});
    }

    Time delay = Seconds(0.0);
    Simulator::Schedule(delay, &TestUlMuMimoPhyTransmission::Reset, this);
    delay += Seconds(1.0);

    //---------------------------------------------------------------------------
    // Verify that all HE TB PPDUs using full BW MU-MIMO have been corrected received
    Simulator::Schedule(delay,
                        &TestUlMuMimoPhyTransmission::LogScenario,
                        this,
                        "Reception of HE TB PPDUs using full BW MU-MIMO");
    ScheduleTest(delay,
                 {1, 2, 3},
                 WifiPhyState::IDLE,
                 {
                     std::make_tuple(1, 0, 1000), // One PSDU of 1000 bytes should have been
                                                  // successfully received from STA 1
                     std::make_tuple(1, 0, 1001), // One PSDU of 1001 bytes should have been
                                                  // successfully received from STA 2
                     std::make_tuple(1, 0, 1002)  // One PSDU of 1002 bytes should have been
                                                  // successfully received from STA 3
                 });
    delay += Seconds(1.0);

    //---------------------------------------------------------------------------
    // Send an HE SU PPDU during 400 ns window and verify that all HE TB PPDUs using full BW MU-MIMO
    // have been impacted
    Simulator::Schedule(delay,
                        &TestUlMuMimoPhyTransmission::LogScenario,
                        this,
                        "Reception of HE TB PPDUs HE TB PPDUs using full BW MU-MIMO with an HE SU "
                        "PPDU arriving during the 400 ns window");
    // One HE SU arrives at AP during the 400ns window
    Simulator::Schedule(delay + NanoSeconds(150),
                        &TestUlMuMimoPhyTransmission::SendHeSuPpdu,
                        this,
                        4,
                        1002,
                        2,
                        0);
    ScheduleTest(delay,
                 {1, 2, 3},
                 WifiPhyState::IDLE,
                 {
                     std::make_tuple(0, 1, 0), // Reception of the PSDU from STA 1 should have
                                               // failed (since interference from STA 4)
                     std::make_tuple(0, 1, 0), // Reception of the PSDU from STA 2 should have
                                               // failed (since interference from STA 4)
                     std::make_tuple(0, 1, 0) // Reception of the PSDU from STA 3 should have failed
                                              // (since interference from STA 4)
                 });
    delay += Seconds(1.0);

    //---------------------------------------------------------------------------
    // Send an HE SU PPDU during HE portion reception and verify that all HE TB PPDUs have been
    // impacted
    Simulator::Schedule(delay,
                        &TestUlMuMimoPhyTransmission::LogScenario,
                        this,
                        "Reception of HE TB PPDUs using full BW MU-MIMO with an HE SU PPDU "
                        "arriving during the HE portion");
    // One HE SU arrives at AP during the HE portion
    Simulator::Schedule(delay + MicroSeconds(40),
                        &TestUlMuMimoPhyTransmission::SendHeSuPpdu,
                        this,
                        4,
                        1002,
                        2,
                        0);
    ScheduleTest(delay,
                 {1, 2, 3},
                 WifiPhyState::CCA_BUSY,
                 {
                     std::make_tuple(0, 1, 0), // Reception of the PSDU from STA 1 should have
                                               // failed (since interference from STA 4)
                     std::make_tuple(0, 1, 0), // Reception of the PSDU from STA 2 should have
                                               // failed (since interference from STA 4)
                     std::make_tuple(0, 1, 0) // Reception of the PSDU from STA 3 should have failed
                                              // (since interference from STA 4)
                 });
    delay += Seconds(1.0);

    Simulator::Run();
}

void
TestUlMuMimoPhyTransmission::DoRun()
{
    std::vector<Time> startDelays{NanoSeconds(0), NanoSeconds(100)};

    for (const auto& delayStart : startDelays)
    {
        m_delayStart = delayStart;

        m_frequency = 5180;
        m_channelWidth = 20;
        m_expectedPpduDuration = NanoSeconds(163200);
        NS_LOG_DEBUG("Run UL MU-MIMO PHY transmission test for "
                     << m_channelWidth << " MHz with delay between each HE TB PPDUs of "
                     << m_delayStart);
        RunOne();

        m_frequency = 5190;
        m_channelWidth = 40;
        m_expectedPpduDuration = NanoSeconds(105600);
        NS_LOG_DEBUG("Run UL MU-MIMO PHY transmission test for "
                     << m_channelWidth << " MHz with delay between each HE TB PPDUs of "
                     << m_delayStart);
        RunOne();

        m_frequency = 5210;
        m_channelWidth = 80;
        m_expectedPpduDuration = NanoSeconds(76800);
        NS_LOG_DEBUG("Run UL MU-MIMO PHY transmission test for "
                     << m_channelWidth << " MHz with delay between each HE TB PPDUs of "
                     << m_delayStart);
        RunOne();

        m_frequency = 5250;
        m_channelWidth = 160;
        m_expectedPpduDuration = NanoSeconds(62400);
        NS_LOG_DEBUG("Run UL MU-MIMO PHY transmission test for "
                     << m_channelWidth << " MHz with delay between each HE TB PPDUs of "
                     << m_delayStart);
        RunOne();
    }

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
    : TestSuite("wifi-phy-mu-mimo", Type::UNIT)
{
    AddTestCase(new TestDlMuTxVector, TestCase::Duration::QUICK);
    AddTestCase(new TestDlMuMimoPhyTransmission, TestCase::Duration::QUICK);
    AddTestCase(new TestUlMuMimoPhyTransmission, TestCase::Duration::QUICK);
}

static WifiPhyMuMimoTestSuite WifiPhyMuMimoTestSuite; ///< the test suite
