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
 * Test for many uniform random variable streams.
 */

namespace ns3
{

namespace tests
{

/**
 * @ingroup rng-tests
 * Test case for many uniform distribution random variable stream generators
 */
class ManyUniformRandomVariablesOneGetValueCallTestCase : public TestCase
{
  public:
    ManyUniformRandomVariablesOneGetValueCallTestCase();
    ~ManyUniformRandomVariablesOneGetValueCallTestCase() override;

  private:
    void DoRun() override;
};

ManyUniformRandomVariablesOneGetValueCallTestCase::
    ManyUniformRandomVariablesOneGetValueCallTestCase()
    : TestCase("Many Uniform Random Variables with One GetValue() Call")
{
}

ManyUniformRandomVariablesOneGetValueCallTestCase::
    ~ManyUniformRandomVariablesOneGetValueCallTestCase()
{
}

void
ManyUniformRandomVariablesOneGetValueCallTestCase::DoRun()
{
    const double min = 0.0;
    const double max = 10.0;

    Config::SetDefault("ns3::UniformRandomVariable::Min", DoubleValue(min));
    Config::SetDefault("ns3::UniformRandomVariable::Max", DoubleValue(max));

    // Get 1 value from many uniform random number generators.
    double value;
    const int count = 1000000;
    std::vector<Ptr<UniformRandomVariable>> uniformStreamVector(count);
    for (int i = 0; i < count; i++)
    {
        uniformStreamVector.push_back(CreateObject<UniformRandomVariable>());
        value = uniformStreamVector.back()->GetValue();

        NS_TEST_ASSERT_MSG_GT(value, min, "Value less than minimum.");
        NS_TEST_ASSERT_MSG_LT(value, max, "Value greater than maximum.");
    }
}

/**
 * @ingroup rng-tests
 * Test suite for many uniform distribution random variable stream generators
 */
class ManyUniformRandomVariablesOneGetValueCallTestSuite : public TestSuite
{
  public:
    ManyUniformRandomVariablesOneGetValueCallTestSuite();
};

ManyUniformRandomVariablesOneGetValueCallTestSuite::
    ManyUniformRandomVariablesOneGetValueCallTestSuite()
    : TestSuite("many-uniform-random-variables-one-get-value-call", Type::PERFORMANCE)
{
    AddTestCase(new ManyUniformRandomVariablesOneGetValueCallTestCase);
}

/**
 * @ingroup rng-tests
 * ManuUniformRandomVariablesOneGetValueCallTestSuite instance variable.
 */
static ManyUniformRandomVariablesOneGetValueCallTestSuite
    g_manyUniformRandomVariablesOneGetValueCallTestSuite;

} // namespace tests

} // namespace ns3
