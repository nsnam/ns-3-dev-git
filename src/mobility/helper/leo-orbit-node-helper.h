// Copyright (c) Tim Schubert
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Tim Schubert <ns-3-leo@timschubert.net>
// Porting: Thiago Miyazaki <miyathiago@gmail.com> <t.miyazaki@unesp.br>

#ifndef LEO_ORBIT_NODE_HELPER_H
#define LEO_ORBIT_NODE_HELPER_H

#include "ns3/leo-orbital-shell.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/object-factory.h"

#include <string>

/**
 * @file
 * @ingroup leo
 * LeoOrbitNodeHelper class declaration.
 */

namespace ns3
{

/**
 * @ingroup leo
 * @brief Builds a node container of nodes with LEO positions using a list of
 * orbit definitions.
 */
class LeoOrbitNodeHelper
{
  public:
    /**
     * @brief Construct a LEO Orbit Node Helper.
     *
     * @param resolution time interval between CourseChange notifications
     *        on the installed mobility models.  Smaller values produce
     *        more frequent notifications.  Zero disables periodic
     *        notifications.
     */
    LeoOrbitNodeHelper(const Time& resolution = Seconds(0));

    /// destructor
    virtual ~LeoOrbitNodeHelper() = default;

    /**
     * @brief Install orbital mobility on a node container using an orbit definition.
     * @param nodes node container
     * @param orbit orbit definition
     */
    void Install(NodeContainer nodes, const LeoOrbitalShell& orbit);

    /**
     * @brief Calculates the total number of satellites in a given LEO orbit.
     *
     * This method determines the total number of satellites by multiplying
     * the number of orbital planes by the number of satellites per plane.
     *
     * @param orbit Orbit definition containing the number of planes
     *        and the number of satellites per plane.
     * @return The total number of satellites in the specified orbit.
     */
    std::size_t CalculateNumberOfOrbitSatellites(const LeoOrbitalShell& orbit);

    /**
     * @brief Create satellite nodes and install orbital mobility from a CSV file.
     *
     * The CSV file contains one or more lines, each with 4-6 comma-separated columns:
     *   - altitude (km, from earth surface)
     *   - inclination (degrees relative to equator)
     *   - number of orbital planes
     *   - number of satellites per plane
     *   - phasing factor (optional, defaults to 0)
     *   - RAAN span in degrees (optional, defaults to 360)
     *
     * See LeoOrbitalShell for the definition of these parameters.
     *
     * Planes are evenly spaced by 360 degrees divided by the number of planes, starting from the
     * prime meridian.
     *
     * One example: 1150.0,53.0,32,50, represents 32 orbital planes with 50 satellites each, with 53
     * degrees of inclination in respect to the equator, and with an altitude of 1150 km.
     *
     * @param orbitFile path to orbit definitions file
     * @returns a node container containing the created nodes
     */
    NodeContainer CreateNodesAndInstallMobility(const std::string& orbitFile);

    /**
     * @brief Create satellite nodes and install orbital mobility from a vector
     *        of orbital parameters.
     * @param orbits orbit definitions
     * @returns a node container containing the created nodes
     */
    NodeContainer CreateNodesAndInstallMobility(const std::vector<LeoOrbitalShell>& orbits);

    /**
     * @brief Create satellite nodes and install orbital mobility from a single
     *        orbit definition.
     * @param orbit orbit definition
     * @returns a node container containing the created nodes
     */
    NodeContainer CreateNodesAndInstallMobility(const LeoOrbitalShell& orbit);

    /**
     * Set an attribute for each node
     *
     * @param name name of the attribute
     * @param value value of the attribute
     */
    void SetAttribute(std::string name, const AttributeValue& value);

    /**
     * @brief Set the time resolution for course change notifications.
     * @param resolution the time resolution
     */
    void SetResolution(Time resolution);

  private:
    ObjectFactory m_nodeFactory; ///< Node factory

    /// Time interval between CourseChange notifications
    Time m_resolution;
};

}; // namespace ns3

#endif
