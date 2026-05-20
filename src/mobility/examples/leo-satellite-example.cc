// Copyright (c) Tim Schubert
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Tim Schubert <ns3-leo@timschubert.net>
// Additions & Modifications: Thiago Miyazaki <miyathiago@gmail.com> <t.miyazaki@unesp.br>
// Additions & Modifications: Jesse Chiu <jessest94106@gmail.com>
//
/**
 * @file
 * @ingroup leo
 * Unified LEO satellite example that combines three use cases into a single program,
 * controlled by the --mode switch:
 *
 * - --mode=orbit       Mobility tracing only (position + speed)
 * - --mode=antenna     Antenna pointing toward a ground station (no propagation)
 * - --mode=channel     Full 3GPP NTN propagation loss estimation
 *
 * This example sets up a LEO constellation (from a CSV file or a default orbit),
 * places a ground station, and optionally steers each satellite's antenna toward
 * the ground station. At each CourseChange event, it outputs CSV trace data.
 *
 * Common command-line options:
 * - orbitFile: path to a CSV containing orbit parameters for satellites
 * - traceFile: path to a CSV file to store trace data; if omitted or empty,
 *              traces are written to stdout
 * - resolution: time interval between CourseChange notifications (seconds)
 * - duration: total simulation time in seconds
 *
 * Mode-specific options:
 * - mode: one of orbit, antenna, channel (default: antenna)
 * - groundStation: ground station location as "lat,lon,alt" (e.g., "41.275,1.988,14")
 *                  or "auto" to place it beneath the first satellite (default: auto)
 * - scenario: NTN propagation scenario (channel mode only)
 *             NTN-DenseUrban, NTN-Urban, NTN-Suburban, NTN-Rural (default: NTN-Rural)
 * - frequency: carrier frequency in Hz (channel mode only, default: 2e9)
 * - antennaGain: satellite antenna gain in dBi (antenna/channel mode, default: 34.6)
 * - antennaUpdatePeriod: antenna orientation update interval (default: 500 ms)
 */

#include "math.h"

#include "ns3/channel-condition-model.h"
#include "ns3/core-module.h"
#include "ns3/geocentric-constant-position-mobility-model.h"
#include "ns3/geocentric-ecef-mobility-model.h"
#include "ns3/geographic-positions.h"
#include "ns3/isotropic-antenna-model.h"
#include "ns3/leo-orbit-node-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/three-gpp-propagation-loss-model.h"
#include "ns3/uniform-planar-array.h"

#include <fstream>
#include <map>
#include <sstream>
#include <utility>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LeoSatelliteExample");

/// Simulation mode selected via --mode
enum class LeoMode
{
    Orbit,   ///< Mobility tracing only
    Antenna, ///< Antenna pointing toward ground station
    Channel  ///< Full 3GPP NTN propagation loss
};

/**
 * @brief Ground station definition.
 */
struct GroundStation
{
    Ptr<Node> node; ///< Ground station node
    Vector posGEO;  ///< Ground station geodetic position (latitude/longitude/altitude)
    Vector posECEF; ///< Ground station ECEF position (x/y/z in meters)
};

/// Global ground station
GroundStation groundStation;

/// Global propagation loss model (used by tracer)
Ptr<ThreeGppPropagationLossModel> propagationLossModel;

/**
 * @brief Helper class to trace LEO constellation mobility and/or antenna pointing.
 *
 * This tracer periodically updates satellite antenna orientation toward a
 * fixed ground station, optionally computes propagation loss, and outputs
 * trace data including satellite position, antenna pointing vector endpoints,
 * path loss, slant distance, and elevation angle.
 */
class LeoConstellationTracer
{
  public:
    /**
     * @brief Construct a LeoConstellationTracer.
     *
     * @param groundNode Pointer to the ground station node.
     * @param mode The simulation mode.
     * @param propagationLossModel The propagation loss model (can be null for non-channel modes).
     * @param groundNodePosGEO Ground station geodetic position (lat/lon/alt).
     * @param groundNodePosECEF Ground station ECEF position (x/y/z in meters).
     * @param traceStream Output stream for trace data.
     */
    LeoConstellationTracer(Ptr<Node> groundNode,
                           LeoMode mode,
                           Ptr<ThreeGppPropagationLossModel> propagationLossModel,
                           Vector groundNodePosGEO,
                           Vector groundNodePosECEF,
                           std::ostream* traceStream)
        : m_mode(mode),
          m_groundNode(groundNode),
          m_propagationLossModel(propagationLossModel),
          m_groundNodePosGEO(groundNodePosGEO),
          m_groundNodePosECEF(groundNodePosECEF),
          m_traceStream(traceStream)
    {
    }

