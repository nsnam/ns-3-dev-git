/*
 * Copyright (c) 2025 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 * Author: Amir Ashtari <aashtari@cttc.es>
 * SPDX-License-Identifier: GPL-2.0-only
 */

/**
 * @file sionna-rt-channel-example.cc
 * @brief Example demonstrating how to use the Sionna RT channel model in ns-3.
 *
 * This example shows how to configure and use the Sionna RT channel classes to
 * compute the average SNR between two static/moving nodes in a scene computed
 * using the sionna.rt Python library. The example:
 *  - Configures a SionnaRtSpectrumPropagationLossModel instance to use the
 *    sionna.rt scene and path solver,
 *  - Creates a simple two-node network (SimpleNetDevice),
 *  - Configures mobility and uniform planar arrays for antennas on each node,
 *  - Applies simple DFT-based beamforming to point arrays at the intended peer,
 *  - Computes average SNR periodically and logs results to the console and a file.
 *
 * The example uses default 30 GHz operation, 18 MHz equivalent LTE bandwidth
 * (100 RBs), and `ThreeGppAntennaModel` elements arranged in small planar arrays.
 *
 * Prerequisites:
 *  - The sionna.rt Python library must be available and imported properly by the
 *    Sionna RT channel model (check PYTHONPATH and environment).
 *
 *
 * Common command-line options (and defaults):
 *  - frequency:   Operating frequency in Hz (default 30e9)
 *  - txPow:       Tx power in dBm (default 49.0)
 *  - noiseFigure: Noise figure in dB (default 9.0)
 *  - distance:    Distance between tx and rx in meters (default 50.0)
 *  - simTime:     Simulation time in milliseconds (default 2000)
 *  - timeRes:     Time resolution for periodic SNR compute (ms) (default 10)
 *  - IsImageRenderedEnabled: Enable rendering of scene images (default true)
 *  - outputFileName:      Output filename prefix for scene images (default "sionna-rt-scene3-")
 *  - outputFileDirectory: Directory for scene image output (default "sionna-rt-images2")
 *
 * The example also exposes several Sionna RT path solver configuration parameters:
 *  - maxDepth: maximum reflection/refraction depth
 *  - los: include line-of-sight
 *  - specularReflection: enable specular reflections
 *  - diffuseReflection: enable diffuse reflections
 *  - diffraction: enable diffraction
 *  - edgeDiffraction: enable edge diffraction
 *  - refraction: enable refractions
 *  - syntheticArray: use synthetic array processing
 *  - seed: random seed used by the path solver
 *
 * The main computed outputs are:
 *  - Per-iteration SNR
 *  - Average rx power
 *  - A log file named "snr-trace.txt" containing time-stamped SNR values
 *
 * @note The channel update period is set using the ns-3 attribute:
 *       ns3::SionnaRtChannelModel::UpdatePeriod
 *
 */

#include "pybind11/pybind11.h"

#include "ns3/channel-condition-model.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/constant-velocity-mobility-model.h"
#include "ns3/core-module.h"
#include "ns3/lte-spectrum-value-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/simple-net-device.h"
#include "ns3/sionna-rt-channel-model.h"
#include "ns3/sionna-rt-spectrum-propagation-loss-model.h"
#include "ns3/spectrum-signal-parameters.h"
#include "ns3/three-gpp-antenna-model.h"
#include "ns3/uniform-planar-array.h"

#include <iomanip>
#include <iostream>

NS_LOG_COMPONENT_DEFINE("SionnaRTChannelExample");
namespace py = pybind11;
using namespace ns3;

static Ptr<SionnaRtSpectrumPropagationLossModel>
    m_spectrumLossModel;  //!< the SpectrumPropagationLossModel object
static int g_snrSamples;  //!< number of SNR samples computed
static double g_snrSumDb; //!< running sum of SNR values in dB

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
    Ptr<OutputStreamWrapper> stream; //!< output stream wrapper for SNR trace
};

/**
 * Perform the beamforming using the DFT beamforming method
 * @param thisDevice the device performing the beamforming
 * @param thisAntenna the antenna object associated to thisDevice
 * @param otherDevice the device towards which point the beam
 */
