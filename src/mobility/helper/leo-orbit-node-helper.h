// Copyright (c) Tim Schubert
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Tim Schubert <ns-3-leo@timschubert.net>
// Porting: Thiago Miyazaki <miyathiago@gmail.com> <t.miyazaki@unesp.br>

#ifndef LEO_ORBIT_NODE_HELPER_H
#define LEO_ORBIT_NODE_HELPER_H

#include "ns3/leo-circular-orbit-mobility-model.h"
#include "ns3/leo-orbit.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"

#include <string>

/**
 * @file
 * @ingroup leo
 */

namespace ns3
{

/**
 * @ingroup leo
 * @brief Builds a node container of nodes with LEO positions using a list of
 * orbit definitions.
 *
 * Adds orbits with from a file for each node.
 */
class LeoOrbitNodeHelper
{
  public:
    /**
     * Construct a LEO Orbit Node Helper which is used to make life easier when working
     * with mobility models.
     * @param timeStep the time precision for the mobility, which determines the granularity of
     * positions generated in a given orbit (the faster the orbital speed, the smallest
     */
    LeoOrbitNodeHelper(const Time& timeStep);

    /// destructor
    virtual ~LeoOrbitNodeHelper();

    /**
     * @brief Install orbits from orbitFile.
     *
     * Install orbits from orbitFile containing one or more lines, each with 4 columns separated by
     * colons (:). The first column contains the orbit altitude (from the surface of the earth). The
     * second column contains the orbital plane inclination. The third column contains the number of
     * orbital planes, uniformly distributed (180/orbital planes). The fourth column contains the
     * number of satellites per orbital plane.
     *
     * One example: 1150.0:53.0:32:50, represents 32 orbits with 50 satellites each, with 53 degrees
     * of inclination in respect to the equator, and with an altitude of 1150 km.
     *
     * @param orbitFile path to orbit definitions file
     * @returns a node container containing nodes using the specified attributes
     */
    NodeContainer Install(const std::string& orbitFile);

    /**
     *
     * @param orbits orbit definitions
     * @returns a node container containing nodes using the specified attributes
     */
    NodeContainer Install(const std::vector<LeoOrbit>& orbits);

    /**
     *
     * @param orbit orbit definition
     * @returns a node container containing nodes using the specified attributes
     */
    NodeContainer Install(const LeoOrbit& orbit);

    /**
     * Set an attribute for each node
     *
     * @param name name of the attribute
     * @param value value of the attribute
     */
    void SetAttribute(std::string name, const AttributeValue& value);

    /**
     * Generates a Progress Vector and makes ptr point to the newly created vector.
     * A progress vector holds all positions that a node may be along its orbit. It is an
     * offset value regarding the node itself and its orbit.
     *
     * @param orbit a LeoOrbit object that holds orbit information
     * @returns a shared pointer to the vector containing positions of satellites for the specified
     * LEO orbit
     */
    std::shared_ptr<std::vector<double>> GenerateProgressVector(const LeoOrbit& orbit) const;

  private:
    ObjectFactory m_nodeFactory; ///< Node factory

    /// The period at which the mobility model is updated. Higher orbital speeds require smaller
    /// steps.
    Time m_timeStep;
};

}; // namespace ns3

#endif
