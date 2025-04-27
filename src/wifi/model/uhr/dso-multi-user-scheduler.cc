/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "dso-multi-user-scheduler.h"

#include "uhr-frame-exchange-manager.h"

#include "ns3/he-configuration.h"
#include "ns3/he-phy.h"
#include "ns3/log.h"
#include "ns3/wifi-acknowledgment.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-protection.h"
#include "ns3/wifi-psdu.h"

#include <algorithm>
#include <list>
#include <numeric>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DsoMultiUserScheduler");

NS_OBJECT_ENSURE_REGISTERED(DsoMultiUserScheduler);

TypeId
DsoMultiUserScheduler::GetTypeId()
{
    static TypeId tid = TypeId("ns3::DsoMultiUserScheduler")
                            .SetParent<RrMultiUserScheduler>()
                            .SetGroupName("Wifi")
                            .AddConstructor<DsoMultiUserScheduler>();
    return tid;
}

DsoMultiUserScheduler::DsoMultiUserScheduler()
{
    NS_LOG_FUNCTION(this);
}

DsoMultiUserScheduler::~DsoMultiUserScheduler()
{
    NS_LOG_FUNCTION_NOARGS();
}

MultiUserScheduler::TxFormat
DsoMultiUserScheduler::SelectTxFormat()
{
    NS_LOG_FUNCTION(this);

    if (const auto format = DoSelectTxFormat())
    {
        return *format;
    }

    if (m_initialFrame && m_dsoTxop)
    {
        m_dsoTxop = false;
        m_dsoRuAllocation.clear();
    }

    auto mpdu = m_edca->PeekNextMpdu(m_linkId);
    if (mpdu && !m_apMac->GetUhrSupported(mpdu->GetHeader().GetAddr1()))
    {
        return SU_TX;
    }

    if (m_initialFrame && m_allowedWidth >= MHz_t{80})
    {
        // try sending a BSRP Trigger Frame as a DSO ICF
        const auto direction = (!m_enableUlOfdma || GetLastTxFormat(m_linkId) != DL_MU_TX)
                                   ? WifiDirection::DOWNLINK
                                   : WifiDirection::UPLINK;
        if (const auto txFormat = TrySendingDsoIcf(direction); txFormat != DL_MU_TX)
        {
            return txFormat;
        }
        if (m_enableUlOfdma)
        {
            if (const auto txFormat = TrySendingDsoIcf(WifiDirection::UPLINK); txFormat != DL_MU_TX)
            {
                return txFormat;
            }
        }
    }

    // try sending a regular BSRP Trigger Frame
    if (!m_dsoTxop && m_enableUlOfdma && m_enableBsrp &&
        (GetLastTxFormat(m_linkId) == DL_MU_TX ||
         ((m_initialFrame || m_trigger.GetType() != TriggerFrameType::BSRP_TRIGGER) && !mpdu)))
    {
        TxFormat txFormat = TrySendingBsrpTf();

        if (txFormat != DL_MU_TX)
        {
            return txFormat;
        }
    }

    // try sending a Basic Trigger Frame
    if (m_enableUlOfdma &&
        ((GetLastTxFormat(m_linkId) == DL_MU_TX) ||
         (!m_dsoTxop && (m_trigger.GetType() == TriggerFrameType::BSRP_TRIGGER)) || !mpdu))
    {
        if (const auto txFormat = TrySendingBasicTf(); txFormat != DL_MU_TX)
        {
            return txFormat;
        }
    }

    // otherwise, try sending a DL MU PPDU
    return TrySendingDlMuPpdu();
}

std::optional<MultiUserScheduler::TxFormat>
DsoMultiUserScheduler::DoSelectTxFormat()
{
    return std::nullopt;
}

