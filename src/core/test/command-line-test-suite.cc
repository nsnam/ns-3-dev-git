/*
 * Copyright (c) 2008 INRIA
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/global-value.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/system-path.h"
#include "ns3/test.h"
#include "ns3/type-id.h"

#include <cstdarg>
#include <cstdlib>
#include <sstream>

/**
 * \file
 * \ingroup core-tests
 * \ingroup commandline
 * \ingroup commandline-tests
 * CommandLine test suite.
 */

/**
 * \ingroup core-tests
 * \defgroup commandline-tests CommandLine test suite
 */

namespace ns3
{

namespace tests
{

/**
 * \ingroup commandline-tests
 * A test base class that drives Command Line parsing
 */
class CommandLineTestCaseBase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * \param description purpose of this TestCase
     */
    CommandLineTestCaseBase(std::string description);

    /** Destructor */
    ~CommandLineTestCaseBase() override
    {
    }

    /**
     * Exercise the CommandLine with the provided arguments
     *
     * \param cmd the configured CommandLine
     * \param n the number of arguments
     */
    void Parse(CommandLine& cmd, int n, ...);

    /** Test iteration counter to give each test a unique name. */
    static int m_count;
};

int CommandLineTestCaseBase::m_count = 0;

CommandLineTestCaseBase::CommandLineTestCaseBase(std::string description)
    : TestCase(description)
{
}

void
CommandLineTestCaseBase::Parse(CommandLine& cmd, int n, ...)
{
    std::stringstream ss;
    ss << GetParent()->GetName() << "-testcase-" << m_count << "-" << GetName();
    ++m_count;

    int argc = n + 1;                  // test name will go in argv[0], other n to follow
    char** argv = new char*[argc + 1]; // extra entry for final null
    argv[argc] = nullptr;

    argv[0] = new char[strlen(ss.str().c_str()) + 1];
    strcpy(argv[0], ss.str().c_str());

    va_list ap;
    va_start(ap, n);
    for (int i = 1; i < argc; ++i)
    {
        char* arg = va_arg(ap, char*);
        argv[i] = new char[strlen(arg) + 1];
        strcpy(argv[i], arg);
    }
    va_end(ap);

    cmd.Parse(argc, argv);

    // Clean up all the new's
    for (int i = 0; i < argc; ++i)
    {
        delete[] argv[i];
    }
    delete[] argv;
}

/**
 * \ingroup commandline-tests
 * Test boolean Command Line processing
 */
class CommandLineBooleanTestCase : public CommandLineTestCaseBase
{
  public:
    /** Constructor */
    CommandLineBooleanTestCase();

    /** Destructor */
    ~CommandLineBooleanTestCase() override
    {
    }

  private:
    /** Run the test */
    void DoRun() override;
};

CommandLineBooleanTestCase::CommandLineBooleanTestCase()
    : CommandLineTestCaseBase("boolean")
{
}

void
CommandLineBooleanTestCase::DoRun()
{
    CommandLine cmd;
    bool myBool = true;
    bool myDefaultFalseBool = false;

    cmd.AddValue("my-bool", "help", myBool);
    cmd.AddValue("my-false-bool", "help", myDefaultFalseBool);

    Parse(cmd, 1, "--my-bool=0");
    NS_TEST_ASSERT_MSG_EQ(myBool,
                          false,
                          "CommandLine did not correctly set a boolean value to false, given 0");

    Parse(cmd, 1, "--my-bool=1");
    NS_TEST_ASSERT_MSG_EQ(myBool,
                          true,
                          "CommandLine did not correctly set a boolean value to true, given 1");

    Parse(cmd, 1, "--my-bool");
    NS_TEST_ASSERT_MSG_EQ(myBool,
                          false,
                          "CommandLine did not correctly toggle a default true boolean value to "
                          "false, given no argument");

    Parse(cmd, 1, "--my-false-bool");
    NS_TEST_ASSERT_MSG_EQ(myDefaultFalseBool,
                          true,
                          "CommandLine did not correctly toggle a default false boolean value to "
                          "true, given no argument");

    Parse(cmd, 1, "--my-bool=t");
    NS_TEST_ASSERT_MSG_EQ(
        myBool,
        true,
        "CommandLine did not correctly set a boolean value to true, given 't' argument");

    Parse(cmd, 1, "--my-bool=true");
    NS_TEST_ASSERT_MSG_EQ(
        myBool,
        true,
        "CommandLine did not correctly set a boolean value to true, given \"true\" argument");
}

/**
 * \ingroup commandline-tests
 * Test \c uint8_t Command Line processing
 */
class CommandLineUint8tTestCase : public CommandLineTestCaseBase
{
  public:
    /** Constructor */
    CommandLineUint8tTestCase();

