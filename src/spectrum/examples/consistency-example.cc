/*
 * Copyright (c) 2021 NYU WIRELESS, Tandon School of Engineering,
 * New York University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 */

/**
 * This example shows how to configure the 3GPP channel model for spatially
 * consistent simulations with mobility of the nodes.
 * The simulation involves two nodes which are placed at a certain
 * distance from each other (one of which moves away with constant speed from the other node)
 * and communicate through a wireless channel at 28 GHz with a bandwidth of 20 MHz.
 * The default propagation environment is 3D-urban micro (UMi-StreetCanyon)
 * and it can be configured by changing the value of the string "scenario".
 * Each node hosts a SimpleNetDevice and has an antenna array with 16 elements.
 */

#include "ns3/channel-condition-model.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/constant-velocity-mobility-model.h"
#include "ns3/core-module.h"
#include "ns3/lte-spectrum-value-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/simple-net-device.h"
#include "ns3/three-gpp-channel-model.h"
#include "ns3/three-gpp-propagation-loss-model.h"
#include "ns3/three-gpp-spectrum-propagation-loss-model.h"
#include "ns3/uniform-planar-array.h"

#include <fstream>

NS_LOG_COMPONENT_DEFINE("ConsistencyExample");

using namespace ns3;

static Ptr<ThreeGppPropagationLossModel>
    m_propagationLossModel; //!< the PropagationLossModel object
static Ptr<ThreeGppSpectrumPropagationLossModel>
    m_spectrumLossModel; //!< the SpectrumPropagationLossModel object

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
    PhasedArrayModel::ComplexVector antennaWeights;

    // retrieve the position of the two devices
    Vector aPos = thisDevice->GetNode()->GetObject<MobilityModel>()->GetPosition();
    Vector bPos = otherDevice->GetNode()->GetObject<MobilityModel>()->GetPosition();

    // compute the azimuth and the elevation angles
    Angles completeAngle(bPos, aPos);
    double hAngleRadian = completeAngle.GetAzimuth();

    double vAngleRadian = completeAngle.GetInclination(); // the elevation angle

    // retrieve the number of antenna elements
    int totNoArrayElements = thisAntenna->GetNumberOfElements();

    // the total power is divided equally among the antenna elements
    double power = 1 / sqrt(totNoArrayElements);

    // compute the antenna weights
    for (int ind = 0; ind < totNoArrayElements; ind++)
    {
        Vector loc = thisAntenna->GetElementLocation(ind);
        double phase = -2 * M_PI *
                       (sin(vAngleRadian) * cos(hAngleRadian) * loc.x +
                        sin(vAngleRadian) * sin(hAngleRadian) * loc.y + cos(vAngleRadian) * loc.z);
        antennaWeights.push_back(exp(std::complex<double>(0, phase)) * power);
    }

    // store the antenna weights
    thisAntenna->SetBeamformingVector(antennaWeights);
}

/**
 * Compute the average SNR
 * @param txMob the tx mobility model
 * @param rxMob the rx mobility model
 * @param txPow the transmitting power in dBm
 * @param noiseFigure the noise figure in dB
 */
