/*
 * Copyright (c) 2019 SIGNET Lab, Department of Information Engineering,
 * Copyright (c) 2026, Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/abort.h"
#include "ns3/angles.h"
#include "ns3/boolean.h"
#include "ns3/channel-condition-model.h"
#include "ns3/config.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/constant-velocity-mobility-model.h"
#include "ns3/double.h"
#include "ns3/ism-spectrum-value-helper.h"
#include "ns3/isotropic-antenna-model.h"
#include "ns3/log.h"
#include "ns3/node-container.h"
#include "ns3/pointer.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-signal-parameters.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/three-gpp-antenna-model.h"
#include "ns3/three-gpp-channel-model.h"
#include "ns3/three-gpp-spectrum-propagation-loss-model.h"
#include "ns3/uinteger.h"
#include "ns3/uniform-planar-array.h"

#include <valarray>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ThreeGppChannelTestSuite");

/**
 * @ingroup spectrum-tests
 *
 * Test case for the ThreeGppChannelModel class.
 * 1) check if the channel matrix has the correct dimensions
 * 2) check if the channel matrix is correctly normalized
 */
class ThreeGppChannelMatrixComputationTest : public TestCase
{
  public:
    /**
     *Constructor
     * @param txAntennaElements the number of rows and columns of the antenna array of the
     * transmitter
     * @param rxAntennaElements the number of rows and columns of the antenna array of
     * @param txPorts the number of vertical and horizontal ports of the antenna array
     * of the transmitter
     * @param rxPorts the number of vertical and horizontal ports of the antenna
     * array of the receiver
     */
    ThreeGppChannelMatrixComputationTest(uint32_t txAntennaElements = 2,
                                         uint32_t rxAntennaElements = 2,
                                         uint32_t txPorts = 1,
                                         uint32_t rxPorts = 1);

    /**
     * Destructor
     */
    ~ThreeGppChannelMatrixComputationTest() override;

  private:
    /**
     * Build the test scenario
     */
    void DoRun() override;

    /**
     * Compute the Frobenius norm of the channel matrix and stores it in m_normVector
     * @param channelModel the ThreeGppChannelModel object used to generate the channel matrix
     * @param txMob the mobility model of the first node
     * @param rxMob the mobility model of the second node
     * @param txAntenna the antenna object associated to the first node
     * @param rxAntenna the antenna object associated to the second node
     */
    void DoComputeNorm(Ptr<ThreeGppChannelModel> channelModel,
                       Ptr<MobilityModel> txMob,
                       Ptr<MobilityModel> rxMob,
                       Ptr<PhasedArrayModel> txAntenna,
                       Ptr<PhasedArrayModel> rxAntenna);

    std::vector<double> m_normVector; //!< each element is the norm of a channel realization
    uint32_t m_txAntennaElements{4};  //!< number of rows and columns of tx antenna array
    uint32_t m_rxAntennaElements{4};  //!< number of rows and columns of rx antenna array
    uint32_t m_txPorts{1}; //!< number of horizontal and vertical ports of tx antenna array
    uint32_t m_rxPorts{1}; //!< number of horizontal and vertical ports of rx antenna array
};

ThreeGppChannelMatrixComputationTest::ThreeGppChannelMatrixComputationTest(
    uint32_t txAntennaElements,
    uint32_t rxAntennaElements,
    uint32_t txPorts,
    uint32_t rxPorts)
    : TestCase("Check the dimensions and the norm of the channel matrix")
{
    m_txAntennaElements = txAntennaElements;
    m_rxAntennaElements = rxAntennaElements;
    m_txPorts = txPorts;
    m_rxPorts = rxPorts;
}

ThreeGppChannelMatrixComputationTest::~ThreeGppChannelMatrixComputationTest()
{
}

void
ThreeGppChannelMatrixComputationTest::DoComputeNorm(Ptr<ThreeGppChannelModel> channelModel,
                                                    Ptr<MobilityModel> txMob,
                                                    Ptr<MobilityModel> rxMob,
                                                    Ptr<PhasedArrayModel> txAntenna,
                                                    Ptr<PhasedArrayModel> rxAntenna)
{
    uint64_t txAntennaElements = txAntenna->GetNumElems();
    uint64_t rxAntennaElements = rxAntenna->GetNumElems();

    Ptr<const ThreeGppChannelModel::ChannelMatrix> channelMatrix =
        channelModel->GetChannel(txMob, rxMob, txAntenna, rxAntenna);

    double channelNorm = 0;
    uint16_t numTotalClusters = channelMatrix->m_channel.GetNumPages();
    for (uint16_t cIndex = 0; cIndex < numTotalClusters; cIndex++)
    {
        double clusterNorm = 0;
        for (uint64_t sIndex = 0; sIndex < txAntennaElements; sIndex++)
        {
            for (uint64_t uIndex = 0; uIndex < rxAntennaElements; uIndex++)
            {
                clusterNorm +=
                    std::pow(std::abs(channelMatrix->m_channel(uIndex, sIndex, cIndex)), 2);
            }
        }
        channelNorm += clusterNorm;
    }
    m_normVector.push_back(channelNorm);
}

void
ThreeGppChannelMatrixComputationTest::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    // Build the scenario for the test
    uint32_t updatePeriodMs = 100; // update period in ms

    // create the channel condition model
    Ptr<ChannelConditionModel> channelConditionModel =
        CreateObject<AlwaysLosChannelConditionModel>();

    // create the ThreeGppChannelModel object used to generate the channel matrix
    Ptr<ThreeGppChannelModel> channelModel = CreateObject<ThreeGppChannelModel>();
    channelModel->SetAttribute("Frequency", DoubleValue(60.0e9));
    channelModel->SetAttribute("Scenario", StringValue("RMa"));
    channelModel->SetAttribute("ChannelConditionModel", PointerValue(channelConditionModel));
    channelModel->SetAttribute("UpdatePeriod", TimeValue(MilliSeconds(updatePeriodMs)));
    channelModel->AssignStreams(1);

    // create the tx and rx nodes
    NodeContainer nodes;
    nodes.Create(2);

    // create the tx and rx mobility models and set their positions
    Ptr<MobilityModel> txMob = CreateObject<ConstantPositionMobilityModel>();
    txMob->SetPosition(Vector(0.0, 0.0, 10.0));
    Ptr<ConstantVelocityMobilityModel> rxMob = CreateObject<ConstantVelocityMobilityModel>();
    rxMob->SetPosition(Vector(30.0, 0.0, 10.0));
    rxMob->SetVelocity(Vector(50, 0, 0));

    // associate the nodes and the mobility models
    nodes.Get(0)->AggregateObject(txMob);
    nodes.Get(1)->AggregateObject(rxMob);

    // create the tx and rx antennas and set their dimensions
    Ptr<PhasedArrayModel> txAntenna = CreateObjectWithAttributes<UniformPlanarArray>(
        "NumColumns",
        UintegerValue(m_txAntennaElements),
        "NumRows",
        UintegerValue(m_txAntennaElements),
        "AntennaElement",
        PointerValue(CreateObject<IsotropicAntennaModel>()),
        "NumVerticalPorts",
        UintegerValue(m_txPorts),
        "NumHorizontalPorts",
        UintegerValue(m_txPorts));

    Ptr<PhasedArrayModel> rxAntenna = CreateObjectWithAttributes<UniformPlanarArray>(
        "NumColumns",
        UintegerValue(m_rxAntennaElements),
        "NumRows",
        UintegerValue(m_rxAntennaElements),
        "AntennaElement",
        PointerValue(CreateObject<IsotropicAntennaModel>()),
        "NumVerticalPorts",
        UintegerValue(m_rxPorts),
        "NumHorizontalPorts",
        UintegerValue(m_rxPorts));
    // generate the channel matrix
    Ptr<const ThreeGppChannelModel::ChannelMatrix> channelMatrix =
        channelModel->GetChannel(txMob, rxMob, txAntenna, rxAntenna);

    // check the channel matrix dimensions, expected H[cluster][rx][tx]
    NS_TEST_ASSERT_MSG_EQ(
        channelMatrix->m_channel.GetNumCols(),
        m_txAntennaElements * m_txAntennaElements,
        "The third dimension of H should be equal to the number of tx antenna elements");
    NS_TEST_ASSERT_MSG_EQ(
        channelMatrix->m_channel.GetNumRows(),
        m_rxAntennaElements * m_rxAntennaElements,
        "The second dimension of H should be equal to the number of rx antenna elements");

    // test if the channel matrix is correctly generated
    uint16_t numIt = 2000;
    for (uint16_t i = 0; i < numIt; i++)
    {
        Simulator::Schedule(MilliSeconds(updatePeriodMs * i + i),
                            &ThreeGppChannelMatrixComputationTest::DoComputeNorm,
                            this,
                            channelModel,
                            txMob,
                            rxMob,
                            txAntenna,
                            rxAntenna);
    }

    Simulator::Run();

    // compute the sample mean
    double sampleMean = 0;
    for (auto i : m_normVector)
    {
        sampleMean += i;
    }
    sampleMean /= numIt;

    // compute the sample standard deviation
    double sampleStd = 0;
    for (auto i : m_normVector)
    {
        const double diff = i - sampleMean;
        sampleStd += diff * diff;
    }
    sampleStd = std::sqrt(sampleStd / (numIt - 1));

    NS_TEST_ASSERT_MSG_NE(sampleStd,
                          0,
                          "The STD should be different from zero. Channel matrix not updated");

    if (sampleStd != 0)
    {
        // perform the one sample t-test with a significance level of 0.05 to test
        // the hypothesis "E [|H|^2] = M*N, where |H| indicates the Frobenius norm of
        // H, M is the number of transmit antenna elements, and N is the number of
        // the receive antenna elements"
        double t = (sampleMean - m_txAntennaElements * m_txAntennaElements * m_rxAntennaElements *
                                     m_rxAntennaElements) /
                   (sampleStd / std::sqrt(numIt));

        // Using a significance level of 0.05, we reject the null hypothesis if |t| is
        // greater than the critical value from a t-distribution with df = numIt-1

        NS_TEST_ASSERT_MSG_EQ_TOL(
            std::abs(t),
            0,
            1.65,
            "We reject the hypothesis E[|H|^2] = M*N with a significance level of 0.05");
    }

    Simulator::Destroy();
}

/**
 * @ingroup spectrum-tests
 *
 * Test case for the ThreeGppChannelModel class.
 * It checks if the channel realizations are correctly updated during the
 * simulation.
 */
class ThreeGppChannelMatrixUpdateTest : public TestCase
{
  public:
    /**
     *Constructor
     * @param txAntennaElements the number of rows and columns of the antenna array of the
     * transmitter
     * @param rxAntennaElements the number of rows and columns of the antenna array of
     * @param txPorts the number of vertical and horizontal ports of the antenna array
     * of the transmitter
     * @param rxPorts the number of vertical and horizontal ports of the antenna
     * array of the receiver
     */
    ThreeGppChannelMatrixUpdateTest(uint32_t txAntennaElements = 2,
                                    uint32_t rxAntennaElements = 4,
                                    uint32_t txPorts = 1,
                                    uint32_t rxPorts = 1);

    /**
     * Destructor
     */
    ~ThreeGppChannelMatrixUpdateTest() override;

