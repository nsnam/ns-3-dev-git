/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/test.h"
#include "ns3/wifi-state.h"

#include <optional>

using namespace ns3;

/**
 * @ingroup wifi-test
 * @ingroup tests
 * Check state interval history and immediate transition cancellation.
 */
class WifiStateTest : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param ignoreEmptyInterval whether to ignore empty intervals
     */
    WifiStateTest(bool ignoreEmptyInterval);

  private:
    /**
     * States used to exercise interval history.
     */
    enum State : uint8_t
    {
        A,
        B,
        C
    };

    /**
     * Check the interval history for a state.
     *
     * @param state the state to query
     * @param start expected most recent start
     * @param end expected most recent end, including an ongoing interval
     * @param completed expected most recent completed end
     */
    void Check(State state,
               std::optional<Time> start,
               std::optional<Time> end,
               std::optional<Time> completed);

    void DoRun() override;

    bool m_ignoreEmptyInterval;                ///< Whether to ignore empty intervals
    std::optional<WifiState<State>> m_testVar; ///< State tracker under test
};

WifiStateTest::WifiStateTest(bool ignoreEmptyInterval)
    : TestCase(std::string("Wi-Fi state history with empty intervals ") +
               (ignoreEmptyInterval ? "ignored" : "retained")),
      m_ignoreEmptyInterval(ignoreEmptyInterval)
{
}

void
WifiStateTest::Check(State state,
                     std::optional<Time> expStart,
                     std::optional<Time> expEnd,
                     std::optional<Time> expCompletedEnd)
{
    const auto lastStart = m_testVar->GetLastStart(state);
    NS_TEST_EXPECT_MSG_EQ(lastStart.has_value(),
                          expStart.has_value(),
                          "Unexpected start availability");
    if (lastStart && expStart)
    {
        NS_TEST_EXPECT_MSG_EQ(*lastStart, *expStart, "Unexpected start");
    }

    const auto lastEnd = m_testVar->GetLastEnd(state);
    NS_TEST_EXPECT_MSG_EQ(lastEnd.has_value(), expEnd.has_value(), "Unexpected end availability");
    if (lastEnd && expEnd)
    {
        NS_TEST_EXPECT_MSG_EQ(*lastEnd, *expEnd, "Unexpected end");
    }

    const auto lastCompletedEnd = m_testVar->GetLastCompletedEnd(state);
    NS_TEST_EXPECT_MSG_EQ(lastCompletedEnd.has_value(),
                          expCompletedEnd.has_value(),
                          "Unexpected completed end availability");
    if (lastCompletedEnd && expCompletedEnd)
    {
        NS_TEST_EXPECT_MSG_EQ(*lastCompletedEnd, *expCompletedEnd, "Unexpected completed end");
    }
}

