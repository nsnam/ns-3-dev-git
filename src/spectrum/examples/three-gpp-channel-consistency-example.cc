/*
 * Copyright (c) 2020, University of Padova, Dep. of Information Engineering, SIGNET lab
 * Copyright (c) 2026, Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

/*
 * @file
 * This example shows how to configure spatially consistent channel updates in a mobile environment.
 * This example is using the mobility-update mechanism of Procedure A described in
 * 3GPP TR 38.901, Sec. 7.6.3.2, to evolve an existing channel realization as the endpoints move.
 * Spatially consistent channel updates are enabled by setting the
 * ThreeGppChannelModel UpdatePeriod attribute to a non-zero value.
 *
 * The example is inspired by 3GPP TR 38.901 Sec. 7.8.5 Config. 2 scenario setup for the
 * evaluation of channel consistency with a moving user. The channel model is
 * configured as 3GPP UMi Street Canyon, while the channel condition model is set to be a
 * deterministic channel condition model based on the configured building obstacles, according to a
 * typical 3GPP urban setup. See the illustration in the following figure:
 *
 *   _______    _______
 *  |      |    |      |
 *  |      |    |      |
 *  |      |    |      |
 *  |______|    |______|
 *    < < < < <
 *   _______  ^  _______
 *  |      |  ^ |      |
 *  |      |  ^ |      |
 *  |      |  ^ |      |
 *  |______| UE |______|
 *           * gNB
 *
 * gNB is at the fixed position (shown by * in the previous illustration), and at the height of 10
 * meters. The user is moving away from gNB as illustrated in the figure with ^ and <. The link is
 * initially in LOS and later transitions to NLOS because of the configured buildings.
 *
 * The UE moves with a speed of 30 km/h, the frequency used is 30 GHz, and the channel update is 0.5
 * ms.
 *
 * The antenna arrays use fixed quasi-omnidirectional beamforming vectors throughout the simulation
 * to isolate the SNR variation caused by channel evolution from changes due to beamforming updates.
 * The antenna bearing angles are set to 90 degrees at the gNB and -90 degrees at the UE.
 *
 * This example generates the output file '3gpp-channel-consistency-output.txt'. Each row of this
 * output file is organized as follows:
 * Time[s] TxPosX[m] TxPosY[m] RxPosX[m] RxPosY[m] ChannelState SNR[dB] Pathloss[dB]
 * 1st_C_Power[dB] 2nd_C_Power[dB] 3rd_C_Power[dB]
 * 1st_C_Delay[s] 1st_C_Aoa[Degree] 1st_C_Zoa[Degree]
 * 2nd_C_Delay[s] 2nd_C_Aoa[Degree] 2nd_C_Zoa[Degree]
 * 3rd_C_Delay[s] 3rd_C_Aoa[Degree] 3rd_C_Zoa[Degree]
 * C1ComponentPower[dB] C2ComponentPower[dB] C3ComponentPower[dB]
 *
 * The cluster powers, delays, and angles are obtained from the 3GPP channel parameters.
 * The channel-component powers are computed from the corresponding components of the generated
 * channel matrix as the mean squared magnitude over all TX/RX antenna-element pairs. In LOS,
 * the first channel component therefore includes the deterministic LOS contribution coherently
 * combined with the strongest stochastic cluster by the channel model.
 *
 * The additional Python script three-gpp-channel-consistency-example.py reads the output and
 * generates static figures for the scenario, SNR, and channel-component and cluster metrics. The
 * --animations option also generates 3gpp-channel-consistency.gif and
 * channel_consistency_component_powers.gif, which represent the scenario, UE mobility, SNR,
 * and SNR changes, and component power and changes in component powers.
 */

#include "ns3/buildings-module.h"
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/spectrum-signal-parameters.h"
#include "ns3/three-gpp-channel-model.h"
#include "ns3/three-gpp-propagation-loss-model.h"
#include "ns3/three-gpp-spectrum-propagation-loss-model.h"
#include "ns3/uniform-planar-array.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <limits>

using namespace ns3;

/// the log component
NS_LOG_COMPONENT_DEFINE("ThreeGppChannelConsistencyExample");

