/*
 * Copyright (c) 2023 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "wifi-gcr-test.h"

#include "ns3/ampdu-subframe-header.h"
#include "ns3/attribute-container.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/ctrl-headers.h"
#include "ns3/he-configuration.h"
#include "ns3/ht-configuration.h"
#include "ns3/ideal-wifi-manager.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/mgt-action-headers.h"
#include "ns3/mgt-headers.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/node-list.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/pointer.h"
#include "ns3/qos-txop.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/rr-multi-user-scheduler.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/string.h"
#include "ns3/vht-configuration.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-ppdu.h"
#include "ns3/wifi-psdu.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/yans-wifi-phy.h"

#include <algorithm>
#include <functional>
#include <iomanip>
#include <iterator>
#include <limits>
#include <numeric>

using namespace ns3;

namespace
{

/**
 * Get the number of GCR STAs.
 *
 * @param stas information about all STAs of the test
 * @return the number of GCR STAs
 */
std::size_t
GetNumGcrStas(const std::vector<GcrTestBase::StaInfo>& stas)
{
    return std::count_if(stas.cbegin(), stas.cend(), [](const auto& staInfo) {
        return (staInfo.gcrCapable);
    });
}

/**
 * Get the number of non-GCR STAs.
 *
 * @param stas information about all STAs of the test
 * @return the number of non-GCR STAs
 */
std::size_t
GetNumNonGcrStas(const std::vector<GcrTestBase::StaInfo>& stas)
{
    return stas.size() - GetNumGcrStas(stas);
}

/**
 * Get the number of non-HT STAs.
 *
 * @param stas information about all STAs of the test
 * @return the number of non-HT STAs
 */
std::size_t
GetNumNonHtStas(const std::vector<GcrTestBase::StaInfo>& stas)
{
    return std::count_if(stas.cbegin(), stas.cend(), [](const auto& staInfo) {
        return staInfo.standard < WIFI_STANDARD_80211n;
    });
}

/**
 * Lambda to print stations information.
 */
auto printStasInfo = [](const std::vector<GcrTestBase::StaInfo>& v) {
    std::stringstream ss;
    std::size_t index = 0;
    ss << "{";
    for (const auto& staInfo : v)
    {
        ss << "STA" << ++index << ": GCRcapable=" << staInfo.gcrCapable
           << " standard=" << staInfo.standard << " maxBw=" << staInfo.maxChannelWidth
           << " maxNss=" << +staInfo.maxNumStreams << " minGi=" << staInfo.minGi << "; ";
    }
    ss << "}";
    return ss.str();
};

/**
 * Get the node ID from the context string.
 *
 * @param context the context string
 * @return the corresponding node ID
 */
uint32_t
ConvertContextToNodeId(const std::string& context)
{
    auto sub = context.substr(10);
    auto pos = sub.find("/Device");
    return std::stoi(sub.substr(0, pos));
}

/**
 * Get the maximum number of groupcast MPDUs that can be in flight.
 *
 * @param maxNumMpdus the configured maximum number of MPDUs
 * @param stas information about all STAs of the test
 * @return the maximum number of groupcast MPDUs that can be in flight
 */
uint16_t
GetGcrMaxNumMpdus(uint16_t maxNumMpdus, const std::vector<GcrTestBase::StaInfo>& stas)
{
    uint16_t limit = 1024;
    for (const auto& staInfo : stas)
    {
        if (staInfo.standard < WIFI_STANDARD_80211ax)
        {
            limit = 64;
            break;
        }
        if (staInfo.standard < WIFI_STANDARD_80211be)
        {
            limit = 256;
        }
    }
    return std::min(limit, maxNumMpdus);
}

constexpr uint16_t MULTICAST_PROTOCOL{1}; ///< protocol to create socket for multicast
constexpr uint16_t UNICAST_PROTOCOL{2};   ///< protocol to create socket for unicast

constexpr uint32_t maxRtsCtsThreshold{4692480}; ///< maximum value for RTS/CTS threshold

constexpr bool GCR_CAPABLE_STA{true};    ///< STA that is GCR capable
constexpr bool GCR_INCAPABLE_STA{false}; ///< STA that is not GCR capable

} // namespace

/**
 * Extended IdealWifiManager class for the purpose of the tests.
 */
class IdealWifiManagerForGcrTest : public IdealWifiManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::IdealWifiManagerForGcrTest")
                                .SetParent<IdealWifiManager>()
                                .SetGroupName("Wifi")
                                .AddConstructor<IdealWifiManagerForGcrTest>();
        return tid;
    }

    void DoReportAmpduTxStatus(WifiRemoteStation* station,
                               uint16_t nSuccessfulMpdus,
                               uint16_t nFailedMpdus,
                               double rxSnr,
                               double dataSnr,
                               MHz_u dataChannelWidth,
                               uint8_t dataNss) override
    {
        m_blockAckSenders.insert(station->m_state->m_address);
        IdealWifiManager::DoReportAmpduTxStatus(station,
                                                nSuccessfulMpdus,
                                                nFailedMpdus,
                                                rxSnr,
                                                dataSnr,
                                                dataChannelWidth,
                                                dataNss);
    }

    GcrManager::GcrMembers m_blockAckSenders; ///< hold set of BACK senders that have passed
                                              ///< success/failure infos to RSM
};

NS_OBJECT_ENSURE_REGISTERED(IdealWifiManagerForGcrTest);

NS_LOG_COMPONENT_DEFINE("WifiGcrTest");

GcrTestBase::GcrTestBase(const std::string& testName, const GcrParameters& params)
    : TestCase(testName),
      m_testName{testName},
      m_params{params},
      m_expectGcrUsed{(GetNumGcrStas(params.stas) > 0)},
      m_expectedMaxNumMpdusInPsdu(
          m_expectGcrUsed ? GetGcrMaxNumMpdus(m_params.maxNumMpdusInPsdu, params.stas) : 1U),
      m_packets{0},
      m_nTxApRts{0},
      m_nTxApCts{0},
      m_totalTx{0},
      m_nTxGroupcastInCurrentTxop{0},
      m_nTxRtsInCurrentTxop{0},
      m_nTxCtsInCurrentTxop{0},
      m_nTxAddbaReq{0},
      m_nTxAddbaResp{0},
      m_nTxDelba{0},
      m_nTxGcrAddbaReq{0},
      m_nTxGcrAddbaResp{0},
      m_nTxGcrDelba{0}
{
    m_params.maxNumMpdusInPsdu = m_expectGcrUsed ? params.maxNumMpdusInPsdu : 1U;
}

void
GcrTestBase::PacketGenerated(std::string context, Ptr<const Packet> p, const Address& addr)
{
    m_packets++;
    if ((m_packets % m_expectedMaxNumMpdusInPsdu) == 0)
    {
        m_groupcastClient->SetAttribute("Interval", TimeValue(MilliSeconds(10)));
    }
    else
    {
        m_groupcastClient->SetAttribute("Interval", TimeValue(Seconds(0)));
    }
}

void
GcrTestBase::Transmit(std::string context,
                      WifiConstPsduMap psduMap,
                      WifiTxVector txVector,
                      double txPowerW)
{
    auto psdu = psduMap.cbegin()->second;
    auto mpdu = *psdu->begin();
    auto addr1 = mpdu->GetHeader().GetAddr1();
    const auto nodeId = ConvertContextToNodeId(context);
    if (addr1.IsGroup() && !addr1.IsBroadcast() && mpdu->GetHeader().IsQosData())
    {
        const auto expectedChannelWidth =
            std::min_element(m_params.stas.cbegin(),
                             m_params.stas.cend(),
                             [](const auto& sta1, const auto& sta2) {
                                 return sta1.maxChannelWidth < sta2.maxChannelWidth;
                             })
                ->maxChannelWidth;
        NS_TEST_EXPECT_MSG_EQ(txVector.GetChannelWidth(),
                              expectedChannelWidth,
                              "Incorrect channel width for groupcast frame");
        const auto expectedNss =
            std::min_element(m_params.stas.cbegin(),
                             m_params.stas.cend(),
                             [](const auto& sta1, const auto& sta2) {
                                 return sta1.maxNumStreams < sta2.maxNumStreams;
                             })
                ->maxNumStreams;
        const auto expectedGi = std::max_element(m_params.stas.cbegin(),
                                                 m_params.stas.cend(),
                                                 [](const auto& sta1, const auto& sta2) {
                                                     return sta1.minGi < sta2.minGi;
                                                 })
                                    ->minGi;
        NS_TEST_EXPECT_MSG_EQ(+txVector.GetNss(),
                              +expectedNss,
                              "Incorrect number of spatial streams for groupcast frame");
        NS_TEST_EXPECT_MSG_EQ(txVector.GetGuardInterval(),
                              expectedGi,
                              "Incorrect guard interval duration for groupcast frame");
        Mac48Address expectedGroupAddress{"01:00:5e:40:64:01"};
        Mac48Address groupConcealmentAddress{"01:0F:AC:47:43:52"};
        const auto expectConcealmentUsed =
            m_expectGcrUsed &&
            (GetNumNonGcrStas(m_params.stas) == 0 || mpdu->GetHeader().IsRetry());
        std::vector<StaInfo> addressedStas{};
        if (!expectConcealmentUsed)
        {
            std::copy_if(m_params.stas.cbegin(),
                         m_params.stas.cend(),
                         std::back_inserter(addressedStas),
                         [](const auto& sta) { return !sta.gcrCapable; });
        }
        else
        {
            std::copy_if(m_params.stas.cbegin(),
                         m_params.stas.cend(),
                         std::back_inserter(addressedStas),
                         [](const auto& sta) { return sta.gcrCapable; });
        }
        NS_ASSERT(!addressedStas.empty());
        const auto minStandard = std::min_element(addressedStas.cbegin(),
                                                  addressedStas.cend(),
                                                  [](const auto& sta1, const auto& sta2) {
                                                      return sta1.standard < sta2.standard;
                                                  })
                                     ->standard;
        const auto expectedModulationClass = GetModulationClassForStandard(minStandard);
        NS_TEST_EXPECT_MSG_EQ(txVector.GetModulationClass(),
                              expectedModulationClass,
                              "Incorrect modulation class for groupcast frame");
        NS_TEST_EXPECT_MSG_EQ(
            addr1,
            (expectConcealmentUsed ? groupConcealmentAddress : expectedGroupAddress),
            "Unexpected address1");
        NS_TEST_EXPECT_MSG_EQ(mpdu->GetHeader().IsQosAmsdu(),
                              expectConcealmentUsed,
                              "MSDU aggregation should " << (expectConcealmentUsed ? "" : "not ")
                                                         << "be used");
        if (mpdu->GetHeader().IsQosAmsdu())
        {
            const auto numAmsduSubframes = std::distance(mpdu->begin(), mpdu->end());
            NS_TEST_EXPECT_MSG_EQ(
                numAmsduSubframes,
                1,
                "Only one A-MSDU subframe should be used in concealed group addressed frames");
            NS_TEST_EXPECT_MSG_EQ(mpdu->begin()->second.GetDestinationAddr(),
                                  expectedGroupAddress,
                                  "Unexpected DA field in A-MSDU subframe");
        }
        m_totalTx++;
        if (const auto it = m_params.mpdusToCorruptPerPsdu.find(m_totalTx);
            it != m_params.mpdusToCorruptPerPsdu.cend())
        {
            std::map<uint8_t, std::list<uint64_t>> uidListPerSta{};
            uint8_t numStas = m_params.stas.size();
            for (uint8_t i = 0; i < numStas; ++i)
            {
                uidListPerSta.insert({i, {}});
            }
            for (std::size_t i = 0; i < psdu->GetNMpdus(); ++i)
            {
                for (uint8_t staId = 0; staId < numStas; ++staId)
                {
                    const auto& corruptedMpdusForSta =
                        (it->second.count(0) != 0)
                            ? it->second.at(0)
                            : ((it->second.count(staId + 1) != 0) ? it->second.at(staId + 1)
                                                                  : std::set<uint8_t>{});
                    auto corruptIndex = (m_apWifiMac->GetGcrManager()->GetRetransmissionPolicy() ==
                                         GroupAddressRetransmissionPolicy::GCR_BLOCK_ACK)
                                            ? psdu->GetHeader(i).GetSequenceNumber()
                                            : i;
                    if (std::find(corruptedMpdusForSta.cbegin(),
                                  corruptedMpdusForSta.cend(),
                                  corruptIndex + 1) != std::end(corruptedMpdusForSta))
                    {
                        NS_LOG_INFO("STA " << staId + 1 << ": corrupted MPDU #" << i + 1 << " (seq="
                                           << psdu->GetHeader(i).GetSequenceNumber() << ")"
                                           << " for frame #" << +m_totalTx);
                        uidListPerSta.at(staId).push_back(psdu->GetAmpduSubframe(i)->GetUid());
                    }
                    else
                    {
                        NS_LOG_INFO("STA " << staId + 1 << ": uncorrupted MPDU #" << i + 1
                                           << " (seq=" << psdu->GetHeader(i).GetSequenceNumber()
                                           << ")"
                                           << " for frame #" << +m_totalTx);
                    }
                }
            }
            for (uint8_t staId = 0; staId < numStas; ++staId)
            {
                m_errorModels.at(staId)->SetList(uidListPerSta.at(staId));
            }
        }
        else
        {
            NS_LOG_INFO("Do not corrupt frame #" << +m_totalTx);
            for (auto& errorModel : m_errorModels)
            {
                errorModel->SetList({});
            }
        }
    }
    else if (mpdu->GetHeader().IsRts())
    {
        const auto isGroupcast = (m_params.numUnicastPackets == 0) ||
                                 ((m_params.startUnicast < m_params.startGroupcast) &&
                                  (Simulator::Now() > m_params.startGroupcast)) ||
                                 ((m_params.startGroupcast < m_params.startUnicast) &&
                                  (Simulator::Now() < m_params.startUnicast));
        if (isGroupcast)
        {
            NS_TEST_EXPECT_MSG_EQ(nodeId, 0, "STAs are not expected to send RTS frames");
            NS_LOG_INFO("AP: start protection and initiate RTS-CTS");
            NS_TEST_EXPECT_MSG_EQ(mpdu->GetHeader().GetAddr2(),
                                  m_apWifiMac->GetAddress(),
                                  "Incorrect Address2 set for RTS frame");
            const auto it = std::find_if(m_stasWifiMac.cbegin(),
                                         m_stasWifiMac.cend(),
                                         [addr = mpdu->GetHeader().GetAddr1()](const auto& mac) {
                                             return addr == mac->GetAddress();
                                         });
            NS_TEST_EXPECT_MSG_EQ((it != m_stasWifiMac.cend()),
                                  true,
                                  "Incorrect Address1 set for RTS frame");
            m_nTxApRts++;
            if (m_params.rtsFramesToCorrupt.count(m_nTxApRts))
            {
                NS_LOG_INFO("Corrupt RTS frame #" << +m_nTxApRts);
                const auto uid = mpdu->GetPacket()->GetUid();
                for (auto& errorModel : m_errorModels)
                {
                    errorModel->SetList({uid});
                }
            }
            else
            {
                NS_LOG_INFO("Do not corrupt RTS frame #" << +m_nTxApRts);
                for (auto& errorModel : m_errorModels)
                {
                    errorModel->SetList({});
                }
            }
        }
    }
    else if (mpdu->GetHeader().IsCts())
    {
        const auto isGroupcast = (m_params.numUnicastPackets == 0) ||
                                 ((m_params.startUnicast < m_params.startGroupcast) &&
                                  (Simulator::Now() > m_params.startGroupcast)) ||
                                 ((m_params.startGroupcast < m_params.startUnicast) &&
                                  (Simulator::Now() < m_params.startUnicast));
        if (isGroupcast)
        {
            if (nodeId == 0)
            {
                NS_LOG_INFO("AP: start protection and initiate CTS-to-self");
                NS_TEST_EXPECT_MSG_EQ(mpdu->GetHeader().GetAddr1(),
                                      m_apWifiMac->GetAddress(),
                                      "Incorrect Address1 set for CTS-to-self frame");
                m_nTxApCts++;
            }
            else
            {
                const auto staId = nodeId - 1;
                NS_LOG_INFO("STA" << staId + 1 << ": send CTS response");
                NS_TEST_EXPECT_MSG_EQ(mpdu->GetHeader().GetAddr1(),
                                      m_apWifiMac->GetAddress(),
                                      "Incorrect Address1 set for CTS frame");
                m_txCtsPerSta.at(staId)++;
                if (const auto it = m_params.ctsFramesToCorrupt.find(m_txCtsPerSta.at(staId));
                    it != m_params.ctsFramesToCorrupt.cend())
                {
                    NS_LOG_INFO("Corrupt CTS frame #" << +m_txCtsPerSta.at(staId));
                    const auto uid = mpdu->GetPacket()->GetUid();
                    m_apErrorModel->SetList({uid});
                }
                else
                {
                    NS_LOG_INFO("Do not corrupt CTS frame #" << +m_txCtsPerSta.at(staId));
                    m_apErrorModel->SetList({});
                }
            }
        }
    }
    else if (mpdu->GetHeader().IsAction())
    {
        WifiActionHeader actionHdr;
        Ptr<Packet> packet = mpdu->GetPacket()->Copy();
        packet->RemoveHeader(actionHdr);
        auto [category, action] = WifiActionHeader::Peek(mpdu->GetPacket());
        if (category == WifiActionHeader::BLOCK_ACK)
        {
            Mac48Address expectedGroupAddress{"01:00:5e:40:64:01"};
            if (action.blockAck == WifiActionHeader::BLOCK_ACK_ADDBA_REQUEST)
            {
                MgtAddBaRequestHeader reqHdr;
                packet->RemoveHeader(reqHdr);
                const auto isGcr = reqHdr.GetGcrGroupAddress().has_value();
                NS_LOG_INFO("AP: send " << (isGcr ? "GCR " : "") << "ADDBA request");
                const auto expectedGcr =
                    m_expectGcrUsed && ((m_params.numUnicastPackets == 0) ||
                                        ((m_params.startUnicast < m_params.startGroupcast) &&
                                         (Simulator::Now() > m_params.startGroupcast)) ||
                                        ((m_params.startGroupcast < m_params.startUnicast) &&
                                         (Simulator::Now() < m_params.startUnicast)));
                NS_TEST_EXPECT_MSG_EQ(isGcr,
                                      expectedGcr,
                                      "GCR address should "
                                          << (expectedGcr ? "" : "not ")
                                          << "be set in ADDBA request sent from AP");
                if (isGcr)
                {
                    m_nTxGcrAddbaReq++;
                    NS_TEST_EXPECT_MSG_EQ(reqHdr.GetGcrGroupAddress().value(),
                                          expectedGroupAddress,
                                          "Incorrect GCR address in ADDBA request sent from AP");
                    if (m_params.addbaReqsToCorrupt.count(m_nTxGcrAddbaReq))
                    {
                        NS_LOG_INFO("Corrupt ADDBA request #" << +m_nTxGcrAddbaReq);
                        const auto uid = mpdu->GetPacket()->GetUid();
                        for (auto& errorModel : m_errorModels)
                        {
                            errorModel->SetList({uid});
                        }
                    }
                    else
                    {
                        NS_LOG_INFO("Do not corrupt ADDBA request #" << +m_nTxGcrAddbaReq);
                        for (auto& errorModel : m_errorModels)
                        {
                            errorModel->SetList({});
                        }
                    }
                }
                else
                {
                    m_nTxAddbaReq++;
                }
            }
            else if (action.blockAck == WifiActionHeader::BLOCK_ACK_ADDBA_RESPONSE)
            {
                MgtAddBaResponseHeader respHdr;
                packet->RemoveHeader(respHdr);
                const auto isGcr = respHdr.GetGcrGroupAddress().has_value();
                NS_LOG_INFO("STA" << nodeId << ": send " << (isGcr ? "GCR " : "")
                                  << "ADDBA response");
                const auto expectedGcr =
                    m_expectGcrUsed && ((m_params.numUnicastPackets == 0) ||
                                        ((m_params.startUnicast < m_params.startGroupcast) &&
                                         (Simulator::Now() > m_params.startGroupcast)) ||
                                        ((m_params.startGroupcast < m_params.startUnicast) &&
                                         (Simulator::Now() < m_params.startUnicast)));
                NS_TEST_EXPECT_MSG_EQ(isGcr,
                                      expectedGcr,
                                      "GCR address should "
                                          << (expectedGcr ? "" : "not ")
                                          << "be set in ADDBA response sent from STA " << nodeId);
                if (isGcr)
                {
                    m_nTxGcrAddbaResp++;
                    NS_TEST_EXPECT_MSG_EQ(respHdr.GetGcrGroupAddress().value(),
                                          expectedGroupAddress,
                                          "Incorrect GCR address in ADDBA request sent from STA "
                                              << nodeId);
                    if (const auto it = m_params.addbaRespsToCorrupt.find(m_nTxGcrAddbaResp);
                        it != m_params.addbaRespsToCorrupt.cend())
                    {
                        NS_LOG_INFO("Corrupt ADDBA response #" << +m_nTxGcrAddbaResp);
                        const auto uid = mpdu->GetPacket()->GetUid();
                        m_apErrorModel->SetList({uid});
                    }
                    else
                    {
                        NS_LOG_INFO("Do not corrupt ADDBA response #" << +m_nTxGcrAddbaResp);
                        m_apErrorModel->SetList({});
                    }
                }
                else
                {
                    m_nTxAddbaResp++;
                }
            }
            else if (action.blockAck == WifiActionHeader::BLOCK_ACK_DELBA)
            {
                MgtDelBaHeader delbaHdr;
                packet->RemoveHeader(delbaHdr);
                const auto isGcr = delbaHdr.GetGcrGroupAddress().has_value();
                NS_LOG_INFO("AP: send " << (isGcr ? "GCR " : "") << "DELBA frame");
                const auto expectedGcr =
                    m_expectGcrUsed && ((m_params.numUnicastPackets == 0) ||
                                        ((m_params.startUnicast < m_params.startGroupcast) &&
                                         (Simulator::Now() > m_params.startGroupcast)) ||
                                        ((m_params.startGroupcast < m_params.startUnicast) &&
                                         (Simulator::Now() < m_params.startUnicast)));
                NS_TEST_EXPECT_MSG_EQ(isGcr,
                                      expectedGcr,
                                      "GCR address should "
                                          << (expectedGcr ? "" : "not ")
                                          << "be set in DELBA frame sent from AP");
                if (isGcr)
                {
                    m_nTxGcrDelba++;
                }
                else
                {
                    m_nTxDelba++;
                }
            }
        }
    }
}