  private:
    /**
     * Build the test scenario
     */
    void DoRun() override;

    /**
     * This method first slightly moves the user and then calculates the channel matrix.
     * This function is used for channel matrix computation at different time instants
     * and to check if whether the channel matrix is updated following the updatePeriod
     * configuration.
     * @param channelModel the ThreeGppChannelModel object used to generate the channel matrix
     * @param txMob the mobility model of the first node
     * @param rxMob the mobility model of the second node
     * @param txAntenna the antenna object associated to the first node
     * @param rxAntenna the antenna object associated to the second node
     * @param update whether if the channel matrix should be updated or not
     */
    void MoveRxAndComputerSnr(Ptr<ThreeGppChannelModel> channelModel,
                              Ptr<MobilityModel> txMob,
                              Ptr<MobilityModel> rxMob,
                              Ptr<PhasedArrayModel> txAntenna,
                              Ptr<PhasedArrayModel> rxAntenna,
                              bool update);

    Ptr<const ThreeGppChannelModel::ChannelMatrix>
        m_currentChannel;      //!< used by DoGetChannel to store the current channel matrix
    bool m_initialized{false}; //!< tracks whether previous matrix snapshot is set
    ComplexMatrixArray m_previousChannelMatrix; //!< snapshot of previous channel matrix
    uint32_t m_txAntennaElements{4};            //!< number of rows and columns of tx antenna array
    uint32_t m_rxAntennaElements{4};            //!< number of rows and columns of rx antenna array
    uint32_t m_txPorts{1}; //!< number of horizontal and vertical ports of tx antenna array
    uint32_t m_rxPorts{1}; //!< number of horizontal and vertical ports of rx antenna array
};

ThreeGppChannelMatrixUpdateTest::ThreeGppChannelMatrixUpdateTest(uint32_t txAntennaElements,
                                                                 uint32_t rxAntennaElements,
                                                                 uint32_t txPorts,
                                                                 uint32_t rxPorts)
    : TestCase("Check if the channel realizations are correctly updated during the simulation")
{
    m_txAntennaElements = txAntennaElements;
    m_rxAntennaElements = rxAntennaElements;
    m_txPorts = txPorts;
    m_rxPorts = rxPorts;
}

ThreeGppChannelMatrixUpdateTest::~ThreeGppChannelMatrixUpdateTest()
{
}

void
ThreeGppChannelMatrixUpdateTest::MoveRxAndComputerSnr(Ptr<ThreeGppChannelModel> channelModel,
                                                      Ptr<MobilityModel> txMob,
                                                      Ptr<MobilityModel> rxMob,
                                                      Ptr<PhasedArrayModel> txAntenna,
                                                      Ptr<PhasedArrayModel> rxAntenna,
                                                      bool update)
{
    // 3GPP channel update is activated only if the UE displacement is at least 1e-6, hence we
    // add this minor movement to allow triggering of the channel update per the configured channel
    // update period
    rxMob->SetPosition(Vector(rxMob->GetPosition().x + 0.01,
                              rxMob->GetPosition().y + 0.01,
                              rxMob->GetPosition().z));

    // retrieve the channel matrix
    Ptr<const ThreeGppChannelModel::ChannelMatrix> newChannelMatrix =
        channelModel->GetChannel(txMob, rxMob, txAntenna, rxAntenna);

    // For in-place updates, the ChannelMatrix pointer may remain the same.
    // Deep-compare the matrix values with a small tolerance.
    if (!m_initialized)
    {
        m_previousChannelMatrix = newChannelMatrix->m_channel; // deep copy
        m_currentChannel = newChannelMatrix;
        m_initialized = true;
        return;
    }

    // Use absolute tolerance on complex values (magnitude comparison)
    std::complex<double> tol(1e-12, 0.0);
    bool equal = m_previousChannelMatrix.IsAlmostEqual(newChannelMatrix->m_channel, tol);
    bool changed = !equal;

    NS_TEST_ASSERT_MSG_EQ(
        changed,
        update,
        Simulator::Now().GetMilliSeconds()
            << " The channel matrix is not updated respecting the updatePeriod configuration");

    // Update the previous snapshot if an update was expected and occurred
    if (changed)
    {
        m_previousChannelMatrix = newChannelMatrix->m_channel;
        m_currentChannel = newChannelMatrix;
    }
}

