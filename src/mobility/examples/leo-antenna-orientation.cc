// Copyright (c) Tim Schubert
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Tim Schubert <ns-3-leo@timschubert.net>
// Additions & Modifications: Thiago Miyazaki <miyathiago@gmail.com> <t.miyazaki@unesp.br>

#include "math.h"

#include "ns3/core-module.h"
#include "ns3/geocentric-constant-position-mobility-model.h"
#include "ns3/geocentric-ecef-mobility-model.h"
#include "ns3/geographic-positions.h"
#include "ns3/isotropic-antenna-model.h"
#include "ns3/leo-orbit-node-helper.h"
#include "ns3/uniform-planar-array.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LeoAntennaOrientationExample");

Vector
    groundNodePosGEO; ///< Vector with ground station geodetic position (relative to earth surface)
Vector groundNodePosECEF; ///< Vector with ground station geocentric position (relative to center of
                          ///< earth)

/**
 * Point antenna downtilt and bearing angle of a satellite node antenna towards a fixed ground
 * station (groundNodePosGEO)
 * @param node the satellite node
 * @param period the period in which update the antenna angles (to be computed according to orbit
 * height)
 */
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

/**
 * Generate next position for a satellite node, given the node position in ECEF coordinate,
 * an azimuth and inclination
 * @param nodePosECEF the satellite node current position
 * @param azimuth the horizontal inclination from current position
 * @param zenith the vertical inclination from current position
 * @returns new coordinate for satellite node
 */
Vector
GeneratePoint(const Vector& nodePosECEF, const double azimuth, const double zenith)
{
    // convert node position vector from ECEF (x, y, z) to GEO (Geodesic/Geographic : lon/lat/alt)
    const Vector nodePosGEO =
        GeographicPositions::CartesianToGeographicCoordinates(nodePosECEF,
                                                              GeographicPositions::SPHERE);

    // in the plot, this is the magnitude of the vector that represent the antenna orientation
    // leaving the node and reaching the ground station
    const double orientationVectorLength = (groundNodePosECEF - nodePosECEF).GetLength();

    // pre-calculate sin(zenith)
    double s = std::sin(zenith);

    // newly generated point
    const Vector newPointUnitVectorInTopo(s * std::cos(azimuth),
                                          s * std::sin(azimuth),
                                          std::cos(zenith));

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

/**
 * Generate next position for a satellite node in a given orbit defined by its mobility model,
 * and move the satellite to said position
 * @param mob the mobility model for the satellite node
 */
void
CourseChange(Ptr<const MobilityModel> mob)
{
    const Vector pos = mob->GetPosition();
    Ptr<const Node> node = mob->GetObject<Node>();
    Ptr<UniformPlanarArray> ant = node->GetObject<UniformPlanarArray>();
    const Vector newP = GeneratePoint(pos, ant->GetAlpha(), ant->GetBeta());

    std::cout << Simulator::Now() << ":" << node->GetId() << ":" << pos.x << ":" << pos.y << ":"
              << pos.z << ":" << newP.x << ":" << newP.y << ":" << newP.z << std::endl;
}

int
main(int argc, char* argv[])
{
    // Satellite parameter
    double satAntennaGainDb = 24; // dB

    CommandLine cmd(__FILE__);
    std::string orbitFile;
    std::string traceFile;
    uint32_t duration = 60;    // seconds
    uint32_t precision = 1000; // milliseconds
    cmd.AddValue("orbitFile", "CSV file with orbit parameters", orbitFile);
    cmd.AddValue("traceFile", "CSV file to store mobility trace in", traceFile);
    cmd.AddValue("precision", "Mobility model time precision in milliseconds", precision);
    cmd.AddValue("duration", "Duration of the simulation in seconds", duration);
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
        satellites = orbit.Install(LeoOrbit(1180, 30, 1, 16));
        // satellites = orbit.Install({LeoOrbit(1200, 20, 32, 16), LeoOrbit(1180, 30, 12, 10)});
    }

    Config::ConnectWithoutContextFailSafe("/NodeList/*/$ns3::MobilityModel/CourseChange",
                                          MakeCallback(&CourseChange));

    // create node on the ground, create mobility, set position and aggregate mobility
    Ptr<Node> groundNode = CreateObject<Node>();
    Ptr<GeocentricEcefMobilityModel> groundNodeMobility =
        CreateObject<GeocentricEcefMobilityModel>();

    groundNodeMobility->SetGeographicPosition(
        Vector(41.27522532634883, 1.9876349942412692, 14)); // CTTC LatLon

    // set global vars with GEO and ECEF positions for the Ground Node
    groundNodePosGEO = groundNodeMobility->GetGeographicPosition();
    groundNodePosECEF = groundNodeMobility->GetGeocentricPosition();
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

    std::cout << "Time:Satellite:x:y:z:x_2:y_2:z_2" << std::endl;

    Simulator::Stop(Time(Seconds(duration)));
    Simulator::Run();
    Simulator::Destroy();

    out.close();
    std::cout.rdbuf(coutbuf);
}