bool
GcrTestBase::IsUsingAmpduOrSmpdu() const
{
    return ((m_params.maxNumMpdusInPsdu > 1) ||
            std::none_of(m_params.stas.cbegin(), m_params.stas.cend(), [](const auto& staInfo) {
                return (staInfo.standard < WIFI_STANDARD_80211ac);
            }));
}

void
GcrTestBase::PhyRx(std::string context,
                   Ptr<const Packet> p,
                   double snr,
                   WifiMode mode,
                   WifiPreamble preamble)
{
    const auto packetSize = p->GetSize();
    if (packetSize < m_params.packetSize)
    {
        // ignore small packets (ACKs, ...)
        return;
    }
    Ptr<Packet> packet = p->Copy();
    if (IsUsingAmpduOrSmpdu())
    {
        AmpduSubframeHeader ampduHdr;
        packet->RemoveHeader(ampduHdr);
    }
    WifiMacHeader hdr;
    packet->PeekHeader(hdr);
    if (!hdr.IsData() || !hdr.GetAddr1().IsGroup())
    {
        // ignore non-data frames and unicast data frames
        return;
    }
    const auto staId = ConvertContextToNodeId(context) - 1;
    NS_ASSERT(staId <= m_params.stas.size());
    m_phyRxPerSta.at(staId)++;
}

void
GcrTestBase::NotifyTxopTerminated(Time startTime, Time duration, uint8_t linkId)
{
    NS_LOG_INFO("AP: terminated TXOP");
    NS_TEST_EXPECT_MSG_EQ((m_nTxGroupcastInCurrentTxop <= 1),
                          true,
                          "An MPDU and a retransmission of the same MPDU shall not be transmitted "
                          "within the same GCR TXOP");
    NS_TEST_EXPECT_MSG_EQ((m_nTxRtsInCurrentTxop + m_nTxCtsInCurrentTxop <= 1),
                          true,
                          "No more than one protection frame exchange per GCR TXOP");
    m_nTxGroupcastInCurrentTxop = 0;
    m_nTxRtsInCurrentTxop = 0;
    m_nTxCtsInCurrentTxop = 0;
}

void
GcrTestBase::CheckResults()
{
    NS_LOG_FUNCTION(this);

    const auto expectedNumRts =
        (m_expectGcrUsed && (m_params.gcrProtectionMode == GroupcastProtectionMode::RTS_CTS) &&
         (m_params.rtsThreshold < (m_params.packetSize * m_params.maxNumMpdusInPsdu)))
            ? m_totalTx + m_params.rtsFramesToCorrupt.size() + m_params.ctsFramesToCorrupt.size()
            : 0U;
    NS_TEST_EXPECT_MSG_EQ(+m_nTxApRts, +expectedNumRts, "Unexpected number of RTS frames");

    const auto expectedNumCts =
        (m_expectGcrUsed && (m_params.gcrProtectionMode == GroupcastProtectionMode::RTS_CTS) &&
         (m_params.rtsThreshold < (m_params.packetSize * m_params.maxNumMpdusInPsdu)))
            ? m_totalTx + m_params.ctsFramesToCorrupt.size()
            : 0U;
    const auto totalNumCts = std::accumulate(m_txCtsPerSta.cbegin(), m_txCtsPerSta.cend(), 0U);
    NS_TEST_EXPECT_MSG_EQ(totalNumCts, expectedNumCts, "Unexpected number of CTS frames");

    const auto expectedNumCtsToSelf =
        (m_expectGcrUsed && (m_params.gcrProtectionMode == GroupcastProtectionMode::CTS_TO_SELF))
            ? m_totalTx
            : 0U;
    NS_TEST_EXPECT_MSG_EQ(+m_nTxApCts,
                          +expectedNumCtsToSelf,
                          "Unexpected number of CTS-to-self frames");

    uint8_t numStas = m_params.stas.size();
    for (uint8_t i = 0; i < numStas; ++i)
    {
        NS_TEST_EXPECT_MSG_EQ(m_rxUnicastPerSta.at(i),
                              m_params.numUnicastPackets,
                              "Unexpected number of received unicast packets for STA " << i + 1);
    }

    const auto htCapableStas =
        std::count_if(m_params.stas.cbegin(), m_params.stas.cend(), [](const auto& staInfo) {
            return (staInfo.standard >= WIFI_STANDARD_80211n);
        });
    if (m_params.numUnicastPackets > 0)
    {
        NS_TEST_EXPECT_MSG_EQ(+m_nTxAddbaReq,
                              +htCapableStas,
                              "Incorrect number of transmitted ADDBA requests");
        NS_TEST_EXPECT_MSG_EQ(+m_nTxAddbaResp,
                              +htCapableStas,
                              "Incorrect number of transmitted ADDBA responses");
        NS_TEST_EXPECT_MSG_EQ(+m_nTxDelba,
                              ((m_params.baInactivityTimeout > 0 && m_params.numUnicastPackets > 1)
                                   ? +htCapableStas
                                   : 0),
                              "Incorrect number of transmitted DELBA frames");
    }

    const auto gcrCapableStas =
        std::count_if(m_params.stas.cbegin(), m_params.stas.cend(), [](const auto& staInfo) {
            return staInfo.gcrCapable;
        });
    const auto isGcrBa = m_apWifiMac->GetGcrManager()->GetRetransmissionPolicy() ==
                         GroupAddressRetransmissionPolicy::GCR_BLOCK_ACK;
    if (m_params.numGroupcastPackets > 0 && (isGcrBa || (m_params.maxNumMpdusInPsdu > 1)))
    {
        NS_TEST_EXPECT_MSG_EQ(+m_nTxGcrAddbaReq,
                              gcrCapableStas + m_params.addbaReqsToCorrupt.size(),
                              "Incorrect number of transmitted GCR ADDBA requests");
        NS_TEST_EXPECT_MSG_EQ(+m_nTxGcrAddbaResp,
                              gcrCapableStas + m_params.addbaRespsToCorrupt.size(),
                              "Incorrect number of transmitted GCR ADDBA responses");
        NS_TEST_EXPECT_MSG_EQ(+m_nTxGcrDelba,
                              ((m_params.baInactivityTimeout > 0) ? +gcrCapableStas : 0),
                              "Incorrect number of transmitted GCR DELBA frames");
    }
    else
    {
        NS_TEST_EXPECT_MSG_EQ(+m_nTxGcrAddbaReq, 0, "Unexpected GCR ADDBA requests");
        NS_TEST_EXPECT_MSG_EQ(+m_nTxGcrAddbaResp, 0, "Unexpected GCR ADDBA responses");
        NS_TEST_EXPECT_MSG_EQ(+m_nTxGcrDelba, 0, "Unexpected GCR DELBA frames");
    }
}

