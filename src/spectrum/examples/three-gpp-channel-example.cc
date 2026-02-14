/*
 * Copyright (c) 2019 SIGNET Lab, Department of Information Engineering,
 * University of Padova
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

/**
 * @file
 * This example shows how to configure the 3GPP channel model classes to
 * compute the SNR between two nodes.
 * The simulation involves two static nodes which are placed at a certain
 * distance from each other and communicate through a wireless channel at
 * 2 GHz with a bandwidth of 18 MHz. The default propagation environment is
 * 3D-urban macro (UMa), and it can be configured changing the value of the
 * string "scenario".
 * Each node hosts has an antenna array with 4 antenna elements.
 *
 * The example writes its main results to the file ``snr-trace.txt``.
 * Each row contains: (i) the simulation time (s), (ii) the SNR obtained with
 * steering vectors (``DoBeamforming``), (iii) the SNR obtained with matched-filter
 * combining, and (iv) the propagation gain.
 */

#include "ns3/channel-condition-model.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/constant-velocity-mobility-model.h"
#include "ns3/core-module.h"
#include "ns3/lte-spectrum-value-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/spectrum-signal-parameters.h"
#include "ns3/three-gpp-channel-model.h"
#include "ns3/three-gpp-propagation-loss-model.h"
#include "ns3/three-gpp-spectrum-propagation-loss-model.h"
#include "ns3/uniform-planar-array.h"

#include <fstream>

NS_LOG_COMPONENT_DEFINE("ThreeGppChannelExample");

using namespace ns3;

static Ptr<ThreeGppPropagationLossModel>
    m_propagationLossModel; //!< the PropagationLossModel object
static Ptr<ThreeGppSpectrumPropagationLossModel>
    m_spectrumLossModel; //!< the SpectrumPropagationLossModel object

/**
 * @brief A structure that holds the parameters for the
 * ComputeSnr function. In this way the problem with the limited
 * number of parameters of method Schedule is avoided.
 */
struct ComputeSnrParams
{
    Ptr<MobilityModel> txMob;        //!< the tx mobility model
    Ptr<MobilityModel> rxMob;        //!< the rx mobility model
    double txPow;                    //!< the tx power in dBm
    double noiseFigure;              //!< the noise figure in dB
    Ptr<PhasedArrayModel> txAntenna; //!< the tx antenna array
    Ptr<PhasedArrayModel> rxAntenna; //!< the rx antenna array
};

/**
 * Perform a simple (DFT/steering) beamforming toward the other node.
 *
 * This function builds a per-element complex weight vector based on the geometric
 * direction between two nodes and the antenna element locations returned by
 * `PhasedArrayModel::GetElementLocation()`. The weights are unit-norm (equal
 * power across elements) and have the form:
 *
 *   w[i] = exp(j * phase_i) / sqrt(N),
 *   phase_i = sign * 2*pi * ( u · r_i )
 *
 * where `u` is the unit direction corresponding to the azimuth/inclination
 * angles of the line-of-sight vector between the nodes, `r_i` is the location of
 * element `i` in the array coordinate system, and `N` is the number of antenna
 * elements.
 *
 * Sign convention and how to use this function:
 *
 * The 3GPP channel coefficients are constructed using spatial phase terms of the
 * form exp(+j*2*pi*(u·r)) (see `ThreeGppChannelModel::GetNewChannel()`), i.e., the
 * array *response/steering vector* toward a direction is proportional to:
 *
 *   a(theta)[i] = exp(+j*2*pi*(u·r_i)).
 *
 * In standard array processing, a transmit precoder that steers energy toward
 * that direction uses the complex conjugate of the response, therefore
 * sign = -1  (phase = -2*pi*(u·r_i)).
 *
 * For receive combining, the effective scalar channel is computed using the
 * Hermitian inner product:
 *
 *   h_eff = w_rx^H * H * w_tx .
 *
 * To obtain matched-filter (maximum-ratio) receive combining toward the same
 * direction, the stored receive weight vector should be proportional to the
 * response a(theta), so that `w_rx^H` applies the conjugation at combining time
 * sign = +1  (phase = +2*pi*(u·r_i)).
 *
 * Therefore, when this function is used to set beamforming vectors explicitly in
 * this example:
 *   - Use `sign = -1` when generating TX weights (precoding).
 *   - Use `sign = +1` when generating RX weights (so that Hermitian combining
 *     applies the conjugation).
 *
 * @param txMob The mobility model of the node for which the weights are generated.
 * @param thisAntenna The antenna array on which the beamforming vector is set.
 * @param rxMob The mobility model of the peer node toward/from which the beam is steered.
 * @param sign Phase progression sign: typically -1 for TX weights and +1 for RX weights.
 */
