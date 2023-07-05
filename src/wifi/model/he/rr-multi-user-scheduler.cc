/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
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
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "rr-multi-user-scheduler.h"

#include "he-configuration.h"
#include "he-frame-exchange-manager.h"
#include "he-phy.h"

#include "ns3/log.h"
#include "ns3/wifi-acknowledgment.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-protection.h"
#include "ns3/wifi-psdu.h"

#include <algorithm>
#include <numeric>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RrMultiUserScheduler");

NS_OBJECT_ENSURE_REGISTERED(RrMultiUserScheduler);

TypeId
RrMultiUserScheduler::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::RrMultiUserScheduler")
            .SetParent<MultiUserScheduler>()
            .SetGroupName("Wifi")
            .AddConstructor<RrMultiUserScheduler>()
            .AddAttribute("NStations",
                          "The maximum number of stations that can be granted an RU in a DL MU "
                          "OFDMA transmission",
                          UintegerValue(4),
                          MakeUintegerAccessor(&RrMultiUserScheduler::m_nStations),
                          MakeUintegerChecker<uint8_t>(1, 74))
            .AddAttribute("EnableTxopSharing",
                          "If enabled, allow A-MPDUs of different TIDs in a DL MU PPDU.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&RrMultiUserScheduler::m_enableTxopSharing),
                          MakeBooleanChecker())
            .AddAttribute("ForceDlOfdma",
                          "If enabled, return DL_MU_TX even if no DL MU PPDU could be built.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&RrMultiUserScheduler::m_forceDlOfdma),
                          MakeBooleanChecker())
            .AddAttribute("EnableUlOfdma",
                          "If enabled, return UL_MU_TX if DL_MU_TX was returned the previous time.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&RrMultiUserScheduler::m_enableUlOfdma),
                          MakeBooleanChecker())
            .AddAttribute("EnableBsrp",
                          "If enabled, send a BSRP Trigger Frame before an UL MU transmission.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&RrMultiUserScheduler::m_enableBsrp),
                          MakeBooleanChecker())
            .AddAttribute(
                "UlPsduSize",
                "The default size in bytes of the solicited PSDU (to be sent in a TB PPDU)",
                UintegerValue(500),
                MakeUintegerAccessor(&RrMultiUserScheduler::m_ulPsduSize),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute("UseCentral26TonesRus",
                          "If enabled, central 26-tone RUs are allocated, too, when the "
                          "selected RU type is at least 52 tones.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&RrMultiUserScheduler::m_useCentral26TonesRus),
                          MakeBooleanChecker())
            .AddAttribute(
                "MaxCredits",
                "Maximum amount of credits a station can have. When transmitting a DL MU PPDU, "
                "the amount of credits received by each station equals the TX duration (in "
                "microseconds) divided by the total number of stations. Stations that are the "
                "recipient of the DL MU PPDU have to pay a number of credits equal to the TX "
                "duration (in microseconds) times the allocated bandwidth share",
                TimeValue(Seconds(1)),
                MakeTimeAccessor(&RrMultiUserScheduler::m_maxCredits),
                MakeTimeChecker());
    return tid;
}

RrMultiUserScheduler::RrMultiUserScheduler()
{
    NS_LOG_FUNCTION(this);
}

RrMultiUserScheduler::~RrMultiUserScheduler()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
RrMultiUserScheduler::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_apMac);
    m_apMac->TraceConnectWithoutContext(
        "AssociatedSta",
        MakeCallback(&RrMultiUserScheduler::NotifyStationAssociated, this));
    m_apMac->TraceConnectWithoutContext(
        "DeAssociatedSta",
        MakeCallback(&RrMultiUserScheduler::NotifyStationDeassociated, this));
    for (const auto& ac : wifiAcList)
    {
        m_staListDl.insert({ac.first, {}});
    }
    MultiUserScheduler::DoInitialize();
}

void
RrMultiUserScheduler::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_staListDl.clear();
    m_staListUl.clear();
    m_candidates.clear();
    m_txParams.Clear();
    m_apMac->TraceDisconnectWithoutContext(
        "AssociatedSta",
        MakeCallback(&RrMultiUserScheduler::NotifyStationAssociated, this));
    m_apMac->TraceDisconnectWithoutContext(
        "DeAssociatedSta",
        MakeCallback(&RrMultiUserScheduler::NotifyStationDeassociated, this));
    MultiUserScheduler::DoDispose();
}

