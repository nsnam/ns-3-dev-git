/*
 * Copyright (c) 2013 Magister Solutions (original test-lte-handover-delay.cc)
 * Copyright (c) 2021 University of Washington (handover failure cases)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sachin Nayak <sachinnn@uw.edu>
 */

#include "ns3/boolean.h"
#include "ns3/callback.h"
#include "ns3/config.h"
#include "ns3/data-rate.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/log.h"
#include "ns3/lte-helper.h"
#include "ns3/lte-ue-net-device.h"
#include "ns3/mobility-helper.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/point-to-point-epc-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/position-allocator.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LteHandoverFailureTest");

/**
 * @ingroup lte-test
 *
 * @brief Verifying that a handover failure occurs due to various causes
 *
 * Handover failure cases dealt with in this test include the below.
 *
 * 1. Handover failure due to max random access channel (RACH) attempts from UE to target eNodeB
 * 2. Handover failure due to non-allocation of non-contention preamble to UE at target eNodeB
 * 3. Handover failure due to HANDOVER JOINING timeout (3 cases)
 * 4. Handover failure due to HANDOVER LEAVING timeout (3 cases)
 *
 * \sa ns3::LteHandoverFailureTestCase
 */
class LteHandoverFailureTestCase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param name the name of the test case, to be displayed in the test result
     * @param useIdealRrc if true, use the ideal RRC
     * @param handoverTime the time of handover
     * @param simulationDuration duration of the simulation
     * @param numberOfRaPreambles number of random access preambles available for contention based
     RACH process
     *                            number of non-contention preambles available for handover = (64 -
     numberRaPreambles)
     *                            as numberOfRaPreambles out of the max 64 are reserved contention
     based RACH process
     * @param preambleTransMax maximum number of random access preamble transmissions from UE to
     eNodeB
     * @param raResponseWindowSize window length for reception of random access response (RAR)
     * @param handoverJoiningTimeout time before which RRC RECONFIGURATION COMPLETE must be received
                                     at target eNodeB after it receives a handover request
                                     Else, the UE context is destroyed in the RRC.
                                     Timeout can occur before different stages as below.
                                     i. Reception of RRC CONNECTION RECONFIGURATION at source eNodeB
                                     ii. Non-contention random access procedure from UE to target
     eNodeB iii. Reception of RRC CONNECTION RECONFIGURATION COMPLETE at target eNodeB
     * @param handoverLeavingTimeout time before which source eNodeB must receive a UE context
     release from target eNodeB or RRC CONNECTION RESTABLISHMENT from UE after issuing a handover
     request Else, the UE context is destroyed in the RRC. Timeout can occur before any of the cases
     in HANDOVER JOINING TIMEOUT
     * @param targeteNodeBPosition position of the target eNodeB
     */
    LteHandoverFailureTestCase(std::string name,
                               bool useIdealRrc,
                               Time handoverTime,
                               Time simulationDuration,
                               uint8_t numberOfRaPreambles,
                               uint8_t preambleTransMax,
                               uint8_t raResponseWindowSize,
                               Time handoverJoiningTimeout,
                               Time handoverLeavingTimeout,
                               uint16_t targeteNodeBPosition)
        : TestCase(name),
          m_useIdealRrc(useIdealRrc),
          m_handoverTime(handoverTime),
          m_simulationDuration(simulationDuration),
          m_numberOfRaPreambles(numberOfRaPreambles),
          m_preambleTransMax(preambleTransMax),
          m_raResponseWindowSize(raResponseWindowSize),
          m_handoverJoiningTimeout(handoverJoiningTimeout),
          m_handoverLeavingTimeout(handoverLeavingTimeout),
          m_targeteNodeBPosition(targeteNodeBPosition),
          m_hasHandoverFailureOccurred(false)
    {
    }

  private:
    /**
     * @brief Run a simulation of a two eNodeB network using the parameters
     *        provided to the constructor function.
     */
    void DoRun() override;

    /**
     * @brief Called at the end of simulation and verifies that a handover
     *        and a handover failure has occurred in the simulation.
     */
    void DoTeardown() override;

    /**
     * UE handover start callback function to indicate start of handover
     * @param context the context string
     * @param imsi the IMSI
     * @param sourceCellId the source cell ID
     * @param rnti the RNTI
     * @param targetCellId the target cell ID
     */
    void UeHandoverStartCallback(std::string context,
                                 uint64_t imsi,
                                 uint16_t sourceCellId,
                                 uint16_t rnti,
                                 uint16_t targetCellId);

    /**
     * Handover failure callback due to maximum RACH transmissions reached from UE to target eNodeB
     * @param context the context string
     * @param imsi the IMSI
     * @param rnti the RNTI
     * @param targetCellId the target cell ID
     */
    void HandoverFailureMaxRach(std::string context,
                                uint64_t imsi,
                                uint16_t rnti,
                                uint16_t targetCellId);

    /**
     * Handover failure callback due to non-allocation of non-contention preamble at target eNodeB
     * @param context the context string
     * @param imsi the IMSI
     * @param rnti the RNTI
     * @param targetCellId the target cell ID
     */
    void HandoverFailureNoPreamble(std::string context,
                                   uint64_t imsi,
                                   uint16_t rnti,
                                   uint16_t targetCellId);

    /**
     * Handover failure callback due to handover joining timeout at target eNodeB
     * @param context the context string
     * @param imsi the IMSI
     * @param rnti the RNTI
     * @param targetCellId the target cell ID
     */
    void HandoverFailureJoining(std::string context,
                                uint64_t imsi,
                                uint16_t rnti,
                                uint16_t targetCellId);

    /**
     * Handover failure callback due to handover leaving timeout at source eNodeB
     * @param context the context string
     * @param imsi the IMSI
     * @param rnti the RNTI
     * @param targetCellId the target cell ID
     */
    void HandoverFailureLeaving(std::string context,
                                uint64_t imsi,
                                uint16_t rnti,
                                uint16_t targetCellId);

    bool m_useIdealRrc;             ///< use ideal RRC?
    Time m_handoverTime;            ///< handover time
    Time m_simulationDuration;      ///< the simulation duration
    uint8_t m_numberOfRaPreambles;  ///< number of random access preambles for contention based RACH
                                    ///< process
    uint8_t m_preambleTransMax;     ///< max number of RACH preambles possible from UE to eNodeB
    uint8_t m_raResponseWindowSize; ///< window length for reception of RAR
    Time m_handoverJoiningTimeout;  ///< handover joining timeout duration at target eNodeB
    Time m_handoverLeavingTimeout;  ///< handover leaving timeout duration at source eNodeB
    uint16_t m_targeteNodeBPosition;   ///< position of the target eNodeB
    bool m_hasHandoverFailureOccurred; ///< has handover failure occurred in simulation

    // end of class LteHandoverFailureTestCase
};

