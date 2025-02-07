/*
 * Copyright (c) 2023 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Ryo Okuda <c611901200@tokushima-u.ac.jp>
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include "zigbee-nwk-tables.h"

#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <iomanip>

namespace ns3
{
namespace zigbee
{

NS_LOG_COMPONENT_DEFINE("ZigbeeNwkTables");

/***********************************************************
 *                RREQ Retry Table Entry
 ***********************************************************/

RreqRetryTableEntry::RreqRetryTableEntry(uint8_t rreqId,
                                         EventId rreqRetryEvent,
                                         uint8_t rreqRetryCount)
{
    m_rreqId = rreqId;
    m_rreqRetryEventId = rreqRetryEvent;
    m_rreqRetryCount = rreqRetryCount;
}

uint8_t
RreqRetryTableEntry::GetRreqId() const
{
    return m_rreqId;
}

void
RreqRetryTableEntry::SetRreqRetryCount(uint8_t rreqRetryCount)
{
    m_rreqRetryCount = rreqRetryCount;
}

uint8_t
RreqRetryTableEntry::GetRreqRetryCount() const
{
    return m_rreqRetryCount;
}

void
RreqRetryTableEntry::SetRreqEventId(EventId eventId)
{
    m_rreqRetryEventId = eventId;
}

EventId
RreqRetryTableEntry::GetRreqEventId()
{
    return m_rreqRetryEventId;
}

void
RreqRetryTableEntry::Print(Ptr<OutputStreamWrapper> stream) const
{
    std::ostream* os = stream->GetStream();
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);
    *os << std::setw(9) << static_cast<uint32_t>(m_rreqId);
    *os << std::setw(12) << static_cast<uint32_t>(m_rreqRetryCount);
    *os << std::setw(9) << (m_rreqRetryEventId.IsPending() ? "TRUE" : "FALSE");
    *os << std::endl;
    (*os).copyfmt(oldState);
}

/***********************************************************
 *                RREQ Retry Table
 ***********************************************************/
bool
RreqRetryTable::AddEntry(Ptr<RreqRetryTableEntry> entry)
{
    m_rreqRetryTable.emplace_back(entry);
    return true;
}

bool
RreqRetryTable::LookUpEntry(uint8_t rreqId, Ptr<RreqRetryTableEntry>& entryFound)
{
    NS_LOG_FUNCTION(this << rreqId);

    for (const auto& entry : m_rreqRetryTable)
    {
        if (entry->GetRreqId() == rreqId)
        {
            entryFound = entry;
            return true;
        }
    }
    return false;
}

void
RreqRetryTable::Delete(uint8_t rreqId)
{
    std::erase_if(m_rreqRetryTable, [&rreqId](Ptr<RreqRetryTableEntry> entry) {
        return entry->GetRreqId() == rreqId;
    });
}

void
RreqRetryTable::Dispose()
{
    for (auto element : m_rreqRetryTable)
    {
        element = nullptr;
    }
    m_rreqRetryTable.clear();
}

void
RreqRetryTable::Print(Ptr<OutputStreamWrapper> stream) const
{
    std::ostream* os = stream->GetStream();
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);
    *os << "ZigBee RREQ retry table\n";
    *os << std::setw(9) << "RREQ ID";
    *os << std::setw(12) << "RREQ Count";
    *os << std::setw(9) << "Pending" << std::endl;

    for (const auto& entry : m_rreqRetryTable)
    {
        entry->Print(stream);
    }
    *stream->GetStream() << std::endl;
}

/***********************************************************
 *                Routing Table Entry
 ***********************************************************/

RoutingTableEntry::RoutingTableEntry(Mac16Address dst,
                                     RouteStatus status,
                                     bool noRouteCache,
                                     bool manyToOne,
                                     bool routeRecordReq,
                                     bool groupID,
                                     Mac16Address nextHopAddr)
{
    m_destination = dst;
    m_nextHopAddr = nextHopAddr;
    m_status = status;
    m_noRouteCache = noRouteCache;
    m_manyToOne = manyToOne;
    m_routeRecordReq = routeRecordReq;
    m_groupId = groupID;
}

RoutingTableEntry::RoutingTableEntry()
{
}

RoutingTableEntry::~RoutingTableEntry()
{
}