static void
DoBeamforming(Ptr<NetDevice> thisDevice,
              Ptr<PhasedArrayModel> thisAntenna,
              Ptr<NetDevice> otherDevice)
{
    // retrieve the position of the two devices
    Vector aPos = thisDevice->GetNode()->GetObject<MobilityModel>()->GetPosition();
    Vector bPos = otherDevice->GetNode()->GetObject<MobilityModel>()->GetPosition();

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
        double phase = -2 * M_PI *
                       (sinVAngleRadian * cosHAngleRadian * loc.x +
                        sinVAngleRadian * sinHAngleRadian * loc.y + cosVAngleRadian * loc.z);
        antennaWeights[ind] = exp(std::complex<double>(0, phase)) * power;
    }

    // store the antenna weights
    thisAntenna->SetBeamformingVector(antennaWeights);
}

/**
 * Print the python executable and version
 */
static void
PrintPythonExecutable()
{
    py::object sys = py::module_::import("sys");
    py::print("Python executable:", sys.attr("executable"));
    py::print("Python version:", sys.attr("version"));
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

    // Zero pathloss :: already included in the SionnaRtSpectrumPropagationLossModel
    // so no need to add it here

    NS_ASSERT_MSG(params.txAntenna, "params.txAntenna is nullptr!");
    NS_ASSERT_MSG(params.rxAntenna, "params.rxAntenna is nullptr!");

    // apply the fast fading and the beamforming gain
    auto rxParams = m_spectrumLossModel->CalcRxPowerSpectralDensity(txParams,
                                                                    params.txMob,
                                                                    params.rxMob,
                                                                    params.txAntenna,
                                                                    params.rxAntenna);
    auto rxPsd = rxParams->psd;
    double rxPowDb = 10 * log10(Sum(*rxPsd) * 180e3);
    double snrDb = 10 * log10(Sum(*rxPsd) / Sum(*noisePsd));
    NS_LOG_DEBUG("Average rx power " << rxPowDb << " dB");
    NS_LOG_DEBUG("Average SNR " << snrDb << " dB");

    g_snrSamples++;
    g_snrSumDb += snrDb;

    std::cout << "  [t=" << std::fixed << std::setprecision(3) << Simulator::Now().GetSeconds()
              << "s]"
              << "  SNR = " << std::setprecision(2) << snrDb << " dB"
              << "  |  Rx power = " << rxPowDb << " dBm" << std::endl;

    // print the SNR and pathloss values in the snr-trace.txt file
    auto* stream = params.stream->GetStream();
    *stream << Simulator::Now().GetSeconds() << " " << snrDb << std::endl;
}