void
ThreeGppChannelMatrixUpdateTest::DoRun()
{
    m_initialized = false;
    m_previousChannelMatrix = ComplexMatrixArray();
    m_currentChannel = nullptr;

    // Build the scenario for the test
    uint32_t updatePeriodMs = 100; // update period in ms

    // create the channel condition model
    Ptr<ChannelConditionModel> channelConditionModel =
        CreateObject<AlwaysLosChannelConditionModel>();

    // create the ThreeGppChannelModel object used to generate the channel matrix
    Ptr<ThreeGppChannelModel> channelModel = CreateObject<ThreeGppChannelModel>();
    channelModel->SetAttribute("Frequency", DoubleValue(60.0e9));
    channelModel->SetAttribute("Scenario", StringValue("UMa"));
    channelModel->SetAttribute("ChannelConditionModel", PointerValue(channelConditionModel));
    channelModel->SetAttribute("UpdatePeriod", TimeValue(MilliSeconds(updatePeriodMs)));

    // create the tx and rx nodes
    NodeContainer nodes;
    nodes.Create(2);

    // create the tx and rx mobility models and set their positions
    Ptr<MobilityModel> txMob = CreateObject<ConstantPositionMobilityModel>();
    txMob->SetPosition(Vector(0.0, 0.0, 10.0));
    Ptr<MobilityModel> rxMob = CreateObject<ConstantPositionMobilityModel>();
    rxMob->SetPosition(Vector(100.0, 0.0, 1.6));

    // associate the nodes and the mobility models
    nodes.Get(0)->AggregateObject(txMob);
    nodes.Get(1)->AggregateObject(rxMob);

    // create the tx and rx antennas and set the their dimensions
    Ptr<PhasedArrayModel> txAntenna = CreateObjectWithAttributes<UniformPlanarArray>(
        "NumColumns",
        UintegerValue(m_txAntennaElements),
        "NumRows",
        UintegerValue(m_txAntennaElements),
        "AntennaElement",
        PointerValue(CreateObject<IsotropicAntennaModel>()),
        "NumVerticalPorts",
        UintegerValue(m_txPorts),
        "NumHorizontalPorts",
        UintegerValue(m_txPorts));

    Ptr<PhasedArrayModel> rxAntenna = CreateObjectWithAttributes<UniformPlanarArray>(
        "NumColumns",
        UintegerValue(m_rxAntennaElements),
        "NumRows",
        UintegerValue(m_rxAntennaElements),
        "AntennaElement",
        PointerValue(CreateObject<IsotropicAntennaModel>()),
        "NumVerticalPorts",
        UintegerValue(m_rxPorts),
        "NumHorizontalPorts",
        UintegerValue(m_rxPorts));

    // check if the channel matrix is correctly updated

    // compute the channel matrix for the first time
    uint32_t firstTimeMs =
        1; // time instant at which the channel matrix is generated for the first time
    Simulator::Schedule(MilliSeconds(firstTimeMs),
                        &ThreeGppChannelMatrixUpdateTest::MoveRxAndComputerSnr,
                        this,
                        channelModel,
                        txMob,
                        rxMob,
                        txAntenna,
                        rxAntenna,
                        true);

    // call GetChannel before the update period is exceeded, the channel matrix
    // should not be updated
    Simulator::Schedule(MilliSeconds(firstTimeMs + updatePeriodMs / 2),
                        &ThreeGppChannelMatrixUpdateTest::MoveRxAndComputerSnr,
                        this,
                        channelModel,
                        txMob,
                        rxMob,
                        txAntenna,
                        rxAntenna,
                        false);

    // call GetChannel when the update period is exceeded, the channel matrix
    // should be recomputed
    Simulator::Schedule(MilliSeconds(firstTimeMs + updatePeriodMs + 1),
                        &ThreeGppChannelMatrixUpdateTest::MoveRxAndComputerSnr,
                        this,
                        channelModel,
                        txMob,
                        rxMob,
                        txAntenna,
                        rxAntenna,
                        true);

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup spectrum-tests
 *
 * Test case for channel consistency Procedure A (according to 3GPP 38.901 Section 7.6.3.)
 *
 * This test verifies that the consistent channel updates produce smoother changes in the channel
 * matrix than generating a completely new channel. The test checks both channel condition LOS and
 * NLOS. This is very important because, according to the Procedure A, the per-cluster variables
 * updates are performed differently for LOS and NLOS. For example, there are different formulas for
 * different channel conditions for cluster delay update (eq. 7.6-9) and cluster departure/arrival
 * angles (eq. 7.6-10b and 7.6-10c). The test allows configuring different speeds and frequencies.
 * Based on these parameters is calculated the channel coherence time and the channel update period
 * is set based on this value.
 *
 * The test verifies that the evolution of the general metric like the Frobenius channel norm, and
 * per-cluster variables such as cluster delay, cluster power, and cluster angles (AOA and ZOA) is
 * smoother with channel updates than with the new channel realizations. For the cluster metrics is
 * selected the third cluster as defined in Table 7.8.5 for the calibration of the channel
 * consistency.
 *
 * The test checks that the channel consistent updates according 3GPP 38.901 Procedure A result in a
 * channel that evolves more smoothly than generating a completely new channel. Variations are at
 * least 5 times lower than these of new channel realizations.
 *
 * To be able to get the results not only for the updated channel but also for the new channel
 * realizations, we create new nodes that are tracking the update positions of the original node for
 * which the channel updates are being generated. New nodes are forcing new channel realizations.
 */
class ThreeGppChannelConsistencyTest : public TestCase
{
  public:
    /**
     *Constructor
     */
    ThreeGppChannelConsistencyTest(ChannelCondition::LosConditionValue losCondition,
                                   double speedKmh,
                                   double freqHz);

    /**
     * Destructor
     */
    ~ThreeGppChannelConsistencyTest() override;

    /**
     * ChannelConsistencyMetrics structure contains the metrics that are needed to test the channel
     * consistency.
     */
    struct ChannelConsistencyMetrics
    {
        //! the channel norm value
        double channelNorm;
        //! the third cluster power in dB
        double thirdClusterPower_dB;
        //! the third cluster delay
        double thirdClusterDelay;
        //! the third cluster AOA
        double thirdClusterAoa;
        //! the third cluster ZOA
        double thirdClusterZoa;

        /**
         * Operator + for easier calculations of the other metrics (e.g., mean values).
         * @param other the other object of ChannelConsistencyMetrics
         * @return the sum of the metrics
         */
        ChannelConsistencyMetrics operator+(const ChannelConsistencyMetrics& other) const
        {
            return ChannelConsistencyMetrics{channelNorm + other.channelNorm,
                                             thirdClusterPower_dB + other.thirdClusterPower_dB,
                                             thirdClusterDelay + other.thirdClusterDelay,
                                             thirdClusterAoa + other.thirdClusterAoa,
                                             thirdClusterZoa + other.thirdClusterZoa};
        }

        /**
         * Operator += for easier calculations of the other metrics (e.g., mean)
         * @param other the other object of the  channel consistency metrics
         * @return the sum of the metrics
         */
        ChannelConsistencyMetrics operator+=(const ChannelConsistencyMetrics& other)
        {
            this->channelNorm = this->channelNorm + other.channelNorm;
            this->thirdClusterPower_dB = this->thirdClusterPower_dB + other.thirdClusterPower_dB;
            this->thirdClusterDelay = this->thirdClusterDelay + other.thirdClusterDelay;
            this->thirdClusterAoa = this->thirdClusterAoa + other.thirdClusterAoa;
            this->thirdClusterZoa = this->thirdClusterZoa + other.thirdClusterZoa;
            return *this;
        }

        /**
         * Operator / used for easier calculations of the statistics, and test conditions.
         * @param iterations The value with which will be divided each metric.
         * @return the new object with the metrics that are divided by the given value.
         */
        ChannelConsistencyMetrics operator/(double iterations) const
        {
            ChannelConsistencyMetrics result;
            result.channelNorm = this->channelNorm / iterations;
            result.thirdClusterPower_dB = this->thirdClusterPower_dB / iterations;
            result.thirdClusterDelay = this->thirdClusterDelay / iterations;
            result.thirdClusterAoa = this->thirdClusterAoa / iterations;
            result.thirdClusterZoa = this->thirdClusterZoa / iterations;
            return result;
        }

        /**
         * Operator /= used for easier calculations of the statistics, and test conditions.
         * @param iterations The value with which will be divided each metric.
         * @return the new object with the metrics that are divided by the given value.s
         */
        ChannelConsistencyMetrics operator/=(double iterations)
        {
            *this = *this / iterations;
            return *this;
        }

        /**
         *Used for easier calculations of the statistics, and test conditions.
         *@return the new object with the absolute values of all the metrics
         */
        ChannelConsistencyMetrics abs() const
        {
            return ChannelConsistencyMetrics{std::abs(channelNorm),
                                             std::abs(thirdClusterPower_dB),
                                             std::abs(thirdClusterDelay),
                                             std::abs(thirdClusterAoa),
                                             std::abs(thirdClusterZoa)};
        }

        /**
         * Used for checking the channel consistency test condition.
         * @param other the values of the object containing the set of consistency metric
         * @return true if the metrics of this object have lower values than these of the other
         * object
         */
        bool operator<(const ChannelConsistencyMetrics& other) const
        {
            return (this->channelNorm < other.channelNorm) &&
                   (this->thirdClusterPower_dB < other.thirdClusterPower_dB) &&
                   (this->thirdClusterDelay < other.thirdClusterDelay) &&
                   (this->thirdClusterAoa < other.thirdClusterAoa) &&
                   (this->thirdClusterZoa < other.thirdClusterZoa);
        }
    };

    /**
     * Used for the calculation of the delta of all the metrics between 2 channel consistency metric
     * objects. Returns a new object containing these delta values for each of the metrics.
     * @param metrics1 The first channel consistency metrics object.
     * @param metrics2 The second channel consistency metrics object.
     * @return The channel consistency object containing the deltas of each of the metrics between
     * the first and the second object.
     */
    ChannelConsistencyMetrics ComputeDeltas(ChannelConsistencyMetrics metrics1,
                                            ChannelConsistencyMetrics metrics2)
    {
        ChannelConsistencyMetrics deltas;
        deltas.channelNorm = metrics1.channelNorm - metrics2.channelNorm;
        deltas.thirdClusterPower_dB = metrics1.thirdClusterPower_dB - metrics2.thirdClusterPower_dB;
        deltas.thirdClusterDelay = metrics1.thirdClusterDelay - metrics2.thirdClusterDelay;
        deltas.thirdClusterAoa = metrics1.thirdClusterAoa - metrics2.thirdClusterAoa;
        deltas.thirdClusterZoa = metrics1.thirdClusterZoa - metrics2.thirdClusterZoa;
        return deltas;
    }

  private:
    /**
     * Build the test scenario
     */
    void DoRun() override;

    /**
     * Compute the channel consistency metrics to evaluate the varying rate of general channel
     * metrics like:
     *    - Frobenius norm of the channel matrix and stores it in m_normVector, and per-cluster
     * metrics like:
     *    - Cluster power (of the third cluster as per Table 7.8.5 from 38.901 for the calibration
     * of the channel consistency)
     *    - Cluster delay (of the third cluster as per Table 7.8.5 from 38.901 for the calibration
     * of the channel consistency)
     *    - Cluster AOA and ZOA (of the third cluster as per Table 7.8.5 from 38.901 for the
     * calibration of the channel consistency)
     *
     * @param channelModel the ThreeGppChannelModel object used to generate the channel matrix
     * @param txMob the mobility model of the first node
     * @param rxMob the mobility model of the second node
     * @param txAntenna the antenna object associated to the first node
     * @param rxAntenna the antenna object associated to the second node
     * @param metricsVectorToBeUsed the metrics vector to be used as the output of the function
     * @param distancesVectorToBeUsed the metrics distances vector to be used as the output of the
     * function
     */
    void DoComputeConsistencyMetrics(
        Ptr<ThreeGppChannelModel> channelModel,
        Ptr<MobilityModel> txMob,
        Ptr<MobilityModel> rxMob,
        Ptr<PhasedArrayModel> txAntenna,
        Ptr<PhasedArrayModel> rxAntenna,
        std::vector<ChannelConsistencyMetrics>* metricsVectorToBeUsed,
        std::vector<ChannelConsistencyMetrics>* distancesVectorToBeUsed);

    //! consistency metrics for the channel updates
    std::vector<ChannelConsistencyMetrics> m_consistencyMetricsChannelUpdates;
    //! consistency metrics for the new channel realizations
    std::vector<ChannelConsistencyMetrics> m_consistencyMetricsNewChannelRealizations;
    //! consistency metric deltas for the channel updates
    std::vector<ChannelConsistencyMetrics> m_consistencyMetricsDeltasChannelUpdates;
    //! consistency metric deltas for the new channel realizations
    std::vector<ChannelConsistencyMetrics> m_consistencyMetricsDeltasNewChannelRealizations;

    uint32_t m_txAntennaElements{4}; //!< number of rows and columns of tx antenna array
    uint32_t m_rxAntennaElements{4}; //!< number of rows and columns of rx antenna array
    //! The type of channel condition to be set; can be LOS or NLOS
    ChannelCondition::LosConditionValue m_losCondition{ChannelCondition::LOS};
    //! The speed of the user to be used in the test case
    double m_speedKmh{30.0};
    //! The carrier frequency to be used in the test case
    double m_freqHz{4.0e9};
};

/**
 * ThreeGppChannelConsistency constructor
 * @param losCondition the LOS condition to be used in the test setup
 * @param speedKmh the speed of the user to be used in the test setup
 * @param freqHz the central carrier frequency to be used in the test setup
 */
ThreeGppChannelConsistencyTest::ThreeGppChannelConsistencyTest(
    ChannelCondition::LosConditionValue losCondition,
    double speedKmh,
    double freqHz)
    : TestCase(std::string("Check the channel consistency feature for losCondition ") +
               ((losCondition == ChannelCondition::LOS) ? "LOS" : "NLOS") +
               ", speed=" + std::to_string(speedKmh) +
               " kmph, and freq=" + std::to_string(freqHz / 1e9) + " GHz.")
{
    m_losCondition = losCondition;
    m_speedKmh = speedKmh;
    m_freqHz = freqHz;
}

ThreeGppChannelConsistencyTest::~ThreeGppChannelConsistencyTest()
{
}

void
ThreeGppChannelConsistencyTest::DoComputeConsistencyMetrics(
    Ptr<ThreeGppChannelModel> channelModel,
    Ptr<MobilityModel> txMob,
    Ptr<MobilityModel> rxMob,
    Ptr<PhasedArrayModel> txAntenna,
    Ptr<PhasedArrayModel> rxAntenna,
    std::vector<ChannelConsistencyMetrics>* metricsVectorToBeUsed,
    std::vector<ChannelConsistencyMetrics>* distancesVectorToBeUsed)
{
    uint64_t txAntennaElements = txAntenna->GetNumElems();
    uint64_t rxAntennaElements = rxAntenna->GetNumElems();

    Ptr<const ThreeGppChannelModel::ChannelMatrix> channelMatrix =
        channelModel->GetChannel(txMob, rxMob, txAntenna, rxAntenna);

    double channelNorm = 0;
    uint16_t numTotalClusters = channelMatrix->m_channel.GetNumPages();
    for (uint16_t cIndex = 0; cIndex < numTotalClusters; cIndex++)
    {
        double clusterNorm = 0;
        for (uint64_t sIndex = 0; sIndex < txAntennaElements; sIndex++)
        {
            for (uint64_t uIndex = 0; uIndex < rxAntennaElements; uIndex++)
            {
                clusterNorm +=
                    std::pow(std::abs(channelMatrix->m_channel(uIndex, sIndex, cIndex)), 2);
            }
        }
        channelNorm += clusterNorm;
    }

    Ptr<const MatrixBasedChannelModel::ChannelParams> baseParams =
        channelModel->GetParams(txMob, rxMob);
    Ptr<const ThreeGppChannelModel::ThreeGppChannelParams> channelParams =
        DynamicCast<const ThreeGppChannelModel::ThreeGppChannelParams>(baseParams);

    double thirdClusterPower_dB = std::numeric_limits<double>::quiet_NaN();
    // delay of the third cluster
    double thirdClusterDelay = std::numeric_limits<double>::quiet_NaN();
    // AOA of the third cluster
    double thirdClusterAoa = std::numeric_limits<double>::quiet_NaN();
    // ZOA of the third cluster
    double thirdClusterZoa = std::numeric_limits<double>::quiet_NaN();

    if (channelParams->m_clusterPower.size() > 2)
    {
        uint8_t thirdClusterIndex = 2; // the index of the third cluster
        thirdClusterPower_dB = 10 * log10(channelParams->m_clusterPower.at(thirdClusterIndex));
        // delay of the third cluster
        thirdClusterDelay = channelParams->m_delay.at(thirdClusterIndex);
        // AOA of the third cluster
        thirdClusterAoa = channelParams->m_angle.at(0).at(thirdClusterIndex);
        // ZOA of the third cluster
        thirdClusterZoa = channelParams->m_angle.at(1).at(thirdClusterIndex);
    }

    metricsVectorToBeUsed->push_back(
        {channelNorm, thirdClusterPower_dB, thirdClusterDelay, thirdClusterAoa, thirdClusterZoa});

    if (metricsVectorToBeUsed->size() > 1)
    {
        ChannelConsistencyMetrics deltas =
            ComputeDeltas(metricsVectorToBeUsed->at(metricsVectorToBeUsed->size() - 2),
                          metricsVectorToBeUsed->at(metricsVectorToBeUsed->size() - 1));
        // we calculate the absolute value of the difference between the last two norms
        distancesVectorToBeUsed->push_back(deltas);
    }
}

void
ThreeGppChannelConsistencyTest::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    double speedMps = m_speedKmh * 1000 / 3600;
    // calculate doppler spread
    double fDoppler = (speedMps * m_freqHz) / 3e8;
    // calculate channel coherence time
    Time cTime = Seconds(1 / fDoppler);
    // set update period
    const Time& updatePeriod = cTime;

    uint16_t iterations = 100;
    m_consistencyMetricsChannelUpdates.reserve(iterations);
    m_consistencyMetricsNewChannelRealizations.reserve(iterations);
    m_consistencyMetricsDeltasChannelUpdates.reserve(iterations - 1);
    m_consistencyMetricsDeltasNewChannelRealizations.reserve(iterations - 1);

    // create the channel condition model
    Ptr<ChannelConditionModel> channelConditionModel;

    if (m_losCondition == ChannelCondition::LOS)
    {
        channelConditionModel = CreateObject<AlwaysLosChannelConditionModel>();
    }
    else
    {
        channelConditionModel = CreateObject<NeverLosChannelConditionModel>();
    }

    // create the ThreeGppChannelModel object used to generate the channel matrix
    Ptr<ThreeGppChannelModel> channelModel = CreateObject<ThreeGppChannelModel>();
    channelModel->SetAttribute("Frequency", DoubleValue(m_freqHz));
    channelModel->SetAttribute("Scenario", StringValue("RMa"));
    channelModel->SetAttribute("ChannelConditionModel", PointerValue(channelConditionModel));
    channelModel->SetAttribute("UpdatePeriod", TimeValue(updatePeriod));
    channelModel->AssignStreams(1);

    // create the tx and rx nodes
    NodeContainer nodeTx;
    NodeContainer nodeRx;
    NodeContainer nodesRxOther;
    // we compare the channel update between the TX1 and RX1, and the independent channel
    // generations between the same TX and 100 different RX that are at the time instant of the
    // channel generation at the same position as the RX1
    nodeTx.Create(1);
    nodeRx.Create(1);
    nodesRxOther.Create(100);

    // create the tx and rx mobility models and set their positions
    Ptr<MobilityModel> txMob = CreateObject<ConstantPositionMobilityModel>();
    txMob->SetPosition(Vector(0.0, 0.0, 10.0));
    Ptr<ConstantVelocityMobilityModel> rxMob = CreateObject<ConstantVelocityMobilityModel>();
    rxMob->SetPosition(Vector(30.0, 0.0, 1.0));
    rxMob->SetVelocity(Vector(speedMps, 0, 0));

    // associate the nodes and the mobility models
    nodeTx.Get(0)->AggregateObject(txMob);
    nodeRx.Get(0)->AggregateObject(rxMob);

    std::vector<Ptr<ConstantVelocityMobilityModel>> rxMobCopies{100};
    rxMobCopies.reserve(100);

    for (uint32_t i = 0; i < 100; i++)
    {
        Ptr<ConstantVelocityMobilityModel> rxMobCopy =
            CreateObject<ConstantVelocityMobilityModel>();
        rxMobCopy->SetPosition(Vector(30.0, 0.0, 1.0));
        rxMobCopy->SetVelocity(Vector(speedMps, 0, 0));
        nodesRxOther.Get(i)->AggregateObject(rxMobCopy);
        rxMobCopies[i] = rxMobCopy;
    }

    // create the tx and rx antennas and set the their dimensions
    Ptr<PhasedArrayModel> txAntenna = CreateObjectWithAttributes<UniformPlanarArray>(
        "NumColumns",
        UintegerValue(m_txAntennaElements),
        "NumRows",
        UintegerValue(m_txAntennaElements),
        "AntennaElement",
        PointerValue(CreateObject<IsotropicAntennaModel>()),
        "BearingAngle",
        DoubleValue(0));

    Ptr<PhasedArrayModel> rxAntenna = CreateObjectWithAttributes<UniformPlanarArray>(
        "NumColumns",
        UintegerValue(m_rxAntennaElements),
        "NumRows",
        UintegerValue(m_rxAntennaElements),
        "AntennaElement",
        PointerValue(CreateObject<IsotropicAntennaModel>()),
        "BearingAngle",
        DoubleValue(M_PI));

    // test if the channel matrix is correctly updated
    for (uint16_t i = 0; i < iterations; i++)
    {
        Simulator::Schedule(updatePeriod * i + NanoSeconds(1),
                            &ThreeGppChannelConsistencyTest::DoComputeConsistencyMetrics,
                            this,
                            channelModel,
                            txMob,
                            rxMob,
                            txAntenna,
                            rxAntenna,
                            &m_consistencyMetricsChannelUpdates,
                            &m_consistencyMetricsDeltasChannelUpdates);

        Simulator::Schedule(updatePeriod * i + NanoSeconds(1),
                            &ThreeGppChannelConsistencyTest::DoComputeConsistencyMetrics,
                            this,
                            channelModel,
                            txMob,
                            rxMobCopies[i],
                            txAntenna,
                            rxAntenna,
                            &m_consistencyMetricsNewChannelRealizations,
                            &m_consistencyMetricsDeltasNewChannelRealizations);
    }

    Simulator::Run();

    // compute the sample mean of the channel updates
    ChannelConsistencyMetrics metricMeanChannelUpdates =
        m_consistencyMetricsDeltasChannelUpdates.at(0).abs();
    ChannelConsistencyMetrics metricMeanNewChannelRealizations =
        m_consistencyMetricsDeltasNewChannelRealizations.at(0).abs();

    for (auto i : m_consistencyMetricsDeltasChannelUpdates)
    {
        metricMeanChannelUpdates += i.abs();
    }
    metricMeanChannelUpdates /= m_consistencyMetricsDeltasChannelUpdates.size();

    for (auto i : m_consistencyMetricsDeltasNewChannelRealizations)
    {
        metricMeanNewChannelRealizations += i.abs();
    }
    metricMeanNewChannelRealizations /= m_consistencyMetricsDeltasNewChannelRealizations.size();

    NS_TEST_ASSERT_MSG_EQ(metricMeanChannelUpdates < metricMeanNewChannelRealizations / 5,
                          true,
                          "Check that the average distances between the consistency metrics of "
                          "the consecutive channel updates "
                          " are lower than of the consecutive new channel generations.");
    Simulator::Destroy();
}

