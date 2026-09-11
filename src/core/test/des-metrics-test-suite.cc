/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Author: Srajan Gupta <srajangupta1818@gmail.com>
 */

#include "ns3/des-metrics.h"
#include "ns3/system-path.h"
#include "ns3/test.h"

#include <fstream>
#include <list>
#include <sstream>
#include <string>

/**
 * @file
 * @ingroup core-tests
 * Tests for the DesMetrics trace file header.
 */

namespace ns3
{

namespace tests
{

/**
 * @ingroup core-tests
 * Verify that DesMetrics writes the command line arguments into the JSON
 * header, and that it emits a placeholder when no arguments are available.
 */
class DesMetricsCommandLineArgsTestCase : public TestCase
{
  public:
    DesMetricsCommandLineArgsTestCase();
    ~DesMetricsCommandLineArgsTestCase() override = default;

  private:
    void DoRun() override;
};

DesMetricsCommandLineArgsTestCase::DesMetricsCommandLineArgsTestCase()
    : TestCase("Check DesMetrics command line arguments header")
{
}

void
DesMetricsCommandLineArgsTestCase::DoRun()
{
    std::string tempDir = CreateTempDirFilename("des-metrics-test.json");
    std::list<std::string> components = SystemPath::Split(tempDir);
    components.pop_back();
    tempDir = SystemPath::Join(components.begin(), components.end());

    // The first file, named after args[0], must record the arguments.
    // The second Initialize call closes the previous file. The second file,
    // opened with no arguments, must record the placeholder instead.
    DesMetrics::Get()->Initialize({"des-metrics-test", "--foo", "bar"}, tempDir);
    DesMetrics::Get()->Initialize({}, tempDir);
    DesMetrics::Get()->Initialize({"des-metrics-test-close"}, tempDir);

    auto readFile = [](const std::string& path) {
        std::ifstream file(path);
        std::ostringstream contents;
        contents << file.rdbuf();
        return contents.str();
    };

    const std::string withArgs = readFile(SystemPath::Append(tempDir, "des-metrics-test.json"));
    const std::string withoutArgs = readFile(SystemPath::Append(tempDir, "desTraceFile.json"));

    NS_TEST_ASSERT_MSG_NE(withArgs.find("\"command_line_arguments\" : "
                                        "\"des-metrics-test --foo bar\","),
                          std::string::npos,
                          "Command line arguments were not written into the header");
    NS_TEST_ASSERT_MSG_NE(withoutArgs.find("\"command_line_arguments\" : "
                                           "\"[argv empty or not available]\","),
                          std::string::npos,
                          "Placeholder was not written for empty arguments");
}

/**
 * @ingroup core-tests
 * DesMetrics test suite.
 */
class DesMetricsTestSuite : public TestSuite
{
  public:
    DesMetricsTestSuite();
};

DesMetricsTestSuite::DesMetricsTestSuite()
    : TestSuite("des-metrics", Type::UNIT)
{
    AddTestCase(new DesMetricsCommandLineArgsTestCase, TestCase::Duration::QUICK);
}

/**
 * @ingroup core-tests
 * DesMetricsTestSuite instance variable.
 */
static DesMetricsTestSuite g_desMetricsTestSuite;

} // namespace tests

} // namespace ns3
