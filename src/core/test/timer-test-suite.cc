/*
 * Copyright (c) 2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/timer.h"

/**
 * @file
 * @ingroup timer-tests
 * Timer test suite
 */

/**
 * @ingroup core-tests
 * @defgroup timer-tests Timer tests
 */

namespace
{

// clang-format off

/// Function with one int parameter.
void bari(int) {}
/// Function with two int parameters.
void bar2i(int, int) {}
/// Function with three int parameters.
void bar3i(int, int, int) {}
/// Function with four int parameters.
void bar4i(int, int, int, int) {}
/// Function with five int parameters.
void bar5i(int, int, int, int, int) {}
/// Function with one const int reference parameter.
void barcir(const int&) {}
/// Function with one int reference parameter.
void barir(int&) {}

// clang-format on

} // anonymous namespace

using namespace ns3;

/**
 * @ingroup timer-tests
 *
 * @brief Check correct state transitions.
 */
class TimerStateTestCase : public TestCase
{
  public:
    TimerStateTestCase();
    void DoRun() override;
};

TimerStateTestCase::TimerStateTestCase()
    : TestCase("Check correct state transitions")
{
}

void
TimerStateTestCase::DoRun()
{
    Timer timer = Timer(Timer::CANCEL_ON_DESTROY);

    timer.SetFunction(&bari);
    timer.SetArguments(1);
    timer.SetDelay(Seconds(10));
    NS_TEST_ASSERT_MSG_EQ(!timer.IsRunning(), true, "");
    NS_TEST_ASSERT_MSG_EQ(timer.IsExpired(), true, "");
    NS_TEST_ASSERT_MSG_EQ(!timer.IsSuspended(), true, "");
    NS_TEST_ASSERT_MSG_EQ(timer.GetState(), Timer::EXPIRED, "");
    timer.Schedule();
    NS_TEST_ASSERT_MSG_EQ(timer.IsRunning(), true, "");
    NS_TEST_ASSERT_MSG_EQ(!timer.IsExpired(), true, "");
    NS_TEST_ASSERT_MSG_EQ(!timer.IsSuspended(), true, "");
    NS_TEST_ASSERT_MSG_EQ(timer.GetState(), Timer::RUNNING, "");
    timer.Suspend();
    NS_TEST_ASSERT_MSG_EQ(!timer.IsRunning(), true, "");
    NS_TEST_ASSERT_MSG_EQ(!timer.IsExpired(), true, "");
    NS_TEST_ASSERT_MSG_EQ(timer.IsSuspended(), true, "");
    NS_TEST_ASSERT_MSG_EQ(timer.GetState(), Timer::SUSPENDED, "");
    timer.Resume();
    NS_TEST_ASSERT_MSG_EQ(timer.IsRunning(), true, "");
    NS_TEST_ASSERT_MSG_EQ(!timer.IsExpired(), true, "");
    NS_TEST_ASSERT_MSG_EQ(!timer.IsSuspended(), true, "");
    NS_TEST_ASSERT_MSG_EQ(timer.GetState(), Timer::RUNNING, "");
    timer.Cancel();
    NS_TEST_ASSERT_MSG_EQ(!timer.IsRunning(), true, "");
    NS_TEST_ASSERT_MSG_EQ(timer.IsExpired(), true, "");
    NS_TEST_ASSERT_MSG_EQ(!timer.IsSuspended(), true, "");
    NS_TEST_ASSERT_MSG_EQ(timer.GetState(), Timer::EXPIRED, "");
}

/**
 * @ingroup timer-tests
 *
 * @brief Check that Timer template magic is working.
 */
class TimerTemplateTestCase : public TestCase
{
  public:
    TimerTemplateTestCase();
    void DoRun() override;
    void DoTeardown() override;

    // clang-format off

    /// Member function with one int parameter.
    void bazi(int) {}
    /// Member function with two int parameters.
    void baz2i(int, int) {}
    /// Member function with three int parameters.
    void baz3i(int, int, int) {}
    /// Member function with four int parameters.
    void baz4i(int, int, int, int) {}
    /// Member function with five int parameters.
    void baz5i(int, int, int, int, int) {}
    /// Member function with six int parameters.
    void baz6i(int, int, int, int, int, int) {}
    /// Member function with one const int reference parameter.
    void bazcir(const int&) {}
    /// Member function with one int reference parameter.
    void bazir(int&) {}
    /// Member function with one int pointer parameter.
    void bazip(int*) {}
    /// Member function with one const int pointer parameter.
    void bazcip(const int*) {}

    // clang-format on
};

TimerTemplateTestCase::TimerTemplateTestCase()
    : TestCase("Check that template magic is working")
{
}

void
TimerTemplateTestCase::DoRun()
{
    Timer timer = Timer(Timer::CANCEL_ON_DESTROY);

    int a = 0;
    int& b = a;
    const int& c = a;

    timer.SetFunction(&bari);
    timer.SetArguments(2);
    timer.SetArguments(a);
    timer.SetArguments(b);
    timer.SetArguments(c);
    timer.SetFunction(&barir);
    timer.SetArguments(2);
    timer.SetArguments(a);
    timer.SetArguments(b);
    timer.SetArguments(c);
    timer.SetFunction(&barcir);
    timer.SetArguments(2);
    timer.SetArguments(a);
    timer.SetArguments(b);
    timer.SetArguments(c);
    // the following call cannot possibly work and is flagged by
    // a runtime error.
    // timer.SetArguments (0.0);
    timer.SetDelay(Seconds(1));
    timer.Schedule();

    timer.SetFunction(&TimerTemplateTestCase::bazi, this);
    timer.SetArguments(3);
    timer.SetFunction(&TimerTemplateTestCase::bazir, this);
    timer.SetArguments(3);
    timer.SetFunction(&TimerTemplateTestCase::bazcir, this);
    timer.SetArguments(3);

    timer.SetFunction(&bar2i);
    timer.SetArguments(1, 1);
    timer.SetFunction(&bar3i);
    timer.SetArguments(1, 1, 1);
    timer.SetFunction(&bar4i);
    timer.SetArguments(1, 1, 1, 1);
    timer.SetFunction(&bar5i);
    timer.SetArguments(1, 1, 1, 1, 1);
    // unsupported in simulator class
    // timer.SetFunction (&bar6i);
    // timer.SetArguments (1, 1, 1, 1, 1, 1);

    timer.SetFunction(&TimerTemplateTestCase::baz2i, this);
    timer.SetArguments(1, 1);
    timer.SetFunction(&TimerTemplateTestCase::baz3i, this);
    timer.SetArguments(1, 1, 1);
    timer.SetFunction(&TimerTemplateTestCase::baz4i, this);
    timer.SetArguments(1, 1, 1, 1);
    timer.SetFunction(&TimerTemplateTestCase::baz5i, this);
    timer.SetArguments(1, 1, 1, 1, 1);
    // unsupported in simulator class
    // timer.SetFunction (&TimerTemplateTestCase::baz6i, this);
    // timer.SetArguments (1, 1, 1, 1, 1, 1);

    Simulator::Run();
    Simulator::Destroy();
}

void
TimerTemplateTestCase::DoTeardown()
{
    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup timer-tests
 *
 * @brief The timer Test Suite.
 */
class TimerTestSuite : public TestSuite
{
  public:
    TimerTestSuite()
        : TestSuite("timer", Type::UNIT)
    {
        AddTestCase(new TimerStateTestCase(), TestCase::Duration::QUICK);
        AddTestCase(new TimerTemplateTestCase(), TestCase::Duration::QUICK);
    }
};

static TimerTestSuite g_timerTestSuite; //!< Static variable for test initialization