void
RoutingTableEntry::SetDestination(Mac16Address dst)
{
    m_destination = dst;
}

Mac16Address
RoutingTableEntry::GetDestination() const
{
    return m_destination;
}

void
RoutingTableEntry::SetStatus(RouteStatus status)
{
    m_status = status;
}

RouteStatus
RoutingTableEntry::GetStatus() const
{
    return m_status;
}

bool
RoutingTableEntry::IsNoRouteCache() const
{
    return m_noRouteCache;
}

bool
RoutingTableEntry::IsManyToOne() const
{
    return m_manyToOne;
}

bool
RoutingTableEntry::IsRouteRecordReq() const
{
    return m_routeRecordReq;
}

bool
RoutingTableEntry::IsGroupIdPresent() const
{
    return m_groupId;
}

void
RoutingTableEntry::SetNextHopAddr(Mac16Address nextHopAddr)
{
    m_nextHopAddr = nextHopAddr;
}

Mac16Address
RoutingTableEntry::GetNextHopAddr() const
{
    return m_nextHopAddr;
}

void
RoutingTableEntry::SetLifeTime(Time lt)
{
    m_lifeTime = lt + Simulator::Now();
}

Time
RoutingTableEntry::GetLifeTime() const
{
    return m_lifeTime - Simulator::Now();
}

void
RoutingTableEntry::Print(Ptr<OutputStreamWrapper> stream) const
{
    std::ostream* os = stream->GetStream();
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    std::ostringstream dst;
    std::ostringstream nextHop;

    dst << m_destination;
    nextHop << m_nextHopAddr;

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);
    *os << std::setw(16) << dst.str();
    *os << std::setw(10) << nextHop.str();

    switch (m_status)
    {
    case ROUTE_ACTIVE:
        *os << std::setw(21) << "ACTIVE";
        break;
    case ROUTE_DISCOVERY_UNDERWAY:
        *os << std::setw(21) << "DISCOVERY_UNDERWAY";
        break;
    case ROUTE_DISCOVER_FAILED:
        *os << std::setw(21) << "DISCOVERY_FAILED";
        break;
    case ROUTE_INACTIVE:
        *os << std::setw(21) << "INACTIVE";
        break;
    case ROUTE_VALIDATION_UNDERWAY:
        *os << std::setw(21) << "VALIDATION_UNDERWAY";
        break;
    }

    *os << std::setw(16) << (m_noRouteCache ? "TRUE" : "FALSE");
    *os << std::setw(16) << (m_manyToOne ? "TRUE" : "FALSE");
    *os << std::setw(16) << (m_routeRecordReq ? "TRUE" : "FALSE");
    *os << std::setw(16) << (m_groupId ? "TRUE" : "FALSE");
    *os << std::endl;
    (*os).copyfmt(oldState);
}

/***********************************************************
 *                Routing Table
 ***********************************************************/

RoutingTable::RoutingTable()
{
    m_maxTableSize = 32;
}

bool
RoutingTable::AddEntry(Ptr<RoutingTableEntry> rt)
{
    if (m_routingTable.size() < m_maxTableSize)
    {
        m_routingTable.emplace_back(rt);
        return true;
    }
    else
    {
        return false;
    }
}

void
RoutingTable::Purge()
{
    std::erase_if(m_routingTable, [](Ptr<RoutingTableEntry> entry) {
        return Simulator::Now() >= entry->GetLifeTime();
    });
}

void
RoutingTable::IdentifyExpiredEntries()
{
    for (const auto& entry : m_routingTable)
    {
        if (Simulator::Now() >= entry->GetLifeTime())
        {
            entry->SetStatus(ROUTE_INACTIVE);
        }
    }
}

void
RoutingTable::Delete(Mac16Address dst)
{
    std::erase_if(m_routingTable,
                  [&dst](Ptr<RoutingTableEntry> entry) { return entry->GetDestination() == dst; });
}

void
RoutingTable::DeleteExpiredEntry()
{
    auto it = std::find_if(
        m_routingTable.begin(),
        m_routingTable.end(),
        [](Ptr<RoutingTableEntry> entry) { return entry->GetStatus() == ROUTE_INACTIVE; });

    if (it != m_routingTable.end())
    {
        m_routingTable.erase(it);
    }
}

