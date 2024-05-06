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
                          MakeTimeAccessor(&MultiUserScheduler::m_accessReqInterval),
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
                          MakeBooleanChecker());
    return tid;
}

MultiUserScheduler::MultiUserScheduler()
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
    m_accessReqTimer.Cancel();
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
        m_accessReqTimer =
            Simulator::Schedule(m_accessReqInterval, &MultiUserScheduler::AccessReqTimeout, this);
    }
}

void
MultiUserScheduler::SetAccessReqInterval(Time interval)
{
    NS_LOG_FUNCTION(this << interval.As(Time::MS));
    m_accessReqInterval = interval;
    // start the timer if past initialization
    if (m_accessReqInterval.IsStrictlyPositive() && IsInitialized())
    {
        m_accessReqTimer =
            Simulator::Schedule(m_accessReqInterval, &MultiUserScheduler::AccessReqTimeout, this);
    }
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

void
MultiUserScheduler::AccessReqTimeout()
{
    NS_LOG_FUNCTION(this);

    // request channel access if not requested yet
    auto edca = m_apMac->GetQosTxop(m_accessReqAc);

    for (uint8_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
    {
        if (edca->GetAccessStatus(linkId) == Txop::NOT_REQUESTED)
        {
            m_apMac->GetChannelAccessManager(linkId)->RequestAccess(edca);
        }
    }

    // restart timer
    if (m_accessReqInterval.IsStrictlyPositive())
    {
        m_accessReqTimer =
            Simulator::Schedule(m_accessReqInterval, &MultiUserScheduler::AccessReqTimeout, this);
    }
}

MultiUserScheduler::TxFormat
MultiUserScheduler::NotifyAccessGranted(Ptr<QosTxop> edca,
                                        Time availableTime,
                                        bool initialFrame,
                                        uint16_t allowedWidth,
                                        uint8_t linkId)
{
    NS_LOG_FUNCTION(this << edca << availableTime << initialFrame << allowedWidth << linkId);

    m_edca = edca;
    m_availableTime = availableTime;
    m_initialFrame = initialFrame;
    m_allowedWidth = allowedWidth;
    m_linkId = linkId;

    if (m_accessReqTimer.IsPending() && m_restartTimerUponAccess)
    {
        // restart access timer
        m_accessReqTimer.Cancel();
        if (m_accessReqInterval.IsStrictlyPositive())
        {
            m_accessReqTimer = Simulator::Schedule(m_accessReqInterval,
                                                   &MultiUserScheduler::AccessReqTimeout,
                                                   this);
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
MultiUserScheduler::GetLastTxFormat(uint8_t linkId)
{
    return m_lastTxInfo[linkId].lastTxFormat;
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
