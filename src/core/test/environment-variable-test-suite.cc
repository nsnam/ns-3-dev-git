/*
 * Copyright (c) 2022 Lawrence Livermore National Laboratory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "ns3/environment-variable.h"
#include "ns3/test.h"

#include <cstdlib>  // setenv, unsetenv
#include <stdlib.h> // getenv

namespace ns3
{

namespace tests
{

/**
 * \file
 * \ingroup environ-var-tests
 * Environment variable caching test suite
 */

/**
 * \ingroup core-tests
 * \defgroup environ-var-tests Environment variable caching tests
 */

/**
 * \ingroup environ-var-tests
 *
 * EnvironmentVariable tests
 */
class EnvVarTestCase : public TestCase
{
  public:
    /** Constructor */
    EnvVarTestCase();

    /** Destructor */
    ~EnvVarTestCase() override = default;

  private:
    /** Run the tests */
    void DoRun() override;

    /** The key,value store */
    using KeyValueStore = EnvironmentVariable::Dictionary::KeyValueStore;

    /** The return type from EnvironmentVariable::Get() */
    using KeyFoundType = EnvironmentVariable::KeyFoundType;

    /**
     * Set the test environment variable.
     * \param where The test condition being checked.
     * \param value The value to set.
     */
    void SetVariable(const std::string& where, const std::string& value);

    /**
     * Unset the test environment variable.
     * \param where The test condition being checked.
     */
    void UnsetVariable(const std::string& where);

    /**
     * Read \p envValue and check that it contains only the key,value pairs
     * from \p expect.
     * \param where The test condition being checked.
     * \param envValue The environment variable to parse and check.
     * \param expect The set of key,values expected.
     */
    void Check(const std::string& where, const std::string& envValue, KeyValueStore expect);

    /**
     * Set and Check the variable.
     * \param where The test condition being checked.
     * \param envValue The environment variable to parse and check.
     * \param expect The set of key,values expected.
     */
    void SetAndCheck(const std::string& where, const std::string& envValue, KeyValueStore expect);

    /**
     * Check the result from a Get.
     * \param where The test condition being checked.
     * \param key The key to check.
     * \param expect The expected result.
     */
    void CheckGet(const std::string& where, const std::string& key, KeyFoundType expect);

    /**
     * Set, Check, and Get a variable.
     * \param where The test condition being checked.
     * \param envValue The environment variable to parse and check.
     * \param expectDict The set of key,values expected.
     * \param key The key to check.
     * \param expectValue The expected result.
     */
    void SetCheckAndGet(const std::string& where,
                        const std::string& envValue,
                        KeyValueStore expectDict,
                        const std::string& key,
                        KeyFoundType expectValue);

    /** Always use a non-default delimiter. */
    const std::string m_delimiter{"|"};

    /** Test environment variable name. */
    const std::string m_variable{"NS_ENVVAR_TEST"};

}; // class EnvVarTestCase

EnvVarTestCase::EnvVarTestCase()
    : TestCase("environment-variable-cache")
{
}

void
EnvVarTestCase::SetVariable(const std::string& where, const std::string& value)
{
    EnvironmentVariable::Clear();
    int ok = setenv(m_variable.c_str(), value.c_str(), 1);
    NS_TEST_EXPECT_MSG_EQ(ok, 0, where << ": failed to set variable");

    // Double check
    const char* envCstr = std::getenv(m_variable.c_str());
    NS_TEST_EXPECT_MSG_NE(envCstr, nullptr, where << ": failed to retrieve variable just set");
    NS_TEST_EXPECT_MSG_EQ(envCstr, value, where << ": failed to retrieve value just set");
}

void
EnvVarTestCase::UnsetVariable(const std::string& where)
{
    EnvironmentVariable::Clear();
    int ok = unsetenv(m_variable.c_str());
    NS_TEST_EXPECT_MSG_EQ(ok, 0, where << ": failed to unset variable");
}

void
EnvVarTestCase::Check(const std::string& where, const std::string& envValue, KeyValueStore expect)
{
    auto dict = EnvironmentVariable::GetDictionary(m_variable, m_delimiter)->GetStore();

    // Print the dict and expect
    std::size_t i{0};
    std::cout << where << " variable: '" << envValue << "' [" << dict.size() << "]:";
    for (const auto& v : dict)
    {
        std::cout << "\n    [" << i++ << "] '" << v.first << "'\t'" << v.second << "'";
    }
    std::cout << "\n  expect [" << expect.size() << "]:";
    i = 0;
    for (const auto& v : expect)
    {
        std::cout << "\n    [" << i++ << "] '" << v.first << "'\t'" << v.second << "'";
    }
    std::cout << "\n  dict[" << dict.size() << "], expect[" << expect.size() << "]" << std::endl;

    NS_TEST_ASSERT_MSG_EQ(dict.size(), expect.size(), where << ": unequal dictionary sizes");

    // Check that no extra key,value appear
    std::cout << "  extras:\n";
    i = 0;
    for (const auto& v : dict)
    {
        auto loc = expect.find(v.first);
        bool ok = loc != expect.end();
        std::cout << "    [" << i++ << "] '" << v.first << "', '" << v.second << "' "
                  << (ok ? "found" : "not found") << std::endl;
        NS_TEST_ASSERT_MSG_EQ((loc != expect.end()),
                              true,
                              where << ": unexpected key found: " << v.first << ", " << v.second);
    }
    std::cout << "  no extra keys appear" << std::endl;

    // Check that all expected key,value appear
    std::cout << "  expected:\n";
    i = 0;
    for (const auto& v : expect)
    {
        auto loc = dict.find(v.first);
        bool ok = loc != dict.end();
        std::cout << "    [" << i++ << "] '" << v.first << "', '" << v.second << "' "
                  << (ok ? "found" : "not found") << std::endl;
        NS_TEST_ASSERT_MSG_EQ((loc != dict.end()),
                              true,
                              where << ": expected key not found: " << v.second);

        NS_TEST_ASSERT_MSG_EQ(v.second, loc->second, where << ": value mismatch");
    }
    std::cout << "  all expected key,value appear\n" << std::endl;
}