bool
RoutingTable::LookUpEntry(Mac16Address dstAddr, Ptr<RoutingTableEntry>& entryFound)
{
    NS_LOG_FUNCTION(this << dstAddr);

    IdentifyExpiredEntries();

    for (const auto& entry : m_routingTable)
    {
        if (entry->GetDestination() == dstAddr)
        {
            entryFound = entry;
            return true;
        }
    }
    return false;
}

void
RoutingTable::Print(Ptr<OutputStreamWrapper> stream) const
{
    std::ostream* os = stream->GetStream();
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);
    *os << "ZigBee Routing table\n";
    *os << std::setw(16) << "Destination";
    *os << std::setw(10) << "Next hop";
    *os << std::setw(21) << "Status";
    *os << std::setw(16) << "No route cache";
    *os << std::setw(16) << "Many-to-one";
    *os << std::setw(16) << "Route record";
    *os << std::setw(16) << "Group Id flag" << std::endl;

    for (const auto& entry : m_routingTable)
    {
        entry->Print(stream);
    }
    *stream->GetStream() << std::endl;
}

void
RoutingTable::Dispose()
{
    for (auto element : m_routingTable)
    {
        element = nullptr;
    }
    m_routingTable.clear();
}

uint32_t
RoutingTable::GetSize()
{
    return m_routingTable.size();
}

void
RoutingTable::SetMaxTableSize(uint32_t size)
{
    m_maxTableSize = size;
}

uint32_t
RoutingTable::GetMaxTableSize() const
{
    return m_maxTableSize;
}

/***********************************************************
 *                Routing Discovery Table Entry
 ***********************************************************/

RouteDiscoveryTableEntry::RouteDiscoveryTableEntry(uint8_t rreqId,
                                                   Mac16Address src,
                                                   Mac16Address snd,
                                                   uint8_t forwardCost,
                                                   uint8_t residualCost,
                                                   Time expTime)
{
    m_routeRequestId = rreqId;
    m_sourceAddr = src;
    m_senderAddr = snd;
    m_forwardCost = forwardCost;
    m_residualCost = residualCost;
    m_expirationTime = expTime;
}

RouteDiscoveryTableEntry::RouteDiscoveryTableEntry()
{
}

RouteDiscoveryTableEntry::~RouteDiscoveryTableEntry()
{
}

uint8_t
RouteDiscoveryTableEntry::GetRreqId() const
{
    return m_routeRequestId;
}

Mac16Address
RouteDiscoveryTableEntry::GetSourceAddr() const
{
    return m_sourceAddr;
}

Mac16Address
RouteDiscoveryTableEntry::GetSenderAddr() const
{
    return m_senderAddr;
}

uint8_t
RouteDiscoveryTableEntry::GetForwardCost() const
{
    return m_forwardCost;
}

uint8_t
RouteDiscoveryTableEntry::GetResidualCost() const
{
    return m_residualCost;
}

void
RouteDiscoveryTableEntry::SetForwardCost(uint8_t pathCost)
{
    m_forwardCost = pathCost;
}

void
RouteDiscoveryTableEntry::SetSenderAddr(Mac16Address sender)
{
    m_senderAddr = sender;
}

void
RouteDiscoveryTableEntry::SetResidualCost(uint8_t pathcost)
{
    m_residualCost = pathcost;
}

Time
RouteDiscoveryTableEntry::GetExpTime() const
{
    return m_expirationTime;
}

void
RouteDiscoveryTableEntry::SetExpTime(Time exp)
{
    m_expirationTime = exp;
}

void
RouteDiscoveryTableEntry::Print(Ptr<OutputStreamWrapper> stream) const
{
    std::ostream* os = stream->GetStream();
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    std::ostringstream sourceAddr;
    std::ostringstream senderAddr;
    std::ostringstream expTime;

    sourceAddr << m_sourceAddr;
    senderAddr << m_senderAddr;
    expTime << m_expirationTime.As(Time::S);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);
    *os << std::setw(10) << static_cast<uint32_t>(m_routeRequestId);
    *os << std::setw(16) << sourceAddr.str();
    *os << std::setw(16) << senderAddr.str();
    *os << std::setw(16) << static_cast<uint32_t>(m_forwardCost);
    *os << std::setw(16) << static_cast<uint32_t>(m_residualCost);
    *os << std::setw(16) << expTime.str();
    *os << std::endl;

    (*os).copyfmt(oldState);
}

