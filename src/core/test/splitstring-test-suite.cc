/*
 * Copyright (c) 2022 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "ns3/string.h"
#include "ns3/test.h"

namespace ns3
{

namespace tests
{

/**
 * @file
 * @ingroup core-tests
 * SplitString test suite implementation.
 */

/**
 * @ingroup core-tests
 * @defgroup environ-var-tests Environment variable caching tests
 */

/**
 * @ingroup core-tests
 *
 * SplitString tests.
 */
class SplitStringTestCase : public TestCase
{
  public:
    /** Constructor */
    SplitStringTestCase();

    /** Destructor */
    ~SplitStringTestCase() override = default;

  private:
    /** Run the tests */
    void DoRun() override;

    /**
     * Read \p str and check that it contains only the key,value pairs
     * from \p expect.
     * @param where The test condition being checked.
     * @param str The environment variable to parse and check.
     * @param expect The set of key,values expected.
     */
    void Check(const std::string& where, const std::string& str, const StringVector& expect);

    /** Test suite delimiter. */
    const std::string m_delimiter{":|:"};

    // end of class SplitStringTestCase
};

SplitStringTestCase::SplitStringTestCase()
    : TestCase("split-string")
{
}

void
SplitStringTestCase::Check(const std::string& where,
                           const std::string& str,
                           const StringVector& expect)
{
    const StringVector res = SplitString(str, m_delimiter);

    // Print the res and expect
    std::cout << where << ": '" << str << "'\nindex\texpect[" << expect.size() << "]\t\tresult["
              << res.size() << "]" << (expect.size() != res.size() ? "\tFAIL SIZE" : "")
              << "\n    ";
    NS_TEST_EXPECT_MSG_EQ(expect.size(),
                          res.size(),
                          "res and expect have different number of entries");
    for (std::size_t i = 0; i < std::max(res.size(), expect.size()); ++i)
    {
        const std::string r = (i < res.size() ? res[i] : "''" + std::to_string(i));
        const std::string e = (i < expect.size() ? expect[i] : "''" + std::to_string(i));
        const bool ok = (r == e);
        std::cout << i << "\t'" << e << (ok ? "'\t== '" : "'\t!= '") << r
                  << (!ok ? "'\tFAIL MATCH" : "'") << "\n    ";
        NS_TEST_EXPECT_MSG_EQ(e, r, "res[i] does not match expect[i]");
    }
    std::cout << std::endl;
}

void
SplitStringTestCase::DoRun()
{
    // Test points

    // Empty string
    Check("empty", "", {""});

    // No delimiter
    Check("no-delim", "token", {"token"});

    // Extra leading, trailing delimiter: ":string",  "string:"
    Check("front-:|:", ":|:token", {"", "token"});
    Check("back-:|:", "token:|:", {"token", ""});

    // Double delimiter: ":|::|:token", "token:|::|:"
    Check("front-:|::|:", ":|::|:token", {"", "", "token"});
    Check("back-:|::|:", "token:|::|:", {"token", "", ""});

    // Two tokens: "token1:|:token2"
    Check("two", "token1:|:token2", {"token1", "token2"});

    // Extra/double delimiters:|: ":|::|:token1:|:token2", ":|:token1:|:token2",
    // "token1:|::|:token2", "token1:|:token2:|:",
    Check(":|::|:two", ":|::|:token1:|:token2", {"", "", "token1", "token2"});
    Check(":|:one:|:two", ":|:token1:|:token2", {"", "token1", "token2"});
    Check("double:|:", "token1:|::|:token2", {"token1", "", "token2"});
    Check("two:|:", "token1:|:token2:|::|:", {"token1", "token2", "", ""});
}

/**
 * @ingroup typeid-tests
 *
 * TypeId test suites.
 */
class SplitStringTestSuite : public TestSuite
{
  public:
    SplitStringTestSuite();
};

SplitStringTestSuite::SplitStringTestSuite()
    : TestSuite("split-string")
{
    AddTestCase(new SplitStringTestCase);
}

/**
 * @ingroup environ-var-tests
 * Static variable for test initialization.
 */
static SplitStringTestSuite g_SplitStringTestSuite;

} // namespace tests

} // namespace ns3
