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

#include "ns3/callback.h"
#include "ns3/test.h"

#include <stdint.h>

using namespace ns3;

/**
 * \file
 * \ingroup callback-tests
 * Callback test suite
 */

/**
 * \ingroup core-tests
 * \defgroup callback-tests Callback tests
 */

/**
 * \ingroup callback-tests
 *
 * Test the basic Callback mechanism.
 */
class BasicCallbackTestCase : public TestCase
{
  public:
    BasicCallbackTestCase();

    ~BasicCallbackTestCase() override
    {
    }

    /**
     * Callback 1 target function.
     */
    void Target1()
    {
        m_test1 = true;
    }

    /**
     * Callback 2 target function.
     * \return two.
     */
    int Target2()
    {
        m_test2 = true;
        return 2;
    }

    /**
     * Callback 3 target function.
     * \param a A parameter (unused).
     */
    void Target3(double a [[maybe_unused]])
    {
        m_test3 = true;
    }

    /**
     * Callback 4 target function.
     * \param a A parameter (unused).
     * \param b Another parameter (unused).
     * \return four.
     */
    int Target4(double a [[maybe_unused]], int b [[maybe_unused]])
    {
        m_test4 = true;
        return 4;
    }

  private:
    void DoRun() override;
    void DoSetup() override;

    bool m_test1; //!< true if Target1 has been called, false otherwise.
    bool m_test2; //!< true if Target2 has been called, false otherwise.
    bool m_test3; //!< true if Target3 has been called, false otherwise.
    bool m_test4; //!< true if Target4 has been called, false otherwise.
};

/**
 * Variable to verify that a callback has been called.
 * @{
 */
static bool gBasicCallbackTest5;
static bool gBasicCallbackTest6;
static bool gBasicCallbackTest7;
static bool gBasicCallbackTest8;

/** @} */

/**
 * Callback 5 target function.
 */
void
BasicCallbackTarget5()
{
    gBasicCallbackTest5 = true;
}

/**
 * Callback 6 target function.
 */
void
BasicCallbackTarget6(int)
{
    gBasicCallbackTest6 = true;
}

/**
 * Callback 6 target function.
 * \param a The value passed by the callback.
 * \return the value of the calling function.
 */
int
BasicCallbackTarget7(int a)
{
    gBasicCallbackTest7 = true;
    return a;
}

BasicCallbackTestCase::BasicCallbackTestCase()
    : TestCase("Check basic Callback mechanism")
{
}

void
BasicCallbackTestCase::DoSetup()
{
    m_test1 = false;
    m_test2 = false;
    m_test3 = false;
    m_test4 = false;
    gBasicCallbackTest5 = false;
    gBasicCallbackTest6 = false;
    gBasicCallbackTest7 = false;
    gBasicCallbackTest8 = false;
}

void
BasicCallbackTestCase::DoRun()
{
    //
    // Make sure we can declare and compile a Callback pointing to a member
    // function returning void and execute it.
    //
    Callback<void> target1(&BasicCallbackTestCase::Target1, this);
    target1();
    NS_TEST_ASSERT_MSG_EQ(m_test1, true, "Callback did not fire");

    //
    // Make sure we can declare and compile a Callback pointing to a member
    // function that returns an int and execute it.
    //
    Callback<int> target2;
    target2 = Callback<int>(&BasicCallbackTestCase::Target2, this);
    target2();
    NS_TEST_ASSERT_MSG_EQ(m_test2, true, "Callback did not fire");

    //
    // Make sure we can declare and compile a Callback pointing to a member
    // function that returns void, takes a double parameter, and execute it.
    //
    Callback<void, double> target3 = Callback<void, double>(&BasicCallbackTestCase::Target3, this);
    target3(0.0);
    NS_TEST_ASSERT_MSG_EQ(m_test3, true, "Callback did not fire");

    //
    // Make sure we can declare and compile a Callback pointing to a member
    // function that returns void, takes two parameters, and execute it.
    //
    Callback<int, double, int> target4 =
        Callback<int, double, int>(&BasicCallbackTestCase::Target4, this);
    target4(0.0, 1);
    NS_TEST_ASSERT_MSG_EQ(m_test4, true, "Callback did not fire");

    //
    // Make sure we can declare and compile a Callback pointing to a non-member
    // function that returns void, and execute it.  This is a lower level call
    // than MakeCallback so we have got to include at least two arguments to make
    // sure that the constructor is properly disambiguated.  If the arguments are
    // not needed, we just pass in dummy values.
    //
    Callback<void> target5 = Callback<void>(&BasicCallbackTarget5);
    target5();
    NS_TEST_ASSERT_MSG_EQ(gBasicCallbackTest5, true, "Callback did not fire");

    //
    // Make sure we can declare and compile a Callback pointing to a non-member
    // function that returns void, takes one integer argument and execute it.
    //
    Callback<void, int> target6 = Callback<void, int>(&BasicCallbackTarget6);
    target6(1);
    NS_TEST_ASSERT_MSG_EQ(gBasicCallbackTest6, true, "Callback did not fire");

    //
    // Make sure we can declare and compile a Callback pointing to a non-member
    // function that returns int, takes one integer argument and execute it.
    //
    Callback<int, int> target7 = Callback<int, int>(&BasicCallbackTarget7);
    target7(1);
    NS_TEST_ASSERT_MSG_EQ(gBasicCallbackTest7, true, "Callback did not fire");

    //
    // Make sure we can create a callback pointing to a lambda.
    //
    Callback<double, int> target8([](int p) {
        gBasicCallbackTest8 = true;
        return p / 2.;
    });
    target8(5);
    NS_TEST_ASSERT_MSG_EQ(gBasicCallbackTest8, true, "Callback did not fire");
}

