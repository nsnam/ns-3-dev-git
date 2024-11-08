/*
 * Copyright (c) 2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#include "ns3/assert.h"
#include "ns3/callback.h"
#include "ns3/command-line.h"

#include <iostream>

/**
 * @file
 * @ingroup core-examples
 * @ingroup callback
 * Example program illustrating use of callback functions and methods.
 *
 * See \ref callback
 */

using namespace ns3;

namespace
{

/**
 * Example Callback function.
 *
 * @param [in] a The first argument.
 * @param [in] b The second argument.
 * @returns The first argument.
 */
double
CbOne(double a, double b)
{
    std::cout << "invoke cbOne a=" << a << ", b=" << b << std::endl;
    return a;
}

/** Example Callback class. */
class MyCb
{
  public:
    /**
     * Example Callback class method.
     *
     * @param [in] a The argument.
     * @returns -5
     */
    int CbTwo(double a)
    {
        std::cout << "invoke cbTwo a=" << a << std::endl;
        return -5;
    }
};

} // unnamed namespace

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    // return type: double
    // first arg type: double
    // second arg type: double
    Callback<double, double, double> one;
    // build callback instance which points to cbOne function
    one = MakeCallback(&CbOne);
    // this is not a null callback
    NS_ASSERT(!one.IsNull());
    // invoke cbOne function through callback instance
    double retOne;
    retOne = one(10.0, 20.0);
    // callback returned expected value
    NS_ASSERT(retOne == 10.0);

    // return type: int
    // first arg type: double
    Callback<int, double> two;
    MyCb cb;
    // build callback instance which points to MyCb::cbTwo
    two = MakeCallback(&MyCb::CbTwo, &cb);
    // this is not a null callback
    NS_ASSERT(!two.IsNull());
    // invoke MyCb::cbTwo through callback instance
    int retTwo;
    retTwo = two(10.0);
    // callback returned expected value
    NS_ASSERT(retTwo == -5);

    two = MakeNullCallback<int, double>();
    // invoking a null callback is just like
    // invoking a null function pointer:
    // it will crash.
    // int retTwoNull = two (20.0);
    NS_ASSERT(two.IsNull());

#if 0
  // The below type mismatch between CbOne() and callback two will fail to
  // compile if enabled in this program.
  two = MakeCallback (&CbOne);
#endif

#if 0
  // This is a slightly different example, in which the code will compile
  // but because callbacks are type-safe, will cause a fatal error at runtime
  // (the difference here is that Assign() is called instead of operator=)
  Callback<void, float> three;
  three.Assign (MakeCallback (&CbOne));
#endif

    return 0;
}
