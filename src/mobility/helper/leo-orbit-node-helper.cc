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
#include "ns3/integer.h"
#include "ns3/log.h"
#include "ns3/waypoint.h"

#include <fstream>
#include <vector>

using namespace std;

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("LeoOrbitNodeHelper");

LeoOrbitNodeHelper::LeoOrbitNodeHelper(const Time& precision)
{
    m_nodeFactory.SetTypeId("ns3::Node");
    m_precision = precision;
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
        for (int32_t j = 0, remainderCopy = remainder; j < orbit.sats;
             j++, progressVectorIndex += truncatedRegularStepSize, remainderCopy--)
        {
            Ptr<Node> node = CreateObject<Node>();
            mobility.Install(node);
            Ptr<LeoCircularOrbitMobilityModel> currentNodeMobModel =
                node->GetObject<LeoCircularOrbitMobilityModel>();
            // associates the progress vector to the node
            currentNodeMobModel->SetProgressVectorPointer(progressVector);
            if (remainderCopy > 0)
            {
                // there's a remainder
                currentNodeMobModel->SetNodeIndexAtProgressVector(progressVectorIndex);
                // adds one space to dilute the remainder
                progressVectorIndex++;
            }
            else
            {
                // there's no remainder
                currentNodeMobModel->SetNodeIndexAtProgressVector(progressVectorIndex);
            }
            currentNodeMobModel->UpdateNodePositionAndScheduleEvent();
            satelliteContainer.Add(node);
        }
    }
    return satelliteContainer;
}

NodeContainer
LeoOrbitNodeHelper::Install(const std::string& orbitFile)
{
    NS_LOG_FUNCTION(this << orbitFile);

    NodeContainer nodes;
    ifstream orbits;
    orbits.open(orbitFile, ifstream::in);
    LeoOrbit orbit;
    while ((orbits >> orbit))
    {
        nodes.Add(Install(orbit));
        NS_LOG_DEBUG("Added orbit plane");
    }
    orbits.close();

    NS_LOG_DEBUG("Added " << nodes.GetN() << " nodes");

    return nodes;
}

NodeContainer
LeoOrbitNodeHelper::Install(const vector<LeoOrbit>& orbits)
{
    NS_LOG_FUNCTION(this << orbits);

    NodeContainer nodes;
    for (uint64_t i = 0; i < orbits.size(); i++)
    {
        nodes.Add(Install(orbits[i]));
        NS_LOG_DEBUG("Added orbit plane");
    }

    NS_LOG_DEBUG("Added " << nodes.GetN() << " nodes");

    return nodes;
}

std::shared_ptr<std::vector<double>>
LeoOrbitNodeHelper::GenerateProgressVector(const LeoOrbit& orbit) const
{
    // Creates the container and associates it to the pointer passed via argument
    auto progVecPtr = std::make_shared<std::vector<double>>();

    double nodeSpeed = sqrt(LEO_EARTH_GM_KM_E10 / (LEO_EARTH_RAD_KM + orbit.alt)) * 1e5;
    double orbitPerimeter = ((LEO_EARTH_RAD_KM * 1000) + orbit.alt) * 2 * M_PI;

    // Calculates the step size for the progress vector - the step size represents how much the
    // node progresses along its orbit during a single time resolution interval. Since the model
    // only fetches the node position at each interval, we only need to store its offset at each
    // interval.
    double progressStepPerTimeResolutionInterval = nodeSpeed * m_precision.GetSeconds();

    // Ensure correct gradient (not against earth rotation)
    int sign = 1;
    // Convert to radians then apply inverted signal if needed
    if (((orbit.inc / 180.0) * M_PI) > M_PI / 2)
    {
        sign = -1;
    }

    // It progresses step by step until it makes one cycle along the orbit, and at each step, we
    // calculate the angle/offset - creating a vector with all possible offsets given a time res.
    for (double i = 0; i < orbitPerimeter; i += progressStepPerTimeResolutionInterval)
    {
        // 2pi * sign * amountProgressed / earth circular perimeter
        progVecPtr->push_back((i / orbitPerimeter) * 2 * M_PI * sign);
    }

    return progVecPtr;
}

}; // namespace ns3