/// the PropagationLossModel object
static Ptr<ThreeGppPropagationLossModel> g_propagationLossModel;
/// the SpectrumPropagationLossModel object
static Ptr<ThreeGppSpectrumPropagationLossModel> g_spectrumLossModel;
/// the ChannelConditionModel object
static Ptr<ChannelConditionModel> g_condModel;

/**
 * @brief A structure that holds the parameters for the ComputeSnr
 * function.
 */
struct ComputeSnrParams
{
    Ptr<MobilityModel> txMob;               //!< the tx mobility model
    Ptr<MobilityModel> rxMob;               //!< the rx mobility model
    Ptr<SpectrumSignalParameters> txParams; //!< the params of the tx signal
    double noiseFigure;                     //!< the noise figure in dB
    Ptr<PhasedArrayModel> txAntenna;        //!< the tx antenna array
    Ptr<PhasedArrayModel> rxAntenna;        //!< the rx antenna array
    Ptr<ThreeGppChannelModel> channelModel; //!< the channel model
};

/**
 * Metrics for the first three channel components and clusters.
 */
struct ChannelMetrics
{
    std::array<double, 3> componentPowerDb;
    //!< C1, C2, and C3 channel-component powers in dB,
    //!< computed from the corresponding channel-matrix
    //!< components
    std::array<double, 3> clusterPowerDb;
    //!< C1, C2, and C3 cluster powers in dB, taken from
    //!< the 3GPP channel parameters
    std::array<double, 3> delay; //!< C1, C2, and C3 delays in seconds
    std::array<double, 3> aoa;   //!< C1, C2, and C3 azimuth arrival angles in degrees
    std::array<double, 3> zoa;   //!< C1, C2, and C3 zenith arrival angles in degrees
};

/**
 * Convert a non-negative linear power ratio to dB.
 *
 * @param power the linear power ratio
 * @return the power ratio in dB
 */
static double
PowerRatioToDb(double power)
{
    return 10.0 * std::log10(std::max(power, std::numeric_limits<double>::min()));
}

/**
 * Get metrics for the first three channel components and clusters.
 *
 * The channel-component power is computed as the mean squared magnitude
 * of the coefficients in each channel-matrix component (squared Frobenius
 * norm normalized by the number of TX/RX antenna-element pairs). In LOS,
 * component 0 therefore includes the deterministic LOS contribution
 * coherently combined with the strongest stochastic cluster by the
 * channel model.
 *
 * The cluster power, delay, AoA, and ZoA are taken directly from the
 * channel parameters. These quantities retain their cluster-level
 * definition and are used for the cluster-based spatial-consistency
 * metrics.
 *
 * @param params the objects used to obtain the channel and channel parameters
 * @return channel-component and cluster metrics for the first three entries
 */
