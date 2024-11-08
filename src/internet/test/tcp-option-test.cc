/*
 * Copyright (c) 2014 Natale Patriciello <natale.patriciello@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#include "ns3/core-module.h"
#include "ns3/tcp-option-ts.h"
#include "ns3/tcp-option-winscale.h"
#include "ns3/tcp-option.h"
#include "ns3/test.h"

#include <string.h>

using namespace ns3;

/**
 * @ingroup internet-test
 *
 * @brief TCP Window Scaling option Test
 */
class TcpOptionWSTestCase : public TestCase
{
  public:
    /**
     * @brief Constructor.
     * @param name Test description.
     * @param scale Window scaling.
     */
    TcpOptionWSTestCase(std::string name, uint8_t scale);

    /**
     * @brief Serialization test.
     */
    void TestSerialize();
    /**
     * @brief Deserialization test.
     */
    void TestDeserialize();

  private:
    void DoRun() override;
    void DoTeardown() override;

    uint8_t m_scale; //!< Window scaling.
    Buffer m_buffer; //!< Buffer.
};

TcpOptionWSTestCase::TcpOptionWSTestCase(std::string name, uint8_t scale)
    : TestCase(name)
{
    m_scale = scale;
}

void
TcpOptionWSTestCase::DoRun()
{
    TestSerialize();
    TestDeserialize();
}

void
TcpOptionWSTestCase::TestSerialize()
{
    TcpOptionWinScale opt;

    opt.SetScale(m_scale);
    NS_TEST_EXPECT_MSG_EQ(m_scale, opt.GetScale(), "Scale isn't saved correctly");

    m_buffer.AddAtStart(opt.GetSerializedSize());

    opt.Serialize(m_buffer.Begin());
}

void
TcpOptionWSTestCase::TestDeserialize()
{
    TcpOptionWinScale opt;

    Buffer::Iterator start = m_buffer.Begin();
    uint8_t kind = start.PeekU8();

    NS_TEST_EXPECT_MSG_EQ(kind, TcpOption::WINSCALE, "Different kind found");

    opt.Deserialize(start);

    NS_TEST_EXPECT_MSG_EQ(m_scale, opt.GetScale(), "Different scale found");
}

void
TcpOptionWSTestCase::DoTeardown()
{
}

/**
 * @ingroup internet-test
 *
 * @brief TCP TimeStamp option Test
 */
class TcpOptionTSTestCase : public TestCase
{
  public:
    /**
     * @brief Constructor.
     * @param name Test description.
     */
    TcpOptionTSTestCase(std::string name);

    /**
     * @brief Serialization test.
     */
    void TestSerialize();
    /**
     * @brief Deserialization test.
     */
    void TestDeserialize();

  private:
    void DoRun() override;
    void DoTeardown() override;

    uint32_t m_timestamp; //!< TimeStamp.
    uint32_t m_echo;      //!< Echoed TimeStamp.
    Buffer m_buffer;      //!< Buffer.
};

TcpOptionTSTestCase::TcpOptionTSTestCase(std::string name)
    : TestCase(name)
{
    m_timestamp = 0;
    m_echo = 0;
}

void
TcpOptionTSTestCase::DoRun()
{
    Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable>();

    for (uint32_t i = 0; i < 1000; ++i)
    {
        m_timestamp = x->GetInteger();
        m_echo = x->GetInteger();
        TestSerialize();
        TestDeserialize();
    }
}

void
TcpOptionTSTestCase::TestSerialize()
{
    TcpOptionTS opt;

    opt.SetTimestamp(m_timestamp);
    opt.SetEcho(m_echo);

    NS_TEST_EXPECT_MSG_EQ(m_timestamp, opt.GetTimestamp(), "TS isn't saved correctly");
    NS_TEST_EXPECT_MSG_EQ(m_echo, opt.GetEcho(), "echo isn't saved correctly");

    m_buffer.AddAtStart(opt.GetSerializedSize());

    opt.Serialize(m_buffer.Begin());
}

void
TcpOptionTSTestCase::TestDeserialize()
{
    TcpOptionTS opt;

    Buffer::Iterator start = m_buffer.Begin();
    uint8_t kind = start.PeekU8();

    NS_TEST_EXPECT_MSG_EQ(kind, TcpOption::TS, "Different kind found");

    opt.Deserialize(start);

    NS_TEST_EXPECT_MSG_EQ(m_timestamp, opt.GetTimestamp(), "Different TS found");
    NS_TEST_EXPECT_MSG_EQ(m_echo, opt.GetEcho(), "Different echo found");
}

void
TcpOptionTSTestCase::DoTeardown()
{
}

/**
 * @ingroup internet-test
 *
 * @brief TCP options TestSuite
 */
class TcpOptionTestSuite : public TestSuite
{
  public:
    TcpOptionTestSuite()
        : TestSuite("tcp-option", Type::UNIT)
    {
        for (uint8_t i = 0; i < 15; ++i)
        {
            AddTestCase(new TcpOptionWSTestCase("Testing window scale value", i),
                        TestCase::Duration::QUICK);
        }
        AddTestCase(new TcpOptionTSTestCase("Testing serialization of random values for timestamp"),
                    TestCase::Duration::QUICK);
    }
};

static TcpOptionTestSuite g_TcpOptionTestSuite; //!< Static variable for test initialization