/**
 * @ingroup spectrum-tests
 *
 * Test case for the ThreeGppChannelModel class.
 * It checks if the channel realizations are correctly
 * updated after a change in the number of antenna elements.
 */
class ThreeGppAntennaSetupChangedTest : public TestCase
{
  public:
    /**
     * Constructor
     */
    ThreeGppAntennaSetupChangedTest();

    /**
     * Destructor
     */
    ~ThreeGppAntennaSetupChangedTest() override;

  private:
    /**
     * Build the test scenario
     */
    void DoRun() override;

    /**
     * This method is used to schedule the channel matrix computation at different
     * time instants and to check if it correctly updated
     * @param channelModel the ThreeGppChannelModel object used to generate the channel matrix
     * @param txMob the mobility model of the first node
     * @param rxMob the mobility model of the second node
     * @param txAntenna the antenna object associated to the first node
     * @param rxAntenna the antenna object associated to the second node
     * @param update whether if the channel matrix should be updated or not
     */
    void DoGetChannel(Ptr<ThreeGppChannelModel> channelModel,
                      Ptr<MobilityModel> txMob,
                      Ptr<MobilityModel> rxMob,
                      Ptr<PhasedArrayModel> txAntenna,
                      Ptr<PhasedArrayModel> rxAntenna,
                      bool update);

    Ptr<const ThreeGppChannelModel::ChannelMatrix>
        m_currentChannel;            //!< used by DoGetChannel to store the current channel matrix
    uint32_t m_txAntennaElements{4}; //!< number of rows and columns of tx antenna array
    uint32_t m_rxAntennaElements{4}; //!< number of rows and columns of rx antenna array
    uint32_t m_txPorts{1}; //!< number of horizontal and vertical ports of tx antenna array
    uint32_t m_rxPorts{1}; //!< number of horizontal and vertical ports of rx antenna array
};

ThreeGppAntennaSetupChangedTest::ThreeGppAntennaSetupChangedTest()
    : TestCase("Check if the channel realizations are correctly updated after antenna port changes "
               "during the simulation")
{
}

ThreeGppAntennaSetupChangedTest::~ThreeGppAntennaSetupChangedTest()
{
}

void
ThreeGppAntennaSetupChangedTest::DoGetChannel(Ptr<ThreeGppChannelModel> channelModel,
                                              Ptr<MobilityModel> txMob,
                                              Ptr<MobilityModel> rxMob,
                                              Ptr<PhasedArrayModel> txAntenna,
                                              Ptr<PhasedArrayModel> rxAntenna,
                                              bool update)
{
    // retrieve the channel matrix
    Ptr<const ThreeGppChannelModel::ChannelMatrix> channelMatrix =
        channelModel->GetChannel(txMob, rxMob, txAntenna, rxAntenna);

    if (m_currentChannel)
    {
        // compare the old and the new channel matrices
        NS_TEST_ASSERT_MSG_EQ((m_currentChannel->m_channel != channelMatrix->m_channel),
                              update,
                              Simulator::Now().GetMilliSeconds()
                                  << " The channel matrix is not correctly updated");
    }
    m_currentChannel = channelMatrix;
}

