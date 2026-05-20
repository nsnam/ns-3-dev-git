// Copyright (c) Tim Schubert
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Tim Schubert <ns-3-leo@timschubert.net>
// Porting: Thiago Miyazaki <miyathiago@gmail.com> <t.miyazaki@unesp.br>

#include "leo-orbit-node-helper.h"

#include "mobility-helper.h"

#include "ns3/csv-reader.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"

#include <fstream>

using namespace std;

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("LeoOrbitNodeHelper");

LeoOrbitNodeHelper::LeoOrbitNodeHelper(const Time& resolution)
{
    NS_LOG_FUNCTION(this << resolution.As(Time::S));
    m_nodeFactory.SetTypeId("ns3::Node");
    m_resolution = resolution;
}

void
LeoOrbitNodeHelper::SetAttribute(string name, const AttributeValue& value)
{
    NS_LOG_FUNCTION(this);
    m_nodeFactory.Set(name, value);
}

void
LeoOrbitNodeHelper::SetResolution(Time resolution)
{
    NS_LOG_FUNCTION(this << resolution.As(Time::S));
    m_resolution = resolution;
}

std::size_t
LeoOrbitNodeHelper::CalculateNumberOfOrbitSatellites(const LeoOrbitalShell& orbit)
{
    return orbit.planes * orbit.sats;
}

NodeContainer
LeoOrbitNodeHelper::CreateNodesAndInstallMobility(const LeoOrbitalShell& orbit)
{
    NS_LOG_FUNCTION(this << orbit);

    NodeContainer satelliteContainer;
    for (std::size_t i = 0; i < CalculateNumberOfOrbitSatellites(orbit); i++)
    {
        satelliteContainer.Add(m_nodeFactory.Create<Node>());
    }

    Install(satelliteContainer, orbit);

    return satelliteContainer;
}

void
LeoOrbitNodeHelper::Install(NodeContainer nodes, const LeoOrbitalShell& orbit)
{
    NS_LOG_FUNCTION(this << &orbit);

    NS_LOG_INFO("Installing shell: altitude = "
                << orbit.alt << " km, inclination = " << orbit.inc << " deg, " << orbit.planes
                << " planes, " << orbit.sats << " sats/plane, phasing = " << orbit.phasing
                << ", RAAN span = " << orbit.raanSpanDeg << " deg");

    bool planes = orbit.planes == 0;
    bool sats = orbit.sats == 0;
    bool alt = orbit.alt <= 0;
    bool raan = orbit.raanSpanDeg <= 0 || orbit.raanSpanDeg > 360;
    bool phasing = orbit.phasing >= orbit.planes;

    NS_ABORT_MSG_IF(planes || sats || alt || raan || phasing,
                    (planes ? "Number of orbital planes must be > 0; " : "")
                        << (sats ? "Number of satellites per plane must be > 0; " : "")
                        << (alt ? "Orbital altitude must be > 0 km; " : "")
                        << (raan ? "RAAN span must be in (0, 360] degrees; " : "")
                        << (phasing ? "Phasing factor must be in [0, planes - 1]" : ""));

    if (nodes.GetN() != static_cast<uint32_t>(orbit.planes * orbit.sats))
    {
        NS_LOG_WARN("Number of nodes " << nodes.GetN() << " does not match constellation size "
                                       << orbit.planes * orbit.sats);
    }

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::LeoCircularOrbitPositionAllocator",
                                  "NumOrbits",
                                  UintegerValue(orbit.planes),
                                  "NumSatellites",
                                  UintegerValue(orbit.sats),
                                  "PhasingFactor",
                                  UintegerValue(orbit.phasing),
                                  "RaanSpanDeg",
                                  DoubleValue(orbit.raanSpanDeg));
    mobility.SetMobilityModel("ns3::LeoCircularOrbitMobilityModel",
                              "Altitude",
                              DoubleValue(orbit.alt),
                              "Inclination",
                              DoubleValue(orbit.inc),
                              "Resolution",
                              TimeValue(m_resolution));
    mobility.Install(nodes);
}

NodeContainer
LeoOrbitNodeHelper::CreateNodesAndInstallMobility(const std::string& orbitFile)
{
    NS_LOG_FUNCTION(this << orbitFile);

    // Read orbit file contents
    std::vector<LeoOrbitalShell> orbits;
    CsvReader csv(orbitFile);
    while (csv.FetchNextRow())
    {
        // Require at least 4 columns; allow up to 6
        if (csv.ColumnCount() < 4)
        {
            NS_LOG_WARN("Skipping row " << csv.RowNumber() << " of " << orbitFile
                                        << ": expected at least 4 columns, got "
                                        << csv.ColumnCount());
            continue;
        }

        LeoOrbitalShell orbit{};

        bool ok = csv.GetValue(0, orbit.alt);
        ok &= csv.GetValue(1, orbit.inc);
        ok &= csv.GetValue(2, orbit.planes);
        ok &= csv.GetValue(3, orbit.sats);
        if (!ok)
        {
            NS_LOG_WARN("Skipping row " << csv.RowNumber() << " of " << orbitFile
                                        << ": non-numeric value in required column");
            continue;
        }

        // Optional 5th column: Walker Delta phasing factor
        csv.GetValue(4, orbit.phasing);
        // Optional 6th column: RAAN span in degrees (360 = Delta, 180 = Star)
        csv.GetValue(5, orbit.raanSpanDeg);
        orbits.push_back(orbit);
    }

    NS_ABORT_MSG_IF(orbits.empty(),
                    "No valid orbit rows found in " << orbitFile << "; check file format");

    return CreateNodesAndInstallMobility(orbits);
}

NodeContainer
LeoOrbitNodeHelper::CreateNodesAndInstallMobility(const vector<LeoOrbitalShell>& orbits)
{
    NS_LOG_FUNCTION(this << orbits);

    NodeContainer nodes;
    for (auto& orbit : orbits)
    {
        nodes.Add(CreateNodesAndInstallMobility(orbit));
        NS_LOG_DEBUG("Added orbit plane");
    }

    NS_LOG_DEBUG("Added " << nodes.GetN() << " nodes");

    return nodes;
}

}; // namespace ns3