/**
 * \ingroup callback-tests
 *
 * Test the MakeCallback mechanism.
 */
class MakeCallbackTestCase : public TestCase
{
  public:
    MakeCallbackTestCase();

    ~MakeCallbackTestCase() override
    {
    }

    /**
     * Callback 1 target function.
     */
    void Target1()
    {
        m_test1 = true;
    }

    /**
     * Callback 2 target function.
     * \return two.
     */
    int Target2()
    {
        m_test2 = true;
        return 2;
    }

    /**
     * Callback 3 target function.
     * \param a A parameter (unused).
     */
    void Target3(double a [[maybe_unused]])
    {
        m_test3 = true;
    }

    /**
     * Callback 4 target function.
     * \param a A parameter (unused).
     * \param b Another parameter (unused).
     * \return four.
     */
    int Target4(double a [[maybe_unused]], int b [[maybe_unused]])
    {
        m_test4 = true;
        return 4;
    }

  private:
    void DoRun() override;
    void DoSetup() override;

    bool m_test1; //!< true if Target1 has been called, false otherwise.
    bool m_test2; //!< true if Target2 has been called, false otherwise.
    bool m_test3; //!< true if Target3 has been called, false otherwise.
    bool m_test4; //!< true if Target4 has been called, false otherwise.
};

/**
 * Variable to verify that a callback has been called.
 * @{
 */
static bool gMakeCallbackTest5;
static bool gMakeCallbackTest6;
static bool gMakeCallbackTest7;

/** @} */

/**
 * MakeCallback 5 target function.
 */
void
MakeCallbackTarget5()
{
    gMakeCallbackTest5 = true;
}

/**
 * MakeCallback 6 target function.
 */
void
MakeCallbackTarget6(int)
{
    gMakeCallbackTest6 = true;
}

/**
 * MakeCallback 7 target function.
 * \param a The value passed by the callback.
 * \return the value of the calling function.
 */
int
MakeCallbackTarget7(int a)
{
    gMakeCallbackTest7 = true;
    return a;
}

MakeCallbackTestCase::MakeCallbackTestCase()
    : TestCase("Check MakeCallback() mechanism")
{
}

void
MakeCallbackTestCase::DoSetup()
{
    m_test1 = false;
    m_test2 = false;
    m_test3 = false;
    m_test4 = false;
    gMakeCallbackTest5 = false;
    gMakeCallbackTest6 = false;
    gMakeCallbackTest7 = false;
}