/***********************************************************
 *                Route Discovery Table
 ***********************************************************/

RouteDiscoveryTable::RouteDiscoveryTable()
{
    m_maxTableSize = 32;
}

bool
RouteDiscoveryTable::AddEntry(Ptr<RouteDiscoveryTableEntry> rt)
{
    Purge();

    if (m_routeDscTable.size() < m_maxTableSize)
    {
        m_routeDscTable.emplace_back(rt);
        return true;
    }
    else
    {
        return false;
    }
}

bool
RouteDiscoveryTable::LookUpEntry(uint8_t id,
                                 Mac16Address src,
                                 Ptr<RouteDiscoveryTableEntry>& entryFound)
{
    NS_LOG_FUNCTION(this << id);
    Purge();
    for (const auto& entry : m_routeDscTable)
    {
        if (entry->GetRreqId() == id && entry->GetSourceAddr() == src)
        {
            entryFound = entry;
            return true;
        }
    }
    return false;
}

void
RouteDiscoveryTable::Purge()
{
    std::erase_if(m_routeDscTable, [](Ptr<RouteDiscoveryTableEntry> entry) {
        return entry->GetExpTime() < Simulator::Now();
    });
}

void
RouteDiscoveryTable::Delete(uint8_t id, Mac16Address src)
{
    std::erase_if(m_routeDscTable, [&id, &src](Ptr<RouteDiscoveryTableEntry> entry) {
        return (entry->GetRreqId() == id && entry->GetSourceAddr() == src);
    });
}

void
RouteDiscoveryTable::Print(Ptr<OutputStreamWrapper> stream)
{
    Purge();

    std::ostream* os = stream->GetStream();
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);
    *os << "ZigBee Route Discovery table\n";
    *os << std::setw(10) << "RREQ ID";
    *os << std::setw(16) << "Source Address";
    *os << std::setw(16) << "Sender Address";
    *os << std::setw(16) << "Forward Cost";
    *os << std::setw(16) << "Residual Cost";
    *os << "Expiration time" << std::endl;

    for (const auto& entry : m_routeDscTable)
    {
        entry->Print(stream);
    }
    *stream->GetStream() << std::endl;
}

void
RouteDiscoveryTable::Dispose()
{
    for (auto element : m_routeDscTable)
    {
        element = nullptr;
    }
    m_routeDscTable.clear();
}

/***********************************************************
 *                Neighbor Table Entry
 ***********************************************************/

NeighborTableEntry::NeighborTableEntry(Mac64Address extAddr,
                                       Mac16Address nwkAddr,
                                       NwkDeviceType deviceType,
                                       bool rxOnWhenIdle,
                                       uint16_t endDevConfig,
                                       Time timeoutCounter,
                                       Time devTimeout,
                                       Relationship relationship,
                                       uint8_t txFailure,
                                       uint8_t lqi,
                                       uint8_t outgoingCost,
                                       uint8_t age,
                                       bool keepaliveRx,
                                       uint8_t macInterfaceIndex)
{
    m_extAddr = extAddr;
    m_nwkAddr = nwkAddr;
    m_deviceType = deviceType;
    m_rxOnWhenIdle = rxOnWhenIdle;
    m_endDevConfig = endDevConfig;
    m_timeoutCounter = timeoutCounter;
    m_devTimeout = devTimeout;
    m_relationship = relationship;
    m_txFailure = txFailure;
    m_lqi = lqi;
    m_outgoingCost = outgoingCost;
    m_age = age;
    m_keepaliveRx = keepaliveRx;
    m_macInterfaceIndex = macInterfaceIndex;
}

NeighborTableEntry::NeighborTableEntry()
{
}

NeighborTableEntry::~NeighborTableEntry()
{
}

Mac64Address
NeighborTableEntry::GetExtAddr() const
{
    return m_extAddr;
}

Mac16Address
NeighborTableEntry::GetNwkAddr() const
{
    return m_nwkAddr;
}

NwkDeviceType
NeighborTableEntry::GetDeviceType() const
{
    return m_deviceType;
}

bool
NeighborTableEntry::IsRxOnWhenIdle() const
{
    return m_rxOnWhenIdle;
}

uint16_t
NeighborTableEntry::GetEndDevConfig() const
{
    return m_endDevConfig;
}