void
ThreeGppAntennaSetupChangedTest::DoRun()
{
    // Build the scenario for the test
    uint32_t updatePeriodMs = 100; // update period in ms

    // create the channel condition model
    Ptr<ChannelConditionModel> channelConditionModel =
        CreateObject<AlwaysLosChannelConditionModel>();

    // create the ThreeGppChannelModel object used to generate the channel matrix
    Ptr<ThreeGppChannelModel> channelModel = CreateObject<ThreeGppChannelModel>();
    channelModel->SetAttribute("Frequency", DoubleValue(60.0e9));
    channelModel->SetAttribute("Scenario", StringValue("UMa"));
    channelModel->SetAttribute("ChannelConditionModel", PointerValue(channelConditionModel));
    channelModel->SetAttribute("UpdatePeriod", TimeValue(MilliSeconds(updatePeriodMs)));

    // create the tx and rx nodes
    NodeContainer nodes;
    nodes.Create(2);

    // create the tx and rx mobility models and set their positions
    Ptr<MobilityModel> txMob = CreateObject<ConstantPositionMobilityModel>();
    txMob->SetPosition(Vector(0.0, 0.0, 10.0));
    Ptr<MobilityModel> rxMob = CreateObject<ConstantPositionMobilityModel>();
    rxMob->SetPosition(Vector(100.0, 0.0, 1.6));

    // associate the nodes and the mobility models
    nodes.Get(0)->AggregateObject(txMob);
    nodes.Get(1)->AggregateObject(rxMob);

    // create the tx and rx antennas and set the their dimensions
    Ptr<PhasedArrayModel> txAntenna = CreateObjectWithAttributes<UniformPlanarArray>(
        "NumColumns",
        UintegerValue(m_txAntennaElements),
        "NumRows",
        UintegerValue(m_txAntennaElements),
        "AntennaElement",
        PointerValue(CreateObject<IsotropicAntennaModel>()),
        "NumVerticalPorts",
        UintegerValue(m_txPorts),
        "NumHorizontalPorts",
        UintegerValue(m_txPorts));

    Ptr<PhasedArrayModel> rxAntenna = CreateObjectWithAttributes<UniformPlanarArray>(
        "NumColumns",
        UintegerValue(m_rxAntennaElements),
        "NumRows",
        UintegerValue(m_rxAntennaElements),
        "AntennaElement",
        PointerValue(CreateObject<IsotropicAntennaModel>()),
        "NumVerticalPorts",
        UintegerValue(m_rxPorts),
        "NumHorizontalPorts",
        UintegerValue(m_rxPorts));

    // check if the channel matrix is correctly updated

    // compute the channel matrix for the first time
    Simulator::Schedule(MilliSeconds(1),
                        &ThreeGppAntennaSetupChangedTest::DoGetChannel,
                        this,
                        channelModel,
                        txMob,
                        rxMob,
                        txAntenna,
                        rxAntenna,
                        true);

    // call GetChannel before the update period is exceeded, the channel matrix
    // should not be updated
    Simulator::Schedule(MilliSeconds(2),
                        &ThreeGppAntennaSetupChangedTest::DoGetChannel,
                        this,
                        channelModel,
                        txMob,
                        rxMob,
                        txAntenna,
                        rxAntenna,
                        false);

    // after changing the number of antenna ports, the channel matrix
    // should be recomputed
    Simulator::Schedule(MilliSeconds(3),
                        [&txAntenna]() { txAntenna->SetNumRows(txAntenna->GetNumRows() + 1); });
    Simulator::Schedule(MilliSeconds(4),
                        &ThreeGppAntennaSetupChangedTest::DoGetChannel,
                        this,
                        channelModel,
                        txMob,
                        rxMob,
                        txAntenna,
                        rxAntenna,
                        true);

    // after recomputing it once, the channel matrix should be cached
    Simulator::Schedule(MilliSeconds(5),
                        &ThreeGppAntennaSetupChangedTest::DoGetChannel,
                        this,
                        channelModel,
                        txMob,
                        rxMob,
                        txAntenna,
                        rxAntenna,
                        false);

    // after recomputing it once, the channel matrix should be cached
    Simulator::Schedule(MilliSeconds(6),
                        [&rxAntenna]() { rxAntenna->SetNumRows(rxAntenna->GetNumRows() + 1); });
    Simulator::Schedule(MilliSeconds(7),
                        &ThreeGppAntennaSetupChangedTest::DoGetChannel,
                        this,
                        channelModel,
                        txMob,
                        rxMob,
                        txAntenna,
                        rxAntenna,
                        true);

    // after recomputing it once, the channel matrix should be cached
    Simulator::Schedule(MilliSeconds(8),
                        &ThreeGppAntennaSetupChangedTest::DoGetChannel,
                        this,
                        channelModel,
                        txMob,
                        rxMob,
                        txAntenna,
                        rxAntenna,
                        false);

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup spectrum-tests
 * @brief A structure that holds the parameters for the function
 * CheckLongTermUpdate. In this way the problem with the limited
 * number of parameters of method Schedule is avoided.
 */
struct CheckLongTermUpdateParams
{
    Ptr<ThreeGppSpectrumPropagationLossModel>
        lossModel; //!< the ThreeGppSpectrumPropagationLossModel object used to compute the rx PSD
    Ptr<SpectrumSignalParameters> txParams; //!< the params of the tx signal
    Ptr<MobilityModel> txMob;               //!< the mobility model of the tx device
    Ptr<MobilityModel> rxMob;               //!< the mobility model of the rx device
    Ptr<SpectrumValue> rxPsdOld;            //!< the previously received PSD
    Ptr<const ComplexMatrixArray>
        rxSpectrumChannelMatrix;     //!< the spectrum channel matrix of the received signal
    Ptr<PhasedArrayModel> txAntenna; //!< the antenna array of the tx device
    Ptr<PhasedArrayModel> rxAntenna; //!< the antenna array of the rx device
};

/**
 * @ingroup spectrum-tests
 *
 * Test case for the ThreeGppSpectrumPropagationLossModelTest class.
 * 1) checks if the long term components for the direct and the reverse link
 *    are the same
 * 2) checks if the long term component is updated when changing the beamforming
 *    vectors
 * 3) checks if the long term is updated when changing the channel matrix
 */
class ThreeGppSpectrumPropagationLossModelTest : public TestCase
{
  public:
    /**
     * Constructor
     * @param txAntennaElements the number of rows and columns of the antenna array of the
     * transmitter
     * @param rxAntennaElements the number of rows and columns of the antenna array of
     * the receiver
     * @param txPorts the number of vertical and horizontal ports of the antenna array
     * of the transmitter
     * @param rxPorts the number of vertical and horizontal ports of the antenna
     * array of the receiver
     */
    ThreeGppSpectrumPropagationLossModelTest(uint32_t txAntennaElements = 4,
                                             uint32_t rxAntennaElements = 4,
                                             uint32_t txPorts = 1,
                                             uint32_t rxPorts = 1);
    /**
     * Destructor
     */
    ~ThreeGppSpectrumPropagationLossModelTest() override;

  private:
    /**
     * Build the test scenario
     */
    void DoRun() override;

    /**
     * Perform beamforming from this node towards the other node
     * @param thisMob the mobility model of this device
     * @param thisAntenna the antenna object associated to thisDevice
     * @param otherMob the mobility model of the other device
     * @param otherAntenna the antenna array object of to otherDevice
     */
    void DoBeamforming(Ptr<MobilityModel> thisMob,
                       Ptr<PhasedArrayModel> thisAntenna,
                       Ptr<MobilityModel> otherMob,
                       Ptr<PhasedArrayModel> otherAntenna);

    /**
     *  Test of the long-term component is correctly updated when the positions and beamforming
     * change
     * @param params a structure that contains the set of parameters needed by
     * CheckUpdateAfterChangingPositionsAndBeamforming in order to perform calculations
     */
    void CheckUpdateAfterChangingPositionsAndBeamforming(const CheckLongTermUpdateParams& params);

    /**
     * Test of the long-term component is correctly updated when the update period expires.
     * This function slightly moves the position of the tx node to allow the update to be performed.
     * @param params a structure that contains the set of parameters needed by CheckLongTermUpdate
     * in order to perform calculations
     */
    void CheckLongTermUpdate(CheckLongTermUpdateParams& params);

    uint32_t m_txAntennaElements{4}; //!< number of rows and columns of tx antenna array
    uint32_t m_rxAntennaElements{4}; //!< number of rows and columns of rx antenna array
    uint32_t m_txPorts{1}; //!< number of horizontal and vertical ports of tx antenna array
    uint32_t m_rxPorts{1}; //!< number of horizontal and vertical ports of rx antenna array
    //! used as the input for the long-term update check after the update period expires
    Ptr<SpectrumSignalParameters> m_rxParamsOld;
};

ThreeGppSpectrumPropagationLossModelTest::ThreeGppSpectrumPropagationLossModelTest(
    uint32_t txAntennaElements,
    uint32_t rxAntennaElements,
    uint32_t txPorts,
    uint32_t rxPorts)
    : TestCase("Test case for the ThreeGppSpectrumPropagationLossModel class")
{
    m_txAntennaElements = txAntennaElements;
    m_rxAntennaElements = rxAntennaElements;
    m_txPorts = txPorts;
    m_rxPorts = rxPorts;
}

ThreeGppSpectrumPropagationLossModelTest::~ThreeGppSpectrumPropagationLossModelTest()
{
}

void
ThreeGppSpectrumPropagationLossModelTest::DoBeamforming(Ptr<MobilityModel> thisMob,
                                                        Ptr<PhasedArrayModel> thisAntenna,
                                                        Ptr<MobilityModel> otherMob,
                                                        Ptr<PhasedArrayModel> otherAntenna)
{
    Vector aPos = thisMob->GetPosition();
    Vector bPos = otherMob->GetPosition();

    // compute the azimuth and the elevation angles
    Angles completeAngle(bPos, aPos);

    PhasedArrayModel::ComplexVector antennaWeights =
        thisAntenna->GetBeamformingVector(completeAngle);
    thisAntenna->SetBeamformingVector(antennaWeights);
}

void
ThreeGppSpectrumPropagationLossModelTest::CheckUpdateAfterChangingPositionsAndBeamforming(
    const CheckLongTermUpdateParams& params)
{
    // Reciprocity check: evaluate reverse direction after changing positions/beamforming.
    auto rxPsdNewParams = params.lossModel->DoCalcRxPowerSpectralDensity(params.txParams,
                                                                         params.rxMob,
                                                                         params.txMob,
                                                                         params.rxAntenna,
                                                                         params.txAntenna);

    NS_TEST_ASSERT_MSG_EQ((*params.rxPsdOld == *rxPsdNewParams->psd),
                          false,
                          "Changing the positions and BF vectors the rx PSD does not change");

    NS_TEST_ASSERT_MSG_EQ(
        (*params.rxSpectrumChannelMatrix == *rxPsdNewParams->spectrumChannelMatrix),
        false,
        "Changing the BF should change de frequency domain channel matrix");

    m_rxParamsOld = rxPsdNewParams;
}

void
ThreeGppSpectrumPropagationLossModelTest::CheckLongTermUpdate(CheckLongTermUpdateParams& params)
{
    // we need to slightly move the node to allow the channel to be updated upon the update period,
    // but less than 1 meter, which would trigger new channel generation
    params.txMob->SetPosition(Vector(params.txMob->GetPosition().x + 0.1,
                                     params.txMob->GetPosition().y,
                                     params.txMob->GetPosition().z));

    auto rxPsdNewParams = params.lossModel->DoCalcRxPowerSpectralDensity(params.txParams,
                                                                         params.txMob,
                                                                         params.rxMob,
                                                                         params.txAntenna,
                                                                         params.rxAntenna);
    NS_TEST_ASSERT_MSG_EQ((*m_rxParamsOld->psd == *rxPsdNewParams->psd),
                          false,
                          "The long term is not updated when the channel matrix is recomputed");
}

void
ThreeGppSpectrumPropagationLossModelTest::DoRun()
{
    // Build the scenario for the test
    Config::SetDefault("ns3::ThreeGppChannelModel::UpdatePeriod", TimeValue(MilliSeconds(100)));

    // create the ChannelConditionModel object to be used to retrieve the
    // channel condition
    Ptr<ChannelConditionModel> condModel = CreateObject<AlwaysLosChannelConditionModel>();

    // create the ThreeGppSpectrumPropagationLossModel object, set frequency,
    // scenario and channel condition model to be used
    Ptr<ThreeGppSpectrumPropagationLossModel> lossModel =
        CreateObject<ThreeGppSpectrumPropagationLossModel>();
    lossModel->SetChannelModelAttribute("Frequency", DoubleValue(2.4e9));
    lossModel->SetChannelModelAttribute("Scenario", StringValue("UMa"));
    lossModel->SetChannelModelAttribute(
        "ChannelConditionModel",
        PointerValue(condModel)); // create the ThreeGppChannelModel object used to generate the
                                  // channel matrix

    // create the tx and rx nodes
    NodeContainer nodes;
    nodes.Create(2);

    // create the tx and rx mobility models and set their positions
    Ptr<MobilityModel> txMob = CreateObject<ConstantPositionMobilityModel>();
    txMob->SetPosition(Vector(0.0, 0.0, 10.0));
    Ptr<MobilityModel> rxMob = CreateObject<ConstantPositionMobilityModel>();
    rxMob->SetPosition(
        Vector(15.0, 0.0, 10.0)); // in this position the channel condition is always LOS

    // associate the nodes and the mobility models
    nodes.Get(0)->AggregateObject(txMob);
    nodes.Get(1)->AggregateObject(rxMob);

    // create the tx and rx antennas and set the their dimensions
    Ptr<PhasedArrayModel> txAntenna = CreateObjectWithAttributes<UniformPlanarArray>(
        "NumColumns",
        UintegerValue(m_txAntennaElements),
        "NumRows",
        UintegerValue(m_txAntennaElements),
        "AntennaElement",
        PointerValue(CreateObject<IsotropicAntennaModel>()),
        "NumVerticalPorts",
        UintegerValue(m_txPorts),
        "NumHorizontalPorts",
        UintegerValue(m_txPorts));

    Ptr<PhasedArrayModel> rxAntenna = CreateObjectWithAttributes<UniformPlanarArray>(
        "NumColumns",
        UintegerValue(m_rxAntennaElements),
        "NumRows",
        UintegerValue(m_rxAntennaElements),
        "AntennaElement",
        PointerValue(CreateObject<IsotropicAntennaModel>()),
        "NumVerticalPorts",
        UintegerValue(m_rxPorts),
        "NumHorizontalPorts",
        UintegerValue(m_rxPorts));

    // set the beamforming vectors
    DoBeamforming(txMob, txAntenna, rxMob, rxAntenna);
    DoBeamforming(rxMob, rxAntenna, txMob, txAntenna);

    // create the tx psd
    SpectrumValue5MhzFactory sf;
    double txPower = 0.1; // Watts
    uint32_t channelNumber = 1;
    Ptr<SpectrumValue> txPsd = sf.CreateTxPowerSpectralDensity(txPower, channelNumber);
    Ptr<SpectrumSignalParameters> txParams = Create<SpectrumSignalParameters>();
    txParams->psd = txPsd->Copy();

    // compute the rx psd
    auto rxParamsOld =
        lossModel->DoCalcRxPowerSpectralDensity(txParams, txMob, rxMob, txAntenna, rxAntenna);

    // 1) check reciprocity: rx PSD is equal for both the direct and the reverse channel
    auto rxParamsNew =
        lossModel->DoCalcRxPowerSpectralDensity(txParams, rxMob, txMob, rxAntenna, txAntenna);
    if ((rxParamsOld->spectrumChannelMatrix->GetNumCols() ==
         rxParamsNew->spectrumChannelMatrix->GetNumCols()) &&
        (rxParamsOld->spectrumChannelMatrix->GetNumRows() ==
         rxParamsNew->spectrumChannelMatrix->GetNumRows()) &&
        (rxParamsOld->spectrumChannelMatrix->GetNumCols() == 1) &&
        (rxParamsOld->spectrumChannelMatrix->GetNumRows() == 1))
    {
        // this is only really true in case of SISO (1x1 non-polarized port array)
        const auto& oldValues = rxParamsOld->psd->GetValues();
        const auto& newValues = rxParamsNew->psd->GetValues();

        NS_TEST_ASSERT_MSG_EQ(oldValues.size(),
                              newValues.size(),
                              "The long term for the direct and the reverse channel are different");
        for (size_t i = 0; i < oldValues.size(); i++)
        {
            NS_TEST_ASSERT_MSG_EQ_TOL(
                oldValues.at(i),
                newValues.at(i),
                1e-9,
                "The long term for the direct and the reverse channel are different");
        }
    }

    // 2) check if the long term is updated when changing the BF vector
    // change the position of the rx device and recompute the beamforming vectors
    rxMob->SetPosition(Vector(10.0, 5.0, 10.0));
    PhasedArrayModel::ComplexVector txBfVector = txAntenna->GetBeamformingVector();
    txBfVector[0] = std::complex<double>(0.0, 0.0);
    txAntenna->SetBeamformingVector(txBfVector);
    CheckLongTermUpdateParams paramsPositionUpdate{lossModel,
                                                   txParams,
                                                   rxMob,
                                                   txMob,
                                                   rxParamsOld->psd,
                                                   rxParamsOld->spectrumChannelMatrix,
                                                   rxAntenna,
                                                   txAntenna};
    Simulator::Schedule(
        MilliSeconds(50),
        &ThreeGppSpectrumPropagationLossModelTest::CheckUpdateAfterChangingPositionsAndBeamforming,
        this,
        paramsPositionUpdate);

    // 3) check if the long term is updated when the channel matrix is recomputed
    CheckLongTermUpdateParams params{lossModel,
                                     txParams,
                                     txMob,
                                     rxMob,
                                     rxParamsOld->psd,
                                     rxParamsOld->spectrumChannelMatrix,
                                     txAntenna,
                                     rxAntenna};
    Simulator::Schedule(MilliSeconds(151),
                        &ThreeGppSpectrumPropagationLossModelTest::CheckLongTermUpdate,
                        this,
                        params);

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup spectrum-tests
 *
 * Test case that test the correct use of the multi-port antennas in spectrum.
 * The test does the following:
 * 1) Generates a time domain channel matrix of a fixed size
 * (num gNB elements = 32 (4x8), num UE elements = 16 (4x4), num Clusters).
 * This matrix is called channelMatrixM0.
 * 2) Divides gNB antenna and UE antenna into ports using a fixed element to port
 * mapping (gNB: 1 vertical port, 4 horizontal ports, UE: 1 vertical port,
 * 2 horizontal ports). Using the channelMatrixM0, it generates port to port
 * long term channel matrix using the both gNB and UE having beams directed towards the other.
 * The resulting long term matrix dimensions are gNBports = 4, UE ports = 2, num Clusters.
 * This channel matrix is called matrixA.
 * 3) Constructs a single port to single port long term channel matrix
 * using the initial time domain channel matrix (channelMatrixM0) and beams
 * from gNB and UE towards each other. Single port mapping means gNB: 1 vertical port,
 * 1 horizontal port, UE: 1 vertical port, 1 horizontal port.
 * Matrix dimensions are: gNBports = 1, UE ports = 1, num Clusters. This long
 * term channel matrix is called matrixB.
 * 4) Creates a single port to single port long term channel matrix between
 * two virtual gNB and UE antenna by using matrixA and beam facing each other.
 * Matrix dimension of the resulting matrix are gNBports = 1, UE ports = 1, num Clusters.
 * This long term channel matrix is called matrixC.
 * 5) Checks that matrixB and matrixC are identical.
 */
