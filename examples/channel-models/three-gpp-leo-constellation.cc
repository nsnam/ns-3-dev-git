// Copyright (c) Tim Schubert
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Tim Schubert <ns-3-leo@timschubert.net>
// Additions & Modifications: Thiago Miyazaki <miyathiago@gmail.com> <t.miyazaki@unesp.br>

#include "math.h"

#include "ns3/channel-condition-model.h"
#include "ns3/core-module.h"
#include "ns3/geocentric-constant-position-mobility-model.h"
#include "ns3/geocentric-ecef-mobility-model.h"
#include "ns3/geographic-positions.h"
#include "ns3/isotropic-antenna-model.h"
#include "ns3/leo-orbit-node-helper.h"
#include "ns3/three-gpp-propagation-loss-model.h"
#include "ns3/uniform-planar-array.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LeoLinkExample");

Vector groundNodePosGEO;
Vector groundNodePosECEF;
Ptr<Node> groundNode;
Ptr<ThreeGppPropagationLossModel> propagationLossModel;

void
UpdateAntennaOrientation(Ptr<Node> node, Time period)
{
    auto mobility = node->GetObject<MobilityModel>();
    const auto satelliteNodePositionECEF = mobility->GetPosition();

    // convert from ECEF (x, y, z) to Geodetic/Geographic (lon/lat/alt)
    const Vector satelliteNodePositionGEO =
        GeographicPositions::CartesianToGeographicCoordinates(satelliteNodePositionECEF,
                                                              GeographicPositions::SPHERE);

    // makes translation (origin == sat node) and converts to Topocentric
    const Vector translatedTopo =
        GeographicPositions::GeographicToTopocentricCoordinates(groundNodePosGEO,
                                                                satelliteNodePositionGEO,
                                                                GeographicPositions::SPHERE);

    // provide target point in Topocentric
    const Angles angles(translatedTopo);

    // set antenna angles
    auto satelliteNodeAntenna = node->GetObject<UniformPlanarArray>();
    satelliteNodeAntenna->SetAlpha(angles.GetAzimuth());
    satelliteNodeAntenna->SetBeta(angles.GetInclination());

    // schedules itself again after a certain period
    Simulator::Schedule(period, &UpdateAntennaOrientation, node, period);
}

Vector
GeneratePoint(const Vector& nodePosECEF, const double azimuth, const double inclination)
{
    // convert node position vector from ECEF (x, y, z) to GEO (Geodesic/Geographic : lon/lat/alt)
    const Vector nodePosGEO =
        GeographicPositions::CartesianToGeographicCoordinates(nodePosECEF,
                                                              GeographicPositions::SPHERE);

    // in the plot, this is the magnitude of the vector that represent the antenna orientation
    // leaving the node and reaching the ground station
    const double orientationVectorLength = (groundNodePosECEF - nodePosECEF).GetLength();

    // pre-calculate sin(inclination)
    double s = std::sin(inclination);

    // newly generated point
    const Vector newPointUnitVectorInTopo(s * std::cos(azimuth),
                                          s * std::sin(azimuth),
                                          std::cos(inclination));

    // apply magnitude to the unit vector
    const Vector newPointWithMagnitudeTopo = newPointUnitVectorInTopo * orientationVectorLength;

    // convert from Topocentric/ENU to Geographic (2nd arg. must be in GEO)
    const Vector newPointWithMagnitudeGEO =
        GeographicPositions::TopocentricToGeographicCoordinates(newPointWithMagnitudeTopo,
                                                                nodePosGEO,
                                                                GeographicPositions::SPHERE);

    // convert from Geographic to Cartesian - the plot uses cartesian coordinates.
    const Vector newPointWithMagnitudeECEF =
        GeographicPositions::GeographicToCartesianCoordinates(newPointWithMagnitudeGEO.x,
                                                              newPointWithMagnitudeGEO.y,
                                                              newPointWithMagnitudeGEO.z,
                                                              GeographicPositions::SPHERE);

    return newPointWithMagnitudeECEF;
}

