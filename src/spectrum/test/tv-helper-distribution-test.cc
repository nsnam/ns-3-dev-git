/*
 * Copyright (c) 2014 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Benjamin Cizdziel <ben.cizdziel@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/test.h"
#include "ns3/tv-spectrum-transmitter-helper.h"

NS_LOG_COMPONENT_DEFINE("TvHelperDistributionTest");

using namespace ns3;

/**
 * @ingroup spectrum-tests
 *
 * This test verifies the accuracy of the private GetRandomNumTransmitters()
 * method in the TvSpectrumTransmitterHelper class. The method generates a
 * random number corresponding to the number of TV transmitters to create based
 * on the given location density (low, medium, or high) and maximum possible
 * number of TV channels. Low density will generate a transmitter for between
 * one (a single transmitter) and one third of the number of possible channels,
 * medium density will generate a transmitter for between one third and two
 * thirds, and high density will generate a transmitter for between two thirds
 * and all of the possible channels. In this test, it is verified that the
 * lower (1) and upper (max number of possible channels input) bounds are not
 * exceeded and that the number of transmitters to be generated does not overlap
 * between adjacent densities. For example, given 60 possible channels, for low
 * density between 1 and 20 transmitters can be created, for medium density
 * between 21 and 40 transmitters can be created, and for high density between
 * 41 and 60 transmitters can be created (all inclusive). This is tested with
 * various cases.
 */
class TvHelperDistributionTestCase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param maxNumTransmitters maximum number of transmitters.
     */
    TvHelperDistributionTestCase(uint32_t maxNumTransmitters);
    ~TvHelperDistributionTestCase() override;

  private:
    void DoRun() override;
    /**
     * Build the test name
     * @param maxNumTransmitters maximum number of transmitters.
     * @return The test name
     */
    static std::string Name(uint32_t maxNumTransmitters);
    uint32_t m_maxNumTransmitters; //!< Maximum number of transmitters.
};

std::string
TvHelperDistributionTestCase::Name(uint32_t maxNumTransmitters)
{
    std::ostringstream oss;
    oss << "Max Number of Transmitters = " << maxNumTransmitters;
    return oss.str();
}

TvHelperDistributionTestCase::TvHelperDistributionTestCase(uint32_t maxNumTransmitters)
    : TestCase(Name(maxNumTransmitters)),
      m_maxNumTransmitters(maxNumTransmitters)
{
}

TvHelperDistributionTestCase::~TvHelperDistributionTestCase()
{
}

void
TvHelperDistributionTestCase::DoRun()
{
    NS_LOG_FUNCTION(m_maxNumTransmitters);
    TvSpectrumTransmitterHelper tvTransHelper;
    uint32_t rand;
    uint32_t maxLow = 0;
    uint32_t minMid = m_maxNumTransmitters;
    uint32_t maxMid = 0;
    uint32_t minHigh = m_maxNumTransmitters;
    for (int i = 0; i < 30; i++)
    {
        rand = tvTransHelper.GetRandomNumTransmitters(TvSpectrumTransmitterHelper::DENSITY_LOW,
                                                      m_maxNumTransmitters);
        NS_TEST_ASSERT_MSG_GT(rand, 0, "lower bound exceeded");
        if (rand > maxLow)
        {
            maxLow = rand;
        }
    }
    for (int i = 0; i < 30; i++)
    {
        rand = tvTransHelper.GetRandomNumTransmitters(TvSpectrumTransmitterHelper::DENSITY_MEDIUM,
                                                      m_maxNumTransmitters);
        if (rand < minMid)
        {
            minMid = rand;
        }
        if (rand > maxMid)
        {
            maxMid = rand;
        }
    }
    for (int i = 0; i < 30; i++)
    {
        rand = tvTransHelper.GetRandomNumTransmitters(TvSpectrumTransmitterHelper::DENSITY_HIGH,
                                                      m_maxNumTransmitters);
        NS_TEST_ASSERT_MSG_LT(rand, m_maxNumTransmitters + 1, "upper bound exceeded");
        if (rand < minHigh)
        {
            minHigh = rand;
        }
    }
    NS_TEST_ASSERT_MSG_LT(maxLow, minMid, "low density overlaps with medium density");
    NS_TEST_ASSERT_MSG_LT(maxMid, minHigh, "medium density overlaps with high density");
}

/**
 * @ingroup spectrum-tests
 *
 * Test suite for the TvSpectrumTransmitterHelper class
 */
class TvHelperDistributionTestSuite : public TestSuite
{
  public:
    TvHelperDistributionTestSuite();
};

TvHelperDistributionTestSuite::TvHelperDistributionTestSuite()
    : TestSuite("tv-helper-distribution", Type::UNIT)
{
    NS_LOG_INFO("creating TvHelperDistributionTestSuite");
    for (uint32_t maxNumTransmitters = 3; maxNumTransmitters <= 203; maxNumTransmitters += 10)
    {
        AddTestCase(new TvHelperDistributionTestCase(maxNumTransmitters),
                    TestCase::Duration::QUICK);
    }
}

/// Static variable for test initialization
static TvHelperDistributionTestSuite g_TvHelperDistributionTestSuite;
