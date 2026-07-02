/*
 * Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

#include "ns3/example-as-test.h"
#include "ns3/log.h"
#include "ns3/test.h"

#include <list>
#include <map>
#include <set>
#include <sstream>
#include <vector>

/**
 * @file
 * @ingroup core-tests
 * @defgroup logging-tests Logging test suite
 * Logging test suite, checking the log lines emitted by log-line-test-example.
 */

namespace ns3
{

namespace tests
{

/**
 * @ingroup logging-tests
 * Verify that ParameterLogger limits the number of logged container elements.
 */
class ParameterLoggerTestCase : public TestCase
{
  public:
    /** Constructor. */
    ParameterLoggerTestCase();

  private:
    void DoRun() override;
};

ParameterLoggerTestCase::ParameterLoggerTestCase()
    : TestCase("Check ParameterLogger container output")
{
}

void
ParameterLoggerTestCase::DoRun()
{
    std::ostringstream stream;
    ParameterLogger logger(stream);
    const auto originalMaximum = logger.GetMaxLoggedContainerElements();
    logger.SetMaxLoggedContainerElements(2);

    logger << std::vector<int>{1, 2, 3} << 4;
    NS_TEST_EXPECT_MSG_EQ(stream.str(), "vector:[1, 2, ...], 4", "Unexpected vector output");

    stream.str("");
    ParameterLogger(stream) << std::list<int>{1, 2, 3};
    NS_TEST_EXPECT_MSG_EQ(stream.str(), "list:[1, 2, ...]", "Unexpected list output");

    stream.str("");
    ParameterLogger(stream) << std::set<int>{1, 2, 3};
    NS_TEST_EXPECT_MSG_EQ(stream.str(), "set:[1, 2, ...]", "Unexpected set output");

    stream.str("");
    ParameterLogger(stream) << std::map<int, int>{{1, 10}, {2, 20}, {3, 30}};
    NS_TEST_EXPECT_MSG_EQ(stream.str(), "map:[1: 10, 2: 20, ...]", "Unexpected map output");

    stream.str("");
    ParameterLogger(stream) << std::vector<int>{1, 2};
    NS_TEST_EXPECT_MSG_EQ(stream.str(), "vector:[1, 2]", "Unexpected exact-limit output");

    logger.SetMaxLoggedContainerElements(0);
    stream.str("");
    ParameterLogger(stream) << std::vector<int>{1, 2, 3};
    NS_TEST_EXPECT_MSG_EQ(stream.str(), "vector:[...]", "Unexpected zero-limit output");

    logger.SetMaxLoggedContainerElements(originalMaximum);
}

/**
 * @ingroup logging-tests
 * Run log-line-test-example as a test, checking the assembled log lines: the
 * time, node, function and context prefixes, nested logging from a
 * user-defined operator<<, and the line in progress emitted when the
 * program dies while assembling it.
 */
class LogTestSuite : public TestSuite
{
  public:
    LogTestSuite();
};

LogTestSuite::LogTestSuite()
    : TestSuite("log", Type::UNIT)
{
#ifdef NS3_LOG_ENABLE
    // Log lines are only assembled when logging is compiled in.
    AddTestCase(new ExampleAsTestCase("core-example-log-line-test",
                                      "log-line-test-example",
                                      NS_TEST_SOURCEDIR,
                                      "--time=9e18 --context=7 --label=lbl --nested"));
    AddTestCase(new ExampleAsTestCase("core-example-log-line-test-fatal",
                                      "log-line-test-example",
                                      NS_TEST_SOURCEDIR,
                                      "--time=9e18 --context=7 --label=lbl --nested --mode=fatal",
                                      false));
#ifdef NS3_ASSERT_ENABLE
    AddTestCase(new ExampleAsTestCase("core-example-log-line-test-assert",
                                      "log-line-test-example",
                                      NS_TEST_SOURCEDIR,
                                      "--time=9e18 --context=7 --label=lbl --mode=assert",
                                      false));
#endif
#endif

    AddTestCase(new ParameterLoggerTestCase, TestCase::Duration::QUICK);
}

/**
 * @ingroup logging-tests
 * LogTestSuite instance variable.
 */
static LogTestSuite g_logTestSuite;

} // namespace tests

} // namespace ns3
