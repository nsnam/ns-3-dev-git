/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jaume Nin <jaume.nin@cttc,cat>
 * Based on NodeList implementation by Mathieu Lacage  <mathieu.lacage@sophia.inria.fr>
 *
 */

#ifndef BUILDING_LIST_H_
#define BUILDING_LIST_H_

#include "ns3/ptr.h"

#include <vector>

namespace ns3
{

class Building;

/**
 * @ingroup buildings
 *
 * Container for Building class
 */
class BuildingList
{
  public:
    /// Const Iterator
    typedef std::vector<Ptr<Building>>::const_iterator Iterator;

    /**
     * @param building building to add
     * @returns index of building in list.
     *
     * This method is called automatically from Building::Building so
     * the user has little reason to call it himself.
     */
    static uint32_t Add(Ptr<Building> building);
    /**
     * @returns a C++ iterator located at the beginning of this
     *          list.
     */
    static Iterator Begin();
    /**
     * @returns a C++ iterator located at the end of this
     *          list.
     */
    static Iterator End();
    /**
     * @param n index of requested building.
     * @returns the Building associated to index n.
     */
    static Ptr<Building> GetBuilding(uint32_t n);
    /**
     * @returns the number of buildings currently in the list.
     */
    static uint32_t GetNBuildings();
};

} // namespace ns3

#endif /* BUILDING_LIST_H_ */