void
MakeCallbackTestCase::DoRun()
{
    //
    // Make sure we can declare and make a Callback pointing to a member
    // function returning void and execute it.
    //
    Callback<void> target1 = MakeCallback(&MakeCallbackTestCase::Target1, this);
    target1();
    NS_TEST_ASSERT_MSG_EQ(m_test1, true, "Callback did not fire");

    //
    // Make sure we can declare and make a Callback pointing to a member
    // function that returns an int and execute it.
    //
    Callback<int> target2 = MakeCallback(&MakeCallbackTestCase::Target2, this);
    target2();
    NS_TEST_ASSERT_MSG_EQ(m_test2, true, "Callback did not fire");

    //
    // Make sure we can declare and make a Callback pointing to a member
    // function that returns void, takes a double parameter, and execute it.
    //
    Callback<void, double> target3 = MakeCallback(&MakeCallbackTestCase::Target3, this);
    target3(0.0);
    NS_TEST_ASSERT_MSG_EQ(m_test3, true, "Callback did not fire");

    //
    // Make sure we can declare and make a Callback pointing to a member
    // function that returns void, takes two parameters, and execute it.
    //
    Callback<int, double, int> target4 = MakeCallback(&MakeCallbackTestCase::Target4, this);
    target4(0.0, 1);
    NS_TEST_ASSERT_MSG_EQ(m_test4, true, "Callback did not fire");

    //
    // Make sure we can declare and make a Callback pointing to a non-member
    // function that returns void, and execute it.  This uses a higher level call
    // than in the basic tests so we do not need to include any dummy arguments
    // here.
    //
    Callback<void> target5 = MakeCallback(&MakeCallbackTarget5);
    target5();
    NS_TEST_ASSERT_MSG_EQ(gMakeCallbackTest5, true, "Callback did not fire");

    //
    // Make sure we can declare and compile a Callback pointing to a non-member
    // function that returns void, takes one integer argument and execute it.
    // This uses a higher level call than in the basic tests so we do not need to
    // include any dummy arguments here.
    //
    Callback<void, int> target6 = MakeCallback(&MakeCallbackTarget6);
    target6(1);
    NS_TEST_ASSERT_MSG_EQ(gMakeCallbackTest6, true, "Callback did not fire");

    //
    // Make sure we can declare and compile a Callback pointing to a non-member
    // function that returns int, takes one integer argument and execute it.
    // This uses a higher level call than in the basic tests so we do not need to
    // include any dummy arguments here.
    //
    Callback<int, int> target7 = MakeCallback(&MakeCallbackTarget7);
    target7(1);
    NS_TEST_ASSERT_MSG_EQ(gMakeCallbackTest7, true, "Callback did not fire");
}

/**
 * \ingroup callback-tests
 *
 * Test the MakeBoundCallback mechanism.
 */
class MakeBoundCallbackTestCase : public TestCase
{
  public:
    MakeBoundCallbackTestCase();

    ~MakeBoundCallbackTestCase() override
    {
    }

    /**
     * Member function to test the creation of a bound callback pointing to a member function
     *
     * \param a first argument
     * \param b second argument
     * \param c third argument
     * \return the sum of the arguments
     */
    int BoundTarget(int a, int b, int c)
    {
        return a + b + c;
    }

  private:
    void DoRun() override;
    void DoSetup() override;
};

/**
 * Variable to verify that a callback has been called.
 * @{
 */
static int gMakeBoundCallbackTest1;
static bool* gMakeBoundCallbackTest2;
static bool* gMakeBoundCallbackTest3a;
static int gMakeBoundCallbackTest3b;
static int gMakeBoundCallbackTest4a;
static int gMakeBoundCallbackTest4b;
static int gMakeBoundCallbackTest5a;
static int gMakeBoundCallbackTest5b;
static int gMakeBoundCallbackTest5c;
static int gMakeBoundCallbackTest6a;
static int gMakeBoundCallbackTest6b;
static int gMakeBoundCallbackTest6c;
static int gMakeBoundCallbackTest7a;
static int gMakeBoundCallbackTest7b;
static int gMakeBoundCallbackTest7c;
static int gMakeBoundCallbackTest8a;
static int gMakeBoundCallbackTest8b;
static int gMakeBoundCallbackTest8c;
static int gMakeBoundCallbackTest9a;
static int gMakeBoundCallbackTest9b;
static int gMakeBoundCallbackTest9c;
static int gMakeBoundCallbackTest9d;

/** @} */

// Note: doxygen compounds don not work due to params / return variability.

/**
 * MakeBoundCallback 1 target function.
 * \param a The value passed by the callback.
 */
void
MakeBoundCallbackTarget1(int a)
{
    gMakeBoundCallbackTest1 = a;
}

/**
 * MakeBoundCallback 2 target function.
 * \param a The value passed by the callback.
 */
void
MakeBoundCallbackTarget2(bool* a)
{
    gMakeBoundCallbackTest2 = a;
}

/**
 * MakeBoundCallback 3 target function.
 * \param a The value passed by the callback.
 * \param b The value passed by the callback.
 * \return the value 1234.
 */
int
MakeBoundCallbackTarget3(bool* a, int b)
{
    gMakeBoundCallbackTest3a = a;
    gMakeBoundCallbackTest3b = b;
    return 1234;
}

/**
 * MakeBoundCallback 4 target function.
 * \param a The value passed by the callback.
 * \param b The value passed by the callback.
 */
void
MakeBoundCallbackTarget4(int a, int b)
{
    gMakeBoundCallbackTest4a = a;
    gMakeBoundCallbackTest4b = b;
}

/**
 * MakeBoundCallback 5 target function.
 * \param a The value passed by the callback.
 * \param b The value passed by the callback.
 * \return the value 1234.
 */
int
MakeBoundCallbackTarget5(int a, int b)
{
    gMakeBoundCallbackTest5a = a;
    gMakeBoundCallbackTest5b = b;
    return 1234;
}

