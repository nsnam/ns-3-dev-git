/*
 * Copyright (c) 2007 INRIA
 * Copyright (C) 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Author: Nicola Baldo <nbaldo@cttc.es> (took position-allocator and turned it into
 * building-allocator)
 */
#ifndef BUILDING_ALLOCATOR_H
#define BUILDING_ALLOCATOR_H

#include "building-container.h"

#include "ns3/object-factory.h"
#include "ns3/object.h"
#include "ns3/position-allocator.h"
#include "ns3/vector.h"

namespace ns3
{

class Building;

/**
 * @ingroup buildings
 * @brief Allocate buildings on a rectangular 2d grid.
 *
 * This class allows to create a set of buildings positioned on a
 * rectangular 2D grid. Under the hood, this class uses two instances
 * of GridPositionAllocator.
 */
class GridBuildingAllocator : public Object
{
  public:
    GridBuildingAllocator();
    ~GridBuildingAllocator() override;

    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Set an attribute to be used for each new building to be created
     *
     * @param n attribute name
     * @param v attribute value
     */
    void SetBuildingAttribute(std::string n, const AttributeValue& v);

    /**
     * Create a set of buildings allocated on a grid
     *
     * @param n the number of buildings to create
     *
     * @return the BuildingContainer that contains the newly created buildings
     */
    BuildingContainer Create(uint32_t n) const;

  private:
    /**
     * Pushes the attributes into the relevant position allocators
     */
    void PushAttributes() const;

    mutable uint32_t m_current;                     //!< Current building number
    GridPositionAllocator::LayoutType m_layoutType; //!< Layout type
    double m_xMin;                                  //!< The x coordinate where the grid starts
    double m_yMin;                                  //!< The y coordinate where the grid starts
    uint32_t m_n;                                   //!< The number of objects laid out on a line
    double m_lengthX; //!< The length of the wall of each building along the X axis.
    double m_lengthY; //!< The length of the wall of each building along the Y axis.
    double m_deltaX;  //!< The x space between buildings
    double m_deltaY;  //!< The y space between buildings
    double m_height;  //!< The height of the building (roof level)

    mutable ObjectFactory m_buildingFactory;                 //!< The building factory
    Ptr<GridPositionAllocator> m_lowerLeftPositionAllocator; //!< The upper left position allocator
    Ptr<GridPositionAllocator>
        m_upperRightPositionAllocator; //!< The upper right position allocator
};

} // namespace ns3

#endif /* BUILDING_ALLOCATOR_H */