class ThreeGppCalcLongTermMultiPortTest : public TestCase
{
  public:
    /**
     * Constructor
     */
    ThreeGppCalcLongTermMultiPortTest();

    /**
     * Destructor
     */
    ~ThreeGppCalcLongTermMultiPortTest() override;

  private:
    /**
     * Build the test scenario
     */
    void DoRun() override;
};

ThreeGppCalcLongTermMultiPortTest::ThreeGppCalcLongTermMultiPortTest()
    : TestCase("Check long term channel matrix generation when multiple ports at TX and RX are "
               "being used.")
{
}

ThreeGppCalcLongTermMultiPortTest::~ThreeGppCalcLongTermMultiPortTest()
{
}

void
ThreeGppCalcLongTermMultiPortTest::DoRun()
{
    // create the channel condition model
    Ptr<ChannelConditionModel> channelConditionModel =
        CreateObject<AlwaysLosChannelConditionModel>();

    // create the ThreeGppChannelModel object used to generate the channel matrix
    Ptr<ThreeGppChannelModel> channelModel = CreateObject<ThreeGppChannelModel>();
    channelModel->SetAttribute("Frequency", DoubleValue(2.0e9));
    channelModel->SetAttribute("Scenario", StringValue("RMa"));
    channelModel->SetAttribute("ChannelConditionModel", PointerValue(channelConditionModel));

    // create the tx and rx nodes
    NodeContainer nodes;
    nodes.Create(2);

    // create the tx and rx mobility models and set their positions
    Ptr<MobilityModel> txMob = CreateObject<ConstantPositionMobilityModel>();
    txMob->SetPosition(Vector(0.0, 0.0, 10.0));
    Ptr<MobilityModel> rxMob = CreateObject<ConstantPositionMobilityModel>();
    rxMob->SetPosition(Vector(10.0, 0.0, 10.0));

    // associate the nodes and the mobility models
    nodes.Get(0)->AggregateObject(txMob);
    nodes.Get(1)->AggregateObject(rxMob);

    // create the tx and rx antennas and set the their dimensions
    Ptr<PhasedArrayModel> txAntenna1 = CreateObjectWithAttributes<UniformPlanarArray>(
        "NumColumns",
        UintegerValue(8),
        "NumRows",
        UintegerValue(4),
        "AntennaElement",
        PointerValue(CreateObject<IsotropicAntennaModel>()),
        "NumVerticalPorts",
        UintegerValue(1),
        "NumHorizontalPorts",
        UintegerValue(4));

    Ptr<PhasedArrayModel> rxAntenna1 = CreateObjectWithAttributes<UniformPlanarArray>(
        "NumColumns",
        UintegerValue(4),
        "NumRows",
        UintegerValue(4),
        "AntennaElement",
        PointerValue(CreateObject<IsotropicAntennaModel>()),
        "NumVerticalPorts",
        UintegerValue(1),
        "NumHorizontalPorts",
        UintegerValue(2));

    // compute the azimuth and the elevation angles
    Angles completeAngleTxRx(rxMob->GetPosition(), txMob->GetPosition());
    Angles completeAngleRxTx(txMob->GetPosition(), rxMob->GetPosition());

    txAntenna1->SetBeamformingVector(txAntenna1->GetBeamformingVector(completeAngleTxRx));
    rxAntenna1->SetBeamformingVector(rxAntenna1->GetBeamformingVector(completeAngleRxTx));

    // generate the channel matrix
    Ptr<const ThreeGppChannelModel::ChannelMatrix> channelMatrixM0 =
        channelModel->GetChannel(txMob, rxMob, txAntenna1, rxAntenna1);

    // create ThreeGppSpectrumPropagationLossModel instance so that we
    // can call CalcLongTerm
    Ptr<const ThreeGppSpectrumPropagationLossModel> threeGppSplm =
        CreateObject<ThreeGppSpectrumPropagationLossModel>();

    Ptr<const MatrixBasedChannelModel::Complex3DVector> matrixA =
        threeGppSplm->CalcLongTerm(channelMatrixM0, txAntenna1, rxAntenna1);

    // create the tx and rx antennas and set the their dimensions
    Ptr<PhasedArrayModel> txAntenna2 = CreateObjectWithAttributes<UniformPlanarArray>(
        "NumColumns",
        UintegerValue(8),
        "NumRows",
        UintegerValue(4),
        "AntennaElement",
        PointerValue(CreateObject<IsotropicAntennaModel>()),
        "NumVerticalPorts",
        UintegerValue(1),
        "NumHorizontalPorts",
        UintegerValue(1));

    Ptr<PhasedArrayModel> rxAntenna2 = CreateObjectWithAttributes<UniformPlanarArray>(
        "NumColumns",
        UintegerValue(4),
        "NumRows",
        UintegerValue(4),
        "AntennaElement",
        PointerValue(CreateObject<IsotropicAntennaModel>()),
        "NumVerticalPorts",
        UintegerValue(1),
        "NumHorizontalPorts",
        UintegerValue(1));

    txAntenna2->SetBeamformingVector(txAntenna2->GetBeamformingVector(completeAngleTxRx));
    rxAntenna2->SetBeamformingVector(rxAntenna2->GetBeamformingVector(completeAngleRxTx));

    Ptr<const MatrixBasedChannelModel::Complex3DVector> matrixB =
        threeGppSplm->CalcLongTerm(channelMatrixM0, txAntenna2, rxAntenna2);

    // create the tx and rx antennas and set the their dimensions
    Ptr<PhasedArrayModel> txAntenna3 = CreateObjectWithAttributes<UniformPlanarArray>(
        "NumColumns",
        UintegerValue(4),
        "NumRows",
        UintegerValue(1),
        "AntennaElement",
        PointerValue(CreateObject<IsotropicAntennaModel>()),
        "NumVerticalPorts",
        UintegerValue(1),
        "NumHorizontalPorts",
        UintegerValue(1),
        "AntennaHorizontalSpacing",
        DoubleValue(1));

    Ptr<PhasedArrayModel> rxAntenna3 = CreateObjectWithAttributes<UniformPlanarArray>(
        "NumColumns",
        UintegerValue(2),
        "NumRows",
        UintegerValue(1),
        "AntennaElement",
        PointerValue(CreateObject<IsotropicAntennaModel>()),
        "NumVerticalPorts",
        UintegerValue(1),
        "NumHorizontalPorts",
        UintegerValue(1),
        "AntennaHorizontalSpacing",
        DoubleValue(1));

    Ptr<ThreeGppChannelModel::ChannelMatrix> channelMatrixMA =
        Create<ThreeGppChannelModel::ChannelMatrix>();
    channelMatrixMA->m_channel = *matrixA;

    txAntenna3->SetBeamformingVector(txAntenna3->GetBeamformingVector(completeAngleTxRx));
    rxAntenna3->SetBeamformingVector(rxAntenna3->GetBeamformingVector(completeAngleRxTx));

    Ptr<const MatrixBasedChannelModel::Complex3DVector> matrixC =
        threeGppSplm->CalcLongTerm(channelMatrixMA, txAntenna3, rxAntenna3);

    NS_TEST_ASSERT_MSG_EQ(matrixB->IsAlmostEqual(*matrixC, 1e-6),
                          true,
                          "Matrix B and Matrix C should be equal.");

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * Structure that contains some of the main configuration parameters of the antenna
 * array that are used in the ThreeGppMimoPolarizationTest
 */
struct MimoPolarizationAntennaParams
{
    uint32_t m_rows = 1;        //!< the number of rows of antenna array
    uint32_t m_cols = 2;        //!< the number of columns of antenna array
    uint32_t m_vPorts = 1;      //!< the number of vertical ports of antenna array
    uint32_t m_hPorts = 2;      //!< the number of horizontal ports of antenna array
    bool m_isotropic = false;   //!< defines whether the antenna elements are isotropic
    double m_polSlantAngle = 0; //!< polarization angle of the antenna array
    double m_bearingAngle = 0;  //!< bearing angle of the antenna array

    /**
     * Constructor
     * Currently only configurable through constructor are polSlantAngle and bearingAngle.
     * @param isotropic whether the antenna elements are isotropic, or 3GPP
     * @param polSlantAngle the polarization slant angle
     * @param bearingAngle the bearing angle
     */
    MimoPolarizationAntennaParams(bool isotropic, double polSlantAngle = 0, double bearingAngle = 0)
        : m_isotropic(isotropic),
          m_polSlantAngle(polSlantAngle),
          m_bearingAngle(bearingAngle)
    {
    }
};

/**
 * @ingroup spectrum-tests
 * This test tests that the channel matrix is correctly generated when dual-polarized
 * antennas are being used at TX and RX. In the conditions in which the channel between
 * the TX and Rx device is LOS channel, and the beams of the transmitter and the
 * receiver are pointing one towards the other, then in the presence of multiple ports
 * at the TX and RX, and the antenna array at the TX and RX are dual polarized,
 * the channel matrix should exhibit the strong symmetry between the two polarizations.
 * E.g. if we have 1x2 antenna elements and two polarizations at both TX and RX,
 * and the 1x2 ports at the TX and RX, then the channel matrix will have the
 * structure as:
 *
 *                ch00 ch01 |ch02 ch03
 *   Hvv  Hvh     ch10 ch11 |ch12 ch13
 *             =  --------------------
 *   Hhv  Hhh     ch20 ch21 |ch22 ch23
 *                ch30 ch31 |ch32 ch33
 *
 * We test different cases of the polarization slant angles of the TX and RX,
 * e.g., 0, 30, 45, 90.
 * In each of these setups we check if the channel matrix in its strongest
 * cluster experiences strong symmetry, and if the values appear in pairs.
 * We also test additional cases in which we change the bearing angle and
 * the height of the TX. In these cases we also observe strong symmetry, with
 * the difference that in these cases we can observe different values in the
 * pairs. We can still observe strong impact of the dual polarization on the
 * channel matrix.
 */
class ThreeGppMimoPolarizationTest : public TestCase
{
  public:
    /**
     * Constructor that receives MIMO polarization parameters of TX and RX
     * devices
     * @param testCaseName the test case name
     * @param txLoc the position of the transmitter
     * @param txAntennaParams the antenna parameters of the transmitter
     * @param rxLoc the position of the receiver
     * @param rxAntennaParams the antenna parameters of the receiver
     * @param testChannel the test matrix that represent the strongest cluster
     * @param tolerance the tolerance to be used when testing
     */
    ThreeGppMimoPolarizationTest(std::string testCaseName,
                                 Vector txLoc,
                                 const MimoPolarizationAntennaParams& txAntennaParams,
                                 Vector rxLoc,
                                 const MimoPolarizationAntennaParams& rxAntennaParams,
                                 std::valarray<std::complex<double>> testChannel,
                                 double tolerance);

    /**
     * Destructor
     */
    ~ThreeGppMimoPolarizationTest() override;

  private:
    /**
     * Build the test scenario
     */
    void DoRun() override;

    /**
     * @brief Function that can be used to configure the antenna using the set of
     * parameters.
     *
     * @param params The parameters to be set to the antenna
     * @return A pointer to the antenna that is created and configured by using input params
     */
    Ptr<PhasedArrayModel> CreateAndConfigureAntenna(const MimoPolarizationAntennaParams& params);

    Vector m_txLoc;                           //!< Position of the TX device
    MimoPolarizationAntennaParams m_txParams; //!< Parameters used to configure the TX antenna array
    Vector m_rxLoc;                           //!< Position of the RX device
    MimoPolarizationAntennaParams m_rxParams; //!< Parameters used to configure the RX antenna array
    std::valarray<std::complex<double>>
        m_testChannel;  //!< The test value for the matrix representing the strongest cluster
    double m_tolerance; //!< The tolerance to be used when comparing the channel matrix with the
                        //!< test matrix
};

ThreeGppMimoPolarizationTest::ThreeGppMimoPolarizationTest(
    std::string testCaseName,
    Vector txLoc,
    const MimoPolarizationAntennaParams& txParams,
    Vector rxLoc,
    const MimoPolarizationAntennaParams& rxParams,
    std::valarray<std::complex<double>> testChannel,
    double tolerance)
    : TestCase("Test MIMO using dual polarization." + testCaseName),
      m_txLoc(txLoc),
      m_txParams(txParams),
      m_rxLoc(rxLoc),
      m_rxParams(rxParams),
      m_testChannel(testChannel),
      m_tolerance(tolerance)
{
}

ThreeGppMimoPolarizationTest::~ThreeGppMimoPolarizationTest()
{
}

Ptr<PhasedArrayModel>
ThreeGppMimoPolarizationTest::CreateAndConfigureAntenna(const MimoPolarizationAntennaParams& params)
{
    NS_LOG_FUNCTION(this);
    Ptr<AntennaModel> antenna;
    if (params.m_isotropic)
    {
        antenna = CreateObject<IsotropicAntennaModel>();
    }
    else
    {
        antenna = CreateObject<ThreeGppAntennaModel>();
    }
    // create the tx and rx antennas and set the their dimensions
    return CreateObjectWithAttributes<UniformPlanarArray>("NumColumns",
                                                          UintegerValue(params.m_cols),
                                                          "NumRows",
                                                          UintegerValue(params.m_rows),
                                                          "AntennaElement",
                                                          PointerValue(antenna),
                                                          "NumVerticalPorts",
                                                          UintegerValue(params.m_vPorts),
                                                          "NumHorizontalPorts",
                                                          UintegerValue(params.m_hPorts),
                                                          "BearingAngle",
                                                          DoubleValue(params.m_bearingAngle),
                                                          "PolSlantAngle",
                                                          DoubleValue(params.m_polSlantAngle),
                                                          "IsDualPolarized",
                                                          BooleanValue(true));
}

void
ThreeGppMimoPolarizationTest::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    // create the ThreeGppChannelModel object used to generate the channel matrix
    Ptr<ThreeGppChannelModel> channelModel = CreateObject<ThreeGppChannelModel>();
    channelModel->SetAttribute("Frequency", DoubleValue(60e9));
    channelModel->SetAttribute("Scenario", StringValue("RMa"));
    channelModel->SetAttribute("ChannelConditionModel",
                               PointerValue(CreateObject<AlwaysLosChannelConditionModel>()));

    int64_t randomStream = 1;
    randomStream += channelModel->AssignStreams(randomStream);

    // create the tx and rx nodes
    NodeContainer nodes;
    nodes.Create(2);

    // create the tx and rx mobility models and set their positions
    Ptr<MobilityModel> txMob = CreateObject<ConstantPositionMobilityModel>();
    txMob->SetPosition(m_txLoc);
    Ptr<MobilityModel> rxMob = CreateObject<ConstantPositionMobilityModel>();
    rxMob->SetPosition(m_rxLoc);

    // associate the nodes and the mobility models
    nodes.Get(0)->AggregateObject(txMob);
    nodes.Get(1)->AggregateObject(rxMob);

    // create the tx and rx antennas and set the their dimensions
    Ptr<PhasedArrayModel> txAntenna = CreateAndConfigureAntenna(m_txParams);
    Ptr<PhasedArrayModel> rxAntenna = CreateAndConfigureAntenna(m_rxParams);

    // configure direct beamforming vectors to point to each other
    txAntenna->SetBeamformingVector(
        txAntenna->GetBeamformingVector(Angles(rxMob->GetPosition(), txMob->GetPosition())));
    rxAntenna->SetBeamformingVector(
        rxAntenna->GetBeamformingVector(Angles(txMob->GetPosition(), rxMob->GetPosition())));

    // generate the time domain channel matrix
    Ptr<const ThreeGppChannelModel::ChannelMatrix> channelMatrix =
        channelModel->GetChannel(txMob, rxMob, txAntenna, rxAntenna);

    // test whether the channel matrix for the first cluster experiences strong
    // symmetry that is caused by existence of dual polarized ports at the
    // transmitter and the receiver
    const std::complex<double>* strongestClusterPtr = channelMatrix->m_channel.GetPagePtr(0);
    size_t matrixSize =
        channelMatrix->m_channel.GetNumRows() * channelMatrix->m_channel.GetNumCols();

    MatrixBasedChannelModel::Complex2DVector strongestCluster(
        channelMatrix->m_channel.GetNumRows(),
        channelMatrix->m_channel.GetNumCols(),
        std::valarray<std::complex<double>>(strongestClusterPtr, matrixSize));

    MatrixBasedChannelModel::Complex2DVector testChannel(channelMatrix->m_channel.GetNumRows(),
                                                         channelMatrix->m_channel.GetNumCols(),
                                                         m_testChannel);

    NS_LOG_INFO("Channel matrix:" << strongestCluster);
    NS_LOG_INFO("Test channel matrix: " << testChannel);

    NS_TEST_ASSERT_MSG_EQ(
        strongestCluster.IsAlmostEqual(testChannel, m_tolerance),
        true,
        "The strongest cluster and the test channel matrix should be almost equal");

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup spectrum-tests
 *
 * Test suite for the ThreeGppChannelModel class
 */
class ThreeGppChannelTestSuite : public TestSuite
{
  public:
    /**
     * Constructor
     */
    ThreeGppChannelTestSuite();
};

ThreeGppChannelTestSuite::ThreeGppChannelTestSuite()
    : TestSuite("three-gpp-channel", Type::UNIT)
{
    AddTestCase(
        new ThreeGppChannelConsistencyTest(ChannelCondition::LosConditionValue::LOS, 30, 4e9),
        TestCase::Duration::QUICK);
    AddTestCase(
        new ThreeGppChannelConsistencyTest(ChannelCondition::LosConditionValue::NLOS, 30, 4e9),
        TestCase::Duration::QUICK);
    AddTestCase(
        new ThreeGppChannelConsistencyTest(ChannelCondition::LosConditionValue::LOS, 30, 30e9),
        TestCase::Duration::QUICK);
    AddTestCase(
        new ThreeGppChannelConsistencyTest(ChannelCondition::LosConditionValue::NLOS, 30, 30e9),
        TestCase::Duration::QUICK);
    AddTestCase(
        new ThreeGppChannelConsistencyTest(ChannelCondition::LosConditionValue::LOS, 10, 30e9),
        TestCase::Duration::QUICK);
    AddTestCase(
        new ThreeGppChannelConsistencyTest(ChannelCondition::LosConditionValue::NLOS, 10, 30e9),
        TestCase::Duration::QUICK);
    AddTestCase(
        new ThreeGppChannelConsistencyTest(ChannelCondition::LosConditionValue::LOS, 10, 4e9),
        TestCase::Duration::QUICK);
    AddTestCase(
        new ThreeGppChannelConsistencyTest(ChannelCondition::LosConditionValue::NLOS, 10, 4e9),
        TestCase::Duration::QUICK);

    AddTestCase(new ThreeGppChannelMatrixComputationTest(2, 2, 1, 1), TestCase::Duration::QUICK);
    AddTestCase(new ThreeGppChannelMatrixComputationTest(4, 2, 1, 1), TestCase::Duration::QUICK);
    AddTestCase(new ThreeGppChannelMatrixComputationTest(2, 2, 2, 2), TestCase::Duration::QUICK);
    AddTestCase(new ThreeGppChannelMatrixComputationTest(4, 4, 2, 2), TestCase::Duration::QUICK);
    AddTestCase(new ThreeGppChannelMatrixComputationTest(4, 2, 2, 1), TestCase::Duration::QUICK);
    AddTestCase(new ThreeGppChannelMatrixUpdateTest(2, 4, 1, 1), TestCase::Duration::QUICK);
    AddTestCase(new ThreeGppChannelMatrixUpdateTest(2, 2, 1, 1), TestCase::Duration::QUICK);
    AddTestCase(new ThreeGppChannelMatrixUpdateTest(2, 4, 2, 2), TestCase::Duration::QUICK);
    AddTestCase(new ThreeGppChannelMatrixUpdateTest(2, 2, 2, 2), TestCase::Duration::QUICK);
    AddTestCase(new ThreeGppAntennaSetupChangedTest(), TestCase::Duration::QUICK);
    AddTestCase(new ThreeGppSpectrumPropagationLossModelTest(4, 4, 1, 1),
                TestCase::Duration::QUICK);

    AddTestCase(new ThreeGppSpectrumPropagationLossModelTest(4, 4, 2, 2),
                TestCase::Duration::QUICK);
    AddTestCase(new ThreeGppSpectrumPropagationLossModelTest(4, 2, 2, 2),
                TestCase::Duration::QUICK);
    AddTestCase(new ThreeGppSpectrumPropagationLossModelTest(4, 2, 2, 1),
                TestCase::Duration::QUICK);

    AddTestCase(new ThreeGppCalcLongTermMultiPortTest(), TestCase::Duration::QUICK);

    /**
     *  The TX and RX antennas are configured face-to-face.
     *  When polarization slant angles are 0 and 0 at TX and RX,
     *  we expect the strongest cluster to be similar to the following matrix:
     *   (5.9,0) (5.9,0) (0,0)     (0,0)
     *   (5.9,0) (5.9,0) (0,0)     (0,0)
     *   (0,0)   (0,0)   (-5.8,)   (-5.8,0)
     *   (0,0)   (0,0)   (-5.8,0)  (-5.8,0)
     */

    std::valarray<std::complex<double>> testChannel1 =
        {5.9, 5.9, 0, 0, 5.9, 5.9, 0, 0, 0, 0, -5.8, -5.8, 0, 0, -5.8, -5.8};
    AddTestCase(new ThreeGppMimoPolarizationTest("Face-to-face. 0 and 0 pol. slant angles.",
                                                 Vector{0, 0, 3},
                                                 MimoPolarizationAntennaParams(false, 0, 0),
                                                 Vector{9, 0, 3},
                                                 MimoPolarizationAntennaParams(false, 0, M_PI),
                                                 testChannel1,
                                                 0.9),
                TestCase::Duration::QUICK);

    /**
     *  The TX and RX antennas are configured face-to-face.
     *  When polarization slant angles are 30 and 0 at TX and RX,
     *  we expect the strongest cluster to be similar to the following matrix:
     *   (5,0)   (5,0)   (3,0)   (3,0)
     *   (5,0)   (5,0)   (3,0)   (3,0)
     *   (3,0)   (3,0)   (-5,0)  (-5,0)
     *   (3,0)   (3,0)   (-5,0)  (-5,0)
     */

    AddTestCase(
        new ThreeGppMimoPolarizationTest("Face-to-face. 30 and 0 pol. slant angles.",
                                         Vector{0, 0, 3},
                                         MimoPolarizationAntennaParams(false, M_PI / 6, 0),
                                         Vector{6, 0, 3},
                                         MimoPolarizationAntennaParams(false, 0, M_PI),
                                         {5, 5, 3, 3, 5, 5, 3, 3, 3, 3, -5, -5, 3, 3, -5, -5},
                                         0.8),
        TestCase::Duration::QUICK);

    /**
     *  The TX and RX antennas are configured face-to-face.
     *  When polarization slant angles are 45 and 0 at TX and RX,
     *  we expect the strongest cluster to be similar to the following matrix:
     *  (4,0)  (4,0)  (4,0)  (4,0)
     *  (4,0)  (4,0)  (4,0)  (4,0)
     *  (4,0)  (4,0)  (4,0)  (4,0)
     *  (4,0)  (4,0)  (4,0)  (4,0)
     */

    AddTestCase(
        new ThreeGppMimoPolarizationTest("Face-to-face. 45 and 0 pol. slant angles.",
                                         Vector{0, 0, 3},
                                         MimoPolarizationAntennaParams(false, M_PI / 4, 0),
                                         Vector{6, 0, 3},
                                         MimoPolarizationAntennaParams(false, 0, M_PI),
                                         {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, -4, -4, 4, 4, -4, -4},
                                         0.7),
        TestCase::Duration::QUICK);

    /**
     *  The TX and RX antennas are configured face-to-face.
     *  When polarization slant angles are 45 and 0 at TX and RX,
     *  we expect the strongest cluster to be similar to the following matrix:
     *  (0,0)  (0,0)  (5.9,0)  (5.9,0)
     *  (0,0)  (0,0)  (5.9,0)  (5.9,0)
     *  (5.8,0)  (5.8,0)  (0,0)  (0,0)
     *  (5.8,0)  (5.8,0)  (0,0)  (0,0)
     */

    AddTestCase(new ThreeGppMimoPolarizationTest(
                    "Face-to-face. 90 and 0 pol. slant angles.",
                    Vector{0, 0, 3},
                    MimoPolarizationAntennaParams(false, M_PI / 2, 0),
                    Vector{6, 0, 3},
                    MimoPolarizationAntennaParams(false, 0, M_PI),
                    {0, 0, 5.8, 5.8, 0, 0, 5.8, 5.8, 5.9, 5.9, 0, 0, 5.9, 5.9, 0, 0},
                    0.9),
                TestCase::Duration::QUICK);

    /**
     *  The TX and RX antennas are face to face. We test the configuration of
     *  the bearing angle along with the configuration of the different position
     *  of the RX antenna, and the bearing angles.
     *  When polarization slant angles are 0 and 0 at TX and RX,
     *  we expect the strongest cluster to be similar to the following matrix:
     *   (5.9,0) (5.9,0) (0,0)     (0,0)
     *   (5.9,0) (5.9,0) (0,0)     (0,0)
     *   (0,0)   (0,0)   (-5.8,)   (-5.8,0)
     *   (0,0)   (0,0)   (-5.8,0)  (-5.8,0)
     *  Notice that we expect almost the same matrix as in the first case in
     *  which
     */

    AddTestCase(
        new ThreeGppMimoPolarizationTest("Face-to-face. Different positions. Different bearing "
                                         "angles. 0 and 0 pol. slant angles.",
                                         Vector{0, 0, 3},
                                         MimoPolarizationAntennaParams(false, 0, M_PI / 4),
                                         Vector{6.363961031, 6.363961031, 3},
                                         MimoPolarizationAntennaParams(false, 0, -(M_PI / 4) * 3),
                                         testChannel1,
                                         0.9),
        TestCase::Duration::QUICK);

    /**
     *  The TX and RX antenna have different height.
     *  Bearing angle is configured to point one toward the other.
     *  When polarization slant angles are 0 and 0 at TX and RX,
     *  we expect the strongest cluster to be similar to the following matrix:
     *   (2.5,-4.7)  (2.5,-4.7)   (0,0)     (0,0)
     *   (2.5,-4.7)  (2.5,-4.7)   (0,0)     (0,0)
     *   (0,0)   (0,0)    (-2.4,4)    (-2.4,4)
     *   (0,0)   (0,0)    (-2.4,4)    (-2.4,4)
     */
    AddTestCase(new ThreeGppMimoPolarizationTest(
                    "Not face-to-face. Different heights. 0 and 0 pol. slant angles.",
                    Vector{0, 0, 10},
                    MimoPolarizationAntennaParams(false, 0, 0),
                    Vector{30, 0, 3},
                    MimoPolarizationAntennaParams(false, 0, M_PI),
                    {{2.5, -4.7},
                     {2.5, -4.7},
                     0,
                     0,
                     {2.5, -4.7},
                     {2.5, -4.7},
                     0,
                     0,
                     0,
                     0,
                     {-2.4, 4},
                     {-2.4, 4},
                     0,
                     0,
                     {-2.4, 4},
                     {-2.4, 4}},
                    0.5),
                TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static ThreeGppChannelTestSuite myTestSuite;