/**
 * MakeBoundCallback 5 target function.
 * \param a The value passed by the callback.
 * \param b The value passed by the callback.
 * \param c The value passed by the callback.
 * \return the value 1234.
 */
int
MakeBoundCallbackTarget6(int a, int b, int c)
{
    gMakeBoundCallbackTest6a = a;
    gMakeBoundCallbackTest6b = b;
    gMakeBoundCallbackTest6c = c;
    return 1234;
}

/**
 * MakeBoundCallback 7 target function.
 * \param a The value passed by the callback.
 * \param b The value passed by the callback.
 * \param c The value passed by the callback.
 */
void
MakeBoundCallbackTarget7(int a, int b, int c)
{
    gMakeBoundCallbackTest7a = a;
    gMakeBoundCallbackTest7b = b;
    gMakeBoundCallbackTest7c = c;
}

/**
 * MakeBoundCallback 8 target function.
 * \param a The value passed by the callback.
 * \param b The value passed by the callback.
 * \param c The value passed by the callback.
 * \return the value 1234.
 */
int
MakeBoundCallbackTarget8(int a, int b, int c)
{
    gMakeBoundCallbackTest8a = a;
    gMakeBoundCallbackTest8b = b;
    gMakeBoundCallbackTest8c = c;
    return 1234;
}

/**
 * MakeBoundCallback 5 target function.
 * \param a The value passed by the callback.
 * \param b The value passed by the callback.
 * \param c The value passed by the callback.
 * \param d The value passed by the callback.
 * \return the value 1234.
 */
int
MakeBoundCallbackTarget9(int a, int b, int c, int d)
{
    gMakeBoundCallbackTest9a = a;
    gMakeBoundCallbackTest9b = b;
    gMakeBoundCallbackTest9c = c;
    gMakeBoundCallbackTest9d = d;
    return 1234;
}

MakeBoundCallbackTestCase::MakeBoundCallbackTestCase()
    : TestCase("Check MakeBoundCallback() mechanism")
{
}

void
MakeBoundCallbackTestCase::DoSetup()
{
    gMakeBoundCallbackTest1 = 0;
    gMakeBoundCallbackTest2 = nullptr;
    gMakeBoundCallbackTest3a = nullptr;
    gMakeBoundCallbackTest3b = 0;
    gMakeBoundCallbackTest4a = 0;
    gMakeBoundCallbackTest4b = 0;
    gMakeBoundCallbackTest5a = 0;
    gMakeBoundCallbackTest5b = 0;
    gMakeBoundCallbackTest5c = 0;
    gMakeBoundCallbackTest6a = 0;
    gMakeBoundCallbackTest6b = 0;
    gMakeBoundCallbackTest6c = 0;
    gMakeBoundCallbackTest7a = 0;
    gMakeBoundCallbackTest7b = 0;
    gMakeBoundCallbackTest7c = 0;
    gMakeBoundCallbackTest8a = 0;
    gMakeBoundCallbackTest8b = 0;
    gMakeBoundCallbackTest8c = 0;
    gMakeBoundCallbackTest9a = 0;
    gMakeBoundCallbackTest9b = 0;
    gMakeBoundCallbackTest9c = 0;
    gMakeBoundCallbackTest9d = 0;
}