void
GcrTestBase::DoSetup()
{
    NS_LOG_FUNCTION(this << m_testName);

    // WifiHelper::EnableLogComponents();
    // LogComponentEnable("WifiGcrTest", LOG_LEVEL_ALL);

    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(2);
    int64_t streamNumber = 100;

    Config::SetDefault("ns3::WifiMacQueue::MaxDelay", TimeValue(m_params.maxLifetime));
    const auto maxPacketsInQueue = std::max<uint16_t>(m_params.numGroupcastPackets + 1, 500);
    Config::SetDefault("ns3::WifiMacQueue::MaxSize",
                       StringValue(std::to_string(maxPacketsInQueue) + "p"));

    const auto maxChannelWidth =
        std::max_element(m_params.stas.cbegin(),
                         m_params.stas.cend(),
                         [](const auto& lhs, const auto& rhs) {
                             return lhs.maxChannelWidth < rhs.maxChannelWidth;
                         })
            ->maxChannelWidth;
    const auto maxNss = std::max_element(m_params.stas.cbegin(),
                                         m_params.stas.cend(),
                                         [](const auto& lhs, const auto& rhs) {
                                             return lhs.maxNumStreams < rhs.maxNumStreams;
                                         })
                            ->maxNumStreams;

    uint8_t numStas = m_params.stas.size();
    NodeContainer wifiApNode(1);
    NodeContainer wifiStaNodes(numStas);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211be);
    wifi.SetRemoteStationManager("ns3::IdealWifiManagerForGcrTest",
                                 "RtsCtsThreshold",
                                 UintegerValue(m_params.rtsThreshold),
                                 "NonUnicastMode",
                                 (GetNumNonHtStas(m_params.stas) == 0)
                                     ? StringValue("HtMcs0")
                                     : StringValue("OfdmRate6Mbps"));

    wifi.ConfigHtOptions("ShortGuardIntervalSupported", BooleanValue(true));
    wifi.ConfigHeOptions("GuardInterval", TimeValue(NanoSeconds(800)));

    WifiMacHelper apMacHelper;
    apMacHelper.SetType("ns3::ApWifiMac",
                        "Ssid",
                        SsidValue(Ssid("ns-3-ssid")),
                        "BeaconGeneration",
                        BooleanValue(true),
                        "RobustAVStreamingSupported",
                        BooleanValue(true));
    ConfigureGcrManager(apMacHelper);

    WifiMacHelper staMacHelper;
    staMacHelper.SetType("ns3::StaWifiMac",
                         "Ssid",
                         SsidValue(Ssid("ns-3-ssid")),
                         "ActiveProbing",
                         BooleanValue(false),
                         "QosSupported",
                         BooleanValue(true));
    ConfigureGcrManager(staMacHelper);

    NetDeviceContainer apDevice;
    NetDeviceContainer staDevices;

    const auto differentChannelWidths =
        std::any_of(m_params.stas.cbegin(),
                    m_params.stas.cend(),
                    [maxChannelWidth](const auto& staInfo) {
                        return (staInfo.maxChannelWidth != maxChannelWidth);
                    });
    if (differentChannelWidths)
    {
        SpectrumWifiPhyHelper phyHelper;
        phyHelper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

        auto channel = CreateObject<MultiModelSpectrumChannel>();
        phyHelper.SetChannel(channel);

        apDevice = wifi.Install(phyHelper, apMacHelper, wifiApNode);
        auto staNodesIt = wifiStaNodes.Begin();
        for (const auto& staInfo : m_params.stas)
        {
            wifi.SetStandard(staInfo.standard);
            staDevices.Add(wifi.Install(phyHelper, staMacHelper, *staNodesIt));
            ++staNodesIt;
        }

        // Uncomment the lines below to write PCAP files
        // phyHelper.EnablePcap("wifi-gcr_AP", apDevice);
        // phyHelper.EnablePcap("wifi-gcr_STA", staDevices);
    }
    else
    {
        YansWifiPhyHelper phyHelper;
        phyHelper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

        auto channel = YansWifiChannelHelper::Default();
        channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
        phyHelper.SetChannel(channel.Create());

        apDevice = wifi.Install(phyHelper, apMacHelper, wifiApNode);
        auto staNodesIt = wifiStaNodes.Begin();
        for (const auto& staInfo : m_params.stas)
        {
            wifi.SetStandard(staInfo.standard);
            staDevices.Add(wifi.Install(phyHelper, staMacHelper, *staNodesIt));
            ++staNodesIt;
        }

        // Uncomment the lines below to write PCAP files
        // phyHelper.EnablePcap("wifi-gcr_AP", apDevice);
        // phyHelper.EnablePcap("wifi-gcr_STA", staDevices);
    }

    // Assign fixed streams to random variables in use
    streamNumber += WifiHelper::AssignStreams(apDevice, streamNumber);
    streamNumber += WifiHelper::AssignStreams(staDevices, streamNumber);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    for (uint8_t i = 0; i < numStas; ++i)
    {
        positionAlloc->Add(Vector(i, 0.0, 0.0));
    }
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);

    auto apNetDevice = DynamicCast<WifiNetDevice>(apDevice.Get(0));
    m_apWifiMac = DynamicCast<ApWifiMac>(apNetDevice->GetMac());
    m_apWifiMac->SetAttribute("BE_MaxAmsduSize", UintegerValue(0));
    m_apWifiMac->SetAttribute(
        "BE_MaxAmpduSize",
        UintegerValue((m_params.maxNumMpdusInPsdu > 1)
                          ? (m_params.maxNumMpdusInPsdu * (m_params.packetSize + 100))
                          : 0));

    m_apWifiMac->SetAttribute("BE_BlockAckInactivityTimeout",
                              UintegerValue(m_params.baInactivityTimeout));
    m_apWifiMac->GetQosTxop(AC_BE)->SetTxopLimit(m_params.txopLimit);

    m_apWifiMac->GetWifiPhy(0)->SetOperatingChannel(
        WifiPhy::ChannelTuple{0, maxChannelWidth, WIFI_PHY_BAND_5GHZ, 0});

    m_apWifiMac->GetWifiPhy(0)->SetNumberOfAntennas(maxNss);
    m_apWifiMac->GetWifiPhy(0)->SetMaxSupportedTxSpatialStreams(maxNss);
    m_apWifiMac->GetWifiPhy(0)->SetMaxSupportedRxSpatialStreams(maxNss);

    m_apErrorModel = CreateObject<ListErrorModel>();
    m_apWifiMac->GetWifiPhy(0)->SetPostReceptionErrorModel(m_apErrorModel);

    for (uint8_t i = 0; i < numStas; ++i)
    {
        auto staNetDevice = DynamicCast<WifiNetDevice>(staDevices.Get(i));
        auto staWifiMac = DynamicCast<StaWifiMac>(staNetDevice->GetMac());
        staWifiMac->SetRobustAVStreamingSupported(m_params.stas.at(i).gcrCapable);
        m_stasWifiMac.emplace_back(staWifiMac);

        if (m_params.stas.at(i).standard >= WIFI_STANDARD_80211n)
        {
            auto staHtConfiguration = CreateObject<HtConfiguration>();
            staHtConfiguration->m_40MHzSupported =
                (m_params.stas.at(i).standard >= WIFI_STANDARD_80211ac ||
                 m_params.stas.at(i).maxChannelWidth >= MHz_u{40});
            staHtConfiguration->m_sgiSupported = (m_params.stas.at(i).minGi == NanoSeconds(400));
            staNetDevice->SetHtConfiguration(staHtConfiguration);
        }
        if (m_params.stas.at(i).standard >= WIFI_STANDARD_80211ac)
        {
            auto staVhtConfiguration = CreateObject<VhtConfiguration>();
            staVhtConfiguration->m_160MHzSupported =
                (m_params.stas.at(i).maxChannelWidth >= MHz_u{160});
            staNetDevice->SetVhtConfiguration(staVhtConfiguration);
        }
        if (m_params.stas.at(i).standard >= WIFI_STANDARD_80211ax)
        {
            auto staHeConfiguration = CreateObject<HeConfiguration>();
            staHeConfiguration->SetGuardInterval(
                std::max(m_params.stas.at(i).minGi, NanoSeconds(800)));
            staNetDevice->SetHeConfiguration(staHeConfiguration);
        }

        staWifiMac->GetWifiPhy(0)->SetOperatingChannel(
            WifiPhy::ChannelTuple{0, m_params.stas.at(i).maxChannelWidth, WIFI_PHY_BAND_5GHZ, 0});

        staWifiMac->GetWifiPhy(0)->SetNumberOfAntennas(m_params.stas.at(i).maxNumStreams);
        staWifiMac->GetWifiPhy(0)->SetMaxSupportedTxSpatialStreams(
            m_params.stas.at(i).maxNumStreams);
        staWifiMac->GetWifiPhy(0)->SetMaxSupportedRxSpatialStreams(
            m_params.stas.at(i).maxNumStreams);

        auto errorModel = CreateObject<ListErrorModel>();
        m_errorModels.push_back(errorModel);
        staWifiMac->GetWifiPhy(0)->SetPostReceptionErrorModel(errorModel);

        m_phyRxPerSta.push_back(0);
        m_txCtsPerSta.push_back(0);
        m_rxGroupcastPerSta.emplace_back();
        m_rxUnicastPerSta.emplace_back();
    }

    // give packet socket powers to nodes.
    PacketSocketHelper packetSocket;
    packetSocket.Install(wifiStaNodes);
    packetSocket.Install(wifiApNode);

    if (m_params.numGroupcastPackets > 0)
    {
        PacketSocketAddress groupcastSocket;
        groupcastSocket.SetSingleDevice(apDevice.Get(0)->GetIfIndex());
        groupcastSocket.SetPhysicalAddress(
            Mac48Address::GetMulticast(Ipv4Address("239.192.100.1")));
        groupcastSocket.SetProtocol(MULTICAST_PROTOCOL);

        m_groupcastClient = CreateObject<PacketSocketClient>();
        m_groupcastClient->SetAttribute("MaxPackets", UintegerValue(m_params.numGroupcastPackets));
        m_groupcastClient->SetAttribute("PacketSize", UintegerValue(m_params.packetSize));
        m_groupcastClient->SetAttribute("Interval", TimeValue(Seconds(0)));
        m_groupcastClient->SetRemote(groupcastSocket);
        wifiApNode.Get(0)->AddApplication(m_groupcastClient);
        m_groupcastClient->SetStartTime(m_params.startGroupcast);
        m_groupcastClient->SetStopTime(m_params.duration);

        for (uint8_t i = 0; i < numStas; ++i)
        {
            auto groupcastServer = CreateObject<PacketSocketServer>();
            groupcastServer->SetLocal(groupcastSocket);
            wifiStaNodes.Get(i)->AddApplication(groupcastServer);
            groupcastServer->SetStartTime(Seconds(0.0));
            groupcastServer->SetStopTime(m_params.duration);
        }
    }

    if (m_params.numUnicastPackets > 0)
    {
        uint8_t staIndex = 0;
        for (uint8_t i = 0; i < numStas; ++i)
        {
            PacketSocketAddress unicastSocket;
            unicastSocket.SetSingleDevice(apDevice.Get(0)->GetIfIndex());
            unicastSocket.SetPhysicalAddress(staDevices.Get(staIndex++)->GetAddress());
            unicastSocket.SetProtocol(UNICAST_PROTOCOL);

            auto unicastClient = CreateObject<PacketSocketClient>();
            unicastClient->SetAttribute("PacketSize", UintegerValue(m_params.packetSize));
            unicastClient->SetAttribute("MaxPackets", UintegerValue(m_params.numUnicastPackets));
            unicastClient->SetAttribute("Interval", TimeValue(Seconds(0)));
            unicastClient->SetRemote(unicastSocket);
            wifiApNode.Get(0)->AddApplication(unicastClient);
            unicastClient->SetStartTime(m_params.startUnicast);
            unicastClient->SetStopTime(m_params.duration);

            auto unicastServer = CreateObject<PacketSocketServer>();
            unicastServer->SetLocal(unicastSocket);
            wifiStaNodes.Get(i)->AddApplication(unicastServer);
            unicastServer->SetStartTime(Seconds(0.0));
            unicastServer->SetStopTime(m_params.duration);
        }
    }

    PointerValue ptr;
    m_apWifiMac->GetAttribute("BE_Txop", ptr);
    ptr.Get<QosTxop>()->TraceConnectWithoutContext(
        "TxopTrace",
        MakeCallback(&GcrTestBase::NotifyTxopTerminated, this));

    Config::Connect("/NodeList/*/$ns3::Node/ApplicationList/*/$ns3::PacketSocketClient/Tx",
                    MakeCallback(&GcrTestBase::PacketGenerated, this));

    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phys/0/PhyTxPsduBegin",
                    MakeCallback(&GcrTestBase::Transmit, this));

    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/State/RxOk",
                    MakeCallback(&GcrTestBase::PhyRx, this));

    Config::Connect("/NodeList/*/ApplicationList/*/$ns3::PacketSocketServer/Rx",
                    MakeCallback(&GcrTestBase::Receive, this));
}

void
GcrTestBase::DoRun()
{
    NS_LOG_FUNCTION(this << m_testName);

    Simulator::Stop(m_params.duration);
    Simulator::Run();

    CheckResults();

    Simulator::Destroy();
}

GcrUrTest::GcrUrTest(const std::string& testName,
                     const GcrParameters& commonParams,
                     const GcrUrParameters& gcrUrParams)
    : GcrTestBase(testName, commonParams),
      m_gcrUrParams{gcrUrParams},
      m_currentUid{0}
{
}

void
GcrUrTest::PacketGenerated(std::string context, Ptr<const Packet> p, const Address& addr)
{
    if (!m_gcrUrParams.packetsPauzeAggregation.has_value() ||
        m_packets < m_gcrUrParams.packetsPauzeAggregation.value() ||
        m_packets > m_gcrUrParams.packetsResumeAggregation.value())
    {
        GcrTestBase::PacketGenerated(context, p, addr);
        return;
    }
    m_packets++;
    if (m_packets == (m_gcrUrParams.packetsPauzeAggregation.value() + 1))
    {
        m_groupcastClient->SetAttribute("Interval", TimeValue(MilliSeconds(10)));
    }
    if (m_gcrUrParams.packetsResumeAggregation.has_value() &&
        (m_packets == (m_gcrUrParams.packetsResumeAggregation.value() + 1)))
    {
        m_groupcastClient->SetAttribute("Interval", TimeValue(MilliSeconds(0)));
    }
}

