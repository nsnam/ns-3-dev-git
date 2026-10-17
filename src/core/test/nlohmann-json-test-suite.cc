/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tom Henderson <tomh@tomh.org>
 */

#include "nlohmann/json.hpp"

#include "ns3/test.h"

#include <string>

/**
 * @file
 * @ingroup core-tests
 * Smoke test for the vendored nlohmann/json third-party header.
 */

namespace ns3
{

namespace tests
{

/**
 * @ingroup core-tests
 * Parse a trivial JSON document and check that the expected fields
 * round-trip correctly.
 */
class NlohmannJsonParseTestCase : public TestCase
{
  public:
    NlohmannJsonParseTestCase();
    ~NlohmannJsonParseTestCase() override = default;

  private:
    void DoRun() override;
};

NlohmannJsonParseTestCase::NlohmannJsonParseTestCase()
    : TestCase("Parse a trivial JSON document with nlohmann::json")
{
}

void
NlohmannJsonParseTestCase::DoRun()
{
    const std::string input = R"({
        "name": "ns-3",
        "version": 3,
        "enabled": true,
        "modules": ["core", "network", "internet"]
    })";

    nlohmann::json doc;
    bool parsed = false;
    try
    {
        doc = nlohmann::json::parse(input);
        parsed = true;
    }
    catch (const nlohmann::json::parse_error&)
    {
        parsed = false;
    }
    NS_TEST_ASSERT_MSG_EQ(parsed, true, "nlohmann::json::parse threw on valid JSON");

    NS_TEST_ASSERT_MSG_EQ(doc.is_object(), true, "Top-level value should be an object");
    NS_TEST_ASSERT_MSG_EQ(doc["name"].get<std::string>(),
                          "ns-3",
                          "String field did not round-trip");
    NS_TEST_ASSERT_MSG_EQ(doc["version"].get<int>(), 3, "Integer field did not round-trip");
    NS_TEST_ASSERT_MSG_EQ(doc["enabled"].get<bool>(), true, "Boolean field did not round-trip");
    NS_TEST_ASSERT_MSG_EQ(doc["modules"].is_array(), true, "Array field should be an array");
    NS_TEST_ASSERT_MSG_EQ(doc["modules"].size(),
                          static_cast<std::size_t>(3),
                          "Array field has unexpected length");
    NS_TEST_ASSERT_MSG_EQ(doc["modules"][0].get<std::string>(),
                          "core",
                          "Array element did not round-trip");
}

/**
 * @ingroup core-tests
 * nlohmann/json test suite.
 */
class NlohmannJsonTestSuite : public TestSuite
{
  public:
    NlohmannJsonTestSuite();
};

NlohmannJsonTestSuite::NlohmannJsonTestSuite()
    : TestSuite("nlohmann-json")
{
    AddTestCase(new NlohmannJsonParseTestCase);
}

/**
 * @ingroup core-tests
 * NlohmannJsonTestSuite instance variable.
 */
static NlohmannJsonTestSuite g_nlohmannJsonTestSuite;

} // namespace tests

} // namespace ns3