WifiTxVector
DsoMultiUserScheduler::GetTxVectorForUlMu(std::function<bool(const MasterInfo&)> canBeSolicited)
{
    NS_LOG_FUNCTION(this);

    const auto heConfiguration = m_apMac->GetHeConfiguration();
    NS_ASSERT(heConfiguration);

    WifiTxVector txVector;
    txVector.SetGuardInterval(heConfiguration->GetGuardInterval());
    txVector.SetBssColor(heConfiguration->m_bssColor);
    txVector.SetPreambleType(WIFI_PREAMBLE_UHR_TB);
    txVector.SetEhtPpduType(0);

    auto firstCandidate = std::find_if(m_staListUl.begin(), m_staListUl.end(), canBeSolicited);
    if (firstCandidate == m_staListUl.cend())
    {
        NS_LOG_DEBUG("No candidate");
        return txVector;
    }

    // determine RUs to allocate to stations;
    auto count = std::min<std::size_t>(m_nStations, m_staListUl.size());
    std::size_t nCentral26TonesRus{0};
    auto ruType = WifiRu::GetEqualSizedRusForStations(m_allowedWidth,
                                                      count,
                                                      nCentral26TonesRus,
                                                      WIFI_MOD_CLASS_UHR);
    const auto staSupportedWidth =
        GetWifiRemoteStationManager(m_linkId)->GetChannelWidthSupported(firstCandidate->address);
    while (staSupportedWidth < WifiRu::GetBandwidth(ruType))
    {
        m_allowedWidth /= 2;
        ruType = WifiRu::GetEqualSizedRusForStations(m_allowedWidth,
                                                     count,
                                                     nCentral26TonesRus,
                                                     WIFI_MOD_CLASS_UHR);
    }
    txVector.SetChannelWidth(m_allowedWidth);

    // iterate over the associated stations until an enough number of stations is identified
    auto staIt = firstCandidate;
    m_candidates.clear();

    while (staIt != m_staListUl.end() &&
           txVector.GetHeMuUserInfoMap().size() < std::min<std::size_t>(m_nStations, count))
    {
        NS_LOG_DEBUG("Next candidate STA (MAC=" << staIt->address << ", AID=" << staIt->aid << ")");

        // prepare the MAC header of a frame that would be sent to the candidate station,
        // just for the purpose of retrieving the TXVECTOR used to transmit to that station
        WifiMacHeader hdr(WIFI_MAC_QOSDATA);
        hdr.SetAddr1(GetWifiRemoteStationManager(m_linkId)
                         ->GetAffiliatedStaAddress(staIt->address)
                         .value_or(staIt->address));
        hdr.SetAddr2(m_apMac->GetFrameExchangeManager(m_linkId)->GetAddress());
        const auto suTxVector =
            GetWifiRemoteStationManager(m_linkId)->GetDataTxVector(hdr, m_allowedWidth);
        txVector.SetHeMuUserInfo(staIt->aid,
                                 {HeRu::RuSpec(), // assigned later by FinalizeTxVector
                                  suTxVector.GetMode().GetMcsValue(),
                                  suTxVector.GetNss()});
        m_candidates.emplace_back(staIt, nullptr);

        // move to the next candidate station in the list
        staIt = std::find_if(++staIt, m_staListUl.end(), canBeSolicited);
    }

    if (txVector.GetHeMuUserInfoMap().empty())
    {
        NS_LOG_DEBUG("No suitable station");
        return txVector;
    }

    FinalizeTxVector(txVector);
    return txVector;
}