    /**
     * @brief Compute a point along a satellite's antenna pointing direction.
     *
     * Given a satellite position in ECEF and its antenna azimuth and
     * inclination angles, this function returns a second ECEF point at the
     * end of the antenna orientation vector (whose length equals the
     * satellite-to-ground-station distance).  The two points can be used
     * to visualize the antenna pointing direction.
     *
     * @param nodePosECEF the satellite node ECEF position.
     * @param azimuth the antenna azimuth angle in radians.
     * @param inclination the antenna inclination angle in radians.
     * @return ECEF coordinate of the endpoint of the orientation vector.
     */
    Vector GeneratePoint(const Vector& nodePosECEF,
                         const double azimuth,
                         const double inclination) const;

    /**
     * @brief Periodically steer a satellite node's antenna toward the ground station.
     *
     * The antenna angles are computed from the relative position between the
     * satellite and a fixed ground station, then applied to the node's uniform
     * planar antenna array.
     *
     * @param node the satellite node.
     * @param period the interval at which to update the antenna angles (seconds).
     */
    void UpdateAntennaOrientation(Ptr<Node> node, Time period) const;

    /**
     * @brief CourseChange callback that outputs trace data.
     *
     * The output columns depend on the mode:
     * - orbit: Time,Satellite,x,y,z,Speed
     * - antenna: Time,Satellite,x,y,z,x_2,y_2,z_2
     * - channel: Time,Satellite,x,y,z,x_2,y_2,z_2,PathLoss_dB,SlantDistance_m,ElevationAngle_deg
     *
     * @param mob the satellite mobility model that changed course.
     */
    void CourseChange(Ptr<const MobilityModel> mob) const;

  private:
    LeoMode m_mode;                                           ///< Simulation mode
    Ptr<Node> m_groundNode;                                   ///< Ground station node
    Ptr<ThreeGppPropagationLossModel> m_propagationLossModel; ///< Propagation loss model
    Vector m_groundNodePosGEO;                                ///< Ground station geodetic position
    Vector m_groundNodePosECEF;                               ///< Ground station ECEF position
    std::ostream* m_traceStream;                              ///< Output stream for trace data
};

void
LeoConstellationTracer::UpdateAntennaOrientation(Ptr<Node> node, Time period) const
{
    auto mobility = node->GetObject<MobilityModel>();
    const auto satelliteNodePositionECEF = mobility->GetPosition();

    // Convert from ECEF (x [m], y [m], z [m]) to Geodetic/Geographic (lat [deg], lon [deg],
    // alt [m])
    const Vector satelliteNodePositionGEO =
        GeographicPositions::CartesianToGeographicCoordinates(satelliteNodePositionECEF,
                                                              GeographicPositions::SPHERE);

    // makes translation (origin == sat node) and converts to Topocentric
    const Vector translatedTopo =
        GeographicPositions::GeographicToTopocentricCoordinates(groundStation.posGEO,
                                                                satelliteNodePositionGEO,
                                                                GeographicPositions::SPHERE);

    // provide target point in Topocentric
    const Angles angles(translatedTopo);

    // set antenna angles
    auto satelliteNodeAntenna = node->GetObject<UniformPlanarArray>();
    satelliteNodeAntenna->SetAlpha(angles.GetAzimuth());
    satelliteNodeAntenna->SetBeta(angles.GetInclination());

    // schedules itself again after a certain period
    Simulator::Schedule(period,
                        &LeoConstellationTracer::UpdateAntennaOrientation,
                        this,
                        node,
                        period);
}

