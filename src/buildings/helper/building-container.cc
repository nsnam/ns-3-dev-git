/*
 * Copyright (c) 2008 INRIA
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr> (original node-container.cc)
 *         Nicola Baldo (wrote building-container.cc based on node-container.cc)
 */
#include "building-container.h"

#include "ns3/building-list.h"
#include "ns3/names.h"

namespace ns3
{

BuildingContainer::BuildingContainer()
{
}

BuildingContainer::BuildingContainer(Ptr<Building> building)
{
    m_buildings.push_back(building);
}

BuildingContainer::BuildingContainer(std::string buildingName)
{
    Ptr<Building> building = Names::Find<Building>(buildingName);
    m_buildings.push_back(building);
}

BuildingContainer::Iterator
BuildingContainer::Begin() const
{
    return m_buildings.begin();
}

BuildingContainer::Iterator
BuildingContainer::End() const
{
    return m_buildings.end();
}

uint32_t
BuildingContainer::GetN() const
{
    return m_buildings.size();
}

Ptr<Building>
BuildingContainer::Get(uint32_t i) const
{
    return m_buildings[i];
}

void
BuildingContainer::Create(uint32_t n)
{
    for (uint32_t i = 0; i < n; i++)
    {
        m_buildings.push_back(CreateObject<Building>());
    }
}

void
BuildingContainer::Add(BuildingContainer other)
{
    for (auto i = other.Begin(); i != other.End(); i++)
    {
        m_buildings.push_back(*i);
    }
}

void
BuildingContainer::Add(Ptr<Building> building)
{
    m_buildings.push_back(building);
}

void
BuildingContainer::Add(std::string buildingName)
{
    Ptr<Building> building = Names::Find<Building>(buildingName);
    m_buildings.push_back(building);
}

BuildingContainer
BuildingContainer::GetGlobal()
{
    BuildingContainer c;
    for (auto i = BuildingList::Begin(); i != BuildingList::End(); ++i)
    {
        c.Add(*i);
    }
    return c;
}

} // namespace ns3