    /** Destructor */
    ~CommandLineUint8tTestCase() override
    {
    }

  private:
    /** Run the test */
    void DoRun() override;
};

CommandLineUint8tTestCase::CommandLineUint8tTestCase()
    : CommandLineTestCaseBase("uint8_t")
{
}

void
CommandLineUint8tTestCase::DoRun()
{
    CommandLine cmd;
    uint8_t myUint8 = 10;

    cmd.AddValue("my-uint8", "help", myUint8);

    Parse(cmd, 1, "--my-uint8=1");
    NS_TEST_ASSERT_MSG_EQ(myUint8,
                          1,
                          "CommandLine did not correctly set a uint8_t value to 1, given 1");
}

/**
 * \ingroup commandline-tests
 * Test int Command Line processing
 */
class CommandLineIntTestCase : public CommandLineTestCaseBase
{
  public:
    /** Constructor */
    CommandLineIntTestCase();

    /** Destructor */
    ~CommandLineIntTestCase() override
    {
    }

  private:
    /** Run the test */
    void DoRun() override;
};

CommandLineIntTestCase::CommandLineIntTestCase()
    : CommandLineTestCaseBase("int")
{
}

void
CommandLineIntTestCase::DoRun()
{
    CommandLine cmd;
    bool myBool = true;
    int32_t myInt32 = 10;

    cmd.AddValue("my-bool", "help", myBool);
    cmd.AddValue("my-int32", "help", myInt32);

    Parse(cmd, 2, "--my-bool=0", "--my-int32=-3");
    NS_TEST_ASSERT_MSG_EQ(myBool,
                          false,
                          "CommandLine did not correctly set a boolean value to false");
    NS_TEST_ASSERT_MSG_EQ(myInt32, -3, "CommandLine did not correctly set an integer value to -3");

    Parse(cmd, 2, "--my-bool=1", "--my-int32=+2");
    NS_TEST_ASSERT_MSG_EQ(myBool,
                          true,
                          "CommandLine did not correctly set a boolean value to true");
    NS_TEST_ASSERT_MSG_EQ(myInt32, +2, "CommandLine did not correctly set an integer value to +2");
}

/**
 * \ingroup commandline-tests
 * Test unsigned int Command Line processing
 */
class CommandLineUnsignedIntTestCase : public CommandLineTestCaseBase
{
  public:
    /** Constructor */
    CommandLineUnsignedIntTestCase();

    /** Destructor */
    ~CommandLineUnsignedIntTestCase() override
    {
    }

  private:
    /** Run the test */
    void DoRun() override;
};

CommandLineUnsignedIntTestCase::CommandLineUnsignedIntTestCase()
    : CommandLineTestCaseBase("unsigned-int")
{
}

void
CommandLineUnsignedIntTestCase::DoRun()
{
    CommandLine cmd;
    bool myBool = true;
    uint32_t myUint32 = 10;

    cmd.AddValue("my-bool", "help", myBool);
    cmd.AddValue("my-uint32", "help", myUint32);

    Parse(cmd, 2, "--my-bool=0", "--my-uint32=9");

    NS_TEST_ASSERT_MSG_EQ(myBool,
                          false,
                          "CommandLine did not correctly set a boolean value to false");
    NS_TEST_ASSERT_MSG_EQ(myUint32,
                          9,
                          "CommandLine did not correctly set an unsigned integer value to 9");
}

/**
 * \ingroup commandline-tests
 * Test string Command Line processing
 */
class CommandLineStringTestCase : public CommandLineTestCaseBase
{
  public:
    /** Constructor */
    CommandLineStringTestCase();

