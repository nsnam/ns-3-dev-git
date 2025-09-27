// Copyright (c) Tim Schubert
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Tim Schubert <ns-3-leo@timschubert.net>
// Porting: Thiago Miyazaki <miyathiago@gmail.com> <t.miyazaki@unesp.br>

#include "leo-orbit-node-helper.h"

#include "math.h"
#include "mobility-helper.h"

#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/geographic-positions.h"
#include "ns3/integer.h"
#include "ns3/log.h"
#include "ns3/waypoint.h"

#include <fstream>
#include <vector>

using namespace std;

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("LeoOrbitNodeHelper");

LeoOrbitNodeHelper::LeoOrbitNodeHelper(const Time& timeStep)
{
    m_nodeFactory.SetTypeId("ns3::Node");
    m_timeStep = timeStep;
}

LeoOrbitNodeHelper::~LeoOrbitNodeHelper()
{
}

void
LeoOrbitNodeHelper::SetAttribute(string name, const AttributeValue& value)
{
    m_nodeFactory.Set(name, value);
}

NodeContainer
LeoOrbitNodeHelper::Install(const LeoOrbit& orbit)
{
    NS_LOG_FUNCTION(this << orbit);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::LeoCircularOrbitPositionAllocator",
                                  "NumOrbits",
                                  IntegerValue(orbit.planes),
                                  "NumSatellites",
                                  IntegerValue(orbit.sats));
    mobility.SetMobilityModel("ns3::LeoCircularOrbitMobilityModel",
                              "Altitude",
                              DoubleValue(orbit.alt),
                              "Inclination",
                              DoubleValue(orbit.inc));

    auto progressVector = GenerateProgressVector(orbit);

    // This section:
    // - Creates a node, installs mobility, associate node to a progress vector, and adds node to
    // a NodeContainer.
    // - It also calculates the initial offset for nodes that share the same orbit, assigning each
    // node a different starting index, which will be increased at every call to Update().
    // - It tries to distribute nodes evenly within the Progress Vector, by diluting the remainder
    // space between the first nodes.

    uint32_t truncatedRegularStepSize = progressVector->size() / orbit.sats;
    int32_t remainder = progressVector->size() % orbit.sats;
    uint64_t progressVectorIndex;

    NodeContainer satelliteContainer;
    for (uint16_t i = 0; i < orbit.planes; i++)
    {
        progressVectorIndex = 0;
        auto rem = remainder;
        for (int32_t j = 0; j < orbit.sats; j++)
        {
            auto node = m_nodeFactory.Create<Node>();
            mobility.Install(node);
            auto currentNodeMobModel = node->GetObject<LeoCircularOrbitMobilityModel>();
            // associates the progress vector to the node
            currentNodeMobModel->SetProgressVectorPointer(progressVector);
            currentNodeMobModel->SetNodeIndexAtProgressVector(progressVectorIndex);
            if (rem > 0)
            {
                // there's a remainder
                // adds one space to dilute the remainder
                progressVectorIndex++;
            }
            currentNodeMobModel->UpdateNodePositionAndScheduleEvent();
            satelliteContainer.Add(node);
            progressVectorIndex += truncatedRegularStepSize;
            rem--;
        }
    }
    return satelliteContainer;
}

NodeContainer
LeoOrbitNodeHelper::Install(const std::string& orbitFile)
{
    NS_LOG_FUNCTION(this << orbitFile);

    NodeContainer nodes;
    ifstream orbitsf;
    orbitsf.open(orbitFile, ifstream::in);
    LeoOrbit orbit;
    while ((orbitsf >> orbit))
    {
        nodes.Add(Install(orbit));
        NS_LOG_DEBUG("Added orbit plane");
    }
    orbitsf.close();

    NS_LOG_DEBUG("Added " << nodes.GetN() << " nodes");

    return nodes;
}

NodeContainer
LeoOrbitNodeHelper::Install(const vector<LeoOrbit>& orbits)
{
    NS_LOG_FUNCTION(this << orbits);

    NodeContainer nodes;
    for (auto& orbit : orbits)
    {
        nodes.Add(Install(orbit));
        NS_LOG_DEBUG("Added orbit plane");
    }

    NS_LOG_DEBUG("Added " << nodes.GetN() << " nodes");

    return nodes;
}

constexpr double M_TO_KM = 1000; ///< Meters in a kilometer

std::shared_ptr<std::vector<double>>
LeoOrbitNodeHelper::GenerateProgressVector(const LeoOrbit& orbit) const
{
    // sqrt((km^3/s^2) / km) => sqrt(km^2/s^2) => km/s * 1000m/km = m/s
    const auto earthRadiusKm = (GeographicPositions::EARTH_SEMIMAJOR_AXIS / M_TO_KM);
    const auto orbitHeight = earthRadiusKm + orbit.alt;             // km
    double nodeSpeed = sqrt(LEO_EARTH_GGC / orbitHeight);           // km/s
    double orbitPerimeter = (earthRadiusKm + orbit.alt) * 2 * M_PI; // 2*pi*r km

    // Calculates the step size for the progress vector - the step size represents how much the
    // node progresses along its orbit during a single time resolution interval. Since the model
    // only fetches the node position at each interval, we only need to store its offset at each
    // interval.
    double stepSize = nodeSpeed * m_timeStep.GetSeconds(); // km

    // Ensure correct gradient (not against earth rotation)
    int sign = 1;
    // Convert to radians then apply inverted signal if needed
    if (((orbit.inc / 180.0) * M_PI) > M_PI / 2)
    {
        sign = -1;
    }

    // Fraction of orbit per step
    double stepFraction = stepSize / orbitPerimeter;

    // Total number of steps in the orbit
    std::size_t steps = std::round(1.0 / stepFraction);

    // Angular advance per step, anomaly per step
    double stepAngle = sign * 2 * M_PI * stepFraction;

    // Creates the container and associates it to the pointer passed via argument
    auto progVecPtr = std::make_shared<std::vector<double>>(steps);

    // It progresses step by step until it makes one cycle along the orbit, and at each step, we
    // calculate the angle/offset - creating a vector with all possible offsets given a time res.
    for (std::size_t i = 0; i < steps; i++)
    {
        // 2pi * sign * amountProgressed / earth circular perimeter
        progVecPtr->at(i) = i * stepAngle;
    }

    return progVecPtr;
}

}; // namespace ns3