MultiUserScheduler::TxFormat
RrMultiUserScheduler::SelectTxFormat()
{
    NS_LOG_FUNCTION(this);

    Ptr<const WifiMpdu> mpdu = m_edca->PeekNextMpdu(m_linkId);

    if (mpdu && !m_apMac->GetHeSupported(mpdu->GetHeader().GetAddr1()))
    {
        return SU_TX;
    }

    if (m_enableUlOfdma && m_enableBsrp && (GetLastTxFormat(m_linkId) == DL_MU_TX || !mpdu))
    {
        TxFormat txFormat = TrySendingBsrpTf();

        if (txFormat != DL_MU_TX)
        {
            return txFormat;
        }
    }
    else if (m_enableUlOfdma && ((GetLastTxFormat(m_linkId) == DL_MU_TX) ||
                                 (m_trigger.GetType() == TriggerFrameType::BSRP_TRIGGER) || !mpdu))
    {
        TxFormat txFormat = TrySendingBasicTf();

        if (txFormat != DL_MU_TX)
        {
            return txFormat;
        }
    }

    return TrySendingDlMuPpdu();
}

template <class Func>
WifiTxVector
RrMultiUserScheduler::GetTxVectorForUlMu(Func canBeSolicited)
{
    NS_LOG_FUNCTION(this);

    // determine RUs to allocate to stations
    auto count = std::min<std::size_t>(m_nStations, m_staListUl.size());
    std::size_t nCentral26TonesRus;
    HeRu::GetEqualSizedRusForStations(m_allowedWidth, count, nCentral26TonesRus);
    NS_ASSERT(count >= 1);

    if (!m_useCentral26TonesRus)
    {
        nCentral26TonesRus = 0;
    }

    Ptr<HeConfiguration> heConfiguration = m_apMac->GetHeConfiguration();
    NS_ASSERT(heConfiguration);

    WifiTxVector txVector;
    txVector.SetPreambleType(WIFI_PREAMBLE_HE_TB);
    txVector.SetChannelWidth(m_allowedWidth);
    txVector.SetGuardInterval(heConfiguration->GetGuardInterval().GetNanoSeconds());
    txVector.SetBssColor(heConfiguration->GetBssColor());

    // iterate over the associated stations until an enough number of stations is identified
    auto staIt = m_staListUl.begin();
    m_candidates.clear();

    while (staIt != m_staListUl.end() &&
           txVector.GetHeMuUserInfoMap().size() <
               std::min<std::size_t>(m_nStations, count + nCentral26TonesRus))
    {
        NS_LOG_DEBUG("Next candidate STA (MAC=" << staIt->address << ", AID=" << staIt->aid << ")");

        if (!canBeSolicited(*staIt))
        {
            NS_LOG_DEBUG("Skipping station based on provided function object");
            staIt++;
            continue;
        }

        if (txVector.GetPreambleType() == WIFI_PREAMBLE_EHT_TB &&
            !m_apMac->GetEhtSupported(staIt->address))
        {
            NS_LOG_DEBUG(
                "Skipping non-EHT STA because this Trigger Frame is only soliciting EHT STAs");
            staIt++;
            continue;
        }

        uint8_t tid = 0;
        while (tid < 8)
        {
            // check that a BA agreement is established with the receiver for the
            // considered TID, since ack sequences for UL MU require block ack
            if (m_apMac->GetBaAgreementEstablishedAsRecipient(staIt->address, tid))
            {
                break;
            }
            ++tid;
        }
        if (tid == 8)
        {
            NS_LOG_DEBUG("No Block Ack agreement established with " << staIt->address);
            staIt++;
            continue;
        }

        // if the first candidate STA is an EHT STA, we switch to soliciting EHT TB PPDUs
        if (txVector.GetHeMuUserInfoMap().empty())
        {
            if (m_apMac->GetEhtSupported() && m_apMac->GetEhtSupported(staIt->address))
            {
                txVector.SetPreambleType(WIFI_PREAMBLE_EHT_TB);
                txVector.SetEhtPpduType(0);
            }
            // TODO otherwise, make sure the TX width does not exceed 160 MHz
        }

        // prepare the MAC header of a frame that would be sent to the candidate station,
        // just for the purpose of retrieving the TXVECTOR used to transmit to that station
        WifiMacHeader hdr(WIFI_MAC_QOSDATA);
        hdr.SetAddr1(GetWifiRemoteStationManager(m_linkId)
                         ->GetAffiliatedStaAddress(staIt->address)
                         .value_or(staIt->address));
        hdr.SetAddr2(m_apMac->GetFrameExchangeManager(m_linkId)->GetAddress());
        WifiTxVector suTxVector =
            GetWifiRemoteStationManager(m_linkId)->GetDataTxVector(hdr, m_allowedWidth);
        txVector.SetHeMuUserInfo(staIt->aid,
                                 {HeRu::RuSpec(), // assigned later by FinalizeTxVector
                                  suTxVector.GetMode().GetMcsValue(),
                                  suTxVector.GetNss()});
        m_candidates.emplace_back(staIt, nullptr);

        // move to the next station in the list
        staIt++;
    }

    if (txVector.GetHeMuUserInfoMap().empty())
    {
        NS_LOG_DEBUG("No suitable station");
        return txVector;
    }

    FinalizeTxVector(txVector);
    return txVector;
}

