/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Jaume Nin <jaume.nin@cttc,cat>
 * Based on BuildingList implementation by Mathieu Lacage  <mathieu.lacage@sophia.inria.fr>
 *
 */
#include "building-list.h"

#include "building.h"

#include "ns3/assert.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/object-vector.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BuildingList");

/**
 * \brief private implementation detail of the BuildingList API.
 */
class BuildingListPriv : public Object
{
  public:
    /**
     * \brief Get the type ID.
     * \return The object TypeId.
     */
    static TypeId GetTypeId();
    BuildingListPriv();
    ~BuildingListPriv() override;

    /**
     * Add a Building to the list.
     *
     * \param building building to add
     * \returns index of building in list.
     */
    uint32_t Add(Ptr<Building> building);
    /**
     * Returns an iterator to the start of the list.
     *
     * \returns iterator to the begin of the container.
     */
    BuildingList::Iterator Begin() const;
    /**
     * Returns an iterator to the end of the list.
     *
     * \returns iterator to the end of the container.
     */
    BuildingList::Iterator End() const;
    /**
     * Gets the n-th Building in the container
     * \param n Building position
     * \returns a pointer to the Building
     */
    Ptr<Building> GetBuilding(uint32_t n);
    /**
     * Gets the number of Building in the container
     * \returns the container size
     */
    uint32_t GetNBuildings();

    /**
     * Get the Singleton instance of BuildingListPriv (or create one)
     * \return the BuildingListPriv instance
     */
    static Ptr<BuildingListPriv> Get();

  private:
    void DoDispose() override;
    /**
     * Get the Singleton instance of BuildingListPriv (or create one)
     * \return the BuildingListPriv instance
     */
    static Ptr<BuildingListPriv>* DoGet();
    /**
     * Dispose the Singleton instance of BuildingListPriv.
     *
     * \note: this function is automatically called at the simulation end.
     *
     */
    static void Delete();
    std::vector<Ptr<Building>> m_buildings; //!< Container of Building
};

NS_OBJECT_ENSURE_REGISTERED(BuildingListPriv);

TypeId
BuildingListPriv::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::BuildingListPriv")
            .SetParent<Object>()
            .SetGroupName("Buildings")
            .AddAttribute("BuildingList",
                          "The list of all buildings created during the simulation.",
                          ObjectVectorValue(),
                          MakeObjectVectorAccessor(&BuildingListPriv::m_buildings),
                          MakeObjectVectorChecker<Building>());
    return tid;
}

Ptr<BuildingListPriv>
BuildingListPriv::Get()
{
    return *DoGet();
}

Ptr<BuildingListPriv>*
BuildingListPriv::DoGet()
{
    static Ptr<BuildingListPriv> ptr = nullptr;
    if (!ptr)
    {
        ptr = CreateObject<BuildingListPriv>();
        Config::RegisterRootNamespaceObject(ptr);
        Simulator::ScheduleDestroy(&BuildingListPriv::Delete);
    }
    return &ptr;
}

void
BuildingListPriv::Delete()
{
    NS_LOG_FUNCTION_NOARGS();
    Config::UnregisterRootNamespaceObject(Get());
    (*DoGet()) = nullptr;
}

BuildingListPriv::BuildingListPriv()
{
    NS_LOG_FUNCTION_NOARGS();
}

BuildingListPriv::~BuildingListPriv()
{
}

void
BuildingListPriv::DoDispose()
{
    NS_LOG_FUNCTION_NOARGS();
    for (auto i = m_buildings.begin(); i != m_buildings.end(); i++)
    {
        Ptr<Building> building = *i;
        building->Dispose();
        *i = nullptr;
    }
    m_buildings.erase(m_buildings.begin(), m_buildings.end());
    Object::DoDispose();
}

uint32_t
BuildingListPriv::Add(Ptr<Building> building)
{
    uint32_t index = m_buildings.size();
    m_buildings.push_back(building);
    Simulator::ScheduleWithContext(index, TimeStep(0), &Building::Initialize, building);
    return index;
}

BuildingList::Iterator
BuildingListPriv::Begin() const
{
    return m_buildings.begin();
}

BuildingList::Iterator
BuildingListPriv::End() const
{
    return m_buildings.end();
}

uint32_t
BuildingListPriv::GetNBuildings()
{
    return m_buildings.size();
}

Ptr<Building>
BuildingListPriv::GetBuilding(uint32_t n)
{
    NS_ASSERT_MSG(n < m_buildings.size(),
                  "Building index " << n << " is out of range (only have " << m_buildings.size()
                                    << " buildings).");
    return m_buildings.at(n);
}

} // namespace ns3

/**
 * The implementation of the public static-based API
 * which calls into the private implementation through
 * the simulation singleton.
 */
namespace ns3
{

uint32_t
BuildingList::Add(Ptr<Building> building)
{
    return BuildingListPriv::Get()->Add(building);
}

BuildingList::Iterator
BuildingList::Begin()
{
    return BuildingListPriv::Get()->Begin();
}

BuildingList::Iterator
BuildingList::End()
{
    return BuildingListPriv::Get()->End();
}

Ptr<Building>
BuildingList::GetBuilding(uint32_t n)
{
    return BuildingListPriv::Get()->GetBuilding(n);
}

uint32_t
BuildingList::GetNBuildings()
{
    return BuildingListPriv::Get()->GetNBuildings();
}

} // namespace ns3