void
LteHandoverFailureTestCase::DoRun()
{
    NS_LOG_INFO(this << " " << GetName());
    uint32_t previousSeed = RngSeedManager::GetSeed();
    uint64_t previousRun = RngSeedManager::GetRun();
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(2);

    /*
     * Helpers.
     */
    auto epcHelper = CreateObject<PointToPointEpcHelper>();

    auto lteHelper = CreateObject<LteHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    // Set parameters for helpers based on the test case parameters.
    lteHelper->SetAttribute("UseIdealRrc", BooleanValue(m_useIdealRrc));
    Config::SetDefault("ns3::LteEnbMac::NumberOfRaPreambles", UintegerValue(m_numberOfRaPreambles));
    Config::SetDefault("ns3::LteEnbMac::PreambleTransMax", UintegerValue(m_preambleTransMax));
    Config::SetDefault("ns3::LteEnbMac::RaResponseWindowSize",
                       UintegerValue(m_raResponseWindowSize));
    Config::SetDefault("ns3::LteEnbRrc::HandoverJoiningTimeoutDuration",
                       TimeValue(m_handoverJoiningTimeout));
    Config::SetDefault("ns3::LteEnbRrc::HandoverLeavingTimeoutDuration",
                       TimeValue(m_handoverLeavingTimeout));

    // Set PHY model to drastically decrease with distance.
    lteHelper->SetPathlossModelType(TypeId::LookupByName("ns3::LogDistancePropagationLossModel"));
    lteHelper->SetPathlossModelAttribute("Exponent", DoubleValue(3.5));
    lteHelper->SetPathlossModelAttribute("ReferenceLoss", DoubleValue(35));
    /*
     * Physical layer.
     *
     * eNodeB 0                    UE                         eNodeB 1
     *
     *    x ----------------------- x -------------------------- x
     *              200 m               m_targeteNodeBPosition
     *  source                                                 target
     */
    // Create nodes.
    NodeContainer enbNodes;
    enbNodes.Create(2);
    auto ueNode = CreateObject<Node>();

    // Setup mobility
    auto posAlloc = CreateObject<ListPositionAllocator>();
    posAlloc->Add(Vector(0, 0, 0));
    posAlloc->Add(Vector(m_targeteNodeBPosition, 0, 0));
    posAlloc->Add(Vector(200, 0, 0));

    MobilityHelper mobilityHelper;
    mobilityHelper.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilityHelper.SetPositionAllocator(posAlloc);
    mobilityHelper.Install(enbNodes);
    mobilityHelper.Install(ueNode);

    /*
     * Link layer.
     */
    auto enbDevs = lteHelper->InstallEnbDevice(enbNodes);
    auto ueDev = lteHelper->InstallUeDevice(ueNode).Get(0);
    auto castedUeDev = DynamicCast<LteUeNetDevice>(ueDev);
    // Working value from before we started resetting g_nextStreamIndex. For more details
    // see https://gitlab.com/nsnam/ns-3-dev/-/merge_requests/2178#note_2143793903
    castedUeDev->GetPhy()->GetDlSpectrumPhy()->AssignStreams(175);

    /*
     * Network layer.
     */
    InternetStackHelper inetStackHelper;
    inetStackHelper.Install(ueNode);
    Ipv4InterfaceContainer ueIfs;
    ueIfs = epcHelper->AssignUeIpv4Address(ueDev);

    // Setup traces.
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/HandoverStart",
                    MakeCallback(&LteHandoverFailureTestCase::UeHandoverStartCallback, this));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverFailureMaxRach",
                    MakeCallback(&LteHandoverFailureTestCase::HandoverFailureMaxRach, this));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverFailureNoPreamble",
                    MakeCallback(&LteHandoverFailureTestCase::HandoverFailureNoPreamble, this));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverFailureJoining",
                    MakeCallback(&LteHandoverFailureTestCase::HandoverFailureJoining, this));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverFailureLeaving",
                    MakeCallback(&LteHandoverFailureTestCase::HandoverFailureLeaving, this));

    // Prepare handover.
    lteHelper->AddX2Interface(enbNodes);
    lteHelper->Attach(ueDev, enbDevs.Get(0));
    lteHelper->HandoverRequest(m_handoverTime, ueDev, enbDevs.Get(0), enbDevs.Get(1));

    // Run simulation.
    Simulator::Stop(m_simulationDuration);
    Simulator::Run();
    Simulator::Destroy();

    RngSeedManager::SetSeed(previousSeed);
    RngSeedManager::SetRun(previousRun);
}