void
MakeBoundCallbackTestCase::DoRun()
{
    //
    // This is slightly tricky to explain.  A bound Callback allows us to package
    // up arguments for use later.  The arguments are bound when the callback is
    // created and the code that fires the Callback does not know they are there.
    //
    // Since the callback is *declared* according to the way it will be used, the
    // arguments are not seen there.  However, the target function of the callback
    // will have the provided arguments present.  The MakeBoundCallback template
    // function is what connects the two together and where you provide the
    // arguments to be bound.
    //
    // Here we declare a Callback that returns a void and takes no parameters.
    // MakeBoundCallback connects this Callback to a target function that returns
    // void and takes an integer argument.  That integer argument is bound to the
    // value 1234.  When the Callback is fired, no integer argument is provided
    // directly.  The argument is provided by bound Callback mechanism.
    //
    Callback<void> target1 = MakeBoundCallback(&MakeBoundCallbackTarget1, 1234);
    target1();
    NS_TEST_ASSERT_MSG_EQ(gMakeBoundCallbackTest1,
                          1234,
                          "Callback did not fire or binding not correct");

    //
    // Make sure we can bind a pointer value (a common use case).
    //
    bool a;
    Callback<void> target2 = MakeBoundCallback(&MakeBoundCallbackTarget2, &a);
    target2();
    NS_TEST_ASSERT_MSG_EQ(gMakeBoundCallbackTest2,
                          &a,
                          "Callback did not fire or binding not correct");

    //
    // Make sure we can mix and match bound and unbound arguments.  This callback
    // returns an integer so we should see that appear.
    //
    Callback<int, int> target3 = MakeBoundCallback(&MakeBoundCallbackTarget3, &a);
    int result = target3(2468);
    NS_TEST_ASSERT_MSG_EQ(result, 1234, "Return value of callback not correct");
    NS_TEST_ASSERT_MSG_EQ(gMakeBoundCallbackTest3a,
                          &a,
                          "Callback did not fire or binding not correct");
    NS_TEST_ASSERT_MSG_EQ(gMakeBoundCallbackTest3b,
                          2468,
                          "Callback did not fire or argument not correct");

    //
    // Test the TwoBound variant
    //
    Callback<void> target4 = MakeBoundCallback(&MakeBoundCallbackTarget4, 3456, 5678);
    target4();
    NS_TEST_ASSERT_MSG_EQ(gMakeBoundCallbackTest4a,
                          3456,
                          "Callback did not fire or binding not correct");
    NS_TEST_ASSERT_MSG_EQ(gMakeBoundCallbackTest4b,
                          5678,
                          "Callback did not fire or binding not correct");

    Callback<int> target5 = MakeBoundCallback(&MakeBoundCallbackTarget5, 3456, 5678);
    int resultTwoA = target5();
    NS_TEST_ASSERT_MSG_EQ(resultTwoA, 1234, "Return value of callback not correct");
    NS_TEST_ASSERT_MSG_EQ(gMakeBoundCallbackTest5a,
                          3456,
                          "Callback did not fire or binding not correct");
    NS_TEST_ASSERT_MSG_EQ(gMakeBoundCallbackTest5b,
                          5678,
                          "Callback did not fire or binding not correct");

    Callback<int, int> target6 = MakeBoundCallback(&MakeBoundCallbackTarget6, 3456, 5678);
    int resultTwoB = target6(6789);
    NS_TEST_ASSERT_MSG_EQ(resultTwoB, 1234, "Return value of callback not correct");
    NS_TEST_ASSERT_MSG_EQ(gMakeBoundCallbackTest6a,
                          3456,
                          "Callback did not fire or binding not correct");
    NS_TEST_ASSERT_MSG_EQ(gMakeBoundCallbackTest6b,
                          5678,
                          "Callback did not fire or binding not correct");
    NS_TEST_ASSERT_MSG_EQ(gMakeBoundCallbackTest6c,
                          6789,
                          "Callback did not fire or argument not correct");

    //
    // Test the ThreeBound variant
    //
    Callback<void> target7 = MakeBoundCallback(&MakeBoundCallbackTarget7, 2345, 3456, 4567);
    target7();
    NS_TEST_ASSERT_MSG_EQ(gMakeBoundCallbackTest7a,
                          2345,
                          "Callback did not fire or binding not correct");
    NS_TEST_ASSERT_MSG_EQ(gMakeBoundCallbackTest7b,
                          3456,
                          "Callback did not fire or binding not correct");
    NS_TEST_ASSERT_MSG_EQ(gMakeBoundCallbackTest7c,
                          4567,
                          "Callback did not fire or binding not correct");

    Callback<int> target8 = MakeBoundCallback(&MakeBoundCallbackTarget8, 2345, 3456, 4567);
    int resultThreeA = target8();
    NS_TEST_ASSERT_MSG_EQ(resultThreeA, 1234, "Return value of callback not correct");
    NS_TEST_ASSERT_MSG_EQ(gMakeBoundCallbackTest8a,
                          2345,
                          "Callback did not fire or binding not correct");
    NS_TEST_ASSERT_MSG_EQ(gMakeBoundCallbackTest8b,
                          3456,
                          "Callback did not fire or binding not correct");
    NS_TEST_ASSERT_MSG_EQ(gMakeBoundCallbackTest8c,
                          4567,
                          "Callback did not fire or binding not correct");

    Callback<int, int> target9 = MakeBoundCallback(&MakeBoundCallbackTarget9, 2345, 3456, 4567);
    int resultThreeB = target9(5678);
    NS_TEST_ASSERT_MSG_EQ(resultThreeB, 1234, "Return value of callback not correct");
    NS_TEST_ASSERT_MSG_EQ(gMakeBoundCallbackTest9a,
                          2345,
                          "Callback did not fire or binding not correct");
    NS_TEST_ASSERT_MSG_EQ(gMakeBoundCallbackTest9b,
                          3456,
                          "Callback did not fire or binding not correct");
    NS_TEST_ASSERT_MSG_EQ(gMakeBoundCallbackTest9c,
                          4567,
                          "Callback did not fire or binding not correct");
    NS_TEST_ASSERT_MSG_EQ(gMakeBoundCallbackTest9d,
                          5678,
                          "Callback did not fire or binding not correct");

    //
    // Test creating a bound callback pointing to a member function. Also, make sure that
    // an argument can be bound to a reference.
    //
    int b = 1;
    Callback<int> target10 =
        Callback<int>(&MakeBoundCallbackTestCase::BoundTarget, this, std::ref(b), 5, 2);
    NS_TEST_ASSERT_MSG_EQ(target10(), 8, "Bound callback returned an unexpected value");
    b = 3;
    NS_TEST_ASSERT_MSG_EQ(target10(), 10, "Bound callback returned an unexpected value");
}