MultiUserScheduler::TxFormat
RrMultiUserScheduler::TrySendingBsrpTf()
{
    NS_LOG_FUNCTION(this);

    if (m_staListUl.empty())
    {
        NS_LOG_DEBUG("No HE stations associated: return SU_TX");
        return TxFormat::SU_TX;
    }

    // only consider stations that have setup the current link
    WifiTxVector txVector = GetTxVectorForUlMu([this](const MasterInfo& info) {
        const auto& staList = m_apMac->GetStaList(m_linkId);
        return staList.find(info.aid) != staList.cend();
    });

    if (txVector.GetHeMuUserInfoMap().empty())
    {
        NS_LOG_DEBUG("No suitable station found");
        return TxFormat::DL_MU_TX;
    }

    m_trigger = CtrlTriggerHeader(TriggerFrameType::BSRP_TRIGGER, txVector);
    txVector.SetGuardInterval(m_trigger.GetGuardInterval());

    auto item = GetTriggerFrame(m_trigger, m_linkId);
    m_triggerMacHdr = item->GetHeader();

    m_txParams.Clear();
    // set the TXVECTOR used to send the Trigger Frame
    m_txParams.m_txVector =
        m_apMac->GetWifiRemoteStationManager(m_linkId)->GetRtsTxVector(m_triggerMacHdr.GetAddr1());

    if (!GetHeFem(m_linkId)->TryAddMpdu(item, m_txParams, m_availableTime))
    {
        // sending the BSRP Trigger Frame is not possible, hence return NO_TX. In
        // this way, no transmission will occur now and the next time we will
        // try again sending a BSRP Trigger Frame.
        NS_LOG_DEBUG("Remaining TXOP duration is not enough for BSRP TF exchange");
        return NO_TX;
    }

    // Compute the time taken by each station to transmit 8 QoS Null frames
    Time qosNullTxDuration = Seconds(0);
    for (const auto& userInfo : m_trigger)
    {
        Time duration = WifiPhy::CalculateTxDuration(GetMaxSizeOfQosNullAmpdu(m_trigger),
                                                     txVector,
                                                     m_apMac->GetWifiPhy(m_linkId)->GetPhyBand(),
                                                     userInfo.GetAid12());
        qosNullTxDuration = Max(qosNullTxDuration, duration);
    }

    if (m_availableTime != Time::Min())
    {
        // TryAddMpdu only considers the time to transmit the Trigger Frame
        NS_ASSERT(m_txParams.m_protection &&
                  m_txParams.m_protection->protectionTime != Time::Min());
        NS_ASSERT(m_txParams.m_acknowledgment &&
                  m_txParams.m_acknowledgment->acknowledgmentTime.IsZero());
        NS_ASSERT(m_txParams.m_txDuration != Time::Min());

        if (m_txParams.m_protection->protectionTime + m_txParams.m_txDuration // BSRP TF tx time
                + m_apMac->GetWifiPhy(m_linkId)->GetSifs() + qosNullTxDuration >
            m_availableTime)
        {
            NS_LOG_DEBUG("Remaining TXOP duration is not enough for BSRP TF exchange");
            return NO_TX;
        }
    }

    uint16_t ulLength;
    std::tie(ulLength, qosNullTxDuration) = HePhy::ConvertHeTbPpduDurationToLSigLength(
        qosNullTxDuration,
        m_trigger.GetHeTbTxVector(m_trigger.begin()->GetAid12()),
        m_apMac->GetWifiPhy(m_linkId)->GetPhyBand());
    NS_LOG_DEBUG("Duration of QoS Null frames: " << qosNullTxDuration.As(Time::MS));
    m_trigger.SetUlLength(ulLength);

    return UL_MU_TX;
}

