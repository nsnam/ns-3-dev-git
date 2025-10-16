/*
 * Copyright (c) 2020, University of Padova, Dep. of Information Engineering, SIGNET lab
 * Copyright (c) 2026, Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

/*
 * @file
 * This example shows how to configure the channel consistent updates in a mobile environment.
 * This example is using the channel consistency feature of the 3GPP channel model described in
 * 3GPP TR 38.901, Sec. 7.6.3. This feature enables the generation of spatially
 * correlated channel realizations for simulations with mobility. This feature is
 * automatically enabled by setting up the UpdatePeriod attribute of ThreeGppChannelModel to be
 * different than 0. The example is the following 38.901 7.8.5 scenario setup Config2 parameters
 * for channel consistency evaluation of mobile users (Procedure A). The channel model is
 * 3GPP UMi Street Canyon, while the channel condition model is set to be a deterministic
 * channel condition model based on the buildings obstacles that are configured in the
 * example, according to a typical 3GPP urban setup. See the illustration in the following figure:
 *
 *   _______    _______
 *  |      |    |      |
 *  |      |    |      |
 *  |      |    |      |
 *  |      |    |      |
 *  |      |    |      |
 *  |______|    |______|
 *    < < < < <
 *   _______  ^  _______
 *  |      |  ^ |      |
 *  |      |  ^ |      |
 *  |      |  ^ |      |
 *  |      |  ^ |      |
 *  |      | UE |      |
 *  |______|    |______|
 *           * gNB
 *
 * gNB is at the fixed position (shown by * in the previous illustration), and at the height of 10
 * meters. The user is moving away from gNB as illustrated in the figure with ^ and <, and first, it
 * is in the LOS state, and later, after turning into a perpendicular street, it is in the NLOS
 * state.
 *
 * The UE moves with a speed of 30km/h, the frequency used is 30GHz, and the channel update is
 * 0.5ms, configured based on the channel coherence time.
 *
 * The antenna array is set to quasiomni, and it is fixed during the whole simulation to eliminate
 * possible effects of the impact of the beamforming update while the user is moving. The example
 * tries to focus only on the changes in the SNR caused by the channel updates as the user moves.
 * The bearing angels are set to 90 at gNB and -90 at UE.
 *
 * This example generates the output file '3gpp-channel-consistency-output.txt'. Each row of this
 * output file is organized as follows:
 * Time[s] TxPosX[m] TxPosY[m] RxPosX[m] RxPosY[m] ChannelState SNR[dB] Pathloss[dB]
 * An additional python script three-gpp-channel-consistency-example.py reads this
 * output file and generates two figures:
 * (i) 3gpp-channel-consistency.gif, a GIF representing the simulation scenario and the UE mobility;
 * it includes the "zoomed view" of the SNR, and also it includes the variation of the
 * channel updates.
 * (ii) 3gpp-channel-consistency-snr.png, which represents the behavior of the SNR over
 * the time, the absolute value and the varying rate over time.
 *
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

#include <fstream>

using namespace ns3;

/// the log component
NS_LOG_COMPONENT_DEFINE("ThreeGppChannelConsistencyExample");

/// the PropagationLossModel object
static Ptr<ThreeGppPropagationLossModel> m_propagationLossModel;
/// the SpectrumPropagationLossModel object
static Ptr<ThreeGppSpectrumPropagationLossModel> m_spectrumLossModel;
/// the ChannelConditionModel object
static Ptr<ChannelConditionModel> m_condModel;

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
};

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

    double power = 1 / sqrt(numElemsPerPort);
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
                c = exp(std::complex<double>(0, M_PI * ind * ind / antennaRows));
            }
            else
            {
                c = exp(std::complex<double>(0, M_PI * ind * (ind + 1) / antennaRows));
            }
            for (uint32_t ind2 = 0; ind2 < antennaColumns; ind2++)
            {
                std::complex<double> d = 0.0;
                if (antennaColumns % 2 == 0)
                {
                    d = exp(std::complex<double>(0, M_PI * ind2 * ind2 / antennaColumns));
                }
                else
                {
                    d = exp(std::complex<double>(0, M_PI * ind2 * (ind2 + 1) / antennaColumns));
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
    Ptr<ChannelCondition> cond = m_condModel->GetChannelCondition(params.txMob, params.rxMob);
    // apply the pathloss
    double propagationGainDb = m_propagationLossModel->CalcRxPower(0, params.txMob, params.rxMob);
    double propagationGainLinear = std::pow(10.0, (propagationGainDb) / 10.0);
    *(params.txParams->psd) *= propagationGainLinear;
    // apply the fast fading and the beamforming gain
    auto rxParams = m_spectrumLossModel->CalcRxPowerSpectralDensity(params.txParams,
                                                                    params.txMob,
                                                                    params.rxMob,
                                                                    params.txAntenna,
                                                                    params.rxAntenna);
    Ptr<SpectrumValue> rxPsd = rxParams->psd;
    // create the noise psd
    // taken from lte-spectrum-value-helper
    const double kT_dBm_Hz = -174.0; // dBm/Hz
    double kT_W_Hz = std::pow(10.0, (kT_dBm_Hz - 30) / 10.0);
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
      << cond->GetLosCondition() << " "                  // channel state
      << 10 * log10(Sum(*rxPsd) / Sum(*noisePsd)) << " " // SNR [dB]
      << -propagationGainDb << std::endl;                // pathloss [dB]
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
    double frequency = 30e9;          // operating frequency in Hz
    double txPow_dbm = 35.0;          // tx power in dBm
    double noiseFigure = 9.0;         // noise figure in dB
    Time simTime = Seconds(40);       // simulation time
    Time timeRes = MicroSeconds(500); // time resolution and the channel update time
    double speed = 8.33;              // speed of the mobile node in the scenario [m/s]
    double rbWidthHz = 720e3;         // RB width in Hz
    uint32_t numRb = 275;             // number of resource blocks
    double gNBHeight = 10;            // the height of the gNB
    double ueHeight =
        1.1; // the height of the UE (a bit higher than 1m because of the BP distance calculation)

    // fix random parameters
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    uint64_t stream = 10e3;

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
    double buildingSizeX = 250 - 3.5 * 2 - 3; // m
    double buildingSizeY = 433 - 3.5 * 2 - 3; // m
    double streetWidth = 20;                  // m
    double buildingHeight = 10;               // m
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
    txMob->SetPosition(Vector(maxAxisX / 2 - streetWidth / 2, -20, gNBHeight));
    nodes.Get(0)->AggregateObject(txMob);

    Time nextWaypoint;
    rxMob = CreateObject<WaypointMobilityModel>();
    rxMob->GetObject<WaypointMobilityModel>()->AddWaypoint(
        Waypoint(nextWaypoint, Vector(maxAxisX / 2 - streetWidth / 2, maxAxisY / 4, ueHeight)));
    nextWaypoint += Seconds((maxAxisY - maxAxisY / 2 - streetWidth) / 2 / speed);
    rxMob->GetObject<WaypointMobilityModel>()->AddWaypoint(
        Waypoint(nextWaypoint,
                 Vector(maxAxisX / 2 - streetWidth / 2, maxAxisY / 2 - streetWidth / 2, ueHeight)));
    nextWaypoint += Seconds((maxAxisX - streetWidth) / 2 / speed);
    rxMob->GetObject<WaypointMobilityModel>()->AddWaypoint(
        Waypoint(nextWaypoint, Vector(0.0, maxAxisY / 2 - streetWidth / 2, ueHeight)));
    nextWaypoint = Seconds(0);
    nodes.Get(1)->AggregateObject(rxMob);

    // create the channel condition model
    m_condModel = CreateObject<BuildingsChannelConditionModel>();

    // create the propagation loss model
    m_propagationLossModel = CreateObject<ThreeGppUmiStreetCanyonPropagationLossModel>();

    m_propagationLossModel->SetAttribute("Frequency", DoubleValue(frequency));
    m_propagationLossModel->SetAttribute("ShadowingEnabled", BooleanValue(true));
    m_propagationLossModel->SetAttribute("ChannelConditionModel", PointerValue(m_condModel));
    stream += m_propagationLossModel->AssignStreams(stream);

    // create the channel model
    Ptr<ThreeGppChannelModel> channelModel = CreateObject<ThreeGppChannelModel>();
    channelModel->SetAttribute("Scenario", StringValue("UMi-StreetCanyon"));
    channelModel->SetAttribute("Frequency", DoubleValue(frequency));
    channelModel->SetAttribute("ChannelConditionModel", PointerValue(m_condModel));
    channelModel->SetAttribute("UpdatePeriod", TimeValue(timeRes));
    stream += channelModel->AssignStreams(stream);

    // create the spectrum propagation loss model
    m_spectrumLossModel = CreateObjectWithAttributes<ThreeGppSpectrumPropagationLossModel>(
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
    double txPow_w = std::pow(10., (txPow_dbm - 30) / 10);
    double txPowDens = (txPow_w / (numRb * rbWidthHz));
    (*txPsd) = txPowDens;
    txParams->psd = txPsd->Copy();

    CreateQuasiOmniBf(txAntenna);
    CreateQuasiOmniBf(rxAntenna);

    for (int i = 0; i < simTime / timeRes; i++)
    {
        ComputeSnrParams params{txMob, rxMob, txParams->Copy(), noiseFigure, txAntenna, rxAntenna};
        Simulator::Schedule(timeRes * i, &ComputeSnr, params);
    }

    // initialize the output file
    std::ofstream f;
    f.open("3gpp-channel-consistency-output.txt", std::ios::out);
    f << "Time[s] TxPosX[m] TxPosY[m] RxPosX[m] RxPosY[m] ChannelState SNR[dB] Pathloss[dB]"
      << std::endl;
    f.close();

    // print the list of buildings to file
    PrintGnuplottableBuildingListToFile("3gpp-channel-consistency-buildings.txt");

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
