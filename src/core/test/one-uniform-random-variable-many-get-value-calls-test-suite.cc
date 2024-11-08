/*
 * Copyright (c) 2012 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mitch Watrous (watrous@u.washington.edu)
 */

#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/random-variable-stream.h"
#include "ns3/test.h"

#include <vector>

/**
 * @file
 * @ingroup core-tests
 * @ingroup randomvariable
 * @ingroup rng-tests
 * Test for one uniform random variable stream.
 */

namespace ns3
{

namespace tests
{

/**
 * @ingroup rng-tests
 * Test case for one uniform distribution random variable stream generator
 */
class OneUniformRandomVariableManyGetValueCallsTestCase : public TestCase
{
  public:
    OneUniformRandomVariableManyGetValueCallsTestCase();
    ~OneUniformRandomVariableManyGetValueCallsTestCase() override;

  private:
    void DoRun() override;
};

OneUniformRandomVariableManyGetValueCallsTestCase::
    OneUniformRandomVariableManyGetValueCallsTestCase()
    : TestCase("One Uniform Random Variable with Many GetValue() Calls")
{
}

OneUniformRandomVariableManyGetValueCallsTestCase::
    ~OneUniformRandomVariableManyGetValueCallsTestCase()
{
}

void
OneUniformRandomVariableManyGetValueCallsTestCase::DoRun()
{
    const double min = 0.0;
    const double max = 10.0;

    Config::SetDefault("ns3::UniformRandomVariable::Min", DoubleValue(min));
    Config::SetDefault("ns3::UniformRandomVariable::Max", DoubleValue(max));

    Ptr<UniformRandomVariable> uniform = CreateObject<UniformRandomVariable>();

    // Get many values from 1 random number generator.
    double value;
    const int count = 100000000;
    for (int i = 0; i < count; i++)
    {
        value = uniform->GetValue();

        NS_TEST_ASSERT_MSG_GT(value, min, "Value less than minimum.");
        NS_TEST_ASSERT_MSG_LT(value, max, "Value greater than maximum.");
    }
}

/**
 * @ingroup rng-tests
 * Test suite for one uniform distribution random variable stream generator
 */
class OneUniformRandomVariableManyGetValueCallsTestSuite : public TestSuite
{
  public:
    OneUniformRandomVariableManyGetValueCallsTestSuite();
};

OneUniformRandomVariableManyGetValueCallsTestSuite::
    OneUniformRandomVariableManyGetValueCallsTestSuite()
    : TestSuite("one-uniform-random-variable-many-get-value-calls", Type::PERFORMANCE)
{
    AddTestCase(new OneUniformRandomVariableManyGetValueCallsTestCase);
}

/**
 * @ingroup rng-tests
 * OneUniformRandomVariableManyGetValueCallsTestSuite instance variable.
 */
static OneUniformRandomVariableManyGetValueCallsTestSuite
    g_oneUniformRandomVariableManyGetValueCallsTestSuite;

} // namespace tests

} // namespace ns3
