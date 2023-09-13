/*
 * Copyright (c) 2022 SIGNET Lab, Department of Information Engineering,
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <ns3/abort.h>
#include <ns3/config.h>
#include <ns3/constant-position-mobility-model.h>
#include <ns3/double.h>
#include <ns3/isotropic-antenna-model.h>
#include <ns3/log.h>
#include <ns3/mobility-helper.h>
#include <ns3/node-container.h>
#include <ns3/pointer.h>
#include <ns3/simulator.h>
#include <ns3/string.h>
#include <ns3/test.h>
#include <ns3/three-gpp-antenna-model.h>
#include <ns3/three-gpp-channel-model.h>
#include <ns3/three-gpp-spectrum-propagation-loss-model.h>
#include <ns3/two-ray-spectrum-propagation-loss-model.h>
#include <ns3/uinteger.h>
#include <ns3/uniform-planar-array.h>

#include <array>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TwoRaySplmTestSuite");

/**
 * \ingroup spectrum-tests
 *
 * Test case for the TwoRaySpectrumPropagationLossModel class.
 *
 * Check that the average of the Fluctuating Two Ray (FTR) fading model
 * is consistent with the theoretical value.
 */
class FtrFadingModelAverageTest : public TestCase
{
  public:
    /**
     * Constructor
     */
    FtrFadingModelAverageTest();

    /**
     * Destructor
     */
    ~FtrFadingModelAverageTest() override;

  private:
    /**
     * Build the test scenario
     */
    void DoRun() override;

    /**
     * Compute the average of the FTR squared norm.
     * \param [in] ftrParams The FTR parameters.
     * \returns The average of \c N_MEASUREMENTS FTR samples.
     */
    double FtrSquaredNormAverage(
        const TwoRaySpectrumPropagationLossModel::FtrParams& ftrParams) const;

    /**
     * Compute the expected mean of the FTR squared norm.
     *
     * The expected mean \f$ \mathbb{E} \left[ h^2 \right] \f$ can be computed as
     * \f$ \mathbb{E}\left[ |h|^2 \right] = V_1^2 + V_2^2 + 2*\sigma \f$, where:
     * \f$ \sigma^2 \f$ is the variance of the Gaussian distributed random variables
     * which model the real and complex component of the specular terms and
     * \f$ V_1 \f$ and \f$ V_2 \f$ are the constant amplitudes of the reflected components.ù
     * In turn, this equals to \f$ 2*\sigma^2 \left( 1 + K \right) \f$.
     *
     * See J. M. Romero-Jerez, F. J. Lopez-Martinez, J. F. Paris and A. Goldsmith, "The Fluctuating
     * Two-Ray Fading Model for mmWave Communications," 2016 IEEE Globecom Workshops (GC Wkshps) for
     * further details.
     *
     * \param [in] sigma The sigma parameter of the FTR model.
     * \param [in] k The k parameter of the FTR model.
     * \returns The expected mean.
     */
    constexpr double FtrSquaredNormExpectedMean(double sigma, double k) const;

    /// Number of samples to draw when populating the distribution.
    static constexpr uint32_t N_MEASUREMENTS{100000};

    /**
     * Tolerance for testing FTR's expectations against theoretical values,
     * expressed as a fraction of the expected mean.
     */
    static constexpr double TOLERANCE{1e-2};

    /// Number of different values for each FTR parameter.
    static constexpr uint8_t NUM_VALUES{3};

    /// Maximum value for the m parameter.
    static constexpr uint16_t MAX_M_VALUE{1000};
};

FtrFadingModelAverageTest::FtrFadingModelAverageTest()
    : TestCase("Check that the average of the Fluctuating Two Ray model is consistent with the "
               "theoretical expectation")
{
}

FtrFadingModelAverageTest::~FtrFadingModelAverageTest()
{
}

double
FtrFadingModelAverageTest::FtrSquaredNormAverage(
    const TwoRaySpectrumPropagationLossModel::FtrParams& ftrParams) const
{
    NS_LOG_FUNCTION(this);
    double sum = 0.0;
    auto twoRaySplm = CreateObject<TwoRaySpectrumPropagationLossModel>();
    twoRaySplm->AssignStreams(1);
    for (uint32_t i = 0; i < N_MEASUREMENTS; ++i)
    {
        double value = twoRaySplm->GetFtrFastFading(ftrParams);
        sum += value;
    }
    double valueMean = sum / N_MEASUREMENTS;
    return valueMean;
}