WifiTxVector
DsoMultiUserScheduler::GetTxVectorForDlMu(std::function<bool(const MasterInfo&)> canBeSolicited)
{
    NS_LOG_FUNCTION(this);

    const auto primaryAc = m_edca->GetAccessCategory();
    auto currTid = wifiAcList.at(primaryAc).GetHighTid();
    auto mpdu = m_edca->PeekNextMpdu(m_linkId);
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
            const auto firstTid = (acIt->first == primaryAc ? currTid : acIt->second.GetHighTid());
            tids.emplace_back(firstTid);
            tids.emplace_back(acIt->second.GetOtherTid(firstTid));
        }
    }
    else
    {
        tids.emplace_back(currTid);
    }

    const auto heConfiguration = m_apMac->GetHeConfiguration();
    NS_ASSERT(heConfiguration);

    WifiTxVector txVector;
    txVector.SetPreambleType(WIFI_PREAMBLE_UHR_MU);
    txVector.SetChannelWidth(m_allowedWidth);
    txVector.SetGuardInterval(heConfiguration->GetGuardInterval());
    txVector.SetBssColor(heConfiguration->m_bssColor);
    txVector.SetEhtPpduType(0);

    auto firstCandidate =
        std::find_if(m_staListDl[primaryAc].begin(), m_staListDl[primaryAc].end(), canBeSolicited);
    if (firstCandidate == m_staListDl[primaryAc].cend())
    {
        NS_LOG_DEBUG("No candidate");
        return txVector;
    }

    // determine RUs to allocate to stations;
    auto count = std::min<std::size_t>(m_nStations, m_staListUl.size());
    std::size_t nCentral26TonesRus{0};
    auto ruType = WifiRu::GetEqualSizedRusForStations(m_allowedWidth,
                                                      count,
                                                      nCentral26TonesRus,
                                                      WIFI_MOD_CLASS_UHR);
    const auto staSupportedWidth =
        GetWifiRemoteStationManager(m_linkId)->GetChannelWidthSupported(firstCandidate->address);
    while (staSupportedWidth < WifiRu::GetBandwidth(ruType))
    {
        m_allowedWidth /= 2;
        ruType = WifiRu::GetEqualSizedRusForStations(m_allowedWidth,
                                                     count,
                                                     nCentral26TonesRus,
                                                     WIFI_MOD_CLASS_UHR);
    }
    txVector.SetChannelWidth(m_allowedWidth);

    // iterate over the associated stations until an enough number of stations is identified
    auto staIt = firstCandidate;
    m_candidates.clear();

    // The TXOP limit can be exceeded by the TXOP holder if it does not transmit more
    // than one Data or Management frame in the TXOP and the frame is not in an A-MPDU
    // consisting of more than one MPDU (Sec. 10.22.2.8 of 802.11-2016).
    // For the moment, we are considering just one MPDU per receiver.
    const auto actualAvailableTime = (m_initialFrame ? Time::Min() : m_availableTime);

    WifiTxParameters txParams;
    while (staIt != m_staListDl[primaryAc].end() &&
           m_candidates.size() < std::min(static_cast<std::size_t>(m_nStations), count))
    {
        NS_LOG_DEBUG("Next candidate STA (MAC=" << staIt->address << ", AID=" << staIt->aid << ")");

        // check if the AP has at least one frame to be sent to the current station
        for (uint8_t tid : tids)
        {
            const auto ac = QosUtilsMapTidToAc(tid);
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
                    const auto suTxVector =
                        GetWifiRemoteStationManager(m_linkId)->GetDataTxVector(mpdu->GetHeader(),
                                                                               m_allowedWidth);

                    auto txVectorCopy = txVector;
                    const auto currRuType =
                        (m_candidates.size() < count ? ruType : RuType::RU_26_TONE);
                    const auto ru = WifiRu::RuSpec(EhtRu::RuSpec{currRuType, 1, true, true});
                    txVector.SetHeMuUserInfo(
                        staIt->aid,
                        {ru, suTxVector.GetMode().GetMcsValue(), suTxVector.GetNss()});
                    txParams.m_txVector = txVector;

                    if (!GetHeFem(m_linkId)->TryAddMpdu(mpdu, txParams, actualAvailableTime))
                    {
                        NS_LOG_DEBUG("Adding the peeked frame violates the time constraints");
                        txVector = txVectorCopy;
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
            else
            {
                NS_LOG_DEBUG("No BA agreement established with " << staIt->address
                                                                 << " for TID=" << +tid);
            }
        }

        if (!mpdu && m_dsoRuAllocation.contains(staIt->aid))
        {
            NS_LOG_DEBUG("use Qos Null frame for " << staIt->address);
            WifiMacHeader header(WIFI_MAC_QOSDATA_NULL);
            header.SetAddr1(staIt->address);
            header.SetAddr2(m_apMac->GetFrameExchangeManager(m_linkId)->GetAddress());
            header.SetAddr3(m_apMac->GetFrameExchangeManager(m_linkId)->GetAddress());
            header.SetDsTo();
            header.SetDsNotFrom();
            mpdu = Create<WifiMpdu>(Create<Packet>(), header);
            m_candidates.emplace_back(staIt, mpdu);
        }

        // move to the next station in the list
        staIt = std::find_if(++staIt, m_staListDl[primaryAc].end(), canBeSolicited);
    }

    // if only QoS Null frames have been added to fill in allocated RUs, we should not transmit a DL
    // MU PPDU
    if (std::all_of(m_candidates.cbegin(), m_candidates.cend(), [](const auto& candidate) {
            return !candidate.second->GetHeader().HasData();
        }))
    {
        m_candidates.clear();
    }

    if (txVector.GetHeMuUserInfoMap().empty())
    {
        NS_LOG_DEBUG("No suitable station");
        return txVector;
    }

    FinalizeTxVector(txVector);
    return txVector;
}