static ChannelMetrics
GetChannelMetrics(const ComputeSnrParams& params)
{
    Ptr<const MatrixBasedChannelModel::ChannelMatrix> channelMatrix =
        params.channelModel->GetChannel(params.txMob,
                                        params.rxMob,
                                        params.txAntenna,
                                        params.rxAntenna);

    Ptr<const MatrixBasedChannelModel::ChannelParams> baseParams =
        params.channelModel->GetParams(params.txMob, params.rxMob);

    Ptr<const ThreeGppChannelModel::ThreeGppChannelParams> channelParams =
        DynamicCast<const ThreeGppChannelModel::ThreeGppChannelParams>(baseParams);

    NS_ABORT_MSG_IF(channelMatrix == nullptr, "The channel matrix is not available");

    NS_ABORT_MSG_IF(channelParams == nullptr, "The 3GPP channel parameters are not available");

    NS_ABORT_MSG_IF(channelMatrix->m_channel.GetNumPages() < 3,
                    "The channel matrix has fewer than three components");

    NS_ABORT_MSG_IF(channelParams->m_clusterPower.size() < 3,
                    "The channel has fewer than three cluster powers");

    NS_ABORT_MSG_IF(channelParams->m_delay.size() < 3,
                    "The channel has fewer than three cluster delays");

    NS_ABORT_MSG_IF(channelParams->m_angle.size() <= MatrixBasedChannelModel::ZOA_INDEX ||
                        channelParams->m_angle[MatrixBasedChannelModel::AOA_INDEX].size() < 3 ||
                        channelParams->m_angle[MatrixBasedChannelModel::ZOA_INDEX].size() < 3,
                    "The channel has fewer than three cluster arrival angles");

    const size_t numRows = channelMatrix->m_channel.GetNumRows();
    const size_t numCols = channelMatrix->m_channel.GetNumCols();
    const auto numElements = static_cast<double>(numRows * numCols);

    std::array<double, 3> componentPower{0.0, 0.0, 0.0};

    for (size_t componentIndex = 0; componentIndex < componentPower.size(); ++componentIndex)
    {
        for (size_t row = 0; row < numRows; ++row)
        {
            for (size_t col = 0; col < numCols; ++col)
            {
                componentPower[componentIndex] +=
                    std::norm(channelMatrix->m_channel(row, col, componentIndex));
            }
        }

        componentPower[componentIndex] /= numElements;
    }

    return {{PowerRatioToDb(componentPower[0]),
             PowerRatioToDb(componentPower[1]),
             PowerRatioToDb(componentPower[2])},

            {PowerRatioToDb(channelParams->m_clusterPower[0]),
             PowerRatioToDb(channelParams->m_clusterPower[1]),
             PowerRatioToDb(channelParams->m_clusterPower[2])},

            {channelParams->m_delay[0], channelParams->m_delay[1], channelParams->m_delay[2]},

            {channelParams->m_angle[MatrixBasedChannelModel::AOA_INDEX][0],
             channelParams->m_angle[MatrixBasedChannelModel::AOA_INDEX][1],
             channelParams->m_angle[MatrixBasedChannelModel::AOA_INDEX][2]},

            {channelParams->m_angle[MatrixBasedChannelModel::ZOA_INDEX][0],
             channelParams->m_angle[MatrixBasedChannelModel::ZOA_INDEX][1],
             channelParams->m_angle[MatrixBasedChannelModel::ZOA_INDEX][2]}};
}

/**
 * Set QuasiOmni beamforming vector to the antenna array
 * @param antenna the antenna array to which will be set quasi omni beamforming vector
 */
static void
CreateQuasiOmniBf(Ptr<PhasedArrayModel> antenna)
{
    PhasedArrayModel::ComplexVector antennaWeights;

    auto antennaRows = antenna->GetNumRows();
    auto antennaColumns = antenna->GetNumColumns();
    auto numElemsPerPort = antenna->GetNumElemsPerPort();

    double power = 1 / std::sqrt(numElemsPerPort);
    size_t numPolarizations = antenna->IsDualPol() ? 2 : 1;

    PhasedArrayModel::ComplexVector omni(antennaRows * antennaColumns * numPolarizations);
    uint16_t bfIndex = 0;
    for (size_t pol = 0; pol < numPolarizations; pol++)
    {
        for (uint32_t ind = 0; ind < antennaRows; ind++)
        {
            std::complex<double> c = 0.0;
            if (antennaRows % 2 == 0)
            {
                c = std::exp(std::complex<double>(0, M_PI * ind * ind / antennaRows));
            }
            else
            {
                c = std::exp(std::complex<double>(0, M_PI * ind * (ind + 1) / antennaRows));
            }
            for (uint32_t ind2 = 0; ind2 < antennaColumns; ind2++)
            {
                std::complex<double> d = 0.0;
                if (antennaColumns % 2 == 0)
                {
                    d = std::exp(std::complex<double>(0, M_PI * ind2 * ind2 / antennaColumns));
                }
                else
                {
                    d = std::exp(
                        std::complex<double>(0, M_PI * ind2 * (ind2 + 1) / antennaColumns));
                }
                omni[bfIndex] = (c * d * power);
                bfIndex++;
            }
        }
    }

    antenna->SetBeamformingVector(omni);
}

/**
 * Computes the SNR
 * @param params A structure that holds a bunch of parameters needed by ComputeSnr function to
 * calculate the average SNR
 */