double constexpr FtrFadingModelAverageTest::FtrSquaredNormExpectedMean(double sigma, double k) const
{
    return 2 * sigma * (1 + k);
}

void
FtrFadingModelAverageTest::DoRun()
{
    std::array<double, NUM_VALUES> sigma;
    std::array<double, NUM_VALUES> k;
    std::array<double, NUM_VALUES> delta;

    // Generate a set of values for the FTR model parameters
    for (uint8_t j = 0; j < NUM_VALUES; j++)
    {
        double power = std::pow(2, j);

        sigma[j] = power;
        k[j] = power;
        delta[j] = double(j) / NUM_VALUES; // Delta must be in [0, 1]
    }

    auto unifRv = CreateObject<UniformRandomVariable>();

    // Check the consistency of the empirical average for a set of values of the FTR model
    // parameters
    for (uint8_t l = 0; l < NUM_VALUES; l++)
    {
        for (uint8_t m = 0; m < NUM_VALUES; m++)
        {
            for (uint8_t n = 0; n < NUM_VALUES; n++)
            {
                auto ftrParams = TwoRaySpectrumPropagationLossModel::FtrParams(
                    unifRv->GetInteger(1, MAX_M_VALUE),
                    sigma[l], // Average should be independent from m
                    k[m],
                    delta[n]);
                double valueMean = FtrSquaredNormAverage(ftrParams);
                double expectedMean = FtrSquaredNormExpectedMean(ftrParams.m_sigma, ftrParams.m_k);

                NS_TEST_ASSERT_MSG_EQ_TOL(valueMean,
                                          expectedMean,
                                          expectedMean * TOLERANCE,
                                          "wrong mean value");
            }
        }
    }
}

/**
 * \ingroup spectrum-tests
 *
 * Test case for the TwoRaySpectrumPropagationLossModel class.
 *
 * Check that the overall array response at boresight coincides with the
 * expected theoretical values
 */
class ArrayResponseTest : public TestCase
{
  public:
    /**
     * The constructor of the test case
     * \param txAntElem the antenna element of the TX antenna panel
     * \param rxAntElem the antenna element of the RX antenna panel
     * \param txNumAntennas the number of antenna elements of the TX antenna panel
     * \param rxNumAntennas the number of antenna elements of the RX antenna panel
     * \param txPosVec the position of the TX
     * \param rxPosVec the position of the RX
     * \param txBearing the bearing angle of the TX antenna panel
     * \param rxBearing the bearing angle of the RX antenna panel
     * \param expectedGain the theoretically expected gain for the above parameters
     */
    ArrayResponseTest(Ptr<AntennaModel> txAntElem,
                      Ptr<AntennaModel> rxAntElem,
                      uint16_t txNumAntennas,
                      uint16_t rxNumAntennas,
                      Vector txPosVec,
                      Vector rxPosVec,
                      double txBearing,
                      double rxBearing,
                      double expectedGain);

    /**
     * Destructor
     */
    ~ArrayResponseTest() override;

  private:
    /**
     * Build the test scenario
     */
    void DoRun() override;

    /**
     * Tolerance for testing value produced by the simulator against
     * expected theoretical value, as a fraction of the expected value.
     */
    static constexpr double TOLERANCE{1e-8};

    Ptr<AntennaModel> m_txAntElem; //!< the antenna element of the TX antenna panel
    Ptr<AntennaModel> m_rxAntElem; //!< the antenna element of the RX antenna panel
    uint16_t m_txNumAntennas;      //!< the number of antenna elements of the TX antenna panel
    uint16_t m_rxNumAntennas;      //!< the number of antenna elements of the RX antenna panel
    Vector m_txPosVec;             //!< the position of the TX
    Vector m_rxPosVec;             //!< the position of the RX
    double m_txBearing;            //!< the bearing angle of the TX antenna panel [rad]
    double m_rxBearing;            //!< the bearing angle of the RX antenna panel [rad]
    double m_expectedGain;         //!< the gain which is theoretically expected  [db]
};

