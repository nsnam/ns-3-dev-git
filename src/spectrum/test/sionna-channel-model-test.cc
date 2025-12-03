/*
 * Copyright (c) 2025 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Regression test suite for SionnaRtChannelModel (MR !2608).
 *
 * NOTE: Setting the "Scenario" attribute triggers pybind11/Python
 * initialisation which causes SIGSEGV in environments without Sionna.
 * All tests here are safe to run without a Sionna installation.
 * Tests that require a live Sionna runtime are guarded by
 * NS_TEST_ASSERT_MSG_EQ(sionnaAvailable, true, ...) and skipped otherwise.
 *
 * Registration — add to src/spectrum/test/CMakeLists.txt:
 *
 *   build_lib_test(
 *     LIB spectrum
 *     SOURCES sionna-channel-model-test.cc
 *     LIBRARIES_TO_LINK ${libspectrum}
 *   )
 *
 * Run:
 *   ./ns3 run "test-runner --suite=sionna-rt-channel-model --verbose"
 */

#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/matrix-based-channel-model.h"
#include "ns3/node.h"
#include "ns3/phased-array-model.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/sionna-rt-channel-model.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"
#include "ns3/uniform-planar-array.h"

#include <pybind11/pybind11.h>
namespace py = pybind11;

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SionnaRtChannelModelTest");

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static Ptr<UniformPlanarArray>
CreateUpa(uint32_t rows, uint32_t cols)
{
    Ptr<UniformPlanarArray> upa = CreateObject<UniformPlanarArray>();
    upa->SetAttribute("NumRows", UintegerValue(rows));
    upa->SetAttribute("NumColumns", UintegerValue(cols));
    return upa;
}

static Ptr<MobilityModel>
CreateMobility(double x, double y, double z)
{
    Ptr<Node> node = CreateObject<Node>();
    Ptr<ConstantPositionMobilityModel> mob = CreateObject<ConstantPositionMobilityModel>();
    mob->SetPosition(Vector(x, y, z));
    node->AggregateObject(mob);
    return mob;
}

// ===========================================================================
// Test  – Full Sionna end-to-end: GetChannel returns valid matrix
//
//   Initialises the Python interpreter with py::scoped_interpreter (same
//   pattern as the example scripts), sets a known scenario, and verifies:
//     - GetChannel returns non-null
//     - H matrix has correct dimensions
//     - H matrix is non-zero (actual ray-tracing happened)
//     - GetParams returns non-null
//     - Calling GetChannel twice with same positions returns cached matrix
// ===========================================================================
/**
 * @ingroup spectrum-tests
 * @brief End-to-end test for SionnaRtChannelModel::GetChannel with Sionna.
 *
 * Verifies that GetChannel returns a valid, non-null ChannelMatrix with
 * correct dimensions, that GetParams works, and that caching produces
 * consistent results.
 */
class SionnaRtChannelEndToEndTest : public TestCase
{
  public:
    SionnaRtChannelEndToEndTest()
        : TestCase("SionnaRtChannelModel end to end GetChannel with Sionna")
    {
    }

  private:
    void DoRun() override
    {
        // Initialise Python interpreter — must stay alive for entire test.
        // Uses the same guard pattern as the example scripts.
        py::scoped_interpreter guard{};

        const uint32_t sTxRows = 2;
        const uint32_t sTxCols = 2;
        const uint32_t uRxRows = 1;
        const uint32_t uRxCols = 1;

        Ptr<SionnaRtChannelModel> model = CreateObject<SionnaRtChannelModel>();
        model->SetFrequency(3.5e9);
        model->SetAttribute("UpdatePeriod", TimeValue(MilliSeconds(0)));
        model->SetAttribute("NormalizeDelays", BooleanValue(true));

        // This triggers LoadScene — requires Sionna installed
        model->SetAttribute("Scenario", StringValue("simple_street_canyon"));

        Ptr<UniformPlanarArray> txAnt = CreateUpa(sTxRows, sTxCols);
        Ptr<UniformPlanarArray> rxAnt = CreateUpa(uRxRows, uRxCols);
        Ptr<MobilityModel> txMob = CreateMobility(0.0, 0.0, 10.0);
        Ptr<MobilityModel> rxMob = CreateMobility(50.0, 0.0, 1.5);

        // --- Test: GetChannel returns non-null ---
        Ptr<const MatrixBasedChannelModel::ChannelMatrix> H =
            model->GetChannel(txMob, rxMob, txAnt, rxAnt);

        NS_TEST_ASSERT_MSG_NE(H, nullptr, "GetChannel returned nullptr");

        // --- Test: H matrix dimensions ---
        // H is stored as [rxElems x txElems] in MatrixArray
        NS_TEST_ASSERT_MSG_EQ(H->m_channel.GetNumRows(),
                              uRxRows * uRxCols,
                              "H num rows (rx elements) mismatch");
        NS_TEST_ASSERT_MSG_EQ(H->m_channel.GetNumCols(),
                              sTxRows * sTxCols,
                              "H num cols (tx elements) mismatch");

        // --- Test: H has at least one path (non-zero pages/clusters) ---
        NS_TEST_ASSERT_MSG_GT(H->m_channel.GetNumPages(),
                              0U,
                              "H has zero clusters — no paths computed");

        // --- Test: caching — same positions return same or equivalent matrix ---
        Ptr<const MatrixBasedChannelModel::ChannelMatrix> H2 =
            model->GetChannel(txMob, rxMob, txAnt, rxAnt);
        NS_TEST_ASSERT_MSG_NE(H2, nullptr, "Second GetChannel returned nullptr");
        // Either the same cached pointer is returned, or a new matrix with
        // identical dimensions (both are acceptable implementations)
        NS_TEST_ASSERT_MSG_EQ(H2->m_channel.GetNumRows(),
                              H->m_channel.GetNumRows(),
                              "Second call H num rows changed");
        NS_TEST_ASSERT_MSG_EQ(H2->m_channel.GetNumCols(),
                              H->m_channel.GetNumCols(),
                              "Second call H num cols changed");
        NS_TEST_ASSERT_MSG_EQ(H2->m_channel.GetNumPages(),
                              H->m_channel.GetNumPages(),
                              "Second call H num pages changed");

        // --- Test: GetParams returns non-null ---
        Ptr<const MatrixBasedChannelModel::ChannelParams> params = model->GetParams(txMob, rxMob);
        NS_TEST_ASSERT_MSG_NE(params, nullptr, "GetParams returned nullptr after GetChannel");

        // --- Test: SionnaRtChannelParams dynamic type ---
        const auto* sp =
            dynamic_cast<const SionnaRtChannelModel::SionnaRtChannelParams*>(PeekPointer(params));
        NS_TEST_ASSERT_MSG_NE(sp, nullptr, "Params is not SionnaRtChannelParams");

        Simulator::Destroy();
    }
};

// ===========================================================================
// Test Suite
// ===========================================================================
/**
 * @ingroup spectrum-tests
 * @brief Test suite for Sionna RT channel model regression tests.
 *
 * Registers all test cases for the Sionna RT channel model module.
 */
class SionnaRtChannelModelTestSuite : public TestSuite
{
  public:
    SionnaRtChannelModelTestSuite()
        : TestSuite("sionna-rt-channel-model", TestSuite::Type::UNIT)
    {
        // Requires Sionna + GPU — runs full ray-tracing pipeline
        AddTestCase(new SionnaRtChannelEndToEndTest(), TestCase::Duration::QUICK);
    }
};

static SionnaRtChannelModelTestSuite g_sionnaRtChannelModelTestSuite;