static void
ComputeSnr(const ComputeSnrParams& params)
{
    // check the channel condition
    Ptr<ChannelCondition> cond = g_condModel->GetChannelCondition(params.txMob, params.rxMob);
    // apply the pathloss
    double propagationGainDb = g_propagationLossModel->CalcRxPower(0, params.txMob, params.rxMob);
    double propagationGainLinear = std::pow(10.0, (propagationGainDb) / 10.0);
    *(params.txParams->psd) *= propagationGainLinear;
    // apply the fast fading and the beamforming gain
    auto rxParams =
        g_spectrumLossModel->CalcRxPowerSpectralDensity(params.txParams,
                                                        params.txMob,
                                                        params.rxMob,
                                                        params.txAntenna,
                                                        params.rxAntenna,
                                                        params.txAntenna->GetBeamformingVector(),
                                                        params.rxAntenna->GetBeamformingVector());
    Ptr<SpectrumValue> rxPsd = rxParams->psd;
    const ChannelMetrics metrics = GetChannelMetrics(params);
    // create the noise psd
    // taken from lte-spectrum-value-helper
    const double kTDbmHz = -174.0; // dBm/Hz
    double kT_W_Hz = std::pow(10.0, (kTDbmHz - 30) / 10.0);
    double noiseFigureLinear = std::pow(10.0, params.noiseFigure / 10.0);
    double noisePowerSpectralDensity = kT_W_Hz * noiseFigureLinear;
    Ptr<SpectrumValue> noisePsd = Create<SpectrumValue>(params.txParams->psd->GetSpectrumModel());
    (*noisePsd) = noisePowerSpectralDensity;
    // print the SNR and pathloss values to the output file
    std::ofstream f;
    f.open("3gpp-channel-consistency-output.txt", std::ios::out | std::ios::app);
    f << Simulator::Now().GetSeconds() << " " // time [s]
      << params.txMob->GetPosition().x << " " << params.txMob->GetPosition().y << " "
      << params.rxMob->GetPosition().x << " " << params.rxMob->GetPosition().y << " "
      << cond->GetLosCondition() << " "                       // channel state
      << 10 * std::log10(Sum(*rxPsd) / Sum(*noisePsd)) << " " // SNR [dB]
      << -propagationGainDb << " "                            // pathloss [dB]
      << metrics.clusterPowerDb[0] << " "                     // C1 cluster power [dB]
      << metrics.clusterPowerDb[1] << " "                     // C2 cluster power [dB]
      << metrics.clusterPowerDb[2] << " "                     // C3 cluster power [dB]
      << metrics.delay[0] << " " << metrics.aoa[0] << " " << metrics.zoa[0] << " "
      << metrics.delay[1] << " " << metrics.aoa[1] << " " << metrics.zoa[1] << " "
      << metrics.delay[2] << " " << metrics.aoa[2] << " " << metrics.zoa[2] << " "
      << metrics.componentPowerDb[0] << " " << metrics.componentPowerDb[1] << " "
      << metrics.componentPowerDb[2] << std::endl;

    f.close();
}

/**
 * Generates a GNU-plottable file representing the buildings deployed in the
 * scenario
 * @param filename the name of the output file
 */
void
PrintGnuplottableBuildingListToFile(std::string filename)
{
    std::ofstream outFile;
    outFile.open(filename, std::ios_base::out | std::ios_base::trunc);
    if (!outFile.is_open())
    {
        NS_LOG_ERROR("Can't open file " << filename);
        return;
    }
    for (auto it = BuildingList::Begin(); it != BuildingList::End(); ++it)
    {
        Box box = (*it)->GetBoundaries();
        outFile << box.xMin << " " << box.yMin << " " << box.xMax << " " << box.yMax << std::endl;
    }
}