ArrayResponseTest::ArrayResponseTest(Ptr<AntennaModel> txAntElem,
                                     Ptr<AntennaModel> rxAntElem,
                                     uint16_t txNumAntennas,
                                     uint16_t rxNumAntennas,
                                     Vector txPosVec,
                                     Vector rxPosVec,
                                     double txBearing,
                                     double rxBearing,
                                     double expectedGain)
    // TODO: Create a string with the test parameters as the test case name like in
    // test-uniform-planar-array ?
    : TestCase("Check that the overall array response gain has the proper trend with respect to"
               "the number of antennas and the type of single element antenna"),
      m_txAntElem(txAntElem),
      m_rxAntElem(rxAntElem),
      m_txNumAntennas(txNumAntennas),
      m_rxNumAntennas(rxNumAntennas),
      m_txPosVec(txPosVec),
      m_rxPosVec(rxPosVec),
      m_txBearing(txBearing),
      m_rxBearing(rxBearing),
      m_expectedGain(expectedGain)
{
}

ArrayResponseTest::~ArrayResponseTest()
{
}

void
ArrayResponseTest::DoRun()
{
    auto twoRaySplm = CreateObject<TwoRaySpectrumPropagationLossModel>();
    twoRaySplm->AssignStreams(1);

    // Create and assign the channel condition model
    auto channelConditionModel = CreateObject<AlwaysLosChannelConditionModel>();
    twoRaySplm->SetAttribute("ChannelConditionModel", PointerValue(channelConditionModel));

    // Create the TX and RX antenna arrays
    auto txArray = CreateObject<UniformPlanarArray>();
    auto rxArray = CreateObject<UniformPlanarArray>();
    txArray->SetAttribute("AntennaElement", PointerValue(m_txAntElem));
    rxArray->SetAttribute("AntennaElement", PointerValue(m_rxAntElem));

    // Create the corresponding mobility models
    auto txPos = CreateObject<ConstantPositionMobilityModel>();
    auto rxPos = CreateObject<ConstantPositionMobilityModel>();
    txPos->SetAttribute("Position", VectorValue(m_txPosVec));
    rxPos->SetAttribute("Position", VectorValue(m_rxPosVec));

    // Rotate the arrays
    txArray->SetAttribute("BearingAngle", DoubleValue(m_txBearing));
    rxArray->SetAttribute("BearingAngle", DoubleValue(m_rxBearing));

    // Set the antenna arrays dimensions. Arrays are assumed to be squared.
    txArray->SetAttribute("NumRows", UintegerValue(std::sqrt(m_txNumAntennas)));
    txArray->SetAttribute("NumColumns", UintegerValue(std::sqrt(m_txNumAntennas)));
    rxArray->SetAttribute("NumRows", UintegerValue(std::sqrt(m_rxNumAntennas)));
    rxArray->SetAttribute("NumColumns", UintegerValue(std::sqrt(m_rxNumAntennas)));

    // Compute the beamforming vectors
    auto txBfVec = txArray->GetBeamformingVector(Angles(m_rxPosVec, m_txPosVec));
    auto rxBfVec = rxArray->GetBeamformingVector(Angles(m_txPosVec, m_rxPosVec));
    txArray->SetBeamformingVector(txBfVec);
    rxArray->SetBeamformingVector(rxBfVec);

    // Compute the overall array response
    double gainTxRx = twoRaySplm->CalcBeamformingGain(txPos, rxPos, txArray, rxArray);
    double gainRxTx = twoRaySplm->CalcBeamformingGain(rxPos, txPos, rxArray, txArray);

    NS_TEST_EXPECT_MSG_EQ_TOL(gainTxRx, gainRxTx, gainTxRx * TOLERANCE, "gain should be symmetric");
    NS_TEST_EXPECT_MSG_EQ_TOL(10 * log10(gainTxRx),
                              m_expectedGain,
                              m_expectedGain * TOLERANCE,
                              "gain different from the theoretically expected value");
}

/**
 * \ingroup spectrum-tests
 *
 * Test case for the TwoRaySpectrumPropagationLossModel class.
 *
 * Check that the average overall channel gain obtained using the
 * TwoRaySpectrumPropagationLossModel class is close (it is, after all,
 * a simplified and performance-oriented model) to the one obtained using
 * the ThreeGppSpectrumPropagationLossModel
 */