bool
DsoMultiUserScheduler::CanSolicitStaInTxop(const MasterInfo& info) const
{
    if (m_dsoTxop && !m_dsoRuAllocation.empty() && !m_dsoRuAllocation.contains(info.aid))
    {
        NS_LOG_INFO("STA with AID " << info.aid
                                    << " has no RU allocated for this DSO TXOP: skipping station");
        return false;
    }
    return true;
}

bool
DsoMultiUserScheduler::CanSolicitStaInBsrpTf(const MasterInfo& info, WifiDirection direction) const
{
    if (!CanSolicitStaInTxop(info))
    {
        return false;
    }
    return RrMultiUserScheduler::CanSolicitStaInBsrpTf(info, direction);
}

MultiUserScheduler::TxFormat
DsoMultiUserScheduler::TrySendingDsoIcf(WifiDirection direction)
{
    NS_LOG_FUNCTION(this << direction);
    auto staList = (direction == WifiDirection::DOWNLINK) ? m_staListDl[m_edca->GetAccessCategory()]
                                                          : m_staListUl;

    if (staList.empty())
    {
        NS_LOG_DEBUG("No UHR stations associated: return SU_TX");
        return TxFormat::SU_TX;
    }

    auto fctCb = std::bind(&DsoMultiUserScheduler::CanSolicitStaInBsrpTf,
                           this,
                           std::placeholders::_1,
                           direction);
    const auto firstCandidate = std::find_if(staList.begin(), staList.end(), fctCb);
    if (firstCandidate == staList.end())
    {
        NS_LOG_DEBUG("No suitable station found: return DL_MU_TX");
        return TxFormat::DL_MU_TX;
    }

    m_dsoTxop = true;

    auto txVector = (direction == WifiDirection::DOWNLINK) ? GetTxVectorForDlMu(fctCb)
                                                           : GetTxVectorForUlMu(fctCb);
    txVector.SetPreambleType(WIFI_PREAMBLE_UHR_TB);

    if (txVector.GetHeMuUserInfoMap().empty())
    {
        NS_LOG_DEBUG("No suitable station found");
        m_dsoTxop = false;
        return TxFormat::DL_MU_TX;
    }

    m_trigger = CtrlTriggerHeader(TriggerFrameType::BSRP_TRIGGER, txVector);
    txVector.SetGuardInterval(m_trigger.GetGuardInterval());

    auto item = GetTriggerFrame(m_trigger, m_linkId);
    m_triggerMacHdr = item->GetHeader();

    m_txParams.Clear();
    // set the TXVECTOR used to send the Trigger Frame
    m_txParams.m_txVector =
        m_apMac->GetWifiRemoteStationManager(m_linkId)->GetRtsTxVector(m_triggerMacHdr.GetAddr1(),
                                                                       m_allowedWidth);

    m_apMac->GetWifiRemoteStationManager(m_linkId)->AdjustTxVectorForIcf(m_txParams.m_txVector);

    if (!GetHeFem(m_linkId)->TryAddMpdu(item, m_txParams, m_availableTime))
    {
        // sending the BSRP Trigger Frame is not possible, hence return NO_TX. In
        // this way, no transmission will occur now and the next time we will
        // try again sending a BSRP Trigger Frame.
        NS_LOG_DEBUG("Remaining TXOP duration is not enough for BSRP TF exchange");
        m_dsoTxop = false;
        return NO_TX;
    }

    // Compute the time taken by each station to transmit 8 QoS Null frames
    Time qosNullTxDuration;
    for (const auto& userInfo : m_trigger)
    {
        Time duration = WifiPhy::CalculateTxDuration(GetMaxSizeOfQosNullAmpdu(m_trigger),
                                                     txVector,
                                                     m_apMac->GetWifiPhy(m_linkId)->GetPhyBand(),
                                                     userInfo.GetAid12());
        qosNullTxDuration = Max(qosNullTxDuration, duration);
    }

    NS_ASSERT(m_txParams.m_txDuration.has_value());
    m_triggerTxDuration = m_txParams.m_txDuration.value();

    if (m_availableTime != Time::Min())
    {
        // TryAddMpdu only considers the time to transmit the Trigger Frame
        NS_ASSERT(m_txParams.m_protection && m_txParams.m_protection->protectionTime.has_value());
        NS_ASSERT(m_txParams.m_acknowledgment &&
                  m_txParams.m_acknowledgment->acknowledgmentTime.has_value() &&
                  m_txParams.m_acknowledgment->acknowledgmentTime->IsZero());

        if (*m_txParams.m_protection->protectionTime + *m_txParams.m_txDuration // BSRP TF tx time
                + m_apMac->GetWifiPhy(m_linkId)->GetSifs() + qosNullTxDuration >
            m_availableTime)
        {
            NS_LOG_DEBUG("Remaining TXOP duration is not enough for BSRP TF exchange");
            m_dsoTxop = false;
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

void
DsoMultiUserScheduler::NotifyStationAssociated(uint16_t aid, Mac48Address address)
{
    NS_LOG_FUNCTION(this << aid << address);
    if (!m_apMac->GetUhrSupported(address))
    {
        NS_LOG_INFO("STA " << address << " does not support UHR: skipping station");
        return;
    }
    RrMultiUserScheduler::NotifyStationAssociated(aid, address);
}

void
DsoMultiUserScheduler::NotifyStationDeassociated(uint16_t aid, Mac48Address address)
{
    NS_LOG_FUNCTION(this << aid << address);
    if (!m_apMac->GetUhrSupported(address))
    {
        return;
    }
    RrMultiUserScheduler::NotifyStationDeassociated(aid, address);
}

MultiUserScheduler::TxFormat
DsoMultiUserScheduler::TrySendingDlMuPpdu()
{
    NS_LOG_FUNCTION(this);

    if (const auto primaryAc = m_edca->GetAccessCategory(); m_staListDl[primaryAc].empty())
    {
        NS_LOG_DEBUG("No UHR stations associated: return SU_TX");
        return TxFormat::SU_TX;
    }

    const auto txVector =
        GetTxVectorForDlMu(std::bind_front(&DsoMultiUserScheduler::CanSolicitStaInTxop, this));

    if (txVector.GetHeMuUserInfoMap().empty())
    {
        NS_LOG_DEBUG("The AP does not have suitable frames to transmit");
        return m_dsoTxop ? TxFormat::NO_TX : TxFormat::SU_TX;
    }

    m_txParams.m_txVector = txVector;
    return TxFormat::DL_MU_TX;
}

void
DsoMultiUserScheduler::FinalizeTxVector(WifiTxVector& txVector)
{
    // Do not log txVector because GetTxVectorForUlMu or GetTxVectorForUlMu() left RUs undefined and
    // printing them will crash the simulation
    NS_LOG_FUNCTION(this);

    auto candidateIt = m_candidates.begin(); // iterator over the list of candidate receivers

    if (m_dsoTxop && !m_dsoRuAllocation.empty())
    {
        NS_LOG_DEBUG("RUs already allocated for ongoing DSO TXOP");

        // use the RU allocation from the DSO TXOP
        txVector.GetHeMuUserInfoMap().clear();
        for (const auto& [aid, ruSpec] : m_dsoRuAllocation)
        {
            txVector.SetHeMuUserInfo(aid, ruSpec);
        }

        // remove candidates that are not in the DSO RU allocation
        for (; candidateIt != m_candidates.end();)
        {
            if (!m_dsoRuAllocation.contains(candidateIt->first->aid))
            {
                candidateIt = m_candidates.erase(candidateIt);
            }
            else
            {
                ++candidateIt;
            }
        }
        return;
    }

    auto nRusAssigned = m_candidates.size();
    // FIXME: currently, all STAs are assumed to support the same width
    const auto staSupportedWidth = GetWifiRemoteStationManager(m_linkId)->GetChannelWidthSupported(
        candidateIt->first->address);
    m_allowedWidth =
        std::min<MHz_t>(m_allowedWidth,
                        staSupportedWidth * std::max<std::size_t>(1, (nRusAssigned / 2) * 2));
    txVector.SetChannelWidth(m_allowedWidth);

    // compute how many stations can be granted an RU and the RU size
    std::size_t nCentral26TonesRus;
    const auto ruType = WifiRu::GetEqualSizedRusForStations(m_allowedWidth,
                                                            nRusAssigned,
                                                            nCentral26TonesRus,
                                                            txVector.GetModulationClass());

    NS_LOG_DEBUG(nRusAssigned << " station" << ((nRusAssigned > 1) ? "s are" : " is")
                              << " being assigned a " << ruType << " RU");

    // re-allocate RUs based on the actual number of candidate stations
    WifiTxVector::HeMuUserInfoMap heMuUserInfoMap;
    std::swap(heMuUserInfoMap, txVector.GetHeMuUserInfoMap());

    auto ruSet = WifiRu::GetRusOfType(m_allowedWidth, ruType, WIFI_MOD_CLASS_UHR);
    auto ruSetIt = ruSet.begin();
    auto central26TonesRus =
        WifiRu::GetCentral26TonesRus(m_allowedWidth, ruType, WIFI_MOD_CLASS_UHR);
    auto central26TonesRusIt = central26TonesRus.begin();

    for (std::size_t i = 0; i < nRusAssigned; i++)
    {
        NS_ASSERT(candidateIt != m_candidates.end());
        auto mapIt = heMuUserInfoMap.find(candidateIt->first->aid);
        NS_ASSERT(mapIt != heMuUserInfoMap.end());
        const HeMuUserInfo userInfo{(i < nRusAssigned ? *ruSetIt++ : *central26TonesRusIt++),
                                    mapIt->second.mcs,
                                    mapIt->second.nss};
        if (m_dsoTxop)
        {
            m_dsoRuAllocation[mapIt->first] = userInfo;
        }

        txVector.SetHeMuUserInfo(mapIt->first, userInfo);
        candidateIt++;
    }

    // remove candidates that will not be served
    m_candidates.erase(candidateIt, m_candidates.end());
}

} // namespace ns3