static void
ComputeSnr(Ptr<MobilityModel> txMob, Ptr<MobilityModel> rxMob, double txPow, double noiseFigure)
{
    // Create the tx PSD using the LteSpectrumValueHelper
    // 100 RBs corresponds to 18 MHz (1 RB = 180 kHz)
    // EARFCN 100 corresponds to 2125.00 MHz
    std::vector<int> activeRbs0(100);
    for (int i = 0; i < 100; i++)
    {
        activeRbs0[i] = i;
    }
    Ptr<SpectrumValue> txPsd =
        LteSpectrumValueHelper::CreateTxPowerSpectralDensity(2100, 100, txPow, activeRbs0);
    Ptr<SpectrumValue> rxPsd = txPsd->Copy();
    NS_LOG_DEBUG("Average tx power " << 10 * log10(Sum(*txPsd) * 180e3) << " dB");

    // create the noise PSD
    Ptr<SpectrumValue> noisePsd =
        LteSpectrumValueHelper::CreateNoisePowerSpectralDensity(2100, 100, noiseFigure);
    NS_LOG_DEBUG("Average noise power " << 10 * log10(Sum(*noisePsd) * 180e3) << " dB");

    // apply the pathloss
    double propagationGainDb = m_propagationLossModel->CalcRxPower(0, txMob, rxMob);
    NS_LOG_DEBUG("Pathloss " << -propagationGainDb << " dB");
    double propagationGainLinear = std::pow(10.0, (propagationGainDb) / 10.0);
    *(rxPsd) *= propagationGainLinear;

    // apply the fast fading and the beamforming gain
    rxPsd = m_spectrumLossModel->CalcRxPowerSpectralDensity(rxPsd, txMob, rxMob);
    NS_LOG_DEBUG("Average rx power " << 10 * log10(Sum(*rxPsd) * 180e3) << " dB");

    // compute the SNR
    NS_LOG_DEBUG("Average SNR " << 10 * log10(Sum(*rxPsd) / Sum(*noisePsd)) << " dB");

    // print the SNR and pathloss values in the snr-trace.txt file
    std::ofstream f;
    f.open("snr-trace.txt", std::ios::out | std::ios::app);
    f << Simulator::Now().GetSeconds() << " " << 10 * log10(Sum(*rxPsd) / Sum(*noisePsd)) << " "
      << propagationGainDb << std::endl;
    f.close();
}

int
main(int argc, char* argv[])
{
    double frequency = 28e9;                   // operating frequency in Hz
    double txPow = 30.0;                       // tx power in dBm
    double noiseFigure = 9.0;                  // noise figure in dB
    double distance = 10.0;                    // initial distance between tx and rx nodes in meters
    uint32_t simTime = 100000;                 // simulation time in milliseconds
    uint32_t timeRes = 2;                      // time resolution in milliseconds
    std::string scenario = "UMi-StreetCanyon"; // 3GPP propagation scenario
    double rxSpeed = 1.8;                      // speed of the rx node in m/s
    bool consistency = true;                   // simulate with spatial consistency

    Config::SetDefault("ns3::ThreeGppChannelModel::UpdatePeriod",
                       TimeValue(MilliSeconds(10))); // channel update period
    Config::SetDefault(
        "ns3::ThreeGppChannelModel::Consistency",
        BooleanValue(consistency)); // enable-disable spatial consistency in the simulation
    Config::SetDefault("ns3::ThreeGppChannelConditionModel::UpdatePeriod",
                       TimeValue(MilliSeconds(0.0))); // do not update the channel condition

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
    txMob->SetPosition(Vector(0.0, 0.0, 10.0));
    Ptr<MobilityModel> rxMob = CreateObject<ConstantVelocityMobilityModel>();
    rxMob->SetPosition(Vector(distance, 0.0, 1.6));
    rxMob->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(rxSpeed, 0.0, 0.0));

    // assign the mobility models to the nodes
    nodes.Get(0)->AggregateObject(txMob);
    nodes.Get(1)->AggregateObject(rxMob);

    // create the antenna objects and set their dimensions
    Ptr<PhasedArrayModel> txAntenna =
        CreateObjectWithAttributes<UniformPlanarArray>("NumColumns",
                                                       UintegerValue(4),
                                                       "NumRows",
                                                       UintegerValue(4));
    Ptr<PhasedArrayModel> rxAntenna =
        CreateObjectWithAttributes<UniformPlanarArray>("NumColumns",
                                                       UintegerValue(4),
                                                       "NumRows",
                                                       UintegerValue(4));

    // initialize the devices in the ThreeGppSpectrumPropagationLossModel
    m_spectrumLossModel->AddDevice(txDev, txAntenna);
    m_spectrumLossModel->AddDevice(rxDev, rxAntenna);

    // set the beamforming vectors
    DoBeamforming(txDev, txAntenna, rxDev);
    DoBeamforming(rxDev, rxAntenna, txDev);

    for (int i = 0; i < floor(simTime / timeRes); i++)
    {
        Simulator::Schedule(MilliSeconds(timeRes * i),
                            &ComputeSnr,
                            txMob,
                            rxMob,
                            txPow,
                            noiseFigure);
    }

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
