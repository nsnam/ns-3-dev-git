/*
 * Copyright (c) 2020 Universita' di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "ns3/lollipop-counter.h"
#include "ns3/test.h"

#include <limits>
#include <string>

using namespace ns3;

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Lollipop Counter Test
 */
class LollipopCounterTest : public TestCase
{
  public:
    void DoRun() override;
    LollipopCounterTest();
};

LollipopCounterTest::LollipopCounterTest()
    : TestCase("Lollipop Counter implementation")
{
}

void
LollipopCounterTest::DoRun()
{
    LollipopCounter<uint8_t> counter8a;
    LollipopCounter<uint8_t> counter8b;

    bool greater;
    bool lesser;
    bool equal;
    bool isComparable;

    // tests with uint8_t
    counter8a = 240;
    counter8b = 5;

    greater = counter8a > counter8b;
    lesser = counter8a < counter8b;
    equal = counter8a == counter8b;
    isComparable = counter8a.IsComparable(counter8b);

    NS_TEST_EXPECT_MSG_EQ(greater, true, "240 is greater than 5");
    NS_TEST_EXPECT_MSG_EQ(lesser, false, "240 is not lesser than 5");
    NS_TEST_EXPECT_MSG_EQ(equal, false, "240 is not equal to 5");
    NS_TEST_EXPECT_MSG_EQ(isComparable, true, "240 is comparable with 5");

    counter8a = 250;
    counter8b = 5;

    greater = counter8a > counter8b;
    lesser = counter8a < counter8b;
    equal = counter8a == counter8b;
    isComparable = counter8a.IsComparable(counter8b);

    NS_TEST_EXPECT_MSG_EQ(greater, false, "250 is not greater than 5");
    NS_TEST_EXPECT_MSG_EQ(lesser, true, "250 is lesser than 5");
    NS_TEST_EXPECT_MSG_EQ(equal, false, "250 is not equal to 5");
    NS_TEST_EXPECT_MSG_EQ(isComparable, true, "250 is comparable with 5");

    counter8a = 127;
    counter8b = 1;

    greater = counter8a > counter8b;
    lesser = counter8a < counter8b;
    equal = counter8a == counter8b;
    isComparable = counter8a.IsComparable(counter8b);

    NS_TEST_EXPECT_MSG_EQ(greater, false, "127 is not greater than 1");
    NS_TEST_EXPECT_MSG_EQ(lesser, true, "127 is lesser than 1");
    NS_TEST_EXPECT_MSG_EQ(equal, false, "127 is not equal to 1");
    NS_TEST_EXPECT_MSG_EQ(isComparable, true, "127 is comparable with 1");

    counter8a = 127;
    counter8b = 115;

    greater = counter8a > counter8b;
    lesser = counter8a < counter8b;
    equal = counter8a == counter8b;
    isComparable = counter8a.IsComparable(counter8b);

    NS_TEST_EXPECT_MSG_EQ(greater, true, "127 is greater than 115");
    NS_TEST_EXPECT_MSG_EQ(lesser, false, "127 is not lesser than 115");
    NS_TEST_EXPECT_MSG_EQ(equal, false, "127 is not equal to 115");
    NS_TEST_EXPECT_MSG_EQ(isComparable, true, "127 is comparable with 115");

    counter8a = 127;
    counter8b = 100;

    greater = counter8a > counter8b;
    lesser = counter8a < counter8b;
    equal = counter8a == counter8b;
    isComparable = counter8a.IsComparable(counter8b);

    NS_TEST_EXPECT_MSG_EQ(greater, false, "127 is not greater than 100");
    NS_TEST_EXPECT_MSG_EQ(lesser, false, "127 is not lesser than 100");
    NS_TEST_EXPECT_MSG_EQ(equal, false, "127 is not equal to 100");
    NS_TEST_EXPECT_MSG_EQ(isComparable, false, "127 is not comparable with 100");

    counter8a = 12;
    counter8b = 233;

    greater = counter8a > counter8b;
    lesser = counter8a < counter8b;
    equal = counter8a == counter8b;
    isComparable = counter8a.IsComparable(counter8b);

    NS_TEST_EXPECT_MSG_EQ(greater, false, "12 is not greater than 233");
    NS_TEST_EXPECT_MSG_EQ(lesser, true, "12 is lesser than 233");
    NS_TEST_EXPECT_MSG_EQ(equal, false, "12 is not equal to 233");
    NS_TEST_EXPECT_MSG_EQ(isComparable, true, "12 is comparable with 233");

    counter8a = 255;
    counter8b = counter8a++;
    NS_TEST_EXPECT_MSG_EQ((255 == counter8b), true, "Correct interpretation of postfix operator");
    NS_TEST_EXPECT_MSG_EQ((0 == counter8a), true, "Correct interpretation of postfix operator");

    counter8a = 255;
    counter8b = ++counter8a;
    NS_TEST_EXPECT_MSG_EQ((0 == counter8b), true, "Correct interpretation of prefix operator");
    NS_TEST_EXPECT_MSG_EQ((0 == counter8a), true, "Correct interpretation of prefix operator");
}

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Lollipop Counter TestSuite
 */
class LollipopCounterTestSuite : public TestSuite
{
  public:
    LollipopCounterTestSuite();

  private:
};

LollipopCounterTestSuite::LollipopCounterTestSuite()
    : TestSuite("lollipop-counter", Type::UNIT)
{
    AddTestCase(new LollipopCounterTest(), TestCase::Duration::QUICK);
}

static LollipopCounterTestSuite
    g_lollipopCounterTestSuite; //!< Static variable for test initialization
