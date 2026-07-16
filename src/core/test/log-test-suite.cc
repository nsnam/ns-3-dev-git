/*
 * Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

#include "ns3/example-as-test.h"

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
}

/**
 * @ingroup logging-tests
 * LogTestSuite instance variable.
 */
static LogTestSuite g_logTestSuite;

} // namespace tests

} // namespace ns3