Time
NeighborTableEntry::GetTimeoutCounter() const
{
    return m_timeoutCounter;
}

Time
NeighborTableEntry::GetDevTimeout() const
{
    return m_devTimeout - Simulator::Now();
}

uint8_t
NeighborTableEntry::GetRelationship() const
{
    return m_relationship;
}

uint8_t
NeighborTableEntry::GetTxFailure() const
{
    return m_txFailure;
}

uint8_t
NeighborTableEntry::GetLqi() const
{
    return m_lqi;
}

uint8_t
NeighborTableEntry::GetOutgoingCost() const
{
    return m_outgoingCost;
}

uint8_t
NeighborTableEntry::GetAge() const
{
    return m_age;
}

uint64_t
NeighborTableEntry::GetIncBeaconTimestamp() const
{
    return m_incBeaconTimestamp;
}

uint64_t
NeighborTableEntry::GetBeaconTxTimeOffset() const
{
    return m_beaconTxTimeOffset;
}

uint8_t
NeighborTableEntry::GetMacInterfaceIndex() const
{
    return m_macInterfaceIndex;
}

uint32_t
NeighborTableEntry::GetMacUcstBytesTx() const
{
    return m_macUcstBytesTx;
}

uint32_t
NeighborTableEntry::GetMacUcstBytesRx() const
{
    return m_macUcstBytesRx;
}

uint64_t
NeighborTableEntry::GetExtPanId() const
{
    return m_extPanId;
}

uint8_t
NeighborTableEntry::GetLogicalCh() const
{
    return m_logicalCh;
}

uint8_t
NeighborTableEntry::GetDepth() const
{
    return m_depth;
}

uint8_t
NeighborTableEntry::GetBeaconOrder() const
{
    return m_bo;
}

uint8_t
NeighborTableEntry::IsPotentialParent() const
{
    return m_potentialParent;
}

void
NeighborTableEntry::SetNwkAddr(Mac16Address nwkAddr)
{
    m_nwkAddr = nwkAddr;
}

void
NeighborTableEntry::SetDeviceType(NwkDeviceType devType)
{
    m_deviceType = devType;
}

void
NeighborTableEntry::SetRxOnWhenIdle(bool onWhenIdle)
{
    m_rxOnWhenIdle = onWhenIdle;
}

void
NeighborTableEntry::SetEndDevConfig(uint16_t conf)
{
    m_endDevConfig = conf;
}

void
NeighborTableEntry::SetTimeoutCounter(Time counter)
{
    m_timeoutCounter = counter;
}

void
NeighborTableEntry::SetDevTimeout(Time timeout)
{
    m_devTimeout = timeout;
}

void
NeighborTableEntry::SetRelationship(Relationship relationship)
{
    m_relationship = relationship;
}

void
NeighborTableEntry::SetTxFailure(uint8_t failure)
{
    m_txFailure = failure;
}

void
NeighborTableEntry::SetLqi(uint8_t lqi)
{
    m_lqi = lqi;
}

void
NeighborTableEntry::SetOutgoingCost(uint8_t cost)
{
    m_outgoingCost = cost;
}

void
NeighborTableEntry::SetAge(uint8_t age)
{
    m_age = age;
}

void
NeighborTableEntry::SetIncBeaconTimestamp(uint64_t timestamp)
{
    m_incBeaconTimestamp = timestamp;
}

void
NeighborTableEntry::SetBeaconTxTimeOffset(uint64_t offset)
{
    m_beaconTxTimeOffset = offset;
}

void
NeighborTableEntry::SetMacUcstBytesTx(uint32_t txBytes)
{
    m_macUcstBytesTx = txBytes;
}

void
NeighborTableEntry::SetMacUcstBytesRx(uint32_t rxBytes)
{
    m_macUcstBytesRx = rxBytes;
}

void
NeighborTableEntry::SetExtPanId(uint64_t extPanId)
{
    m_extPanId = extPanId;
}

void
NeighborTableEntry::SetLogicalCh(uint8_t channel)
{
    m_logicalCh = channel;
}

void
NeighborTableEntry::SetDepth(uint8_t depth)
{
    m_depth = depth;
}

void
NeighborTableEntry::SetBeaconOrder(uint8_t bo)
{
    m_bo = bo;
}

