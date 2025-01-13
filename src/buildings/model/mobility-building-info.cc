/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marco Miozzo  <marco.miozzo@cttc.es>
 *
 */

#include "mobility-building-info.h"

#include "building-list.h"

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/position-allocator.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MobilityBuildingInfo");

NS_OBJECT_ENSURE_REGISTERED(MobilityBuildingInfo);

TypeId
MobilityBuildingInfo::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MobilityBuildingInfo")
                            .SetParent<Object>()
                            .SetGroupName("Buildings")
                            .AddConstructor<MobilityBuildingInfo>();

    return tid;
}

void
MobilityBuildingInfo::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    Ptr<MobilityModel> mm = this->GetObject<MobilityModel>();
    MakeConsistent(mm);
}

MobilityBuildingInfo::MobilityBuildingInfo()
{
    NS_LOG_FUNCTION(this);
    m_indoor = false;
    m_nFloor = 1;
    m_roomX = 1;
    m_roomY = 1;
    m_cachedPosition = Vector(0, 0, 0);
}

MobilityBuildingInfo::MobilityBuildingInfo(Ptr<Building> building)
    : m_myBuilding(building)
{
    NS_LOG_FUNCTION(this);
    m_indoor = false;
    m_nFloor = 1;
    m_roomX = 1;
    m_roomY = 1;
}

bool
MobilityBuildingInfo::IsIndoor()
{
    NS_LOG_FUNCTION(this);
    Ptr<MobilityModel> mm = this->GetObject<MobilityModel>();
    Vector currentPosition = mm->GetPosition();
    bool posNotEqual = (currentPosition < m_cachedPosition) || (m_cachedPosition < currentPosition);
    if (posNotEqual)
    {
        MakeConsistent(mm);
    }

    return m_indoor;
}

void
MobilityBuildingInfo::SetIndoor(Ptr<Building> building,
                                uint8_t nfloor,
                                uint8_t nroomx,
                                uint8_t nroomy)
{
    NS_LOG_FUNCTION(this);
    m_indoor = true;
    m_myBuilding = building;
    m_nFloor = nfloor;
    m_roomX = nroomx;
    m_roomY = nroomy;

    NS_ASSERT(m_roomX > 0);
    NS_ASSERT(m_roomX <= building->GetNRoomsX());
    NS_ASSERT(m_roomY > 0);
    NS_ASSERT(m_roomY <= building->GetNRoomsY());
    NS_ASSERT(m_nFloor > 0);
    NS_ASSERT(m_nFloor <= building->GetNFloors());
}

void
MobilityBuildingInfo::SetIndoor(uint8_t nfloor, uint8_t nroomx, uint8_t nroomy)
{
    NS_LOG_FUNCTION(this);
    m_indoor = true;
    m_nFloor = nfloor;
    m_roomX = nroomx;
    m_roomY = nroomy;

    NS_ASSERT_MSG(m_myBuilding, "Node does not have any building defined");
    NS_ASSERT(m_roomX > 0);
    NS_ASSERT(m_roomX <= m_myBuilding->GetNRoomsX());
    NS_ASSERT(m_roomY > 0);
    NS_ASSERT(m_roomY <= m_myBuilding->GetNRoomsY());
    NS_ASSERT(m_nFloor > 0);
    NS_ASSERT(m_nFloor <= m_myBuilding->GetNFloors());
}

void
MobilityBuildingInfo::SetOutdoor()
{
    NS_LOG_FUNCTION(this);
    m_indoor = false;
}

uint8_t
MobilityBuildingInfo::GetFloorNumber()
{
    NS_LOG_FUNCTION(this);
    return m_nFloor;
}

uint8_t
MobilityBuildingInfo::GetRoomNumberX()
{
    NS_LOG_FUNCTION(this);
    return m_roomX;
}

uint8_t
MobilityBuildingInfo::GetRoomNumberY()
{
    NS_LOG_FUNCTION(this);
    return m_roomY;
}

Ptr<Building>
MobilityBuildingInfo::GetBuilding()
{
    NS_LOG_FUNCTION(this);
    return m_myBuilding;
}

void
MobilityBuildingInfo::MakeConsistent(Ptr<MobilityModel> mm)
{
    bool found = false;
    Vector pos = mm->GetPosition();
    for (auto bit = BuildingList::Begin(); bit != BuildingList::End(); ++bit)
    {
        NS_LOG_LOGIC("checking building " << (*bit)->GetId() << " with boundaries "
                                          << (*bit)->GetBoundaries());
        if ((*bit)->IsInside(pos))
        {
            NS_LOG_LOGIC("MobilityBuildingInfo " << this << " pos " << pos
                                                 << " falls inside building " << (*bit)->GetId());
            NS_ABORT_MSG_UNLESS(found == false,
                                " MobilityBuildingInfo already inside another building!");
            found = true;
            uint16_t floor = (*bit)->GetFloor(pos);
            uint16_t roomX = (*bit)->GetRoomX(pos);
            uint16_t roomY = (*bit)->GetRoomY(pos);
            SetIndoor(*bit, floor, roomX, roomY);
        }
    }
    if (!found)
    {
        NS_LOG_LOGIC("MobilityBuildingInfo " << this << " pos " << pos << " is outdoor");
        SetOutdoor();
    }

    m_cachedPosition = pos;
}

} // namespace ns3