void
CourseChange(Ptr<const MobilityModel> mob)
{
    // Skip if ground node is not yet initialized
    if (!groundNode)
    {
        return;
    }

    const Vector pos = mob->GetPosition();
    Ptr<const Node> node = mob->GetObject<Node>();
    Ptr<UniformPlanarArray> ant = node->GetObject<UniformPlanarArray>();
    const Vector newP = GeneratePoint(pos, ant->GetAlpha(), ant->GetBeta());

    // Compute slant distance between satellite and ground station
    Vector satPosECEF = pos;
    double slantDistance = (satPosECEF - groundNodePosECEF).GetLength();

    // Compute elevation angle from ground station to satellite
    // Convert satellite position to geographic coordinates
    Vector satPosGEO =
        GeographicPositions::CartesianToGeographicCoordinates(satPosECEF,
                                                              GeographicPositions::SPHERE);

    // Convert to topocentric coordinates (from ground station perspective)
    Vector topoVector =
        GeographicPositions::GeographicToTopocentricCoordinates(satPosGEO,
                                                                groundNodePosGEO,
                                                                GeographicPositions::SPHERE);

    // Calculate elevation angle (90 degrees - inclination angle)
    Angles angles(topoVector);
    double elevationAngle = 90.0 - angles.GetInclination() * 180.0 / M_PI; // Convert to degrees

    // Compute propagation loss between satellite and ground station
    double pathLossDb = 0.0;
    if (propagationLossModel)
    {
        Ptr<MobilityModel> satMobility = node->GetObject<MobilityModel>();
        Ptr<MobilityModel> groundMobility = groundNode->GetObject<MobilityModel>();
        NS_LOG_DEBUG("Ground Node GetPosition(): " << groundMobility->GetPosition());
        NS_LOG_DEBUG("Satellite GetPosition(): " << satMobility->GetPosition());

        // Calculate path loss using CalcRxPower with reference tx power of 0 dBm
        // Path loss = Tx Power - Rx Power
        double txPowerDbm = 0.0;
        double rxPowerDbm =
            propagationLossModel->CalcRxPower(txPowerDbm, satMobility, groundMobility);
        pathLossDb = txPowerDbm - rxPowerDbm;
    }

    std::cout << Simulator::Now() << ":" << node->GetId() << ":" << pos.x << ":" << pos.y << ":"
              << pos.z << ":" << newP.x << ":" << newP.y << ":" << newP.z << ":" << pathLossDb
              << ":" << slantDistance << ":" << elevationAngle << std::endl;
}