MultiUserScheduler::TxFormat
RrMultiUserScheduler::TrySendingBasicTf()
{
    NS_LOG_FUNCTION(this);

    if (m_staListUl.empty())
    {
        NS_LOG_DEBUG("No HE stations associated: return SU_TX");
        return TxFormat::SU_TX;
    }

    // check if an UL OFDMA transmission is possible after a DL OFDMA transmission
    NS_ABORT_MSG_IF(m_ulPsduSize == 0, "The UlPsduSize attribute must be set to a non-null value");

    // only consider stations that have setup the current link and do not have
    // reported a null queue size
    WifiTxVector txVector = GetTxVectorForUlMu([this](const MasterInfo& info) {
        const auto& staList = m_apMac->GetStaList(m_linkId);
        return staList.find(info.aid) != staList.cend() &&
               m_apMac->GetMaxBufferStatus(info.address) > 0;
    });

    if (txVector.GetHeMuUserInfoMap().empty())
    {
        NS_LOG_DEBUG("No suitable station found");
        return TxFormat::DL_MU_TX;
    }

    uint32_t maxBufferSize = 0;

    for (const auto& candidate : txVector.GetHeMuUserInfoMap())
    {
        auto address = m_apMac->GetMldOrLinkAddressByAid(candidate.first);
        NS_ASSERT_MSG(address, "AID " << candidate.first << " not found");

        uint8_t queueSize = m_apMac->GetMaxBufferStatus(*address);
        if (queueSize == 255)
        {
            NS_LOG_DEBUG("Buffer status of station " << *address << " is unknown");
            maxBufferSize = std::max(maxBufferSize, m_ulPsduSize);
        }
        else if (queueSize == 254)
        {
            NS_LOG_DEBUG("Buffer status of station " << *address << " is not limited");
            maxBufferSize = 0xffffffff;
        }
        else
        {
            NS_LOG_DEBUG("Buffer status of station " << *address << " is " << +queueSize);
            maxBufferSize = std::max(maxBufferSize, static_cast<uint32_t>(queueSize * 256));
        }
    }

    if (maxBufferSize == 0)
    {
        return DL_MU_TX;
    }

    m_trigger = CtrlTriggerHeader(TriggerFrameType::BASIC_TRIGGER, txVector);
    txVector.SetGuardInterval(m_trigger.GetGuardInterval());

    auto item = GetTriggerFrame(m_trigger, m_linkId);
    m_triggerMacHdr = item->GetHeader();

    // compute the maximum amount of time that can be granted to stations.
    // This value is limited by the max PPDU duration
    Time maxDuration = GetPpduMaxTime(txVector.GetPreambleType());

    m_txParams.Clear();
    // set the TXVECTOR used to send the Trigger Frame
    m_txParams.m_txVector =
        m_apMac->GetWifiRemoteStationManager(m_linkId)->GetRtsTxVector(m_triggerMacHdr.GetAddr1());

    if (!GetHeFem(m_linkId)->TryAddMpdu(item, m_txParams, m_availableTime))
    {
        // an UL OFDMA transmission is not possible, hence return NO_TX. In
        // this way, no transmission will occur now and the next time we will
        // try again performing an UL OFDMA transmission.
        NS_LOG_DEBUG("Remaining TXOP duration is not enough for UL MU exchange");
        return NO_TX;
    }

    if (m_availableTime != Time::Min())
    {
        // TryAddMpdu only considers the time to transmit the Trigger Frame
        NS_ASSERT(m_txParams.m_protection &&
                  m_txParams.m_protection->protectionTime != Time::Min());
        NS_ASSERT(m_txParams.m_acknowledgment &&
                  m_txParams.m_acknowledgment->acknowledgmentTime != Time::Min());
        NS_ASSERT(m_txParams.m_txDuration != Time::Min());

        maxDuration = Min(maxDuration,
                          m_availableTime - m_txParams.m_protection->protectionTime -
                              m_txParams.m_txDuration - m_apMac->GetWifiPhy(m_linkId)->GetSifs() -
                              m_txParams.m_acknowledgment->acknowledgmentTime);
        if (maxDuration.IsNegative())
        {
            NS_LOG_DEBUG("Remaining TXOP duration is not enough for UL MU exchange");
            return NO_TX;
        }
    }

    // Compute the time taken by each station to transmit a frame of maxBufferSize size
    Time bufferTxTime = Seconds(0);
    for (const auto& userInfo : m_trigger)
    {
        Time duration = WifiPhy::CalculateTxDuration(maxBufferSize,
                                                     txVector,
                                                     m_apMac->GetWifiPhy(m_linkId)->GetPhyBand(),
                                                     userInfo.GetAid12());
        bufferTxTime = Max(bufferTxTime, duration);
    }

    if (bufferTxTime < maxDuration)
    {
        // the maximum buffer size can be transmitted within the allowed time
        maxDuration = bufferTxTime;
    }
    else
    {
        // maxDuration may be a too short time. If it does not allow any station to
        // transmit at least m_ulPsduSize bytes, give up the UL MU transmission for now
        Time minDuration = Seconds(0);
        for (const auto& userInfo : m_trigger)
        {
            Time duration =
                WifiPhy::CalculateTxDuration(m_ulPsduSize,
                                             txVector,
                                             m_apMac->GetWifiPhy(m_linkId)->GetPhyBand(),
                                             userInfo.GetAid12());
            minDuration = (minDuration.IsZero() ? duration : Min(minDuration, duration));
        }

        if (maxDuration < minDuration)
        {
            // maxDuration is a too short time, hence return NO_TX. In this way,
            // no transmission will occur now and the next time we will try again
            // performing an UL OFDMA transmission.
            NS_LOG_DEBUG("Available time " << maxDuration.As(Time::MS) << " is too short");
            return NO_TX;
        }
    }

    // maxDuration is the time to grant to the stations. Finalize the Trigger Frame
    uint16_t ulLength;
    std::tie(ulLength, maxDuration) =
        HePhy::ConvertHeTbPpduDurationToLSigLength(maxDuration,
                                                   txVector,
                                                   m_apMac->GetWifiPhy(m_linkId)->GetPhyBand());
    NS_LOG_DEBUG("TB PPDU duration: " << maxDuration.As(Time::MS));
    m_trigger.SetUlLength(ulLength);
    // set Preferred AC to the AC that gained channel access
    for (auto& userInfo : m_trigger)
    {
        userInfo.SetBasicTriggerDepUserInfo(0, 0, m_edca->GetAccessCategory());
    }

    UpdateCredits(m_staListUl, maxDuration, txVector);

    return UL_MU_TX;
}