class OverallGainAverageTest : public TestCase
{
  public:
    /**
     * The constructor of the test case
     * \param txAntElem the antenna element of the TX antenna panel
     * \param rxAntElem the antenna element of the RX antenna panel
     * \param txNumAntennas the number of antenna elements of the TX antenna panel
     * \param rxNumAntennas the number of antenna elements of the RX antenna panel
     * \param fc the carrier frequency
     * \param threeGppScenario the 3GPP scenario
     */
    OverallGainAverageTest(Ptr<AntennaModel> txAntElem,
                           Ptr<AntennaModel> rxAntElem,
                           uint16_t txNumAntennas,
                           uint16_t rxNumAntennas,
                           double fc,
                           std::string threeGppScenario);

    /**
     * Computes the overall power of a PSD
     *
     * \param psd the PSD
     * \returns the overall power of the PSD, obtained as the integral of the
     *          sub-bands power over the PSD domain
     */
    double ComputePowerSpectralDensityOverallPower(Ptr<const SpectrumValue> psd);

    /**
     * Creates a PSD centered at fc, of bandwidth bw and sub-bands of
     * width rbWidth
     *
     * \param fc the central frequency of the PSD
     * \returns a PSD centered at fc, of bandwidth bw and sub-bands of
     *          width rbWidths
     */
    Ptr<SpectrumValue> CreateTxPowerSpectralDensity(double fc);

    /**
     * Destructor
     */
    ~OverallGainAverageTest() override;

  private:
    /**
     * Build the test scenario
     */
    void DoRun() override;

    /**
     * Tolerance for testing average channel gain produced by the Two Ray model with
     * respect to the 3GPP model, which serves as the reference.
     */
    static constexpr double TOLERANCE{0.02};

    /// Number of samples to draw when estimating the average.
    static constexpr uint32_t N_MEASUREMENTS{1000};

    /// The simulation bandwidth. Results are independent from this parameter.
    static constexpr double M_BW{200e6};

    /**
     * The width of a RB, which in turn specifies the resolution of the generated PSDs.
     *  Results are independent from this parameter.
     */
    static constexpr double M_RB_WIDTH{1e6};

    Ptr<AntennaModel> m_txAntElem;  //!< the antenna element of the TX antenna panel
    Ptr<AntennaModel> m_rxAntElem;  //!< the antenna element of the RX antenna panel
    uint16_t m_txNumAntennas;       //!< the number of antenna elements of the TX antenna panel
    uint16_t m_rxNumAntennas;       //!< the number of antenna elements of the RX antenna panel
    double m_fc;                    //!< the carrier frequency
    std::string m_threeGppScenario; //!< the 3GPP scenario
};

OverallGainAverageTest::OverallGainAverageTest(Ptr<AntennaModel> txAntElem,
                                               Ptr<AntennaModel> rxAntElem,
                                               uint16_t txNumAntennas,
                                               uint16_t rxNumAntennas,
                                               double fc,
                                               std::string threeGppScenario)
    // TODO: Create a string with the test parameters as the test case name like in
    // test-uniform-planar-array ?
    : TestCase("Check that the overall array response gain has the proper trend with respect to"
               "the number of antennas and the type of single element antenna"),
      m_txAntElem(txAntElem),
      m_rxAntElem(rxAntElem),
      m_txNumAntennas(txNumAntennas),
      m_rxNumAntennas(rxNumAntennas),
      m_fc(fc),
      m_threeGppScenario(threeGppScenario)
{
}

OverallGainAverageTest::~OverallGainAverageTest()
{
}

double
OverallGainAverageTest::ComputePowerSpectralDensityOverallPower(Ptr<const SpectrumValue> psd)
{
    return Integral(*psd);
}