/**
 * \ingroup callback-tests
 *
 * Test the callback equality implementation.
 */
class CallbackEqualityTestCase : public TestCase
{
  public:
    CallbackEqualityTestCase();

    ~CallbackEqualityTestCase() override
    {
    }

    /**
     * Member function used to test equality of callbacks.
     *
     * \param a first argument
     * \param b second argument
     * \return the sum of the arguments
     */
    int TargetMember(double a, int b)
    {
        return static_cast<int>(a) + b;
    }

  private:
    void DoRun() override;
    void DoSetup() override;
};

/**
 * Non-member function used to test equality of callbacks.
 *
 * \param a first argument
 * \param b second argument
 * \return the sum of the arguments
 */
int
CallbackEqualityTarget(double a, int b)
{
    return static_cast<int>(a) + b;
}

CallbackEqualityTestCase::CallbackEqualityTestCase()
    : TestCase("Check Callback equality test")
{
}

void
CallbackEqualityTestCase::DoSetup()
{
}

void
CallbackEqualityTestCase::DoRun()
{
    //
    // Make sure that two callbacks pointing to the same member function
    // compare equal.
    //
    Callback<int, double, int> target1a(&CallbackEqualityTestCase::TargetMember, this);
    Callback<int, double, int> target1b =
        MakeCallback(&CallbackEqualityTestCase::TargetMember, this);
    NS_TEST_ASSERT_MSG_EQ(target1a.IsEqual(target1b), true, "Equality test failed");

    //
    // Make sure that two callbacks pointing to the same member function
    // compare equal, after binding the first argument.
    //
    Callback<int, int> target2a(&CallbackEqualityTestCase::TargetMember, this, 1.5);
    Callback<int, int> target2b = target1b.Bind(1.5);
    NS_TEST_ASSERT_MSG_EQ(target2a.IsEqual(target2b), true, "Equality test failed");

    //
    // Make sure that two callbacks pointing to the same member function
    // compare equal, after binding the first two arguments.
    //
    Callback<int> target3a(target2a, 2);
    Callback<int> target3b = target1b.Bind(1.5, 2);
    NS_TEST_ASSERT_MSG_EQ(target3a.IsEqual(target3b), true, "Equality test failed");

    //
    // Make sure that two callbacks pointing to the same member function do
    // not compare equal if they are bound to different arguments.
    //
    Callback<int> target3c = target1b.Bind(1.5, 3);
    NS_TEST_ASSERT_MSG_EQ(target3c.IsEqual(target3b), false, "Equality test failed");

    //
    // Make sure that two callbacks pointing to the same non-member function
    // compare equal.
    //
    Callback<int, double, int> target4a(&CallbackEqualityTarget);
    Callback<int, double, int> target4b = MakeCallback(&CallbackEqualityTarget);
    NS_TEST_ASSERT_MSG_EQ(target4a.IsEqual(target4b), true, "Equality test failed");

    //
    // Make sure that two callbacks pointing to the same non-member function
    // compare equal, after binding the first argument.
    //
    Callback<int, int> target5a(&CallbackEqualityTarget, 1.5);
    Callback<int, int> target5b = target4b.Bind(1.5);
    NS_TEST_ASSERT_MSG_EQ(target5a.IsEqual(target5b), true, "Equality test failed");

    //
    // Make sure that two callbacks pointing to the same non-member function
    // compare equal, after binding the first two arguments.
    //
    Callback<int> target6a(target5a, 2);
    Callback<int> target6b = target4b.Bind(1.5, 2);
    NS_TEST_ASSERT_MSG_EQ(target6a.IsEqual(target6b), true, "Equality test failed");

    //
    // Make sure that two callbacks pointing to the same non-member function do
    // not compare equal if they are bound to different arguments.
    //
    Callback<int> target6c = target4b.Bind(1.5, 3);
    NS_TEST_ASSERT_MSG_EQ(target6c.IsEqual(target6b), false, "Equality test failed");

    //
    // Check that we cannot compare lambdas.
    //
    Callback<double, int, double> target7a([](int p, double d) { return d + p / 2.; });
    Callback<double, int, double> target7b([](int p, double d) { return d + p / 2.; });
    NS_TEST_ASSERT_MSG_EQ(target7a.IsEqual(target7b), false, "Compared lambdas?");

    //
    // Make sure that a callback pointing to a lambda and a copy of it compare equal.
    //
    Callback<double, int, double> target7c(target7b);
    NS_TEST_ASSERT_MSG_EQ(target7c.IsEqual(target7b), true, "Equality test failed");

    //
    // Make sure that a callback pointing to a lambda and a copy of it compare equal,
    // after binding the first argument.
    //
    Callback<double, double> target8b = target7b.Bind(1);
    Callback<double, double> target8c(target7c, 1);
    NS_TEST_ASSERT_MSG_EQ(target8b.IsEqual(target8c), true, "Equality test failed");

    //
    // Make sure that a callback pointing to a lambda and a copy of it compare equal,
    // after binding the first two arguments.
    //
    Callback<double> target9b = target8b.Bind(2.0);
    Callback<double> target9c(target8c, 2.0);
    NS_TEST_ASSERT_MSG_EQ(target9b.IsEqual(target9c), true, "Equality test failed");

    //
    // Make sure that a callback pointing to a lambda and a copy of it do not compare
    // equal if they are bound to different arguments.
    //
    Callback<double> target9d = target8b.Bind(4);
    NS_TEST_ASSERT_MSG_EQ(target9d.IsEqual(target9c), false, "Equality test failed");
}