int
main(int argc, char* argv[])
{
    double frequency = 30e9;               // operating frequency in Hz
    double txPowerDbm = 35.0;              // tx power in dBm
    double noiseFigure = 9.0;              // noise figure in dB
    Time updatePeriod = MicroSeconds(500); // time resolution and the channel update time
    double speed = 30.0 / 3.6;             // speed of the mobile node in the scenario [m/s]
    double rbWidthHz = 720e3;              // RB width in Hz
    uint32_t numRb = 275;                  // number of resource blocks
    double gnbHeight = 10;                 // the height of the gNB
    double ueHeight =
        1.5; // the height of the UE (a bit higher than 1m because of the BP distance calculation)

    // fix random parameters
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    uint64_t stream = 10000;

    // create the nodes
    NodeContainer nodes;
    nodes.Create(2);

    // create the tx and rx devices
    Ptr<SimpleNetDevice> txDev = CreateObject<SimpleNetDevice>();
    Ptr<SimpleNetDevice> rxDev = CreateObject<SimpleNetDevice>();

    // associate the nodes and the devices
    nodes.Get(0)->AddDevice(txDev);
    txDev->SetNode(nodes.Get(0));
    nodes.Get(1)->AddDevice(rxDev);
    rxDev->SetNode(nodes.Get(1));

    // create the antenna objects and set their dimensions
    Ptr<PhasedArrayModel> txAntenna =
        CreateObjectWithAttributes<UniformPlanarArray>("NumColumns",
                                                       UintegerValue(2),
                                                       "NumRows",
                                                       UintegerValue(2),
                                                       "BearingAngle",
                                                       DoubleValue(M_PI / 2));
    Ptr<PhasedArrayModel> rxAntenna =
        CreateObjectWithAttributes<UniformPlanarArray>("NumColumns",
                                                       UintegerValue(2),
                                                       "NumRows",
                                                       UintegerValue(2),
                                                       "BearingAngle",
                                                       DoubleValue(-M_PI / 2));

    Ptr<MobilityModel> txMob;
    Ptr<MobilityModel> rxMob;

    // create a grid of buildings
    double buildingSizeX = (250 - 3.5 * 2 - 3) / 2; // m
    double buildingSizeY = (433 - 3.5 * 2 - 3) / 4; // m
    double streetWidth = 20;                        // m
    double buildingHeight = 10;                     // m
    uint32_t numBuildingsX = 2;
    uint32_t numBuildingsY = 2;
    double maxAxisX = (buildingSizeX + streetWidth) * numBuildingsX;
    double maxAxisY = (buildingSizeY + streetWidth) * numBuildingsY;

    std::vector<Ptr<Building>> buildingVector;
    for (uint32_t buildingIdX = 0; buildingIdX < numBuildingsX; ++buildingIdX)
    {
        for (uint32_t buildingIdY = 0; buildingIdY < numBuildingsY; ++buildingIdY)
        {
            Ptr<Building> building;
            building = CreateObject<Building>();

            building->SetBoundaries(Box(buildingIdX * (buildingSizeX + streetWidth),
                                        buildingIdX * (buildingSizeX + streetWidth) + buildingSizeX,
                                        buildingIdY * (buildingSizeY + streetWidth),
                                        buildingIdY * (buildingSizeY + streetWidth) + buildingSizeY,
                                        0.0,
                                        buildingHeight));
            building->SetNRoomsX(1);
            building->SetNRoomsY(1);
            building->SetNFloors(1);
            buildingVector.push_back(building);
        }
    }

    // set the mobility models of the TX(gNB) and RX(mobile UE)
    txMob = CreateObject<ConstantPositionMobilityModel>();
    txMob->SetPosition(Vector(maxAxisX / 2 - streetWidth / 2, -20, gnbHeight));
    nodes.Get(0)->AggregateObject(txMob);

    const Vector firstPosition(maxAxisX / 2 - streetWidth / 2, -10, ueHeight);
    const Vector secondPosition(maxAxisX / 2 - streetWidth / 2,
                                maxAxisY / 2 - streetWidth / 2,
                                ueHeight);
    const Vector thirdPosition(0.0, maxAxisY / 2 - streetWidth / 2, ueHeight);

    Time nextWaypoint;
    rxMob = CreateObject<WaypointMobilityModel>();
    rxMob->GetObject<WaypointMobilityModel>()->AddWaypoint(Waypoint(nextWaypoint, firstPosition));
    nextWaypoint += Seconds(CalculateDistance(firstPosition, secondPosition) / speed);
    rxMob->GetObject<WaypointMobilityModel>()->AddWaypoint(Waypoint(nextWaypoint, secondPosition));
    nextWaypoint += Seconds(CalculateDistance(secondPosition, thirdPosition) / speed);
    rxMob->GetObject<WaypointMobilityModel>()->AddWaypoint(Waypoint(nextWaypoint, thirdPosition));
    const Time simTime = nextWaypoint;
    nodes.Get(1)->AggregateObject(rxMob);

    // create the channel condition model
    g_condModel = CreateObject<BuildingsChannelConditionModel>();

    // create the propagation loss model
    g_propagationLossModel = CreateObject<ThreeGppUmiStreetCanyonPropagationLossModel>();

    g_propagationLossModel->SetAttribute("Frequency", DoubleValue(frequency));
    g_propagationLossModel->SetAttribute("ShadowingEnabled", BooleanValue(true));
    g_propagationLossModel->SetAttribute("ChannelConditionModel", PointerValue(g_condModel));
    stream += g_propagationLossModel->AssignStreams(stream);

    // create the channel model
    Ptr<ThreeGppChannelModel> channelModel = CreateObject<ThreeGppChannelModel>();
    channelModel->SetAttribute("Scenario", StringValue("UMi-StreetCanyon"));
    channelModel->SetAttribute("Frequency", DoubleValue(frequency));
    channelModel->SetAttribute("ChannelConditionModel", PointerValue(g_condModel));
    channelModel->SetAttribute("UpdatePeriod", TimeValue(updatePeriod));
    stream += channelModel->AssignStreams(stream);

    // create the spectrum propagation loss model
    g_spectrumLossModel = CreateObjectWithAttributes<ThreeGppSpectrumPropagationLossModel>(
        "ChannelModel",
        PointerValue(channelModel));

    BuildingsHelper::Install(nodes);

    // create the tx power spectral density
    Bands rbs;
    double freqSubBand = frequency;
    for (uint32_t n = 0; n < numRb; ++n)
    {
        BandInfo rb;
        rb.fl = freqSubBand;
        freqSubBand += rbWidthHz / 2;
        rb.fc = freqSubBand;
        freqSubBand += rbWidthHz / 2;
        rb.fh = freqSubBand;
        rbs.push_back(rb);
    }
    Ptr<SpectrumModel> spectrumModel = Create<SpectrumModel>(rbs);
    Ptr<SpectrumValue> txPsd = Create<SpectrumValue>(spectrumModel);
    Ptr<SpectrumSignalParameters> txParams = Create<SpectrumSignalParameters>();
    double txPowerW = std::pow(10., (txPowerDbm - 30) / 10);
    double txPowDens = (txPowerW / (numRb * rbWidthHz));
    (*txPsd) = txPowDens;
    txParams->psd = txPsd->Copy();

    CreateQuasiOmniBf(txAntenna);
    CreateQuasiOmniBf(rxAntenna);

    for (int i = 0; i < simTime / updatePeriod; i++)
    {
        ComputeSnrParams
            params{txMob, rxMob, txParams->Copy(), noiseFigure, txAntenna, rxAntenna, channelModel};
        Simulator::Schedule(updatePeriod * i, &ComputeSnr, params);
    }
    ComputeSnrParams finalParams{
        txMob,
        rxMob,
        txParams->Copy(),
        noiseFigure,
        txAntenna,
        rxAntenna,
        channelModel,
    };
    Simulator::Schedule(simTime, &ComputeSnr, finalParams);
    Simulator::Stop(simTime);

    // initialize the output file
    std::ofstream f;
    f.open("3gpp-channel-consistency-output.txt", std::ios::out);
    f << "Time[s] TxPosX[m] TxPosY[m] RxPosX[m] RxPosY[m] ChannelState SNR[dB] Pathloss[dB] "
         "1st_C_Power[dB] 2nd_C_Power[dB] 3rd_C_Power[dB] "
         "1st_C_Delay[s] 1st_C_Aoa[Degree] 1st_C_Zoa[Degree] "
         "2nd_C_Delay[s] 2nd_C_Aoa[Degree] 2nd_C_Zoa[Degree] "
         "3rd_C_Delay[s] 3rd_C_Aoa[Degree] 3rd_C_Zoa[Degree] "
         "C1ComponentPower[dB] C2ComponentPower[dB] C3ComponentPower[dB]"
      << std::endl;
    f.close();

    // print the list of buildings to file
    PrintGnuplottableBuildingListToFile("3gpp-channel-consistency-buildings.txt");

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
