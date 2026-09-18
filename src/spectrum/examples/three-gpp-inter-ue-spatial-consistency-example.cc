/*
 * Copyright (c) 2026, Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

/*
 * @file
 * This example demonstrates the inter-UE (drop-based) spatial consistency of
 * 3GPP TR 38.901, Sec. 7.6.3, enabled by the InterUeSpatialConsistency
 * attribute of the ChannelConditionModel and consulted by
 * ThreeGppPropagationLossModel (shadow fading and O2I penetration loss),
 * ThreeGppChannelModel (LSPs and cluster/ray-specific variables of the fast
 * fading) and ThreeGppChannelConditionModel (LOS/NLOS and indoor states).
 *
 * A single site transmits and the SNR is evaluated on a uniform grid of
 * terminal positions, in the same way a Radio Environment Map (REM) is
 * produced: for every evaluated position the propagation, channel condition
 * and channel models are re-created, so that each point corresponds to an
 * independent "drop". Mirroring the BeamShape mode of the nr module REM
 * helper, the site uses a 4x4 array of 3GPP TR 38.901 antenna elements with a
 * uniform direct-path (DFT) beam pointing straight in front of the panel
 * (towards +x) at a configurable distance, and the terminal (the map probe)
 * uses a 2x2 array of isotropic elements with a fixed quasi-omni beam. Without spatial
 * consistency each drop draws independent shadow fading and the resulting map
 * is salt-and-pepper noise. With spatial consistency enabled the shadow
 * fading (and the LSPs) are drawn from per-site spatially-correlated Gaussian
 * random fields with the correlation distances of TR 38.901 Table 7.5-6, so
 * nearby positions obtain correlated values and the map shows smooth
 * shadowing "blobs".
 *
 * The example generates the output file
 * 'three-gpp-inter-ue-spatial-consistency-<simTag>.out', with one row per grid point:
 * x[m] y[m] SNR[dB] losCondition(0=LOS,1=NLOS)
 *
 * The companion script three-gpp-inter-ue-spatial-consistency-example.py runs the
 * example twice (spatial consistency off/on) and plots the two maps, their
 * small-scale residuals, and the SNR CDFs side by side.
 */

#include "ns3/angles.h"
#include "ns3/boolean.h"
#include "ns3/channel-condition-model.h"
#include "ns3/command-line.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/double.h"
#include "ns3/isotropic-antenna-model.h"
#include "ns3/node-container.h"
#include "ns3/pointer.h"
#include "ns3/simple-net-device.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-signal-parameters.h"
#include "ns3/string.h"
#include "ns3/three-gpp-antenna-model.h"
#include "ns3/three-gpp-channel-model.h"
#include "ns3/three-gpp-propagation-loss-model.h"
#include "ns3/three-gpp-spectrum-propagation-loss-model.h"
#include "ns3/uinteger.h"
#include "ns3/uniform-planar-array.h"

#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ThreeGppSpatialConsistencyExample");

/**
 * Set a direct (DFT) beamforming vector to the antenna array, steering the
 * beam from the antenna position towards the target position.
 * @param antenna the antenna array to configure
 * @param antennaPos the position of the antenna
 * @param targetPos the position the beam points at
 * @param taper apply a Hamming amplitude taper to suppress the sidelobe nulls
 */
static void
PointBeamTowards(Ptr<PhasedArrayModel> antenna,
                 const Vector& antennaPos,
                 const Vector& targetPos,
                 bool taper)
{
    Angles angles(targetPos, antennaPos);
    // use the (un-conjugated) steering vector: the 3GPP channel model applies
    // the ray phase exp(+j 2 pi r.d / lambda), so the coherent combining
    // weights towards direction r are its element-wise conjugate, which is the
    // steering vector itself (GetBeamformingVector would conjugate it,
    // mirroring the beam through the array plane)
    PhasedArrayModel::ComplexVector bf = antenna->GetSteeringVector(angles);
    Ptr<UniformPlanarArray> upa = DynamicCast<UniformPlanarArray>(antenna);
    uint32_t numCols = upa->GetNumColumns();
    uint32_t numRows = upa->GetNumRows();
    double norm2 = 0;
    for (size_t i = 0; i < bf.GetSize(); i++)
    {
        if (taper)
        {
            // Hamming amplitude taper across rows and columns: suppresses the
            // hard sidelobe nulls of the uniform DFT beam (at the cost of a
            // slightly wider main lobe), as real deployments do
            size_t planarIndex = i % (numRows * numCols);
            size_t col = planarIndex % numCols;
            size_t row = planarIndex / numCols;
            auto hamming = [](size_t k, uint32_t n) {
                return n == 1 ? 1.0 : 0.54 - 0.46 * std::cos(2 * M_PI * k / (n - 1));
            };
            bf[i] *= hamming(col, numCols) * hamming(row, numRows);
        }
        norm2 += std::norm(bf[i]);
    }
    for (size_t i = 0; i < bf.GetSize(); i++)
    {
        bf[i] /= std::sqrt(norm2);
    }
    antenna->SetBeamformingVector(bf);
}