/**
 * \ingroup callback-tests
 *
 * Test the Nullify mechanism.
 */
class NullifyCallbackTestCase : public TestCase
{
  public:
    NullifyCallbackTestCase();

    ~NullifyCallbackTestCase() override
    {
    }

    /**
     * Callback 1 target function.
     */
    void Target1()
    {
        m_test1 = true;
    }

  private:
    void DoRun() override;
    void DoSetup() override;

    bool m_test1; //!< true if Target1 has been called, false otherwise.
};

NullifyCallbackTestCase::NullifyCallbackTestCase()
    : TestCase("Check Nullify() and IsNull()")
{
}

void
NullifyCallbackTestCase::DoSetup()
{
    m_test1 = false;
}

void
NullifyCallbackTestCase::DoRun()
{
    //
    // Make sure we can declare and make a Callback pointing to a member
    // function returning void and execute it.
    //
    Callback<void> target1 = MakeCallback(&NullifyCallbackTestCase::Target1, this);
    target1();
    NS_TEST_ASSERT_MSG_EQ(m_test1, true, "Callback did not fire");

    NS_TEST_ASSERT_MSG_EQ(target1.IsNull(), false, "Working Callback reports IsNull()");

    target1.Nullify();

    NS_TEST_ASSERT_MSG_EQ(target1.IsNull(), true, "Nullified Callback reports not IsNull()");
}

/**
 * \ingroup callback-tests
 *
 * Make sure that various MakeCallback template functions compile and execute;
 * doesn't check an results of the execution.
 */
class MakeCallbackTemplatesTestCase : public TestCase
{
  public:
    MakeCallbackTemplatesTestCase();

    ~MakeCallbackTemplatesTestCase() override
    {
    }

    /**
     * Callback 1 target function.
     */
    void Target1()
    {
        m_test1 = true;
    }

  private:
    void DoRun() override;

    bool m_test1; //!< true if Target1 has been called, false otherwise.
};

/**
 * Test function - does nothing.
 * @{
 */
void TestFZero(){};
void TestFOne(int){};
void TestFTwo(int, int){};
void TestFThree(int, int, int){};
void TestFFour(int, int, int, int){};
void TestFFive(int, int, int, int, int){};
void TestFSix(int, int, int, int, int, int){};

void TestFROne(int&){};
void TestFRTwo(int&, int&){};
void TestFRThree(int&, int&, int&){};
void TestFRFour(int&, int&, int&, int&){};
void TestFRFive(int&, int&, int&, int&, int&){};
void TestFRSix(int&, int&, int&, int&, int&, int&){};

/** @} */

/**
 * \ingroup callback-tests
 *
 * Class used to check the capability of callbacks to call
 * public, protected, and private functions.
 */
class CallbackTestParent
{
  public:
    /// A public function.
    void PublicParent()
    {
    }

  protected:
    /// A protected function.
    void ProtectedParent()
    {
    }

    /// A static protected function.
    static void StaticProtectedParent()
    {
    }

  private:
    /// A private function.
    void PrivateParent()
    {
    }
};

/**
 * \ingroup callback-tests
 *
 * Derived class used to check the capability of callbacks to call
 * public, protected, and private functions.
 */
