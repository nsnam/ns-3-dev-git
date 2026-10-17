/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Boh Jie Qi <jieqiboh5836@gmail.com>
 */

#include "sixlowpan-nd-binding-table.h"

#include "sixlowpan-nd-protocol.h"

#include "ns3/ipv6-address.h"
#include "ns3/ipv6-interface.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/ipv6-routing-protocol.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

#include <algorithm>
#include <iomanip>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SixLowPanNdBindingTable");

NS_OBJECT_ENSURE_REGISTERED(SixLowPanNdBindingTable);

/// Duration before an entry transitions to STALE state, in hours (RFC 8929).
static const uint8_t STALE_DURATION = 24; // hours

TypeId
SixLowPanNdBindingTable::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SixLowPanNdBindingTable")
            .SetParent<Object>()
            .SetGroupName("SixLowPan")
            .AddConstructor<SixLowPanNdBindingTable>()
            .AddAttribute("StaleDuration",
                          "The duration (in hours) an entry remains in STALE state before "
                          "being removed from the binding table.",
                          TimeValue(Hours(STALE_DURATION)),
                          MakeTimeAccessor(&SixLowPanNdBindingTable::m_staleDuration),
                          MakeTimeChecker(Time(0), Hours(0xffff)));
    return tid;
}

SixLowPanNdBindingTable::SixLowPanNdBindingTable()
    : m_staleDuration(Hours(STALE_DURATION)) // Default 24 hours
{
    NS_LOG_FUNCTION(this);
}

SixLowPanNdBindingTable::~SixLowPanNdBindingTable()
{
    NS_LOG_FUNCTION(this);
    DoDispose();
}

void
SixLowPanNdBindingTable::DoDispose()
{
    NS_LOG_FUNCTION(this);

    // Clean up all entries
    for (auto& entry : m_sixLowPanNdBindingTable)
    {
        delete entry.second;
    }
    m_sixLowPanNdBindingTable.clear();

    m_device = nullptr;
    m_interface = nullptr;
    m_icmpv6 = nullptr;

    Object::DoDispose();
}

Ptr<NetDevice>
SixLowPanNdBindingTable::GetDevice() const
{
    NS_LOG_FUNCTION(this);
    return m_device;
}

void
SixLowPanNdBindingTable::SetDevice(Ptr<NetDevice> device,
                                   Ptr<Ipv6Interface> interface,
                                   Ptr<Icmpv6L4Protocol> icmpv6)
{
    NS_LOG_FUNCTION(this << device << interface << icmpv6);
    m_interface = interface;
    m_icmpv6 = icmpv6;

    if (device == nullptr)
    {
        NS_LOG_ERROR("Device is null, cannot set device for SixLowPanNdBindingTable");
        return;
    }
    m_device = device;
}

Ptr<Ipv6Interface>
SixLowPanNdBindingTable::GetInterface() const
{
    return m_interface;
}

SixLowPanNdBindingTable::SixLowPanNdBindingTableEntry*
SixLowPanNdBindingTable::Lookup(Ipv6Address dst)
{
    NS_LOG_FUNCTION(this << dst);

    auto it = m_sixLowPanNdBindingTable.find(dst);
    return it != m_sixLowPanNdBindingTable.end() ? it->second : nullptr;
}

SixLowPanNdBindingTable::SixLowPanNdBindingTableEntry*
SixLowPanNdBindingTable::Add(Ipv6Address to)
{
    NS_LOG_FUNCTION(this << to);

    auto [it, inserted] = m_sixLowPanNdBindingTable.emplace(to, nullptr);
    if (!inserted)
    {
        NS_LOG_DEBUG("Entry for " << to << " already exists");
        return it->second;
    }

    auto* entry = new SixLowPanNdBindingTable::SixLowPanNdBindingTableEntry(this);
    it->second = entry;
    entry->SetIpv6Address(to);

    NS_LOG_DEBUG("Added new binding table entry for " << to);
    return entry;
}

void
SixLowPanNdBindingTable::Remove(SixLowPanNdBindingTable::SixLowPanNdBindingTableEntry* entry)
{
    NS_LOG_FUNCTION(this << entry);

    auto it = m_sixLowPanNdBindingTable.find(entry->GetIpv6Address());
    if (it == m_sixLowPanNdBindingTable.end())
    {
        NS_LOG_WARN("Entry not found in binding table");
        return;
    }
    NS_LOG_DEBUG("Removing binding table entry for " << it->first);
    delete it->second;
    m_sixLowPanNdBindingTable.erase(it);
}

void
SixLowPanNdBindingTable::PrintBindingTable(Ptr<OutputStreamWrapper> stream)
{
    NS_LOG_FUNCTION(this << stream);

    std::ostream* os = stream->GetStream();

    // Collect entries in a vector for sorting
    std::vector<std::pair<Ipv6Address, SixLowPanNdBindingTableEntry*>> entries;
    for (const auto& i : m_sixLowPanNdBindingTable)
    {
        entries.emplace_back(i.first, i.second);
    }

    // Sort by IPv6 address
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    // Print sorted entries
    for (const auto& entry : entries)
    {
        entry.second->Print(*os);
        *os << std::endl;
    }
}

// SixLowPanNdBindingTableEntry Implementation

SixLowPanNdBindingTable::SixLowPanNdBindingTableEntry::SixLowPanNdBindingTableEntry(
    SixLowPanNdBindingTable* bt)
    : m_rovr(16, 0),
      m_type(STALE), // Default state; MarkReachable() must be called after Add()
      m_reachableTimer(Timer::CANCEL_ON_DESTROY),
      m_staleTimer(Timer::CANCEL_ON_DESTROY),
      m_bindingTable(bt)
{
    NS_LOG_FUNCTION(this << bt);
}