void
GcrUrTest::Transmit(std::string context,
                    WifiConstPsduMap psduMap,
                    WifiTxVector txVector,
                    double txPowerW)
{
    auto psdu = psduMap.cbegin()->second;
    auto mpdu = *psdu->begin();
    auto addr1 = mpdu->GetHeader().GetAddr1();
    if (addr1.IsGroup() && !addr1.IsBroadcast() && mpdu->GetHeader().IsQosData())
    {
        if (const auto uid = mpdu->GetPacket()->GetUid(); m_currentUid != uid)
        {
            m_totalTxGroupcasts.push_back(0);
            m_currentUid = uid;
            m_currentMpdu = nullptr;
        }
        if (m_totalTxGroupcasts.back() == 0)
        {
            NS_LOG_INFO("AP: groupcast initial transmission (#MPDUs=" << psdu->GetNMpdus() << ")");
            for (std::size_t i = 0; i < psdu->GetNMpdus(); ++i)
            {
                NS_TEST_EXPECT_MSG_EQ(
                    psdu->GetHeader(i).IsRetry(),
                    false,
                    "retry flag should not be set for the first groupcast transmission");
            }
            m_currentMpdu = mpdu;
        }
        else
        {
            NS_ASSERT(m_currentMpdu);
            NS_TEST_EXPECT_MSG_EQ(m_expectGcrUsed, true, "GCR service should not be used");
            NS_LOG_INFO("AP: groupcast sollicited retry #"
                        << +m_totalTxGroupcasts.back() << " (#MPDUs=" << psdu->GetNMpdus() << ")");
            for (std::size_t i = 0; i < psdu->GetNMpdus(); ++i)
            {
                NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(i).IsRetry(),
                                      true,
                                      "retry flag should be set for unsolicited retries");
            }
            NS_TEST_EXPECT_MSG_EQ((mpdu->GetHeader().IsQosAmsdu() ? mpdu->begin()->first->GetSize()
                                                                  : mpdu->GetPacket()->GetSize()),
                                  (m_currentMpdu->GetHeader().IsQosAmsdu()
                                       ? m_currentMpdu->begin()->first->GetSize()
                                       : m_currentMpdu->GetPacket()->GetSize()),
                                  "Unexpected MPDU size");
        }
        if (m_params.maxNumMpdusInPsdu > 1)
        {
            const uint16_t prevTxMpdus =
                (m_totalTxGroupcasts.size() - 1) * m_expectedMaxNumMpdusInPsdu;
            const uint16_t remainingMpdus = m_gcrUrParams.packetsPauzeAggregation.has_value()
                                                ? m_params.numGroupcastPackets
                                                : m_params.numGroupcastPackets - prevTxMpdus;
            NS_TEST_EXPECT_MSG_EQ(
                psdu->GetNMpdus(),
                (IsUsingAmpduOrSmpdu() ? std::min(m_expectedMaxNumMpdusInPsdu, remainingMpdus) : 1),
                "Incorrect number of aggregated MPDUs");
            const auto nonAggregatedMpdus = (m_gcrUrParams.packetsResumeAggregation.value_or(0) -
                                             m_gcrUrParams.packetsPauzeAggregation.value_or(0));
            const uint16_t threshold =
                (m_gcrUrParams.packetsPauzeAggregation.value_or(0) / m_params.maxNumMpdusInPsdu) +
                nonAggregatedMpdus;
            for (std::size_t i = 0; i < psdu->GetNMpdus(); ++i)
            {
                auto previousMpdusNotAggregated =
                    (m_totalTxGroupcasts.size() > threshold) ? nonAggregatedMpdus : 0;
                auto expectedSeqNum = IsUsingAmpduOrSmpdu()
                                          ? ((i + prevTxMpdus) - previousMpdusNotAggregated)
                                          : (((m_totalTxGroupcasts.size() - 1) +
                                              (m_gcrUrParams.packetsPauzeAggregation.value_or(0) /
                                               m_params.maxNumMpdusInPsdu)) -
                                             previousMpdusNotAggregated);
                NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(i).GetSequenceNumber(),
                                      expectedSeqNum,
                                      "unexpected sequence number");
            }
        }
        else
        {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetNMpdus(), 1, "MPDU aggregation should not be used");
            NS_TEST_EXPECT_MSG_EQ(mpdu->GetHeader().GetSequenceNumber(),
                                  (m_totalTxGroupcasts.size() - 1),
                                  "unexpected sequence number");
        }
        m_totalTxGroupcasts.back()++;
        m_nTxGroupcastInCurrentTxop++;
    }
    else if (mpdu->GetHeader().IsRts())
    {
        m_nTxRtsInCurrentTxop++;
    }
    else if (const auto nodeId = ConvertContextToNodeId(context);
             (mpdu->GetHeader().IsCts() && (nodeId == 0)))
    {
        m_nTxCtsInCurrentTxop++;
    }
    GcrTestBase::Transmit(context, psduMap, txVector, txPowerW);
}

void
GcrUrTest::Receive(std::string context, Ptr<const Packet> p, const Address& adr)
{
    const auto staId = ConvertContextToNodeId(context) - 1;
    NS_LOG_INFO("STA" << staId + 1 << ": multicast packet forwarded up at attempt "
                      << +m_totalTxGroupcasts.back());
    m_rxGroupcastPerSta.at(staId).push_back(m_totalTxGroupcasts.back());
}

void
GcrUrTest::ConfigureGcrManager(WifiMacHelper& macHelper)
{
    macHelper.SetGcrManager("ns3::WifiDefaultGcrManager",
                            "RetransmissionPolicy",
                            StringValue("GCR_UR"),
                            "UnsolicitedRetryLimit",
                            UintegerValue(m_gcrUrParams.nGcrRetries),
                            "GcrProtectionMode",
                            EnumValue(m_params.gcrProtectionMode));
}

bool
GcrUrTest::IsUsingAmpduOrSmpdu() const
{
    if (!GcrTestBase::IsUsingAmpduOrSmpdu())
    {
        return false;
    }
    if (GetNumNonHtStas(m_params.stas) > 0)
    {
        return false;
    }
    const auto nonAggregatedMpdus = (m_gcrUrParams.packetsResumeAggregation.value_or(0) -
                                     m_gcrUrParams.packetsPauzeAggregation.value_or(0));
    const uint16_t threshold =
        (m_gcrUrParams.packetsPauzeAggregation.value_or(0) / m_params.maxNumMpdusInPsdu) +
        nonAggregatedMpdus;
    const auto expectAmpdu =
        (!m_gcrUrParams.packetsPauzeAggregation.has_value() ||
         (m_totalTxGroupcasts.size() <=
          (m_gcrUrParams.packetsPauzeAggregation.value() / m_params.maxNumMpdusInPsdu)) ||
         (m_totalTxGroupcasts.size() > threshold));
    return expectAmpdu;
}

void
GcrUrTest::CheckResults()
{
    GcrTestBase::CheckResults();

    const auto expectedMaxNumMpdusInPsdu =
        (GetNumNonHtStas(m_params.stas) == 0) ? m_expectedMaxNumMpdusInPsdu : 1;
    const std::size_t numNonRetryGroupcastFrames =
        m_gcrUrParams.packetsPauzeAggregation.has_value()
            ? (m_params.numGroupcastPackets -
               std::ceil(static_cast<double>(m_gcrUrParams.packetsPauzeAggregation.value()) /
                         expectedMaxNumMpdusInPsdu) -
               std::ceil(static_cast<double>(m_params.numGroupcastPackets -
                                             m_gcrUrParams.packetsResumeAggregation.value()) /
                         expectedMaxNumMpdusInPsdu))
            : std::ceil(static_cast<double>(m_params.numGroupcastPackets -
                                            m_params.expectedDroppedGroupcastMpdus.size()) /
                        expectedMaxNumMpdusInPsdu);
    NS_TEST_EXPECT_MSG_EQ(m_totalTxGroupcasts.size(),
                          numNonRetryGroupcastFrames,
                          "Unexpected number of non-retransmitted groupcast frames");

    NS_ASSERT(!m_totalTxGroupcasts.empty());
    const auto totalTxGroupcastFrames =
        std::accumulate(m_totalTxGroupcasts.cbegin(), m_totalTxGroupcasts.cend(), 0U);
    uint8_t numRetries = m_expectGcrUsed ? m_gcrUrParams.nGcrRetries : 0;
    // with test conditions, one more retry when A-MPDU is not used
    const auto nonAmpduPackets = m_gcrUrParams.packetsPauzeAggregation.has_value()
                                     ? (m_gcrUrParams.packetsResumeAggregation.value() -
                                        m_gcrUrParams.packetsPauzeAggregation.value())
                                     : 0;
    ;
    uint16_t expectedTxAttempts =
        m_gcrUrParams.packetsPauzeAggregation.has_value() &&
                (m_gcrUrParams.expectedSkippedRetries > 0)
            ? (std::ceil((1 + numRetries - m_gcrUrParams.expectedSkippedRetries) *
                         ((static_cast<double>(m_params.numGroupcastPackets - nonAmpduPackets) /
                           expectedMaxNumMpdusInPsdu))) +
               ((1 + numRetries - (m_gcrUrParams.expectedSkippedRetries - 1)) * nonAmpduPackets))
            : ((1 + numRetries - m_gcrUrParams.expectedSkippedRetries) *
               numNonRetryGroupcastFrames);
    NS_TEST_EXPECT_MSG_EQ(totalTxGroupcastFrames,
                          expectedTxAttempts,
                          "Unexpected number of transmission attempts");

    uint8_t numStas = m_params.stas.size();
    for (uint8_t i = 0; i < numStas; ++i)
    {
        numRetries =
            (m_params.stas.at(i).standard >= WIFI_STANDARD_80211n) ? m_gcrUrParams.nGcrRetries : 0;
        expectedTxAttempts =
            m_gcrUrParams.packetsPauzeAggregation.has_value() &&
                    (m_gcrUrParams.expectedSkippedRetries > 0)
                ? (std::ceil((1 + numRetries - m_gcrUrParams.expectedSkippedRetries) *
                             ((static_cast<double>(m_params.numGroupcastPackets - nonAmpduPackets) /
                               expectedMaxNumMpdusInPsdu))) +
                   ((1 + numRetries - (m_gcrUrParams.expectedSkippedRetries - 1)) *
                    nonAmpduPackets))
                : ((1 + numRetries - m_gcrUrParams.expectedSkippedRetries) *
                   numNonRetryGroupcastFrames);

        // calculate the amount of corrupted PSDUs and the expected number of retransmission per
        // MPDU
        uint8_t corruptedPsdus = 0;
        uint8_t prevExpectedNumAttempt = 1;
        uint8_t prevPsduNum = 1;
        uint8_t droppedPsdus = 0;
        auto prevDropped = false;
        auto maxNumMpdusInPsdu =
            (GetNumNonHtStas(m_params.stas) == 0) ? m_params.maxNumMpdusInPsdu : 1;
        for (uint16_t j = 0; j < m_params.numGroupcastPackets; ++j)
        {
            uint8_t expectedNumAttempt = 1;
            const auto psduNum = ((j / maxNumMpdusInPsdu) + 1);
            const auto packetInAmpdu = (maxNumMpdusInPsdu > 1) ? ((j % maxNumMpdusInPsdu) + 1) : 1;
            if (psduNum > prevPsduNum)
            {
                prevExpectedNumAttempt = 1;
                prevDropped = false;
            }
            prevPsduNum = psduNum;
            for (auto& mpduToCorruptPerPsdu : m_params.mpdusToCorruptPerPsdu)
            {
                if (mpduToCorruptPerPsdu.first <= ((psduNum - 1) * (1 + numRetries)))
                {
                    continue;
                }
                if (mpduToCorruptPerPsdu.first > (psduNum * (1 + numRetries)))
                {
                    continue;
                }
                if (((GetNumGcrStas(m_params.stas) > 0) && GetNumNonHtStas(m_params.stas) > 0) &&
                    (numRetries == 0) &&
                    (mpduToCorruptPerPsdu.first % m_gcrUrParams.nGcrRetries) != 1)
                {
                    continue;
                }
                const auto& corruptedMpdusForSta =
                    (mpduToCorruptPerPsdu.second.count(0) != 0)
                        ? mpduToCorruptPerPsdu.second.at(0)
                        : ((mpduToCorruptPerPsdu.second.count(i + 1) != 0)
                               ? mpduToCorruptPerPsdu.second.at(i + 1)
                               : std::set<uint8_t>{});
                if (corruptedMpdusForSta.count(packetInAmpdu) == 0)
                {
                    break;
                }
                if ((maxNumMpdusInPsdu == 1) ||
                    ((corruptedMpdusForSta.size() == 2) && (packetInAmpdu == 2)))
                {
                    corruptedPsdus++;
                }
                expectedNumAttempt++;
            }
            if (const auto nMaxAttempts =
                    m_params.stas.at(i).gcrCapable ? (m_gcrUrParams.nGcrRetries + 1) : 1;
                (expectedNumAttempt > nMaxAttempts) ||
                (m_params.expectedDroppedGroupcastMpdus.count(j + 1) != 0))
            {
                droppedPsdus++;
                prevDropped = true;
                continue;
            }
            expectedNumAttempt = (prevDropped && (psduNum < 2))
                                     ? 1
                                     : std::max(expectedNumAttempt, prevExpectedNumAttempt);
            prevExpectedNumAttempt = expectedNumAttempt;
            const std::size_t rxPsdus = (j - droppedPsdus);
            NS_ASSERT(m_rxGroupcastPerSta.at(i).size() > rxPsdus);
            NS_TEST_EXPECT_MSG_EQ(+m_rxGroupcastPerSta.at(i).at(rxPsdus),
                                  +expectedNumAttempt,
                                  "Packet has not been forwarded up at the expected TX attempt");
        }
        const std::size_t rxPackets = (m_params.numGroupcastPackets - droppedPsdus);
        NS_TEST_EXPECT_MSG_EQ(+m_rxGroupcastPerSta.at(i).size(),
                              rxPackets,
                              "STA" + std::to_string(i + 1) +
                                  " did not receive the expected number of groupcast packets");
        NS_TEST_EXPECT_MSG_EQ(+m_phyRxPerSta.at(i),
                              (expectedTxAttempts - corruptedPsdus),
                              "STA" + std::to_string(i + 1) +
                                  " did not receive the expected number of retransmissions");
    }
}

GcrBaTest::GcrBaTest(const std::string& testName,
                     const GcrParameters& commonParams,
                     const GcrBaParameters& gcrBaParams)
    : GcrTestBase(testName, commonParams),
      m_gcrBaParams{gcrBaParams},
      m_nTxGcrBar{0},
      m_nTxGcrBlockAck{0},
      m_nTxBlockAck{0},
      m_firstTxSeq{0},
      m_lastTxSeq{-1},
      m_nTxGcrBarsInCurrentTxop{0}
{
}

void
GcrBaTest::PacketGenerated(std::string context, Ptr<const Packet> p, const Address& addr)
{
    if (m_params.rtsFramesToCorrupt.empty() && m_params.ctsFramesToCorrupt.empty())
    {
        return;
    }
    GcrTestBase::PacketGenerated(context, p, addr);
}

