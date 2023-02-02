/*
 * Copyright (c) 2019 SIGNET Lab, Department of Information Engineering,
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

#include "ns3/abort.h"
#include "ns3/angles.h"
#include "ns3/channel-condition-model.h"
#include "ns3/config.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/double.h"
#include "ns3/ism-spectrum-value-helper.h"
#include "ns3/isotropic-antenna-model.h"
#include "ns3/log.h"
#include "ns3/node-container.h"
#include "ns3/pointer.h"
#include "ns3/simple-net-device.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-signal-parameters.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/three-gpp-channel-model.h"
#include "ns3/three-gpp-spectrum-propagation-loss-model.h"
#include "ns3/uinteger.h"
#include "ns3/uniform-planar-array.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ThreeGppChannelTestSuite");

/**
 * \ingroup spectrum-tests
 *
 * Test case for the ThreeGppChannelModel class.
 * 1) check if the channel matrix has the correct dimensions
 * 2) check if the channel matrix is correctly normalized
 */
class ThreeGppChannelMatrixComputationTest : public TestCase
{
  public:
    /**
     * Constructor
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
     * \param channelModel the ThreeGppChannelModel object used to generate the channel matrix
     * \param txMob the mobility model of the first node
     * \param rxMob the mobility model of the second node
     * \param txAntenna the antenna object associated to the first node
     * \param rxAntenna the antenna object associated to the second node
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
    uint64_t txAntennaElements = txAntenna->GetNumberOfElements();
    uint64_t rxAntennaElements = rxAntenna->GetNumberOfElements();

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
        CreateObject<NeverLosChannelConditionModel>();

    // create the ThreeGppChannelModel object used to generate the channel matrix
    Ptr<ThreeGppChannelModel> channelModel = CreateObject<ThreeGppChannelModel>();
    channelModel->SetAttribute("Frequency", DoubleValue(60.0e9));
    channelModel->SetAttribute("Scenario", StringValue("RMa"));
    channelModel->SetAttribute("ChannelConditionModel", PointerValue(channelConditionModel));
    channelModel->SetAttribute("UpdatePeriod", TimeValue(MilliSeconds(updatePeriodMs - 1)));
    channelModel->AssignStreams(1);

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

    // create the tx and rx mobility models and set their positions
    Ptr<MobilityModel> txMob = CreateObject<ConstantPositionMobilityModel>();
    txMob->SetPosition(Vector(0.0, 0.0, 10.0));
    Ptr<MobilityModel> rxMob = CreateObject<ConstantPositionMobilityModel>();
    rxMob->SetPosition(Vector(100.0, 0.0, 10.0));

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
    uint16_t numIt = 1000;
    for (uint16_t i = 0; i < numIt; i++)
    {
        Simulator::Schedule(MilliSeconds(updatePeriodMs * i),
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
        sampleStd += ((i - sampleMean) * (i - sampleMean));
    }
    sampleStd = std::sqrt(sampleStd / (numIt - 1));

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

    Simulator::Destroy();
}

/**
 * \ingroup spectrum-tests
 *
 * Test case for the ThreeGppChannelModel class.
 * It checks if the channel realizations are correctly updated during the
 * simulation.
 */
class ThreeGppChannelMatrixUpdateTest : public TestCase
{
  public:
    /**
     * Constructor
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
     * This method is used to schedule the channel matrix computation at different
     * time instants and to check if it correctly updated
     * \param channelModel the ThreeGppChannelModel object used to generate the channel matrix
     * \param txMob the mobility model of the first node
     * \param rxMob the mobility model of the second node
     * \param txAntenna the antenna object associated to the first node
     * \param rxAntenna the antenna object associated to the second node
     * \param update whether if the channel matrix should be updated or not
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
ThreeGppChannelMatrixUpdateTest::DoGetChannel(Ptr<ThreeGppChannelModel> channelModel,
                                              Ptr<MobilityModel> txMob,
                                              Ptr<MobilityModel> rxMob,
                                              Ptr<PhasedArrayModel> txAntenna,
                                              Ptr<PhasedArrayModel> rxAntenna,
                                              bool update)
{
    // retrieve the channel matrix
    Ptr<const ThreeGppChannelModel::ChannelMatrix> channelMatrix =
        channelModel->GetChannel(txMob, rxMob, txAntenna, rxAntenna);

    if (!m_currentChannel)
    {
        // this is the first time we compute the channel matrix, we initialize
        // m_currentChannel
        m_currentChannel = channelMatrix;
    }
    else
    {
        // compare the old and the new channel matrices
        NS_TEST_ASSERT_MSG_EQ((m_currentChannel->m_channel != channelMatrix->m_channel),
                              update,
                              Simulator::Now().GetMilliSeconds()
                                  << " The channel matrix is not correctly updated");
    }
}

void
ThreeGppChannelMatrixUpdateTest::DoRun()
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

    // create the tx and rx devices
    Ptr<SimpleNetDevice> txDev = CreateObject<SimpleNetDevice>();
    Ptr<SimpleNetDevice> rxDev = CreateObject<SimpleNetDevice>();

    // associate the nodes and the devices
    nodes.Get(0)->AddDevice(txDev);
    txDev->SetNode(nodes.Get(0));
    nodes.Get(1)->AddDevice(rxDev);
    rxDev->SetNode(nodes.Get(1));

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
                        &ThreeGppChannelMatrixUpdateTest::DoGetChannel,
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
                        &ThreeGppChannelMatrixUpdateTest::DoGetChannel,
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
                        &ThreeGppChannelMatrixUpdateTest::DoGetChannel,
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
 * \ingroup spectrum-tests
 * \brief A structure that holds the parameters for the function
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
    Ptr<PhasedArrayModel> txAntenna;        //!< the antenna array of the tx device
    Ptr<PhasedArrayModel> rxAntenna;        //!< the antenna array of the rx device
};

/**
 * \ingroup spectrum-tests
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
     * Points the beam of thisDevice towards otherDevice
     * \param thisDevice the device to configure
     * \param thisAntenna the antenna object associated to thisDevice
     * \param otherDevice the device to communicate with
     * \param otherAntenna the antenna object associated to otherDevice
     */
    void DoBeamforming(Ptr<NetDevice> thisDevice,
                       Ptr<PhasedArrayModel> thisAntenna,
                       Ptr<NetDevice> otherDevice,
                       Ptr<PhasedArrayModel> otherAntenna);