static void
DoBeamforming(Ptr<MobilityModel> txMob,
              Ptr<PhasedArrayModel> thisAntenna,
              Ptr<MobilityModel> rxMob,
              double sign)
{
    // retrieve the position of the two nodes
    Vector aPos = txMob->GetPosition();
    Vector bPos = rxMob->GetPosition();

    // compute the azimuth and the elevation angles
    Angles completeAngle(bPos, aPos);
    double hAngleRadian = completeAngle.GetAzimuth();

    double vAngleRadian = completeAngle.GetInclination(); // the elevation angle

    // retrieve the number of antenna elements and resize the vector
    uint64_t totNoArrayElements = thisAntenna->GetNumElems();
    PhasedArrayModel::ComplexVector antennaWeights(totNoArrayElements);

    // the total power is divided equally among the antenna elements
    double power = 1.0 / sqrt(totNoArrayElements);

    // compute the antenna weights
    const double sinVAngleRadian = sin(vAngleRadian);
    const double cosVAngleRadian = cos(vAngleRadian);
    const double sinHAngleRadian = sin(hAngleRadian);
    const double cosHAngleRadian = cos(hAngleRadian);

    for (uint64_t ind = 0; ind < totNoArrayElements; ind++)
    {
        Vector loc = thisAntenna->GetElementLocation(ind);
        double phase = sign * 2 * M_PI *
                       (sinVAngleRadian * cosHAngleRadian * loc.x +
                        sinVAngleRadian * sinHAngleRadian * loc.y + cosVAngleRadian * loc.z);
        antennaWeights[ind] = exp(std::complex<double>(0, phase)) * power;
    }

    // store the antenna weights
    thisAntenna->SetBeamformingVector(antennaWeights);
}

/**
 * Build the RX matched-filter (maximum-ratio) combining vector by aggregating
 * the contributions of all clusters and normalizing the result:
 * \f[
 * \mathbf{g} = \sum_{c} \mathbf{H}_{c}\mathbf{w}_{\rm tx}, \qquad
 * \mathbf{w}_{\rm rx} = \frac{\mathbf{g}}{\|\mathbf{g}\|}
 * \f]
 *
 * The per-cluster channel matrix \f$\mathbf{H}_{c}\f$ is obtained from
 * ``params->m_channel(u, s, c)`` (u: RX element index, s: TX element index, c: cluster index).
 *
 * @param params Channel parameters containing the per-cluster channel matrix.
 * @param txAntenna TX phased-array model.
 * @param rxAntenna RX phased-array model.
 * @param wTx Unit-norm TX beamforming/precoding vector (size must match
 * ``txAntenna->GetNumElems()``).
 *
 * @return The RX combining vector (size ``rxAntenna->GetNumElems()``), normalized to unit norm
 *         (unless the channel is all-zero, in which case a zero vector is returned).
 */