void
WifiStateTest::DoRun()
{
    // In the snapshots below, ignore is m_ignoreEmptyInterval, X:[begin,end] is a completed
    // interval, X@start is a current or pending state, and '-' means no pending state.
    // All times are in seconds. For loop snapshots, i identifies the iteration.
    Simulator::Schedule(Seconds(1), [this]() {
        m_testVar.emplace(A, m_ignoreEmptyInterval);
        Check(A, Seconds(1), Seconds(1), std::nullopt);
        Check(B, std::nullopt, std::nullopt, std::nullopt);
        m_testVar->SetState(B);
        // +--------+-----------+-----------+-------------+
        // | ignore | m_current | m_pending | m_completed |
        // +--------+-----------+-----------+-------------+
        // | true   | B@1       | -         | {}          |
        // | false  | B@1       | -         | {A:[1,1]}   |
        // +--------+-----------+-----------+-------------+
        Check(B, Seconds(1), Seconds(1), std::nullopt);
        m_testVar->SetState(A);
        // +--------+-----------+-----------+--------------------+
        // | ignore | m_current | m_pending | m_completed        |
        // +--------+-----------+-----------+--------------------+
        // | true   | A@1       | -         | {}                 |
        // | false  | A@1       | -         | {A:[1,1], B:[1,1]} |
        // +--------+-----------+-----------+--------------------+
        Check(A,
              Seconds(1),
              Seconds(1),
              m_ignoreEmptyInterval ? std::nullopt : std::make_optional(Seconds(1)));
        if (m_ignoreEmptyInterval)
        {
            Check(B, std::nullopt, std::nullopt, std::nullopt);
        }
        else
        {
            Check(B, Seconds(1), Seconds(1), Seconds(1));
        }
    });

    Simulator::Schedule(Seconds(11), [&]() {
        m_testVar->SetState(B);
        // +--------+-----------+-----------+---------------------+
        // | ignore | m_current | m_pending | m_completed         |
        // +--------+-----------+-----------+---------------------+
        // | true   | B@11      | A@1       | {}                  |
        // | false  | B@11      | -         | {A:[1,11], B:[1,1]} |
        // +--------+-----------+-----------+---------------------+
        Check(A, Seconds(1), Seconds(11), Seconds(11));
        Check(B,
              Seconds(11),
              Seconds(11),
              m_ignoreEmptyInterval ? std::nullopt : std::make_optional(Seconds(1)));
        m_testVar->SetState(B);
        // +--------+-----------+-----------+---------------------+
        // | ignore | m_current | m_pending | m_completed         |
        // +--------+-----------+-----------+---------------------+
        // | true   | B@11      | A@1       | {}                  |
        // | false  | B@11      | -         | {A:[1,11], B:[1,1]} |
        // +--------+-----------+-----------+---------------------+
        m_testVar->SetState(A);
        // +--------+-----------+-----------+-----------------------+
        // | ignore | m_current | m_pending | m_completed           |
        // +--------+-----------+-----------+-----------------------+
        // | true   | A@1       | -         | {}                    |
        // | false  | A@11      | -         | {A:[1,11], B:[11,11]} |
        // +--------+-----------+-----------+-----------------------+
        NS_TEST_ASSERT_MSG_EQ(m_testVar->GetCurrent(), A, "Expected the original state");
        if (m_ignoreEmptyInterval)
        {
            Check(A, Seconds(1), Seconds(11), std::nullopt);
            Check(B, std::nullopt, std::nullopt, std::nullopt);
        }
        else
        {
            Check(A, Seconds(11), Seconds(11), Seconds(11));
            Check(B, Seconds(11), Seconds(11), Seconds(11));
        }
        m_testVar->SetState(B);
        // +--------+-----------+-----------+------------------------+
        // | ignore | m_current | m_pending | m_completed            |
        // +--------+-----------+-----------+------------------------+
        // | true   | B@11      | A@1       | {}                     |
        // | false  | B@11      | -         | {A:[11,11], B:[11,11]} |
        // +--------+-----------+-----------+------------------------+
    });
    Simulator::Schedule(Seconds(16), [&]() {
        m_testVar->SetState(B);
        // +--------+-----------+-----------+------------------------+
        // | ignore | m_current | m_pending | m_completed            |
        // +--------+-----------+-----------+------------------------+
        // | true   | B@11      | A@1       | {}                     |
        // | false  | B@11      | -         | {A:[11,11], B:[11,11]} |
        // +--------+-----------+-----------+------------------------+
        Check(A, m_ignoreEmptyInterval ? Seconds(1) : Seconds(11), Seconds(11), Seconds(11));
        Check(B,
              Seconds(11),
              Seconds(16),
              m_ignoreEmptyInterval ? std::nullopt : std::make_optional(Seconds(11)));
    });
    Simulator::Schedule(Seconds(21), [&]() {
        m_testVar->SetState(A);
        // +--------+-----------+-----------+------------------------+
        // | ignore | m_current | m_pending | m_completed            |
        // +--------+-----------+-----------+------------------------+
        // | true   | A@21      | B@11      | {A:[1,11]}             |
        // | false  | A@21      | -         | {A:[11,11], B:[11,21]} |
        // +--------+-----------+-----------+------------------------+
        Check(A, Seconds(21), Seconds(21), Seconds(11));
        Check(B, Seconds(11), Seconds(21), Seconds(21));
    });
    Simulator::Schedule(Seconds(41), [&]() {
        for (int i = 0; i < 2; ++i)
        {
            m_testVar->SetState(B);
            // +------------+-----------+-----------+------------------------+
            // | ignore / i | m_current | m_pending | m_completed            |
            // +------------+-----------+-----------+------------------------+
            // | true / 0   | B@41      | A@21      | {A:[1,11], B:[11,21]}  |
            // | true / 1   | B@41      | A@21      | {A:[1,11], B:[11,21]}  |
            // | false / 0  | B@41      | -         | {A:[21,41], B:[11,21]} |
            // | false / 1  | B@41      | -         | {A:[41,41], B:[41,41]} |
            // +------------+-----------+-----------+------------------------+
            NS_TEST_ASSERT_MSG_EQ(m_testVar->GetCurrent(), B, "Expected state B");
            Check(A,
                  m_ignoreEmptyInterval || i == 0 ? Seconds(21) : Seconds(41),
                  Seconds(41),
                  Seconds(41));
            Check(B,
                  Seconds(41),
                  Seconds(41),
                  m_ignoreEmptyInterval || i == 0 ? Seconds(21) : Seconds(41));
            m_testVar->SetState(A);
            // +------------+-----------+-----------+------------------------+
            // | ignore / i | m_current | m_pending | m_completed            |
            // +------------+-----------+-----------+------------------------+
            // | true / 0   | A@21      | -         | {A:[1,11], B:[11,21]}  |
            // | true / 1   | A@21      | -         | {A:[1,11], B:[11,21]}  |
            // | false / 0  | A@41      | -         | {A:[21,41], B:[41,41]} |
            // | false / 1  | A@41      | -         | {A:[41,41], B:[41,41]} |
            // +------------+-----------+-----------+------------------------+
            NS_TEST_ASSERT_MSG_EQ(m_testVar->GetCurrent(), A, "Expected state A");
            Check(A,
                  m_ignoreEmptyInterval ? Seconds(21) : Seconds(41),
                  Seconds(41),
                  m_ignoreEmptyInterval ? Seconds(11) : Seconds(41));
            Check(B,
                  m_ignoreEmptyInterval ? Seconds(11) : Seconds(41),
                  m_ignoreEmptyInterval ? Seconds(21) : Seconds(41),
                  m_ignoreEmptyInterval ? Seconds(21) : Seconds(41));
        }
    });
    Simulator::Schedule(Seconds(51), [&]() {
        m_testVar->SetState(C);
        // +--------+-----------+-----------+------------------------+
        // | ignore | m_current | m_pending | m_completed            |
        // +--------+-----------+-----------+------------------------+
        // | true   | C@51      | A@21      | {A:[1,11], B:[11,21]}  |
        // | false  | C@51      | -         | {A:[41,51], B:[41,41]} |
        // +--------+-----------+-----------+------------------------+
        m_testVar->SetState(B);
        // +--------+-----------+-----------+-----------------------------------+
        // | ignore | m_current | m_pending | m_completed                       |
        // +--------+-----------+-----------+-----------------------------------+
        // | true   | B@51      | A@21      | {A:[1,11], B:[11,21]}             |
        // | false  | B@51      | -         | {A:[41,51], B:[41,41], C:[51,51]} |
        // +--------+-----------+-----------+-----------------------------------+
        m_testVar->SetState(A);
        // +--------+-----------+-----------+-----------------------------------+
        // | ignore | m_current | m_pending | m_completed                       |
        // +--------+-----------+-----------+-----------------------------------+
        // | true   | A@21      | -         | {A:[1,11], B:[11,21]}             |
        // | false  | A@51      | -         | {A:[41,51], B:[51,51], C:[51,51]} |
        // +--------+-----------+-----------+-----------------------------------+
        NS_TEST_ASSERT_MSG_EQ(m_testVar->GetCurrent(), A, "Expected the original state");
        if (m_ignoreEmptyInterval)
        {
            Check(A, Seconds(21), Seconds(51), Seconds(11));
            Check(B, Seconds(11), Seconds(21), Seconds(21));
            Check(C, std::nullopt, std::nullopt, std::nullopt);
        }
        else
        {
            Check(A, Seconds(51), Seconds(51), Seconds(51));
            Check(B, Seconds(51), Seconds(51), Seconds(51));
            Check(C, Seconds(51), Seconds(51), Seconds(51));
        }
    });
    Simulator::Schedule(Seconds(52), [&]() {
        m_testVar->SetState(B);
        // +--------+-----------+-----------+-----------------------------------+
        // | ignore | m_current | m_pending | m_completed                       |
        // +--------+-----------+-----------+-----------------------------------+
        // | true   | B@52      | A@21      | {A:[1,11], B:[11,21]}             |
        // | false  | B@52      | -         | {A:[51,52], B:[51,51], C:[51,51]} |
        // +--------+-----------+-----------+-----------------------------------+
    });
    Simulator::Schedule(Seconds(53), [&]() {
        m_testVar->SetState(C);
        // +--------+-----------+-----------+-----------------------------------+
        // | ignore | m_current | m_pending | m_completed                       |
        // +--------+-----------+-----------+-----------------------------------+
        // | true   | C@53      | B@52      | {A:[21,52], B:[11,21]}            |
        // | false  | C@53      | -         | {A:[51,52], B:[52,53], C:[51,51]} |
        // +--------+-----------+-----------+-----------------------------------+
    });
    Simulator::Schedule(Seconds(54), [&]() {
        m_testVar->SetState(B);
        // +--------+-----------+-----------+-----------------------------------+
        // | ignore | m_current | m_pending | m_completed                       |
        // +--------+-----------+-----------+-----------------------------------+
        // | true   | B@54      | C@53      | {A:[21,52], B:[52,53]}            |
        // | false  | B@54      | -         | {A:[51,52], B:[52,53], C:[53,54]} |
        // +--------+-----------+-----------+-----------------------------------+
    });
    Simulator::Schedule(Seconds(55), [&]() {
        m_testVar->SetState(C);
        // +--------+-----------+-----------+-----------------------------------+
        // | ignore | m_current | m_pending | m_completed                       |
        // +--------+-----------+-----------+-----------------------------------+
        // | true   | C@55      | B@54      | {A:[21,52], B:[52,53], C:[53,54]} |
        // | false  | C@55      | -         | {A:[51,52], B:[54,55], C:[53,54]} |
        // +--------+-----------+-----------+-----------------------------------+
    });
    Simulator::Schedule(Seconds(56), [&]() {
        m_testVar->SetState(B);
        // +--------+-----------+-----------+-----------------------------------+
        // | ignore | m_current | m_pending | m_completed                       |
        // +--------+-----------+-----------+-----------------------------------+
        // | true   | B@56      | C@55      | {A:[21,52], B:[54,55], C:[53,54]} |
        // | false  | B@56      | -         | {A:[51,52], B:[54,55], C:[55,56]} |
        // +--------+-----------+-----------+-----------------------------------+
        Check(A, m_ignoreEmptyInterval ? Seconds(21) : Seconds(51), Seconds(52), Seconds(52));
        Check(B, Seconds(56), Seconds(56), Seconds(55));
        Check(C, Seconds(55), Seconds(56), Seconds(56));
    });
    Simulator::Schedule(Seconds(57), [&]() {
        m_testVar->SetState(A);
        // +--------+-----------+-----------+-----------------------------------+
        // | ignore | m_current | m_pending | m_completed                       |
        // +--------+-----------+-----------+-----------------------------------+
        // | true   | A@57      | B@56      | {A:[21,52], B:[54,55], C:[55,56]} |
        // | false  | A@57      | -         | {A:[51,52], B:[56,57], C:[55,56]} |
        // +--------+-----------+-----------+-----------------------------------+
        Check(A, Seconds(57), Seconds(57), Seconds(52));
    });

    Simulator::Run();
    m_testVar.reset();
    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 * Test suite for Wi-Fi state history.
 */
class WifiStateTestSuite : public TestSuite
{
  public:
    WifiStateTestSuite()
        : TestSuite("wifi-state", Type::UNIT)
    {
        AddTestCase(new WifiStateTest(true), TestCase::Duration::QUICK);
        AddTestCase(new WifiStateTest(false), TestCase::Duration::QUICK);
    }
};

static WifiStateTestSuite g_wifiStateTestSuite; ///< Wi-Fi state test suite