    /**
     * Test of the long term component is correctly updated when the channel
     * matrix is recomputed
     * \param params a structure that contains the set of parameters needed by CheckLongTermUpdate
     * in order to perform calculations
     */
    void CheckLongTermUpdate(const CheckLongTermUpdateParams& params);

    uint32_t m_txAntennaElements{4}; //!< number of rows and columns of tx antenna array
    uint32_t m_rxAntennaElements{4}; //!< number of rows and columns of rx antenna array
    uint32_t m_txPorts{1}; //!< number of horizontal and vertical ports of tx antenna array
    uint32_t m_rxPorts{1}; //!< number of horizontal and vertical ports of rx antenna array
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
ThreeGppSpectrumPropagationLossModelTest::DoBeamforming(Ptr<NetDevice> thisDevice,
                                                        Ptr<PhasedArrayModel> thisAntenna,
                                                        Ptr<NetDevice> otherDevice,
                                                        Ptr<PhasedArrayModel> otherAntenna)
{
    Vector aPos = thisDevice->GetNode()->GetObject<MobilityModel>()->GetPosition();
    Vector bPos = otherDevice->GetNode()->GetObject<MobilityModel>()->GetPosition();

    // compute the azimuth and the elevation angles
    Angles completeAngle(bPos, aPos);

    PhasedArrayModel::ComplexVector antennaWeights =
        thisAntenna->GetBeamformingVector(completeAngle);
    thisAntenna->SetBeamformingVector(antennaWeights);
}

void
ThreeGppSpectrumPropagationLossModelTest::CheckLongTermUpdate(
    const CheckLongTermUpdateParams& params)
{
    auto rxPsdNewParams = params.lossModel->DoCalcRxPowerSpectralDensity(params.txParams,
                                                                         params.txMob,
                                                                         params.rxMob,
                                                                         params.txAntenna,
                                                                         params.rxAntenna);
    NS_TEST_ASSERT_MSG_EQ((*params.rxPsdOld == *rxPsdNewParams->psd),
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

    // create the tx and rx devices
    Ptr<SimpleNetDevice> txDev = CreateObject<SimpleNetDevice>();
    Ptr<SimpleNetDevice> rxDev = CreateObject<SimpleNetDevice>();

    // associate the nodes and the devices
    nodes.Get(0)->AddDevice(txDev);
    txDev->SetNode(nodes.Get(0));
    nodes.Get(1)->AddDevice(rxDev);
    rxDev->SetNode(nodes.Get(1));

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
        UintegerValue(m_rxAntennaElements),
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
    DoBeamforming(txDev, txAntenna, rxDev, rxAntenna);
    DoBeamforming(rxDev, rxAntenna, txDev, txAntenna);

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

    // 1) check that the rx PSD is equal for both the direct and the reverse channel
    auto rxParamsNew =
        lossModel->DoCalcRxPowerSpectralDensity(txParams, rxMob, txMob, rxAntenna, txAntenna);
    NS_TEST_ASSERT_MSG_EQ((*rxParamsOld->psd == *rxParamsNew->psd),
                          true,
                          "The long term for the direct and the reverse channel are different");

    // 2) check if the long term is updated when changing the BF vector
    // change the position of the rx device and recompute the beamforming vectors
    rxMob->SetPosition(Vector(10.0, 5.0, 10.0));
    PhasedArrayModel::ComplexVector txBfVector = txAntenna->GetBeamformingVector();
    txBfVector[0] = std::complex<double>(0.0, 0.0);
    txAntenna->SetBeamformingVector(txBfVector);

    rxParamsNew =
        lossModel->DoCalcRxPowerSpectralDensity(txParams, rxMob, txMob, rxAntenna, txAntenna);
    NS_TEST_ASSERT_MSG_EQ((*rxParamsOld->psd == *rxParamsNew->psd),
                          false,
                          "Changing the BF vectors the rx PSD does not change");

    NS_TEST_ASSERT_MSG_EQ(
        (*rxParamsOld->spectrumChannelMatrix == *rxParamsNew->spectrumChannelMatrix),
        false,
        "Changing the BF should change de frequency domain channel matrix");

    // update rxPsdOld
    rxParamsOld = rxParamsNew;

    // 3) check if the long term is updated when the channel matrix is recomputed
    CheckLongTermUpdateParams
        params{lossModel, txParams, txMob, rxMob, rxParamsOld->psd, txAntenna, rxAntenna};
    Simulator::Schedule(MilliSeconds(101),
                        &ThreeGppSpectrumPropagationLossModelTest::CheckLongTermUpdate,
                        this,
                        params);

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * \ingroup spectrum-tests
 *
 * Test case for the multi-port antenna operations in spectrum. It checks that
 * channel matrices after using different multi-port mappings are the same.
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

    // create the tx and rx devices
    Ptr<SimpleNetDevice> txDev = CreateObject<SimpleNetDevice>();
    Ptr<SimpleNetDevice> rxDev = CreateObject<SimpleNetDevice>();

    // associate the nodes and the devices
    nodes.Get(0)->AddDevice(txDev);
    txDev->SetNode(nodes.Get(0));
    nodes.Get(1)->AddDevice(rxDev);
    rxDev->SetNode(nodes.Get(1));

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
 * \ingroup spectrum-tests
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
    : TestSuite("three-gpp-channel", UNIT)
{
    AddTestCase(new ThreeGppChannelMatrixComputationTest(2, 2, 1, 1), TestCase::QUICK);
    AddTestCase(new ThreeGppChannelMatrixComputationTest(4, 2, 1, 1), TestCase::QUICK);
    AddTestCase(new ThreeGppChannelMatrixComputationTest(2, 2, 2, 2), TestCase::QUICK);
    AddTestCase(new ThreeGppChannelMatrixComputationTest(4, 4, 2, 2), TestCase::QUICK);
    AddTestCase(new ThreeGppChannelMatrixComputationTest(4, 2, 2, 1), TestCase::QUICK);
    AddTestCase(new ThreeGppChannelMatrixUpdateTest(2, 4, 1, 1), TestCase::QUICK);
    AddTestCase(new ThreeGppChannelMatrixUpdateTest(2, 2, 1, 1), TestCase::QUICK);
    AddTestCase(new ThreeGppChannelMatrixUpdateTest(2, 4, 2, 2), TestCase::QUICK);
    AddTestCase(new ThreeGppChannelMatrixUpdateTest(2, 2, 2, 2), TestCase::QUICK);
    AddTestCase(new ThreeGppSpectrumPropagationLossModelTest(4, 4, 1, 1), TestCase::QUICK);
    AddTestCase(new ThreeGppSpectrumPropagationLossModelTest(4, 4, 2, 2), TestCase::QUICK);
    AddTestCase(new ThreeGppSpectrumPropagationLossModelTest(4, 2, 2, 2), TestCase::QUICK);
    AddTestCase(new ThreeGppSpectrumPropagationLossModelTest(4, 2, 2, 1), TestCase::QUICK);
    AddTestCase(new ThreeGppCalcLongTermMultiPortTest(), TestCase::QUICK);
}

/// Static variable for test initialization
static ThreeGppChannelTestSuite myTestSuite;