static ns3::PhasedArrayModel::ComplexVector
BuildRxMatchedFilterOverAllClusters(
    ns3::Ptr<const ns3::MatrixBasedChannelModel::ChannelMatrix> params,
    ns3::Ptr<const ns3::PhasedArrayModel> txAntenna,
    ns3::Ptr<const ns3::PhasedArrayModel> rxAntenna,
    const ns3::PhasedArrayModel::ComplexVector& wTx)
{
    using C = std::complex<double>;

    const uint64_t nTx = txAntenna->GetNumElems();
    const uint64_t nRx = rxAntenna->GetNumElems();
    const size_t numClusters = params->m_channel.GetNumPages();

    NS_ASSERT_MSG(wTx.GetSize() == nTx, "wTx size does not match TX antenna elements");
    NS_ASSERT_MSG(params->m_channel.GetNumRows() == nRx, "Channel rows != #RX elements");
    NS_ASSERT_MSG(params->m_channel.GetNumCols() == nTx, "Channel cols != #TX elements");

    ns3::PhasedArrayModel::ComplexVector g(nRx);

    // g = sum_c (H_c * wTx)
    for (size_t c = 0; c < numClusters; ++c)
    {
        for (uint64_t u = 0; u < nRx; ++u)
        {
            C acc{0.0, 0.0};
            for (uint64_t s = 0; s < nTx; ++s)
            {
                acc += params->m_channel(u, s, c) * wTx[s];
            }
            g[u] += acc;
        }
    }
    // Normalize: wRx = g / ||g||
    double norm2 = 0.0;
    for (uint64_t u = 0; u < nRx; ++u)
    {
        norm2 += std::norm(g[u]);
    }
    if (norm2 > 0.0)
    {
        const double invNorm = 1.0 / std::sqrt(norm2);
        for (uint64_t u = 0; u < nRx; ++u)
        {
            g[u] *= invNorm;
        }
    }
    return g; // unit-norm (unless channel is all-zero)
}

/**
 * Compute the average SNR
 * @param params A structure that holds the parameters that are needed to perform calculations in
 * ComputeSnr
 */
static void
ComputeSnr(const ComputeSnrParams& params)
{
    // Create the tx PSD using the LteSpectrumValueHelper
    // 100 RBs corresponds to 18 MHz (1 RB = 180 kHz)
    // EARFCN 100 corresponds to 2125.00 MHz
    std::vector<int> activeRbs0(100);
    for (int i = 0; i < 100; i++)
    {
        activeRbs0[i] = i;
    }
    auto txPsd =
        LteSpectrumValueHelper::CreateTxPowerSpectralDensity(2100, 100, params.txPow, activeRbs0);
    auto txParams = Create<SpectrumSignalParameters>();
    txParams->psd = txPsd->Copy();
    NS_LOG_DEBUG("Average tx power " << 10 * log10(Sum(*txPsd) * 180e3) << " dB");

    // create the noise PSD
    auto noisePsd =
        LteSpectrumValueHelper::CreateNoisePowerSpectralDensity(2100, 100, params.noiseFigure);
    NS_LOG_DEBUG("Average noise power " << 10 * log10(Sum(*noisePsd) * 180e3) << " dB");

    // apply the pathloss
    double propagationGainDb = m_propagationLossModel->CalcRxPower(0, params.txMob, params.rxMob);
    NS_LOG_DEBUG("Pathloss " << -propagationGainDb << " dB");
    double propagationGainLinear = std::pow(10.0, (propagationGainDb) / 10.0);
    *(txParams->psd) *= propagationGainLinear;

    NS_ASSERT_MSG(params.txAntenna, "params.txAntenna is nullptr!");
    NS_ASSERT_MSG(params.rxAntenna, "params.rxAntenna is nullptr!");

    // set/update the beamforming vectors on each call
    double sign = -1.0;
    DoBeamforming(params.txMob, params.txAntenna, params.rxMob, sign);
    sign = +1.0;
    DoBeamforming(params.rxMob, params.rxAntenna, params.txMob, sign);
    // apply the fast fading and the beamforming gain
    auto rxParams = m_spectrumLossModel->CalcRxPowerSpectralDensity(txParams,
                                                                    params.txMob,
                                                                    params.rxMob,
                                                                    params.txAntenna,
                                                                    params.rxAntenna);
    auto rxPsdSteering = rxParams->psd;
    NS_LOG_DEBUG("Average rx power " << 10 * log10(Sum(*rxPsdSteering) * 180e3) << " dB");
    NS_LOG_DEBUG("Average SNR " << 10 * log10(Sum(*rxPsdSteering) / Sum(*noisePsd)) << " dB");

    Ptr<ThreeGppChannelModel> channelModel =
        DynamicCast<ThreeGppChannelModel>(m_spectrumLossModel->GetChannelModel());
    Ptr<const ThreeGppChannelModel::ChannelMatrix> channelMatrix =
        channelModel->GetChannel(params.txMob, params.rxMob, params.txAntenna, params.rxAntenna);

    const auto& wTx = params.txAntenna->GetBeamformingVectorRef();
    auto wRx =
        BuildRxMatchedFilterOverAllClusters(channelMatrix, params.txAntenna, params.rxAntenna, wTx);
    params.rxAntenna->SetBeamformingVector(wRx); // Install RX matched-filter (MRC) combiner

    auto rxParamsMatchedFilterComb =
        m_spectrumLossModel->CalcRxPowerSpectralDensity(txParams,
                                                        params.txMob,
                                                        params.rxMob,
                                                        params.txAntenna,
                                                        params.rxAntenna);

    auto rxPsdMatchedFilterComb = rxParamsMatchedFilterComb->psd;
    NS_LOG_DEBUG("Average rx power when using matched filter combining "
                 << 10 * log10(Sum(*rxPsdMatchedFilterComb) * 180e3) << " dB");
    NS_LOG_DEBUG("Average SNR when using matched filter combining "
                 << 10 * log10(Sum(*rxPsdMatchedFilterComb) / Sum(*noisePsd)) << " dB");

    // print the SNR and pathloss values in the snr-trace.txt file
    std::ofstream f;
    f.open("snr-trace.txt", std::ios::out | std::ios::app);
    f << Simulator::Now().GetSeconds() << " " << 10 * log10(Sum(*rxPsdSteering) / Sum(*noisePsd))
      << " " << 10 * log10(Sum(*rxPsdMatchedFilterComb) / Sum(*noisePsd)) << " "
      << propagationGainDb << std::endl;
    f.close();
}