void
RrMultiUserScheduler::NotifyStationAssociated(uint16_t aid, Mac48Address address)
{
    NS_LOG_FUNCTION(this << aid << address);

    if (!m_apMac->GetHeSupported(address))
    {
        return;
    }

    auto mldOrLinkAddress = m_apMac->GetMldOrLinkAddressByAid(aid);
    NS_ASSERT_MSG(mldOrLinkAddress, "AID " << aid << " not found");

    for (auto& staList : m_staListDl)
    {
        // if this is not the first STA of a non-AP MLD to be notified, an entry
        // for this non-AP MLD already exists
        const auto staIt = std::find_if(staList.second.cbegin(),
                                        staList.second.cend(),
                                        [aid](auto&& info) { return info.aid == aid; });
        if (staIt == staList.second.cend())
        {
            staList.second.push_back(MasterInfo{aid, *mldOrLinkAddress, 0.0});
        }
    }

    const auto staIt = std::find_if(m_staListUl.cbegin(), m_staListUl.cend(), [aid](auto&& info) {
        return info.aid == aid;
    });
    if (staIt == m_staListUl.cend())
    {
        m_staListUl.push_back(MasterInfo{aid, *mldOrLinkAddress, 0.0});
    }
}

void
RrMultiUserScheduler::NotifyStationDeassociated(uint16_t aid, Mac48Address address)
{
    NS_LOG_FUNCTION(this << aid << address);

    if (!m_apMac->GetHeSupported(address))
    {
        return;
    }

    auto mldOrLinkAddress = m_apMac->GetMldOrLinkAddressByAid(aid);
    NS_ASSERT_MSG(mldOrLinkAddress, "AID " << aid << " not found");

    if (m_apMac->IsAssociated(*mldOrLinkAddress))
    {
        // Another STA of the non-AP MLD is still associated
        return;
    }

    for (auto& staList : m_staListDl)
    {
        staList.second.remove_if([&aid](const MasterInfo& info) { return info.aid == aid; });
    }
    m_staListUl.remove_if([&aid](const MasterInfo& info) { return info.aid == aid; });
}