int
main(int argc, char* argv[])
{
    py::scoped_interpreter guard{}; // Python stays alive for whole program

    // to check the python executable and version
    PrintPythonExecutable();

    std::cout << "\n--- Sionna-RT Channel Example ---\n" << std::endl;

    double frequency = 30e9;  // operating frequency in Hz (corresponds to EARFCN 2100)
    double txPow = 49.0;      // tx power in dBm
    double noiseFigure = 9.0; // noise figure in dB
    double distance = 50.0;   // distance between tx and rx nodes in meters
    uint32_t simTime = 20;    // simulation time in milliseconds
    uint32_t timeRes = 10;    // time resolution in milliseconds

    std::string Scenario = "simple_street_canyon_with_cars"; // propagation scenario

    bool enableGnbIso = false;          // enable isotropic elements at gNB
    bool enableUeIso = false;           // enable isotropic elements at UE
    bool enableGnbDualPolarized = true; // enable dual-polarized elements at gNB
    bool enableUeDualPolarized = true;  // enable dual-polarized elements at UE

    bool IsImageRenderedEnabled = false;               // enable rendering of scene images to file
    Vector CameraPosition(Vector(70.0, -20.0, 190.0)); // Camera position
    Vector CameraLookAt(Vector(0.0, 0.0, 4.0));        // Camera look-at point
    std::string filenamePrefix = "sionna-rt-scene-";   // output file name for scene images
    std::string filedirectory = "sionna-rt-images";    // output file directory for scene images

    // Sionna RT path solver configuration defaults
    SionnaRtChannelModel::RtPathSolverConfig RtPathSolverConfig;
    RtPathSolverConfig.maxDepth = 2;              // Maximum reflection/refraction depth
    RtPathSolverConfig.los = true;                // Include line-of-sight path
    RtPathSolverConfig.specularReflection = true; // Enable specular reflections
    RtPathSolverConfig.diffuseReflection = true;  // Enable diffuse reflections
    RtPathSolverConfig.diffraction = true;        // Disable diffraction
    RtPathSolverConfig.edgeDiffraction = true;    // Disable edge diffraction
    RtPathSolverConfig.refraction = true;         // Enable refractions
    RtPathSolverConfig.syntheticArray = false;    // Use synthetic array processing
    RtPathSolverConfig.seed = 41;                 // Random seed

    CommandLine cmd;
    cmd.AddValue("Scenario", "Propagation scenario", Scenario);

    cmd.AddValue("frequency", "Operating frequency in Hz", frequency);

    cmd.AddValue("enableGnbIso", "Enable isotropic elements at gNB", enableGnbIso);
    cmd.AddValue("enableUeIso", "Enable isotropic elements at UE", enableUeIso);
    cmd.AddValue("enableGnbDualPolarized",
                 "Enable dual-polarized elements at gNB",
                 enableGnbDualPolarized);
    cmd.AddValue("enableUeDualPolarized",
                 "Enable dual-polarized elements at UE",
                 enableUeDualPolarized);

    cmd.AddValue("IsImageRenderedEnabled",
                 "Enable rendering of scene images to file",
                 IsImageRenderedEnabled);
    cmd.AddValue("outputFileName", "Output file name for scene images", filenamePrefix);
    cmd.AddValue("outputFileDirectory", "Output file directory for scene images", filedirectory);
    cmd.AddValue("cameraPosition", "Camera position for scene rendering", CameraPosition);
    cmd.AddValue("cameraLookAt", "Camera look-at point for scene rendering", CameraLookAt);

    cmd.AddValue("txPow", "Tx power in dBm", txPow);
    cmd.AddValue("noiseFigure", "Noise figure in dB", noiseFigure);
    cmd.AddValue("distance", "Distance between tx and rx nodes in meters", distance);

    cmd.AddValue("simTime", "Simulation time in milliseconds", simTime);
    cmd.AddValue("timeRes", "Time resolution in milliseconds", timeRes);

    cmd.AddValue("maxDepth",
                 "Maximum reflection/refraction depth for sionna-rt configuration",
                 RtPathSolverConfig.maxDepth);
    cmd.AddValue("los",
                 "Include line-of-sight path for sionna-rt configuration",
                 RtPathSolverConfig.los);
    cmd.AddValue("specularReflection",
                 "Enable specular reflections for sionna-rt configuration",
                 RtPathSolverConfig.specularReflection);
    cmd.AddValue("diffuseReflection",
                 "Enable diffuse reflections for sionna-rt configuration",
                 RtPathSolverConfig.diffuseReflection);
    cmd.AddValue("refraction",
                 "Enable refractions for sionna-rt configuration",
                 RtPathSolverConfig.refraction);
    cmd.AddValue("syntheticArray",
                 "Use synthetic array processing for sionna-rt configuration",
                 RtPathSolverConfig.syntheticArray);
    cmd.AddValue("diffraction",
                 "Enable diffraction for sionna-rt configuration",
                 RtPathSolverConfig.diffraction);
    cmd.AddValue("edgeDiffraction",
                 "Enable edge diffraction for sionna-rt configuration",
                 RtPathSolverConfig.edgeDiffraction);
    cmd.AddValue("seed", "Random seed", RtPathSolverConfig.seed);

    cmd.Parse(argc, argv);

    // set the channel update period used by the Sionna RT channel model
    Config::SetDefault("ns3::SionnaRtChannelModel::UpdatePeriod",
                       TimeValue(MilliSeconds(5))); // update the channel at each iteration

    RngSeedManager::SetSeed(RtPathSolverConfig.seed);
    RngSeedManager::SetRun(RtPathSolverConfig.seed);

    // create and configure the Sionna-RT spectrum propagation loss model
    m_spectrumLossModel = CreateObject<SionnaRtSpectrumPropagationLossModel>();
    m_spectrumLossModel->SetChannelModelAttribute("Frequency", DoubleValue(frequency));
    m_spectrumLossModel->SetChannelModelAttribute(
        "Scenario",
        StringValue(Scenario)); // set the propagation scenario

    // enable image rendering and set output filenames/paths if desired
    m_spectrumLossModel->SetChannelModelAttribute(
        "IsImageRenderedEnabled",
        BooleanValue(IsImageRenderedEnabled)); // enable rendering of scene images to file
    m_spectrumLossModel->SetChannelModelAttribute(
        "OutputImageDirectory",
        StringValue(filedirectory)); // set output directory for scene images
    m_spectrumLossModel->SetChannelModelAttribute(
        "OutputImageName",
        StringValue(filenamePrefix)); // set the output file name for scene images

    // set up camera parameters used for scene rendering
    m_spectrumLossModel->SetChannelModelAttribute("CameraPosition", VectorValue(CameraPosition));
    m_spectrumLossModel->SetChannelModelAttribute("CameraLookAt", VectorValue(CameraLookAt));

    // apply the solver configuration
    m_spectrumLossModel->SetRtPathSolverConfig(RtPathSolverConfig);

    // create the tx and rx nodes
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

    // create the tx and rx mobility models, set the positions
    Ptr<MobilityModel> txMob = CreateObject<ConstantPositionMobilityModel>();
    txMob->SetPosition(Vector(-20.0, 00.0, 0.0));

    Ptr<ConstantVelocityMobilityModel> rxMob = CreateObject<ConstantVelocityMobilityModel>();
    rxMob->SetPosition(Vector(distance - 20.0, 00.0, 0));
    rxMob->SetVelocity(Vector(00.0, -20.0, 0.0));

    // assign the mobility models to the nodes
    nodes.Get(0)->AggregateObject(txMob);
    nodes.Get(1)->AggregateObject(rxMob);

    // create the antenna objects and set their dimensions
    Ptr<PhasedArrayModel> txAntenna =
        CreateObjectWithAttributes<UniformPlanarArray>("NumColumns",
                                                       UintegerValue(2),
                                                       "NumRows",
                                                       UintegerValue(2));
    if (!enableGnbIso)
    {
        txAntenna->SetAttribute("AntennaElement",
                                PointerValue(CreateObject<ThreeGppAntennaModel>()));
    }

    txAntenna->SetAttribute("IsDualPolarized", BooleanValue(enableGnbDualPolarized));

    Ptr<PhasedArrayModel> rxAntenna =
        CreateObjectWithAttributes<UniformPlanarArray>("NumColumns",
                                                       UintegerValue(2),
                                                       "NumRows",
                                                       UintegerValue(2));
    if (!enableUeIso)
    {
        rxAntenna->SetAttribute("AntennaElement",
                                PointerValue(CreateObject<ThreeGppAntennaModel>()));
    }

    rxAntenna->SetAttribute("IsDualPolarized", BooleanValue(enableUeDualPolarized));

    // set the beamforming vectors
    DoBeamforming(txDev, txAntenna, rxDev);
    DoBeamforming(rxDev, rxAntenna, txDev);

    // Create the SNR trace output stream wrapper (kept alive for the full simulation)
    Ptr<OutputStreamWrapper> snrOutputStream =
        Create<OutputStreamWrapper>("snr-trace.txt", std::ios::out);

    for (int i = 0; i < floor(simTime / timeRes); i++)
    {
        ComputeSnrParams
            params{txMob, rxMob, txPow, noiseFigure, txAntenna, rxAntenna, snrOutputStream};
        Simulator::Schedule(MilliSeconds(timeRes * i), &ComputeSnr, params);
    }

    try
    {
        Simulator::Run();
        Simulator::Destroy();
    }
    catch (const py::error_already_set& e)
    {
        std::cerr << "\n[FAILURE] Sionna/Python error during simulation:\n"
                  << "  " << e.what() << "\n"
                  << "  Check that the Sionna virtual environment is active and the\n"
                  << "  scene name (--Scenario) is correct.\n"
                  << std::endl;
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n[FAILURE] Simulation error:\n"
                  << "  " << e.what() << "\n"
                  << std::endl;
        return 1;
    }

    if (g_snrSamples > 0)
    {
        std::cout << "\n[SUCCESS] Simulation completed successfully.\n"
                  << "  Computed " << g_snrSamples << " SNR sample(s), "
                  << "average SNR = " << std::fixed << std::setprecision(2)
                  << (g_snrSumDb / g_snrSamples) << " dB\n"
                  << "  Results written to snr-trace.txt\n"
                  << std::endl;
    }
    else
    {
        std::cerr << "\n[FAILURE] Simulation ran but no SNR samples were produced.\n"
                  << "  Check that simTime > 0 and the channel model is configured correctly.\n"
                  << std::endl;
        return 1;
    }

    return 0;
}