    /** Destructor */
    ~CommandLineStringTestCase() override
    {
    }

  private:
    /** Run the test */
    void DoRun() override;
};

CommandLineStringTestCase::CommandLineStringTestCase()
    : CommandLineTestCaseBase("string")
{
}

void
CommandLineStringTestCase::DoRun()
{
    CommandLine cmd;
    uint32_t myUint32 = 10;
    std::string myStr = "MyStr";

    cmd.AddValue("my-uint32", "help", myUint32);
    cmd.AddValue("my-str", "help", myStr);

    Parse(cmd, 2, "--my-uint32=9", "--my-str=XX");

    NS_TEST_ASSERT_MSG_EQ(myUint32,
                          9,
                          "CommandLine did not correctly set an unsigned integer value to 9");
    NS_TEST_ASSERT_MSG_EQ(myStr,
                          "XX",
                          "CommandLine did not correctly set a string value to \"XX\"");
}

/**
 * \ingroup commandline-tests
 * Test order of argument parsing
 */
class CommandLineOrderTestCase : public CommandLineTestCaseBase
{
  public:
    /** Constructor */
    CommandLineOrderTestCase();

    /** Destructor */
    ~CommandLineOrderTestCase() override
    {
    }

  private:
    /** Run the test */
    void DoRun() override;
};

CommandLineOrderTestCase::CommandLineOrderTestCase()
    : CommandLineTestCaseBase("order")
{
}

void
CommandLineOrderTestCase::DoRun()
{
    CommandLine cmd;
    uint32_t myUint32 = 0;

    cmd.AddValue("my-uint32", "help", myUint32);

    Parse(cmd, 2, "--my-uint32=1", "--my-uint32=2");

    NS_TEST_ASSERT_MSG_EQ(myUint32,
                          2,
                          "CommandLine did not correctly set an unsigned integer value to 2");
}

/**
 * \ingroup commandline-tests
 * Test ignoring invalid arguments
 */
class CommandLineInvalidTestCase : public CommandLineTestCaseBase
{
  public:
    /** Constructor */
    CommandLineInvalidTestCase();

    /** Destructor */
    ~CommandLineInvalidTestCase() override
    {
    }

  private:
    /** Run the test */
    void DoRun() override;
};

CommandLineInvalidTestCase::CommandLineInvalidTestCase()
    : CommandLineTestCaseBase("invalid")
{
}

void
CommandLineInvalidTestCase::DoRun()
{
    CommandLine cmd;
    uint32_t myUint32 = 0;

    cmd.AddValue("my-uint32", "help", myUint32);

    Parse(cmd, 2, "quack", "--my-uint32=5");

    NS_TEST_ASSERT_MSG_EQ(myUint32,
                          5,
                          "CommandLine did not correctly set an unsigned integer value to 5");
}

/**
 * \ingroup commandline-tests
 * Test non-option arguments
 */
class CommandLineNonOptionTestCase : public CommandLineTestCaseBase
{
  public:
    /** Constructor */
    CommandLineNonOptionTestCase();

    /** Destructor */
    ~CommandLineNonOptionTestCase() override
    {
    }