Vector
LeoConstellationTracer::GeneratePoint(const Vector& nodePosECEF,
                                      const double azimuth,
                                      const double inclination) const
{
    // Convert from ECEF (x [m], y [m], z [m]) to Geodetic/Geographic (lat [deg], lon [deg],
    // alt [m])
    const Vector nodePosGEO =
        GeographicPositions::CartesianToGeographicCoordinates(nodePosECEF,
                                                              GeographicPositions::SPHERE);

    // in the plot, this is the magnitude of the vector that represent the antenna orientation
    // leaving the node and reaching the ground station
    const double orientationVectorLength = (groundStation.posECEF - nodePosECEF).GetLength();

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
LeoConstellationTracer::CourseChange(Ptr<const MobilityModel> mob) const
{
    // Skip if ground node is not yet initialized
    if (!m_groundNode)
    {
        return;
    }

    const Vector satPosECEF = mob->GetPosition();
    Ptr<const Node> node = mob->GetObject<Node>();

    // Print header based on mode
    static bool headerPrinted = false;
    if (!headerPrinted)
    {
        switch (m_mode)
        {
        case LeoMode::Orbit:
            (*m_traceStream) << "Time,Satellite,x,y,z,Speed" << std::endl;
            break;
        case LeoMode::Antenna:
            (*m_traceStream) << "Time,Satellite,x,y,z,x_2,y_2,z_2" << std::endl;
            break;
        case LeoMode::Channel:
            (*m_traceStream)
                << "Time,Satellite,x,y,z,x_2,y_2,z_2,PathLoss_dB,SlantDistance_m,ElevationAngle_deg"
                << std::endl;
            break;
        }
        headerPrinted = true;
    }

    // Compute antenna pointing endpoint for antenna/channel modes
    Vector newP;
    if (m_mode == LeoMode::Antenna || m_mode == LeoMode::Channel)
    {
        Ptr<UniformPlanarArray> ant = node->GetObject<UniformPlanarArray>();
        if (ant)
        {
            newP = GeneratePoint(satPosECEF, ant->GetAlpha(), ant->GetBeta());
        }
    }

    if (m_mode == LeoMode::Orbit)
    {
        // Mobility trace: position + speed
        (*m_traceStream) << Simulator::Now() << "," << node->GetId() << "," << satPosECEF.x << ","
                         << satPosECEF.y << "," << satPosECEF.z << ","
                         << mob->GetVelocity().GetLength() << std::endl;
    }
    else if (m_mode == LeoMode::Antenna)
    {
        // Antenna trace: position + antenna pointing endpoint
        (*m_traceStream) << Simulator::Now() << "," << node->GetId() << "," << satPosECEF.x << ","
                         << satPosECEF.y << "," << satPosECEF.z << "," << newP.x << "," << newP.y
                         << "," << newP.z << std::endl;
    }
    else if (m_mode == LeoMode::Channel)
    {
        // Channel trace: position + antenna pointing + propagation metrics
        // Compute slant distance between satellite and ground station
        double slantDistance = (satPosECEF - groundStation.posECEF).GetLength();

        // Compute elevation angle from ground station to satellite
        // Convert satellite position to geographic coordinates
        Vector satPosGEO =
            GeographicPositions::CartesianToGeographicCoordinates(satPosECEF,
                                                                  GeographicPositions::SPHERE);

        // Convert to topocentric coordinates (from ground station perspective)
        Vector topoVector =
            GeographicPositions::GeographicToTopocentricCoordinates(satPosGEO,
                                                                    groundStation.posGEO,
                                                                    GeographicPositions::SPHERE);

        // Calculate elevation angle (90 degrees - inclination angle)
        Angles angles(topoVector);
        double elevationAngle = 90.0 - RadiansToDegrees(angles.GetInclination());

        // Compute propagation loss between satellite and ground station
        double pathLossDb = 0.0;
        if (m_propagationLossModel)
        {
            Ptr<MobilityModel> groundMobility = groundStation.node->GetObject<MobilityModel>();

            // Calculate path loss using CalcRxPower with reference tx power of 0 dBm
            // Path loss = Tx Power - Rx Power
            double txPowerDbm = 0.0;
            double rxPowerDbm = m_propagationLossModel->CalcRxPower(txPowerDbm,
                                                                    ConstCast<MobilityModel>(mob),
                                                                    groundMobility);
            pathLossDb = txPowerDbm - rxPowerDbm;
        }

        (*m_traceStream) << Simulator::Now() << "," << node->GetId() << "," << satPosECEF.x << ","
                         << satPosECEF.y << "," << satPosECEF.z << "," << newP.x << "," << newP.y
                         << "," << newP.z << "," << pathLossDb << "," << slantDistance << ","
                         << elevationAngle << std::endl;
    }
}

/**
 * @brief Parse a ground station location string "lat,lon,alt" into a Vector.
 *
 * @param locationStr string in "lat,lon,alt" format (degrees, degrees, meters).
 * @return Vector with x=lat, y=lon, z=alt.
 */
static Vector
ParseGroundStation(const std::string& locationStr)
{
    std::istringstream iss(locationStr);
    std::string token;
    std::vector<double> values;
    while (std::getline(iss, token, ','))
    {
        values.push_back(std::stod(token));
    }
    NS_ABORT_MSG_IF(values.size() != 3, "Ground station must be in 'lat,lon,alt' format");
    return Vector(values[0], values[1], values[2]);
}

int
main(int argc, char* argv[])
{
    // Satellite antenna gain based on Starlink's report submitted to OFCOM:
    // https://www.ofcom.org.uk/siteassets/resources/documents/consultations/category-3-4-weeks/281181-kepler-communications-inc-application/associated-documents/starlink-annex.pdf?v=393905
    double satAntennaGainDb = 34.6; // dBi

    CommandLine cmd(__FILE__);
    std::string orbitFile;
    std::string traceFile;
    std::string mode = "antenna";
    std::string groundStationStr = "41.275,1.988,14";
    std::string scenario = "NTN-Rural";
    double frequencyHz = 2.0e9; // Default frequency: 2 GHz
    Time duration = Seconds(60);
    Time orbitTimeStepResolution = Seconds(1);
    Time antennaUpdatePeriod = MilliSeconds(500);
    cmd.AddValue("orbitFile",
                 "CSV file with orbit parameters. "
                 "See LeoOrbitNodeHelper::CreateNodesAndInstallMobility() for documentation.",
                 orbitFile);
    cmd.AddValue("traceFile", "CSV file to store trace data in; empty = stdout", traceFile);
    cmd.AddValue("mode", "Mode: orbit, antenna, channel (default: antenna)", mode);
    cmd.AddValue("groundStation",
                 "Ground station location as 'lat,lon,alt' or 'auto' (default: 41.275,1.988,14)",
                 groundStationStr);
    cmd.AddValue("resolution",
                 "Mobility model time resolution step (seconds)",
                 orbitTimeStepResolution);
    cmd.AddValue("duration", "Duration of the simulation in seconds", duration);
    cmd.AddValue("scenario",
                 "NTN propagation scenario (NTN-DenseUrban, NTN-Urban, NTN-Suburban, NTN-Rural) "
                 "(default: NTN-Rural)",
                 scenario);
    cmd.AddValue("frequency", "Carrier frequency in Hz (default: 2e9 Hz)", frequencyHz);
    cmd.AddValue("antennaGain", "Satellite antenna gain in dBi (default: 34.6)", satAntennaGainDb);
    cmd.AddValue("antennaUpdatePeriod",
                 "Antenna orientation update period (default: 500 ms)",
                 antennaUpdatePeriod);
    cmd.Parse(argc, argv);

    // Parse mode
    LeoMode simMode;
    if (mode == "orbit")
    {
        simMode = LeoMode::Orbit;
    }
    else if (mode == "antenna")
    {
        simMode = LeoMode::Antenna;
    }
    else if (mode == "channel")
    {
        simMode = LeoMode::Channel;
    }
    else
    {
        NS_LOG_WARN("Unknown mode: " << mode << ". Defaulting to antenna.");
        simMode = LeoMode::Antenna;
    }

    // Validate that channel mode has all required options
    if (simMode == LeoMode::Channel && groundStationStr == "auto")
    {
        NS_LOG_WARN("Channel mode with groundStation=auto requires a valid orbitFile. "
                    "Setting groundStation to auto will place it beneath the first satellite.");
    }

    LeoOrbitNodeHelper orbit(orbitTimeStepResolution);

    // creates the satellite nodes and put them into a container
    NodeContainer satellites;
    if (!orbitFile.empty())
    {
        satellites = orbit.CreateNodesAndInstallMobility(orbitFile);
    }
    else
    {
        satellites = orbit.CreateNodesAndInstallMobility(LeoOrbitalShell(1180, 30, 1, 16));
    }

    // Parse ground station location
    Vector gsPos;
    if (groundStationStr == "auto")
    {
        // Place ground station beneath the first satellite (same lat/lon, altitude = 0)
        auto firstSatelliteMobility = satellites.Get(0)->GetObject<MobilityModel>();
        auto firstSatellitePositionGEO = GeographicPositions::CartesianToGeographicCoordinates(
            firstSatelliteMobility->GetPosition(),
            GeographicPositions::SPHERE);
        gsPos = Vector(firstSatellitePositionGEO.x, firstSatellitePositionGEO.y, 0);
    }
    else
    {
        gsPos = ParseGroundStation(groundStationStr);
    }

    // create node on the ground, create mobility, set position and aggregate mobility
    groundStation.node = CreateObject<Node>();
    Ptr<GeocentricEcefMobilityModel> groundNodeMobility =
        CreateObject<GeocentricEcefMobilityModel>();
    groundNodeMobility->SetGeographicPosition(gsPos);

    groundStation.posGEO = groundNodeMobility->GetGeographicPosition();
    groundStation.posECEF = groundNodeMobility->GetGeocentricPosition();
    NS_LOG_DEBUG(
        "Ground Node Geocentric Position (ECEF x[m]/y[m]/z[m]): " << groundStation.posECEF);
    NS_LOG_DEBUG(
        "Ground Node GetPosition() (ECEF x[m]/y[m]/z[m]): " << groundNodeMobility->GetPosition());
    NS_LOG_DEBUG("Satellite GetPosition() (ECEF x[m]/y[m]/z[m]): "
                 << satellites.Get(0)->GetObject<MobilityModel>()->GetPosition());
    NS_ASSERT_MSG(groundStation.posECEF == groundNodeMobility->GetPosition(),
                  "ECEF position mismatch. Ensure SetGeographicPosition arguments "
                  "are latitude/longitude/altitude, not cartesian coordinates.");

    groundStation.node->AggregateObject(groundNodeMobility);

    // Open trace file
    std::ofstream traceOs;
    if (!traceFile.empty())
    {
        traceOs.open(traceFile);
    }
    std::ostream* traceStream = (traceOs.is_open()) ? &traceOs : &std::cout;

    // Create propagation loss model for channel mode
    Ptr<ThreeGppPropagationLossModel> propLossModel = nullptr;
    if (simMode == LeoMode::Channel)
    {
        // Supported scenarios: NTN-DenseUrban, NTN-Urban, NTN-Suburban, NTN-Rural
        std::map<std::string, std::pair<TypeId, TypeId>> propTidCondTid{
            {"NTN-DenseUrban",
             {ThreeGppNTNDenseUrbanPropagationLossModel::GetTypeId(),
              ThreeGppNTNDenseUrbanChannelConditionModel::GetTypeId()}},
            {"NTN-Urban",
             {ThreeGppNTNUrbanPropagationLossModel::GetTypeId(),
              ThreeGppNTNUrbanChannelConditionModel::GetTypeId()}},
            {"NTN-Suburban",
             {ThreeGppNTNSuburbanPropagationLossModel::GetTypeId(),
              ThreeGppNTNSuburbanChannelConditionModel::GetTypeId()}},
            {"NTN-Rural",
             {ThreeGppNTNRuralPropagationLossModel::GetTypeId(),
              ThreeGppNTNRuralChannelConditionModel::GetTypeId()}}};

        if (propTidCondTid.find(scenario) == propTidCondTid.end())
        {
            NS_LOG_WARN("Unknown NTN scenario: " << scenario << ". Defaulting to NTN-Rural.");
            scenario = "NTN-Rural";
        }

        auto [propLossTid, chanCondTid] = propTidCondTid[scenario];
        ObjectFactory propagationLossModelFactory;
        propagationLossModelFactory.SetTypeId(propLossTid);
        propLossModel = propagationLossModelFactory.Create<ThreeGppPropagationLossModel>();
        propLossModel->SetAttribute("Frequency", DoubleValue(frequencyHz));
        propLossModel->SetAttribute("ShadowingEnabled", BooleanValue(true));

        ObjectFactory channelConditionModelFactory;
        channelConditionModelFactory.SetTypeId(chanCondTid);
        Ptr<ChannelConditionModel> condModel =
            channelConditionModelFactory.Create<ThreeGppChannelConditionModel>();
        propLossModel->SetChannelConditionModel(condModel);
    }

    LeoConstellationTracer tracer(groundStation.node,
                                  simMode,
                                  propLossModel,
                                  groundStation.posGEO,
                                  groundStation.posECEF,
                                  traceStream);

    // set up trace callback
    Config::ConnectWithoutContextFailSafe(
        "/NodeList/*/$ns3::MobilityModel/CourseChange",
        MakeCallback(&LeoConstellationTracer::CourseChange, &tracer));

    // Install antennas and set up antenna orientation for antenna/channel modes
    if (simMode == LeoMode::Antenna || simMode == LeoMode::Channel)
    {
        ObjectFactory antennaFactory;
        antennaFactory.SetTypeId("ns3::UniformPlanarArray");
        antennaFactory.Set("NumColumns", UintegerValue(1));
        antennaFactory.Set("NumRows", UintegerValue(1));

        ObjectFactory elementFactory;
        elementFactory.SetTypeId("ns3::IsotropicAntennaModel");
        elementFactory.Set("Gain", DoubleValue(satAntennaGainDb));

        Ptr<Node> node;
        for (uint32_t i = 0; i < satellites.GetN(); i++)
        {
            node = satellites.Get(i);
            antennaFactory.Set("AntennaElement",
                               PointerValue(elementFactory.Create<AntennaModel>()));

            node->AggregateObject(antennaFactory.Create<UniformPlanarArray>());

            tracer.UpdateAntennaOrientation(node, antennaUpdatePeriod);
        }
    }

    Simulator::Stop(duration);
    Simulator::Run();
    Simulator::Destroy();

    if (traceOs.is_open())
    {
        traceOs.close();
    }
}