void
GcrBaTest::Transmit(std::string context,
                    WifiConstPsduMap psduMap,
                    WifiTxVector txVector,
                    double txPowerW)
{
    auto psdu = psduMap.cbegin()->second;
    auto mpdu = *psdu->begin();
    const auto nodeId = ConvertContextToNodeId(context);
    auto addr1 = mpdu->GetHeader().GetAddr1();
    if (addr1.IsGroup() && !addr1.IsBroadcast() && mpdu->GetHeader().IsQosData())
    {
        NS_TEST_EXPECT_MSG_EQ(nodeId, 0, "Groupcast transmission from unexpected node");
        NS_LOG_INFO("AP: groupcast transmission (#MPDUs=" << psdu->GetNMpdus() << ")");
        const auto txopLimitAllowsAggregation =
            (m_params.txopLimit.IsZero() || m_params.txopLimit > MicroSeconds(320));
        const uint16_t prevTxMpdus = m_totalTx * m_expectedMaxNumMpdusInPsdu;
        const uint16_t remainingMpdus = m_params.numGroupcastPackets - prevTxMpdus;
        const auto expectedNumAggregates =
            (GetNumNonHtStas(m_params.stas) == 0) && txopLimitAllowsAggregation
                ? (((m_totalTx == 0) || m_params.mpdusToCorruptPerPsdu.empty() ||
                    (!m_params.mpdusToCorruptPerPsdu.empty() &&
                     m_params.mpdusToCorruptPerPsdu.cbegin()->second.size() > 1))
                       ? ((m_params.mpdusToCorruptPerPsdu.empty() &&
                           (GetNumNonGcrStas(m_params.stas) == 0))
                              ? std::min(m_expectedMaxNumMpdusInPsdu, remainingMpdus)
                              : m_expectedMaxNumMpdusInPsdu)
                       : ((!m_params.expectedDroppedGroupcastMpdus.empty() &&
                           m_totalTx <= m_expectedMaxNumMpdusInPsdu)
                              ? m_expectedMaxNumMpdusInPsdu
                              : m_params.mpdusToCorruptPerPsdu.cbegin()
                                    ->second.cbegin()
                                    ->second.size()))
                : 1;
        NS_TEST_EXPECT_MSG_EQ(psdu->GetNMpdus(),
                              expectedNumAggregates,
                              "Incorrect number of aggregated MPDUs");
        const uint16_t maxLastSeqNum = (((m_totalTx + 1) * m_expectedMaxNumMpdusInPsdu) - 1);
        const uint16_t limitLastSeqNum = (m_params.numGroupcastPackets - 1);
        uint16_t expectedLastSeqNum =
            (m_expectGcrUsed && (GetNumNonHtStas(m_params.stas) > 0))
                ? (m_totalTx / 2)
                : (((GetNumNonHtStas(m_params.stas) == 0) && txopLimitAllowsAggregation)
                       ? std::min(maxLastSeqNum, limitLastSeqNum)
                       : m_totalTx);
        for (std::size_t i = 0; i < psdu->GetNMpdus(); ++i)
        {
            const auto isNewTx = (m_lastTxSeq < psdu->GetHeader(i).GetSequenceNumber());
            NS_TEST_EXPECT_MSG_EQ(
                psdu->GetHeader(i).IsRetry(),
                !isNewTx,
                "retry flag should not be set for the first groupcast transmission");
        }
        if (m_expectGcrUsed)
        {
            auto expectedStartSeq = std::min_element(m_rxGroupcastPerSta.cbegin(),
                                                     m_rxGroupcastPerSta.cend(),
                                                     [](const auto& v1, const auto& v2) {
                                                         return v1.size() < v2.size();
                                                     })
                                        ->size();
            if (psdu->GetHeader(0).IsRetry() && GetNumNonGcrStas(m_params.stas) > 0)
            {
                expectedStartSeq -= psdu->GetNMpdus();
            }
            m_firstTxSeq = psdu->GetHeader(0).GetSequenceNumber();
            NS_TEST_EXPECT_MSG_EQ(m_firstTxSeq,
                                  expectedStartSeq,
                                  "Incorrect starting sequence number");
            m_lastTxSeq = psdu->GetHeader(psdu->GetNMpdus() - 1).GetSequenceNumber();
            if (m_totalTx > 0)
            {
                if (!m_params.mpdusToCorruptPerPsdu.empty())
                {
                    expectedLastSeqNum = 0;
                    for (const auto& mpduNumToCorruptPerSta :
                         m_params.mpdusToCorruptPerPsdu.cbegin()->second)
                    {
                        for (const auto mpduNumToCorrupt : mpduNumToCorruptPerSta.second)
                        {
                            const uint16_t mpduSeqNum = mpduNumToCorrupt - 1;
                            expectedLastSeqNum = std::max(mpduSeqNum, expectedLastSeqNum);
                        }
                    }
                    if (!m_params.expectedDroppedGroupcastMpdus.empty() &&
                        m_totalTx <= m_expectedMaxNumMpdusInPsdu)
                    {
                        expectedLastSeqNum += m_totalTx;
                    }
                }
            }
            NS_TEST_EXPECT_MSG_EQ(m_lastTxSeq,
                                  expectedLastSeqNum,
                                  "Incorrect last sequence number");
        }
    }
    else if (!mpdu->GetHeader().GetAddr1().IsBroadcast() && mpdu->GetHeader().IsQosData())
    {
        NS_TEST_EXPECT_MSG_EQ(nodeId, 0, "Unicast transmission from unexpected node");
        NS_LOG_INFO("AP: unicast transmission (#MPDUs=" << psdu->GetNMpdus() << ")");
    }
    else if (mpdu->GetHeader().IsBlockAckReq())
    {
        CtrlBAckRequestHeader blockAckReq;
        mpdu->GetPacket()->PeekHeader(blockAckReq);
        NS_TEST_EXPECT_MSG_EQ(nodeId, 0, "Groupcast transmission from unexpected node");
        uint8_t staId = 0;
        uint8_t numStas = m_params.stas.size();
        for (uint8_t i = 0; i < numStas; ++i)
        {
            if (mpdu->GetHeader().GetAddr1() == m_stasWifiMac.at(i)->GetAddress())
            {
                staId = i + 1;
                break;
            }
        }
        NS_ASSERT(staId != 0);
        NS_LOG_INFO("AP: send " << (blockAckReq.IsGcr() ? "GCR " : "") << "BAR to STA " << +staId);
        m_nTxGcrBar++;
        m_nTxGcrBarsInCurrentTxop++;
        const auto expectedGcr =
            m_expectGcrUsed && ((m_params.numUnicastPackets == 0) ||
                                ((m_params.startUnicast < m_params.startGroupcast) &&
                                 (Simulator::Now() > m_params.startGroupcast)) ||
                                ((m_params.startGroupcast < m_params.startUnicast) &&
                                 (Simulator::Now() < m_params.startUnicast)));
        NS_ASSERT(blockAckReq.IsGcr() == expectedGcr);
        NS_TEST_EXPECT_MSG_EQ(blockAckReq.IsGcr(),
                              expectedGcr,
                              "Expected GCR Block Ack request type sent to STA " << +staId);
        if (blockAckReq.IsGcr())
        {
            const auto expectedStartingSequence =
                ((!m_params.mpdusToCorruptPerPsdu.empty() &&
                  !m_params.expectedDroppedGroupcastMpdus.empty() &&
                  m_nTxGcrBar > m_params.mpdusToCorruptPerPsdu.size())
                     ? m_params.numGroupcastPackets
                     : m_firstTxSeq);
            NS_ASSERT(blockAckReq.GetStartingSequence() == expectedStartingSequence);
            NS_TEST_EXPECT_MSG_EQ(
                blockAckReq.GetStartingSequence(),
                expectedStartingSequence,
                "Incorrect starting sequence in GCR Block Ack request sent to STA " << +staId);
            bool isBarRetry = (m_gcrBaParams.barsToCorrupt.count(m_nTxGcrBar - 1) != 0) ||
                              (m_gcrBaParams.blockAcksToCorrupt.count(m_nTxGcrBlockAck) != 0);
            NS_TEST_EXPECT_MSG_EQ(mpdu->GetHeader().IsRetry(),
                                  isBarRetry,
                                  "Incorrect retry flag set for GCR Block Ack Request");
            if (const auto it = m_gcrBaParams.barsToCorrupt.find(m_nTxGcrBar);
                it != m_gcrBaParams.barsToCorrupt.cend())
            {
                NS_LOG_INFO("Corrupt BAR #" << +m_nTxGcrBar);
                const auto uid = mpdu->GetPacket()->GetUid();
                for (auto& errorModel : m_errorModels)
                {
                    errorModel->SetList({uid});
                }
            }
            else
            {
                NS_LOG_INFO("Do not corrupt BAR #" << +m_nTxGcrBar);
                for (auto& errorModel : m_errorModels)
                {
                    errorModel->SetList({});
                }
            }
        }
    }
    else if (mpdu->GetHeader().IsBlockAck())
    {
        CtrlBAckResponseHeader blockAck;
        mpdu->GetPacket()->PeekHeader(blockAck);
        NS_TEST_EXPECT_MSG_NE(nodeId, 0, "BlockAck transmission from unexpected node");
        NS_LOG_INFO("STA" << nodeId << ": send " << (blockAck.IsGcr() ? "GCR " : "")
                          << "Block ACK");
        const auto expectedGcr = ((m_params.numUnicastPackets == 0) ||
                                  ((m_params.startUnicast < m_params.startGroupcast) &&
                                   (Simulator::Now() > m_params.startGroupcast)) ||
                                  ((m_params.startGroupcast < m_params.startUnicast) &&
                                   (Simulator::Now() < m_params.startUnicast)));
        NS_TEST_EXPECT_MSG_EQ(blockAck.IsGcr(),
                              expectedGcr,
                              "Expected " << (expectedGcr ? "GCR " : "")
                                          << "Block Ack type sent from STA " << nodeId);
        if (expectedGcr)
        {
            m_nTxGcrBlockAck++;
            const auto& corruptedMpdusForSta =
                (m_params.mpdusToCorruptPerPsdu.empty() ||
                 (m_params.mpdusToCorruptPerPsdu.size() < m_totalTx))
                    ? std::set<uint8_t>{}
                    : ((m_params.mpdusToCorruptPerPsdu.at(m_totalTx).count(0) != 0)
                           ? m_params.mpdusToCorruptPerPsdu.at(m_totalTx).at(0)
                           : ((m_params.mpdusToCorruptPerPsdu.at(m_totalTx).count(nodeId) != 0)
                                  ? m_params.mpdusToCorruptPerPsdu.at(m_totalTx).at(nodeId)
                                  : std::set<uint8_t>{}));
            for (int seq = m_firstTxSeq; seq <= m_lastTxSeq; ++seq)
            {
                auto expectedReceived =
                    (corruptedMpdusForSta.empty() || corruptedMpdusForSta.count(seq + 1) == 0);
                NS_TEST_EXPECT_MSG_EQ(
                    blockAck.IsPacketReceived(seq, 0),
                    expectedReceived,
                    "Incorrect bitmap filled in GCR Block Ack response sent from STA " << nodeId);
            }
        }
        else
        {
            m_nTxBlockAck++;
        }
        if (blockAck.IsGcr())
        {
            if (m_gcrBaParams.blockAcksToCorrupt.count(m_nTxGcrBlockAck))
            {
                NS_LOG_INFO("Corrupt Block ACK #" << +m_nTxGcrBlockAck);
                const auto uid = mpdu->GetPacket()->GetUid();
                m_apErrorModel->SetList({uid});
            }
            else
            {
                NS_LOG_INFO("Do not corrupt Block ACK #" << +m_nTxGcrBlockAck);
                m_apErrorModel->SetList({});
            }
        }
    }
    GcrTestBase::Transmit(context, psduMap, txVector, txPowerW);
}

void
GcrBaTest::NotifyTxopTerminated(Time startTime, Time duration, uint8_t linkId)
{
    GcrTestBase::NotifyTxopTerminated(startTime, duration, linkId);
    if (m_nTxGcrBarsInCurrentTxop > 0)
    {
        m_nTxGcrBarsPerTxop.push_back(m_nTxGcrBarsInCurrentTxop);
    }
    m_nTxGcrBarsInCurrentTxop = 0;
}

void
GcrBaTest::Receive(std::string context, Ptr<const Packet> p, const Address& adr)
{
    const auto staId = ConvertContextToNodeId(context) - 1;
    const auto socketAddress = PacketSocketAddress::ConvertFrom(adr);
    if (socketAddress.GetProtocol() == MULTICAST_PROTOCOL)
    {
        NS_LOG_INFO("STA" << staId + 1 << ": multicast packet forwarded up");
        const auto txopLimitAllowsAggregation =
            (m_params.txopLimit.IsZero() || m_params.txopLimit > MicroSeconds(320));
        m_rxGroupcastPerSta.at(staId).push_back(
            (GetNumNonHtStas(m_params.stas) == 0) && txopLimitAllowsAggregation
                ? (m_totalTx - (m_lastTxSeq / m_expectedMaxNumMpdusInPsdu))
                : 1);
    }
    else if (socketAddress.GetProtocol() == UNICAST_PROTOCOL)
    {
        NS_LOG_INFO("STA" << staId + 1 << ": unicast packet forwarded up");
        m_rxUnicastPerSta.at(staId)++;
    }
}

void
GcrBaTest::ConfigureGcrManager(WifiMacHelper& macHelper)
{
    macHelper.SetGcrManager("ns3::WifiDefaultGcrManager",
                            "RetransmissionPolicy",
                            StringValue("GCR_BA"),
                            "GcrProtectionMode",
                            EnumValue(m_params.gcrProtectionMode));
}

void
GcrBaTest::CheckResults()
{
    GcrTestBase::CheckResults();

    if (m_params.numUnicastPackets > 0)
    {
        NS_TEST_EXPECT_MSG_EQ(+m_nTxBlockAck,
                              ((m_params.numUnicastPackets > 1) ? GetNumGcrStas(m_params.stas) : 0),
                              "Incorrect number of transmitted BlockAck frames");
    }

    const auto txopLimitAllowsAggregation =
        (m_params.txopLimit.IsZero() || m_params.txopLimit > MicroSeconds(320));
    auto expectedTotalTx =
        m_expectGcrUsed && txopLimitAllowsAggregation && (GetNumNonHtStas(m_params.stas) == 0)
            ? m_params.mpdusToCorruptPerPsdu.empty()
                  ? std::ceil(static_cast<double>(m_params.numGroupcastPackets -
                                                  m_params.expectedDroppedGroupcastMpdus.size()) /
                              m_expectedMaxNumMpdusInPsdu)
                  : (std::ceil(static_cast<double>(m_params.numGroupcastPackets) /
                               m_expectedMaxNumMpdusInPsdu) +
                     std::ceil(static_cast<double>(m_params.mpdusToCorruptPerPsdu.size()) /
                               m_expectedMaxNumMpdusInPsdu))
            : m_params.numGroupcastPackets;

    const uint8_t numExpectedBars =
        m_expectGcrUsed
            ? (m_params.mpdusToCorruptPerPsdu.empty()
                   ? ((GetNumGcrStas(m_params.stas) * expectedTotalTx) +
                      m_gcrBaParams.barsToCorrupt.size() + m_gcrBaParams.blockAcksToCorrupt.size())
                   : ((GetNumGcrStas(m_params.stas) * expectedTotalTx) +
                      m_gcrBaParams.barsToCorrupt.size() + m_gcrBaParams.blockAcksToCorrupt.size() +
                      m_params.expectedDroppedGroupcastMpdus.size()))
            : 0;
    const uint8_t numExpectedBlockAcks =
        m_expectGcrUsed ? (m_params.mpdusToCorruptPerPsdu.empty()
                               ? ((GetNumGcrStas(m_params.stas) * expectedTotalTx) +
                                  m_gcrBaParams.blockAcksToCorrupt.size())
                               : ((GetNumGcrStas(m_params.stas) * expectedTotalTx) +
                                  m_gcrBaParams.blockAcksToCorrupt.size() +
                                  m_params.expectedDroppedGroupcastMpdus.size()))
                        : 0;
    uint8_t numNonConcealedTx = 0;
    if (m_expectGcrUsed && (GetNumNonHtStas(m_params.stas) > 0))
    {
        numNonConcealedTx = expectedTotalTx;
    }
    else if (m_expectGcrUsed && (GetNumNonGcrStas(m_params.stas) > 0))
    {
        numNonConcealedTx = 1;
    }
    NS_TEST_EXPECT_MSG_EQ(+m_totalTx,
                          expectedTotalTx + numNonConcealedTx,
                          "Incorrect number of transmitted packets");
    NS_TEST_EXPECT_MSG_EQ(+m_nTxGcrBar,
                          +numExpectedBars,
                          "Incorrect number of transmitted GCR BARs");
    NS_TEST_EXPECT_MSG_EQ(+m_nTxGcrBlockAck,
                          +numExpectedBlockAcks,
                          "Incorrect number of transmitted GCR Block ACKs");

    if (!m_gcrBaParams.expectedNTxBarsPerTxop.empty())
    {
        NS_TEST_EXPECT_MSG_EQ(m_nTxGcrBarsPerTxop.size(),
                              m_gcrBaParams.expectedNTxBarsPerTxop.size(),
                              "Incorrect number of TXOPs containing transmission of BAR frame(s)");
        for (std::size_t i = 0; i < m_gcrBaParams.expectedNTxBarsPerTxop.size(); ++i)
        {
            NS_TEST_EXPECT_MSG_EQ(+m_nTxGcrBarsPerTxop.at(i),
                                  +m_gcrBaParams.expectedNTxBarsPerTxop.at(i),
                                  "Incorrect number of BAR(s) transmitted in TXOP");
        }
    }

    uint8_t numStas = m_params.stas.size();
    for (uint8_t i = 0; i < numStas; ++i)
    {
        // calculate the amount of corrupted PSDUs and the expected number of retransmission per
        // MPDU
        uint8_t prevExpectedNumAttempt = 1;
        uint8_t prevPsduNum = 1;
        uint8_t droppedPsdus = 0;
        auto prevDropped = false;
        for (uint16_t j = 0; j < m_params.numGroupcastPackets; ++j)
        {
            uint8_t expectedNumAttempt = 1;
            const auto psduNum = ((j / m_params.maxNumMpdusInPsdu) + 1);
            const auto packetInAmpdu =
                (m_params.maxNumMpdusInPsdu > 1) ? ((j % m_params.maxNumMpdusInPsdu) + 1) : 1;
            if (psduNum > prevPsduNum)
            {
                prevExpectedNumAttempt = 1;
            }
            prevPsduNum = psduNum;
            for (auto& mpduToCorruptPerPsdu : m_params.mpdusToCorruptPerPsdu)
            {
                if (mpduToCorruptPerPsdu.first <= (psduNum - 1))
                {
                    continue;
                }
                const auto& corruptedMpdusForSta =
                    (mpduToCorruptPerPsdu.second.count(0) != 0)
                        ? mpduToCorruptPerPsdu.second.at(0)
                        : ((mpduToCorruptPerPsdu.second.count(i + 1) != 0)
                               ? mpduToCorruptPerPsdu.second.at(i + 1)
                               : std::set<uint8_t>{});
                if (corruptedMpdusForSta.count(packetInAmpdu) == 0)
                {
                    break;
                }
                expectedNumAttempt++;
            }
            if ((!m_expectGcrUsed && (expectedNumAttempt > 1)) ||
                (m_params.expectedDroppedGroupcastMpdus.count(j + 1) != 0))
            {
                droppedPsdus++;
                prevDropped = true;
                continue;
            }
            expectedNumAttempt = (prevDropped && !m_params.mpdusToCorruptPerPsdu.empty())
                                     ? m_params.mpdusToCorruptPerPsdu.size()
                                     : std::max(expectedNumAttempt, prevExpectedNumAttempt);
            prevExpectedNumAttempt = expectedNumAttempt;
            const std::size_t rxPsdus = (j - droppedPsdus);
            NS_TEST_EXPECT_MSG_EQ(+m_rxGroupcastPerSta.at(i).at(rxPsdus),
                                  +expectedNumAttempt,
                                  "Packet has not been forwarded up at the expected TX attempt");
        }
        const std::size_t rxPackets = (m_params.numGroupcastPackets - droppedPsdus);
        NS_TEST_EXPECT_MSG_EQ(+m_rxGroupcastPerSta.at(i).size(),
                              rxPackets,
                              "STA" + std::to_string(i + 1) +
                                  " did not receive the expected number of groupcast packets");
    }

    auto rsm = DynamicCast<IdealWifiManagerForGcrTest>(m_apWifiMac->GetWifiRemoteStationManager());
    NS_ASSERT(rsm);
    NS_TEST_EXPECT_MSG_EQ(rsm->m_blockAckSenders.size(),
                          GetNumGcrStas(m_params.stas),
                          "RSM have not received Block ACK from all members");
}

