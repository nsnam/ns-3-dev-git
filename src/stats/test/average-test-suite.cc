/*
 * Copyright (c) 2012 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mitch Watrous (watrous@u.washington.edu)
 */

#include "ns3/average.h"
#include "ns3/test.h"

#include <cmath>

using namespace ns3;

// Note, the rationale for this particular value of TOLERANCE is not
// documented.  Current value is sufficient for all test platforms.
const double TOLERANCE = 2e-14;

/**
 * @ingroup stats-tests
 *
 * @brief Average class - Test case for a single integer.
 */
class OneIntegerAverageTestCase : public TestCase
{
  public:
    OneIntegerAverageTestCase();
    ~OneIntegerAverageTestCase() override;

  private:
    void DoRun() override;
};

OneIntegerAverageTestCase::OneIntegerAverageTestCase()
    : TestCase("Average Object Test using One Integer")

{
}

OneIntegerAverageTestCase::~OneIntegerAverageTestCase()
{
}

void
OneIntegerAverageTestCase::DoRun()
{
    Average<int> calculator;

    long count = 1;

    double sum = 0;
    double min;
    double max;
    double mean;
    double stddev;
    double variance;

    // Put all of the values into the calculator.
    int multiple = 5;
    int value;
    for (long i = 0; i < count; i++)
    {
        value = multiple * (i + 1);

        calculator.Update(value);

        sum += value;
    }

    // Calculate the expected values for the statistical functions.
    min = multiple;
    max = multiple * count;
    mean = sum / count;
    variance = 0;
    stddev = std::sqrt(variance);

    // Test the calculator.
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.Count(),
                              count,
                              TOLERANCE,
                              "Count value outside of tolerance "
                                  << TOLERANCE << "; difference: " << calculator.Count() - count);
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.Min(),
                              min,
                              TOLERANCE,
                              "Min value outside of tolerance "
                                  << TOLERANCE << "; difference: " << calculator.Min() - min);
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.Max(),
                              max,
                              TOLERANCE,
                              "Max value outside of tolerance "
                                  << TOLERANCE << "; difference: " << calculator.Max() - max);
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.Mean(),
                              mean,
                              TOLERANCE,
                              "Mean value outside of tolerance "
                                  << TOLERANCE << "; difference: " << calculator.Mean() - mean);
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.Stddev(),
                              stddev,
                              TOLERANCE,
                              "Stddev value outside of tolerance "
                                  << TOLERANCE << "; difference: " << calculator.Stddev() - stddev);
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.Var(),
                              variance,
                              TOLERANCE,
                              "Variance value outside of tolerance "
                                  << TOLERANCE << "; difference: " << calculator.Var() - variance);
}

/**
 * @ingroup stats-tests
 *
 * @brief Average class - Test case for five integers.
 */
class FiveIntegersAverageTestCase : public TestCase
{
  public:
    FiveIntegersAverageTestCase();
    ~FiveIntegersAverageTestCase() override;

  private:
    void DoRun() override;
};

FiveIntegersAverageTestCase::FiveIntegersAverageTestCase()
    : TestCase("Average Object Test using Five Integers")

{
}

FiveIntegersAverageTestCase::~FiveIntegersAverageTestCase()
{
}

