/*
 * Copyright (c) 2005 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "txop.h"

#include "channel-access-manager.h"
#include "mac-tx-middle.h"
#include "wifi-mac-queue-scheduler.h"
#include "wifi-mac-queue.h"
#include "wifi-mac-trailer.h"
#include "wifi-mac.h"
#include "wifi-phy.h"

#include "ns3/attribute-container.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/shuffle.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"

#include <iterator>
#include <sstream>

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT WIFI_TXOP_NS_LOG_APPEND_CONTEXT

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Txop");

NS_OBJECT_ENSURE_REGISTERED(Txop);

TypeId
Txop::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::Txop")
            .SetParent<ns3::Object>()
            .SetGroupName("Wifi")
            .AddConstructor<Txop>()
            .AddAttribute("AcIndex",
                          "The AC index of the packets contained in the wifi MAC queue of this "
                          "Txop object.",
                          EnumValue(AcIndex::AC_UNDEF),
                          MakeEnumAccessor<AcIndex>(&Txop::CreateQueue),
                          MakeEnumChecker(AC_BE,
                                          "AC_BE",
                                          AC_BK,
                                          "AC_BK",
                                          AC_VI,
                                          "AC_VI",
                                          AC_VO,
                                          "AC_VO",
                                          AC_BE_NQOS,
                                          "AC_BE_NQOS",
                                          AC_VI,
                                          "AC_VI",
                                          AC_BEACON,
                                          "AC_BEACON",
                                          AC_UNDEF,
                                          "AC_UNDEF"))
            .AddAttribute("MinCw",
                          "The minimum value of the contention window (just for the first link, "
                          "in case of 11be multi-link devices).",
                          TypeId::ATTR_GET | TypeId::ATTR_SET, // do not set at construction time
                          UintegerValue(15),
                          MakeUintegerAccessor((void(Txop::*)(uint32_t)) & Txop::SetMinCw,
                                               (uint32_t(Txop::*)() const) & Txop::GetMinCw),
                          MakeUintegerChecker<uint32_t>(),
                          TypeId::SupportLevel::OBSOLETE,
                          "Use MinCws attribute instead of MinCw")
            .AddAttribute(
                "MinCws",
                "The minimum values of the contention window for all the links (sorted in "
                "increasing order of link ID). An empty vector is ignored and the default value "
                "as per Table 9-155 of the IEEE 802.11-2020 standard will be used. Note that, if "
                "this is a non-AP STA, these values could be overridden by values advertised by "
                "the AP through EDCA Parameter Set elements.",
                AttributeContainerValue<UintegerValue>(),
                MakeAttributeContainerAccessor<UintegerValue>(&Txop::SetMinCws, &Txop::GetMinCws),
                MakeAttributeContainerChecker<UintegerValue>(MakeUintegerChecker<uint32_t>()))
            .AddAttribute("MaxCw",
                          "The maximum value of the contention window (just for the first link, "
                          "in case of 11be multi-link devices).",
                          TypeId::ATTR_GET | TypeId::ATTR_SET, // do not set at construction time
                          UintegerValue(1023),
                          MakeUintegerAccessor((void(Txop::*)(uint32_t)) & Txop::SetMaxCw,
                                               (uint32_t(Txop::*)() const) & Txop::GetMaxCw),
                          MakeUintegerChecker<uint32_t>(),
                          TypeId::SupportLevel::OBSOLETE,
                          "Use MaxCws attribute instead of MaxCw")
            .AddAttribute(
                "MaxCws",
                "The maximum values of the contention window for all the links (sorted in "
                "increasing order of link ID). An empty vector is ignored and the default value "
                "as per Table 9-155 of the IEEE 802.11-2020 standard will be used. Note that, if "
                "this is a non-AP STA, these values could be overridden by values advertised by "
                "the AP through EDCA Parameter Set elements.",
                AttributeContainerValue<UintegerValue>(),
                MakeAttributeContainerAccessor<UintegerValue>(&Txop::SetMaxCws, &Txop::GetMaxCws),
                MakeAttributeContainerChecker<UintegerValue>(MakeUintegerChecker<uint32_t>()))
            .AddAttribute(
                "Aifsn",
                "The AIFSN: the default value conforms to non-QOS (just for the first link, "
                "in case of 11be multi-link devices).",
                TypeId::ATTR_GET | TypeId::ATTR_SET, // do not set at construction time
                UintegerValue(2),
                MakeUintegerAccessor((void(Txop::*)(uint8_t)) & Txop::SetAifsn,
                                     (uint8_t(Txop::*)() const) & Txop::GetAifsn),
                MakeUintegerChecker<uint8_t>(),
                TypeId::SupportLevel::OBSOLETE,
                "Use Aifsns attribute instead of Aifsn")
            .AddAttribute(
                "Aifsns",
                "The values of AIFSN for all the links (sorted in increasing order "
                "of link ID). An empty vector is ignored and the default value as per "
                "Table 9-155 of the IEEE 802.11-2020 standard will be used. Note that, if "
                "this is a non-AP STA, these values could be overridden by values advertised by "
                "the AP through EDCA Parameter Set elements.",
                AttributeContainerValue<UintegerValue>(),
                MakeAttributeContainerAccessor<UintegerValue>(&Txop::SetAifsns, &Txop::GetAifsns),
                MakeAttributeContainerChecker<UintegerValue>(MakeUintegerChecker<uint8_t>()))
            .AddAttribute("TxopLimit",
                          "The TXOP limit: the default value conforms to non-QoS "
                          "(just for the first link, in case of 11be multi-link devices).",
                          TypeId::ATTR_GET | TypeId::ATTR_SET, // do not set at construction time
                          TimeValue(MilliSeconds(0)),
                          MakeTimeAccessor((void(Txop::*)(Time)) & Txop::SetTxopLimit,
                                           (Time(Txop::*)() const) & Txop::GetTxopLimit),
                          MakeTimeChecker(),
                          TypeId::SupportLevel::OBSOLETE,
                          "Use TxopLimits attribute instead of TxopLimit")
            .AddAttribute(
                "TxopLimits",
                "The values of TXOP limit for all the links (sorted in increasing order "
                "of link ID). An empty vector is ignored and the default value as per "
                "Table 9-155 of the IEEE 802.11-2020 standard will be used. Note that, if "
                "this is a non-AP STA, these values could be overridden by values advertised by "
                "the AP through EDCA Parameter Set elements.",
                AttributeContainerValue<TimeValue>(),
                MakeAttributeContainerAccessor<TimeValue>(&Txop::SetTxopLimits,
                                                          &Txop::GetTxopLimits),
                MakeAttributeContainerChecker<TimeValue>(MakeTimeChecker()))
            .AddAttribute("Queue",
                          "The WifiMacQueue object",
                          PointerValue(),
                          MakePointerAccessor(&Txop::GetWifiMacQueue),
                          MakePointerChecker<WifiMacQueue>())
            .AddTraceSource("BackoffTrace",
                            "Trace source for backoff values",
                            MakeTraceSourceAccessor(&Txop::m_backoffTrace),
                            "ns3::Txop::BackoffValueTracedCallback")
            .AddTraceSource("CwTrace",
                            "Trace source for contention window values",
                            MakeTraceSourceAccessor(&Txop::m_cwTrace),
                            "ns3::Txop::CwValueTracedCallback");
    return tid;
}

Txop::Txop()
{
    NS_LOG_FUNCTION(this);
    m_rng = m_shuffleLinkIdsGen.GetRv();
}

Txop::~Txop()
{
    NS_LOG_FUNCTION(this);
}

void
Txop::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_queue = nullptr;
    m_mac = nullptr;
    m_rng = nullptr;
    m_txMiddle = nullptr;
    m_links.clear();
}

void
Txop::CreateQueue(AcIndex aci)
{
    NS_LOG_FUNCTION(this << aci);
    NS_ABORT_MSG_IF(m_queue, "Wifi MAC queue can only be created once");
    m_queue = CreateObject<WifiMacQueue>(aci);
}

std::unique_ptr<Txop::LinkEntity>
Txop::CreateLinkEntity() const
{
    return std::make_unique<LinkEntity>();
}

Txop::LinkEntity&
Txop::GetLink(uint8_t linkId) const
{
    auto it = m_links.find(linkId);
    NS_ASSERT(it != m_links.cend());
    NS_ASSERT(it->second); // check that the pointer owns an object
    return *it->second;
}

const std::map<uint8_t, std::unique_ptr<Txop::LinkEntity>>&
Txop::GetLinks() const
{
    return m_links;
}

void
Txop::SwapLinks(std::map<uint8_t, uint8_t> links)
{
    NS_LOG_FUNCTION(this);

    decltype(m_links) tmp;
    tmp.swap(m_links); // move all links to temporary map
    for (const auto& [from, to] : links)
    {
        auto nh = tmp.extract(from);
        nh.key() = to;
        m_links.insert(std::move(nh));
    }
    // move links remaining in tmp to m_links
    m_links.merge(tmp);
}

void
Txop::SetTxMiddle(const Ptr<MacTxMiddle> txMiddle)
{
    NS_LOG_FUNCTION(this);
    m_txMiddle = txMiddle;
}

void
Txop::SetWifiMac(const Ptr<WifiMac> mac)
{
    NS_LOG_FUNCTION(this << mac);
    m_mac = mac;
    for (const auto linkId : m_mac->GetLinkIds())
    {
        m_links.emplace(linkId, CreateLinkEntity());
    }
}

void
Txop::SetDroppedMpduCallback(DroppedMpdu callback)
{
    NS_LOG_FUNCTION(this << &callback);
    m_droppedMpduCallback = callback;
    m_queue->TraceConnectWithoutContext("DropBeforeEnqueue",
                                        m_droppedMpduCallback.Bind(WIFI_MAC_DROP_FAILED_ENQUEUE));
    m_queue->TraceConnectWithoutContext("Expired",
                                        m_droppedMpduCallback.Bind(WIFI_MAC_DROP_EXPIRED_LIFETIME));
}

Ptr<WifiMacQueue>
Txop::GetWifiMacQueue() const
{
    return m_queue;
}

void
Txop::SetMinCw(uint32_t minCw)
{
    SetMinCw(minCw, 0);
}

void
Txop::SetMinCws(const std::vector<uint32_t>& minCws)
{
    if (minCws.empty())
    {
        // an empty vector is passed to use the default values specified by the standard
        return;
    }

    NS_ABORT_MSG_IF(!m_links.empty() && minCws.size() != m_links.size(),
                    "The size of the given vector (" << minCws.size()
                                                     << ") does not match the number of links ("
                                                     << m_links.size() << ")");
    m_userAccessParams.cwMins = minCws;

    std::size_t i = 0;
    for (const auto& [id, link] : m_links)
    {
        SetMinCw(minCws[i++], id);
    }
}

void
Txop::SetMinCw(uint32_t minCw, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << minCw << linkId);
    NS_ASSERT_MSG(!m_links.empty(),
                  "This function can only be called after that links have been created");
    auto& link = GetLink(linkId);
    bool changed = (link.cwMin != minCw);
    link.cwMin = minCw;
    if (changed)
    {
        ResetCw(linkId);
    }
}

void
Txop::SetMaxCw(uint32_t maxCw)
{
    SetMaxCw(maxCw, 0);
}

void
Txop::SetMaxCws(const std::vector<uint32_t>& maxCws)
{
    if (maxCws.empty())
    {
        // an empty vector is passed to use the default values specified by the standard
        return;
    }

    NS_ABORT_MSG_IF(!m_links.empty() && maxCws.size() != m_links.size(),
                    "The size of the given vector (" << maxCws.size()
                                                     << ") does not match the number of links ("
                                                     << m_links.size() << ")");
    m_userAccessParams.cwMaxs = maxCws;

    std::size_t i = 0;
    for (const auto& [id, link] : m_links)
    {
        SetMaxCw(maxCws[i++], id);
    }
}

void
Txop::SetMaxCw(uint32_t maxCw, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << maxCw << linkId);
    NS_ASSERT_MSG(!m_links.empty(),
                  "This function can only be called after that links have been created");
    auto& link = GetLink(linkId);
    bool changed = (link.cwMax != maxCw);
    link.cwMax = maxCw;
    if (changed)
    {
        ResetCw(linkId);
    }
}

uint32_t
Txop::GetCw(uint8_t linkId) const
{
    return GetLink(linkId).cw;
}

std::size_t
Txop::GetStaRetryCount(uint8_t linkId) const
{
    return GetLink(linkId).staRetryCount;
}

void
Txop::ResetCw(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);
    auto& link = GetLink(linkId);
    link.cw = GetMinCw(linkId);
    m_cwTrace(link.cw, linkId);
    link.staRetryCount = 0;
}

void
Txop::UpdateFailedCw(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);
    auto& link = GetLink(linkId);

    if (link.staRetryCount < m_mac->GetFrameRetryLimit())
    {
        // If QSRC[AC] is less than dot11ShortRetryLimit,
        // - QSRC[AC] shall be incremented by 1.
        // - CW[AC] shall be set to the lesser of CWmax[AC] and 2^QSRC[AC] × (CWmin[AC] + 1) – 1.
        // (Section 10.23.2.2 of 802.11-2020)
        ++link.staRetryCount;
        link.cw =
            std::min(GetMaxCw(linkId), (1 << link.staRetryCount) * (GetMinCw(linkId) + 1) - 1);
    }
    else
    {
        //  Else
        // - QSRC[AC] shall be set to 0.
        // - CW[AC] shall be set to CWmin[AC].
        link.staRetryCount = 0;
        link.cw = GetMinCw(linkId);
    }

    m_cwTrace(link.cw, linkId);
}

uint32_t
Txop::GetBackoffSlots(uint8_t linkId) const
{
    return GetLink(linkId).backoffSlots;
}

Time
Txop::GetBackoffStart(uint8_t linkId) const
{
    return GetLink(linkId).backoffStart;
}

void
Txop::UpdateBackoffSlotsNow(uint32_t nSlots, Time backoffUpdateBound, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << nSlots << backoffUpdateBound << linkId);
    auto& link = GetLink(linkId);

    link.backoffSlots -= nSlots;
    link.backoffStart = backoffUpdateBound;
    NS_LOG_DEBUG("update slots=" << nSlots << " slots, backoff=" << link.backoffSlots);
}

void
Txop::StartBackoffNow(uint32_t nSlots, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << nSlots << linkId);
    auto& link = GetLink(linkId);

    if (link.backoffSlots != 0)
    {
        NS_LOG_DEBUG("reset backoff from " << link.backoffSlots << " to " << nSlots << " slots");
    }
    else
    {
        NS_LOG_DEBUG("start backoff=" << nSlots << " slots");
    }
    link.backoffSlots = nSlots;
    link.backoffStart = Simulator::Now();
}

void
Txop::SetAifsn(uint8_t aifsn)
{
    SetAifsn(aifsn, 0);
}

void
Txop::SetAifsns(const std::vector<uint8_t>& aifsns)
{
    if (aifsns.empty())
    {
        // an empty vector is passed to use the default values specified by the standard
        return;
    }

    NS_ABORT_MSG_IF(!m_links.empty() && aifsns.size() != m_links.size(),
                    "The size of the given vector (" << aifsns.size()
                                                     << ") does not match the number of links ("
                                                     << m_links.size() << ")");
    m_userAccessParams.aifsns = aifsns;

    std::size_t i = 0;
    for (const auto& [id, link] : m_links)
    {
        SetAifsn(aifsns[i++], id);
    }
}

void
Txop::SetAifsn(uint8_t aifsn, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << aifsn << linkId);
    NS_ASSERT_MSG(!m_links.empty(),
                  "This function can only be called after that links have been created");
    GetLink(linkId).aifsn = aifsn;
}

void
Txop::SetTxopLimit(Time txopLimit)
{
    SetTxopLimit(txopLimit, 0);
}

void
Txop::SetTxopLimits(const std::vector<Time>& txopLimits)
{
    if (txopLimits.empty())
    {
        // an empty vector is passed to use the default values specified by the standard
        return;
    }

    NS_ABORT_MSG_IF(!m_links.empty() && txopLimits.size() != m_links.size(),
                    "The size of the given vector (" << txopLimits.size()
                                                     << ") does not match the number of links ("
                                                     << m_links.size() << ")");
    m_userAccessParams.txopLimits = txopLimits;

    std::size_t i = 0;
    for (const auto& [id, link] : m_links)
    {
        SetTxopLimit(txopLimits[i++], id);
    }
}

void
Txop::SetTxopLimit(Time txopLimit, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << txopLimit << linkId);
    NS_ASSERT_MSG(txopLimit.IsPositive(), "TXOP limit cannot be negative");
    NS_ASSERT_MSG((txopLimit.GetMicroSeconds() % 32 == 0),
                  "The TXOP limit must be expressed in multiple of 32 microseconds!");
    NS_ASSERT_MSG(!m_links.empty(),
                  "This function can only be called after that links have been created");
    GetLink(linkId).txopLimit = txopLimit;
}

const Txop::UserDefinedAccessParams&
Txop::GetUserAccessParams() const
{
    return m_userAccessParams;
}

uint32_t
Txop::GetMinCw() const
{
    return GetMinCw(0);
}

std::vector<uint32_t>
Txop::GetMinCws() const
{
    std::vector<uint32_t> ret;
    ret.reserve(m_links.size());
    for (const auto& [id, link] : m_links)
    {
        ret.push_back(link->cwMin);
    }
    return ret;
}

uint32_t
Txop::GetMinCw(uint8_t linkId) const
{
    return GetLink(linkId).cwMin;
}

uint32_t
Txop::GetMaxCw() const
{
    return GetMaxCw(0);
}

std::vector<uint32_t>
Txop::GetMaxCws() const
{
    std::vector<uint32_t> ret;
    ret.reserve(m_links.size());
    for (const auto& [id, link] : m_links)
    {
        ret.push_back(link->cwMax);
    }
    return ret;
}

uint32_t
Txop::GetMaxCw(uint8_t linkId) const
{
    return GetLink(linkId).cwMax;
}

uint8_t
Txop::GetAifsn() const
{
    return GetAifsn(0);
}

std::vector<uint8_t>
Txop::GetAifsns() const
{
    std::vector<uint8_t> ret;
    ret.reserve(m_links.size());
    for (const auto& [id, link] : m_links)
    {
        ret.push_back(link->aifsn);
    }
    return ret;
}

uint8_t
Txop::GetAifsn(uint8_t linkId) const
{
    return GetLink(linkId).aifsn;
}

Time
Txop::GetTxopLimit() const
{
    return GetTxopLimit(0);
}

std::vector<Time>
Txop::GetTxopLimits() const
{
    std::vector<Time> ret;
    ret.reserve(m_links.size());
    for (const auto& [id, link] : m_links)
    {
        ret.push_back(link->txopLimit);
    }
    return ret;
}

Time
Txop::GetTxopLimit(uint8_t linkId) const
{
    return GetLink(linkId).txopLimit;
}

bool
Txop::HasFramesToTransmit(uint8_t linkId)
{
    m_queue->WipeAllExpiredMpdus();
    bool ret = static_cast<bool>(m_queue->Peek(linkId));
    NS_LOG_FUNCTION(this << linkId << ret);
    return ret;
}

void
Txop::Queue(Ptr<WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << *mpdu);

    // channel access can be requested on a blocked link, if the reason for blocking the link
    // is temporary
    auto linkIds = m_mac->GetMacQueueScheduler()->GetLinkIds(
        m_queue->GetAc(),
        mpdu,
        {WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
         WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY});

    // ignore the links for which a channel access request event is already running
    for (auto it = linkIds.begin(); it != linkIds.end();)
    {
        if (const auto& event = GetLink(*it).accessRequest.event; event.IsPending())
        {
            it = linkIds.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // save the status of the AC queues before enqueuing the MPDU (required to determine if
    // backoff is needed)
    std::map<uint8_t, bool> hasFramesToTransmit;
    for (const auto linkId : linkIds)
    {
        hasFramesToTransmit[linkId] = HasFramesToTransmit(linkId);
    }
    m_queue->Enqueue(mpdu);

    // shuffle link IDs not to request channel access on links always in the same order
    std::vector<uint8_t> shuffledLinkIds(linkIds.cbegin(), linkIds.cend());
    Shuffle(shuffledLinkIds.begin(), shuffledLinkIds.end(), m_shuffleLinkIdsGen.GetRv());

    if (!linkIds.empty() && g_log.IsEnabled(ns3::LOG_DEBUG))
    {
        std::stringstream ss;
        std::copy(shuffledLinkIds.cbegin(),
                  shuffledLinkIds.cend(),
                  std::ostream_iterator<uint16_t>(ss, " "));
        NS_LOG_DEBUG("Request channel access on link IDs: " << ss.str());
    }

    for (const auto linkId : shuffledLinkIds)
    {
        // schedule a call to StartAccessIfNeeded() to request channel access after that all the
        // packets of a burst have been enqueued, instead of requesting channel access right after
        // the first packet. The call to StartAccessIfNeeded() is scheduled only after the first
        // packet
        GetLink(linkId).accessRequest.event = Simulator::ScheduleNow(&Txop::StartAccessAfterEvent,
                                                                     this,
                                                                     linkId,
                                                                     hasFramesToTransmit.at(linkId),
                                                                     CHECK_MEDIUM_BUSY);
    }
}

int64_t
Txop::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_rng->SetStream(stream);
    return 1;
}

void
Txop::StartAccessAfterEvent(uint8_t linkId, bool hadFramesToTransmit, bool checkMediumBusy)
{
    NS_LOG_FUNCTION(this << linkId << hadFramesToTransmit << checkMediumBusy);

    if (!m_mac->GetWifiPhy(linkId))
    {
        NS_LOG_DEBUG("No PHY operating on link " << +linkId);
        return;
    }

    if (GetLink(linkId).access != NOT_REQUESTED)
    {
        NS_LOG_DEBUG("Channel access already requested or granted on link " << +linkId);
        return;
    }

    if (!HasFramesToTransmit(linkId))
    {
        NS_LOG_DEBUG("No frames to transmit on link " << +linkId);
        return;
    }

    if (m_mac->GetChannelAccessManager(linkId)->NeedBackoffUponAccess(this,
                                                                      hadFramesToTransmit,
                                                                      checkMediumBusy))
    {
        GenerateBackoff(linkId);
    }

    m_mac->GetChannelAccessManager(linkId)->RequestAccess(this);
}

void
Txop::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    for (const auto& [id, link] : m_links)
    {
        ResetCw(id);
        GenerateBackoff(id);
    }
}

Txop::ChannelAccessStatus
Txop::GetAccessStatus(uint8_t linkId) const
{
    return GetLink(linkId).access;
}

void
Txop::NotifyAccessRequested(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);
    GetLink(linkId).access = REQUESTED;
}

void
Txop::NotifyChannelAccessed(uint8_t linkId, Time txopDuration)
{
    NS_LOG_FUNCTION(this << linkId << txopDuration);
    GetLink(linkId).access = GRANTED;
}

void
Txop::NotifyChannelReleased(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);
    GetLink(linkId).access = NOT_REQUESTED;
    GenerateBackoff(linkId);
    if (HasFramesToTransmit(linkId))
    {
        Simulator::ScheduleNow(&Txop::RequestAccess, this, linkId);
    }
}

void
Txop::RequestAccess(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);
    if (GetLink(linkId).access == NOT_REQUESTED)
    {
        m_mac->GetChannelAccessManager(linkId)->RequestAccess(this);
    }
}

void
Txop::GenerateBackoff(uint8_t linkId)
{
    uint32_t backoff = m_rng->GetInteger(0, GetCw(linkId));
    NS_LOG_FUNCTION(this << linkId << backoff);
    m_backoffTrace(backoff, linkId);
    StartBackoffNow(backoff, linkId);
}

void
Txop::NotifySleep(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);
}

void
Txop::NotifyOff()
{
    NS_LOG_FUNCTION(this);
    m_queue->Flush();
}

void
Txop::NotifyWakeUp(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);
    // before wake up, no packet can be transmitted
    StartAccessAfterEvent(linkId, DIDNT_HAVE_FRAMES_TO_TRANSMIT, DONT_CHECK_MEDIUM_BUSY);
}

void
Txop::NotifyOn()
{
    NS_LOG_FUNCTION(this);
    for (const auto& [id, link] : m_links)
    {
        // before being turned on, no packet can be transmitted
        StartAccessAfterEvent(id, DIDNT_HAVE_FRAMES_TO_TRANSMIT, DONT_CHECK_MEDIUM_BUSY);
    }
}

bool
Txop::IsQosTxop() const
{
    return false;
}

} // namespace ns3