int
main(int argc, char* argv[])
{
    // Satellite parameter
    double satAntennaGainDb = 24; // dB

    CommandLine cmd(__FILE__);
    std::string orbitFile;
    std::string traceFile;
    std::string scenario = "NTN-Rural"; // Default NTN scenario
    double frequencyHz = 2.0e9;         // Default frequency: 2 GHz
    uint32_t duration = 60;             // seconds
    uint32_t precision = 1000;          // milliseconds
    cmd.AddValue("orbitFile", "CSV file with orbit parameters", orbitFile);
    cmd.AddValue("traceFile", "CSV file to store mobility trace in", traceFile);
    cmd.AddValue("precision", "Mobility model time precision in milliseconds", precision);
    cmd.AddValue("duration", "Duration of the simulation in seconds", duration);
    cmd.AddValue("scenario",
                 "NTN propagation scenario (NTN-DenseUrban, NTN-Urban, NTN-Suburban, NTN-Rural)",
                 scenario);
    cmd.AddValue("frequency", "Carrier frequency in Hz", frequencyHz);
    cmd.Parse(argc, argv);

    // sets the time precision for the mobility model, i.e. how often its position will be updated
    Config::SetDefault("ns3::LeoCircularOrbitMobilityModel::Precision",
                       TimeValue(MilliSeconds(precision)));

    LeoOrbitNodeHelper orbit(Time(MilliSeconds(precision)));

    // creates the satellite nodes and put them into a container
    NodeContainer satellites;
    if (!orbitFile.empty())
    {
        satellites = orbit.Install(orbitFile);
    }
    else
    {
        satellites = orbit.Install(LeoOrbit(600, 30, 1, 1));
    }

    Config::ConnectWithoutContextFailSafe("/NodeList/*/$ns3::MobilityModel/CourseChange",
                                          MakeCallback(&CourseChange));

    // create and configure the factories for the channel condition and propagation loss models
    ObjectFactory propagationLossModelFactory;
    ObjectFactory channelConditionModelFactory;

    if (scenario == "NTN-DenseUrban")
    {
        propagationLossModelFactory.SetTypeId(
            ThreeGppNTNDenseUrbanPropagationLossModel::GetTypeId());
        channelConditionModelFactory.SetTypeId(
            ThreeGppNTNDenseUrbanChannelConditionModel::GetTypeId());
    }
    else if (scenario == "NTN-Urban")
    {
        propagationLossModelFactory.SetTypeId(ThreeGppNTNUrbanPropagationLossModel::GetTypeId());
        channelConditionModelFactory.SetTypeId(ThreeGppNTNUrbanChannelConditionModel::GetTypeId());
    }
    else if (scenario == "NTN-Suburban")
    {
        propagationLossModelFactory.SetTypeId(ThreeGppNTNSuburbanPropagationLossModel::GetTypeId());
        channelConditionModelFactory.SetTypeId(
            ThreeGppNTNSuburbanChannelConditionModel::GetTypeId());
    }
    else if (scenario == "NTN-Rural")
    {
        propagationLossModelFactory.SetTypeId(ThreeGppNTNRuralPropagationLossModel::GetTypeId());
        channelConditionModelFactory.SetTypeId(ThreeGppNTNRuralChannelConditionModel::GetTypeId());
    }
    else
    {
        NS_LOG_WARN("Unknown NTN scenario: " << scenario << ". Using NTN-Rural.");
        propagationLossModelFactory.SetTypeId(ThreeGppNTNRuralPropagationLossModel::GetTypeId());
        channelConditionModelFactory.SetTypeId(ThreeGppNTNRuralChannelConditionModel::GetTypeId());
    }

    // create the propagation loss model
    propagationLossModel = propagationLossModelFactory.Create<ThreeGppPropagationLossModel>();
    propagationLossModel->SetAttribute("Frequency", DoubleValue(frequencyHz));
    propagationLossModel->SetAttribute("ShadowingEnabled", BooleanValue(true));

    // create the channel condition model and associate it with the propagation loss model
    Ptr<ChannelConditionModel> condModel =
        channelConditionModelFactory.Create<ThreeGppChannelConditionModel>();
    propagationLossModel->SetChannelConditionModel(condModel);

    // create node on the ground, create mobility, set position and aggregate mobility
    groundNode = CreateObject<Node>();
    Ptr<GeocentricEcefMobilityModel> groundNodeMobility =
        CreateObject<GeocentricEcefMobilityModel>();
    // get the first satellite instantiated and place ground node beneath it
    auto firstSatelliteMobility = satellites.Get(0)->GetObject<MobilityModel>();
    auto firstSatellitePositionGEO =
        GeographicPositions::CartesianToGeographicCoordinates(firstSatelliteMobility->GetPosition(),
                                                              GeographicPositions::SPHERE);
    groundNodeMobility->SetGeographicPosition(
        Vector(firstSatellitePositionGEO.x, firstSatellitePositionGEO.y, 0));

    // set global vars with GEO and ECEF positions for the Ground Node
    groundNodePosGEO = groundNodeMobility->GetGeographicPosition();
    groundNodePosECEF = groundNodeMobility->GetGeocentricPosition();
    NS_LOG_DEBUG("Ground Node Geocentric Position (ECEF x/y/z): " << groundNodePosECEF);
    NS_LOG_DEBUG("Ground Node GetPosition(): " << groundNodeMobility->GetPosition());
    NS_LOG_DEBUG("Satellite GetPosition(): " << firstSatelliteMobility->GetPosition());
    if (groundNodePosECEF != groundNodeMobility->GetPosition())
    {
        std::cerr << "Warning: Ground Node ECEF position does not match Mobility Model position! "
                     "This may cause problems with position-dependent calculations."
                  << std::endl;
    }
    groundNode->AggregateObject(groundNodeMobility);

    Ptr<Node> node;
    for (uint32_t i = 0; i < satellites.GetN(); i++)
    {
        node = satellites.Get(i);
        node->AggregateObject(CreateObjectWithAttributes<UniformPlanarArray>(
            "NumColumns",
            UintegerValue(1),
            "NumRows",
            UintegerValue(1),
            "AntennaElement",
            PointerValue(
                CreateObjectWithAttributes<IsotropicAntennaModel>("Gain",
                                                                  DoubleValue(satAntennaGainDb)))));

        UpdateAntennaOrientation(node, MilliSeconds(500));
    }

    std::streambuf* coutbuf = std::cout.rdbuf();
    // redirect cout if traceFile is specified
    std::ofstream out;
    out.open(traceFile);
    if (out.is_open())
    {
        std::cout.rdbuf(out.rdbuf());
    }

    std::cout << "Time:Satellite:x:y:z:x_2:y_2:z_2:PathLoss_dB:SlantDistance_m:ElevationAngle_deg"
              << std::endl;

    Simulator::Stop(Time(Seconds(duration)));
    Simulator::Run();
    Simulator::Destroy();

    out.close();
    std::cout.rdbuf(coutbuf);
}
