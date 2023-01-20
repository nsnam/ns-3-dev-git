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

#include <cstdlib> // getenv

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
    ~EnvVarTestCase() override;

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

EnvVarTestCase::~EnvVarTestCase()
{
    UnsetVariable("destructor");
}

void
EnvVarTestCase::SetVariable(const std::string& where, const std::string& value)
{
    EnvironmentVariable::Clear();
    bool ok = EnvironmentVariable::Set(m_variable, value);
    NS_TEST_EXPECT_MSG_EQ(ok, true, where << ": failed to set variable");

    // Double check
    const char* envCstr = std::getenv(m_variable.c_str());
    NS_TEST_EXPECT_MSG_NE(envCstr, nullptr, where << ": failed to retrieve variable just set");
    NS_TEST_EXPECT_MSG_EQ(envCstr, value, where << ": failed to retrieve value just set");
}

void
EnvVarTestCase::UnsetVariable(const std::string& where)
{
    EnvironmentVariable::Clear();
    bool ok = EnvironmentVariable::Unset(where);
    NS_TEST_EXPECT_MSG_EQ(ok, true, where << ": failed to unset variable");
}

void
EnvVarTestCase::Check(const std::string& where, const std::string& envValue, KeyValueStore expect)
{
    auto dict = EnvironmentVariable::GetDictionary(m_variable, m_delimiter)->GetStore();

    // Print the expect and which ones were found in dict
    std::cout << "\n"
              << where << " variable: '" << envValue << "', expect[" << expect.size() << "]"
              << ", dict[" << dict.size() << "]\n";

    NS_TEST_EXPECT_MSG_EQ(dict.size(), expect.size(), where << ": unequal dictionary sizes");

    std::size_t i{0};
    for (const auto& kv : expect)
    {
        std::cout << "    [" << i++ << "] '" << kv.first << "'\t'" << kv.second << "'";

        auto loc = dict.find(kv.first);
        bool found = loc != dict.end();
        std::cout << (found ? "\tfound" : "\tNOT FOUND");
        NS_TEST_EXPECT_MSG_EQ(found, true, where << ": expected key not found: " << kv.second);

        if (found)
        {
            bool match = kv.second == loc->second;
            if (match)
            {
                std::cout << ", match";
            }
            else
            {
                std::cout << ", NO MATCH: '" << loc->second << "'";
            }
            NS_TEST_EXPECT_MSG_EQ(kv.second, loc->second, where << ": key found, value mismatch");
        }
        std::cout << "\n";
        ++i;
    }

    // Now just check dict for unexpected values
    i = 0;
    bool first{true};
    for (const auto& kv : dict)
    {
        bool found = expect.find(kv.first) != expect.end();
        if (!found)
        {
            std::cout << (first ? "Unexpected keys:" : "");
            first = false;
            std::cout << "    [" << i << "] '" << kv.first << "'\t'" << kv.second << "'"
                      << " unexpected key, value\n";
        }
        ++i;
    }
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
    // Environment variable not set.
    UnsetVariable("unset");
    Check("unset", "", {});
    auto [found, value] = EnvironmentVariable::Get(m_variable);
    NS_TEST_EXPECT_MSG_EQ(found, false, "unset: variable found when not set");
    NS_TEST_EXPECT_MSG_EQ(value.empty(), true, "unset: non-empty value from unset variable");

    // Variable set but empty
#ifndef __WIN32__
    // Windows doesn't support environment variables with empty values
    SetCheckAndGet("empty", "", {}, "", {true, ""});
#endif

    // Key not in variable
    SetCheckAndGet("no-key",
                   "not|the|right=value",
                   {{"not", ""}, {"the", ""}, {"right", "value"}},
                   "key",
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

    // Finish last line of verbose output
    std::cout << std::endl;
}

/**
 * \ingroup environ-var-tests
 *
 * Environment variable handling test suite.
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
