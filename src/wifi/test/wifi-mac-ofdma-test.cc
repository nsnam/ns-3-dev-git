/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/config.h"
#include "ns3/he-configuration.h"
#include "ns3/he-frame-exchange-manager.h"
#include "ns3/he-phy.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/multi-user-scheduler.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/packet.h"
#include "ns3/qos-utils.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/wifi-acknowledgment.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-protection.h"
#include "ns3/wifi-psdu.h"

#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiMacOfdmaTestSuite");

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Dummy Multi User Scheduler used to test OFDMA ack sequences
 *
 * This Multi User Scheduler returns SU_TX until the simulation time reaches 1.5 seconds
 * (when all BA agreements have been established). Afterwards, it cycles through UL_MU_TX
 * (with a BSRP Trigger Frame), UL_MU_TX (with a Basic Trigger Frame) and DL_MU_TX.
 * This scheduler requires that 4 stations are associated with the AP.
 *
 */
class TestMultiUserScheduler : public MultiUserScheduler
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TestMultiUserScheduler();
    ~TestMultiUserScheduler() override;

  private:
    // Implementation of pure virtual methods of MultiUserScheduler class
    TxFormat SelectTxFormat() override;
    DlMuInfo ComputeDlMuInfo() override;
    UlMuInfo ComputeUlMuInfo() override;

    /**
     * Compute the TX vector to use for MU PPDUs.
     */
    void ComputeWifiTxVector();

    TxFormat m_txFormat;              //!< the format of next transmission
    TriggerFrameType m_ulTriggerType; //!< Trigger Frame type for UL MU
    CtrlTriggerHeader m_trigger;      //!< Trigger Frame to send
    WifiMacHeader m_triggerHdr;       //!< MAC header for Trigger Frame
    WifiTxVector m_txVector;          //!< the TX vector for MU PPDUs
    WifiTxParameters m_txParams;      //!< TX parameters
    WifiPsduMap m_psduMap;            //!< the DL MU PPDU to transmit
    WifiModulationClass m_modClass;   //!< modulation class for DL MU PPDUs and TB PPDUs
};

NS_OBJECT_ENSURE_REGISTERED(TestMultiUserScheduler);

TypeId
TestMultiUserScheduler::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::TestMultiUserScheduler")
            .SetParent<MultiUserScheduler>()
            .SetGroupName("Wifi")
            .AddConstructor<TestMultiUserScheduler>()
            .AddAttribute(
                "ModulationClass",
                "Modulation class for DL MU PPDUs and TB PPDUs.",
                EnumValue(WIFI_MOD_CLASS_HE),
                MakeEnumAccessor<WifiModulationClass>(&TestMultiUserScheduler::m_modClass),
                MakeEnumChecker(WIFI_MOD_CLASS_HE, "HE", WIFI_MOD_CLASS_EHT, "EHT"));
    return tid;
}

TestMultiUserScheduler::TestMultiUserScheduler()
    : m_txFormat(SU_TX),
      m_ulTriggerType(TriggerFrameType::BSRP_TRIGGER)
{
    NS_LOG_FUNCTION(this);
}

TestMultiUserScheduler::~TestMultiUserScheduler()
{
    NS_LOG_FUNCTION_NOARGS();
}

MultiUserScheduler::TxFormat
TestMultiUserScheduler::SelectTxFormat()
{
    NS_LOG_FUNCTION(this);

    // Do not use OFDMA if a BA agreement has not been established with all the stations
    if (Simulator::Now() < Seconds(1.5))
    {
        NS_LOG_DEBUG("Return SU_TX");
        return SU_TX;
    }

    ComputeWifiTxVector();

    if (m_txFormat == SU_TX || m_txFormat == DL_MU_TX ||
        (m_txFormat == UL_MU_TX && m_ulTriggerType == TriggerFrameType::BSRP_TRIGGER))
    {
        // try to send a Trigger Frame
        TriggerFrameType ulTriggerType =
            (m_txFormat == SU_TX || m_txFormat == DL_MU_TX ? TriggerFrameType::BSRP_TRIGGER
                                                           : TriggerFrameType::BASIC_TRIGGER);

        WifiTxVector txVector = m_txVector;
        txVector.SetPreambleType(m_modClass == WIFI_MOD_CLASS_HE ? WIFI_PREAMBLE_HE_TB
                                                                 : WIFI_PREAMBLE_EHT_TB);
        m_trigger = CtrlTriggerHeader(ulTriggerType, txVector);

        txVector.SetGuardInterval(m_trigger.GetGuardInterval());

        uint32_t ampduSize = (ulTriggerType == TriggerFrameType::BSRP_TRIGGER)
                                 ? GetMaxSizeOfQosNullAmpdu(m_trigger)
                                 : 3500; // allows aggregation of 2 MPDUs in TB PPDUs

        auto staList = m_apMac->GetStaList(SINGLE_LINK_OP_ID);
        // ignore non-HE stations
        for (auto it = staList.begin(); it != staList.end();)
        {
            it = m_apMac->GetHeSupported(it->second) ? std::next(it) : staList.erase(it);
        }

        Time duration = WifiPhy::CalculateTxDuration(ampduSize,
                                                     txVector,
                                                     m_apMac->GetWifiPhy()->GetPhyBand(),
                                                     staList.begin()->first);

        uint16_t length;
        std::tie(length, duration) = HePhy::ConvertHeTbPpduDurationToLSigLength(
            duration,
            m_trigger.GetHeTbTxVector(m_trigger.begin()->GetAid12()),
            m_apMac->GetWifiPhy()->GetPhyBand());
        m_trigger.SetUlLength(length);

        Ptr<Packet> packet = Create<Packet>();
        packet->AddHeader(m_trigger);

        m_triggerHdr = WifiMacHeader(WIFI_MAC_CTL_TRIGGER);
        m_triggerHdr.SetAddr1(Mac48Address::GetBroadcast());
        m_triggerHdr.SetAddr2(m_apMac->GetAddress());
        m_triggerHdr.SetDsNotTo();
        m_triggerHdr.SetDsNotFrom();

        auto item = Create<WifiMpdu>(packet, m_triggerHdr);

        m_txParams.Clear();
        // set the TXVECTOR used to send the Trigger Frame
        m_txParams.m_txVector =
            m_apMac->GetWifiRemoteStationManager()->GetRtsTxVector(m_triggerHdr.GetAddr1(),
                                                                   m_allowedWidth);

        if (!GetHeFem(SINGLE_LINK_OP_ID)->TryAddMpdu(item, m_txParams, m_availableTime) ||
            (m_availableTime != Time::Min() &&
             *m_txParams.m_protection->protectionTime + *m_txParams.m_txDuration // TF tx time
                     + m_apMac->GetWifiPhy()->GetSifs() + duration +
                     *m_txParams.m_acknowledgment->acknowledgmentTime >
                 m_availableTime))
        {
            NS_LOG_DEBUG("Remaining TXOP duration is not enough for BSRP TF exchange");
            return SU_TX;
        }

        m_txFormat = UL_MU_TX;
        m_ulTriggerType = ulTriggerType;
    }
    else if (m_txFormat == UL_MU_TX)
    {
        // try to send a DL MU PPDU
        m_psduMap.clear();
        auto staList = m_apMac->GetStaList(SINGLE_LINK_OP_ID);
        // ignore non-HE stations
        for (auto it = staList.cbegin(); it != staList.cend();)
        {
            it = m_apMac->GetHeSupported(it->second) ? std::next(it) : staList.erase(it);
        }
        NS_ABORT_MSG_IF(staList.size() != 4, "There must be 4 associated stations");

        /* Initialize TX params */
        m_txParams.Clear();
        m_txParams.m_txVector = m_txVector;

        for (auto& sta : staList)
        {
            Ptr<WifiMpdu> peeked;
            uint8_t tid;

            for (tid = 0; tid < 8; tid++)
            {
                peeked = m_apMac->GetQosTxop(tid)->PeekNextMpdu(SINGLE_LINK_OP_ID, tid, sta.second);
                if (peeked)
                {
                    break;
                }
            }

            if (!peeked)
            {
                NS_LOG_DEBUG("No frame to send to " << sta.second);
                continue;
            }

            Ptr<WifiMpdu> mpdu = m_apMac->GetQosTxop(tid)->GetNextMpdu(SINGLE_LINK_OP_ID,
                                                                       peeked,
                                                                       m_txParams,
                                                                       m_availableTime,
                                                                       m_initialFrame);
            if (!mpdu)
            {
                NS_LOG_DEBUG("Not enough time to send frames to all the stations");
                return SU_TX;
            }

            std::vector<Ptr<WifiMpdu>> mpduList;
            mpduList = GetHeFem(SINGLE_LINK_OP_ID)
                           ->GetMpduAggregator()
                           ->GetNextAmpdu(mpdu, m_txParams, m_availableTime);

            if (mpduList.size() > 1)
            {
                m_psduMap[sta.first] = Create<WifiPsdu>(std::move(mpduList));
            }
            else
            {
                m_psduMap[sta.first] = Create<WifiPsdu>(mpdu, true);
            }
        }

        if (m_psduMap.empty())
        {
            NS_LOG_DEBUG("No frame to send");
            return SU_TX;
        }

        m_txFormat = DL_MU_TX;
    }
    else
    {
        NS_ABORT_MSG("Cannot get here.");
    }

    NS_LOG_DEBUG("Return " << m_txFormat);
    return m_txFormat;
}

void
TestMultiUserScheduler::ComputeWifiTxVector()
{
    if (m_txVector.IsDlMu())
    {
        // the TX vector has been already computed
        return;
    }

    const auto bw = m_apMac->GetWifiPhy()->GetChannelWidth();

    m_txVector.SetPreambleType(m_modClass == WIFI_MOD_CLASS_HE ? WIFI_PREAMBLE_HE_MU
                                                               : WIFI_PREAMBLE_EHT_MU);
    if (IsEht(m_txVector.GetPreambleType()))
    {
        m_txVector.SetEhtPpduType(0);
    }
    m_txVector.SetChannelWidth(bw);
    m_txVector.SetGuardInterval(m_apMac->GetHeConfiguration()->GetGuardInterval());
    m_txVector.SetTxPowerLevel(
        GetWifiRemoteStationManager(SINGLE_LINK_OP_ID)->GetDefaultTxPowerLevel());

    auto staList = m_apMac->GetStaList(SINGLE_LINK_OP_ID);
    // ignore non-HE stations
    for (auto it = staList.cbegin(); it != staList.cend();)
    {
        it = m_apMac->GetHeSupported(it->second) ? std::next(it) : staList.erase(it);
    }
    NS_ABORT_MSG_IF(staList.size() != 4, "There must be 4 associated stations");

    HeRu::RuType ruType;
    switch (static_cast<uint16_t>(bw))
    {
    case 20:
        ruType = HeRu::RU_52_TONE;
        m_txVector.SetRuAllocation({112}, 0);
        break;
    case 40:
        ruType = HeRu::RU_106_TONE;
        m_txVector.SetRuAllocation({96, 96}, 0);
        break;
    case 80:
        ruType = HeRu::RU_242_TONE;
        m_txVector.SetRuAllocation({192, 192, 192, 192}, 0);
        break;
    case 160:
        ruType = HeRu::RU_484_TONE;
        m_txVector.SetRuAllocation({200, 200, 200, 200, 200, 200, 200, 200}, 0);
        break;
    default:
        NS_ABORT_MSG("Unsupported channel width");
    }

    bool primary80 = true;
    std::size_t ruIndex = 1;

    for (auto& sta : staList)
    {
        if (bw == MHz_u{160} && ruIndex == 3)
        {
            ruIndex = 1;
            primary80 = false;
        }
        m_txVector.SetHeMuUserInfo(sta.first, {{ruType, ruIndex++, primary80}, 11, 1});
    }
    m_txVector.SetSigBMode(VhtPhy::GetVhtMcs5());
}

MultiUserScheduler::DlMuInfo
TestMultiUserScheduler::ComputeDlMuInfo()
{
    NS_LOG_FUNCTION(this);
    return DlMuInfo{m_psduMap, std::move(m_txParams)};
}

MultiUserScheduler::UlMuInfo
TestMultiUserScheduler::ComputeUlMuInfo()
{
    NS_LOG_FUNCTION(this);
    return UlMuInfo{m_trigger, m_triggerHdr, std::move(m_txParams)};
}

/**
 * @ingroup wifi-test
 * The scenarios
 */
enum class WifiOfdmaScenario : uint8_t
{
    HE = 0, // HE AP and HE non-AP STAs
    HE_EHT, // EHT AP, some EHT non-AP STAs and some non-EHT HE non-AP STAs
    EHT     // EHT AP and EHT non-AP STAs
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test OFDMA acknowledgment sequences
 *
 * Run this test with:
 *
 * NS_LOG="WifiMacOfdmaTestSuite=info|prefix_time|prefix_node" ./ns3 run "test-runner
 * --suite=wifi-mac-ofdma"
 *
 * to print the list of transmitted frames only, along with the TX time and the
 * node prefix. Replace 'info' with 'debug' if you want to print the debug messages
 * from the test multi-user scheduler only. Replace 'info' with 'level_debug' if
 * you want to print both the transmitted frames and the debug messages.
 */
class OfdmaAckSequenceTest : public TestCase
{
  public:
    /**
     * MU EDCA Parameter Set
     */
    struct MuEdcaParameterSet
    {
        uint8_t muAifsn;  //!< MU AIFS (0 to disable EDCA)
        uint16_t muCwMin; //!< MU CW min
        uint16_t muCwMax; //!< MU CW max
        uint8_t muTimer;  //!< MU EDCA Timer in units of 8192 microseconds (0 not to use MU EDCA)
    };

    /**
     * Parameters for the OFDMA acknowledgment sequences test
     */
    struct Params
    {
        MHz_u channelWidth;                     ///< PHY channel bandwidth
        WifiAcknowledgment::Method dlMuAckType; ///< DL MU ack sequence type
        uint32_t maxAmpduSize;                  ///< maximum A-MPDU size in bytes
        uint16_t txopLimit;                     ///< TXOP limit in microseconds
        bool continueTxopAfterBsrp; ///< whether to continue TXOP after BSRP TF when TXOP limit is 0
        bool skipMuRtsBeforeBsrp;   ///< whether to skip MU-RTS before BSRP TF
        bool protectedIfResponded; ///< A STA is considered protected if responded to previous frame
        uint16_t nPktsPerSta;      ///< number of packets to send to each station
        MuEdcaParameterSet muEdcaParameterSet; ///< MU EDCA Parameter Set
        WifiOfdmaScenario scenario;            ///< OFDMA scenario to test
    };

    /**
     * Constructor
     *
     * @param params parameters for the OFDMA acknowledgment sequences test
     */
    OfdmaAckSequenceTest(const Params& params);
    ~OfdmaAckSequenceTest() override;

    /**
     * Function to trace packets received by the server application
     * @param context the context
     * @param p the packet
     * @param addr the address
     */
    void L7Receive(std::string context, Ptr<const Packet> p, const Address& addr);
    /**
     * Function to trace CW value used by the given station after the MU exchange
     * @param staIndex the index of the given station
     * @param cw the current Contention Window value
     */
    void TraceCw(uint32_t staIndex, uint32_t cw, uint8_t /* linkId */);
    /**
     * Callback invoked when FrameExchangeManager passes PSDUs to the PHY
     * @param context the context
     * @param psduMap the PSDU map
     * @param txVector the TX vector
     * @param txPowerW the tx power in Watts
     */
    void Transmit(std::string context,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW);
    /**
     * Check correctness of transmitted frames
     * @param sifs the SIFS duration
     * @param slotTime a slot duration
     * @param aifsn the AIFSN
     */
    void CheckResults(Time sifs, Time slotTime, uint8_t aifsn);

