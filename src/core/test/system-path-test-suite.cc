/*
 * Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/system-path.h"
#include "ns3/test.h"

#include <filesystem>
#include <fstream>

using namespace ns3;
using namespace ns3::SystemPath;

/**
 * @ingroup core-tests
 * @defgroup system-path-tests SystemPath test suite
 */

/**
 * @ingroup system-path-tests
 *
 * @brief Tests for SystemPath::Exists().
 *
 * Regression test for issue #781: Exists() used to derive the directory part
 * of the path and search it for the final component, which returned a wrong
 * result for a bare relative filename (the derived directory was the empty
 * string) and mishandled trailing separators. It now delegates to
 * std::filesystem and must agree with it.
 */
class SystemPathExistsTestCaseBug781 : public TestCase
{
  public:
    SystemPathExistsTestCaseBug781()
        : TestCase("SystemPath::Exists matches std::filesystem::exists")
    {
    }

  private:
    void DoRun() override;
};

void
SystemPathExistsTestCaseBug781::DoRun()
{
    namespace fs = std::filesystem;

    // RAII guards so that a failing assertion cannot leak the temporary
    // directory or leave the process working directory changed for later test
    // suites: every NS_TEST_ASSERT_MSG_EQ can return early from DoRun().
    struct DirRemover
    {
        std::string path; ///< Directory removed on scope exit.

        ~DirRemover()
        {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }
    };

    struct CwdRestorer
    {
        std::filesystem::path original; ///< Working directory restored on scope exit.

        ~CwdRestorer()
        {
            std::error_code ec;
            std::filesystem::current_path(original, ec);
        }
    };

    // Create a unique temporary directory and a file inside it.
    std::string tempDir = MakeTemporaryDirectoryName();
    MakeDirectories(tempDir);
    DirRemover dirRemover{tempDir};
    NS_TEST_ASSERT_MSG_EQ(Exists(tempDir), true, "Created directory should exist");
    // A trailing separator must still resolve to the existing directory. The
    // literal '/' is intentional: std::filesystem accepts it as a separator on
    // every platform, including Windows where SYSTEM_PATH_SEP is '\\'.
    NS_TEST_ASSERT_MSG_EQ(Exists(tempDir + "/"),
                          true,
                          "Directory with trailing slash should exist");

    std::string filePath = Append(tempDir, "a-file.txt");
    {
        std::ofstream f(filePath);
        f << "ns-3";
    }
    NS_TEST_ASSERT_MSG_EQ(Exists(filePath), true, "Created file should exist");
    NS_TEST_ASSERT_MSG_EQ(Exists(Append(tempDir, "missing.txt")),
                          false,
                          "Non-existent file must not exist");
    NS_TEST_ASSERT_MSG_EQ(Exists(Append(tempDir, "no/such/dir")),
                          false,
                          "Non-existent nested path must not exist");

    // Regression for issue #781: a bare relative filename (no separator) that
    // exists in the current working directory must be reported as existing.
    CwdRestorer cwdRestorer{fs::current_path()};
    fs::current_path(tempDir);
    NS_TEST_ASSERT_MSG_EQ(Exists("a-file.txt"),
                          true,
                          "Bare filename present in the current directory must exist");
    NS_TEST_ASSERT_MSG_EQ(Exists("definitely-not-here.txt"),
                          false,
                          "Bare filename absent from the current directory must not exist");

    // dirRemover and cwdRestorer clean up on scope exit (cwd restored first,
    // then the directory removed), including when an assertion fails early.
}

/**
 * @ingroup system-path-tests
 *
 * @brief SystemPath test suite.
 */
class SystemPathTestSuite : public TestSuite
{
  public:
    SystemPathTestSuite()
        : TestSuite("system-path", Type::UNIT)
    {
        AddTestCase(new SystemPathExistsTestCaseBug781, TestCase::Duration::QUICK);
    }
};

static SystemPathTestSuite g_systemPathTestSuite; //!< Static variable for test initialization