void
FiveIntegersAverageTestCase::DoRun()
{
    Average<int> calculator;

    long count = 5;

    double sum = 0;
    double sqrSum = 0;
    double min;
    double max;
    double mean;
    double stddev;
    double variance;

    // Put all of the values into the calculator.
    int multiple = 5;
    int value;
    for (long i = 0; i < count; i++)
    {
        value = multiple * (i + 1);

        calculator.Update(value);

        sum += value;
        sqrSum += value * value;
    }

    // Calculate the expected values for the statistical functions.
    min = multiple;
    max = multiple * count;
    mean = sum / count;
    variance = (count * sqrSum - sum * sum) / (count * (count - 1));
    stddev = std::sqrt(variance);

    // Test the calculator.
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.Count(),
                              count,
                              TOLERANCE,
                              "Count value outside of tolerance "
                                  << TOLERANCE << "; difference: " << calculator.Count() - count);
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.Min(),
                              min,
                              TOLERANCE,
                              "Min value outside of tolerance "
                                  << TOLERANCE << "; difference: " << calculator.Min() - min);
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.Max(),
                              max,
                              TOLERANCE,
                              "Max value outside of tolerance "
                                  << TOLERANCE << "; difference: " << calculator.Max() - max);
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.Mean(),
                              mean,
                              TOLERANCE,
                              "Mean value outside of tolerance "
                                  << TOLERANCE << "; difference: " << calculator.Mean() - mean);
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.Stddev(),
                              stddev,
                              TOLERANCE,
                              "Stddev value outside of tolerance "
                                  << TOLERANCE << "; difference: " << calculator.Stddev() - stddev);
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.Var(),
                              variance,
                              TOLERANCE,
                              "Variance value outside of tolerance "
                                  << TOLERANCE << "; difference: " << calculator.Var() - variance);
}

/**
 * @ingroup stats-tests
 *
 * @brief Average class - Test case for five double values.
 */
class FiveDoublesAverageTestCase : public TestCase
{
  public:
    FiveDoublesAverageTestCase();
    ~FiveDoublesAverageTestCase() override;

  private:
    void DoRun() override;
};

FiveDoublesAverageTestCase::FiveDoublesAverageTestCase()
    : TestCase("Average Object Test using Five Double Values")

{
}

FiveDoublesAverageTestCase::~FiveDoublesAverageTestCase()
{
}

void
FiveDoublesAverageTestCase::DoRun()
{
    Average<double> calculator;

    long count = 5;

    double sum = 0;
    double sqrSum = 0;
    double min;
    double max;
    double mean;
    double stddev;
    double variance;

    // Put all of the values into the calculator.
    double multiple = 3.14;
    double value;
    for (long i = 0; i < count; i++)
    {
        value = multiple * (i + 1);

        calculator.Update(value);

        sum += value;
        sqrSum += value * value;
    }

    // Calculate the expected values for the statistical functions.
    min = multiple;
    max = multiple * count;
    mean = sum / count;
    variance = (count * sqrSum - sum * sum) / (count * (count - 1));
    stddev = std::sqrt(variance);

    // Test the calculator.
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.Count(),
                              count,
                              TOLERANCE,
                              "Count value outside of tolerance "
                                  << TOLERANCE << "; difference: " << calculator.Count() - count);
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.Min(),
                              min,
                              TOLERANCE,
                              "Min value outside of tolerance "
                                  << TOLERANCE << "; difference: " << calculator.Min() - min);
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.Max(),
                              max,
                              TOLERANCE,
                              "Max value outside of tolerance "
                                  << TOLERANCE << "; difference: " << calculator.Max() - max);
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.Mean(),
                              mean,
                              TOLERANCE,
                              "Mean value outside of tolerance "
                                  << TOLERANCE << "; difference: " << calculator.Mean() - mean);
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.Stddev(),
                              stddev,
                              TOLERANCE,
                              "Stddev value outside of tolerance "
                                  << TOLERANCE << "; difference: " << calculator.Stddev() - stddev);
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.Var(),
                              variance,
                              TOLERANCE,
                              "Variance value outside of tolerance "
                                  << TOLERANCE << "; difference: " << calculator.Var() - variance);
}

/**
 * @ingroup stats-tests
 *
 * @brief Average class TestSuite
 */
class AverageTestSuite : public TestSuite
{
  public:
    AverageTestSuite();
};

AverageTestSuite::AverageTestSuite()
    : TestSuite("average", Type::UNIT)
{
    AddTestCase(new OneIntegerAverageTestCase, TestCase::Duration::QUICK);
    AddTestCase(new FiveIntegersAverageTestCase, TestCase::Duration::QUICK);
    AddTestCase(new FiveDoublesAverageTestCase, TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static AverageTestSuite averageTestSuite;