/**
 * Set a quasi-omni beamforming vector to the antenna array.
 * @param antenna the antenna array to configure
 */
static void
CreateQuasiOmniBf(Ptr<PhasedArrayModel> antenna)
{
    auto antennaRows = antenna->GetNumRows();
    auto antennaColumns = antenna->GetNumColumns();
    double power = 1 / sqrt(antenna->GetNumElemsPerPort());
    size_t numPolarizations = antenna->IsDualPol() ? 2 : 1;

    PhasedArrayModel::ComplexVector omni(antennaRows * antennaColumns * numPolarizations);
    uint16_t bfIndex = 0;
    for (size_t pol = 0; pol < numPolarizations; pol++)
    {
        for (uint32_t ind = 0; ind < antennaRows; ind++)
        {
            std::complex<double> c =
                (antennaRows % 2 == 0)
                    ? exp(std::complex<double>(0, M_PI * ind * ind / antennaRows))
                    : exp(std::complex<double>(0, M_PI * ind * (ind + 1) / antennaRows));
            for (uint32_t ind2 = 0; ind2 < antennaColumns; ind2++)
            {
                std::complex<double> d =
                    (antennaColumns % 2 == 0)
                        ? exp(std::complex<double>(0, M_PI * ind2 * ind2 / antennaColumns))
                        : exp(std::complex<double>(0, M_PI * ind2 * (ind2 + 1) / antennaColumns));
                omni[bfIndex] = (c * d * power);
                bfIndex++;
            }
        }
    }
    antenna->SetBeamformingVector(omni);
}

