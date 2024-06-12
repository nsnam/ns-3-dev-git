/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "multi-user-scheduler.h"

#include "he-configuration.h"
#include "he-frame-exchange-manager.h"

#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/qos-txop.h"
#include "ns3/wifi-acknowledgment.h"
#include "ns3/wifi-mac-trailer.h"
#include "ns3/wifi-protection.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MultiUserScheduler");

NS_OBJECT_ENSURE_REGISTERED(MultiUserScheduler);

TypeId
MultiUserScheduler::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::MultiUserScheduler")
            .SetParent<Object>()
            .SetGroupName("Wifi")
            .AddAttribute("AccessReqInterval",
                          "Duration of the interval between two consecutive requests for "
                          "channel access made by the MultiUserScheduler. Such requests are "
                          "made independently of the presence of frames in the queues of the "
                          "AP and are intended to allow the AP to coordinate UL MU transmissions "
                          "even without DL traffic. A null duration indicates that such "
                          "requests shall not be made.",
                          TimeValue(Seconds(0)),
                          MakeTimeAccessor(&MultiUserScheduler::SetAccessReqInterval,
                                           &MultiUserScheduler::GetAccessReqInterval),
                          MakeTimeChecker())
            .AddAttribute("AccessReqAc",
                          "The Access Category for which the MultiUserScheduler makes requests "
                          "for channel access.",
                          EnumValue(AcIndex::AC_BE),
                          MakeEnumAccessor<AcIndex>(&MultiUserScheduler::m_accessReqAc),
                          MakeEnumChecker(AcIndex::AC_BE,
                                          "AC_BE",
                                          AcIndex::AC_VI,
                                          "AC_VI",
                                          AcIndex::AC_VO,
                                          "AC_VO",
                                          AcIndex::AC_BK,
                                          "AC_BK"))
            .AddAttribute("DelayAccessReqUponAccess",
                          "If enabled, the access request interval is measured starting "
                          "from the last time an EDCA function obtained channel access. "
                          "Otherwise, the access request interval is measured starting "
                          "from the last time the MultiUserScheduler made a request for "
                          "channel access.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&MultiUserScheduler::m_restartTimerUponAccess),
                          MakeBooleanChecker())
            .AddAttribute("DefaultTbPpduDuration",
                          "Default duration of TB PPDUs solicited via a Basic Trigger Frame. "
                          "This value is used to compute the Duration/ID field of BSRP Trigger "
                          "Frames sent when the TXOP Limit is zero and the FrameExchangeManager "
                          "continues the TXOP a SIFS after receiving response to the BSRP TF. "
                          "This value shall also be used by subclasses when they have no other "
                          "information available to determine the TX duration of solicited PPDUs. "
                          "The default value roughly corresponds to half the maximum PPDU TX "
                          "duration.",
                          TimeValue(MilliSeconds(2)),
                          MakeTimeAccessor(&MultiUserScheduler::m_defaultTbPpduDuration),
                          MakeTimeChecker());
    return tid;
}

MultiUserScheduler::MultiUserScheduler()
    : m_isUnprotectedEmlsrClient([this](uint8_t linkId, Mac48Address address) {
          return m_apMac->GetWifiRemoteStationManager(linkId)->GetEmlsrEnabled(address) &&
                 !m_apMac->GetFrameExchangeManager(linkId)->GetProtectedStas().contains(address);
      })
{
}

MultiUserScheduler::~MultiUserScheduler()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
MultiUserScheduler::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_apMac = nullptr;
    m_edca = nullptr;
    m_lastTxInfo.clear();
    for (auto& accessReqTimer : m_accessReqTimers)
    {
        accessReqTimer.Cancel();
    }
    Object::DoDispose();
}

void
MultiUserScheduler::NotifyNewAggregate()
{
    NS_LOG_FUNCTION(this);
    if (!m_apMac)
    {
        Ptr<ApWifiMac> apMac = this->GetObject<ApWifiMac>();
        // verify that it's a valid AP mac and that
        // the AP mac was not set before
        if (apMac)
        {
            this->SetWifiMac(apMac);
        }
    }
    Object::NotifyNewAggregate();
}

void
MultiUserScheduler::DoInitialize()
{
    NS_LOG_FUNCTION(this);

    if (m_accessReqInterval.IsStrictlyPositive())
    {
        NS_ASSERT(m_accessReqTimers.empty());
        for (uint8_t id = 0; id < m_apMac->GetNLinks(); ++id)
        {
            m_accessReqTimers.emplace_back(
                Simulator::Schedule(m_accessReqInterval,
                                    &MultiUserScheduler::AccessReqTimeout,
                                    this,
                                    id));
        }
    }
}