Ptr<SpectrumValue>
OverallGainAverageTest::CreateTxPowerSpectralDensity(double fc)
{
    uint32_t numRbs = std::floor(M_BW / M_RB_WIDTH);
    double f = fc - (numRbs * M_RB_WIDTH / 2.0);
    double powerTx = 0.0;

    Bands rbs;              // A vector representing each resource block
    std::vector<int> rbsId; // A vector representing the resource block IDs
    rbs.reserve(numRbs);
    rbsId.reserve(numRbs);
    for (uint32_t numrb = 0; numrb < numRbs; ++numrb)
    {
        BandInfo rb;
        rb.fl = f;
        f += M_RB_WIDTH / 2;
        rb.fc = f;
        f += M_RB_WIDTH / 2;
        rb.fh = f;

        rbs.push_back(rb);
        rbsId.push_back(numrb);
    }
    Ptr<SpectrumModel> model = Create<SpectrumModel>(rbs);
    Ptr<SpectrumValue> txPsd = Create<SpectrumValue>(model);

    double powerTxW = std::pow(10., (powerTx - 30) / 10);
    double txPowerDensity = powerTxW / M_BW;

    for (const auto& rbId : rbsId)
    {
        (*txPsd)[rbId] = txPowerDensity;
    }

    return txPsd;
}

void
OverallGainAverageTest::DoRun()
{
    // Create the Two Ray and 3GPP SPLMs
    auto twoRaySplm = CreateObject<TwoRaySpectrumPropagationLossModel>();
    auto threeGppSplm = CreateObject<ThreeGppSpectrumPropagationLossModel>();
    auto threeGppChannelModel = CreateObject<ThreeGppChannelModel>();
    auto channelConditionModel = CreateObject<AlwaysLosChannelConditionModel>();
    twoRaySplm->AssignStreams(1);
    threeGppChannelModel->AssignStreams(1);

    // Pass the needed pointers between the various spectrum instances
    threeGppSplm->SetAttribute("ChannelModel", PointerValue(threeGppChannelModel));
    threeGppChannelModel->SetAttribute("ChannelConditionModel",
                                       PointerValue(channelConditionModel));
    twoRaySplm->SetAttribute("ChannelConditionModel", PointerValue(channelConditionModel));

    // Create the TX and RX nodes and mobility models
    NodeContainer nodes;
    nodes.Create(2);
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    Vector txPosVec(0.0, 0.0, 0.0);
    Vector rxPosVec(5.0, 0.0, 0.0);
    positionAlloc->Add(txPosVec);
    positionAlloc->Add(rxPosVec);
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Create the TX antenna array
    auto txArray = CreateObject<UniformPlanarArray>();
    txArray->SetAttribute("AntennaElement", PointerValue(m_txAntElem));

    // Rotate the array
    txArray->SetAttribute("BearingAngle", DoubleValue(0));

    // Set the antenna array dimensions. Arrays are assumed to be squared.
    txArray->SetAttribute("NumRows", UintegerValue(std::sqrt(m_txNumAntennas)));
    txArray->SetAttribute("NumColumns", UintegerValue(std::sqrt(m_txNumAntennas)));

    // Set the channel simulation parameters
    threeGppChannelModel->SetAttribute("Frequency", DoubleValue(m_fc));
    twoRaySplm->SetAttribute("Frequency", DoubleValue(m_fc));
    threeGppChannelModel->SetAttribute("Scenario", StringValue(m_threeGppScenario));
    twoRaySplm->SetAttribute("Scenario", StringValue(m_threeGppScenario));

    // Disable blockage in order to prevent unwanted variations around the mean value
    threeGppChannelModel->SetAttribute("Blockage", BooleanValue(false));

    // Create the TX PSD
    Ptr<SpectrumValue> txPsd = CreateTxPowerSpectralDensity(m_fc);
    double txPower = ComputePowerSpectralDensityOverallPower(txPsd);

    // Create TX signal parameters
    Ptr<SpectrumSignalParameters> signalParams = Create<SpectrumSignalParameters>();
    signalParams->psd = txPsd;

    // Compute and set the TX beamforming vector
    auto txBfVec = txArray->GetBeamformingVector(Angles(rxPosVec, txPosVec));
    txArray->SetBeamformingVector(txBfVec);

    Ptr<MobilityModel> txMob = nodes.Get(0)->GetObject<MobilityModel>();
    Ptr<MobilityModel> rxMob = nodes.Get(1)->GetObject<MobilityModel>();

    double threeGppGainMean = 0;
    double twoRayGainMean = 0;

    // Compute the overall array responses
    for (uint32_t i = 0; i < N_MEASUREMENTS; ++i)
    {
        // Re-create the RX array at each iteration, to force resampling of the 3GPP channel
        auto rxArray = CreateObject<UniformPlanarArray>();

        // Rotate the RX antenna array and set its dimensions
        rxArray->SetAttribute("AntennaElement", PointerValue(m_rxAntElem));
        rxArray->SetAttribute("BearingAngle", DoubleValue(-M_PI));
        rxArray->SetAttribute("NumRows", UintegerValue(std::sqrt(m_rxNumAntennas)));
        rxArray->SetAttribute("NumColumns", UintegerValue(std::sqrt(m_rxNumAntennas)));

        // Compute and set the RX beamforming vector
        auto rxBfVec = rxArray->GetBeamformingVector(Angles(txPosVec, rxPosVec));
        rxArray->SetBeamformingVector(rxBfVec);

        auto twoRayRxPsd =
            twoRaySplm->DoCalcRxPowerSpectralDensity(signalParams, txMob, rxMob, txArray, rxArray);
        auto threeGppRayRxPsd = threeGppSplm->DoCalcRxPowerSpectralDensity(signalParams,
                                                                           txMob,
                                                                           rxMob,
                                                                           txArray,
                                                                           rxArray);
        double twoRayRxPower = ComputePowerSpectralDensityOverallPower(twoRayRxPsd);
        double threeGppRxPower = ComputePowerSpectralDensityOverallPower(threeGppRayRxPsd);

        twoRayGainMean += (twoRayRxPower / txPower);
        threeGppGainMean += (threeGppRxPower / txPower);
    }

    NS_TEST_EXPECT_MSG_EQ_TOL(
        twoRayGainMean / N_MEASUREMENTS,
        threeGppGainMean / N_MEASUREMENTS,
        twoRayGainMean * TOLERANCE,
        "The 3GPP and Two Ray models should provide similar average channel gains");
}