class CallbackTestClass : public CallbackTestParent
{
  public:
    /**
     * Test function - does nothing.
     * @{
     */
    void TestZero(){};
    void TestOne(int){};
    void TestTwo(int, int){};
    void TestThree(int, int, int){};
    void TestFour(int, int, int, int){};
    void TestFive(int, int, int, int, int){};
    void TestSix(int, int, int, int, int, int){};
    void TestCZero() const {};
    void TestCOne(int) const {};
    void TestCTwo(int, int) const {};
    void TestCThree(int, int, int) const {};
    void TestCFour(int, int, int, int) const {};
    void TestCFive(int, int, int, int, int) const {};
    void TestCSix(int, int, int, int, int, int) const {};

    /** @} */

    /**
     * Tries to make a callback to public and protected functions of a class.
     * Private are not tested because, as expected, the compilation fails.
     */
    void CheckParentalRights()
    {
        MakeCallback(&CallbackTestParent::StaticProtectedParent);
        MakeCallback(&CallbackTestParent::PublicParent, this);
        MakeCallback(&CallbackTestClass::ProtectedParent, this);
        // as expected, fails.
        // MakeCallback (&CallbackTestParent::PrivateParent, this);
        // as expected, fails.
        // Pointers do not carry the access restriction info, so it is forbidden
        // to generate a pointer to a parent's protected function, because
        // this could lead to un-protect them, e.g., by making it public.
        // MakeCallback (&CallbackTestParent::ProtectedParent, this);
    }
};

MakeCallbackTemplatesTestCase::MakeCallbackTemplatesTestCase()
    : TestCase("Check various MakeCallback() template functions")
{
}

void
MakeCallbackTemplatesTestCase::DoRun()
{
    CallbackTestClass that;

    MakeCallback(&CallbackTestClass::TestZero, &that);
    MakeCallback(&CallbackTestClass::TestOne, &that);
    MakeCallback(&CallbackTestClass::TestTwo, &that);
    MakeCallback(&CallbackTestClass::TestThree, &that);
    MakeCallback(&CallbackTestClass::TestFour, &that);
    MakeCallback(&CallbackTestClass::TestFive, &that);
    MakeCallback(&CallbackTestClass::TestSix, &that);

    MakeCallback(&CallbackTestClass::TestCZero, &that);
    MakeCallback(&CallbackTestClass::TestCOne, &that);
    MakeCallback(&CallbackTestClass::TestCTwo, &that);
    MakeCallback(&CallbackTestClass::TestCThree, &that);
    MakeCallback(&CallbackTestClass::TestCFour, &that);
    MakeCallback(&CallbackTestClass::TestCFive, &that);
    MakeCallback(&CallbackTestClass::TestCSix, &that);

    MakeCallback(&TestFZero);
    MakeCallback(&TestFOne);
    MakeCallback(&TestFTwo);
    MakeCallback(&TestFThree);
    MakeCallback(&TestFFour);
    MakeCallback(&TestFFive);
    MakeCallback(&TestFSix);

    MakeCallback(&TestFROne);
    MakeCallback(&TestFRTwo);
    MakeCallback(&TestFRThree);
    MakeCallback(&TestFRFour);
    MakeCallback(&TestFRFive);
    MakeCallback(&TestFRSix);

    MakeBoundCallback(&TestFOne, 1);
    MakeBoundCallback(&TestFTwo, 1);
    MakeBoundCallback(&TestFThree, 1);
    MakeBoundCallback(&TestFFour, 1);
    MakeBoundCallback(&TestFFive, 1);

    MakeBoundCallback(&TestFROne, 1);
    MakeBoundCallback(&TestFRTwo, 1);
    MakeBoundCallback(&TestFRThree, 1);
    MakeBoundCallback(&TestFRFour, 1);
    MakeBoundCallback(&TestFRFive, 1);

    that.CheckParentalRights();
}

/**
 * \ingroup callback-tests
 *
 * \brief The callback Test Suite.
 */
class CallbackTestSuite : public TestSuite
{
  public:
    CallbackTestSuite();
};

CallbackTestSuite::CallbackTestSuite()
    : TestSuite("callback", UNIT)
{
    AddTestCase(new BasicCallbackTestCase, TestCase::QUICK);
    AddTestCase(new MakeCallbackTestCase, TestCase::QUICK);
    AddTestCase(new MakeBoundCallbackTestCase, TestCase::QUICK);
    AddTestCase(new CallbackEqualityTestCase, TestCase::QUICK);
    AddTestCase(new NullifyCallbackTestCase, TestCase::QUICK);
    AddTestCase(new MakeCallbackTemplatesTestCase, TestCase::QUICK);
}

static CallbackTestSuite g_gallbackTestSuite; //!< Static variable for test initialization