void
NeighborTableEntry::SetPotentialParent(bool confirm)
{
    m_potentialParent = confirm;
}

void
NeighborTableEntry::Print(Ptr<OutputStreamWrapper> stream) const
{
    std::ostream* os = stream->GetStream();
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    std::ostringstream extAddr;
    std::ostringstream nwkAddr;
    std::ostringstream devTimeout;

    extAddr << m_extAddr;
    nwkAddr << m_nwkAddr;
    devTimeout << m_devTimeout;

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);
    *os << std::setw(25) << extAddr.str();
    *os << std::setw(13) << nwkAddr.str();
    *os << std::setw(16) << devTimeout.str();
    switch (m_relationship)
    {
    case NBR_PARENT:
        *os << std::setw(16) << "PARENT";
        break;
    case NBR_CHILD:
        *os << std::setw(16) << "CHILD";
        break;
    case NBR_SIBLING:
        *os << std::setw(16) << "SIBLING";
        break;
    case NBR_NONE:
        *os << std::setw(16) << "NONE";
        break;
    case NBR_PREV_CHILD:
        *os << std::setw(16) << "PREVIOUS CHILD";
        break;
    case NBR_UNAUTH_CHILD:
        *os << std::setw(16) << "UNAUTH CHILD";
        break;
    }

    switch (m_deviceType)
    {
    case ZIGBEE_COORDINATOR:
        *os << std::setw(16) << "COORDINATOR";
        break;
    case ZIGBEE_ROUTER:
        *os << std::setw(16) << "ROUTER";
        break;
    case ZIGBEE_ENDDEVICE:
        *os << std::setw(16) << "END DEVICE";
        break;
    }

    *os << std::setw(14) << static_cast<uint16_t>(m_txFailure);
    *os << std::setw(5) << static_cast<uint16_t>(m_lqi);
    *os << std::setw(16) << static_cast<uint16_t>(m_outgoingCost);
    *os << std::setw(8) << static_cast<uint16_t>(m_age);
    *os << std::setw(19) << std::hex << m_extPanId << std::dec;
    *os << std::setw(11) << (m_potentialParent ? "TRUE" : "FALSE");
    *os << std::endl;
    (*os).copyfmt(oldState);
}

/***********************************************************
 *                Neighbor Table
 ***********************************************************/

NeighborTable::NeighborTable()
{
    m_maxTableSize = 32;
}

bool
NeighborTable::AddEntry(Ptr<NeighborTableEntry> entry)
{
    if (m_neighborTable.size() < m_maxTableSize)
    {
        m_neighborTable.emplace_back(entry);
        return true;
    }
    else
    {
        return false;
    }
}

void
NeighborTable::Purge()
{
    std::erase_if(m_neighborTable, [](Ptr<NeighborTableEntry> entry) {
        return Simulator::Now() >= entry->GetTimeoutCounter();
    });
}

void
NeighborTable::Delete(Mac64Address extAddr)
{
    std::erase_if(m_neighborTable, [&extAddr](Ptr<NeighborTableEntry> entry) {
        return entry->GetExtAddr() == extAddr;
    });
}

bool
NeighborTable::LookUpEntry(Mac16Address nwkAddr, Ptr<NeighborTableEntry>& entryFound)
{
    NS_LOG_FUNCTION(this << nwkAddr);
    // Purge();

    for (const auto& entry : m_neighborTable)
    {
        if (entry->GetNwkAddr() == nwkAddr)
        {
            entryFound = entry;
            return true;
        }
    }

    return false;
}

bool
NeighborTable::LookUpEntry(Mac64Address extAddr, Ptr<NeighborTableEntry>& entryFound)
{
    NS_LOG_FUNCTION(this << extAddr);
    // Purge();

    for (const auto& entry : m_neighborTable)
    {
        if (entry->GetExtAddr() == extAddr)
        {
            entryFound = entry;
            return true;
        }
    }

    return false;
}

bool
NeighborTable::GetParent(Ptr<NeighborTableEntry>& entryFound)
{
    NS_LOG_FUNCTION(this);

    for (const auto& entry : m_neighborTable)
    {
        if (entry->GetRelationship() == NBR_PARENT)
        {
            entryFound = entry;
            return true;
        }
    }

    return false;
}