MultiUserScheduler::TxFormat
RrMultiUserScheduler::TrySendingDlMuPpdu()
{
    NS_LOG_FUNCTION(this);

    AcIndex primaryAc = m_edca->GetAccessCategory();

    if (m_staListDl[primaryAc].empty())
    {
        NS_LOG_DEBUG("No HE stations associated: return SU_TX");
        return TxFormat::SU_TX;
    }

    std::size_t count =
        std::min(static_cast<std::size_t>(m_nStations), m_staListDl[primaryAc].size());
    std::size_t nCentral26TonesRus;
    HeRu::RuType ruType =
        HeRu::GetEqualSizedRusForStations(m_allowedWidth, count, nCentral26TonesRus);
    NS_ASSERT(count >= 1);

    if (!m_useCentral26TonesRus)
    {
        nCentral26TonesRus = 0;
    }

    uint8_t currTid = wifiAcList.at(primaryAc).GetHighTid();

    Ptr<WifiMpdu> mpdu = m_edca->PeekNextMpdu(m_linkId);

    if (mpdu && mpdu->GetHeader().IsQosData())
    {
        currTid = mpdu->GetHeader().GetQosTid();
    }

    // determine the list of TIDs to check
    std::vector<uint8_t> tids;

    if (m_enableTxopSharing)
    {
        for (auto acIt = wifiAcList.find(primaryAc); acIt != wifiAcList.end(); acIt++)
        {
            uint8_t firstTid = (acIt->first == primaryAc ? currTid : acIt->second.GetHighTid());
            tids.push_back(firstTid);
            tids.push_back(acIt->second.GetOtherTid(firstTid));
        }
    }
    else
    {
        tids.push_back(currTid);
    }

    Ptr<HeConfiguration> heConfiguration = m_apMac->GetHeConfiguration();
    NS_ASSERT(heConfiguration);

    m_txParams.Clear();
    m_txParams.m_txVector.SetPreambleType(WIFI_PREAMBLE_HE_MU);
    m_txParams.m_txVector.SetChannelWidth(m_allowedWidth);
    m_txParams.m_txVector.SetGuardInterval(heConfiguration->GetGuardInterval().GetNanoSeconds());
    m_txParams.m_txVector.SetBssColor(heConfiguration->GetBssColor());

    // The TXOP limit can be exceeded by the TXOP holder if it does not transmit more
    // than one Data or Management frame in the TXOP and the frame is not in an A-MPDU
    // consisting of more than one MPDU (Sec. 10.22.2.8 of 802.11-2016).
    // For the moment, we are considering just one MPDU per receiver.
    Time actualAvailableTime = (m_initialFrame ? Time::Min() : m_availableTime);

    // iterate over the associated stations until an enough number of stations is identified
    auto staIt = m_staListDl[primaryAc].begin();
    m_candidates.clear();

    std::vector<uint8_t> ruAllocations;
    auto numRuAllocs = m_txParams.m_txVector.GetChannelWidth() / 20;
    ruAllocations.resize(numRuAllocs);
    NS_ASSERT((m_candidates.size() % numRuAllocs) == 0);

    while (staIt != m_staListDl[primaryAc].end() &&
           m_candidates.size() <
               std::min(static_cast<std::size_t>(m_nStations), count + nCentral26TonesRus))
    {
        NS_LOG_DEBUG("Next candidate STA (MAC=" << staIt->address << ", AID=" << staIt->aid << ")");

        if (m_txParams.m_txVector.GetPreambleType() == WIFI_PREAMBLE_EHT_MU &&
            !m_apMac->GetEhtSupported(staIt->address))
        {
            NS_LOG_DEBUG("Skipping non-EHT STA because this DL MU PPDU is sent to EHT STAs only");
            staIt++;
            continue;
        }

        HeRu::RuType currRuType = (m_candidates.size() < count ? ruType : HeRu::RU_26_TONE);

        // check if the AP has at least one frame to be sent to the current station
        for (uint8_t tid : tids)
        {
            AcIndex ac = QosUtilsMapTidToAc(tid);
            NS_ASSERT(ac >= primaryAc);
            // check that a BA agreement is established with the receiver for the
            // considered TID, since ack sequences for DL MU PPDUs require block ack
            if (m_apMac->GetBaAgreementEstablishedAsOriginator(staIt->address, tid))
            {
                mpdu = m_apMac->GetQosTxop(ac)->PeekNextMpdu(m_linkId, tid, staIt->address);

                // we only check if the first frame of the current TID meets the size
                // and duration constraints. We do not explore the queues further.
                if (mpdu)
                {
                    mpdu = GetHeFem(m_linkId)->CreateAliasIfNeeded(mpdu);
                    // Use a temporary TX vector including only the STA-ID of the
                    // candidate station to check if the MPDU meets the size and time limits.
                    // An RU of the computed size is tentatively assigned to the candidate
                    // station, so that the TX duration can be correctly computed.
                    WifiTxVector suTxVector =
                        GetWifiRemoteStationManager(m_linkId)->GetDataTxVector(mpdu->GetHeader(),
                                                                               m_allowedWidth);

                    WifiTxVector txVectorCopy = m_txParams.m_txVector;

                    // the first candidate STA determines the preamble type for the DL MU PPDU
                    if (m_candidates.empty() &&
                        suTxVector.GetPreambleType() == WIFI_PREAMBLE_EHT_MU)
                    {
                        m_txParams.m_txVector.SetPreambleType(WIFI_PREAMBLE_EHT_MU);
                        m_txParams.m_txVector.SetEhtPpduType(0); // indicates DL OFDMA transmission
                    }

                    m_txParams.m_txVector.SetHeMuUserInfo(staIt->aid,
                                                          {{currRuType, 1, true},
                                                           suTxVector.GetMode().GetMcsValue(),
                                                           suTxVector.GetNss()});

                    if (!GetHeFem(m_linkId)->TryAddMpdu(mpdu, m_txParams, actualAvailableTime))
                    {
                        NS_LOG_DEBUG("Adding the peeked frame violates the time constraints");
                        m_txParams.m_txVector = txVectorCopy;
                    }
                    else
                    {
                        // the frame meets the constraints
                        NS_LOG_DEBUG("Adding candidate STA (MAC=" << staIt->address
                                                                  << ", AID=" << staIt->aid
                                                                  << ") TID=" << +tid);
                        m_candidates.emplace_back(staIt, mpdu);
                        break; // terminate the for loop
                    }
                }
                else
                {
                    NS_LOG_DEBUG("No frames to send to " << staIt->address << " with TID=" << +tid);
                }
            }
        }

        // move to the next station in the list
        staIt++;
    }

    if (m_candidates.empty())
    {
        if (m_forceDlOfdma)
        {
            NS_LOG_DEBUG("The AP does not have suitable frames to transmit: return NO_TX");
            return NO_TX;
        }
        NS_LOG_DEBUG("The AP does not have suitable frames to transmit: return SU_TX");
        return SU_TX;
    }

    return TxFormat::DL_MU_TX;
}

