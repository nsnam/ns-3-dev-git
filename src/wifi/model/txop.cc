/*
 * Copyright (c) 2005 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "txop.h"

#include "channel-access-manager.h"
#include "mac-tx-middle.h"
#include "wifi-mac-queue-scheduler.h"
#include "wifi-mac-queue.h"
#include "wifi-mac-trailer.h"
#include "wifi-mac.h"

#include "ns3/attribute-container.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT                                                                      \
    if (m_mac)                                                                                     \
    {                                                                                              \
        std::clog << "[mac=" << m_mac->GetAddress() << "] ";                                       \
    }

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
            .AddAttribute("MinCw",
                          "The minimum value of the contention window (just for the first link, "
                          "in case of 11be multi-link devices).",
                          TypeId::ATTR_GET | TypeId::ATTR_SET, // do not set at construction time
                          UintegerValue(15),
                          MakeUintegerAccessor((void(Txop::*)(uint32_t)) & Txop::SetMinCw,
                                               (uint32_t(Txop::*)() const) & Txop::GetMinCw),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute(
                "MinCws",
                "The minimum values of the contention window for all the links",
                TypeId::ATTR_GET | TypeId::ATTR_SET, // do not set at construction time
                AttributeContainerValue<UintegerValue>(),
                MakeAttributeContainerAccessor<UintegerValue, std::list>(&Txop::SetMinCws,
                                                                         &Txop::GetMinCws),
                MakeAttributeContainerChecker<UintegerValue>(MakeUintegerChecker<uint32_t>()))
            .AddAttribute("MaxCw",
                          "The maximum value of the contention window (just for the first link, "
                          "in case of 11be multi-link devices).",
                          TypeId::ATTR_GET | TypeId::ATTR_SET, // do not set at construction time
                          UintegerValue(1023),
                          MakeUintegerAccessor((void(Txop::*)(uint32_t)) & Txop::SetMaxCw,
                                               (uint32_t(Txop::*)() const) & Txop::GetMaxCw),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute(
                "MaxCws",
                "The maximum values of the contention window for all the links",
                TypeId::ATTR_GET | TypeId::ATTR_SET, // do not set at construction time
                AttributeContainerValue<UintegerValue>(),
                MakeAttributeContainerAccessor<UintegerValue, std::list>(&Txop::SetMaxCws,
                                                                         &Txop::GetMaxCws),
                MakeAttributeContainerChecker<UintegerValue>(MakeUintegerChecker<uint32_t>()))
            .AddAttribute(
                "Aifsn",
                "The AIFSN: the default value conforms to non-QOS (just for the first link, "
                "in case of 11be multi-link devices).",
                TypeId::ATTR_GET | TypeId::ATTR_SET, // do not set at construction time
                UintegerValue(2),
                MakeUintegerAccessor((void(Txop::*)(uint8_t)) & Txop::SetAifsn,
                                     (uint8_t(Txop::*)() const) & Txop::GetAifsn),
                MakeUintegerChecker<uint8_t>())
            .AddAttribute(
                "Aifsns",
                "The values of AIFSN for all the links",
                TypeId::ATTR_GET | TypeId::ATTR_SET, // do not set at construction time
                AttributeContainerValue<UintegerValue>(),
                MakeAttributeContainerAccessor<UintegerValue, std::list>(&Txop::SetAifsns,
                                                                         &Txop::GetAifsns),
                MakeAttributeContainerChecker<UintegerValue>(MakeUintegerChecker<uint8_t>()))
            .AddAttribute("TxopLimit",
                          "The TXOP limit: the default value conforms to non-QoS "
                          "(just for the first link, in case of 11be multi-link devices).",
                          TypeId::ATTR_GET | TypeId::ATTR_SET, // do not set at construction time
                          TimeValue(MilliSeconds(0)),
                          MakeTimeAccessor((void(Txop::*)(Time)) & Txop::SetTxopLimit,
                                           (Time(Txop::*)() const) & Txop::GetTxopLimit),
                          MakeTimeChecker())
            .AddAttribute(
                "TxopLimits",
                "The values of TXOP limit for all the links",
                TypeId::ATTR_GET | TypeId::ATTR_SET, // do not set at construction time
                AttributeContainerValue<TimeValue>(),
                MakeAttributeContainerAccessor<TimeValue, std::list>(&Txop::SetTxopLimits,
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
    : Txop(CreateObject<WifiMacQueue>(AC_BE_NQOS))
{
}

Txop::Txop(Ptr<WifiMacQueue> queue)
    : m_queue(queue)
{
    NS_LOG_FUNCTION(this);
    m_rng = CreateObject<UniformRandomVariable>();
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

std::unique_ptr<Txop::LinkEntity>
Txop::CreateLinkEntity() const
{
    return std::make_unique<LinkEntity>();
}

Txop::LinkEntity&
Txop::GetLink(uint8_t linkId) const
{
    NS_ASSERT(linkId < m_links.size());
    NS_ASSERT(m_links.at(linkId)); // check that the pointer owns an object
    return *m_links.at(linkId);
}

uint8_t
Txop::GetNLinks() const
{
    return m_links.size();
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
    m_links.resize(m_mac->GetNLinks());
    uint8_t linkId = 0;
    for (auto& link : m_links)
    {
        link = CreateLinkEntity();
        link->id = linkId++;
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
    NS_LOG_FUNCTION(this);
    return m_queue;
}

void
Txop::SetMinCw(uint32_t minCw)
{
    SetMinCw(minCw, 0);
}

void
Txop::SetMinCws(std::vector<uint32_t> minCws)
{
    NS_ABORT_IF(minCws.size() != m_links.size());
    for (std::size_t linkId = 0; linkId < minCws.size(); linkId++)
    {
        SetMinCw(minCws[linkId], linkId);
    }
}

void
Txop::SetMinCw(uint32_t minCw, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << minCw << +linkId);
    auto& link = GetLink(linkId);
    bool changed = (link.cwMin != minCw);
    link.cwMin = minCw;
    if (changed == true)
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
Txop::SetMaxCws(std::vector<uint32_t> maxCws)
{
    NS_ABORT_IF(maxCws.size() != m_links.size());
    for (std::size_t linkId = 0; linkId < maxCws.size(); linkId++)
    {
        SetMaxCw(maxCws[linkId], linkId);
    }
}

void
Txop::SetMaxCw(uint32_t maxCw, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << maxCw << +linkId);
    auto& link = GetLink(linkId);
    bool changed = (link.cwMax != maxCw);
    link.cwMax = maxCw;
    if (changed == true)
    {
        ResetCw(linkId);
    }
}

uint32_t
Txop::GetCw(uint8_t linkId) const
{
    return GetLink(linkId).cw;
}

void
Txop::ResetCw(uint8_t linkId)
{
    NS_LOG_FUNCTION(this);
    auto& link = GetLink(linkId);
    link.cw = GetMinCw(linkId);
    m_cwTrace(link.cw, linkId);
}

void
Txop::UpdateFailedCw(uint8_t linkId)
{
    NS_LOG_FUNCTION(this);
    auto& link = GetLink(linkId);
    // see 802.11-2012, section 9.19.2.5
    link.cw = std::min(2 * (link.cw + 1) - 1, GetMaxCw(linkId));
    // if the MU EDCA timer is running, CW cannot be less than MU CW min
    link.cw = std::max(link.cw, GetMinCw(linkId));
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
    NS_LOG_FUNCTION(this << nSlots << backoffUpdateBound << +linkId);
    auto& link = GetLink(linkId);

    link.backoffSlots -= nSlots;
    link.backoffStart = backoffUpdateBound;
    NS_LOG_DEBUG("update slots=" << nSlots << " slots, backoff=" << link.backoffSlots);
}

void
Txop::StartBackoffNow(uint32_t nSlots, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << nSlots << +linkId);
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
Txop::SetAifsns(std::vector<uint8_t> aifsns)
{
    NS_ABORT_IF(aifsns.size() != m_links.size());
    for (std::size_t linkId = 0; linkId < aifsns.size(); linkId++)
    {
        SetAifsn(aifsns[linkId], linkId);
    }
}

void
Txop::SetAifsn(uint8_t aifsn, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +aifsn << +linkId);
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
    NS_ABORT_MSG_IF(txopLimits.size() != m_links.size(),
                    "The size of the given vector (" << txopLimits.size()
                                                     << ") does not match the number of links ("
                                                     << m_links.size() << ")");
    for (std::size_t linkId = 0; linkId < txopLimits.size(); linkId++)
    {
        SetTxopLimit(txopLimits[linkId], linkId);
    }
}

void
Txop::SetTxopLimit(Time txopLimit, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << txopLimit << +linkId);
    NS_ASSERT_MSG((txopLimit.GetMicroSeconds() % 32 == 0),
                  "The TXOP limit must be expressed in multiple of 32 microseconds!");
    GetLink(linkId).txopLimit = txopLimit;
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
    for (std::size_t linkId = 0; linkId < m_links.size(); linkId++)
    {
        ret.push_back(GetMinCw(linkId));
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
    for (std::size_t linkId = 0; linkId < m_links.size(); linkId++)
    {
        ret.push_back(GetMaxCw(linkId));
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
    for (std::size_t linkId = 0; linkId < m_links.size(); linkId++)
    {
        ret.push_back(GetAifsn(linkId));
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
    for (std::size_t linkId = 0; linkId < m_links.size(); linkId++)
    {
        ret.push_back(GetTxopLimit(linkId));
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
    NS_LOG_FUNCTION(this << +linkId << ret);
    return ret;
}

void
Txop::Queue(Ptr<Packet> packet, const WifiMacHeader& hdr)
{
    NS_LOG_FUNCTION(this << packet << &hdr);
    // remove the priority tag attached, if any
    SocketPriorityTag priorityTag;
    packet->RemovePacketTag(priorityTag);
    Queue(Create<WifiMpdu>(packet, hdr));
}

void
Txop::Queue(Ptr<WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << *mpdu);
    const auto linkIds =
        m_mac->GetMacQueueScheduler()->GetLinkIds(m_queue->GetAc(),
                                                  WifiMacQueueContainer::GetQueueId(mpdu));
    for (const auto linkId : linkIds)
    {
        if (m_mac->GetChannelAccessManager(linkId)->NeedBackoffUponAccess(this))
        {
            GenerateBackoff(linkId);
        }
    }
    m_queue->Enqueue(mpdu);
    for (const auto linkId : linkIds)
    {
        StartAccessIfNeeded(linkId);
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
Txop::StartAccessIfNeeded(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +linkId);
    if (HasFramesToTransmit(linkId) && GetLink(linkId).access == NOT_REQUESTED)
    {
        m_mac->GetChannelAccessManager(linkId)->RequestAccess(this);
    }
}

void
Txop::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    for (std::size_t linkId = 0; linkId < m_links.size(); linkId++)
    {
        ResetCw(linkId);
        GenerateBackoff(linkId);
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
    NS_LOG_FUNCTION(this << +linkId);
    GetLink(linkId).access = REQUESTED;
}

void
Txop::NotifyChannelAccessed(uint8_t linkId, Time txopDuration)
{
    NS_LOG_FUNCTION(this << +linkId << txopDuration);
    GetLink(linkId).access = GRANTED;
}

void
Txop::NotifyChannelReleased(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +linkId);
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
    NS_LOG_FUNCTION(this << +linkId);
    if (GetLink(linkId).access == NOT_REQUESTED)
    {
        m_mac->GetChannelAccessManager(linkId)->RequestAccess(this);
    }
}

void
Txop::GenerateBackoff(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +linkId);
    uint32_t backoff = m_rng->GetInteger(0, GetCw(linkId));
    m_backoffTrace(backoff, linkId);
    StartBackoffNow(backoff, linkId);
}

void
Txop::NotifySleep(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +linkId);
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
    NS_LOG_FUNCTION(this << +linkId);
    StartAccessIfNeeded(linkId);
}

void
Txop::NotifyOn()
{
    NS_LOG_FUNCTION(this);
    for (std::size_t linkId = 0; linkId < m_links.size(); linkId++)
    {
        StartAccessIfNeeded(linkId);
    }
}

bool
Txop::IsQosTxop() const
{
    return false;
}

} // namespace ns3