  private:
    void DoRun() override;

    static constexpr uint16_t m_muTimerRes = 8192; ///< MU timer resolution in usec

    /// Information about transmitted frames
    struct FrameInfo
    {
        Time startTx;             ///< start TX time
        Time endTx;               ///< end TX time
        WifiConstPsduMap psduMap; ///< transmitted PSDU map
        WifiTxVector txVector;    ///< TXVECTOR
    };

    uint16_t m_nStations;                       ///< number of stations
    NetDeviceContainer m_staDevices;            ///< stations' devices
    Ptr<WifiNetDevice> m_apDevice;              ///< AP's device
    std::vector<PacketSocketAddress> m_sockets; ///< packet socket addresses for STAs
    MHz_u m_channelWidth;                       ///< PHY channel bandwidth
    uint8_t m_muRtsRuAllocation;                ///< B7-B1 of RU Allocation subfield of MU-RTS
    std::vector<FrameInfo> m_txPsdus;           ///< transmitted PSDUs
    WifiAcknowledgment::Method m_dlMuAckType;   ///< DL MU ack sequence type
    uint32_t m_maxAmpduSize;                    ///< maximum A-MPDU size in bytes
    uint16_t m_txopLimit;                       ///< TXOP limit in microseconds
    bool
        m_continueTxopAfterBsrp; ///< whether to continue TXOP after BSRP TF when TXOP limit is zero
    bool m_skipMuRtsBeforeBsrp;  ///< whether to skip MU-RTS before BSRP TF
    bool m_protectedIfResponded; ///< A STA is considered protected if responded to previous frame
    uint16_t m_nPktsPerSta;      ///< number of packets to send to each station
    MuEdcaParameterSet m_muEdcaParameterSet; ///< MU EDCA Parameter Set
    WifiOfdmaScenario m_scenario;            ///< OFDMA scenario to test
    WifiPreamble m_dlMuPreamble;             ///< expected preamble type for DL MU PPDUs
    WifiPreamble m_tbPreamble;               ///< expected preamble type for TB PPDUs
    bool m_ulPktsGenerated;             ///< whether UL packets for HE TB PPDUs have been generated
    uint16_t m_received;                ///< number of packets received by the stations
    uint16_t m_flushed;                 ///< number of DL packets flushed after DL MU PPDU
    Time m_edcaDisabledStartTime;       ///< time when disabling EDCA started
    std::vector<uint32_t> m_cwValues;   ///< CW used by stations after MU exchange
    const Time m_defaultTbPpduDuration; ///< default TB PPDU duration
};

OfdmaAckSequenceTest::OfdmaAckSequenceTest(const Params& params)
    : TestCase("Check correct operation of DL OFDMA acknowledgment sequences"),
      m_nStations(4),
      m_sockets(m_nStations),
      m_channelWidth(params.channelWidth),
      m_dlMuAckType(params.dlMuAckType),
      m_maxAmpduSize(params.maxAmpduSize),
      m_txopLimit(params.txopLimit),
      m_continueTxopAfterBsrp(params.continueTxopAfterBsrp),
      m_skipMuRtsBeforeBsrp(params.skipMuRtsBeforeBsrp),
      m_protectedIfResponded(params.protectedIfResponded),
      m_nPktsPerSta(params.nPktsPerSta),
      m_muEdcaParameterSet(params.muEdcaParameterSet),
      m_scenario(params.scenario),
      m_ulPktsGenerated(false),
      m_received(0),
      m_flushed(0),
      m_edcaDisabledStartTime(),
      m_cwValues(std::vector<uint32_t>(m_nStations, 2)), // 2 is an invalid CW value
      m_defaultTbPpduDuration(MilliSeconds(2))
{
    switch (m_scenario)
    {
    case WifiOfdmaScenario::HE:
    case WifiOfdmaScenario::HE_EHT:
        m_dlMuPreamble = WIFI_PREAMBLE_HE_MU;
        m_tbPreamble = WIFI_PREAMBLE_HE_TB;
        break;
    case WifiOfdmaScenario::EHT:
        m_dlMuPreamble = WIFI_PREAMBLE_EHT_MU;
        m_tbPreamble = WIFI_PREAMBLE_EHT_TB;
        break;
    }

    switch (static_cast<uint16_t>(m_channelWidth))
    {
    case 20:
        m_muRtsRuAllocation = 61; // p20 index is 0
        break;
    case 40:
        m_muRtsRuAllocation = 65; // p20 index is 0
        break;
    case 80:
        m_muRtsRuAllocation = 67;
        break;
    case 160:
        m_muRtsRuAllocation = 68;
        break;
    default:
        NS_ABORT_MSG("Unhandled channel width (" << m_channelWidth << " MHz)");
    }

    m_txPsdus.reserve(35);
}

OfdmaAckSequenceTest::~OfdmaAckSequenceTest()
{
}

void
OfdmaAckSequenceTest::L7Receive(std::string context, Ptr<const Packet> p, const Address& addr)
{
    if (p->GetSize() >= 1400 && Simulator::Now() > Seconds(1.5))
    {
        m_received++;
    }
}

void
OfdmaAckSequenceTest::TraceCw(uint32_t staIndex, uint32_t cw, uint8_t /* linkId */)
{
    if (m_cwValues.at(staIndex) == 2)
    {
        // store the first CW used after MU exchange (the last one may be used after
        // the MU EDCA timer expired)
        m_cwValues[staIndex] = cw;
    }
}

void
OfdmaAckSequenceTest::Transmit(std::string context,
                               WifiConstPsduMap psduMap,
                               WifiTxVector txVector,
                               double txPowerW)
{
    // skip beacon frames and frames transmitted before 1.5s (association
    // request/response, ADDBA request, ...)
    if (!psduMap.begin()->second->GetHeader(0).IsBeacon() && Simulator::Now() >= Seconds(1.5))
    {
        Time txDuration = WifiPhy::CalculateTxDuration(psduMap, txVector, WIFI_PHY_BAND_5GHZ);
        m_txPsdus.push_back({Simulator::Now(), Simulator::Now() + txDuration, psduMap, txVector});

        for (const auto& [staId, psdu] : psduMap)
        {
            NS_LOG_INFO("Sending "
                        << psdu->GetHeader(0).GetTypeString() << " #MPDUs " << psdu->GetNMpdus()
                        << (psdu->GetHeader(0).IsQosData()
                                ? " TID " + std::to_string(*psdu->GetTids().begin())
                                : "")
                        << std::setprecision(10) << " txDuration " << txDuration << " duration/ID "
                        << psdu->GetHeader(0).GetDuration() << " #TX PSDUs = " << m_txPsdus.size()
                        << " size=" << (*psdu->begin())->GetSize() << "\n"
                        << "TXVECTOR: " << txVector << "\n");
        }
    }

    // Flush the MAC queue of the AP after sending a DL MU PPDU (no need for
    // further transmissions)
    if (txVector.GetPreambleType() == m_dlMuPreamble)
    {
        m_flushed = 0;
        for (uint32_t i = 0; i < m_staDevices.GetN(); i++)
        {
            auto queue =
                m_apDevice->GetMac()->GetQosTxop(static_cast<AcIndex>(i))->GetWifiMacQueue();
            auto staDev = DynamicCast<WifiNetDevice>(m_staDevices.Get(i));
            Ptr<const WifiMpdu> lastInFlight = nullptr;
            Ptr<const WifiMpdu> mpdu;

            while ((mpdu = queue->PeekByTidAndAddress(i * 2,
                                                      staDev->GetMac()->GetAddress(),
                                                      lastInFlight)) != nullptr)
            {
                if (mpdu->IsInFlight())
                {
                    lastInFlight = mpdu;
                }
                else
                {
                    queue->Remove(mpdu);
                    m_flushed++;
                }
            }
        }
    }
    else if (txVector.GetPreambleType() == m_tbPreamble &&
             psduMap.begin()->second->GetHeader(0).HasData())
    {
        Mac48Address sender = psduMap.begin()->second->GetAddr2();

        for (uint32_t i = 0; i < m_staDevices.GetN(); i++)
        {
            auto dev = DynamicCast<WifiNetDevice>(m_staDevices.Get(i));

            if (dev->GetAddress() == sender)
            {
                Ptr<QosTxop> qosTxop = dev->GetMac()->GetQosTxop(static_cast<AcIndex>(i));

                if (m_muEdcaParameterSet.muTimer > 0 && m_muEdcaParameterSet.muAifsn > 0)
                {
                    // stations use worse access parameters, trace CW. MU AIFSN must be large
                    // enough to avoid collisions between stations trying to transmit using EDCA
                    // right after the UL MU transmission and the AP trying to send a DL MU PPDU
                    qosTxop->TraceConnectWithoutContext(
                        "CwTrace",
                        MakeCallback(&OfdmaAckSequenceTest::TraceCw, this).Bind(i));
                }
                else
                {
                    // there is no "protection" against collisions from stations, hence flush
                    // their MAC queues after sending an HE TB PPDU containing QoS data frames,
                    // so that the AP can send a DL MU PPDU
                    qosTxop->GetWifiMacQueue()->Flush();
                }
                break;
            }
        }
    }
    else if (!txVector.IsMu() && psduMap.begin()->second->GetHeader(0).IsBlockAck() &&
             psduMap.begin()->second->GetHeader(0).GetAddr2() == m_apDevice->GetAddress() &&
             m_muEdcaParameterSet.muTimer > 0 && m_muEdcaParameterSet.muAifsn == 0)
    {
        CtrlBAckResponseHeader blockAck;
        psduMap.begin()->second->GetPayload(0)->PeekHeader(blockAck);

        if (blockAck.IsMultiSta())
        {
            // AP is transmitting a multi-STA BlockAck and stations have to disable EDCA,
            // record the starting time
            m_edcaDisabledStartTime =
                Simulator::Now() + m_txPsdus.back().endTx - m_txPsdus.back().startTx;
        }
    }
    else if (!txVector.IsMu() && psduMap.begin()->second->GetHeader(0).IsTrigger() &&
             !m_ulPktsGenerated)
    {
        CtrlTriggerHeader trigger;
        psduMap.begin()->second->GetPayload(0)->PeekHeader(trigger);
        if (trigger.IsBasic())
        {
            // the AP is starting the transmission of the Basic Trigger frame, so generate
            // the configured number of packets at STAs, which are sent in HE TB PPDUs
            Time txDuration = WifiPhy::CalculateTxDuration(psduMap, txVector, WIFI_PHY_BAND_5GHZ);
            for (uint16_t i = 0; i < m_nStations; i++)
            {
                Ptr<PacketSocketClient> client = CreateObject<PacketSocketClient>();
                client->SetAttribute("PacketSize", UintegerValue(1400 + i * 100));
                client->SetAttribute("MaxPackets", UintegerValue(m_nPktsPerSta));
                client->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
                client->SetAttribute("Priority", UintegerValue(i * 2)); // 0, 2, 4 and 6
                client->SetRemote(m_sockets[i]);
                m_staDevices.Get(i)->GetNode()->AddApplication(client);
                client->SetStartTime(txDuration); // start when TX ends
                client->SetStopTime(Seconds(1));  // stop in a second
                client->Initialize();
            }
            m_ulPktsGenerated = true;
        }
    }
}

void
OfdmaAckSequenceTest::CheckResults(Time sifs, Time slotTime, uint8_t aifsn)
{
    CtrlTriggerHeader trigger;
    CtrlBAckResponseHeader blockAck;
    Time tEnd;                         // TX end for a frame
    Time tStart;                       // TX start for the next frame
    Time tolerance = NanoSeconds(500); // due to propagation delay
    Time ifs = (m_txopLimit > 0 ? sifs : sifs + aifsn * slotTime);
    Time navEnd;

    /*
     *        |-------------NAV----------->|         |-----------------NAV------------------->|
     *                 |---------NAV------>|                  |--------------NAV------------->|
     *                           |---NAV-->|                             |--------NAV-------->|
     *    ┌───┐    ┌───┐    ┌────┐    ┌────┐     ┌───┐    ┌───┐    ┌─────┐    ┌────┐    ┌─────┐
     *    │   │    │   │    │    │    │QoS │     │   │    │   │    │     │    │QoS │    │     │
     *    │   │    │   │    │    │    │Null│     │   │    │   │    │     │    │Data│    │     │
     *    │   │    │   │    │    │    ├────┤     │   │    │   │    │     │    ├────┤    │     │
     *    │   │    │   │    │    │    │QoS │     │   │    │   │    │     │    │QoS │    │Multi│
     *    │MU-│    │CTS│    │BSRP│    │Null│     │MU-│    │CTS│    │Basic│    │Data│    │-STA │
     *    │RTS│SIFS│   │SIFS│ TF │SIFS├────┤<IFS>│RTS│SIFS│   │SIFS│ TF  │SIFS├────┤SIFS│Block│
     *    │TF │    │x4 │    │    │    │QoS │     │TF │    │x4 │    │     │    │QoS │    │ Ack │
     *    │   │    │   │    │    │    │Null│     │   │    │   │    │     │    │Data│    │     │
     *    │   │    │   │    │    │    ├────┤     │   │    │   │    │     │    ├────┤    │     │
     *    │   │    │   │    │    │    │QoS │     │   │    │   │    │     │    │QoS │    │     │
     *    │   │    │   │    │    │    │Null│     │   │    │   │    │     │    │Data│    │     │
     * ───┴───┴────┴───┴────┴────┴────┴────┴─────┴───┴────┴───┴────┴─────┴────┴────┴────┴─────┴──
     * From: AP     all       AP        all       AP       all       AP         all       AP
     *   To: all    AP        all       AP        all      AP        all        AP        all
     *
     * NOTE 1:The first MU-RTS is not transmitted if SkipMuRtsBeforeBsrp is true
     * NOTE 2: The second MU-RTS is transmitted if the Trigger Frames are transmitted in separate
     *         TXOPs, or it is a single TXOP and an MU-RTS has not been sent earlier (to protect
     *         the BSRP TF) and STAs are not considered protected if they responded
     */

    tEnd = m_txPsdus[0].endTx;
    navEnd = tEnd + m_txPsdus[0].psduMap[SU_STA_ID]->GetDuration();
    Time ctsNavEnd{0};

    if (!m_skipMuRtsBeforeBsrp)
    {
        // the first packet sent after 1.5s is an MU-RTS Trigger Frame
        NS_TEST_ASSERT_MSG_GT_OR_EQ(m_txPsdus.size(), 5, "Expected at least 5 transmitted packet");
        NS_TEST_EXPECT_MSG_EQ(
            (m_txPsdus[0].psduMap.size() == 1 &&
             m_txPsdus[0].psduMap[SU_STA_ID]->GetHeader(0).IsTrigger() &&
             m_txPsdus[0].psduMap[SU_STA_ID]->GetHeader(0).GetAddr1().IsBroadcast()),
            true,
            "Expected a Trigger Frame");
        m_txPsdus[0].psduMap[SU_STA_ID]->GetPayload(0)->PeekHeader(trigger);
        NS_TEST_EXPECT_MSG_EQ(trigger.IsMuRts(), true, "Expected an MU-RTS Trigger Frame");
        NS_TEST_EXPECT_MSG_EQ(trigger.GetNUserInfoFields(),
                              4,
                              "Expected one User Info field per station");
        NS_TEST_EXPECT_MSG_EQ(m_txPsdus[0].txVector.GetChannelWidth(),
                              m_channelWidth,
                              "Expected the MU-RTS to occupy the entire channel width");
        for (const auto& userInfo : trigger)
        {
            NS_TEST_EXPECT_MSG_EQ(+userInfo.GetMuRtsRuAllocation(),
                                  +m_muRtsRuAllocation,
                                  "Unexpected RU Allocation value in MU-RTS");
        }

        // A first STA sends a CTS frame a SIFS after the reception of the MU-RTS TF
        NS_TEST_EXPECT_MSG_EQ(
            (m_txPsdus[1].txVector.GetPreambleType() != WIFI_PREAMBLE_HE_TB &&
             m_txPsdus[1].psduMap.size() == 1 &&
             m_txPsdus[1].psduMap.begin()->second->GetNMpdus() == 1 &&
             m_txPsdus[1].psduMap.begin()->second->GetHeader(0).GetType() == WIFI_MAC_CTL_CTS),
            true,
            "Expected a CTS frame");
        NS_TEST_EXPECT_MSG_EQ(m_txPsdus[1].txVector.GetChannelWidth(),
                              m_channelWidth,
                              "Expected the CTS to occupy the entire channel width");

        tStart = m_txPsdus[1].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "CTS frame sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart, tEnd + sifs + tolerance, "CTS frame sent too late");
        ctsNavEnd = m_txPsdus[1].endTx + m_txPsdus[1].psduMap[SU_STA_ID]->GetDuration();
        // navEnd <= ctsNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, ctsNavEnd, "Duration/ID in CTS frame is too short");
        NS_TEST_EXPECT_MSG_LT(ctsNavEnd,
                              navEnd + tolerance,
                              "Duration/ID in CTS frame is too long");

        // A second STA sends a CTS frame a SIFS after the reception of the MU-RTS TF
        NS_TEST_EXPECT_MSG_EQ(
            (m_txPsdus[2].txVector.GetPreambleType() != WIFI_PREAMBLE_HE_TB &&
             m_txPsdus[2].psduMap.size() == 1 &&
             m_txPsdus[2].psduMap.begin()->second->GetNMpdus() == 1 &&
             m_txPsdus[2].psduMap.begin()->second->GetHeader(0).GetType() == WIFI_MAC_CTL_CTS),
            true,
            "Expected a CTS frame");
        NS_TEST_EXPECT_MSG_EQ(m_txPsdus[2].txVector.GetChannelWidth(),
                              m_channelWidth,
                              "Expected the CTS to occupy the entire channel width");

        tStart = m_txPsdus[2].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "CTS frame sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart, tEnd + sifs + tolerance, "CTS frame sent too late");
        ctsNavEnd = m_txPsdus[2].endTx + m_txPsdus[2].psduMap[SU_STA_ID]->GetDuration();
        // navEnd <= ctsNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, ctsNavEnd, "Duration/ID in CTS frame is too short");
        NS_TEST_EXPECT_MSG_LT(ctsNavEnd,
                              navEnd + tolerance,
                              "Duration/ID in CTS frame is too long");

        // A third STA sends a CTS frame a SIFS after the reception of the MU-RTS TF
        NS_TEST_EXPECT_MSG_EQ(
            (m_txPsdus[3].txVector.GetPreambleType() != WIFI_PREAMBLE_HE_TB &&
             m_txPsdus[3].psduMap.size() == 1 &&
             m_txPsdus[3].psduMap.begin()->second->GetNMpdus() == 1 &&
             m_txPsdus[3].psduMap.begin()->second->GetHeader(0).GetType() == WIFI_MAC_CTL_CTS),
            true,
            "Expected a CTS frame");
        NS_TEST_EXPECT_MSG_EQ(m_txPsdus[3].txVector.GetChannelWidth(),
                              m_channelWidth,
                              "Expected the CTS to occupy the entire channel width");

        tStart = m_txPsdus[3].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "CTS frame sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart, tEnd + sifs + tolerance, "CTS frame sent too late");
        ctsNavEnd = m_txPsdus[3].endTx + m_txPsdus[3].psduMap[SU_STA_ID]->GetDuration();
        // navEnd <= ctsNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, ctsNavEnd, "Duration/ID in CTS frame is too short");
        NS_TEST_EXPECT_MSG_LT(ctsNavEnd,
                              navEnd + tolerance,
                              "Duration/ID in CTS frame is too long");

        // A fourth STA sends a CTS frame a SIFS after the reception of the MU-RTS TF
        NS_TEST_EXPECT_MSG_EQ(
            (m_txPsdus[4].txVector.GetPreambleType() != WIFI_PREAMBLE_HE_TB &&
             m_txPsdus[4].psduMap.size() == 1 &&
             m_txPsdus[4].psduMap.begin()->second->GetNMpdus() == 1 &&
             m_txPsdus[4].psduMap.begin()->second->GetHeader(0).GetType() == WIFI_MAC_CTL_CTS),
            true,
            "Expected a CTS frame");
        NS_TEST_EXPECT_MSG_EQ(m_txPsdus[4].txVector.GetChannelWidth(),
                              m_channelWidth,
                              "Expected the CTS to occupy the entire channel width");

        tStart = m_txPsdus[4].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "CTS frame sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart, tEnd + sifs + tolerance, "CTS frame sent too late");
        ctsNavEnd = m_txPsdus[4].endTx + m_txPsdus[4].psduMap[SU_STA_ID]->GetDuration();
        // navEnd <= ctsNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, ctsNavEnd, "Duration/ID in CTS frame is too short");
        NS_TEST_EXPECT_MSG_LT(ctsNavEnd,
                              navEnd + tolerance,
                              "Duration/ID in CTS frame is too long");
    }
    else
    {
        // insert 5 elements in m_txPsdus to align the index of the following frames in the
        // two cases (m_skipMuRtsBeforeBsrp true and false)
        m_txPsdus.insert(m_txPsdus.begin(), 5, {});
    }

    // the AP sends a BSRP Trigger Frame
    NS_TEST_ASSERT_MSG_GT_OR_EQ(m_txPsdus.size(), 10, "Expected at least 10 transmitted packet");
    NS_TEST_EXPECT_MSG_EQ((m_txPsdus[5].psduMap.size() == 1 &&
                           m_txPsdus[5].psduMap[SU_STA_ID]->GetHeader(0).IsTrigger() &&
                           m_txPsdus[5].psduMap[SU_STA_ID]->GetHeader(0).GetAddr1().IsBroadcast()),
                          true,
                          "Expected a Trigger Frame");
    m_txPsdus[5].psduMap[SU_STA_ID]->GetPayload(0)->PeekHeader(trigger);
    NS_TEST_EXPECT_MSG_EQ(trigger.IsBsrp(), true, "Expected a BSRP Trigger Frame");
    NS_TEST_EXPECT_MSG_EQ(trigger.GetNUserInfoFields(),
                          4,
                          "Expected one User Info field per station");
    if (!m_skipMuRtsBeforeBsrp)
    {
        tEnd = m_txPsdus[4].endTx;
        tStart = m_txPsdus[5].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "BSRP Trigger Frame sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart, tEnd + sifs + tolerance, "BSRP Trigger Frame sent too late");
    }
    Time bsrpNavEnd = m_txPsdus[5].endTx + m_txPsdus[5].psduMap[SU_STA_ID]->GetDuration();
    if (m_continueTxopAfterBsrp && m_txopLimit == 0)
    {
        // BSRP TF extends the NAV beyond the responses
        navEnd += m_defaultTbPpduDuration;
    }
    // navEnd <= bsrpNavEnd < navEnd + tolerance
    NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, bsrpNavEnd, "Duration/ID in BSRP TF is too short");
    NS_TEST_EXPECT_MSG_LT(bsrpNavEnd, navEnd + tolerance, "Duration/ID in BSRP TF is too long");