int
main(int argc, char* argv[])
{
    std::string scenario = "UMa";            // 3GPP propagation scenario
    std::string condition = "Probabilistic"; // channel condition: Probabilistic, LOS or NLOS
    double frequency = 3.5e9;                // operating frequency in Hz
    double txPow_dbm = 35.0;                 // tx power in dBm
    double noiseFigure = 9.0;                // noise figure in dB
    double rbWidthHz = 360e3;                // resource block width in Hz
    uint32_t numRb = 275;                    // number of resource blocks (~100 MHz: averages the
                          // frequency-selective fading, so the per-point noise of the
                          // map is dominated by the shadow fading)
    double siteHeight = 25.0;        // height of the site [m]
    double beamDistance = 50.0;      // distance in front of the site panel its beam points at [m]
    double terminalHeight = 1.5;     // height of the evaluated terminal [m]
    double xMin = -800.0;            // map x lower bound [m]
    double xMax = 800.0;             // map x upper bound [m]
    double yMin = -800.0;            // map y lower bound [m]
    double yMax = 800.0;             // map y upper bound [m]
    uint32_t xRes = 200;             // number of grid points along x
    uint32_t yRes = 200;             // number of grid points along y
    bool taper = false;              // Hamming-taper the beamforming weights
    bool shadowing = true;           // enable shadow fading
    bool spatialConsistency = false; // enable inter-UE spatial consistency
    bool largeBandwidth = false;     // enable the TR 38.901 Sec. 7.6.2.2 intra-cluster modeling
    std::string simTag = "default";  // tag appended to the output file name

    CommandLine cmd(__FILE__);
    cmd.AddValue("scenario", "3GPP propagation scenario: UMa, UMi or RMa", scenario);
    cmd.AddValue("condition",
                 "Channel condition: Probabilistic (scenario-specific LOS probability), LOS or "
                 "NLOS",
                 condition);
    cmd.AddValue("frequency", "Operating frequency in Hz", frequency);
    cmd.AddValue("txPow", "Transmission power in dBm", txPow_dbm);
    cmd.AddValue("beamDistance",
                 "Distance in front of the site panel its beam points at, in meters",
                 beamDistance);
    cmd.AddValue("xMin", "Map x lower bound in meters", xMin);
    cmd.AddValue("xMax", "Map x upper bound in meters", xMax);
    cmd.AddValue("yMin", "Map y lower bound in meters", yMin);
    cmd.AddValue("yMax", "Map y upper bound in meters", yMax);
    cmd.AddValue("xRes", "Number of grid points along x", xRes);
    cmd.AddValue("yRes", "Number of grid points along y", yRes);
    cmd.AddValue("taper", "Apply a Hamming amplitude taper to the beamforming weights", taper);
    cmd.AddValue("shadowing", "Enable shadow fading", shadowing);
    cmd.AddValue("spatialConsistency",
                 "Enable inter-UE (drop-based) spatial consistency of the shadow fading, of "
                 "the fast-fading generation and of the LOS/NLOS state",
                 spatialConsistency);
    cmd.AddValue("largeBandwidth",
                 "Enable the large bandwidth and large antenna array modeling of TR 38.901 "
                 "Sec. 7.6.2.2 (per-ray delays, offsets and unequal powers) in the fast fading",
                 largeBandwidth);
    cmd.AddValue("simTag", "Tag appended to the output file name", simTag);
    cmd.Parse(argc, argv);

    NS_ABORT_MSG_IF(xRes < 2 || yRes < 2, "The grid resolution must be at least 2x2");

    std::string channelScenario = (scenario == "UMi") ? "UMi-StreetCanyon" : scenario;

    // create the site and terminal nodes; the spatially-correlated fields are
    // keyed on the site node id, so the nodes are created once and reused for
    // every evaluated position
    NodeContainer nodes;
    nodes.Create(2);

    Ptr<SimpleNetDevice> txDev = CreateObject<SimpleNetDevice>();
    Ptr<SimpleNetDevice> rxDev = CreateObject<SimpleNetDevice>();
    nodes.Get(0)->AddDevice(txDev);
    txDev->SetNode(nodes.Get(0));
    nodes.Get(1)->AddDevice(rxDev);
    rxDev->SetNode(nodes.Get(1));

    Ptr<MobilityModel> siteMob = CreateObject<ConstantPositionMobilityModel>();
    siteMob->SetPosition(Vector(0.0, 0.0, siteHeight));
    nodes.Get(0)->AggregateObject(siteMob);

    Ptr<MobilityModel> terminalMob = CreateObject<ConstantPositionMobilityModel>();
    nodes.Get(1)->AggregateObject(terminalMob);

    // as in the nr REM BeamShape mode: the site uses a 4x4 array of 3GPP
    // TR 38.901 elements with a uniform direct-path beam towards a terminal
    // beamDistance meters in front of the panel, and the map probe uses a
    // 2x2 array of isotropic elements with a fixed quasi-omni beam
    auto createAntennas = [&]() {
        Ptr<PhasedArrayModel> txAntenna = CreateObjectWithAttributes<UniformPlanarArray>(
            "NumColumns",
            UintegerValue(4),
            "NumRows",
            UintegerValue(4),
            "AntennaElement",
            PointerValue(CreateObject<ThreeGppAntennaModel>()));
        Ptr<PhasedArrayModel> rxAntenna = CreateObjectWithAttributes<UniformPlanarArray>(
            "NumColumns",
            UintegerValue(2),
            "NumRows",
            UintegerValue(2),
            "AntennaElement",
            PointerValue(CreateObject<IsotropicAntennaModel>()));
        PointBeamTowards(txAntenna,
                         siteMob->GetPosition(),
                         Vector(beamDistance, 0.0, terminalHeight),
                         taper);
        CreateQuasiOmniBf(rxAntenna);
        return std::make_pair(txAntenna, rxAntenna);
    };

    // create the tx power spectral density and the noise power spectral density
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
    double txPow_w = std::pow(10., (txPow_dbm - 30) / 10);
    (*txPsd) = txPow_w / (numRb * rbWidthHz);

    // noise psd, taken from lte-spectrum-value-helper
    const double kT_dBm_Hz = -174.0; // dBm/Hz
    double kT_W_Hz = std::pow(10.0, (kT_dBm_Hz - 30) / 10.0);
    Ptr<SpectrumValue> noisePsd = Create<SpectrumValue>(spectrumModel);
    (*noisePsd) = kT_W_Hz * std::pow(10.0, noiseFigure / 10.0);

    std::string outputFile = "three-gpp-inter-ue-spatial-consistency-" + simTag + ".out";
    std::ofstream f(outputFile, std::ios::out | std::ios::trunc);
    NS_ABORT_MSG_IF(!f.is_open(), "Can't open file " << outputFile);

    for (uint32_t i = 0; i < xRes; i++)
    {
        double x = xMin + i * (xMax - xMin) / (xRes - 1);
        for (uint32_t j = 0; j < yRes; j++)
        {
            double y = yMin + j * (yMax - yMin) / (yRes - 1);
            terminalMob->SetPosition(Vector(x, y, terminalHeight));

            // re-create the models for every evaluated position, so that each
            // point is an independent drop (as a REM generator does). The antenna
            // arrays are re-created too, since a PhasedArrayModel pair tracks
            // whether its channel matrix is up to date and a new channel model
            // expects a not-yet-generated pair.
            auto [txAntenna, rxAntenna] = createAntennas();
            Ptr<ChannelConditionModel> condModel;
            Ptr<ThreeGppPropagationLossModel> lossModel;
            if (scenario == "UMa")
            {
                condModel = CreateObject<ThreeGppUmaChannelConditionModel>();
                lossModel = CreateObject<ThreeGppUmaPropagationLossModel>();
            }
            else if (scenario == "UMi")
            {
                condModel = CreateObject<ThreeGppUmiStreetCanyonChannelConditionModel>();
                lossModel = CreateObject<ThreeGppUmiStreetCanyonPropagationLossModel>();
            }
            else if (scenario == "RMa")
            {
                condModel = CreateObject<ThreeGppRmaChannelConditionModel>();
                lossModel = CreateObject<ThreeGppRmaPropagationLossModel>();
            }
            else
            {
                NS_ABORT_MSG("Unknown scenario " << scenario << ", use UMa, UMi or RMa");
            }
            if (condition == "LOS")
            {
                condModel = CreateObject<AlwaysLosChannelConditionModel>();
            }
            else if (condition == "NLOS")
            {
                condModel = CreateObject<NeverLosChannelConditionModel>();
            }
            else if (condition != "Probabilistic")
            {
                NS_ABORT_MSG("Unknown condition " << condition
                                                  << ", use Probabilistic, LOS or NLOS");
            }
            // the channel condition model is the single switch of the spatial
            // consistency, consulted by the loss and channel models
            condModel->SetAttribute("InterUeSpatialConsistency", BooleanValue(spatialConsistency));
            lossModel->SetAttribute("Frequency", DoubleValue(frequency));
            lossModel->SetAttribute("ShadowingEnabled", BooleanValue(shadowing));
            lossModel->SetAttribute("ChannelConditionModel", PointerValue(condModel));

            Ptr<ThreeGppChannelModel> channelModel = CreateObject<ThreeGppChannelModel>();
            channelModel->SetAttribute("Scenario", StringValue(channelScenario));
            channelModel->SetAttribute("Frequency", DoubleValue(frequency));
            channelModel->SetAttribute("ChannelConditionModel", PointerValue(condModel));
            channelModel->SetAttribute("LargeBandwidthArrayModeling", BooleanValue(largeBandwidth));
            channelModel->SetAttribute("ChannelBandwidth", DoubleValue(numRb * rbWidthHz));

            Ptr<ThreeGppSpectrumPropagationLossModel> spectrumLossModel =
                CreateObjectWithAttributes<ThreeGppSpectrumPropagationLossModel>(
                    "ChannelModel",
                    PointerValue(channelModel));

            Ptr<ChannelCondition> cond = condModel->GetChannelCondition(siteMob, terminalMob);

            // apply the pathloss and the shadow fading
            double propagationGainDb = lossModel->CalcRxPower(0, siteMob, terminalMob);
            Ptr<SpectrumSignalParameters> txParams = Create<SpectrumSignalParameters>();
            txParams->psd = txPsd->Copy();
            *(txParams->psd) *= std::pow(10.0, propagationGainDb / 10.0);

            // apply the fast fading and the beamforming gain
            auto rxParams =
                spectrumLossModel->CalcRxPowerSpectralDensity(txParams,
                                                              siteMob,
                                                              terminalMob,
                                                              txAntenna,
                                                              rxAntenna,
                                                              txAntenna->GetBeamformingVector(),
                                                              rxAntenna->GetBeamformingVector());

            double snrDb = 10 * log10(Sum(*rxParams->psd) / Sum(*noisePsd));
            f << x << " " << y << " " << snrDb << " " << (cond->IsLos() ? 0 : 1) << std::endl;
        }
    }
    f.close();

    std::cout << "Generated " << outputFile << " (" << xRes << "x" << yRes << " grid, scenario "
              << scenario << ", shadowing " << (shadowing ? "on" : "off")
              << ", spatial consistency " << (spatialConsistency ? "on" : "off")
              << ", large bandwidth modeling " << (largeBandwidth ? "on" : "off") << ")"
              << std::endl;

    Simulator::Destroy();
    return 0;
}