void
EnvVarTestCase::SetAndCheck(const std::string& where,
                            const std::string& envValue,
                            KeyValueStore expect)
{
    SetVariable(where, envValue);
    Check(where, envValue, expect);
}

void
EnvVarTestCase::CheckGet(const std::string& where, const std::string& key, KeyFoundType expect)
{
    auto [found, value] = EnvironmentVariable::Get(m_variable, key, m_delimiter);
    NS_TEST_EXPECT_MSG_EQ(found,
                          expect.first,
                          where << ": key '" << key << "' " << (expect.first ? "not " : "")
                                << "found unexpectedly");
    NS_TEST_EXPECT_MSG_EQ(value,
                          expect.second,
                          where << ": incorrect value for key '" << key << "'");
}

void
EnvVarTestCase::SetCheckAndGet(const std::string& where,
                               const std::string& envValue,
                               KeyValueStore expectDict,
                               const std::string& key,
                               KeyFoundType expectValue)
{
    SetAndCheck(where, envValue, expectDict);
    CheckGet(where, key, expectValue);
}

void
EnvVarTestCase::DoRun()
{
    // Test non-default delimiters by default
    // Test empty;no delimiter

    // Test points

    // Environment variable not set.
    UnsetVariable("unset");
    Check("unset", "", {});
    auto [found, value] = EnvironmentVariable::Get(m_variable);
    NS_TEST_EXPECT_MSG_EQ(found, false, "unset: variable found when not set");
    NS_TEST_EXPECT_MSG_EQ(value, "", "unset: non-empty value from unset variable");

    // Variable set but empty
    SetCheckAndGet("empty", "", {}, "", {true, ""});

    // Key not in variable
    SetCheckAndGet("no-key",
                   "not|the|right=value",
                   {{"not", ""}, {"the", ""}, {"right", "value"}},
                   "no-key",
                   {false, ""});

    // Key only (no delimiter):  "key"
    SetCheckAndGet("key-only", "key", {{"key", ""}}, "key", {true, ""});

    // Extra delimiter: ":key",  "key:"
    SetCheckAndGet("front-|", "|key", {{"key", ""}}, "key", {true, ""});
    SetCheckAndGet("back-|", "key|", {{"key", ""}}, "key", {true, ""});

    // Double delimiter: "||key", "key||"
    SetCheckAndGet("front-||", "||key", {{"key", ""}}, "key", {true, ""});
    SetCheckAndGet("back-||", "key||", {{"key", ""}}, "key", {true, ""});

    // Two keys: "key1|key2"
    SetCheckAndGet("two keys", "key1|key2", {{"key1", ""}, {"key2", ""}}, "key1", {true, ""});
    CheckGet("two keys", "key2", {true, ""});

    // Extra/double delimiters| "||key1|key2", "|key1|key2", "key1||key2", "key1|key2|",
    // "key1|key2||"
    SetCheckAndGet("||two keys", "||key1|key2", {{"key1", ""}, {"key2", ""}}, "key1", {true, ""});
    CheckGet("||two keys", "key2", {true, ""});
    SetCheckAndGet("two keys||", "key1|key2||", {{"key1", ""}, {"key2", ""}}, "key1", {true, ""});
    CheckGet("two keys||", "key2", {true, ""});

    // Key=value: "key=value"
    SetCheckAndGet("key-val", "key=value", {{"key", "value"}}, "key", {true, "value"});

    // Mixed key-only, key=value| "key1|key2=|key3|key4=value"
    SetCheckAndGet("mixed",
                   "key1|key2=value|key3|key4=value",
                   {{"key1", ""}, {"key2", "value"}, {"key3", ""}, {"key4", "value"}},
                   "key1",
                   {true, ""});
    CheckGet("mixed", "key2", {true, "value"});
    CheckGet("mixed", "key3", {true, ""});
    CheckGet("mixed", "key4", {true, "value"});

    // Empty/missing value| "key="
    SetCheckAndGet("key=", "key=", {{"key", ""}}, "key", {true, ""});

    // Extra `=`| "key==value"
    SetCheckAndGet("key==", "key==", {{"key", "="}}, "key", {true, "="});
}

/**
 * \ingroup typeid-tests
 *
 * TypeId test suites.
 */
class EnvironmentVariableTestSuite : public TestSuite
{
  public:
    EnvironmentVariableTestSuite();
};

EnvironmentVariableTestSuite::EnvironmentVariableTestSuite()
    : TestSuite("environment-variables")
{
    AddTestCase(new EnvVarTestCase);
}

/**
 * \ingroup environ-var-tests
 * Static variable for test initialization.
 */
static EnvironmentVariableTestSuite g_EnvironmentVariableTestSuite;

} // namespace tests

} // namespace ns3
