/*
 * Copyright (c) 2011 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mitch Watrous (watrous@u.washington.edu)
 */

#include "ns3/basic-data-calculators.h"
#include "ns3/test.h"

#include <cmath>

using namespace ns3;

// See issue #698 for discussion of this tolerance
const double TOLERANCE = 1e-13;

/**
 * @ingroup stats-tests
 *
 * @brief MinMaxAvgTotalCalculator class - Test case for a single integer.
 */
class OneIntegerTestCase : public TestCase
{
  public:
    OneIntegerTestCase();
    ~OneIntegerTestCase() override;

  private:
    void DoRun() override;
};

OneIntegerTestCase::OneIntegerTestCase()
    : TestCase("Basic Statistical Functions using One Integer")

{
}

OneIntegerTestCase::~OneIntegerTestCase()
{
}

void
OneIntegerTestCase::DoRun()
{
    MinMaxAvgTotalCalculator<int> calculator;

    long count = 1;

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
    variance = 0;
    stddev = std::sqrt(variance);

    // Test the calculator.
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getCount(), count, TOLERANCE, "Count value wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getSum(), sum, TOLERANCE, "Sum value wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getMin(), min, TOLERANCE, "Min value wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getMax(), max, TOLERANCE, "Max value wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getMean(), mean, TOLERANCE, "Mean value wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getStddev(), stddev, TOLERANCE, "Stddev value wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getVariance(),
                              variance,
                              TOLERANCE,
                              "Variance value wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getSqrSum(), sqrSum, TOLERANCE, "SqrSum value wrong");
}

/**
 * @ingroup stats-tests
 *
 * @brief MinMaxAvgTotalCalculator class - Test case for five integers.
 */
class FiveIntegersTestCase : public TestCase
{
  public:
    FiveIntegersTestCase();
    ~FiveIntegersTestCase() override;

  private:
    void DoRun() override;
};

FiveIntegersTestCase::FiveIntegersTestCase()
    : TestCase("Basic Statistical Functions using Five Integers")

{
}

FiveIntegersTestCase::~FiveIntegersTestCase()
{
}

void
FiveIntegersTestCase::DoRun()
{
    MinMaxAvgTotalCalculator<int> calculator;

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
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getCount(), count, TOLERANCE, "Count value wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getSum(), sum, TOLERANCE, "Sum value wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getMin(), min, TOLERANCE, "Min value wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getMax(), max, TOLERANCE, "Max value wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getMean(), mean, TOLERANCE, "Mean value wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getStddev(), stddev, TOLERANCE, "Stddev value wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getVariance(),
                              variance,
                              TOLERANCE,
                              "Variance value wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getSqrSum(), sqrSum, TOLERANCE, "SqrSum value wrong");
}

/**
 * @ingroup stats-tests
 *
 * @brief MinMaxAvgTotalCalculator class - Test case for five double values.
 */
class FiveDoublesTestCase : public TestCase
{
  public:
    FiveDoublesTestCase();
    ~FiveDoublesTestCase() override;

  private:
    void DoRun() override;
};

FiveDoublesTestCase::FiveDoublesTestCase()
    : TestCase("Basic Statistical Functions using Five Double Values")

{
}

FiveDoublesTestCase::~FiveDoublesTestCase()
{
}

void
FiveDoublesTestCase::DoRun()
{
    MinMaxAvgTotalCalculator<double> calculator;

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
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getCount(), count, TOLERANCE, "Count value wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getSum(), sum, TOLERANCE, "Sum value wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getMin(), min, TOLERANCE, "Min value wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getMax(), max, TOLERANCE, "Max value wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getMean(), mean, TOLERANCE, "Mean value wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getStddev(), stddev, TOLERANCE, "Stddev value wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getVariance(),
                              variance,
                              TOLERANCE,
                              "Variance value wrong");
    NS_TEST_ASSERT_MSG_EQ_TOL(calculator.getSqrSum(), sqrSum, TOLERANCE, "SqrSum value wrong");
}

/**
 * @ingroup stats-tests
 *
 * @brief MinMaxAvgTotalCalculator class TestSuite
 */
class BasicDataCalculatorsTestSuite : public TestSuite
{
  public:
    BasicDataCalculatorsTestSuite();
};

BasicDataCalculatorsTestSuite::BasicDataCalculatorsTestSuite()
    : TestSuite("basic-data-calculators", Type::UNIT)
{
    AddTestCase(new OneIntegerTestCase, TestCase::Duration::QUICK);
    AddTestCase(new FiveIntegersTestCase, TestCase::Duration::QUICK);
    AddTestCase(new FiveDoublesTestCase, TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static BasicDataCalculatorsTestSuite basicDataCalculatorsTestSuite;