int
main(int argc, char* argv[])
{
    double frequency = 2125.0e6;  // operating frequency in Hz (corresponds to EARFCN 2100)
    double txPow = 49.0;          // tx power in dBm
    double noiseFigure = 9.0;     // noise figure in dB
    double distance = 10.0;       // distance between tx and rx nodes in meters
    uint32_t simTime = 1000;      // simulation time in milliseconds
    uint32_t timeRes = 10;        // time resolution in milliseconds
    std::string scenario = "UMa"; // 3GPP propagation scenario

    CommandLine cmd(__FILE__);
    cmd.AddValue("frequency", "Operating frequency in Hz", frequency);
    cmd.AddValue("txPow", "TX power in dBm", txPow);
    cmd.AddValue("noiseFigure", "Noise figure in dB", noiseFigure);
    cmd.AddValue("distance", "Distance between TX and RX nodes in meters", distance);
    cmd.AddValue("simTime", "Simulation time in milliseconds", simTime);
    cmd.AddValue("timeRes", "Time resolution in milliseconds", timeRes);
    cmd.AddValue("scenario", "3GPP propagation scenario", scenario);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::ThreeGppChannelModel::UpdatePeriod",
                       TimeValue(MilliSeconds(1))); // update the channel at each iteration
    Config::SetDefault("ns3::ThreeGppChannelConditionModel::UpdatePeriod",
                       TimeValue(MilliSeconds(0))); // do not update the channel condition

    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);

    // create and configure the factories for the channel condition and propagation loss models
    ObjectFactory propagationLossModelFactory;
    ObjectFactory channelConditionModelFactory;
    if (scenario == "RMa")
    {
        propagationLossModelFactory.SetTypeId(ThreeGppRmaPropagationLossModel::GetTypeId());
        channelConditionModelFactory.SetTypeId(ThreeGppRmaChannelConditionModel::GetTypeId());
    }
    else if (scenario == "UMa")
    {
        propagationLossModelFactory.SetTypeId(ThreeGppUmaPropagationLossModel::GetTypeId());
        channelConditionModelFactory.SetTypeId(ThreeGppUmaChannelConditionModel::GetTypeId());
    }
    else if (scenario == "UMi-StreetCanyon")
    {
        propagationLossModelFactory.SetTypeId(
            ThreeGppUmiStreetCanyonPropagationLossModel::GetTypeId());
        channelConditionModelFactory.SetTypeId(
            ThreeGppUmiStreetCanyonChannelConditionModel::GetTypeId());
    }
    else if (scenario == "InH-OfficeOpen")
    {
        propagationLossModelFactory.SetTypeId(
            ThreeGppIndoorOfficePropagationLossModel::GetTypeId());
        channelConditionModelFactory.SetTypeId(
            ThreeGppIndoorOpenOfficeChannelConditionModel::GetTypeId());
    }
    else if (scenario == "InH-OfficeMixed")
    {
        propagationLossModelFactory.SetTypeId(
            ThreeGppIndoorOfficePropagationLossModel::GetTypeId());
        channelConditionModelFactory.SetTypeId(
            ThreeGppIndoorMixedOfficeChannelConditionModel::GetTypeId());
    }
    else
    {
        NS_FATAL_ERROR("Unknown scenario");
    }

    // create the propagation loss model
    m_propagationLossModel = propagationLossModelFactory.Create<ThreeGppPropagationLossModel>();
    m_propagationLossModel->SetAttribute("Frequency", DoubleValue(frequency));
    m_propagationLossModel->SetAttribute("ShadowingEnabled", BooleanValue(false));

    // create the spectrum propagation loss model
    m_spectrumLossModel = CreateObject<ThreeGppSpectrumPropagationLossModel>();
    m_spectrumLossModel->SetChannelModelAttribute("Frequency", DoubleValue(frequency));
    m_spectrumLossModel->SetChannelModelAttribute("Scenario", StringValue(scenario));

    // create the channel condition model and associate it with the spectrum and
    // propagation loss model
    Ptr<ChannelConditionModel> condModel =
        channelConditionModelFactory.Create<ThreeGppChannelConditionModel>();
    m_spectrumLossModel->SetChannelModelAttribute("ChannelConditionModel", PointerValue(condModel));
    m_propagationLossModel->SetChannelConditionModel(condModel);

    // create the tx and rx nodes
    NodeContainer nodes;
    nodes.Create(2);

    // create the tx and rx mobility models, set the positions
    Ptr<MobilityModel> txMob = CreateObject<ConstantPositionMobilityModel>();
    txMob->SetPosition(Vector(0.0, 0.0, 10.0));
    Ptr<ConstantVelocityMobilityModel> rxMob = CreateObject<ConstantVelocityMobilityModel>();
    rxMob->SetPosition(Vector(distance, 0.0, 1.6));
    rxMob->SetVelocity(Vector(0.5, 0, 0)); // set small velocity to allow for channel updates

    // assign the mobility models to the nodes
    nodes.Get(0)->AggregateObject(txMob);
    nodes.Get(1)->AggregateObject(rxMob);

    // create the antenna objects and set their dimensions
    Ptr<PhasedArrayModel> txAntenna =
        CreateObjectWithAttributes<UniformPlanarArray>("NumColumns",
                                                       UintegerValue(2),
                                                       "NumRows",
                                                       UintegerValue(2));
    Ptr<PhasedArrayModel> rxAntenna =
        CreateObjectWithAttributes<UniformPlanarArray>("NumColumns",
                                                       UintegerValue(2),
                                                       "NumRows",
                                                       UintegerValue(2));

    for (int i = 0; i < floor(simTime / timeRes); i++)
    {
        ComputeSnrParams params{txMob, rxMob, txPow, noiseFigure, txAntenna, rxAntenna};
        Simulator::Schedule(MilliSeconds(timeRes * i), &ComputeSnr, params);
    }

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