bool
NeighborTable::LookUpForBestParent(uint64_t epid, Ptr<NeighborTableEntry>& entryFound)
{
    bool flag = false;
    uint8_t currentLinkCost = 7;
    uint8_t prevLinkCost = 8;

    for (const auto& entry : m_neighborTable)
    {
        // Note: Permit to join, stack profile , update id and capability are checked when
        //       the beacon is received (beacon notify indication)
        currentLinkCost = GetLinkCost(entry->GetLqi());

        if ((epid == entry->GetExtPanId()) &&
            (entry->GetDeviceType() == ZIGBEE_COORDINATOR ||
             entry->GetDeviceType() == ZIGBEE_ROUTER) &&
            (entry->IsPotentialParent()) && (currentLinkCost <= 3))
        {
            if (currentLinkCost < prevLinkCost)
            {
                entryFound = entry;
                prevLinkCost = currentLinkCost;
            }
            entryFound = entry;
            flag = true;
        }
    }

    return flag;
}

void
NeighborTable::Print(Ptr<OutputStreamWrapper> stream) const
{
    std::ostream* os = stream->GetStream();
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);
    *os << "ZigBee Neighbor Table\n";
    *os << std::setw(25) << "IEEE Address";
    *os << std::setw(13) << "Nwk Address";
    *os << std::setw(16) << "Device Timeout";
    *os << std::setw(16) << "Relationship";
    *os << std::setw(16) << "Device type";
    *os << std::setw(14) << "Tx Failure";
    *os << std::setw(5) << "LQI";
    *os << std::setw(16) << "Outgoing Cost";
    *os << std::setw(8) << "Age";
    *os << std::setw(19) << "Ext PAN ID";
    *os << std::setw(11) << "Pot. Parent";
    *os << std::endl;

    for (const auto& entry : m_neighborTable)
    {
        entry->Print(stream);
    }
    *stream->GetStream() << std::endl;
}

uint32_t
NeighborTable::GetSize()
{
    return m_neighborTable.size();
}

void
NeighborTable::SetMaxTableSize(uint32_t size)
{
    m_maxTableSize = size;
}

uint32_t
NeighborTable::GetMaxTableSize() const
{
    return m_maxTableSize;
}

uint8_t
NeighborTable::GetLinkCost(uint8_t lqi) const
{
    NS_ASSERT_MSG(lqi <= 255, "LQI does not have a valid range");

    uint8_t linkCost;

    if (lqi >= 240)
    {
        linkCost = 1;
    }
    else if (lqi >= 202)
    {
        linkCost = 2;
    }
    else if (lqi >= 154)
    {
        linkCost = 3;
    }
    else if (lqi >= 106)
    {
        linkCost = 4;
    }
    else if (lqi >= 58)
    {
        linkCost = 5;
    }
    else if (lqi >= 11)
    {
        linkCost = 6;
    }
    else
    {
        linkCost = 7;
    }

    return linkCost;
}

void
NeighborTable::Dispose()
{
    for (auto element : m_neighborTable)
    {
        element = nullptr;
    }
    m_neighborTable.clear();
}

/***********************************************************
 *                    PAN Id Table
 ***********************************************************/

PanIdTable::PanIdTable()
{
}

void
PanIdTable::AddEntry(uint64_t extPanId, uint16_t panId)
{
    auto i = m_panIdTable.find(extPanId);
    if (i == m_panIdTable.end())
    {
        // New entry
        m_panIdTable.emplace(extPanId, panId);

        NS_LOG_DEBUG("[New entry, Pan ID Table]"
                     " | ExtPANId: "
                     << extPanId << " | PAN Id: " << panId);
    }
    else
    {
        // Update entry
        if (panId != i->second)
        {
            m_panIdTable[extPanId] = panId;
        }
    }
}

bool
PanIdTable::GetEntry(uint64_t extPanId, uint16_t& panId)
{
    auto i = m_panIdTable.find(extPanId);

    if (i == m_panIdTable.end())
    {
        return false;
    }

    panId = i->second;
    return true;
}

void
PanIdTable::Dispose()
{
    m_panIdTable.clear();
}

/***********************************************************
 *                Broadcast Transaction Table (BTT)
 ***********************************************************/

BroadcastTransactionTable::BroadcastTransactionTable()
{
}

