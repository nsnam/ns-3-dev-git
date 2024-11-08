/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Pavel Boyko <boyko@iitp.ru>
 */

#include "bug780-test.h"
#include "hello-regression-test.h"
#include "tc-regression-test.h"

using namespace ns3;
using namespace olsr;

/**
 * @ingroup olsr-test
 * @ingroup tests
 *
 * Various olsr regression tests
 */
class RegressionTestSuite : public TestSuite
{
  public:
    RegressionTestSuite()
        : TestSuite("routing-olsr-regression", Type::SYSTEM)
    {
        SetDataDir(NS_TEST_SOURCEDIR);
        AddTestCase(new HelloRegressionTest, TestCase::Duration::QUICK);
        AddTestCase(new TcRegressionTest, TestCase::Duration::QUICK);
        AddTestCase(new Bug780Test, TestCase::Duration::QUICK);
    }
};

static RegressionTestSuite g_olsrRegressionTestSuite; //!< Static variable for test initialization