    // A first STA sends a QoS Null frame in a TB PPDU a SIFS after the reception of the BSRP TF
    NS_TEST_EXPECT_MSG_EQ((m_txPsdus[6].txVector.GetPreambleType() == m_tbPreamble &&
                           m_txPsdus[6].psduMap.size() == 1 &&
                           m_txPsdus[6].psduMap.begin()->second->GetNMpdus() == 1),
                          true,
                          "Expected a QoS Null frame in a TB PPDU");
    {
        const WifiMacHeader& hdr = m_txPsdus[6].psduMap.begin()->second->GetHeader(0);
        NS_TEST_EXPECT_MSG_EQ(hdr.GetType(), WIFI_MAC_QOSDATA_NULL, "Expected a QoS Null frame");
        uint16_t staId;
        for (staId = 0; staId < m_nStations; staId++)
        {
            if (DynamicCast<WifiNetDevice>(m_staDevices.Get(staId))->GetAddress() == hdr.GetAddr2())
            {
                break;
            }
        }
        NS_TEST_EXPECT_MSG_NE(+staId, m_nStations, "Sender not found among stations");
        uint8_t tid = staId * 2;
        NS_TEST_EXPECT_MSG_EQ(+hdr.GetQosTid(), +tid, "Expected a TID equal to " << +tid);
    }
    tEnd = m_txPsdus[5].endTx;
    tStart = m_txPsdus[6].startTx;
    NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "QoS Null frame in HE TB PPDU sent too early");
    NS_TEST_EXPECT_MSG_LT(tStart,
                          tEnd + sifs + tolerance,
                          "QoS Null frame in HE TB PPDU sent too late");
    Time qosNullNavEnd = m_txPsdus[6].endTx + m_txPsdus[6].psduMap.begin()->second->GetDuration();
    if (m_txopLimit == 0)
    {
        NS_TEST_EXPECT_MSG_EQ(qosNullNavEnd,
                              m_txPsdus[6].endTx +
                                  (m_continueTxopAfterBsrp ? m_defaultTbPpduDuration : Time{0}),
                              "Expected null Duration/ID for QoS Null frame in HE TB PPDU");
    }
    // navEnd <= qosNullNavEnd < navEnd + tolerance
    NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, qosNullNavEnd, "Duration/ID in QoS Null is too short");
    NS_TEST_EXPECT_MSG_LT(qosNullNavEnd, navEnd + tolerance, "Duration/ID in QoS Null is too long");

    // A second STA sends a QoS Null frame in a TB PPDU a SIFS after the reception of the BSRP TF
    NS_TEST_EXPECT_MSG_EQ((m_txPsdus[7].txVector.GetPreambleType() == m_tbPreamble &&
                           m_txPsdus[7].psduMap.size() == 1 &&
                           m_txPsdus[7].psduMap.begin()->second->GetNMpdus() == 1),
                          true,
                          "Expected a QoS Null frame in a TB PPDU");
    {
        const WifiMacHeader& hdr = m_txPsdus[7].psduMap.begin()->second->GetHeader(0);
        NS_TEST_EXPECT_MSG_EQ(hdr.GetType(), WIFI_MAC_QOSDATA_NULL, "Expected a QoS Null frame");
        uint16_t staId;
        for (staId = 0; staId < m_nStations; staId++)
        {
            if (DynamicCast<WifiNetDevice>(m_staDevices.Get(staId))->GetAddress() == hdr.GetAddr2())
            {
                break;
            }
        }
        NS_TEST_EXPECT_MSG_NE(+staId, m_nStations, "Sender not found among stations");
        uint8_t tid = staId * 2;
        NS_TEST_EXPECT_MSG_EQ(+hdr.GetQosTid(), +tid, "Expected a TID equal to " << +tid);
    }
    tStart = m_txPsdus[7].startTx;
    NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "QoS Null frame in HE TB PPDU sent too early");
    NS_TEST_EXPECT_MSG_LT(tStart,
                          tEnd + sifs + tolerance,
                          "QoS Null frame in HE TB PPDU sent too late");
    qosNullNavEnd = m_txPsdus[7].endTx + m_txPsdus[7].psduMap.begin()->second->GetDuration();
    if (m_txopLimit == 0)
    {
        NS_TEST_EXPECT_MSG_EQ(qosNullNavEnd,
                              m_txPsdus[7].endTx +
                                  (m_continueTxopAfterBsrp ? m_defaultTbPpduDuration : Time{0}),
                              "Expected null Duration/ID for QoS Null frame in HE TB PPDU");
    }
    // navEnd <= qosNullNavEnd < navEnd + tolerance
    NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, qosNullNavEnd, "Duration/ID in QoS Null is too short");
    NS_TEST_EXPECT_MSG_LT(qosNullNavEnd, navEnd + tolerance, "Duration/ID in QoS Null is too long");

    // A third STA sends a QoS Null frame in a TB PPDU a SIFS after the reception of the BSRP TF
    NS_TEST_EXPECT_MSG_EQ((m_txPsdus[8].txVector.GetPreambleType() == m_tbPreamble &&
                           m_txPsdus[8].psduMap.size() == 1 &&
                           m_txPsdus[8].psduMap.begin()->second->GetNMpdus() == 1),
                          true,
                          "Expected a QoS Null frame in an HE TB PPDU");
    {
        const WifiMacHeader& hdr = m_txPsdus[8].psduMap.begin()->second->GetHeader(0);
        NS_TEST_EXPECT_MSG_EQ(hdr.GetType(), WIFI_MAC_QOSDATA_NULL, "Expected a QoS Null frame");
        uint16_t staId;
        for (staId = 0; staId < m_nStations; staId++)
        {
            if (DynamicCast<WifiNetDevice>(m_staDevices.Get(staId))->GetAddress() == hdr.GetAddr2())
            {
                break;
            }
        }
        NS_TEST_EXPECT_MSG_NE(+staId, m_nStations, "Sender not found among stations");
        uint8_t tid = staId * 2;
        NS_TEST_EXPECT_MSG_EQ(+hdr.GetQosTid(), +tid, "Expected a TID equal to " << +tid);
    }
    tStart = m_txPsdus[8].startTx;
    NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "QoS Null frame in HE TB PPDU sent too early");
    NS_TEST_EXPECT_MSG_LT(tStart,
                          tEnd + sifs + tolerance,
                          "QoS Null frame in HE TB PPDU sent too late");
    qosNullNavEnd = m_txPsdus[8].endTx + m_txPsdus[8].psduMap.begin()->second->GetDuration();
    if (m_txopLimit == 0)
    {
        NS_TEST_EXPECT_MSG_EQ(qosNullNavEnd,
                              m_txPsdus[8].endTx +
                                  (m_continueTxopAfterBsrp ? m_defaultTbPpduDuration : Time{0}),
                              "Expected null Duration/ID for QoS Null frame in HE TB PPDU");
    }
    // navEnd <= qosNullNavEnd < navEnd + tolerance
    NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, qosNullNavEnd, "Duration/ID in QoS Null is too short");
    NS_TEST_EXPECT_MSG_LT(qosNullNavEnd, navEnd + tolerance, "Duration/ID in QoS Null is too long");

    // A fourth STA sends a QoS Null frame in a TB PPDU a SIFS after the reception of the BSRP TF
    NS_TEST_EXPECT_MSG_EQ((m_txPsdus[9].txVector.GetPreambleType() == m_tbPreamble &&
                           m_txPsdus[9].psduMap.size() == 1 &&
                           m_txPsdus[9].psduMap.begin()->second->GetNMpdus() == 1),
                          true,
                          "Expected a QoS Null frame in an HE TB PPDU");
    {
        const WifiMacHeader& hdr = m_txPsdus[9].psduMap.begin()->second->GetHeader(0);
        NS_TEST_EXPECT_MSG_EQ(hdr.GetType(), WIFI_MAC_QOSDATA_NULL, "Expected a QoS Null frame");
        uint16_t staId;
        for (staId = 0; staId < m_nStations; staId++)
        {
            if (DynamicCast<WifiNetDevice>(m_staDevices.Get(staId))->GetAddress() == hdr.GetAddr2())
            {
                break;
            }
        }
        NS_TEST_EXPECT_MSG_NE(+staId, m_nStations, "Sender not found among stations");
        uint8_t tid = staId * 2;
        NS_TEST_EXPECT_MSG_EQ(+hdr.GetQosTid(), +tid, "Expected a TID equal to " << +tid);
    }
    tStart = m_txPsdus[9].startTx;
    NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "QoS Null frame in HE TB PPDU sent too early");
    NS_TEST_EXPECT_MSG_LT(tStart,
                          tEnd + sifs + tolerance,
                          "QoS Null frame in HE TB PPDU sent too late");
    qosNullNavEnd = m_txPsdus[9].endTx + m_txPsdus[9].psduMap.begin()->second->GetDuration();
    if (m_txopLimit == 0)
    {
        NS_TEST_EXPECT_MSG_EQ(qosNullNavEnd,
                              m_txPsdus[9].endTx +
                                  (m_continueTxopAfterBsrp ? m_defaultTbPpduDuration : Time{0}),
                              "Expected null Duration/ID for QoS Null frame in HE TB PPDU");
    }
    // navEnd <= qosNullNavEnd < navEnd + tolerance
    NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, qosNullNavEnd, "Duration/ID in QoS Null is too short");
    NS_TEST_EXPECT_MSG_LT(qosNullNavEnd, navEnd + tolerance, "Duration/ID in QoS Null is too long");

    // if the Basic TF is sent in a separate TXOP than the BSRP TF, MU-RTS protection is used for
    // the Basic TF. Otherwise, MU-RTS is sent if an MU-RTS has not been sent earlier (to protect
    // the BSRP TF) and STAs are not considered protected if they responded
    const auto twoTxops = m_txopLimit == 0 && !m_continueTxopAfterBsrp;
    const auto secondMuRts = twoTxops || (m_skipMuRtsBeforeBsrp && !m_protectedIfResponded);

    tEnd = m_txPsdus[9].endTx;
    tStart = m_txPsdus[10].startTx;
    NS_TEST_EXPECT_MSG_LT(tEnd + (twoTxops ? ifs : sifs),
                          tStart,
                          (secondMuRts ? "MU-RTS" : "Basic Trigger Frame") << " sent too early");

    if (!twoTxops)
    {
        NS_TEST_EXPECT_MSG_LT(tStart,
                              tEnd + sifs + tolerance,
                              (secondMuRts ? "MU-RTS" : "Basic Trigger Frame") << " sent too late");
    }

    if (m_txopLimit > 0)
    {
        // Duration/ID of Basic TF still protects until the end of the TXOP
        auto basicTfNavEnd = m_txPsdus[10].endTx + m_txPsdus[10].psduMap[SU_STA_ID]->GetDuration();
        // navEnd <= basicTfNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, basicTfNavEnd, "Duration/ID in MU-RTS is too short");
        NS_TEST_EXPECT_MSG_LT(basicTfNavEnd,
                              navEnd + tolerance,
                              "Duration/ID in MU-RTS is too long");
    }
    else if (m_continueTxopAfterBsrp)
    {
        // the Basic TF sets a new NAV
        navEnd = m_txPsdus[10].endTx + m_txPsdus[10].psduMap[SU_STA_ID]->GetDuration();
    }

    if (secondMuRts)
    {
        // the AP sends another MU-RTS Trigger Frame to protect the Basic TF
        NS_TEST_ASSERT_MSG_GT_OR_EQ(m_txPsdus.size(),
                                    15,
                                    "Expected at least 15 transmitted packet");
        NS_TEST_EXPECT_MSG_EQ(
            (m_txPsdus[10].psduMap.size() == 1 &&
             m_txPsdus[10].psduMap[SU_STA_ID]->GetHeader(0).IsTrigger() &&
             m_txPsdus[10].psduMap[SU_STA_ID]->GetHeader(0).GetAddr1().IsBroadcast()),
            true,
            "Expected a Trigger Frame");
        m_txPsdus[10].psduMap[SU_STA_ID]->GetPayload(0)->PeekHeader(trigger);
        NS_TEST_EXPECT_MSG_EQ(trigger.IsMuRts(), true, "Expected an MU-RTS Trigger Frame");
        NS_TEST_EXPECT_MSG_EQ(trigger.GetNUserInfoFields(),
                              4,
                              "Expected one User Info field per station");
        NS_TEST_EXPECT_MSG_EQ(m_txPsdus[10].txVector.GetChannelWidth(),
                              m_channelWidth,
                              "Expected the MU-RTS to occupy the entire channel width");
        for (const auto& userInfo : trigger)
        {
            NS_TEST_EXPECT_MSG_EQ(+userInfo.GetMuRtsRuAllocation(),
                                  +m_muRtsRuAllocation,
                                  "Unexpected RU Allocation value in MU-RTS");
        }

        // NAV end is now set by the Duration/ID of the second MU-RTS TF
        tEnd = m_txPsdus[10].endTx;
        navEnd = tEnd + m_txPsdus[10].psduMap[SU_STA_ID]->GetDuration();

        // A first STA sends a CTS frame a SIFS after the reception of the MU-RTS TF
        NS_TEST_EXPECT_MSG_EQ(
            (m_txPsdus[11].txVector.GetPreambleType() != WIFI_PREAMBLE_HE_TB &&
             m_txPsdus[11].psduMap.size() == 1 &&
             m_txPsdus[11].psduMap.begin()->second->GetNMpdus() == 1 &&
             m_txPsdus[11].psduMap.begin()->second->GetHeader(0).GetType() == WIFI_MAC_CTL_CTS),
            true,
            "Expected a CTS frame");
        NS_TEST_EXPECT_MSG_EQ(m_txPsdus[11].txVector.GetChannelWidth(),
                              m_channelWidth,
                              "Expected the CTS to occupy the entire channel width");

        tStart = m_txPsdus[11].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "CTS frame sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart, tEnd + sifs + tolerance, "CTS frame sent too late");
        ctsNavEnd = m_txPsdus[11].endTx + m_txPsdus[11].psduMap[SU_STA_ID]->GetDuration();
        // navEnd <= ctsNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, ctsNavEnd, "Duration/ID in CTS frame is too short");
        NS_TEST_EXPECT_MSG_LT(ctsNavEnd,
                              navEnd + tolerance,
                              "Duration/ID in CTS frame is too long");

        // A second STA sends a CTS frame a SIFS after the reception of the MU-RTS TF
        NS_TEST_EXPECT_MSG_EQ(
            (m_txPsdus[12].txVector.GetPreambleType() != WIFI_PREAMBLE_HE_TB &&
             m_txPsdus[12].psduMap.size() == 1 &&
             m_txPsdus[12].psduMap.begin()->second->GetNMpdus() == 1 &&
             m_txPsdus[12].psduMap.begin()->second->GetHeader(0).GetType() == WIFI_MAC_CTL_CTS),
            true,
            "Expected a CTS frame");
        NS_TEST_EXPECT_MSG_EQ(m_txPsdus[12].txVector.GetChannelWidth(),
                              m_channelWidth,
                              "Expected the CTS to occupy the entire channel width");

        tStart = m_txPsdus[12].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "CTS frame sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart, tEnd + sifs + tolerance, "CTS frame sent too late");
        ctsNavEnd = m_txPsdus[12].endTx + m_txPsdus[12].psduMap[SU_STA_ID]->GetDuration();
        // navEnd <= ctsNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, ctsNavEnd, "Duration/ID in CTS frame is too short");
        NS_TEST_EXPECT_MSG_LT(ctsNavEnd,
                              navEnd + tolerance,
                              "Duration/ID in CTS frame is too long");

        // A third STA sends a CTS frame a SIFS after the reception of the MU-RTS TF
        NS_TEST_EXPECT_MSG_EQ(
            (m_txPsdus[13].txVector.GetPreambleType() != WIFI_PREAMBLE_HE_TB &&
             m_txPsdus[13].psduMap.size() == 1 &&
             m_txPsdus[13].psduMap.begin()->second->GetNMpdus() == 1 &&
             m_txPsdus[13].psduMap.begin()->second->GetHeader(0).GetType() == WIFI_MAC_CTL_CTS),
            true,
            "Expected a CTS frame");
        NS_TEST_EXPECT_MSG_EQ(m_txPsdus[13].txVector.GetChannelWidth(),
                              m_channelWidth,
                              "Expected the CTS to occupy the entire channel width");

        tStart = m_txPsdus[13].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "CTS frame sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart, tEnd + sifs + tolerance, "CTS frame sent too late");
        ctsNavEnd = m_txPsdus[13].endTx + m_txPsdus[13].psduMap[SU_STA_ID]->GetDuration();
        // navEnd <= ctsNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, ctsNavEnd, "Duration/ID in CTS frame is too short");
        NS_TEST_EXPECT_MSG_LT(ctsNavEnd,
                              navEnd + tolerance,
                              "Duration/ID in CTS frame is too long");

        // A fourth STA sends a CTS frame a SIFS after the reception of the MU-RTS TF
        NS_TEST_EXPECT_MSG_EQ(
            (m_txPsdus[14].txVector.GetPreambleType() != WIFI_PREAMBLE_HE_TB &&
             m_txPsdus[14].psduMap.size() == 1 &&
             m_txPsdus[14].psduMap.begin()->second->GetNMpdus() == 1 &&
             m_txPsdus[14].psduMap.begin()->second->GetHeader(0).GetType() == WIFI_MAC_CTL_CTS),
            true,
            "Expected a CTS frame");
        NS_TEST_EXPECT_MSG_EQ(m_txPsdus[14].txVector.GetChannelWidth(),
                              m_channelWidth,
                              "Expected the CTS to occupy the entire channel width");

        tStart = m_txPsdus[14].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "CTS frame sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart, tEnd + sifs + tolerance, "CTS frame sent too late");
        ctsNavEnd = m_txPsdus[14].endTx + m_txPsdus[14].psduMap[SU_STA_ID]->GetDuration();
        // navEnd <= ctsNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, ctsNavEnd, "Duration/ID in CTS frame is too short");
        NS_TEST_EXPECT_MSG_LT(ctsNavEnd,
                              navEnd + tolerance,
                              "Duration/ID in CTS frame is too long");

        tEnd = m_txPsdus[14].endTx;
    }
    else
    {
        // insert 5 elements in m_txPsdus to align the index of the following frames in the
        // two cases (TXOP limit null and not null)
        m_txPsdus.insert(std::next(m_txPsdus.begin(), 10), 5, {});
        tEnd = m_txPsdus[9].endTx;
    }

    // the AP sends a Basic Trigger Frame to solicit QoS data frames
    NS_TEST_ASSERT_MSG_GT_OR_EQ(m_txPsdus.size(), 21, "Expected at least 21 transmitted packets");
    NS_TEST_EXPECT_MSG_EQ((m_txPsdus[15].psduMap.size() == 1 &&
                           m_txPsdus[15].psduMap[SU_STA_ID]->GetHeader(0).IsTrigger() &&
                           m_txPsdus[15].psduMap[SU_STA_ID]->GetHeader(0).GetAddr1().IsBroadcast()),
                          true,
                          "Expected a Trigger Frame");
    m_txPsdus[15].psduMap[SU_STA_ID]->GetPayload(0)->PeekHeader(trigger);
    NS_TEST_EXPECT_MSG_EQ(trigger.IsBasic(), true, "Expected a Basic Trigger Frame");
    NS_TEST_EXPECT_MSG_EQ(trigger.GetNUserInfoFields(),
                          4,
                          "Expected one User Info field per station");
    tStart = m_txPsdus[15].startTx;
    NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "Basic Trigger Frame sent too early");
    NS_TEST_EXPECT_MSG_LT(tStart, tEnd + sifs + tolerance, "Basic Trigger Frame sent too late");
    Time basicNavEnd = m_txPsdus[15].endTx + m_txPsdus[15].psduMap[SU_STA_ID]->GetDuration();
    // navEnd <= basicNavEnd < navEnd + tolerance
    NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, basicNavEnd, "Duration/ID in Basic TF is too short");
    NS_TEST_EXPECT_MSG_LT(basicNavEnd, navEnd + tolerance, "Duration/ID in Basic TF is too long");

    // A first STA sends QoS data frames in a TB PPDU a SIFS after the reception of the Basic TF
    NS_TEST_EXPECT_MSG_EQ((m_txPsdus[16].txVector.GetPreambleType() == m_tbPreamble &&
                           m_txPsdus[16].psduMap.size() == 1 &&
                           m_txPsdus[16].psduMap.begin()->second->GetNMpdus() == 2 &&
                           m_txPsdus[16].psduMap.begin()->second->GetHeader(0).IsQosData() &&
                           m_txPsdus[16].psduMap.begin()->second->GetHeader(1).IsQosData()),
                          true,
                          "Expected 2 QoS data frames in an HE TB PPDU");
    tEnd = m_txPsdus[15].endTx;
    tStart = m_txPsdus[16].startTx;
    NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "QoS data frames in HE TB PPDU sent too early");
    NS_TEST_EXPECT_MSG_LT(tStart,
                          tEnd + sifs + tolerance,
                          "QoS data frames in HE TB PPDU sent too late");
    Time qosDataNavEnd = m_txPsdus[16].endTx + m_txPsdus[16].psduMap.begin()->second->GetDuration();
    // navEnd <= qosDataNavEnd < navEnd + tolerance
    NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, qosDataNavEnd, "Duration/ID in QoS Data is too short");
    NS_TEST_EXPECT_MSG_LT(qosDataNavEnd, navEnd + tolerance, "Duration/ID in QoS Data is too long");

    // A second STA sends QoS data frames in a TB PPDU a SIFS after the reception of the Basic TF
    NS_TEST_EXPECT_MSG_EQ((m_txPsdus[17].txVector.GetPreambleType() == m_tbPreamble &&
                           m_txPsdus[17].psduMap.size() == 1 &&
                           m_txPsdus[17].psduMap.begin()->second->GetNMpdus() == 2 &&
                           m_txPsdus[17].psduMap.begin()->second->GetHeader(0).IsQosData() &&
                           m_txPsdus[17].psduMap.begin()->second->GetHeader(1).IsQosData()),
                          true,
                          "Expected 2 QoS data frames in an HE TB PPDU");
    tStart = m_txPsdus[17].startTx;
    NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "QoS data frames in HE TB PPDU sent too early");
    NS_TEST_EXPECT_MSG_LT(tStart,
                          tEnd + sifs + tolerance,
                          "QoS data frames in HE TB PPDU sent too late");
    qosDataNavEnd = m_txPsdus[17].endTx + m_txPsdus[17].psduMap.begin()->second->GetDuration();
    // navEnd <= qosDataNavEnd < navEnd + tolerance
    NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, qosDataNavEnd, "Duration/ID in QoS Data is too short");
    NS_TEST_EXPECT_MSG_LT(qosDataNavEnd, navEnd + tolerance, "Duration/ID in QoS Data is too long");

    // A third STA sends QoS data frames in a TB PPDU a SIFS after the reception of the Basic TF
    NS_TEST_EXPECT_MSG_EQ((m_txPsdus[18].txVector.GetPreambleType() == m_tbPreamble &&
                           m_txPsdus[18].psduMap.size() == 1 &&
                           m_txPsdus[18].psduMap.begin()->second->GetNMpdus() == 2 &&
                           m_txPsdus[18].psduMap.begin()->second->GetHeader(0).IsQosData() &&
                           m_txPsdus[18].psduMap.begin()->second->GetHeader(1).IsQosData()),
                          true,
                          "Expected 2 QoS data frames in an HE TB PPDU");
    tStart = m_txPsdus[18].startTx;
    NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "QoS data frames in HE TB PPDU sent too early");
    NS_TEST_EXPECT_MSG_LT(tStart,
                          tEnd + sifs + tolerance,
                          "QoS data frames in HE TB PPDU sent too late");
    qosDataNavEnd = m_txPsdus[18].endTx + m_txPsdus[18].psduMap.begin()->second->GetDuration();
    // navEnd <= qosDataNavEnd < navEnd + tolerance
    NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, qosDataNavEnd, "Duration/ID in QoS Data is too short");
    NS_TEST_EXPECT_MSG_LT(qosDataNavEnd, navEnd + tolerance, "Duration/ID in QoS Data is too long");

    // A fourth STA sends QoS data frames in a TB PPDU a SIFS after the reception of the Basic TF
    NS_TEST_EXPECT_MSG_EQ((m_txPsdus[19].txVector.GetPreambleType() == m_tbPreamble &&
                           m_txPsdus[19].psduMap.size() == 1 &&
                           m_txPsdus[19].psduMap.begin()->second->GetNMpdus() == 2 &&
                           m_txPsdus[19].psduMap.begin()->second->GetHeader(0).IsQosData() &&
                           m_txPsdus[19].psduMap.begin()->second->GetHeader(1).IsQosData()),
                          true,
                          "Expected 2 QoS data frames in an HE TB PPDU");
    tStart = m_txPsdus[19].startTx;
    NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "QoS data frames in HE TB PPDU sent too early");
    NS_TEST_EXPECT_MSG_LT(tStart,
                          tEnd + sifs + tolerance,
                          "QoS data frames in HE TB PPDU sent too late");
    qosDataNavEnd = m_txPsdus[19].endTx + m_txPsdus[19].psduMap.begin()->second->GetDuration();
    // navEnd <= qosDataNavEnd < navEnd + tolerance
    NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, qosDataNavEnd, "Duration/ID in QoS Data is too short");
    NS_TEST_EXPECT_MSG_LT(qosDataNavEnd, navEnd + tolerance, "Duration/ID in QoS Data is too long");

    // the AP sends a Multi-STA Block Ack
    NS_TEST_EXPECT_MSG_EQ((m_txPsdus[20].psduMap.size() == 1 &&
                           m_txPsdus[20].psduMap[SU_STA_ID]->GetHeader(0).IsBlockAck() &&
                           m_txPsdus[20].psduMap[SU_STA_ID]->GetHeader(0).GetAddr1().IsBroadcast()),
                          true,
                          "Expected a Block Ack");
    m_txPsdus[20].psduMap[SU_STA_ID]->GetPayload(0)->PeekHeader(blockAck);
    NS_TEST_EXPECT_MSG_EQ(blockAck.IsMultiSta(), true, "Expected a Multi-STA Block Ack");
    NS_TEST_EXPECT_MSG_EQ(blockAck.GetNPerAidTidInfoSubfields(),
                          4,
                          "Expected one Per AID TID Info subfield per station");
    for (uint8_t i = 0; i < 4; i++)
    {
        NS_TEST_EXPECT_MSG_EQ(blockAck.GetAckType(i), true, "Expected All-ack context");
        NS_TEST_EXPECT_MSG_EQ(+blockAck.GetTidInfo(i), 14, "Expected All-ack context");
    }
    tEnd = m_txPsdus[19].endTx;
    tStart = m_txPsdus[20].startTx;
    NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "Multi-STA Block Ack sent too early");
    NS_TEST_EXPECT_MSG_LT(tStart, tEnd + sifs + tolerance, "Multi-STA Block Ack sent too late");
    auto multiStaBaNavEnd = m_txPsdus[20].endTx + m_txPsdus[20].psduMap[SU_STA_ID]->GetDuration();
    // navEnd <= multiStaBaNavEnd < navEnd + tolerance
    NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd,
                                multiStaBaNavEnd,
                                "Duration/ID in Multi-STA BlockAck is too short");
    NS_TEST_EXPECT_MSG_LT(multiStaBaNavEnd,
                          navEnd + tolerance,
                          "Duration/ID in Multi-STA BlockAck is too long");

    // if the TXOP limit is not null, MU-RTS protection is not used because the next transmission
    // is protected by the previous MU-RTS Trigger Frame
    if (m_txopLimit == 0)
    {
        // the AP sends an MU-RTS Trigger Frame to protect the DL MU PPDU
        NS_TEST_ASSERT_MSG_GT_OR_EQ(m_txPsdus.size(),
                                    26,
                                    "Expected at least 26 transmitted packet");
        NS_TEST_EXPECT_MSG_EQ(
            (m_txPsdus[21].psduMap.size() == 1 &&
             m_txPsdus[21].psduMap[SU_STA_ID]->GetHeader(0).IsTrigger() &&
             m_txPsdus[21].psduMap[SU_STA_ID]->GetHeader(0).GetAddr1().IsBroadcast()),
            true,
            "Expected a Trigger Frame");
        m_txPsdus[21].psduMap[SU_STA_ID]->GetPayload(0)->PeekHeader(trigger);
        NS_TEST_EXPECT_MSG_EQ(trigger.IsMuRts(), true, "Expected an MU-RTS Trigger Frame");
        NS_TEST_EXPECT_MSG_EQ(trigger.GetNUserInfoFields(),
                              4,
                              "Expected one User Info field per station");
        NS_TEST_EXPECT_MSG_EQ(m_txPsdus[21].txVector.GetChannelWidth(),
                              m_channelWidth,
                              "Expected the MU-RTS to occupy the entire channel width");
        for (const auto& userInfo : trigger)
        {
            NS_TEST_EXPECT_MSG_EQ(+userInfo.GetMuRtsRuAllocation(),
                                  +m_muRtsRuAllocation,
                                  "Unexpected RU Allocation value in MU-RTS");
        }
        tEnd = m_txPsdus[20].endTx;
        tStart = m_txPsdus[21].startTx;
        NS_TEST_EXPECT_MSG_LT_OR_EQ(tEnd + ifs, tStart, "MU-RTS Trigger Frame sent too early");
        tEnd = m_txPsdus[21].endTx;
        navEnd = tEnd + m_txPsdus[21].psduMap[SU_STA_ID]->GetDuration();

        // A first STA sends a CTS frame a SIFS after the reception of the MU-RTS TF
        NS_TEST_EXPECT_MSG_EQ(
            (m_txPsdus[22].txVector.GetPreambleType() != WIFI_PREAMBLE_HE_TB &&
             m_txPsdus[22].psduMap.size() == 1 &&
             m_txPsdus[22].psduMap.begin()->second->GetNMpdus() == 1 &&
             m_txPsdus[22].psduMap.begin()->second->GetHeader(0).GetType() == WIFI_MAC_CTL_CTS),
            true,
            "Expected a CTS frame");
        NS_TEST_EXPECT_MSG_EQ(m_txPsdus[22].txVector.GetChannelWidth(),
                              m_channelWidth,
                              "Expected the CTS to occupy the entire channel width");

        tStart = m_txPsdus[22].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "CTS frame sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart, tEnd + sifs + tolerance, "CTS frame sent too late");
        ctsNavEnd = m_txPsdus[22].endTx + m_txPsdus[22].psduMap[SU_STA_ID]->GetDuration();
        // navEnd <= ctsNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, ctsNavEnd, "Duration/ID in CTS frame is too short");
        NS_TEST_EXPECT_MSG_LT(ctsNavEnd,
                              navEnd + tolerance,
                              "Duration/ID in CTS frame is too long");

        // A second STA sends a CTS frame a SIFS after the reception of the MU-RTS TF
        NS_TEST_EXPECT_MSG_EQ(
            (m_txPsdus[23].txVector.GetPreambleType() != WIFI_PREAMBLE_HE_TB &&
             m_txPsdus[23].psduMap.size() == 1 &&
             m_txPsdus[23].psduMap.begin()->second->GetNMpdus() == 1 &&
             m_txPsdus[23].psduMap.begin()->second->GetHeader(0).GetType() == WIFI_MAC_CTL_CTS),
            true,
            "Expected a CTS frame");
        NS_TEST_EXPECT_MSG_EQ(m_txPsdus[23].txVector.GetChannelWidth(),
                              m_channelWidth,
                              "Expected the CTS to occupy the entire channel width");

        tStart = m_txPsdus[23].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "CTS frame sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart, tEnd + sifs + tolerance, "CTS frame sent too late");
        ctsNavEnd = m_txPsdus[23].endTx + m_txPsdus[23].psduMap[SU_STA_ID]->GetDuration();
        // navEnd <= ctsNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, ctsNavEnd, "Duration/ID in CTS frame is too short");
        NS_TEST_EXPECT_MSG_LT(ctsNavEnd,
                              navEnd + tolerance,
                              "Duration/ID in CTS frame is too long");

        // A third STA sends a CTS frame a SIFS after the reception of the MU-RTS TF
        NS_TEST_EXPECT_MSG_EQ(
            (m_txPsdus[24].txVector.GetPreambleType() != WIFI_PREAMBLE_HE_TB &&
             m_txPsdus[24].psduMap.size() == 1 &&
             m_txPsdus[24].psduMap.begin()->second->GetNMpdus() == 1 &&
             m_txPsdus[24].psduMap.begin()->second->GetHeader(0).GetType() == WIFI_MAC_CTL_CTS),
            true,
            "Expected a CTS frame");
        NS_TEST_EXPECT_MSG_EQ(m_txPsdus[24].txVector.GetChannelWidth(),
                              m_channelWidth,
                              "Expected the CTS to occupy the entire channel width");

        tStart = m_txPsdus[24].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "CTS frame sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart, tEnd + sifs + tolerance, "CTS frame sent too late");
        ctsNavEnd = m_txPsdus[24].endTx + m_txPsdus[24].psduMap[SU_STA_ID]->GetDuration();
        // navEnd <= ctsNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, ctsNavEnd, "Duration/ID in CTS frame is too short");
        NS_TEST_EXPECT_MSG_LT(ctsNavEnd,
                              navEnd + tolerance,
                              "Duration/ID in CTS frame is too long");

        // A fourth STA sends a CTS frame a SIFS after the reception of the MU-RTS TF
        NS_TEST_EXPECT_MSG_EQ(
            (m_txPsdus[25].txVector.GetPreambleType() != WIFI_PREAMBLE_HE_TB &&
             m_txPsdus[25].psduMap.size() == 1 &&
             m_txPsdus[25].psduMap.begin()->second->GetNMpdus() == 1 &&
             m_txPsdus[25].psduMap.begin()->second->GetHeader(0).GetType() == WIFI_MAC_CTL_CTS),
            true,
            "Expected a CTS frame");
        NS_TEST_EXPECT_MSG_EQ(m_txPsdus[25].txVector.GetChannelWidth(),
                              m_channelWidth,
                              "Expected the CTS to occupy the entire channel width");

        tStart = m_txPsdus[25].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "CTS frame sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart, tEnd + sifs + tolerance, "CTS frame sent too late");
        ctsNavEnd = m_txPsdus[25].endTx + m_txPsdus[25].psduMap[SU_STA_ID]->GetDuration();
        // navEnd <= ctsNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, ctsNavEnd, "Duration/ID in CTS frame is too short");
        NS_TEST_EXPECT_MSG_LT(ctsNavEnd,
                              navEnd + tolerance,
                              "Duration/ID in CTS frame is too long");

        tEnd = m_txPsdus[25].endTx;
    }
    else
    {
        // insert 5 elements in m_txPsdus to align the index of the following frames in the
        // two cases (TXOP limit null and not null)
        m_txPsdus.insert(std::next(m_txPsdus.begin(), 21), 5, {});
        tEnd = m_txPsdus[20].endTx;
    }

    // the AP sends a DL MU PPDU
    NS_TEST_ASSERT_MSG_GT_OR_EQ(m_txPsdus.size(), 27, "Expected at least 27 transmitted packet");
    NS_TEST_EXPECT_MSG_EQ(m_txPsdus[26].txVector.GetPreambleType(),
                          m_dlMuPreamble,
                          "Expected a DL MU PPDU");
    NS_TEST_EXPECT_MSG_EQ(m_txPsdus[26].psduMap.size(),
                          4,
                          "Expected 4 PSDUs within the DL MU PPDU");
    // the TX duration cannot exceed the maximum PPDU duration
    NS_TEST_EXPECT_MSG_LT_OR_EQ(m_txPsdus[26].endTx - m_txPsdus[26].startTx,
                                GetPpduMaxTime(m_txPsdus[26].txVector.GetPreambleType()),
                                "TX duration cannot exceed max PPDU duration");
    for (auto& psdu : m_txPsdus[26].psduMap)
    {
        NS_TEST_EXPECT_MSG_LT_OR_EQ(psdu.second->GetSize(),
                                    m_maxAmpduSize,
                                    "Max A-MPDU size exceeded");
    }
    tStart = m_txPsdus[26].startTx;
    NS_TEST_EXPECT_MSG_LT_OR_EQ(tEnd + sifs, tStart, "DL MU PPDU sent too early");
    NS_TEST_EXPECT_MSG_LT(tStart, tEnd + sifs + tolerance, "DL MU PPDU sent too late");

    // The Duration/ID field is the same for all the PSDUs
    auto dlMuNavEnd = m_txPsdus[26].endTx;
    for (auto& psdu : m_txPsdus[26].psduMap)
    {
        if (dlMuNavEnd == m_txPsdus[26].endTx)
        {
            dlMuNavEnd += psdu.second->GetDuration();
        }
        else
        {
            NS_TEST_EXPECT_MSG_EQ(m_txPsdus[26].endTx + psdu.second->GetDuration(),
                                  dlMuNavEnd,
                                  "Duration/ID must be the same for all PSDUs");
        }
    }
    // navEnd <= dlMuNavEnd < navEnd + tolerance
    NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, dlMuNavEnd, "Duration/ID in DL MU PPDU is too short");
    NS_TEST_EXPECT_MSG_LT(dlMuNavEnd, navEnd + tolerance, "Duration/ID in DL MU PPDU is too long");

    std::size_t nTxPsdus = 0;

    if (m_dlMuAckType == WifiAcknowledgment::DL_MU_BAR_BA_SEQUENCE)
    {
        /*
         *        |-----------------------------------------NAV-------------------------------->|
         *                 |----------------------------------NAV------------------------------>|
         *                           |-----------------------------NAV------------------------->|
         *                                   |-------------------------NAV--------------------->|
         *                                            |--NAV->|        |--NAV->|        |--NAV->|
         *    ┌───┐    ┌───┐    ┌────┐    ┌──┐    ┌───┐    ┌──┐    ┌───┐    ┌──┐    ┌───┐    ┌──┐
         *    │   │    │   │    │PSDU│    │  │    │   │    │  │    │   │    │  │    │   │    │  │
         *    │   │    │   │    │  1 │    │  │    │   │    │  │    │   │    │  │    │   │    │  │
         *    │   │    │   │    ├────┤    │  │    │   │    │  │    │   │    │  │    │   │    │  │
         *    │   │    │   │    │PSDU│    │  │    │   │    │  │    │   │    │  │    │   │    │  │
         *    │MU-│    │CTS│    │  2 │    │BA│    │BAR│    │BA│    │BAR│    │BA│    │BAR│    │BA│
         *    │RTS│SIFS│   │SIFS├────┤SIFS│  │SIFS│   │SIFS│  │SIFS│   │SIFS│  │SIFS│   │SIFS│  │
         *    │TF │    │x4 │    │PSDU│    │  │    │   │    │  │    │   │    │  │    │   │    │  │
         *    │   │    │   │    │  3 │    │  │    │   │    │  │    │   │    │  │    │   │    │  │
         *    │   │    │   │    ├────┤    │  │    │   │    │  │    │   │    │  │    │   │    │  │
         *    │   │    │   │    │PSDU│    │  │    │   │    │  │    │   │    │  │    │   │    │  │
         *    │   │    │   │    │  4 │    │  │    │   │    │  │    │   │    │  │    │   │    │  │
         * ───┴───┴────┴───┴────┴────┴────┴──┴────┴───┴────┴──┴────┴───┴────┴──┴────┴───┴────┴──┴──
         * From: AP     all       AP      STA 1    AP     STA 2     AP      STA 3    AP      STA 4
         *   To: all    AP        all      AP     STA 2     AP     STA 3     AP     STA 4     AP
         */
        NS_TEST_EXPECT_MSG_GT_OR_EQ(m_txPsdus.size(), 34, "Expected at least 34 packets");

        // A first STA sends a Block Ack a SIFS after the reception of the DL MU PPDU
        NS_TEST_EXPECT_MSG_EQ((m_txPsdus[27].psduMap.size() == 1 &&
                               m_txPsdus[27].psduMap[SU_STA_ID]->GetHeader(0).IsBlockAck()),
                              true,
                              "Expected a Block Ack");
        tEnd = m_txPsdus[26].endTx;
        tStart = m_txPsdus[27].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "First Block Ack sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart, tEnd + sifs + tolerance, "First Block Ack sent too late");
        Time baNavEnd = m_txPsdus[27].endTx + m_txPsdus[27].psduMap[SU_STA_ID]->GetDuration();
        // The NAV of the first BlockAck, being a response to a QoS Data frame, matches the NAV
        // set by the MU-RTS TF.
        // navEnd <= baNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd,
                                    baNavEnd,
                                    "Duration/ID in 1st BlockAck frame is too short");
        NS_TEST_EXPECT_MSG_LT(baNavEnd,
                              navEnd + tolerance,
                              "Duration/ID in 1st BlockAck is too long");

        // the AP transmits a Block Ack Request an IFS after the reception of the Block Ack
        NS_TEST_EXPECT_MSG_EQ((m_txPsdus[28].psduMap.size() == 1 &&
                               m_txPsdus[28].psduMap[SU_STA_ID]->GetHeader(0).IsBlockAckReq()),
                              true,
                              "Expected a Block Ack Request");
        tEnd = m_txPsdus[27].endTx;
        tStart = m_txPsdus[28].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "First Block Ack Request sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart,
                              tEnd + sifs + tolerance,
                              "First Block Ack Request sent too late");
        // under single protection setting (TXOP limit equal to zero), the NAV of the BlockAckReq
        // only covers the following BlockAck response; under multiple protection setting, the
        // NAV of the BlockAckReq matches the NAV set by the MU-RTS TF
        Time barNavEnd = m_txPsdus[28].endTx + m_txPsdus[28].psduMap[SU_STA_ID]->GetDuration();
        if (m_txopLimit > 0)
        {
            // navEnd <= barNavEnd < navEnd + tolerance
            NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd,
                                        barNavEnd,
                                        "Duration/ID in BlockAckReq is too short");
            NS_TEST_EXPECT_MSG_LT(barNavEnd,
                                  navEnd + tolerance,
                                  "Duration/ID in BlockAckReq is too long");
        }

        // A second STA sends a Block Ack a SIFS after the reception of the Block Ack Request
        NS_TEST_EXPECT_MSG_EQ((m_txPsdus[29].psduMap.size() == 1 &&
                               m_txPsdus[29].psduMap[SU_STA_ID]->GetHeader(0).IsBlockAck()),
                              true,
                              "Expected a Block Ack");
        tEnd = m_txPsdus[28].endTx;
        tStart = m_txPsdus[29].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "Second Block Ack sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart, tEnd + sifs + tolerance, "Second Block Ack sent too late");
        baNavEnd = m_txPsdus[29].endTx + m_txPsdus[29].psduMap[SU_STA_ID]->GetDuration();
        if (m_txopLimit > 0)
        {
            // navEnd <= baNavEnd < navEnd + tolerance
            NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, baNavEnd, "Duration/ID in BlockAck is too short");
            NS_TEST_EXPECT_MSG_LT(baNavEnd,
                                  navEnd + tolerance,
                                  "Duration/ID in BlockAck is too long");
        }
        else
        {
            // barNavEnd <= baNavEnd < barNavEnd + tolerance
            NS_TEST_EXPECT_MSG_LT_OR_EQ(barNavEnd,
                                        baNavEnd,
                                        "Duration/ID in BlockAck is too short");
            NS_TEST_EXPECT_MSG_LT(baNavEnd,
                                  barNavEnd + tolerance,
                                  "Duration/ID in BlockAck is too long");
            NS_TEST_EXPECT_MSG_EQ(baNavEnd,
                                  m_txPsdus[29].endTx,
                                  "Expected null Duration/ID for BlockAck");
        }

        // the AP transmits a Block Ack Request an IFS after the reception of the Block Ack
        NS_TEST_EXPECT_MSG_EQ((m_txPsdus[30].psduMap.size() == 1 &&
                               m_txPsdus[30].psduMap[SU_STA_ID]->GetHeader(0).IsBlockAckReq()),
                              true,
                              "Expected a Block Ack Request");
        tEnd = m_txPsdus[29].endTx;
        tStart = m_txPsdus[30].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "Second Block Ack Request sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart,
                              tEnd + sifs + tolerance,
                              "Second Block Ack Request sent too late");
        // under single protection setting (TXOP limit equal to zero), the NAV of the BlockAckReq
        // only covers the following BlockAck response; under multiple protection setting, the
        // NAV of the BlockAckReq matches the NAV set by the MU-RTS TF
        barNavEnd = m_txPsdus[30].endTx + m_txPsdus[30].psduMap[SU_STA_ID]->GetDuration();
        if (m_txopLimit > 0)
        {
            // navEnd <= barNavEnd < navEnd + tolerance
            NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd,
                                        barNavEnd,
                                        "Duration/ID in BlockAckReq is too short");
            NS_TEST_EXPECT_MSG_LT(barNavEnd,
                                  navEnd + tolerance,
                                  "Duration/ID in BlockAckReq is too long");
        }

        // A third STA sends a Block Ack a SIFS after the reception of the Block Ack Request
        NS_TEST_EXPECT_MSG_EQ((m_txPsdus[31].psduMap.size() == 1 &&
                               m_txPsdus[31].psduMap[SU_STA_ID]->GetHeader(0).IsBlockAck()),
                              true,
                              "Expected a Block Ack");
        tEnd = m_txPsdus[30].endTx;
        tStart = m_txPsdus[31].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "Third Block Ack sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart, tEnd + sifs + tolerance, "Third Block Ack sent too late");
        baNavEnd = m_txPsdus[31].endTx + m_txPsdus[31].psduMap[SU_STA_ID]->GetDuration();
        if (m_txopLimit > 0)
        {
            // navEnd <= baNavEnd < navEnd + tolerance
            NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, baNavEnd, "Duration/ID in BlockAck is too short");
            NS_TEST_EXPECT_MSG_LT(baNavEnd,
                                  navEnd + tolerance,
                                  "Duration/ID in BlockAck is too long");
        }
        else
        {
            // barNavEnd <= baNavEnd < barNavEnd + tolerance
            NS_TEST_EXPECT_MSG_LT_OR_EQ(barNavEnd,
                                        baNavEnd,
                                        "Duration/ID in BlockAck is too short");
            NS_TEST_EXPECT_MSG_LT(baNavEnd,
                                  barNavEnd + tolerance,
                                  "Duration/ID in BlockAck is too long");
            NS_TEST_EXPECT_MSG_EQ(baNavEnd,
                                  m_txPsdus[31].endTx,
                                  "Expected null Duration/ID for BlockAck");
        }

        // the AP transmits a Block Ack Request an IFS after the reception of the Block Ack
        NS_TEST_EXPECT_MSG_EQ((m_txPsdus[32].psduMap.size() == 1 &&
                               m_txPsdus[32].psduMap[SU_STA_ID]->GetHeader(0).IsBlockAckReq()),
                              true,
                              "Expected a Block Ack Request");
        tEnd = m_txPsdus[31].endTx;
        tStart = m_txPsdus[32].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "Third Block Ack Request sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart,
                              tEnd + sifs + tolerance,
                              "Third Block Ack Request sent too late");
        // under single protection setting (TXOP limit equal to zero), the NAV of the BlockAckReq
        // only covers the following BlockAck response; under multiple protection setting, the
        // NAV of the BlockAckReq matches the NAV set by the MU-RTS TF
        barNavEnd = m_txPsdus[32].endTx + m_txPsdus[32].psduMap[SU_STA_ID]->GetDuration();
        if (m_txopLimit > 0)
        {
            // navEnd <= barNavEnd < navEnd + tolerance
            NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd,
                                        barNavEnd,
                                        "Duration/ID in BlockAckReq is too short");
            NS_TEST_EXPECT_MSG_LT(barNavEnd,
                                  navEnd + tolerance,
                                  "Duration/ID in BlockAckReq is too long");
        }

        // A fourth STA sends a Block Ack a SIFS after the reception of the Block Ack Request
        NS_TEST_EXPECT_MSG_EQ((m_txPsdus[33].psduMap.size() == 1 &&
                               m_txPsdus[33].psduMap[SU_STA_ID]->GetHeader(0).IsBlockAck()),
                              true,
                              "Expected a Block Ack");
        tEnd = m_txPsdus[32].endTx;
        tStart = m_txPsdus[33].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "Fourth Block Ack sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart, tEnd + sifs + tolerance, "Fourth Block Ack sent too late");
        baNavEnd = m_txPsdus[33].endTx + m_txPsdus[33].psduMap[SU_STA_ID]->GetDuration();
        if (m_txopLimit > 0)
        {
            // navEnd <= baNavEnd < navEnd + tolerance
            NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, baNavEnd, "Duration/ID in BlockAck is too short");
            NS_TEST_EXPECT_MSG_LT(baNavEnd,
                                  navEnd + tolerance,
                                  "Duration/ID in BlockAck is too long");
        }
        else
        {
            // barNavEnd <= baNavEnd < barNavEnd + tolerance
            NS_TEST_EXPECT_MSG_LT_OR_EQ(barNavEnd,
                                        baNavEnd,
                                        "Duration/ID in BlockAck is too short");
            NS_TEST_EXPECT_MSG_LT(baNavEnd,
                                  barNavEnd + tolerance,
                                  "Duration/ID in BlockAck is too long");
            NS_TEST_EXPECT_MSG_EQ(baNavEnd,
                                  m_txPsdus[33].endTx,
                                  "Expected null Duration/ID for BlockAck");
        }

        nTxPsdus = 34;
    }
    else if (m_dlMuAckType == WifiAcknowledgment::DL_MU_TF_MU_BAR)
    {
        /*
         *          |---------------------NAV------------------------>|
         *                   |-------------------NAV----------------->|
         *                               |---------------NAV--------->|
         *                                            |------NAV----->|
         *      ┌───┐    ┌───┐    ┌──────┐    ┌───────┐    ┌──────────┐
         *      │   │    │   │    │PSDU 1│    │       │    │BlockAck 1│
         *      │   │    │   │    ├──────┤    │MU-BAR │    ├──────────┤
         *      │MU-│    │CTS│    │PSDU 2│    │Trigger│    │BlockAck 2│
         *      │RTS│SIFS│   │SIFS├──────┤SIFS│ Frame │SIFS├──────────┤
         *      │TF │    │x4 │    │PSDU 3│    │       │    │BlockAck 3│
         *      │   │    │   │    ├──────┤    │       │    ├──────────┤
         *      │   │    │   │    │PSDU 4│    │       │    │BlockAck 4│
         * -----┴───┴────┴───┴────┴──────┴────┴───────┴────┴──────────┴───
         * From: AP       all        AP          AP            all
         *   To: all      AP         all         all           AP
         */
        NS_TEST_EXPECT_MSG_GT_OR_EQ(m_txPsdus.size(), 32, "Expected at least 32 packets");

        // the AP transmits a MU-BAR Trigger Frame a SIFS after the transmission of the DL MU PPDU
        NS_TEST_EXPECT_MSG_EQ((m_txPsdus[27].psduMap.size() == 1 &&
                               m_txPsdus[27].psduMap[SU_STA_ID]->GetHeader(0).IsTrigger()),
                              true,
                              "Expected a MU-BAR Trigger Frame");
        tEnd = m_txPsdus[26].endTx;
        tStart = m_txPsdus[27].startTx;
        NS_TEST_EXPECT_MSG_EQ(tStart, tEnd + sifs, "MU-BAR Trigger Frame sent at wrong time");
        auto muBarNavEnd = m_txPsdus[27].endTx + m_txPsdus[27].psduMap[SU_STA_ID]->GetDuration();
        // navEnd <= muBarNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd,
                                    muBarNavEnd,
                                    "Duration/ID in MU-BAR Trigger Frame is too short");
        NS_TEST_EXPECT_MSG_LT(muBarNavEnd,
                              navEnd + tolerance,
                              "Duration/ID in MU-BAR Trigger Frame is too long");

        // A first STA sends a Block Ack in a TB PPDU a SIFS after the reception of the MU-BAR
        NS_TEST_EXPECT_MSG_EQ((m_txPsdus[28].txVector.GetPreambleType() == m_tbPreamble &&
                               m_txPsdus[28].psduMap.size() == 1 &&
                               m_txPsdus[28].psduMap.begin()->second->GetHeader(0).IsBlockAck()),
                              true,
                              "Expected a Block Ack");
        tEnd = m_txPsdus[27].endTx;
        tStart = m_txPsdus[28].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "Block Ack in HE TB PPDU sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart,
                              tEnd + sifs + tolerance,
                              "Block Ack in HE TB PPDU sent too late");
        Time baNavEnd = m_txPsdus[28].endTx + m_txPsdus[28].psduMap.begin()->second->GetDuration();
        // navEnd <= baNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, baNavEnd, "Duration/ID in BlockAck frame is too short");
        NS_TEST_EXPECT_MSG_LT(baNavEnd, navEnd + tolerance, "Duration/ID in BlockAck is too long");
        if (m_txopLimit == 0)
        {
            NS_TEST_EXPECT_MSG_EQ(baNavEnd,
                                  m_txPsdus[28].endTx,
                                  "Expected null Duration/ID for BlockAck");
        }

        // A second STA sends a Block Ack in a TB PPDU a SIFS after the reception of the MU-BAR
        NS_TEST_EXPECT_MSG_EQ((m_txPsdus[29].txVector.GetPreambleType() == m_tbPreamble &&
                               m_txPsdus[29].psduMap.size() == 1 &&
                               m_txPsdus[29].psduMap.begin()->second->GetHeader(0).IsBlockAck()),
                              true,
                              "Expected a Block Ack");
        tStart = m_txPsdus[29].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "Block Ack in HE TB PPDU sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart,
                              tEnd + sifs + tolerance,
                              "Block Ack in HE TB PPDU sent too late");
        baNavEnd = m_txPsdus[29].endTx + m_txPsdus[29].psduMap.begin()->second->GetDuration();
        // navEnd <= baNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, baNavEnd, "Duration/ID in BlockAck frame is too short");
        NS_TEST_EXPECT_MSG_LT(baNavEnd,
                              navEnd + tolerance,
                              "Duration/ID in 1st BlockAck is too long");
        if (m_txopLimit == 0)
        {
            NS_TEST_EXPECT_MSG_EQ(baNavEnd,
                                  m_txPsdus[29].endTx,
                                  "Expected null Duration/ID for BlockAck");
        }

        // A third STA sends a Block Ack in a TB PPDU a SIFS after the reception of the MU-BAR
        NS_TEST_EXPECT_MSG_EQ((m_txPsdus[30].txVector.GetPreambleType() == m_tbPreamble &&
                               m_txPsdus[30].psduMap.size() == 1 &&
                               m_txPsdus[30].psduMap.begin()->second->GetHeader(0).IsBlockAck()),
                              true,
                              "Expected a Block Ack");
        tStart = m_txPsdus[30].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "Block Ack in HE TB PPDU sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart,
                              tEnd + sifs + tolerance,
                              "Block Ack in HE TB PPDU sent too late");
        baNavEnd = m_txPsdus[30].endTx + m_txPsdus[30].psduMap.begin()->second->GetDuration();
        // navEnd <= baNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, baNavEnd, "Duration/ID in BlockAck frame is too short");
        NS_TEST_EXPECT_MSG_LT(baNavEnd,
                              navEnd + tolerance,
                              "Duration/ID in 1st BlockAck is too long");
        if (m_txopLimit == 0)
        {
            NS_TEST_EXPECT_MSG_EQ(baNavEnd,
                                  m_txPsdus[30].endTx,
                                  "Expected null Duration/ID for BlockAck");
        }

        // A fourth STA sends a Block Ack in a TB PPDU a SIFS after the reception of the MU-BAR
        NS_TEST_EXPECT_MSG_EQ((m_txPsdus[31].txVector.GetPreambleType() == m_tbPreamble &&
                               m_txPsdus[31].psduMap.size() == 1 &&
                               m_txPsdus[31].psduMap.begin()->second->GetHeader(0).IsBlockAck()),
                              true,
                              "Expected a Block Ack");
        tStart = m_txPsdus[31].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "Block Ack in HE TB PPDU sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart,
                              tEnd + sifs + tolerance,
                              "Block Ack in HE TB PPDU sent too late");
        baNavEnd = m_txPsdus[31].endTx + m_txPsdus[31].psduMap.begin()->second->GetDuration();
        // navEnd <= baNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, baNavEnd, "Duration/ID in BlockAck frame is too short");
        NS_TEST_EXPECT_MSG_LT(baNavEnd,
                              navEnd + tolerance,
                              "Duration/ID in 1st BlockAck is too long");
        if (m_txopLimit == 0)
        {
            NS_TEST_EXPECT_MSG_EQ(baNavEnd,
                                  m_txPsdus[31].endTx,
                                  "Expected null Duration/ID for BlockAck");
        }

        nTxPsdus = 32;
    }
    else if (m_dlMuAckType == WifiAcknowledgment::DL_MU_AGGREGATE_TF)
    {
        /*
         *          |---------------------NAV----------------------->|
         *                   |-------------------NAV---------------->|
         *                                           |------NAV----->|
         *      ┌───┐    ┌───┐    ┌──────┬───────────┐    ┌──────────┐
         *      │   │    │   │    │PSDU 1│MU-BAR TF 1│    │BlockAck 1│
         *      │   │    │   │    ├──────┼───────────┤    ├──────────┤
         *      │MU-│    │CTS│    │PSDU 2│MU-BAR TF 2│    │BlockAck 2│
         *      │RTS│SIFS│   │SIFS├──────┼───────────┤SIFS├──────────┤
         *      │TF │    │x4 │    │PSDU 3│MU-BAR TF 3│    │BlockAck 3│
         *      │   │    │   │    ├──────┼───────────┤    ├──────────┤
         *      │   │    │   │    │PSDU 4│MU-BAR TF 4│    │BlockAck 4│
         * -----┴───┴────┴───┴────┴──────┴───────────┴────┴──────────┴───
         * From: AP       all            AP                    all
         *   To: all      AP             all                   AP
         */
        NS_TEST_ASSERT_MSG_GT_OR_EQ(m_txPsdus.size(), 31, "Expected at least 31 packets");

        // The last MPDU in each PSDU is a MU-BAR Trigger Frame
        for (auto& psdu : m_txPsdus[26].psduMap)
        {
            NS_TEST_EXPECT_MSG_EQ((*std::prev(psdu.second->end()))->GetHeader().IsTrigger(),
                                  true,
                                  "Expected an aggregated MU-BAR Trigger Frame");
        }

        // A first STA sends a Block Ack in a TB PPDU a SIFS after the reception of the DL MU PPDU
        NS_TEST_EXPECT_MSG_EQ((m_txPsdus[27].txVector.GetPreambleType() == m_tbPreamble &&
                               m_txPsdus[27].psduMap.size() == 1 &&
                               m_txPsdus[27].psduMap.begin()->second->GetHeader(0).IsBlockAck()),
                              true,
                              "Expected a Block Ack");
        tEnd = m_txPsdus[26].endTx;
        tStart = m_txPsdus[27].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "Block Ack in HE TB PPDU sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart,
                              tEnd + sifs + tolerance,
                              "Block Ack in HE TB PPDU sent too late");
        Time baNavEnd = m_txPsdus[27].endTx + m_txPsdus[27].psduMap.begin()->second->GetDuration();
        // navEnd <= baNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, baNavEnd, "Duration/ID in BlockAck frame is too short");
        NS_TEST_EXPECT_MSG_LT(baNavEnd, navEnd + tolerance, "Duration/ID in BlockAck is too long");
        if (m_txopLimit == 0)
        {
            NS_TEST_EXPECT_MSG_EQ(baNavEnd,
                                  m_txPsdus[27].endTx,
                                  "Expected null Duration/ID for BlockAck");
        }

        // A second STA sends a Block Ack in a TB PPDU a SIFS after the reception of the DL MU PPDU
        NS_TEST_EXPECT_MSG_EQ((m_txPsdus[28].txVector.GetPreambleType() == m_tbPreamble &&
                               m_txPsdus[28].psduMap.size() == 1 &&
                               m_txPsdus[28].psduMap.begin()->second->GetHeader(0).IsBlockAck()),
                              true,
                              "Expected a Block Ack");
        tStart = m_txPsdus[28].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "Block Ack in HE TB PPDU sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart,
                              tEnd + sifs + tolerance,
                              "Block Ack in HE TB PPDU sent too late");
        baNavEnd = m_txPsdus[28].endTx + m_txPsdus[28].psduMap.begin()->second->GetDuration();
        // navEnd <= baNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, baNavEnd, "Duration/ID in BlockAck frame is too short");
        NS_TEST_EXPECT_MSG_LT(baNavEnd, navEnd + tolerance, "Duration/ID in BlockAck is too long");
        if (m_txopLimit == 0)
        {
            NS_TEST_EXPECT_MSG_EQ(baNavEnd,
                                  m_txPsdus[28].endTx,
                                  "Expected null Duration/ID for BlockAck");
        }

        // A third STA sends a Block Ack in a TB PPDU a SIFS after the reception of the DL MU PPDU
        NS_TEST_EXPECT_MSG_EQ((m_txPsdus[29].txVector.GetPreambleType() == m_tbPreamble &&
                               m_txPsdus[29].psduMap.size() == 1 &&
                               m_txPsdus[29].psduMap.begin()->second->GetHeader(0).IsBlockAck()),
                              true,
                              "Expected a Block Ack");
        tStart = m_txPsdus[29].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "Block Ack in HE TB PPDU sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart,
                              tEnd + sifs + tolerance,
                              "Block Ack in HE TB PPDU sent too late");
        baNavEnd = m_txPsdus[29].endTx + m_txPsdus[29].psduMap.begin()->second->GetDuration();
        // navEnd <= baNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, baNavEnd, "Duration/ID in BlockAck frame is too short");
        NS_TEST_EXPECT_MSG_LT(baNavEnd, navEnd + tolerance, "Duration/ID in BlockAck is too long");
        if (m_txopLimit == 0)
        {
            NS_TEST_EXPECT_MSG_EQ(baNavEnd,
                                  m_txPsdus[29].endTx,
                                  "Expected null Duration/ID for BlockAck");
        }

        // A fourth STA sends a Block Ack in a TB PPDU a SIFS after the reception of the DL MU PPDU
        NS_TEST_EXPECT_MSG_EQ((m_txPsdus[30].txVector.GetPreambleType() == m_tbPreamble &&
                               m_txPsdus[30].psduMap.size() == 1 &&
                               m_txPsdus[30].psduMap.begin()->second->GetHeader(0).IsBlockAck()),
                              true,
                              "Expected a Block Ack");
        tStart = m_txPsdus[30].startTx;
        NS_TEST_EXPECT_MSG_LT(tEnd + sifs, tStart, "Block Ack in HE TB PPDU sent too early");
        NS_TEST_EXPECT_MSG_LT(tStart,
                              tEnd + sifs + tolerance,
                              "Block Ack in HE TB PPDU sent too late");
        baNavEnd = m_txPsdus[30].endTx + m_txPsdus[30].psduMap.begin()->second->GetDuration();
        // navEnd <= baNavEnd < navEnd + tolerance
        NS_TEST_EXPECT_MSG_LT_OR_EQ(navEnd, baNavEnd, "Duration/ID in BlockAck frame is too short");
        NS_TEST_EXPECT_MSG_LT(baNavEnd, navEnd + tolerance, "Duration/ID in BlockAck is too long");
        if (m_txopLimit == 0)
        {
            NS_TEST_EXPECT_MSG_EQ(baNavEnd,
                                  m_txPsdus[30].endTx,
                                  "Expected null Duration/ID for BlockAck");
        }

        nTxPsdus = 31;
    }

    NS_TEST_EXPECT_MSG_EQ(m_received,
                          m_nPktsPerSta * m_nStations - m_flushed,
                          "Not all DL packets have been received");

    if (m_muEdcaParameterSet.muTimer > 0 && m_muEdcaParameterSet.muAifsn == 0)
    {
        // EDCA disabled, find the first PSDU transmitted by a station not in an
        // HE TB PPDU and check that it was not transmitted before the MU EDCA
        // timer expired
        for (std::size_t i = nTxPsdus; i < m_txPsdus.size(); ++i)
        {
            if (m_txPsdus[i].psduMap.size() == 1 &&
                !m_txPsdus[i].psduMap.begin()->second->GetHeader(0).IsCts() &&
                m_txPsdus[i].psduMap.begin()->second->GetHeader(0).GetAddr2() !=
                    m_apDevice->GetAddress() &&
                !m_txPsdus[i].txVector.IsUlMu())
            {
                NS_TEST_EXPECT_MSG_GT_OR_EQ(
                    m_txPsdus[i].startTx.GetMicroSeconds(),
                    m_edcaDisabledStartTime.GetMicroSeconds() +
                        m_muEdcaParameterSet.muTimer * m_muTimerRes,
                    "A station transmitted before the MU EDCA timer expired");
                break;
            }
        }
    }
    else if (m_muEdcaParameterSet.muTimer > 0 && m_muEdcaParameterSet.muAifsn > 0)
    {
        // stations used worse access parameters after successful UL MU transmission
        for (const auto& cwValue : m_cwValues)
        {
            NS_TEST_EXPECT_MSG_EQ((cwValue == 2 || cwValue >= m_muEdcaParameterSet.muCwMin),
                                  true,
                                  "A station did not set the correct MU CW min");
        }
    }

    m_txPsdus.clear();
}