void
RrMultiUserScheduler::FinalizeTxVector(WifiTxVector& txVector)
{
    // Do not log txVector because GetTxVectorForUlMu() left RUs undefined and
    // printing them will crash the simulation
    NS_LOG_FUNCTION(this);
    NS_ASSERT(txVector.GetHeMuUserInfoMap().size() == m_candidates.size());

    // compute how many stations can be granted an RU and the RU size
    std::size_t nRusAssigned = m_candidates.size();
    std::size_t nCentral26TonesRus;
    HeRu::RuType ruType =
        HeRu::GetEqualSizedRusForStations(m_allowedWidth, nRusAssigned, nCentral26TonesRus);

    NS_LOG_DEBUG(nRusAssigned << " stations are being assigned a " << ruType << " RU");

    if (!m_useCentral26TonesRus || m_candidates.size() == nRusAssigned)
    {
        nCentral26TonesRus = 0;
    }
    else
    {
        nCentral26TonesRus = std::min(m_candidates.size() - nRusAssigned, nCentral26TonesRus);
        NS_LOG_DEBUG(nCentral26TonesRus << " stations are being assigned a 26-tones RU");
    }

    // re-allocate RUs based on the actual number of candidate stations
    WifiTxVector::HeMuUserInfoMap heMuUserInfoMap;
    std::swap(heMuUserInfoMap, txVector.GetHeMuUserInfoMap());

    auto candidateIt = m_candidates.begin(); // iterator over the list of candidate receivers
    auto ruSet = HeRu::GetRusOfType(m_allowedWidth, ruType);
    auto ruSetIt = ruSet.begin();
    auto central26TonesRus = HeRu::GetCentral26TonesRus(m_allowedWidth, ruType);
    auto central26TonesRusIt = central26TonesRus.begin();

    for (std::size_t i = 0; i < nRusAssigned + nCentral26TonesRus; i++)
    {
        NS_ASSERT(candidateIt != m_candidates.end());
        auto mapIt = heMuUserInfoMap.find(candidateIt->first->aid);
        NS_ASSERT(mapIt != heMuUserInfoMap.end());

        txVector.SetHeMuUserInfo(mapIt->first,
                                 {(i < nRusAssigned ? *ruSetIt++ : *central26TonesRusIt++),
                                  mapIt->second.mcs,
                                  mapIt->second.nss});
        candidateIt++;
    }

    // remove candidates that will not be served
    m_candidates.erase(candidateIt, m_candidates.end());
}