  private:
    /** Run the test */
    void DoRun() override;
};

CommandLineNonOptionTestCase::CommandLineNonOptionTestCase()
    : CommandLineTestCaseBase("nonoption")
{
}

void
CommandLineNonOptionTestCase::DoRun()
{
    CommandLine cmd;
    bool myBool = false;
    int32_t myInt = 1;
    std::string myStr = "MyStr";

    cmd.AddNonOption("my-bool", "help", myBool);
    cmd.AddNonOption("my-int", "help", myInt);
    cmd.AddNonOption("my-str", "help", myStr);

    Parse(cmd, 2, "true", "5");

    NS_TEST_ASSERT_MSG_EQ(myBool, true, "CommandLine did not correctly set a boolean non-option");
    NS_TEST_ASSERT_MSG_EQ(myInt,
                          5,
                          "CommandLine did not correctly set an integer non-option value to 5");
    NS_TEST_ASSERT_MSG_EQ(myStr, "MyStr", "CommandLine did not leave a non-option unmodified.");

    Parse(cmd, 5, "false", "6", "newValue", "extraVal1", "extraVal2");

    NS_TEST_ASSERT_MSG_EQ(myBool, false, "CommandLine did not correctly set a boolean non-option");
    NS_TEST_ASSERT_MSG_EQ(myInt,
                          6,
                          "CommandLine did not correctly set an integer non-option value to 5");
    NS_TEST_ASSERT_MSG_EQ(myStr, "newValue", "CommandLine did not leave a non-option unmodified.");

    NS_TEST_ASSERT_MSG_EQ(cmd.GetNExtraNonOptions(),
                          2,
                          "CommandLine did not parse the correct number of extra non-options.");
    NS_TEST_ASSERT_MSG_EQ(cmd.GetExtraNonOption(0),
                          "extraVal1",
                          "CommandLine did not correctly get one extra non-option");
    NS_TEST_ASSERT_MSG_EQ(cmd.GetExtraNonOption(1),
                          "extraVal2",
                          "CommandLine did not correctly get two extra non-option");
}

/**
 * \ingroup commandline-tests
 * Test \c char* buffer argument
 */
class CommandLineCharStarTestCase : public CommandLineTestCaseBase
{
  public:
    /** Constructor */
    CommandLineCharStarTestCase();

    /** Destructor */
    ~CommandLineCharStarTestCase() override
    {
    }

  private:
    /** Run the test */
    void DoRun() override;
};

CommandLineCharStarTestCase::CommandLineCharStarTestCase()
    : CommandLineTestCaseBase("charstar")
{
}

void
CommandLineCharStarTestCase::DoRun()
{
    // char* buffer option
    constexpr int CHARBUF_SIZE = 10;
    char charbuf[CHARBUF_SIZE] = "charstar";

    CommandLine cmd;
    cmd.AddValue("charbuf", "a char* buffer", charbuf, CHARBUF_SIZE);
    Parse(cmd, 1, "--charbuf=deadbeef");

    std::string value{charbuf};

    NS_TEST_ASSERT_MSG_EQ(value, "deadbeef", "CommandLine did not correctly set a char* buffer");
}

/**
 * \ingroup commandline-tests
 * The Test Suite that glues all of the Test Cases together.
 */
class CommandLineTestSuite : public TestSuite
{
  public:
    /** Constructor */
    CommandLineTestSuite();
};

CommandLineTestSuite::CommandLineTestSuite()
    : TestSuite("command-line")
{
    AddTestCase(new CommandLineBooleanTestCase);
    AddTestCase(new CommandLineUint8tTestCase);
    AddTestCase(new CommandLineIntTestCase);
    AddTestCase(new CommandLineUnsignedIntTestCase);
    AddTestCase(new CommandLineStringTestCase);
    AddTestCase(new CommandLineOrderTestCase);
    AddTestCase(new CommandLineInvalidTestCase);
    AddTestCase(new CommandLineNonOptionTestCase);
    AddTestCase(new CommandLineCharStarTestCase);
}

/**
 * \ingroup commandline-tests
 * CommandLineTestSuite instance variable.
 */
static CommandLineTestSuite g_commandLineTestSuite;

} // namespace tests

} // namespace ns3