void
OfdmaAckSequenceTest::DoRun()
{
    uint32_t previousSeed = RngSeedManager::GetSeed();
    uint64_t previousRun = RngSeedManager::GetRun();
    Config::SetGlobal("RngSeed", UintegerValue(2));
    Config::SetGlobal("RngRun", UintegerValue(2));
    int64_t streamNumber = 10;

    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    NodeContainer wifiOldStaNodes;
    NodeContainer wifiNewStaNodes;
    wifiOldStaNodes.Create(m_nStations / 2);
    wifiNewStaNodes.Create(m_nStations - m_nStations / 2);
    NodeContainer wifiStaNodes(wifiOldStaNodes, wifiNewStaNodes);

    Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel>();
    spectrumChannel->AddPropagationLossModel(lossModel);
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);

    SpectrumWifiPhyHelper phy;
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.SetErrorRateModel("ns3::NistErrorRateModel");
    phy.SetChannel(spectrumChannel);
    switch (static_cast<uint16_t>(m_channelWidth))
    {
    case 20:
        phy.Set("ChannelSettings", StringValue("{36, 20, BAND_5GHZ, 0}"));
        break;
    case 40:
        phy.Set("ChannelSettings", StringValue("{38, 40, BAND_5GHZ, 0}"));
        break;
    case 80:
        phy.Set("ChannelSettings", StringValue("{42, 80, BAND_5GHZ, 0}"));
        break;
    case 160:
        phy.Set("ChannelSettings", StringValue("{50, 160, BAND_5GHZ, 0}"));
        break;
    default:
        NS_ABORT_MSG("Invalid channel bandwidth (must be 20, 40, 80 or 160)");
    }

    Config::SetDefault("ns3::HeConfiguration::MuBeAifsn",
                       UintegerValue(m_muEdcaParameterSet.muAifsn));
    Config::SetDefault("ns3::HeConfiguration::MuBeCwMin",
                       UintegerValue(m_muEdcaParameterSet.muCwMin));
    Config::SetDefault("ns3::HeConfiguration::MuBeCwMax",
                       UintegerValue(m_muEdcaParameterSet.muCwMax));
    Config::SetDefault("ns3::HeConfiguration::BeMuEdcaTimer",
                       TimeValue(MicroSeconds(8192 * m_muEdcaParameterSet.muTimer)));

    Config::SetDefault("ns3::HeConfiguration::MuBkAifsn",
                       UintegerValue(m_muEdcaParameterSet.muAifsn));
    Config::SetDefault("ns3::HeConfiguration::MuBkCwMin",
                       UintegerValue(m_muEdcaParameterSet.muCwMin));
    Config::SetDefault("ns3::HeConfiguration::MuBkCwMax",
                       UintegerValue(m_muEdcaParameterSet.muCwMax));
    Config::SetDefault("ns3::HeConfiguration::BkMuEdcaTimer",
                       TimeValue(MicroSeconds(8192 * m_muEdcaParameterSet.muTimer)));

    Config::SetDefault("ns3::HeConfiguration::MuViAifsn",
                       UintegerValue(m_muEdcaParameterSet.muAifsn));
    Config::SetDefault("ns3::HeConfiguration::MuViCwMin",
                       UintegerValue(m_muEdcaParameterSet.muCwMin));
    Config::SetDefault("ns3::HeConfiguration::MuViCwMax",
                       UintegerValue(m_muEdcaParameterSet.muCwMax));
    Config::SetDefault("ns3::HeConfiguration::ViMuEdcaTimer",
                       TimeValue(MicroSeconds(8192 * m_muEdcaParameterSet.muTimer)));

    Config::SetDefault("ns3::HeConfiguration::MuVoAifsn",
                       UintegerValue(m_muEdcaParameterSet.muAifsn));
    Config::SetDefault("ns3::HeConfiguration::MuVoCwMin",
                       UintegerValue(m_muEdcaParameterSet.muCwMin));
    Config::SetDefault("ns3::HeConfiguration::MuVoCwMax",
                       UintegerValue(m_muEdcaParameterSet.muCwMax));
    Config::SetDefault("ns3::HeConfiguration::VoMuEdcaTimer",
                       TimeValue(MicroSeconds(8192 * m_muEdcaParameterSet.muTimer)));

    // increase MSDU lifetime so that it does not expire before the MU EDCA timer ends
    Config::SetDefault("ns3::WifiMacQueue::MaxDelay", TimeValue(Seconds(2)));

    WifiHelper wifi;
    wifi.SetStandard(m_scenario == WifiOfdmaScenario::EHT ? WIFI_STANDARD_80211be
                                                          : WIFI_STANDARD_80211ax);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("HeMcs11"));
    wifi.ConfigHeOptions("MuBeAifsn",
                         UintegerValue(m_muEdcaParameterSet.muAifsn),
                         "MuBeCwMin",
                         UintegerValue(m_muEdcaParameterSet.muCwMin),
                         "MuBeCwMax",
                         UintegerValue(m_muEdcaParameterSet.muCwMax),
                         "BeMuEdcaTimer",
                         TimeValue(MicroSeconds(m_muTimerRes * m_muEdcaParameterSet.muTimer)),
                         // MU EDCA timers must be either all null or all non-null
                         "BkMuEdcaTimer",
                         TimeValue(MicroSeconds(m_muTimerRes * m_muEdcaParameterSet.muTimer)),
                         "ViMuEdcaTimer",
                         TimeValue(MicroSeconds(m_muTimerRes * m_muEdcaParameterSet.muTimer)),
                         "VoMuEdcaTimer",
                         TimeValue(MicroSeconds(m_muTimerRes * m_muEdcaParameterSet.muTimer)));

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");
    mac.SetType("ns3::StaWifiMac",
                "Ssid",
                SsidValue(ssid),
                "BE_MaxAmsduSize",
                UintegerValue(0),
                "BE_MaxAmpduSize",
                UintegerValue(m_maxAmpduSize),
                /* setting blockack threshold for sta's BE queue */
                "BE_BlockAckThreshold",
                UintegerValue(2),
                "BK_MaxAmsduSize",
                UintegerValue(0),
                "BK_MaxAmpduSize",
                UintegerValue(m_maxAmpduSize),
                /* setting blockack threshold for sta's BK queue */
                "BK_BlockAckThreshold",
                UintegerValue(2),
                "VI_MaxAmsduSize",
                UintegerValue(0),
                "VI_MaxAmpduSize",
                UintegerValue(m_maxAmpduSize),
                /* setting blockack threshold for sta's VI queue */
                "VI_BlockAckThreshold",
                UintegerValue(2),
                "VO_MaxAmsduSize",
                UintegerValue(0),
                "VO_MaxAmpduSize",
                UintegerValue(m_maxAmpduSize),
                /* setting blockack threshold for sta's VO queue */
                "VO_BlockAckThreshold",
                UintegerValue(2),
                "ActiveProbing",
                BooleanValue(false));

    m_staDevices = wifi.Install(phy, mac, wifiOldStaNodes);

    wifi.SetStandard(m_scenario == WifiOfdmaScenario::HE ? WIFI_STANDARD_80211ax
                                                         : WIFI_STANDARD_80211be);
    m_staDevices = NetDeviceContainer(m_staDevices, wifi.Install(phy, mac, wifiNewStaNodes));

    // create a listening VHT station
    wifi.SetStandard(WIFI_STANDARD_80211ac);
    wifi.Install(phy, mac, Create<Node>());

    wifi.SetStandard(m_scenario == WifiOfdmaScenario::HE ? WIFI_STANDARD_80211ax
                                                         : WIFI_STANDARD_80211be);

    mac.SetType("ns3::ApWifiMac", "BeaconGeneration", BooleanValue(true));
    mac.SetMultiUserScheduler(
        "ns3::TestMultiUserScheduler",
        "ModulationClass",
        EnumValue(m_scenario == WifiOfdmaScenario::EHT ? WIFI_MOD_CLASS_EHT : WIFI_MOD_CLASS_HE),
        // request channel access at 1.5s
        "AccessReqInterval",
        TimeValue(Seconds(1.5)),
        "DelayAccessReqUponAccess",
        BooleanValue(false),
        "DefaultTbPpduDuration",
        TimeValue(m_defaultTbPpduDuration));
    mac.SetProtectionManager("ns3::WifiDefaultProtectionManager",
                             "EnableMuRts",
                             BooleanValue(true),
                             "SkipMuRtsBeforeBsrp",
                             BooleanValue(m_skipMuRtsBeforeBsrp));
    mac.SetAckManager("ns3::WifiDefaultAckManager",
                      "DlMuAckSequenceType",
                      EnumValue(m_dlMuAckType));
    mac.SetFrameExchangeManager("ProtectedIfResponded",
                                BooleanValue(m_protectedIfResponded),
                                "ContinueTxopAfterBsrp",
                                BooleanValue(m_continueTxopAfterBsrp));

    m_apDevice = DynamicCast<WifiNetDevice>(wifi.Install(phy, mac, wifiApNode).Get(0));

    // Assign fixed streams to random variables in use
    streamNumber += WifiHelper::AssignStreams(NetDeviceContainer(m_apDevice), streamNumber);
    streamNumber += WifiHelper::AssignStreams(m_staDevices, streamNumber);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));
    positionAlloc->Add(Vector(0.0, 1.0, 0.0));
    positionAlloc->Add(Vector(-1.0, 0.0, 0.0));
    positionAlloc->Add(Vector(-1.0, -1.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);

    NetDeviceContainer allDevices(NetDeviceContainer(m_apDevice), m_staDevices);
    for (uint32_t i = 0; i < allDevices.GetN(); i++)
    {
        auto dev = DynamicCast<WifiNetDevice>(allDevices.Get(i));
        // set the same TXOP limit on all ACs
        dev->GetMac()->GetQosTxop(AC_BE)->SetTxopLimit(MicroSeconds(m_txopLimit));
        dev->GetMac()->GetQosTxop(AC_BK)->SetTxopLimit(MicroSeconds(m_txopLimit));
        dev->GetMac()->GetQosTxop(AC_VI)->SetTxopLimit(MicroSeconds(m_txopLimit));
        dev->GetMac()->GetQosTxop(AC_VO)->SetTxopLimit(MicroSeconds(m_txopLimit));
        // set the same AIFSN on all ACs (just to be able to check inter-frame spaces)
        dev->GetMac()->GetQosTxop(AC_BE)->SetAifsn(3);
        dev->GetMac()->GetQosTxop(AC_BK)->SetAifsn(3);
        dev->GetMac()->GetQosTxop(AC_VI)->SetAifsn(3);
        dev->GetMac()->GetQosTxop(AC_VO)->SetAifsn(3);
    }

    PacketSocketHelper packetSocket;
    packetSocket.Install(wifiApNode);
    packetSocket.Install(wifiStaNodes);

    // DL Traffic
    for (uint16_t i = 0; i < m_nStations; i++)
    {
        PacketSocketAddress socket;
        socket.SetSingleDevice(m_apDevice->GetIfIndex());
        socket.SetPhysicalAddress(m_staDevices.Get(i)->GetAddress());
        socket.SetProtocol(1);

        // the first client application generates two packets in order
        // to trigger the establishment of a Block Ack agreement
        Ptr<PacketSocketClient> client1 = CreateObject<PacketSocketClient>();
        client1->SetAttribute("PacketSize", UintegerValue(1400));
        client1->SetAttribute("MaxPackets", UintegerValue(2));
        client1->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
        client1->SetAttribute("Priority", UintegerValue(i * 2)); // 0, 2, 4 and 6
        client1->SetRemote(socket);
        wifiApNode.Get(0)->AddApplication(client1);
        client1->SetStartTime(Seconds(1) + i * MilliSeconds(1));
        client1->SetStopTime(Seconds(2));

        // the second client application generates the selected number of packets,
        // which are sent in DL MU PPDUs.
        Ptr<PacketSocketClient> client2 = CreateObject<PacketSocketClient>();
        client2->SetAttribute("PacketSize", UintegerValue(1400 + i * 100));
        client2->SetAttribute("MaxPackets", UintegerValue(m_nPktsPerSta));
        client2->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
        client2->SetAttribute("Priority", UintegerValue(i * 2)); // 0, 2, 4 and 6
        client2->SetRemote(socket);
        wifiApNode.Get(0)->AddApplication(client2);
        client2->SetStartTime(Seconds(1.5003));
        client2->SetStopTime(Seconds(2.5));

        Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer>();
        server->SetLocal(socket);
        wifiStaNodes.Get(i)->AddApplication(server);
        server->SetStartTime(Seconds(0));
        server->SetStopTime(Seconds(3));
    }

    // UL Traffic
    for (uint16_t i = 0; i < m_nStations; i++)
    {
        m_sockets[i].SetSingleDevice(m_staDevices.Get(i)->GetIfIndex());
        m_sockets[i].SetPhysicalAddress(m_apDevice->GetAddress());
        m_sockets[i].SetProtocol(1);

        // the first client application generates two packets in order
        // to trigger the establishment of a Block Ack agreement
        Ptr<PacketSocketClient> client1 = CreateObject<PacketSocketClient>();
        client1->SetAttribute("PacketSize", UintegerValue(1400));
        client1->SetAttribute("MaxPackets", UintegerValue(2));
        client1->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
        client1->SetAttribute("Priority", UintegerValue(i * 2)); // 0, 2, 4 and 6
        client1->SetRemote(m_sockets[i]);
        wifiStaNodes.Get(i)->AddApplication(client1);
        client1->SetStartTime(Seconds(1.005) + i * MilliSeconds(1));
        client1->SetStopTime(Seconds(2));

        // packets to be included in HE TB PPDUs are generated (by Transmit()) when
        // the first Basic Trigger Frame is sent by the AP

        Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer>();
        server->SetLocal(m_sockets[i]);
        wifiApNode.Get(0)->AddApplication(server);
        server->SetStartTime(Seconds(0));
        server->SetStopTime(Seconds(3));
    }

    Config::Connect("/NodeList/*/ApplicationList/0/$ns3::PacketSocketServer/Rx",
                    MakeCallback(&OfdmaAckSequenceTest::L7Receive, this));
    // Trace PSDUs passed to the PHY on all devices
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxPsduBegin",
                    MakeCallback(&OfdmaAckSequenceTest::Transmit, this));

    Simulator::Stop(Seconds(3));
    Simulator::Run();

    CheckResults(m_apDevice->GetMac()->GetWifiPhy()->GetSifs(),
                 m_apDevice->GetMac()->GetWifiPhy()->GetSlot(),
                 m_apDevice->GetMac()->GetQosTxop(AC_BE)->Txop::GetAifsn());

    Simulator::Destroy();

    // Restore the seed and run number that were in effect before this test
    Config::SetGlobal("RngSeed", UintegerValue(previousSeed));
    Config::SetGlobal("RngRun", UintegerValue(previousRun));
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi MAC OFDMA Test Suite
 */