void
LteHandoverFailureTestCase::UeHandoverStartCallback(std::string context,
                                                    uint64_t imsi,
                                                    uint16_t sourceCellId,
                                                    uint16_t rnti,
                                                    uint16_t targetCellId)
{
    NS_LOG_FUNCTION(this << " " << context << " IMSI-" << imsi << " sourceCellID-" << sourceCellId
                         << " RNTI-" << rnti << " targetCellID-" << targetCellId);
    NS_LOG_INFO("HANDOVER COMMAND received through at UE "
                << imsi << " to handover from " << sourceCellId << " to " << targetCellId);
}

void
LteHandoverFailureTestCase::HandoverFailureMaxRach(std::string context,
                                                   uint64_t imsi,
                                                   uint16_t rnti,
                                                   uint16_t targetCellId)
{
    NS_LOG_FUNCTION(this << context << imsi << rnti << targetCellId);
    m_hasHandoverFailureOccurred = true;
}

void
LteHandoverFailureTestCase::HandoverFailureNoPreamble(std::string context,
                                                      uint64_t imsi,
                                                      uint16_t rnti,
                                                      uint16_t targetCellId)
{
    NS_LOG_FUNCTION(this << context << imsi << rnti << targetCellId);
    m_hasHandoverFailureOccurred = true;
}

void
LteHandoverFailureTestCase::HandoverFailureJoining(std::string context,
                                                   uint64_t imsi,
                                                   uint16_t rnti,
                                                   uint16_t targetCellId)
{
    NS_LOG_FUNCTION(this << context << imsi << rnti << targetCellId);
    m_hasHandoverFailureOccurred = true;
}

