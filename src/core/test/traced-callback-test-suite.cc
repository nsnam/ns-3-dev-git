/*
 * Copyright (c) 2009 University of Washington
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
 */

#include "ns3/test.h"
#include "ns3/traced-callback.h"

using namespace ns3;

/**
 * \file
 * \ingroup tracedcallback-tests
 * TracedCallback test suite
 */

/**
 * \ingroup core-tests
 * \defgroup tracedcallback-tests TracedCallback class tests
 */

/**
 * \ingroup tracedcallback-tests
 *
 * TracedCallback Test case, check basic TracedCallback operation.
 */
class BasicTracedCallbackTestCase : public TestCase
{
  public:
    BasicTracedCallbackTestCase();

    ~BasicTracedCallbackTestCase() override
    {
    }

  private:
    void DoRun() override;

    /**
     * First callback.
     * \param a First parameter.
     * \param b Second parameter.
     */
    void CbOne(uint8_t a, double b);

    CallbackBase m_cbTwo; //!< second callback
    bool m_one;           //!< Variable set by the first callback.
    bool m_two;           //!< Variable set by the second callback.
};

BasicTracedCallbackTestCase::BasicTracedCallbackTestCase()
    : TestCase("Check basic TracedCallback operation")
{
}

void
BasicTracedCallbackTestCase::CbOne(uint8_t /* a */, double /* b */)
{
    m_one = true;
}

void
BasicTracedCallbackTestCase::DoRun()
{
    //
    // Disconnecting callbacks from a traced callback is based on the ability to
    // compare callbacks. Given that lambdas cannot be compared, a callback
    // pointing to a lambda needs to be stored to allow it to be disconnected
    // later. Here we check that it is enough to store the callback as a CallbackBase.
    //
    m_cbTwo = Callback<void, uint8_t, double>([this](uint8_t, double) { m_two = true; });

    //
    // Create a traced callback and connect it up to our target methods.  All that
    // these methods do is to set corresponding member variables m_one and m_two.
    //
    TracedCallback<uint8_t, double> trace;

    //
    // Connect both callbacks to their respective test methods.  If we hit the
    // trace, both callbacks should be called and the two variables  should be set
    // to true.
    //
    trace.ConnectWithoutContext(MakeCallback(&BasicTracedCallbackTestCase::CbOne, this));
    trace.ConnectWithoutContext(m_cbTwo);
    m_one = false;
    m_two = false;
    trace(1, 2);
    NS_TEST_ASSERT_MSG_EQ(m_one, true, "Callback CbOne not called");
    NS_TEST_ASSERT_MSG_EQ(m_two, true, "Callback CbTwo not called");

    //
    // If we now disconnect callback one then only callback two should be called.
    //
    trace.DisconnectWithoutContext(MakeCallback(&BasicTracedCallbackTestCase::CbOne, this));
    m_one = false;
    m_two = false;
    trace(1, 2);
    NS_TEST_ASSERT_MSG_EQ(m_one, false, "Callback CbOne unexpectedly called");
    NS_TEST_ASSERT_MSG_EQ(m_two, true, "Callback CbTwo not called");

    //
    // If we now disconnect callback two then neither callback should be called.
    //
    trace.DisconnectWithoutContext(m_cbTwo);
    m_one = false;
    m_two = false;
    trace(1, 2);
    NS_TEST_ASSERT_MSG_EQ(m_one, false, "Callback CbOne unexpectedly called");
    NS_TEST_ASSERT_MSG_EQ(m_two, false, "Callback CbTwo unexpectedly called");

    //
    // If we connect them back up, then both callbacks should be called.
    //
    trace.ConnectWithoutContext(MakeCallback(&BasicTracedCallbackTestCase::CbOne, this));
    trace.ConnectWithoutContext(m_cbTwo);
    m_one = false;
    m_two = false;
    trace(1, 2);
    NS_TEST_ASSERT_MSG_EQ(m_one, true, "Callback CbOne not called");
    NS_TEST_ASSERT_MSG_EQ(m_two, true, "Callback CbTwo not called");
}

/**
 * \ingroup tracedcallback-tests
 *
 * \brief The traced callback Test Suite.
 */
class TracedCallbackTestSuite : public TestSuite
{
  public:
    TracedCallbackTestSuite();
};

TracedCallbackTestSuite::TracedCallbackTestSuite()
    : TestSuite("traced-callback", UNIT)
{
    AddTestCase(new BasicTracedCallbackTestCase, TestCase::QUICK);
}

static TracedCallbackTestSuite
    g_tracedCallbackTestSuite; //!< Static variable for test initialization
