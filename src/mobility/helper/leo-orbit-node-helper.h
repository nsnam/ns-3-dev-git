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
    /// constructor
    LeoOrbitNodeHelper(const Time& precision);

    /// destructor
    virtual ~LeoOrbitNodeHelper();

    /**
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
     */
    std::shared_ptr<std::vector<double>> GenerateProgressVector(const LeoOrbit& orbit) const;

  private:
    ObjectFactory m_nodeFactory;

    /// The time precision of the model - the rate at which the mobility model is updated.
    Time m_precision;
};

}; // namespace ns3

#endif
