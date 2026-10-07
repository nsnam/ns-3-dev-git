/*
 * Copyright (c) 2025 Arnav Kapoor
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Arnav Kapoor <kapoorarnav43@gmail.com>
 */

#include "hwmp-control-recorder.h"

#include "ns3/callback.h"
#include "ns3/ie-dot11s-prep.h"
#include "ns3/ie-dot11s-preq.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

#include <algorithm>

NS_LOG_COMPONENT_DEFINE("HwmpControlRecorder");

namespace
{
/**
 * @param type the element type
 * @return the element name
 */
const char*
TypeName(HwmpControlRecorder::Type type)
{
    switch (type)
    {
    case HwmpControlRecorder::Type::PREQ:
        return "PREQ";
    case HwmpControlRecorder::Type::PREP:
        return "PREP";
    case HwmpControlRecorder::Type::PERR:
        return "PERR";
    }
    return "?";
}
} // namespace

void
HwmpControlRecorder::Connect(const NetDeviceContainer& meshPoints)
{
    for (uint32_t i = 0; i < meshPoints.GetN(); ++i)
    {
        auto hwmp = meshPoints.Get(i)->GetObject<dot11s::HwmpProtocol>();
        NS_ABORT_MSG_UNLESS(hwmp, "No HWMP protocol on mesh point " << i);
        m_addresses.push_back(Mac48Address::ConvertFrom(meshPoints.Get(i)->GetAddress()));
        hwmp->TraceConnectWithoutContext(
            "PreqReceived",
            MakeCallback(&HwmpControlRecorder::PreqReceived, this).Bind(i));
        hwmp->TraceConnectWithoutContext(
            "PrepReceived",
            MakeCallback(&HwmpControlRecorder::PrepReceived, this).Bind(i));
        hwmp->TraceConnectWithoutContext(
            "PerrReceived",
            MakeCallback(&HwmpControlRecorder::PerrReceived, this).Bind(i));
    }
}

Mac48Address
HwmpControlRecorder::GetAddress(uint32_t index) const
{
    return m_addresses.at(index);
}

const std::vector<HwmpControlRecorder::Event>&
HwmpControlRecorder::GetEvents() const
{
    return m_events;
}

std::size_t
HwmpControlRecorder::Count(const std::function<bool(const Event&)>& predicate) const
{
    return std::count_if(m_events.begin(), m_events.end(), predicate);
}

const HwmpControlRecorder::Event*
HwmpControlRecorder::FindFirst(const std::function<bool(const Event&)>& predicate) const
{
    auto it = std::find_if(m_events.begin(), m_events.end(), predicate);
    return it == m_events.end() ? nullptr : &(*it);
}

uint32_t
HwmpControlRecorder::GetIndex(Mac48Address address) const
{
    auto it = std::find(m_addresses.begin(), m_addresses.end(), address);
    return std::distance(m_addresses.begin(), it);
}

void
HwmpControlRecorder::PreqReceived(uint32_t receiver,
                                  const dot11s::IePreq& preq,
                                  Mac48Address from,
                                  uint32_t interface)
{
    Event event{Simulator::Now(),
                Type::PREQ,
                receiver,
                GetIndex(from),
                preq.GetOriginatorAddress(),
                Mac48Address::GetBroadcast(),
                false,
                false};
    auto targets = preq.GetDestinationList();
    if (!targets.empty())
    {
        event.target = targets.front()->GetDestinationAddress();
        event.doFlag = targets.front()->IsDo();
        event.rfFlag = targets.front()->IsRf();
    }
    NS_LOG_DEBUG(receiver << " <- " << event.transmitter << " " << TypeName(event.type) << " orig "
                          << event.originator << " target " << event.target << " DO "
                          << event.doFlag << " RF " << event.rfFlag);
    m_events.push_back(event);
}

void
HwmpControlRecorder::PrepReceived(uint32_t receiver,
                                  const dot11s::IePrep& prep,
                                  Mac48Address from,
                                  uint32_t interface)
{
    Event event{Simulator::Now(),
                Type::PREP,
                receiver,
                GetIndex(from),
                prep.GetOriginatorAddress(),
                prep.GetDestinationAddress(),
                false,
                false};
    NS_LOG_DEBUG(receiver << " <- " << event.transmitter << " " << TypeName(event.type) << " orig "
                          << event.originator << " dest " << event.target);
    m_events.push_back(event);
}

void
HwmpControlRecorder::PerrReceived(
    uint32_t receiver,
    const std::vector<dot11s::HwmpProtocol::FailedDestination>& destinations,
    Mac48Address from,
    uint32_t interface)
{
    Event event{Simulator::Now(),
                Type::PERR,
                receiver,
                GetIndex(from),
                Mac48Address(),
                destinations.empty() ? Mac48Address() : destinations.front().destination,
                false,
                false};
    NS_LOG_DEBUG(receiver << " <- " << event.transmitter << " " << TypeName(event.type) << " dest "
                          << event.target);
    m_events.push_back(event);
}