void
LteHandoverFailureTestCase::HandoverFailureLeaving(std::string context,
                                                   uint64_t imsi,
                                                   uint16_t rnti,
                                                   uint16_t targetCellId)
{
    NS_LOG_FUNCTION(this << context << imsi << rnti << targetCellId);
    m_hasHandoverFailureOccurred = true;
}

void
LteHandoverFailureTestCase::DoTeardown()
{
    NS_LOG_FUNCTION(this);
    NS_TEST_ASSERT_MSG_EQ(m_hasHandoverFailureOccurred, true, "Handover failure did not occur");
}

/**
 * @ingroup lte-test
 *
 * The following log components can be used to debug this test's behavior:
 * LteHandoverFailureTest:LteEnbRrc:LteEnbMac:LteUeRrc:EpcX2
 *
 * @brief Lte Handover Failure Test Suite
 */
static class LteHandoverFailureTestSuite : public TestSuite
{
  public:
    LteHandoverFailureTestSuite()
        : TestSuite("lte-handover-failure", Type::SYSTEM)
    {
        // Argument sequence for all test cases: useIdealRrc, handoverTime, simulationDuration,
        // numberOfRaPreambles, preambleTransMax, raResponseWindowSize,
        //                                       handoverJoiningTimeout, handoverLeavingTimeout

        // Test cases for REAL RRC protocol
        AddTestCase(new LteHandoverFailureTestCase("REAL Handover failure due to maximum RACH "
                                                   "transmissions reached from UE to target eNodeB",
                                                   false,
                                                   Seconds(0.200),
                                                   Seconds(0.300),
                                                   52,
                                                   3,
                                                   3,
                                                   MilliSeconds(200),
                                                   MilliSeconds(500),
                                                   2500),
                    TestCase::Duration::QUICK);
        AddTestCase(new LteHandoverFailureTestCase(
                        "REAL Handover failure due to non-allocation of non-contention preamble at "
                        "target eNodeB due to max number reached",
                        false,
                        Seconds(0.100),
                        Seconds(0.200),
                        64,
                        50,
                        3,
                        MilliSeconds(200),
                        MilliSeconds(500),
                        1500),
                    TestCase::Duration::QUICK);
        AddTestCase(new LteHandoverFailureTestCase(
                        "REAL Handover failure due to HANDOVER JOINING timeout before reception of "
                        "RRC CONNECTION RECONFIGURATION at source eNodeB",
                        false,
                        Seconds(0.100),
                        Seconds(0.200),
                        52,
                        50,
                        3,
                        MilliSeconds(0),
                        MilliSeconds(500),
                        1500),
                    TestCase::Duration::QUICK);
        AddTestCase(new LteHandoverFailureTestCase(
                        "REAL Handover failure due to HANDOVER JOINING timeout before completion "
                        "of non-contention RACH process to target eNodeB",
                        false,
                        Seconds(0.100),
                        Seconds(0.200),
                        52,
                        50,
                        3,
                        MilliSeconds(15),
                        MilliSeconds(500),
                        1500),
                    TestCase::Duration::QUICK);
        AddTestCase(new LteHandoverFailureTestCase(
                        "REAL Handover failure due to HANDOVER JOINING timeout before reception of "
                        "RRC CONNECTION RECONFIGURATION COMPLETE at target eNodeB",
                        false,
                        Seconds(0.100),
                        Seconds(0.200),
                        52,
                        50,
                        3,
                        MilliSeconds(18),
                        MilliSeconds(500),
                        500),
                    TestCase::Duration::QUICK);
        AddTestCase(new LteHandoverFailureTestCase(
                        "REAL Handover failure due to HANDOVER LEAVING timeout before reception of "
                        "RRC CONNECTION RECONFIGURATION at source eNodeB",
                        false,
                        Seconds(0.100),
                        Seconds(0.200),
                        52,
                        50,
                        3,
                        MilliSeconds(200),
                        MilliSeconds(0),
                        1500),
                    TestCase::Duration::QUICK);
        AddTestCase(new LteHandoverFailureTestCase(
                        "REAL Handover failure due to HANDOVER LEAVING timeout before completion "
                        "of non-contention RACH process to target eNodeB",
                        false,
                        Seconds(0.100),
                        Seconds(0.200),
                        52,
                        50,
                        3,
                        MilliSeconds(200),
                        MilliSeconds(15),
                        1500),
                    TestCase::Duration::QUICK);
        AddTestCase(new LteHandoverFailureTestCase(
                        "REAL Handover failure due to HANDOVER LEAVING timeout before reception of "
                        "RRC CONNECTION RECONFIGURATION COMPLETE at target eNodeB",
                        false,
                        Seconds(0.100),
                        Seconds(0.200),
                        52,
                        50,
                        3,
                        MilliSeconds(200),
                        MilliSeconds(18),
                        500),
                    TestCase::Duration::QUICK);

        // Test cases for IDEAL RRC protocol
        AddTestCase(new LteHandoverFailureTestCase("IDEAL Handover failure due to maximum RACH "
                                                   "transmissions reached from UE to target eNodeB",
                                                   true,
                                                   Seconds(0.100),
                                                   Seconds(0.200),
                                                   52,
                                                   3,
                                                   3,
                                                   MilliSeconds(200),
                                                   MilliSeconds(500),
                                                   1500),
                    TestCase::Duration::QUICK);
        AddTestCase(new LteHandoverFailureTestCase(
                        "IDEAL Handover failure due to non-allocation of non-contention preamble "
                        "at target eNodeB due to max number reached",
                        true,
                        Seconds(0.100),
                        Seconds(0.200),
                        64,
                        50,
                        3,
                        MilliSeconds(200),
                        MilliSeconds(500),
                        1500),
                    TestCase::Duration::QUICK);
        AddTestCase(new LteHandoverFailureTestCase(
                        "IDEAL Handover failure due to HANDOVER JOINING timeout before reception "
                        "of RRC CONNECTION RECONFIGURATION at source eNodeB",
                        true,
                        Seconds(0.100),
                        Seconds(0.200),
                        52,
                        50,
                        3,
                        MilliSeconds(0),
                        MilliSeconds(500),
                        1500),
                    TestCase::Duration::QUICK);
        AddTestCase(new LteHandoverFailureTestCase(
                        "IDEAL Handover failure due to HANDOVER JOINING timeout before completion "
                        "of non-contention RACH process to target eNodeB",
                        true,
                        Seconds(0.100),
                        Seconds(0.200),
                        52,
                        50,
                        3,
                        MilliSeconds(10),
                        MilliSeconds(500),
                        1500),
                    TestCase::Duration::QUICK);
        AddTestCase(new LteHandoverFailureTestCase(
                        "IDEAL Handover failure due to HANDOVER JOINING timeout before reception "
                        "of RRC CONNECTION RECONFIGURATION COMPLETE at target eNodeB",
                        true,
                        Seconds(0.100),
                        Seconds(0.200),
                        52,
                        50,
                        3,
                        MilliSeconds(4),
                        MilliSeconds(500),
                        500),
                    TestCase::Duration::QUICK);
        AddTestCase(new LteHandoverFailureTestCase(
                        "IDEAL Handover failure due to HANDOVER LEAVING timeout before reception "
                        "of RRC CONNECTION RECONFIGURATION at source eNodeB",
                        true,
                        Seconds(0.100),
                        Seconds(0.200),
                        52,
                        50,
                        3,
                        MilliSeconds(500),
                        MilliSeconds(0),
                        1500),
                    TestCase::Duration::QUICK);
        AddTestCase(new LteHandoverFailureTestCase(
                        "IDEAL Handover failure due to HANDOVER LEAVING timeout before completion "
                        "of non-contention RACH process to target eNodeB",
                        true,
                        Seconds(0.100),
                        Seconds(0.200),
                        52,
                        50,
                        3,
                        MilliSeconds(500),
                        MilliSeconds(10),
                        1500),
                    TestCase::Duration::QUICK);
        AddTestCase(new LteHandoverFailureTestCase(
                        "IDEAL Handover failure due to HANDOVER LEAVING timeout before reception "
                        "of RRC CONNECTION RECONFIGURATION COMPLETE at target eNodeB",
                        true,
                        Seconds(0.100),
                        Seconds(0.200),
                        52,
                        50,
                        3,
                        MilliSeconds(500),
                        MilliSeconds(4),
                        500),
                    TestCase::Duration::QUICK);
    }
} g_lteHandoverFailureTestSuite; ///< end of LteHandoverFailureTestSuite ()