void
SixLowPanNdBindingTable::SixLowPanNdBindingTableEntry::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);

    os << m_ipv6Address;

    switch (m_type)
    {
    case REACHABLE:
        os << " REACHABLE";
        break;
    case STALE:
        os << " STALE";
        break;
    }

    os << std::dec;
}

void
SixLowPanNdBindingTable::SixLowPanNdBindingTableEntry::MarkReachable(uint16_t time)
{
    NS_LOG_FUNCTION(this << time);

    m_type = REACHABLE;

    // Cancel existing timers
    if (m_reachableTimer.IsRunning())
    {
        m_reachableTimer.Cancel();
    }

    if (m_staleTimer.IsRunning())
    {
        m_staleTimer.Cancel();
    }

    if (time > 0)
    {
        // Start reachable timer (time is in units of 60 seconds from ARO)
        m_reachableTimer.SetFunction(
            &SixLowPanNdBindingTable::SixLowPanNdBindingTableEntry::FunctionTimeout,
            this);
        m_reachableTimer.SetDelay(Minutes(time));
        m_reachableTimer.Schedule();

        NS_LOG_DEBUG("Entry marked REACHABLE with lifetime " << time << " minutes");
    }
}

void
SixLowPanNdBindingTable::SixLowPanNdBindingTableEntry::MarkStale()
{
    NS_LOG_FUNCTION(this);

    m_type = STALE;

    // Cancel any running timers
    if (m_reachableTimer.IsRunning())
    {
        m_reachableTimer.Cancel();
    }

    if (m_staleTimer.IsRunning())
    {
        m_staleTimer.Cancel();
    }

    // Use the binding table's configurable stale duration
    m_staleTimer.SetFunction(
        &SixLowPanNdBindingTable::SixLowPanNdBindingTableEntry::FunctionTimeout,
        this);
    m_staleTimer.SetDelay(m_bindingTable->m_staleDuration); // Use configurable value
    m_staleTimer.Schedule();

    NS_LOG_DEBUG("Entry marked STALE with duration " << m_bindingTable->m_staleDuration.GetSeconds()
                                                     << " seconds");
}

bool
SixLowPanNdBindingTable::SixLowPanNdBindingTableEntry::IsReachable() const
{
    NS_LOG_FUNCTION(this);
    return (m_type == REACHABLE);
}

bool
SixLowPanNdBindingTable::SixLowPanNdBindingTableEntry::IsStale() const
{
    NS_LOG_FUNCTION(this);
    return (m_type == STALE);
}

void
SixLowPanNdBindingTable::SixLowPanNdBindingTableEntry::FunctionTimeout()
{
    NS_LOG_FUNCTION(this);

    if (m_type == REACHABLE)
    {
        NS_LOG_DEBUG("Registered timer expired, marking entry as STALE");
        MarkStale();
    }
    else if (m_type == STALE)
    {
        NS_LOG_DEBUG("STALE timer expired, removing entry");

        Ptr<NetDevice> device = m_bindingTable->GetDevice();
        Ptr<Node> node = device ? device->GetNode() : nullptr;
        Ptr<Ipv6L3Protocol> ipv6l3Protocol = node ? node->GetObject<Ipv6L3Protocol>() : nullptr;

        if (ipv6l3Protocol)
        {
            ipv6l3Protocol->GetRoutingProtocol()->NotifyRemoveRoute(
                GetIpv6Address(),
                Ipv6Prefix(128),
                Ipv6Address::GetAny(),
                ipv6l3Protocol->GetInterfaceForDevice(device));
        }

        m_bindingTable->Remove(this);
    }
}

void
SixLowPanNdBindingTable::SixLowPanNdBindingTableEntry::SetIpv6Address(Ipv6Address ipv6Address)
{
    NS_LOG_FUNCTION(this << ipv6Address);
    m_ipv6Address = ipv6Address;
}

std::vector<uint8_t>
SixLowPanNdBindingTable::SixLowPanNdBindingTableEntry::GetRovr() const
{
    NS_LOG_FUNCTION(this);
    return m_rovr;
}

void
SixLowPanNdBindingTable::SixLowPanNdBindingTableEntry::SetRovr(const std::vector<uint8_t>& rovr)
{
    NS_LOG_FUNCTION(this << &rovr);
    // We expect ROVR to be 16 bytes. If caller provides more, truncate; if less, pad with zeros.
    if (rovr.size() >= 16)
    {
        m_rovr.assign(rovr.begin(), rovr.begin() + 16);
    }
    else
    {
        m_rovr = rovr;
        m_rovr.resize(16, 0);
    }
}

Ipv6Address
SixLowPanNdBindingTable::SixLowPanNdBindingTableEntry::GetIpv6Address() const
{
    NS_LOG_FUNCTION(this);
    return m_ipv6Address;
}

SixLowPanNdBindingTable*
SixLowPanNdBindingTable::SixLowPanNdBindingTableEntry::GetBindingTable() const
{
    NS_LOG_FUNCTION(this);
    return m_bindingTable;
}

// Stream operator implementation
std::ostream&
operator<<(std::ostream& os, const SixLowPanNdBindingTable::SixLowPanNdBindingTableEntry& entry)
{
    entry.Print(os);
    return os;
}

} // namespace ns3