void
MultiUserScheduler::SetAccessReqInterval(Time interval)
{
    NS_LOG_FUNCTION(this << interval.As(Time::MS));
    m_accessReqInterval = interval;
    // if interval is non-zero, start the timers that are not running if we are past initialization
    if (m_accessReqInterval.IsStrictlyPositive() && IsInitialized())
    {
        m_accessReqTimers.resize(m_apMac->GetNLinks());
        for (uint8_t id = 0; id < m_apMac->GetNLinks(); ++id)
        {
            if (!m_accessReqTimers[id].IsPending())
            {
                m_accessReqTimers[id] = Simulator::Schedule(m_accessReqInterval,
                                                            &MultiUserScheduler::AccessReqTimeout,
                                                            this,
                                                            id);
            }
        }
    }
}

Time
MultiUserScheduler::GetAccessReqInterval() const
{
    return m_accessReqInterval;
}

void
MultiUserScheduler::SetWifiMac(Ptr<ApWifiMac> mac)
{
    NS_LOG_FUNCTION(this << mac);
    m_apMac = mac;

    // When VHT DL MU-MIMO will be supported, we will have to lower this requirement
    // and allow a Multi-user scheduler to be installed on a VHT AP.
    NS_ABORT_MSG_IF(!m_apMac || !m_apMac->GetHeConfiguration(),
                    "MultiUserScheduler can only be installed on HE APs");

    for (uint8_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
    {
        auto heFem = DynamicCast<HeFrameExchangeManager>(m_apMac->GetFrameExchangeManager(linkId));
        NS_ASSERT(heFem);
        heFem->SetMultiUserScheduler(this);
    }
}

Ptr<WifiRemoteStationManager>
MultiUserScheduler::GetWifiRemoteStationManager(uint8_t linkId) const
{
    return m_apMac->GetWifiRemoteStationManager(linkId);
}

Ptr<HeFrameExchangeManager>
MultiUserScheduler::GetHeFem(uint8_t linkId) const
{
    return StaticCast<HeFrameExchangeManager>(m_apMac->GetFrameExchangeManager(linkId));
}

Time
MultiUserScheduler::GetExtraTimeForBsrpTfDurationId(uint8_t linkId) const
{
    return m_defaultTbPpduDuration;
}

void
MultiUserScheduler::AccessReqTimeout(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    // request channel access if not requested yet
    auto edca = m_apMac->GetQosTxop(m_accessReqAc);

    if (edca->GetAccessStatus(linkId) == Txop::NOT_REQUESTED)
    {
        m_apMac->GetChannelAccessManager(linkId)->RequestAccess(edca);
    }

    // restart timer
    if (m_accessReqInterval.IsStrictlyPositive())
    {
        NS_ASSERT(m_accessReqTimers.size() > linkId);
        m_accessReqTimers[linkId] = Simulator::Schedule(m_accessReqInterval,
                                                        &MultiUserScheduler::AccessReqTimeout,
                                                        this,
                                                        linkId);
    }
}

MultiUserScheduler::TxFormat
MultiUserScheduler::NotifyAccessGranted(Ptr<QosTxop> edca,
                                        Time availableTime,
                                        bool initialFrame,
                                        MHz_u allowedWidth,
                                        uint8_t linkId)
{
    NS_LOG_FUNCTION(this << edca << availableTime << initialFrame << allowedWidth << linkId);

    m_edca = edca;
    m_availableTime = availableTime;
    m_initialFrame = initialFrame;
    m_allowedWidth = allowedWidth;
    m_linkId = linkId;

    if (m_accessReqTimers.size() > linkId && m_accessReqTimers[linkId].IsPending() &&
        m_restartTimerUponAccess)
    {
        // restart access timer
        m_accessReqTimers[linkId].Cancel();
        if (m_accessReqInterval.IsStrictlyPositive())
        {
            m_accessReqTimers[linkId] = Simulator::Schedule(m_accessReqInterval,
                                                            &MultiUserScheduler::AccessReqTimeout,
                                                            this,
                                                            linkId);
        }
    }

    TxFormat txFormat = SelectTxFormat();

    if (txFormat == DL_MU_TX)
    {
        m_lastTxInfo[linkId].dlInfo = ComputeDlMuInfo();
    }
    else if (txFormat == UL_MU_TX)
    {
        m_lastTxInfo[linkId].ulInfo = ComputeUlMuInfo();
        CheckTriggerFrame();
    }

    if (txFormat != NO_TX)
    {
        m_lastTxInfo[linkId].lastTxFormat = txFormat;
    }
    return txFormat;
}

MultiUserScheduler::TxFormat
MultiUserScheduler::GetLastTxFormat(uint8_t linkId) const
{
    const auto it = m_lastTxInfo.find(linkId);
    return it != m_lastTxInfo.cend() ? it->second.lastTxFormat : NO_TX;
}

MultiUserScheduler::DlMuInfo&
MultiUserScheduler::GetDlMuInfo(uint8_t linkId)
{
    NS_ABORT_MSG_IF(m_lastTxInfo[linkId].lastTxFormat != DL_MU_TX,
                    "Next transmission is not DL MU");

#ifdef NS3_BUILD_PROFILE_DEBUG
    // check that all the addressed stations support HE
    for (auto& psdu : m_lastTxInfo[linkId].dlInfo.psduMap)
    {
        auto receiver = psdu.second->GetAddr1();
        auto linkId = m_apMac->IsAssociated(receiver);
        NS_ABORT_MSG_IF(!linkId, "Station " << receiver << " should be associated");
        NS_ABORT_MSG_IF(!GetWifiRemoteStationManager(*linkId)->GetHeSupported(receiver),
                        "Station " << psdu.second->GetAddr1() << " does not support HE");
    }
#endif

    return m_lastTxInfo[linkId].dlInfo;
}

MultiUserScheduler::UlMuInfo&
MultiUserScheduler::GetUlMuInfo(uint8_t linkId)
{
    NS_ABORT_MSG_IF(m_lastTxInfo[linkId].lastTxFormat != UL_MU_TX,
                    "Next transmission is not UL MU");

    return m_lastTxInfo[linkId].ulInfo;
}

Ptr<WifiMpdu>
MultiUserScheduler::GetTriggerFrame(const CtrlTriggerHeader& trigger, uint8_t linkId) const
{
    NS_LOG_FUNCTION(this << linkId);

    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(trigger);

    Mac48Address receiver = Mac48Address::GetBroadcast();
    if (trigger.GetNUserInfoFields() == 1)
    {
        auto aid = trigger.begin()->GetAid12();
        auto aidAddrMapIt = m_apMac->GetStaList(linkId).find(aid);
        NS_ASSERT(aidAddrMapIt != m_apMac->GetStaList(linkId).end());
        receiver = aidAddrMapIt->second;
    }

    WifiMacHeader hdr(WIFI_MAC_CTL_TRIGGER);
    hdr.SetAddr1(receiver);
    hdr.SetAddr2(GetHeFem(linkId)->GetAddress());
    hdr.SetDsNotTo();
    hdr.SetDsNotFrom();

    return Create<WifiMpdu>(packet, hdr);
}

void
MultiUserScheduler::CheckTriggerFrame()
{
    NS_LOG_FUNCTION(this);

    // Set the CS Required subfield to true, unless the UL Length subfield is less
    // than or equal to 76 (see Section 26.5.2.5 of 802.11ax-2021)
    m_lastTxInfo[m_linkId].ulInfo.trigger.SetCsRequired(
        m_lastTxInfo[m_linkId].ulInfo.trigger.GetUlLength() > 76);

    GetHeFem(m_linkId)->SetTargetRssi(m_lastTxInfo[m_linkId].ulInfo.trigger);
}

void
MultiUserScheduler::RemoveRecipientsFromTf(
    uint8_t linkId,
    CtrlTriggerHeader& trigger,
    WifiTxParameters& txParams,
    std::function<bool(uint8_t, Mac48Address)> predicate) const
{
    NS_LOG_FUNCTION(this << linkId << &txParams);

    const auto& aidAddrMap = m_apMac->GetStaList(linkId);

    for (auto userInfoIt = trigger.begin(); userInfoIt != trigger.end();)
    {
        const auto addressIt = aidAddrMap.find(userInfoIt->GetAid12());
        NS_ASSERT_MSG(addressIt != aidAddrMap.cend(),
                      "AID " << userInfoIt->GetAid12() << " not found");
        const auto address = addressIt->second;

        if (predicate(linkId, address))
        {
            NS_LOG_INFO("Removing User Info field addressed to " << address);

            userInfoIt = trigger.RemoveUserInfoField(userInfoIt);

            if (txParams.m_acknowledgment->method == WifiAcknowledgment::UL_MU_MULTI_STA_BA)
            {
                auto acknowledgment =
                    static_cast<WifiUlMuMultiStaBa*>(txParams.m_acknowledgment.get());
                for (auto it = acknowledgment->stationsReceivingMultiStaBa.begin();
                     it != acknowledgment->stationsReceivingMultiStaBa.end();)
                {
                    if (it->first.first == address)
                    {
                        it = acknowledgment->stationsReceivingMultiStaBa.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
            }
        }
        else
        {
            ++userInfoIt;
        }
    }
}

void
MultiUserScheduler::RemoveRecipientsFromDlMu(
    uint8_t linkId,
    WifiPsduMap& psduMap,
    WifiTxParameters& txParams,
    std::function<bool(uint8_t, Mac48Address)> predicate) const
{
    NS_LOG_FUNCTION(this << linkId << &txParams);

    const auto& aidAddrMap = m_apMac->GetStaList(linkId);

    for (auto psduMapIt = psduMap.begin(); psduMapIt != psduMap.end();)
    {
        const auto addressIt = aidAddrMap.find(psduMapIt->first);
        NS_ASSERT_MSG(addressIt != aidAddrMap.cend(), "AID " << psduMapIt->first << " not found");
        const auto address = addressIt->second;

        if (predicate(linkId, address))
        {
            NS_LOG_INFO("Removing PSDU addressed to " << address);

            txParams.m_txVector.GetHeMuUserInfoMap().erase(psduMapIt->first);
            psduMapIt = psduMap.erase(psduMapIt);

            if (txParams.m_acknowledgment->method == WifiAcknowledgment::DL_MU_BAR_BA_SEQUENCE)
            {
                auto acknowledgment =
                    static_cast<WifiDlMuBarBaSequence*>(txParams.m_acknowledgment.get());
                acknowledgment->stationsReplyingWithNormalAck.erase(address);
                acknowledgment->stationsReplyingWithBlockAck.erase(address);
                acknowledgment->stationsSendBlockAckReqTo.erase(address);
            }
            else if (txParams.m_acknowledgment->method == WifiAcknowledgment::DL_MU_TF_MU_BAR)
            {
                auto acknowledgment =
                    static_cast<WifiDlMuTfMuBar*>(txParams.m_acknowledgment.get());
                acknowledgment->stationsReplyingWithBlockAck.erase(address);
            }
            else if (txParams.m_acknowledgment->method == WifiAcknowledgment::DL_MU_AGGREGATE_TF)
            {
                auto acknowledgment =
                    static_cast<WifiDlMuAggregateTf*>(txParams.m_acknowledgment.get());
                acknowledgment->stationsReplyingWithBlockAck.erase(address);
            }
        }
        else
        {
            ++psduMapIt;
        }
    }
}

void
MultiUserScheduler::NotifyProtectionCompleted(uint8_t linkId,
                                              WifiPsduMap& psduMap,
                                              WifiTxParameters& txParams)
{
    NS_LOG_FUNCTION(this << linkId << &txParams);

    if (txParams.m_txVector.IsDlMu())
    {
        NS_ASSERT(GetLastTxFormat(linkId) == DL_MU_TX);

        UpdateDlMuAfterProtection(linkId, psduMap, txParams);
    }
    else if (IsTrigger(psduMap))
    {
        NS_ASSERT(GetLastTxFormat(linkId) == UL_MU_TX);

        UpdateTriggerFrameAfterProtection(linkId, GetUlMuInfo(linkId).trigger, txParams);

        if (GetUlMuInfo(linkId).trigger.GetNUserInfoFields() == 0)
        {
            NS_LOG_INFO("No User Info field left");
            psduMap.clear();
        }
        else
        {
            auto mpdu = GetTriggerFrame(GetUlMuInfo(linkId).trigger, linkId);
            GetUlMuInfo(linkId).macHdr = mpdu->GetHeader();
            psduMap =
                WifiPsduMap{{SU_STA_ID, GetHeFem(linkId)->GetWifiPsdu(mpdu, txParams.m_txVector)}};
        }
    }
}

uint32_t
MultiUserScheduler::GetMaxSizeOfQosNullAmpdu(const CtrlTriggerHeader& trigger) const
{
    // find the maximum number of TIDs for which a BlockAck agreement has been established
    // with an STA, among all the STAs solicited by the given Trigger Frame
    uint8_t maxNTids = 0;
    for (const auto& userInfo : trigger)
    {
        auto address = m_apMac->GetMldOrLinkAddressByAid(userInfo.GetAid12());
        NS_ASSERT_MSG(address, "AID " << userInfo.GetAid12() << " not found");

        uint8_t staNTids = 0;
        for (uint8_t tid = 0; tid < 8; tid++)
        {
            if (m_apMac->GetBaAgreementEstablishedAsRecipient(*address, tid))
            {
                staNTids++;
            }
        }
        maxNTids = std::max(maxNTids, staNTids);
    }

    // compute the size in bytes of maxNTids QoS Null frames
    WifiMacHeader header(WIFI_MAC_QOSDATA_NULL);
    header.SetDsTo();
    header.SetDsNotFrom();
    uint32_t headerSize = header.GetSerializedSize();
    uint32_t maxSize = 0;

    for (uint8_t i = 0; i < maxNTids; i++)
    {
        maxSize = MpduAggregator::GetSizeIfAggregated(headerSize + WIFI_MAC_FCS_LENGTH, maxSize);
    }

    return maxSize;
}

} // namespace ns3