void
RrMultiUserScheduler::UpdateCredits(std::list<MasterInfo>& staList,
                                    Time txDuration,
                                    const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << txDuration.As(Time::US) << txVector);

    // find how many RUs have been allocated for each RU type
    std::map<HeRu::RuType, std::size_t> ruMap;
    for (const auto& userInfo : txVector.GetHeMuUserInfoMap())
    {
        ruMap.insert({userInfo.second.ru.GetRuType(), 0}).first->second++;
    }

    // The amount of credits received by each station equals the TX duration (in
    // microseconds) divided by the number of stations.
    double creditsPerSta = txDuration.ToDouble(Time::US) / staList.size();
    // Transmitting stations have to pay a number of credits equal to the TX duration
    // (in microseconds) times the allocated bandwidth share.
    double debitsPerMhz =
        txDuration.ToDouble(Time::US) /
        std::accumulate(ruMap.begin(), ruMap.end(), 0, [](uint16_t sum, auto pair) {
            return sum + pair.second * HeRu::GetBandwidth(pair.first);
        });

    // assign credits to all stations
    for (auto& sta : staList)
    {
        sta.credits += creditsPerSta;
        sta.credits = std::min(sta.credits, m_maxCredits.ToDouble(Time::US));
    }

    // subtract debits to the selected stations
    for (auto& candidate : m_candidates)
    {
        auto mapIt = txVector.GetHeMuUserInfoMap().find(candidate.first->aid);
        NS_ASSERT(mapIt != txVector.GetHeMuUserInfoMap().end());

        candidate.first->credits -= debitsPerMhz * HeRu::GetBandwidth(mapIt->second.ru.GetRuType());
    }

    // sort the list in decreasing order of credits
    staList.sort([](const MasterInfo& a, const MasterInfo& b) { return a.credits > b.credits; });
}

MultiUserScheduler::DlMuInfo
RrMultiUserScheduler::ComputeDlMuInfo()
{
    NS_LOG_FUNCTION(this);

    if (m_candidates.empty())
    {
        return DlMuInfo();
    }

    DlMuInfo dlMuInfo;
    std::swap(dlMuInfo.txParams.m_txVector, m_txParams.m_txVector);
    FinalizeTxVector(dlMuInfo.txParams.m_txVector);

    m_txParams.Clear();
    Ptr<WifiMpdu> mpdu;

    // Compute the TX params (again) by using the stored MPDUs and the final TXVECTOR
    Time actualAvailableTime = (m_initialFrame ? Time::Min() : m_availableTime);

    for (const auto& candidate : m_candidates)
    {
        mpdu = candidate.second;
        NS_ASSERT(mpdu);

        bool ret [[maybe_unused]] =
            GetHeFem(m_linkId)->TryAddMpdu(mpdu, dlMuInfo.txParams, actualAvailableTime);
        NS_ASSERT_MSG(ret,
                      "Weird that an MPDU does not meet constraints when "
                      "transmitted over a larger RU");
    }

    // We have to complete the PSDUs to send
    Ptr<WifiMacQueue> queue;

    for (const auto& candidate : m_candidates)
    {
        // Let us try first A-MSDU aggregation if possible
        mpdu = candidate.second;
        NS_ASSERT(mpdu);
        uint8_t tid = mpdu->GetHeader().GetQosTid();
        NS_ASSERT_MSG(mpdu->GetOriginal()->GetHeader().GetAddr1() == candidate.first->address,
                      "RA of the stored MPDU must match the stored address");

        NS_ASSERT(mpdu->IsQueued());
        Ptr<WifiMpdu> item = mpdu;

        if (!mpdu->GetHeader().IsRetry())
        {
            // this MPDU must have been dequeued from the AC queue and we can try
            // A-MSDU aggregation
            item = GetHeFem(m_linkId)->GetMsduAggregator()->GetNextAmsdu(mpdu,
                                                                         dlMuInfo.txParams,
                                                                         m_availableTime);

            if (!item)
            {
                // A-MSDU aggregation failed or disabled
                item = mpdu;
            }
            m_apMac->GetQosTxop(QosUtilsMapTidToAc(tid))->AssignSequenceNumber(item);
        }

        // Now, let's try A-MPDU aggregation if possible
        std::vector<Ptr<WifiMpdu>> mpduList =
            GetHeFem(m_linkId)->GetMpduAggregator()->GetNextAmpdu(item,
                                                                  dlMuInfo.txParams,
                                                                  m_availableTime);

        if (mpduList.size() > 1)
        {
            // A-MPDU aggregation succeeded, update psduMap
            dlMuInfo.psduMap[candidate.first->aid] = Create<WifiPsdu>(std::move(mpduList));
        }
        else
        {
            dlMuInfo.psduMap[candidate.first->aid] = Create<WifiPsdu>(item, true);
        }
    }

    AcIndex primaryAc = m_edca->GetAccessCategory();
    UpdateCredits(m_staListDl[primaryAc],
                  dlMuInfo.txParams.m_txDuration,
                  dlMuInfo.txParams.m_txVector);

    NS_LOG_DEBUG("Next station to serve has AID=" << m_staListDl[primaryAc].front().aid);

    return dlMuInfo;
}

MultiUserScheduler::UlMuInfo
RrMultiUserScheduler::ComputeUlMuInfo()
{
    return UlMuInfo{m_trigger, m_triggerMacHdr, std::move(m_txParams)};
}

} // namespace ns3