class WifiMacOfdmaTestSuite : public TestSuite
{
  public:
    WifiMacOfdmaTestSuite();
};

WifiMacOfdmaTestSuite::WifiMacOfdmaTestSuite()
    : TestSuite("wifi-mac-ofdma", Type::UNIT)
{
    using MuEdcaParams = std::initializer_list<OfdmaAckSequenceTest::MuEdcaParameterSet>;

    for (auto& muEdcaParameterSet : MuEdcaParams{{0, 0, 0, 0} /* no MU EDCA */,
                                                 {0, 127, 2047, 100} /* EDCA disabled */,
                                                 {10, 127, 2047, 100} /* worse parameters */})
    {
        for (const auto scenario :
             {WifiOfdmaScenario::HE, WifiOfdmaScenario::HE_EHT, WifiOfdmaScenario::EHT})
        {
            AddTestCase(new OfdmaAckSequenceTest(
                            {.channelWidth = MHz_u{20},
                             .dlMuAckType = WifiAcknowledgment::DL_MU_BAR_BA_SEQUENCE,
                             .maxAmpduSize = 10000,
                             .txopLimit = 5632,
                             .continueTxopAfterBsrp = false, // unused because non-zero TXOP limit
                             .skipMuRtsBeforeBsrp = true,
                             .protectedIfResponded = false,
                             .nPktsPerSta = 15,
                             .muEdcaParameterSet = muEdcaParameterSet,
                             .scenario = scenario}),
                        TestCase::Duration::QUICK);
            AddTestCase(new OfdmaAckSequenceTest(
                            {.channelWidth = MHz_u{20},
                             .dlMuAckType = WifiAcknowledgment::DL_MU_AGGREGATE_TF,
                             .maxAmpduSize = 10000,
                             .txopLimit = 5632,
                             .continueTxopAfterBsrp = false, // unused because non-zero TXOP limit
                             .skipMuRtsBeforeBsrp = false,
                             .protectedIfResponded = false,
                             .nPktsPerSta = 15,
                             .muEdcaParameterSet = muEdcaParameterSet,
                             .scenario = scenario}),
                        TestCase::Duration::QUICK);
            AddTestCase(new OfdmaAckSequenceTest(
                            {.channelWidth = MHz_u{20},
                             .dlMuAckType = WifiAcknowledgment::DL_MU_TF_MU_BAR,
                             .maxAmpduSize = 10000,
                             .txopLimit = 5632,
                             .continueTxopAfterBsrp = false, // unused because non-zero TXOP limit
                             .skipMuRtsBeforeBsrp = true,
                             .protectedIfResponded = true,
                             .nPktsPerSta = 15,
                             .muEdcaParameterSet = muEdcaParameterSet,
                             .scenario = scenario}),
                        TestCase::Duration::QUICK);
            AddTestCase(
                new OfdmaAckSequenceTest({.channelWidth = MHz_u{40},
                                          .dlMuAckType = WifiAcknowledgment::DL_MU_BAR_BA_SEQUENCE,
                                          .maxAmpduSize = 10000,
                                          .txopLimit = 0,
                                          .continueTxopAfterBsrp = true,
                                          .skipMuRtsBeforeBsrp = false,
                                          .protectedIfResponded = false,
                                          .nPktsPerSta = 15,
                                          .muEdcaParameterSet = muEdcaParameterSet,
                                          .scenario = scenario}),
                TestCase::Duration::QUICK);
            AddTestCase(
                new OfdmaAckSequenceTest({.channelWidth = MHz_u{40},
                                          .dlMuAckType = WifiAcknowledgment::DL_MU_AGGREGATE_TF,
                                          .maxAmpduSize = 10000,
                                          .txopLimit = 0,
                                          .continueTxopAfterBsrp = false,
                                          .skipMuRtsBeforeBsrp = true,
                                          .protectedIfResponded = false,
                                          .nPktsPerSta = 15,
                                          .muEdcaParameterSet = muEdcaParameterSet,
                                          .scenario = scenario}),
                TestCase::Duration::QUICK);
            AddTestCase(
                new OfdmaAckSequenceTest({.channelWidth = MHz_u{40},
                                          .dlMuAckType = WifiAcknowledgment::DL_MU_TF_MU_BAR,
                                          .maxAmpduSize = 10000,
                                          .txopLimit = 0,
                                          .continueTxopAfterBsrp = true,
                                          .skipMuRtsBeforeBsrp = false,
                                          .protectedIfResponded = true,
                                          .nPktsPerSta = 15,
                                          .muEdcaParameterSet = muEdcaParameterSet,
                                          .scenario = scenario}),
                TestCase::Duration::QUICK);
        }
    }
}

static WifiMacOfdmaTestSuite g_wifiMacOfdmaTestSuite; ///< the test suite