bool
BroadcastTransactionTable::AddEntry(Ptr<BroadcastTransactionRecord> entry)
{
    Purge();
    m_broadcastTransactionTable.emplace_back(entry);
    return true;
}

uint32_t
BroadcastTransactionTable::GetSize()
{
    return m_broadcastTransactionTable.size();
}

void
BroadcastTransactionTable::SetMaxTableSize(uint32_t size)
{
    m_maxTableSize = size;
}

uint32_t
BroadcastTransactionTable::GetMaxTableSize() const
{
    return m_maxTableSize;
}

bool
BroadcastTransactionTable::LookUpEntry(uint8_t seq, Ptr<BroadcastTransactionRecord>& entryFound)
{
    NS_LOG_FUNCTION(this << seq);
    Purge();

    for (const auto& entry : m_broadcastTransactionTable)
    {
        if (entry->GetSeqNum() == seq)
        {
            entryFound = entry;
            return true;
        }
    }
    return false;
}

void
BroadcastTransactionTable::Purge()
{
    std::erase_if(m_broadcastTransactionTable, [](Ptr<BroadcastTransactionRecord> btr) {
        return Simulator::Now() >= btr->GetExpirationTime();
    });
}

void
BroadcastTransactionTable::Dispose()
{
    for (auto element : m_broadcastTransactionTable)
    {
        element = nullptr;
    }
    m_broadcastTransactionTable.clear();
}

void
BroadcastTransactionTable::Print(Ptr<OutputStreamWrapper> stream)
{
    std::ostream* os = stream->GetStream();
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);
    *os << "ZigBee Routing table\n";
    *os << std::setw(16) << "SrcAddress";
    *os << std::setw(16) << "Seq. Num";
    *os << std::setw(16) << "Expiration";
    *os << std::setw(16) << "Count" << std::endl;

    for (const auto& entry : m_broadcastTransactionTable)
    {
        entry->Print(stream);
    }
    *stream->GetStream() << std::endl;
}

/***********************************************************
 *                Broadcast Transaction Record (BTR)
 ***********************************************************/

BroadcastTransactionRecord::BroadcastTransactionRecord(Mac16Address srcAddr, uint8_t seq, Time exp)
{
    m_srcAddr = srcAddr;
    m_sequenceNumber = seq;
    m_expirationTime = exp;
    m_broadcastRetryCount = 0;
}

BroadcastTransactionRecord::BroadcastTransactionRecord()
{
}

BroadcastTransactionRecord::~BroadcastTransactionRecord()
{
}

Mac16Address
BroadcastTransactionRecord::GetSrcAddr() const
{
    return m_srcAddr;
}

void
BroadcastTransactionRecord::SetSrcAddr(Mac16Address src)
{
    m_srcAddr = src;
}

uint8_t
BroadcastTransactionRecord::GetSeqNum() const
{
    return m_sequenceNumber;
}

void
BroadcastTransactionRecord::SetSeqNum(uint8_t seq)
{
    m_sequenceNumber = seq;
}

Time
BroadcastTransactionRecord::GetExpirationTime() const
{
    return m_expirationTime;
}

void
BroadcastTransactionRecord::SetExpirationTime(Time exp)
{
    m_expirationTime = exp;
}

uint8_t
BroadcastTransactionRecord::GetBcstRetryCount() const
{
    return m_broadcastRetryCount;
}

void
BroadcastTransactionRecord::SetBcstRetryCount(uint8_t count)
{
    m_broadcastRetryCount = count;
}

void
BroadcastTransactionRecord::Print(Ptr<OutputStreamWrapper> stream) const
{
    std::ostream* os = stream->GetStream();
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    std::ostringstream sourceAddr;
    std::ostringstream seq;
    std::ostringstream expTime;
    std::ostringstream count;

    sourceAddr << m_srcAddr;
    seq << m_sequenceNumber;
    expTime << (m_expirationTime - Simulator::Now()).As(Time::S);
    count << m_broadcastRetryCount;

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);
    *os << std::setw(16) << sourceAddr.str();
    *os << std::setw(16) << static_cast<uint32_t>(m_sequenceNumber);
    *os << std::setw(16) << expTime.str();
    *os << std::setw(16) << static_cast<uint32_t>(m_broadcastRetryCount);
    *os << std::endl;
    (*os).copyfmt(oldState);
}

} // namespace zigbee
} // namespace ns3