/**
 * \ingroup spectrum-tests
 *
 * Test suite for the TwoRaySpectrumPropagationLossModel class
 */
class TwoRaySplmTestSuite : public TestSuite
{
  public:
    /**
     * Constructor
     */
    TwoRaySplmTestSuite();
};

TwoRaySplmTestSuite::TwoRaySplmTestSuite()
    : TestSuite("two-ray-splm-suite", UNIT)
{
    // Test the GetFtrFastFading function of the TwoRaySpectrumPropagationLossModel class
    AddTestCase(new FtrFadingModelAverageTest, TestCase::QUICK);

    // Test the CalcBeamformingGain function of the TwoRaySpectrumPropagationLossModel class
    auto iso = CreateObject<IsotropicAntennaModel>();
    auto tgpp = CreateObject<ThreeGppAntennaModel>();
    const double maxTgppGain = tgpp->GetGainDb(Angles(0.0, M_PI / 2));

    // Deploy 2 squared antenna arrays which face each other. Steer their beam towards the boresight
    // and check the resulting array gain. SE = single element radiation pattern, N = number of
    // radiating elements, Phi = bearing angle
    //                                 SE tx,   SE rx, N tx, N rx,             position tx, position
    //                                 rx,  Phi tx,  Phi rx,                       expected gain
    AddTestCase(new ArrayResponseTest(iso,
                                      iso,
                                      1,
                                      1,
                                      Vector(0.0, 0.0, 0.0),
                                      Vector(5.0, 0.0, 0.0),
                                      0.0,
                                      -M_PI,
                                      0.0),
                TestCase::QUICK);
    AddTestCase(new ArrayResponseTest(iso,
                                      iso,
                                      4,
                                      1,
                                      Vector(0.0, 0.0, 0.0),
                                      Vector(5.0, 0.0, 0.0),
                                      0.0,
                                      -M_PI,
                                      10 * log10(4)),
                TestCase::QUICK);
    AddTestCase(new ArrayResponseTest(iso,
                                      iso,
                                      16,
                                      1,
                                      Vector(0.0, 0.0, 0.0),
                                      Vector(5.0, 0.0, 0.0),
                                      0.0,
                                      -M_PI,
                                      10 * log10(16)),
                TestCase::QUICK);
    AddTestCase(new ArrayResponseTest(iso,
                                      iso,
                                      64,
                                      1,
                                      Vector(0.0, 0.0, 0.0),
                                      Vector(5.0, 0.0, 0.0),
                                      0.0,
                                      -M_PI,
                                      10 * log10(64)),
                TestCase::QUICK);
    AddTestCase(new ArrayResponseTest(iso,
                                      iso,
                                      4,
                                      4,
                                      Vector(0.0, 0.0, 0.0),
                                      Vector(5.0, 0.0, 0.0),
                                      0.0,
                                      -M_PI,
                                      2 * 10 * log10(4)),
                TestCase::QUICK);
    AddTestCase(new ArrayResponseTest(iso,
                                      iso,
                                      16,
                                      16,
                                      Vector(0.0, 0.0, 0.0),
                                      Vector(5.0, 0.0, 0.0),
                                      0.0,
                                      -M_PI,
                                      2 * 10 * log10(16)),
                TestCase::QUICK);
    AddTestCase(new ArrayResponseTest(iso,
                                      iso,
                                      64,
                                      64,
                                      Vector(0.0, 0.0, 0.0),
                                      Vector(5.0, 0.0, 0.0),
                                      0.0,
                                      -M_PI,
                                      2 * 10 * log10(64)),
                TestCase::QUICK);
    AddTestCase(new ArrayResponseTest(tgpp,
                                      iso,
                                      1,
                                      1,
                                      Vector(0.0, 0.0, 0.0),
                                      Vector(5.0, 0.0, 0.0),
                                      0.0,
                                      -M_PI,
                                      maxTgppGain),
                TestCase::QUICK);
    AddTestCase(new ArrayResponseTest(tgpp,
                                      iso,
                                      4,
                                      1,
                                      Vector(0.0, 0.0, 0.0),
                                      Vector(5.0, 0.0, 0.0),
                                      0.0,
                                      -M_PI,
                                      10 * log10(4) + maxTgppGain),
                TestCase::QUICK);
    AddTestCase(new ArrayResponseTest(tgpp,
                                      iso,
                                      16,
                                      1,
                                      Vector(0.0, 0.0, 0.0),
                                      Vector(5.0, 0.0, 0.0),
                                      0.0,
                                      -M_PI,
                                      10 * log10(16) + maxTgppGain),
                TestCase::QUICK);
    AddTestCase(new ArrayResponseTest(tgpp,
                                      iso,
                                      64,
                                      1,
                                      Vector(0.0, 0.0, 0.0),
                                      Vector(5.0, 0.0, 0.0),
                                      0.0,
                                      -M_PI,
                                      10 * log10(64) + maxTgppGain),
                TestCase::QUICK);
    AddTestCase(new ArrayResponseTest(tgpp,
                                      tgpp,
                                      1,
                                      1,
                                      Vector(0.0, 0.0, 0.0),
                                      Vector(5.0, 0.0, 0.0),
                                      0.0,
                                      -M_PI,
                                      2 * maxTgppGain),
                TestCase::QUICK);
    AddTestCase(new ArrayResponseTest(tgpp,
                                      tgpp,
                                      4,
                                      1,
                                      Vector(0.0, 0.0, 0.0),
                                      Vector(5.0, 0.0, 0.0),
                                      0.0,
                                      -M_PI,
                                      10 * log10(4) + 2 * maxTgppGain),
                TestCase::QUICK);
    AddTestCase(new ArrayResponseTest(tgpp,
                                      tgpp,
                                      16,
                                      1,
                                      Vector(0.0, 0.0, 0.0),
                                      Vector(5.0, 0.0, 0.0),
                                      0.0,
                                      -M_PI,
                                      10 * log10(16) + 2 * maxTgppGain),
                TestCase::QUICK);
    AddTestCase(new ArrayResponseTest(tgpp,
                                      tgpp,
                                      64,
                                      1,
                                      Vector(0.0, 0.0, 0.0),
                                      Vector(5.0, 0.0, 0.0),
                                      0.0,
                                      -M_PI,
                                      10 * log10(64) + 2 * maxTgppGain),
                TestCase::QUICK);
    AddTestCase(new ArrayResponseTest(tgpp,
                                      tgpp,
                                      4,
                                      4,
                                      Vector(0.0, 0.0, 0.0),
                                      Vector(5.0, 0.0, 0.0),
                                      0.0,
                                      -M_PI,
                                      2 * 10 * log10(4) + 2 * maxTgppGain),
                TestCase::QUICK);
    AddTestCase(new ArrayResponseTest(tgpp,
                                      tgpp,
                                      16,
                                      16,
                                      Vector(0.0, 0.0, 0.0),
                                      Vector(5.0, 0.0, 0.0),
                                      0.0,
                                      -M_PI,
                                      2 * 10 * log10(16) + 2 * maxTgppGain),
                TestCase::QUICK);
    AddTestCase(new ArrayResponseTest(tgpp,
                                      tgpp,
                                      64,
                                      64,
                                      Vector(0.0, 0.0, 0.0),
                                      Vector(5.0, 0.0, 0.0),
                                      0.0,
                                      -M_PI,
                                      2 * 10 * log10(64) + 2 * maxTgppGain),
                TestCase::QUICK);

    // Deploy 2 squared antenna arrays which face each other. Steer their beam towards the boresight
    // and check the overall resulting channel gain. SE = single element radiation pattern, N =
    // number of radiating elements, Fc = carrier frequency, Scen = 3GPP scenario
    //                                     SE tx,   SE rx, N tx, N rx,    Fc,    Scen
    AddTestCase(new OverallGainAverageTest(iso, iso, 1, 1, 10e9, "RMa"), TestCase::EXTENSIVE);
    AddTestCase(new OverallGainAverageTest(tgpp, tgpp, 1, 1, 10e9, "RMa"), TestCase::EXTENSIVE);
    AddTestCase(new OverallGainAverageTest(iso, iso, 4, 4, 10e9, "RMa"), TestCase::EXTENSIVE);
    AddTestCase(new OverallGainAverageTest(tgpp, tgpp, 4, 4, 10e9, "RMa"), TestCase::EXTENSIVE);
    AddTestCase(new OverallGainAverageTest(iso, iso, 1, 1, 10e9, "UMa"), TestCase::EXTENSIVE);
    AddTestCase(new OverallGainAverageTest(tgpp, tgpp, 1, 1, 10e9, "UMa"), TestCase::EXTENSIVE);
    AddTestCase(new OverallGainAverageTest(iso, iso, 4, 4, 10e9, "UMa"), TestCase::EXTENSIVE);
    AddTestCase(new OverallGainAverageTest(tgpp, tgpp, 4, 4, 10e9, "UMa"), TestCase::EXTENSIVE);
    AddTestCase(new OverallGainAverageTest(iso, iso, 1, 1, 60e9, "UMi-StreetCanyon"),
                TestCase::EXTENSIVE);
    AddTestCase(new OverallGainAverageTest(tgpp, tgpp, 1, 1, 60e9, "UMi-StreetCanyon"),
                TestCase::EXTENSIVE);
    AddTestCase(new OverallGainAverageTest(iso, iso, 4, 4, 60e9, "UMi-StreetCanyon"),
                TestCase::EXTENSIVE);
    AddTestCase(new OverallGainAverageTest(tgpp, tgpp, 4, 4, 60e9, "UMi-StreetCanyon"),
                TestCase::EXTENSIVE);
    AddTestCase(new OverallGainAverageTest(iso, iso, 1, 1, 60e9, "InH-OfficeOpen"),
                TestCase::EXTENSIVE);
    AddTestCase(new OverallGainAverageTest(tgpp, tgpp, 1, 1, 60e9, "InH-OfficeOpen"),
                TestCase::EXTENSIVE);
    AddTestCase(new OverallGainAverageTest(iso, iso, 4, 4, 60e9, "InH-OfficeOpen"),
                TestCase::EXTENSIVE);
    AddTestCase(new OverallGainAverageTest(tgpp, tgpp, 4, 4, 60e9, "InH-OfficeOpen"),
                TestCase::EXTENSIVE);
    AddTestCase(new OverallGainAverageTest(iso, iso, 1, 1, 100e9, "InH-OfficeMixed"),
                TestCase::EXTENSIVE);
    AddTestCase(new OverallGainAverageTest(tgpp, tgpp, 1, 1, 100e9, "InH-OfficeMixed"),
                TestCase::EXTENSIVE);
    AddTestCase(new OverallGainAverageTest(iso, iso, 4, 4, 100e9, "InH-OfficeMixed"),
                TestCase::EXTENSIVE);
    AddTestCase(new OverallGainAverageTest(tgpp, tgpp, 4, 4, 100e9, "InH-OfficeMixed"),
                TestCase::EXTENSIVE);
}

// Static variable for test initialization
static TwoRaySplmTestSuite myTestSuite;
