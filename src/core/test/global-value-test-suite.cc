/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "ns3/global-value.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"

/**
 * @file
 * @ingroup core
 * @ingroup core-tests
 * @ingroup global-value-tests
 * GlobalValue test suite
 */

/**
 * @ingroup core-tests
 * @defgroup global-value-tests GlobalValue test suite
 */

namespace ns3
{

namespace tests
{

/**
 * @ingroup global-value-tests
 * Test for the ability to get at a GlobalValue.
 */
class GlobalValueTestCase : public TestCase
{
  public:
    /** Constructor. */
    GlobalValueTestCase();

    /** Destructor. */
    ~GlobalValueTestCase() override
    {
    }

  private:
    void DoRun() override;
};

GlobalValueTestCase::GlobalValueTestCase()
    : TestCase("Check GlobalValue mechanism")
{
}

void
GlobalValueTestCase::DoRun()
{
    //
    // Typically these are static globals but we can make one on the stack to
    // keep it hidden from the documentation.
    //
    GlobalValue uint =
        GlobalValue("TestUint", "help text", UintegerValue(10), MakeUintegerChecker<uint32_t>());

    //
    // Make sure we can get at the value and that it was initialized correctly.
    //
    UintegerValue uv;
    uint.GetValue(uv);
    NS_TEST_ASSERT_MSG_EQ(uv.Get(), 10, "GlobalValue \"TestUint\" not initialized as expected");

    //
    // Remove the global value for a valgrind clean run
    //
    GlobalValue::Vector* vector = GlobalValue::GetVector();
    for (auto i = vector->begin(); i != vector->end(); ++i)
    {
        if ((*i) == &uint)
        {
            vector->erase(i);
            break;
        }
    }
}

/**
 * @ingroup global-value-tests
 * The Test Suite that glues all of the Test Cases together.
 */
class GlobalValueTestSuite : public TestSuite
{
  public:
    /** Constructor. */
    GlobalValueTestSuite();
};

GlobalValueTestSuite::GlobalValueTestSuite()
    : TestSuite("global-value")
{
    AddTestCase(new GlobalValueTestCase);
}

/**
 * @ingroup global-value-tests
 * GlobalValueTestSuite instance variable.
 */
static GlobalValueTestSuite g_globalValueTestSuite;

} // namespace tests

} // namespace ns3
