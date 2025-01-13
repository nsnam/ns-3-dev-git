/*
 * Copyright (c) 2011 The Boeing Company
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Tom Henderson <thomas.r.henderson@boeing.com>
 */
#include "ns3/log.h"
#include "ns3/lr-wpan-spectrum-value-helper.h"
#include "ns3/spectrum-value.h"
#include "ns3/test.h"

#include <cmath>

using namespace ns3;
using namespace ns3::lrwpan;

/**
 * @ingroup lr-wpan-test
 * @ingroup tests
 *
 * @brief LrWpan SpectrumValue Helper TestSuite
 */
class LrWpanSpectrumValueHelperTestCase : public TestCase
{
  public:
    LrWpanSpectrumValueHelperTestCase();
    ~LrWpanSpectrumValueHelperTestCase() override;

  private:
    void DoRun() override;
};

LrWpanSpectrumValueHelperTestCase::LrWpanSpectrumValueHelperTestCase()
    : TestCase("Test the 802.15.4 SpectrumValue helper class")
{
}

LrWpanSpectrumValueHelperTestCase::~LrWpanSpectrumValueHelperTestCase()
{
}

void
LrWpanSpectrumValueHelperTestCase::DoRun()
{
    LrWpanSpectrumValueHelper helper;
    Ptr<SpectrumValue> value;
    double pwrWatts;
    for (uint32_t chan = 11; chan <= 26; chan++)
    {
        // 50dBm = 100 W, -50dBm = 0.01 mW
        for (double pwrdBm = -50; pwrdBm < 50; pwrdBm += 10)
        {
            value = helper.CreateTxPowerSpectralDensity(pwrdBm, chan);
            pwrWatts = pow(10.0, pwrdBm / 10.0) / 1000;
            // Test that average power calculation is within +/- 25% of expected
            NS_TEST_ASSERT_MSG_EQ_TOL(helper.TotalAvgPower(value, chan),
                                      pwrWatts,
                                      pwrWatts / 4.0,
                                      "Not equal for channel " << chan << " pwrdBm " << pwrdBm);
        }
    }
}

/**
 * @ingroup lr-wpan-test
 * @ingroup tests
 *
 * @brief LrWpan SpectrumValue Helper TestSuite
 */
class LrWpanSpectrumValueHelperTestSuite : public TestSuite
{
  public:
    LrWpanSpectrumValueHelperTestSuite();
};

LrWpanSpectrumValueHelperTestSuite::LrWpanSpectrumValueHelperTestSuite()
    : TestSuite("lr-wpan-spectrum-value-helper", Type::UNIT)
{
    AddTestCase(new LrWpanSpectrumValueHelperTestCase, TestCase::Duration::QUICK);
}

static LrWpanSpectrumValueHelperTestSuite
    g_lrWpanSpectrumValueHelperTestSuite; //!< Static variable for test initialization