WifiGcrTestSuite::WifiGcrTestSuite()
    : TestSuite("wifi-gcr", Type::UNIT)
{
    using StationsScenarios = std::vector<std::vector<GcrTestBase::StaInfo>>;

    // GCR Unsolicited Retries
    for (auto& [useAmpdu, ampduScenario] :
         std::vector<std::pair<bool, std::string>>{{false, "A-MPDU disabled"},
                                                   {true, "A-MPDU enabled"}})
    {
        for (auto& [rtsThreshold, gcrPotection, protectionName] :
             std::vector<std::tuple<uint32_t, GroupcastProtectionMode, std::string>>{
                 {maxRtsCtsThreshold, GroupcastProtectionMode::RTS_CTS, "no protection"},
                 {500, GroupcastProtectionMode::RTS_CTS, "RTS-CTS"},
                 {1500, GroupcastProtectionMode::CTS_TO_SELF, "CTS-TO-SELF"}})
        {
            for (const auto& stasInfo : StationsScenarios{
                     {{{GCR_INCAPABLE_STA, WIFI_STANDARD_80211a}}},
                     {{{GCR_CAPABLE_STA, WIFI_STANDARD_80211n, MHz_u{40}, 2, NanoSeconds(400)}}},
                     {{{GCR_CAPABLE_STA, WIFI_STANDARD_80211ac}}},
                     {{{GCR_CAPABLE_STA, WIFI_STANDARD_80211ax}}},
                     {{{GCR_CAPABLE_STA, WIFI_STANDARD_80211be}}},
                     {{{GCR_CAPABLE_STA, WIFI_STANDARD_80211ax, MHz_u{80}, 1, NanoSeconds(800)},
                       {GCR_CAPABLE_STA, WIFI_STANDARD_80211be, MHz_u{80}, 1, NanoSeconds(3200)}}},
                     {{{GCR_CAPABLE_STA, WIFI_STANDARD_80211n, MHz_u{20}, 1},
                       {GCR_CAPABLE_STA, WIFI_STANDARD_80211ac, MHz_u{80}, 2},
                       {GCR_CAPABLE_STA, WIFI_STANDARD_80211ax, MHz_u{160}, 3}}},
                     {{{GCR_INCAPABLE_STA, WIFI_STANDARD_80211a},
                       {GCR_CAPABLE_STA, WIFI_STANDARD_80211n}}},
                     {{{GCR_INCAPABLE_STA, WIFI_STANDARD_80211n},
                       {GCR_CAPABLE_STA, WIFI_STANDARD_80211n}}}})
            {
                const auto maxChannelWidth =
                    std::max_element(stasInfo.cbegin(),
                                     stasInfo.cend(),
                                     [](const auto& lhs, const auto& rhs) {
                                         return lhs.maxChannelWidth < rhs.maxChannelWidth;
                                     })
                        ->maxChannelWidth;
                const auto useSpectrum =
                    std::any_of(stasInfo.cbegin(),
                                stasInfo.cend(),
                                [maxChannelWidth](const auto& staInfo) {
                                    return (staInfo.maxChannelWidth != maxChannelWidth);
                                });
                const std::string scenario = "STAs=" + printStasInfo(stasInfo) +
                                             ", protection=" + protectionName + ", " +
                                             ampduScenario;
                AddTestCase(
                    new GcrUrTest("GCR-UR without any lost frames: " + scenario,
                                  {.stas = stasInfo,
                                   .numGroupcastPackets = useAmpdu ? uint16_t(4) : uint16_t(2),
                                   .maxNumMpdusInPsdu = useAmpdu ? uint16_t(2) : uint16_t(1),
                                   .rtsThreshold = rtsThreshold,
                                   .gcrProtectionMode = gcrPotection},
                                  {}),
                    useSpectrum ? TestCase::Duration::EXTENSIVE : TestCase::Duration::QUICK);
                AddTestCase(
                    new GcrUrTest("GCR-UR with first frame lost: " + scenario,
                                  {.stas = stasInfo,
                                   .numGroupcastPackets = useAmpdu ? uint16_t(4) : uint16_t(2),
                                   .maxNumMpdusInPsdu = useAmpdu ? uint16_t(2) : uint16_t(1),
                                   .rtsThreshold = rtsThreshold,
                                   .gcrProtectionMode = gcrPotection,
                                   // if no MPDU aggregation, MPDUs list is ignored
                                   .mpdusToCorruptPerPsdu = {{1, {{0, {1, 2}}}}}},
                                  {}),
                    useSpectrum ? TestCase::Duration::EXTENSIVE : TestCase::Duration::QUICK);
                AddTestCase(
                    new GcrUrTest("GCR-UR with all but last frame lost: " + scenario,
                                  {.stas = stasInfo,
                                   .numGroupcastPackets = useAmpdu ? uint16_t(4) : uint16_t(2),
                                   .maxNumMpdusInPsdu = useAmpdu ? uint16_t(2) : uint16_t(1),
                                   .rtsThreshold = rtsThreshold,
                                   .gcrProtectionMode = gcrPotection,
                                   // if no MPDU aggregation, MPDUs list is ignored
                                   .mpdusToCorruptPerPsdu = {{1, {{0, {1, 2}}}},
                                                             {2, {{0, {1, 2}}}},
                                                             {3, {{0, {1, 2}}}},
                                                             {4, {{0, {1, 2}}}},
                                                             {5, {{0, {1, 2}}}},
                                                             {6, {{0, {1, 2}}}},
                                                             {7, {{0, {1, 2}}}}}},
                                  {}),
                    useSpectrum ? TestCase::Duration::EXTENSIVE : TestCase::Duration::QUICK);
                AddTestCase(
                    new GcrUrTest("GCR-UR with all frames lost: " + scenario,
                                  {.stas = stasInfo,
                                   .numGroupcastPackets = useAmpdu ? uint16_t(4) : uint16_t(2),
                                   .maxNumMpdusInPsdu = useAmpdu ? uint16_t(2) : uint16_t(1),
                                   .rtsThreshold = rtsThreshold,
                                   .gcrProtectionMode = gcrPotection,
                                   // if no MPDU aggregation, MPDUs list is ignored
                                   .mpdusToCorruptPerPsdu = {{1, {{0, {1, 2}}}},
                                                             {2, {{0, {1, 2}}}},
                                                             {3, {{0, {1, 2}}}},
                                                             {4, {{0, {1, 2}}}},
                                                             {5, {{0, {1, 2}}}},
                                                             {6, {{0, {1, 2}}}},
                                                             {7, {{0, {1, 2}}}},
                                                             {8, {{0, {1, 2}}}}}},
                                  {}),
                    useSpectrum ? TestCase::Duration::EXTENSIVE : TestCase::Duration::QUICK);
                if ((GetNumNonGcrStas(stasInfo) == 0) && useAmpdu)
                {
                    AddTestCase(
                        new GcrUrTest("GCR-UR with 1 MPDU always corrupted in first A-MPDU but one "
                                      "different MPDU alternatively, starting with second MPDU: " +
                                          scenario,
                                      {.stas = stasInfo,
                                       .numGroupcastPackets = 4,
                                       .maxNumMpdusInPsdu = 2,
                                       .rtsThreshold = rtsThreshold,
                                       .gcrProtectionMode = gcrPotection,
                                       .mpdusToCorruptPerPsdu = {{1, {{0, {2}}}},
                                                                 {2, {{0, {1}}}},
                                                                 {3, {{0, {2}}}},
                                                                 {4, {{0, {1}}}},
                                                                 {5, {{0, {2}}}},
                                                                 {6, {{0, {1}}}},
                                                                 {7, {{0, {2}}}},
                                                                 {8, {{0, {1}}}}}},
                                      {}),
                        useSpectrum ? TestCase::Duration::EXTENSIVE : TestCase::Duration::QUICK);
                    AddTestCase(
                        new GcrUrTest("GCR-UR with 1 MPDU always corrupted in first A-MPDU but one "
                                      "different MPDU alternatively, starting with first MPDU: " +
                                          scenario,
                                      {.stas = stasInfo,
                                       .numGroupcastPackets = 4,
                                       .maxNumMpdusInPsdu = 2,
                                       .rtsThreshold = rtsThreshold,
                                       .gcrProtectionMode = gcrPotection,
                                       .mpdusToCorruptPerPsdu = {{1, {{0, {1}}}},
                                                                 {2, {{0, {2}}}},
                                                                 {3, {{0, {1}}}},
                                                                 {4, {{0, {2}}}},
                                                                 {5, {{0, {1}}}},
                                                                 {6, {{0, {2}}}},
                                                                 {7, {{0, {1}}}},
                                                                 {8, {{0, {2}}}}}},
                                      {}),
                        useSpectrum ? TestCase::Duration::EXTENSIVE : TestCase::Duration::QUICK);
                    AddTestCase(
                        new GcrUrTest("GCR-UR with all MPDUs always corrupted in first A-MPDU "
                                      "except the first MPDU in the last retransmission: " +
                                          scenario,
                                      {.stas = stasInfo,
                                       .numGroupcastPackets = 4,
                                       .maxNumMpdusInPsdu = 2,
                                       .rtsThreshold = rtsThreshold,
                                       .gcrProtectionMode = gcrPotection,
                                       .mpdusToCorruptPerPsdu = {{1, {{0, {1, 2}}}},
                                                                 {2, {{0, {1, 2}}}},
                                                                 {3, {{0, {1, 2}}}},
                                                                 {4, {{0, {1, 2}}}},
                                                                 {5, {{0, {1, 2}}}},
                                                                 {6, {{0, {1, 2}}}},
                                                                 {7, {{0, {1, 2}}}},
                                                                 {8, {{0, {2}}}}}},
                                      {}),
                        useSpectrum ? TestCase::Duration::EXTENSIVE : TestCase::Duration::QUICK);
                    AddTestCase(
                        new GcrUrTest("GCR-UR with all MPDUs always corrupted in first A-MPDU "
                                      "except the second MPDU in the last retransmission: " +
                                          scenario,
                                      {.stas = stasInfo,
                                       .numGroupcastPackets = 4,
                                       .maxNumMpdusInPsdu = 2,
                                       .rtsThreshold = rtsThreshold,
                                       .gcrProtectionMode = gcrPotection,
                                       .mpdusToCorruptPerPsdu = {{1, {{0, {1, 2}}}},
                                                                 {2, {{0, {1, 2}}}},
                                                                 {3, {{0, {1, 2}}}},
                                                                 {4, {{0, {1, 2}}}},
                                                                 {5, {{0, {1, 2}}}},
                                                                 {6, {{0, {1, 2}}}},
                                                                 {7, {{0, {1, 2}}}},
                                                                 {8, {{0, {1}}}}}},
                                      {}),
                        useSpectrum ? TestCase::Duration::EXTENSIVE : TestCase::Duration::QUICK);
                    AddTestCase(
                        new GcrUrTest("GCR-UR with all MPDUs always corrupted in first A-MPDU: " +
                                          scenario,
                                      {.stas = stasInfo,
                                       .numGroupcastPackets = 4,
                                       .maxNumMpdusInPsdu = 2,
                                       .rtsThreshold = rtsThreshold,
                                       .gcrProtectionMode = gcrPotection,
                                       .mpdusToCorruptPerPsdu = {{1, {{0, {1, 2}}}},
                                                                 {2, {{0, {1, 2}}}},
                                                                 {3, {{0, {1, 2}}}},
                                                                 {4, {{0, {1, 2}}}},
                                                                 {5, {{0, {1, 2}}}},
                                                                 {6, {{0, {1, 2}}}},
                                                                 {7, {{0, {1, 2}}}},
                                                                 {8, {{0, {1, 2}}}}}},
                                      {}),
                        useSpectrum ? TestCase::Duration::EXTENSIVE : TestCase::Duration::QUICK);
                    AddTestCase(new GcrUrTest(
                                    "GCR-UR with 1 MPDU always corrupted in second A-MPDU but one "
                                    "different MPDU alternatively, starting with second MPDU: " +
                                        scenario,
                                    {.stas = stasInfo,
                                     .numGroupcastPackets = 4,
                                     .maxNumMpdusInPsdu = 2,
                                     .rtsThreshold = rtsThreshold,
                                     .gcrProtectionMode = gcrPotection,
                                     .mpdusToCorruptPerPsdu = {{9, {{0, {2}}}},
                                                               {10, {{0, {1}}}},
                                                               {11, {{0, {2}}}},
                                                               {12, {{0, {1}}}},
                                                               {13, {{0, {2}}}},
                                                               {14, {{0, {1}}}},
                                                               {15, {{0, {2}}}},
                                                               {16, {{0, {1}}}}}},
                                    {}),
                                useSpectrum ? TestCase::Duration::EXTENSIVE
                                            : TestCase::Duration::QUICK);
                    AddTestCase(new GcrUrTest(
                                    "GCR-UR with 1 MPDU always corrupted in second A-MPDU but one "
                                    "different MPDU alternatively, starting with first MPDU: " +
                                        scenario,
                                    {.stas = stasInfo,
                                     .numGroupcastPackets = 4,
                                     .maxNumMpdusInPsdu = 2,
                                     .rtsThreshold = rtsThreshold,
                                     .gcrProtectionMode = gcrPotection,
                                     .mpdusToCorruptPerPsdu = {{9, {{0, {1}}}},
                                                               {10, {{0, {2}}}},
                                                               {11, {{0, {1}}}},
                                                               {12, {{0, {2}}}},
                                                               {13, {{0, {1}}}},
                                                               {14, {{0, {2}}}},
                                                               {15, {{0, {1}}}},
                                                               {16, {{0, {2}}}}}},
                                    {}),
                                useSpectrum ? TestCase::Duration::EXTENSIVE
                                            : TestCase::Duration::QUICK);
                    AddTestCase(
                        new GcrUrTest("GCR-UR with all MPDUs always corrupted in second A-MPDU "
                                      "except the first MPDU in the last retransmission: " +
                                          scenario,
                                      {.stas = stasInfo,
                                       .numGroupcastPackets = 4,
                                       .maxNumMpdusInPsdu = 2,
                                       .rtsThreshold = rtsThreshold,
                                       .gcrProtectionMode = gcrPotection,
                                       .mpdusToCorruptPerPsdu = {{9, {{0, {1, 2}}}},
                                                                 {10, {{0, {1, 2}}}},
                                                                 {11, {{0, {1, 2}}}},
                                                                 {12, {{0, {1, 2}}}},
                                                                 {13, {{0, {1, 2}}}},
                                                                 {14, {{0, {1, 2}}}},
                                                                 {15, {{0, {1, 2}}}},
                                                                 {16, {{0, {2}}}}}},
                                      {}),
                        useSpectrum ? TestCase::Duration::EXTENSIVE : TestCase::Duration::QUICK);
                    AddTestCase(
                        new GcrUrTest("GCR-UR with all MPDUs always corrupted in second A-MPDU "
                                      "except the second MPDU in the last retransmission: " +
                                          scenario,
                                      {.stas = stasInfo,
                                       .numGroupcastPackets = 4,
                                       .maxNumMpdusInPsdu = 2,
                                       .rtsThreshold = rtsThreshold,
                                       .gcrProtectionMode = gcrPotection,
                                       .mpdusToCorruptPerPsdu = {{9, {{0, {1, 2}}}},
                                                                 {10, {{0, {1, 2}}}},
                                                                 {11, {{0, {1, 2}}}},
                                                                 {12, {{0, {1, 2}}}},
                                                                 {13, {{0, {1, 2}}}},
                                                                 {14, {{0, {1, 2}}}},
                                                                 {15, {{0, {1, 2}}}},
                                                                 {16, {{0, {1}}}}}},
                                      {}),
                        useSpectrum ? TestCase::Duration::EXTENSIVE : TestCase::Duration::QUICK);
                    AddTestCase(
                        new GcrUrTest("GCR-UR with all MPDUs always corrupted in second A-MPDU: " +
                                          scenario,
                                      {.stas = stasInfo,
                                       .numGroupcastPackets = 4,
                                       .maxNumMpdusInPsdu = 2,
                                       .rtsThreshold = rtsThreshold,
                                       .gcrProtectionMode = gcrPotection,
                                       .mpdusToCorruptPerPsdu = {{9, {{0, {1, 2}}}},
                                                                 {10, {{0, {1, 2}}}},
                                                                 {11, {{0, {1, 2}}}},
                                                                 {12, {{0, {1, 2}}}},
                                                                 {13, {{0, {1, 2}}}},
                                                                 {14, {{0, {1, 2}}}},
                                                                 {15, {{0, {1, 2}}}},
                                                                 {16, {{0, {1, 2}}}}}},
                                      {}),
                        useSpectrum ? TestCase::Duration::EXTENSIVE : TestCase::Duration::QUICK);
                }
            }
        }
    }
    AddTestCase(new GcrUrTest("GCR-UR with 4 skipped retries because of lifetime limit",
                              {.stas = {{GCR_CAPABLE_STA, WIFI_STANDARD_80211n}},
                               .numGroupcastPackets = 1,
                               .maxNumMpdusInPsdu = 1,
                               .maxLifetime = MilliSeconds(1),
                               .rtsThreshold = maxRtsCtsThreshold},
                              {.expectedSkippedRetries = 4}),
                TestCase::Duration::QUICK);
    AddTestCase(new GcrUrTest("GCR-UR with A-MPDU paused during test and number of packets larger "
                              "than MPDU buffer size",
                              {.stas = {{GCR_CAPABLE_STA, WIFI_STANDARD_80211n}},
                               .numGroupcastPackets = 300,
                               .maxNumMpdusInPsdu = 2,
                               .startGroupcast = Seconds(1.0),
                               .maxLifetime = MilliSeconds(500),
                               .rtsThreshold = maxRtsCtsThreshold,
                               .duration = Seconds(3.0)},
                              {.packetsPauzeAggregation = 4, .packetsResumeAggregation = 100}),
                TestCase::Duration::QUICK);
    AddTestCase(new GcrUrTest("GCR-UR with buffer size limit to 64 MPDUs",
                              {.stas = {{GCR_CAPABLE_STA, WIFI_STANDARD_80211n},
                                        {GCR_CAPABLE_STA, WIFI_STANDARD_80211ac},
                                        {GCR_CAPABLE_STA, WIFI_STANDARD_80211ax},
                                        {GCR_CAPABLE_STA, WIFI_STANDARD_80211be}},
                               .numGroupcastPackets = 300,
                               .packetSize = 200,
                               .maxNumMpdusInPsdu = 1024, // capped to 64 because not lowest is HT
                               .rtsThreshold = maxRtsCtsThreshold},
                              {}),
                TestCase::Duration::QUICK);
    AddTestCase(new GcrUrTest("GCR-UR with buffer size limit to 256 MPDUs",
                              {.stas = {{GCR_CAPABLE_STA, WIFI_STANDARD_80211ax},
                                        {GCR_CAPABLE_STA, WIFI_STANDARD_80211ax},
                                        {GCR_CAPABLE_STA, WIFI_STANDARD_80211be},
                                        {GCR_CAPABLE_STA, WIFI_STANDARD_80211be}},
                               .numGroupcastPackets = 300,
                               .packetSize = 200,
                               .maxNumMpdusInPsdu = 1024, // capped to 256 because not lowest is HE
                               .rtsThreshold = maxRtsCtsThreshold},
                              {}),
                TestCase::Duration::QUICK);
    AddTestCase(new GcrUrTest("GCR-UR with buffer size limit to 1024 MPDUs",
                              {.stas = {{GCR_CAPABLE_STA, WIFI_STANDARD_80211be, MHz_u{40}},
                                        {GCR_CAPABLE_STA, WIFI_STANDARD_80211be, MHz_u{40}},
                                        {GCR_CAPABLE_STA, WIFI_STANDARD_80211be, MHz_u{40}},
                                        {GCR_CAPABLE_STA, WIFI_STANDARD_80211be, MHz_u{40}}},
                               .numGroupcastPackets = 1200,
                               .packetSize = 100,
                               .maxNumMpdusInPsdu = 1024,
                               .rtsThreshold = maxRtsCtsThreshold},
                              {}),
                TestCase::Duration::QUICK);
    AddTestCase(new GcrUrTest("GCR-UR with corrupted RTS frames to verify previously assigned "
                              "sequence numbers are properly released",
                              {.stas = {{GCR_CAPABLE_STA, WIFI_STANDARD_80211n}},
                               .numGroupcastPackets = 6,
                               .packetSize = 500,
                               .maxNumMpdusInPsdu = 2,
                               .maxLifetime = MilliSeconds(
                                   1), // reduce lifetime to make sure packets get dropped
                               .rtsThreshold = 500,
                               .rtsFramesToCorrupt = {3, 4, 5},
                               .expectedDroppedGroupcastMpdus = {3, 4}},
                              {.expectedSkippedRetries = 6}),
                TestCase::Duration::QUICK);
    AddTestCase(new GcrUrTest("GCR-UR with corrupted CTS frames to verify previously assigned "
                              "sequence numbers are properly released",
                              {.stas = {{GCR_CAPABLE_STA, WIFI_STANDARD_80211n}},
                               .numGroupcastPackets = 6,
                               .packetSize = 500,
                               .maxNumMpdusInPsdu = 2,
                               .maxLifetime = MilliSeconds(
                                   1), // reduce lifetime to make sure packets get dropped
                               .rtsThreshold = 500,
                               .ctsFramesToCorrupt = {3, 4, 5},
                               .expectedDroppedGroupcastMpdus = {3, 4}},
                              {.expectedSkippedRetries = 6}),
                TestCase::Duration::QUICK);
    AddTestCase(new GcrUrTest("GCR-UR with reduced lifetime, A-MPDU paused during test and number "
                              "of packets larger than MPDU buffer size",
                              {.stas = {{GCR_CAPABLE_STA, WIFI_STANDARD_80211n}},
                               .numGroupcastPackets = 300,
                               .packetSize = 500,
                               .maxNumMpdusInPsdu = 2,
                               .maxLifetime = MilliSeconds(1),
                               .rtsThreshold = maxRtsCtsThreshold,
                               .duration = Seconds(4.0)},
                              {.expectedSkippedRetries = 4,
                               .packetsPauzeAggregation = 4,
                               .packetsResumeAggregation = 100}),
                TestCase::Duration::QUICK);

    // GCR Block ACKs
    for (auto& [groupcastPackets, groupcastStartTime, unicastPackets, unicastStartTime] :
         std::vector<std::tuple<uint16_t, Time, uint16_t, Time>>{
             {2, Seconds(1.0), 0, Seconds(0.0)},  // no unicast
             {2, Seconds(0.5), 1, Seconds(1.0)},  // groupcast then unicast
             {2, Seconds(1.0), 1, Seconds(0.5)}}) // unicast then groupcast
    {
        for (auto& [corruptedBars, corruptedBlockAcks] :
             std::vector<std::pair<std::set<uint8_t>, std::set<uint8_t>>>{{{}, {}},
                                                                          {{1}, {}},
                                                                          {{}, {1}},
                                                                          {{1}, {1}}})
        {
            for (auto& [rtsThreshold, gcrPotection, protectionName] :
                 std::vector<std::tuple<uint32_t, GroupcastProtectionMode, std::string>>{
                     {maxRtsCtsThreshold, GroupcastProtectionMode::RTS_CTS, "no protection"},
                     {500, GroupcastProtectionMode::RTS_CTS, "RTS-CTS"},
                     {1500, GroupcastProtectionMode::CTS_TO_SELF, "CTS-TO-SELF"}})
            {
                for (const auto& stasInfo : StationsScenarios{
                         {{{GCR_INCAPABLE_STA, WIFI_STANDARD_80211a}}},
                         {{{GCR_CAPABLE_STA,
                            WIFI_STANDARD_80211n,
                            MHz_u{40},
                            2,
                            NanoSeconds(400)}}},
                         {{{GCR_CAPABLE_STA, WIFI_STANDARD_80211ac}}},
                         {{{GCR_CAPABLE_STA, WIFI_STANDARD_80211ax}}},
                         {{{GCR_CAPABLE_STA, WIFI_STANDARD_80211be}}},
                         {{{GCR_CAPABLE_STA, WIFI_STANDARD_80211ax, MHz_u{80}, 1, NanoSeconds(800)},
                           {GCR_CAPABLE_STA,
                            WIFI_STANDARD_80211be,
                            MHz_u{80},
                            1,
                            NanoSeconds(3200)}}},
                         {{{GCR_CAPABLE_STA, WIFI_STANDARD_80211n, MHz_u{20}, 1},
                           {GCR_CAPABLE_STA, WIFI_STANDARD_80211ac, MHz_u{80}, 2},
                           {GCR_CAPABLE_STA, WIFI_STANDARD_80211ax, MHz_u{160}, 3}}},
                         {{{GCR_INCAPABLE_STA, WIFI_STANDARD_80211a},
                           {GCR_CAPABLE_STA, WIFI_STANDARD_80211n}}},
                         {{{GCR_INCAPABLE_STA, WIFI_STANDARD_80211n},
                           {GCR_CAPABLE_STA, WIFI_STANDARD_80211n}}}})
                {
                    const auto maxChannelWidth =
                        std::max_element(stasInfo.cbegin(),
                                         stasInfo.cend(),
                                         [](const auto& lhs, const auto& rhs) {
                                             return lhs.maxChannelWidth < rhs.maxChannelWidth;
                                         })
                            ->maxChannelWidth;
                    const auto useSpectrum =
                        std::any_of(stasInfo.cbegin(),
                                    stasInfo.cend(),
                                    [maxChannelWidth](const auto& staInfo) {
                                        return (staInfo.maxChannelWidth != maxChannelWidth);
                                    });
                    std::string scenario =
                        "STAs=" + printStasInfo(stasInfo) + ", protection=" + protectionName +
                        ", corruptBARs=" + std::to_string(!corruptedBars.empty()) +
                        ", corruptBACKs=" + std::to_string(!corruptedBlockAcks.empty());
                    if (unicastPackets > 0)
                    {
                        scenario += ", mixedGroupcastUnicast";
                        if (unicastStartTime > groupcastStartTime)
                        {
                            scenario += " (groupcast before unicast)";
                        }
                        else
                        {
                            scenario += " (unicast before groupcast)";
                        }
                    }
                    AddTestCase(new GcrBaTest("GCR-BA without any corrupted MPDUs: " + scenario,
                                              {.stas = stasInfo,
                                               .numGroupcastPackets = groupcastPackets,
                                               .numUnicastPackets = unicastPackets,
                                               .maxNumMpdusInPsdu = 2,
                                               .startGroupcast = groupcastStartTime,
                                               .startUnicast = unicastStartTime,
                                               .rtsThreshold = rtsThreshold,
                                               .gcrProtectionMode = gcrPotection},
                                              {corruptedBars, corruptedBlockAcks}),
                                useSpectrum ? TestCase::Duration::EXTENSIVE
                                            : TestCase::Duration::QUICK);
                    if (GetNumNonGcrStas(stasInfo) == 0)
                    {
                        AddTestCase(new GcrBaTest("GCR-BA with second MPDU corrupted: " + scenario,
                                                  {.stas = stasInfo,
                                                   .numGroupcastPackets = groupcastPackets,
                                                   .numUnicastPackets = unicastPackets,
                                                   .maxNumMpdusInPsdu = 2,
                                                   .startGroupcast = groupcastStartTime,
                                                   .startUnicast = unicastStartTime,
                                                   .rtsThreshold = rtsThreshold,
                                                   .gcrProtectionMode = gcrPotection,
                                                   .mpdusToCorruptPerPsdu = {{1, {{0, {2}}}}}},
                                                  {corruptedBars, corruptedBlockAcks}),
                                    useSpectrum ? TestCase::Duration::EXTENSIVE
                                                : TestCase::Duration::QUICK);
                        AddTestCase(new GcrBaTest("GCR-BA with first MPDU corrupted: " + scenario,
                                                  {.stas = stasInfo,
                                                   .numGroupcastPackets = groupcastPackets,
                                                   .numUnicastPackets = unicastPackets,
                                                   .maxNumMpdusInPsdu = 2,
                                                   .startGroupcast = groupcastStartTime,
                                                   .startUnicast = unicastStartTime,
                                                   .rtsThreshold = rtsThreshold,
                                                   .gcrProtectionMode = gcrPotection,
                                                   .mpdusToCorruptPerPsdu = {{1, {{0, {1}}}}}},
                                                  {corruptedBars, corruptedBlockAcks}),
                                    useSpectrum ? TestCase::Duration::EXTENSIVE
                                                : TestCase::Duration::QUICK);
                        AddTestCase(new GcrBaTest("GCR-BA with both MPDUs corrupted: " + scenario,
                                                  {.stas = stasInfo,
                                                   .numGroupcastPackets = groupcastPackets,
                                                   .numUnicastPackets = unicastPackets,
                                                   .maxNumMpdusInPsdu = 2,
                                                   .startGroupcast = groupcastStartTime,
                                                   .startUnicast = unicastStartTime,
                                                   .rtsThreshold = rtsThreshold,
                                                   .gcrProtectionMode = gcrPotection,
                                                   .mpdusToCorruptPerPsdu = {{1, {{0, {1, 2}}}}}},
                                                  {corruptedBars, corruptedBlockAcks}),
                                    useSpectrum ? TestCase::Duration::EXTENSIVE
                                                : TestCase::Duration::QUICK);
                        if (GetNumGcrStas(stasInfo) > 1)
                        {
                            AddTestCase(
                                new GcrBaTest("GCR-BA with second MPDU corrupted for first STA: " +
                                                  scenario,
                                              {.stas = stasInfo,
                                               .numGroupcastPackets = groupcastPackets,
                                               .numUnicastPackets = unicastPackets,
                                               .maxNumMpdusInPsdu = 2,
                                               .startGroupcast = groupcastStartTime,
                                               .startUnicast = unicastStartTime,
                                               .rtsThreshold = rtsThreshold,
                                               .gcrProtectionMode = gcrPotection,
                                               .mpdusToCorruptPerPsdu = {{1, {{1, {2}}}}}},
                                              {corruptedBars, corruptedBlockAcks}),
                                useSpectrum ? TestCase::Duration::EXTENSIVE
                                            : TestCase::Duration::QUICK);
                            AddTestCase(
                                new GcrBaTest("GCR-BA with first MPDU corrupted for first STA: " +
                                                  scenario,
                                              {.stas = stasInfo,
                                               .numGroupcastPackets = groupcastPackets,
                                               .numUnicastPackets = unicastPackets,
                                               .maxNumMpdusInPsdu = 2,
                                               .startGroupcast = groupcastStartTime,
                                               .startUnicast = unicastStartTime,
                                               .rtsThreshold = rtsThreshold,
                                               .gcrProtectionMode = gcrPotection,
                                               .mpdusToCorruptPerPsdu = {{1, {{1, {1}}}}}},
                                              {corruptedBars, corruptedBlockAcks}),
                                useSpectrum ? TestCase::Duration::EXTENSIVE
                                            : TestCase::Duration::QUICK);
                            AddTestCase(
                                new GcrBaTest(
                                    "GCR-BA with first different MPDUs corrupted for each STA: " +
                                        scenario,
                                    {.stas = stasInfo,
                                     .numGroupcastPackets = groupcastPackets,
                                     .numUnicastPackets = unicastPackets,
                                     .maxNumMpdusInPsdu = 2,
                                     .startGroupcast = groupcastStartTime,
                                     .startUnicast = unicastStartTime,
                                     .rtsThreshold = rtsThreshold,
                                     .gcrProtectionMode = gcrPotection,
                                     .mpdusToCorruptPerPsdu = {{1, {{1, {1}}, {2, {2}}}}}},
                                    {corruptedBars, corruptedBlockAcks}),
                                useSpectrum ? TestCase::Duration::EXTENSIVE
                                            : TestCase::Duration::QUICK);
                            AddTestCase(new GcrBaTest(
                                            "GCR-BA with first different MPDUs corrupted for each "
                                            "STA with different order: " +
                                                scenario,
                                            {.stas = stasInfo,
                                             .numGroupcastPackets = groupcastPackets,
                                             .numUnicastPackets = unicastPackets,
                                             .maxNumMpdusInPsdu = 2,
                                             .startGroupcast = groupcastStartTime,
                                             .startUnicast = unicastStartTime,
                                             .rtsThreshold = rtsThreshold,
                                             .gcrProtectionMode = gcrPotection,
                                             .mpdusToCorruptPerPsdu = {{1, {{1, {2}}, {2, {1}}}}}},
                                            {corruptedBars, corruptedBlockAcks}),
                                        useSpectrum ? TestCase::Duration::EXTENSIVE
                                                    : TestCase::Duration::QUICK);
                        }
                    }
                }
            }
        }
        std::string scenario = "GCR-BA with dropped MPDU because of lifetime expiry";
        if (unicastPackets > 0)
        {
            scenario += ", mixedGroupcastUnicast";
            if (unicastStartTime > groupcastStartTime)
            {
                scenario += " (groupcast before unicast)";
            }
            else
            {
                scenario += " (unicast before groupcast)";
            }
        }
        AddTestCase(
            new GcrBaTest(scenario,
                          {.stas = {{GCR_CAPABLE_STA, WIFI_STANDARD_80211n}},
                           .numGroupcastPackets =
                               uint16_t(groupcastPackets *
                                        2), // consider more packets to verify TX window is advanced
                           .numUnicastPackets = unicastPackets,
                           .maxNumMpdusInPsdu = 2,
                           .startGroupcast = groupcastStartTime,
                           .startUnicast = unicastStartTime,
                           .maxLifetime = MilliSeconds(2),
                           .rtsThreshold = maxRtsCtsThreshold,
                           .mpdusToCorruptPerPsdu =
                               {{1, {{0, {2}}}}, {2, {{0, {2}}}}, {3, {{0, {2}}}}, {4, {{0, {2}}}}},
                           .expectedDroppedGroupcastMpdus = {2}},
                          {}),
            TestCase::Duration::QUICK);
        scenario = "";
        if (unicastPackets > 0)
        {
            if (unicastStartTime > groupcastStartTime)
            {
                scenario += "Groupcast followed by unicast";
            }
            else
            {
                scenario += "Unicast followed by groupcast";
            }
        }
        else
        {
            scenario += "GCR-BA";
        }
        scenario += " with ";
        AddTestCase(new GcrBaTest(scenario + "ADDBA request corrupted",
                                  {.stas = {{GCR_CAPABLE_STA, WIFI_STANDARD_80211n}},
                                   .numGroupcastPackets = groupcastPackets,
                                   .numUnicastPackets = unicastPackets,
                                   .maxNumMpdusInPsdu = 2,
                                   .startGroupcast = groupcastStartTime,
                                   .startUnicast = unicastStartTime,
                                   .rtsThreshold = maxRtsCtsThreshold,
                                   .addbaReqsToCorrupt = {1}},
                                  {}),
                    TestCase::Duration::QUICK);
        AddTestCase(new GcrBaTest(scenario + "ADDBA response corrupted",
                                  {.stas = {{GCR_CAPABLE_STA, WIFI_STANDARD_80211n}},
                                   .numGroupcastPackets = groupcastPackets,
                                   .numUnicastPackets = unicastPackets,
                                   .maxNumMpdusInPsdu = 2,
                                   .startGroupcast = groupcastStartTime,
                                   .startUnicast = unicastStartTime,
                                   .rtsThreshold = maxRtsCtsThreshold,
                                   .addbaRespsToCorrupt = {1}},
                                  {}),
                    TestCase::Duration::QUICK);
        AddTestCase(new GcrBaTest(scenario + "ADDBA timeout",
                                  {.stas = {{GCR_CAPABLE_STA, WIFI_STANDARD_80211n}},
                                   .numGroupcastPackets = groupcastPackets,
                                   .numUnicastPackets = unicastPackets,
                                   .maxNumMpdusInPsdu = 2,
                                   .startGroupcast = groupcastStartTime,
                                   .startUnicast = unicastStartTime,
                                   .rtsThreshold = maxRtsCtsThreshold,
                                   .addbaReqsToCorrupt = {1, 2, 3, 4, 5, 6, 7, 8}},
                                  {}),
                    TestCase::Duration::QUICK);
        AddTestCase(new GcrBaTest(scenario + "DELBA frames after timeout expires",
                                  {.stas = {{GCR_CAPABLE_STA, WIFI_STANDARD_80211n}},
                                   .numGroupcastPackets = groupcastPackets,
                                   .numUnicastPackets = uint16_t(unicastPackets * 2),
                                   .maxNumMpdusInPsdu = 2,
                                   .startGroupcast = groupcastStartTime,
                                   .startUnicast = unicastStartTime,
                                   .rtsThreshold = maxRtsCtsThreshold,
                                   .baInactivityTimeout = 10},
                                  {}),
                    TestCase::Duration::QUICK);
    }
    AddTestCase(new GcrBaTest(
                    "GCR-BA with BARs sent over 2 TXOPs because of TXOP limit",
                    {.stas = {{GCR_CAPABLE_STA, WIFI_STANDARD_80211n},
                              {GCR_CAPABLE_STA, WIFI_STANDARD_80211ac},
                              {GCR_CAPABLE_STA, WIFI_STANDARD_80211ax}},
                     .numGroupcastPackets = 2,
                     .maxNumMpdusInPsdu = 2,
                     .maxLifetime = Seconds(1.0),
                     .rtsThreshold = maxRtsCtsThreshold,
                     .txopLimit = MicroSeconds(480)},
                    {.expectedNTxBarsPerTxop = {1, 2}}), // 1 BAR in first TXOP, 2 BARs in next TXOP
                TestCase::Duration::QUICK);
    AddTestCase(new GcrBaTest("GCR-BA with TXOP limit not allowing aggregation",
                              {.stas = {{GCR_CAPABLE_STA, WIFI_STANDARD_80211n},
                                        {GCR_CAPABLE_STA, WIFI_STANDARD_80211ac},
                                        {GCR_CAPABLE_STA, WIFI_STANDARD_80211ax}},
                               .numGroupcastPackets = 2,
                               .maxNumMpdusInPsdu = 2,
                               .maxLifetime = Seconds(1.0),
                               .rtsThreshold = maxRtsCtsThreshold,
                               .txopLimit = MicroSeconds(320)},
                              {.expectedNTxBarsPerTxop = {1, 2, 1, 2}}),
                TestCase::Duration::QUICK);
    AddTestCase(new GcrBaTest("GCR-BA with number of packets larger than MPDU buffer size",
                              {.stas = {{GCR_CAPABLE_STA, WIFI_STANDARD_80211n}},
                               .numGroupcastPackets = 300,
                               .maxNumMpdusInPsdu = 2,
                               .rtsThreshold = maxRtsCtsThreshold},
                              {}),
                TestCase::Duration::QUICK);
    AddTestCase(new GcrBaTest("GCR-BA with buffer size limit to 64 MPDUs",
                              {.stas = {{GCR_CAPABLE_STA, WIFI_STANDARD_80211n},
                                        {GCR_CAPABLE_STA, WIFI_STANDARD_80211ac},
                                        {GCR_CAPABLE_STA, WIFI_STANDARD_80211ax},
                                        {GCR_CAPABLE_STA, WIFI_STANDARD_80211be}},
                               .numGroupcastPackets = 300,
                               .packetSize = 500,
                               .maxNumMpdusInPsdu = 1024, // capped to 64 because not lowest is HT
                               .rtsThreshold = maxRtsCtsThreshold},
                              {}),
                TestCase::Duration::QUICK);
    AddTestCase(new GcrBaTest("GCR-BA with buffer size limit to 256 MPDUs",
                              {.stas = {{GCR_CAPABLE_STA, WIFI_STANDARD_80211ax},
                                        {GCR_CAPABLE_STA, WIFI_STANDARD_80211ax},
                                        {GCR_CAPABLE_STA, WIFI_STANDARD_80211be},
                                        {GCR_CAPABLE_STA, WIFI_STANDARD_80211be}},
                               .numGroupcastPackets = 300,
                               .packetSize = 150,
                               .maxNumMpdusInPsdu = 1024, // capped to 256 because not lowest is HE
                               .rtsThreshold = maxRtsCtsThreshold},
                              {}),

                TestCase::Duration::QUICK);
    AddTestCase(new GcrBaTest("GCR-BA with buffer size limit to 1024 MPDUs",
                              {.stas = {{GCR_CAPABLE_STA, WIFI_STANDARD_80211be, MHz_u{40}},
                                        {GCR_CAPABLE_STA, WIFI_STANDARD_80211be, MHz_u{40}},
                                        {GCR_CAPABLE_STA, WIFI_STANDARD_80211be, MHz_u{40}},
                                        {GCR_CAPABLE_STA, WIFI_STANDARD_80211be, MHz_u{40}}},
                               .numGroupcastPackets = 1200,
                               .packetSize = 100,
                               .maxNumMpdusInPsdu = 1024,
                               .rtsThreshold = maxRtsCtsThreshold},
                              {}),
                TestCase::Duration::QUICK);
    AddTestCase(new GcrBaTest("GCR-BA with corrupted RTS frames to verify previously assigned "
                              "sequence numbers are properly released",
                              {.stas = {{GCR_CAPABLE_STA, WIFI_STANDARD_80211n}},
                               .numGroupcastPackets = 6,
                               .packetSize = 500,
                               .maxNumMpdusInPsdu = 2,
                               .maxLifetime = MilliSeconds(
                                   1), // reduce lifetime to make sure packets get dropped
                               .rtsThreshold = 500,
                               .rtsFramesToCorrupt = {2, 3, 4},
                               .expectedDroppedGroupcastMpdus = {3, 4}},
                              {}),
                TestCase::Duration::QUICK);
    AddTestCase(new GcrBaTest("GCR-BA with corrupted CTS frames to verify previously assigned "
                              "sequence numbers are properly released",
                              {.stas = {{GCR_CAPABLE_STA, WIFI_STANDARD_80211n}},
                               .numGroupcastPackets = 6,
                               .packetSize = 500,
                               .maxNumMpdusInPsdu = 2,
                               .maxLifetime = MilliSeconds(
                                   1), // reduce lifetime to make sure packets get dropped
                               .rtsThreshold = 500,
                               .ctsFramesToCorrupt = {2, 3, 4},
                               .expectedDroppedGroupcastMpdus = {3, 4}},
                              {}),
                TestCase::Duration::QUICK);
}

static WifiGcrTestSuite g_wifiGcrTestSuite; ///< the test suite
