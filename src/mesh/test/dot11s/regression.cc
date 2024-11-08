/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Kirill Andreev <andreev@iitp.ru>
 */

#include "hwmp-proactive-regression.h"
#include "hwmp-reactive-regression.h"
#include "hwmp-simplest-regression.h"
#include "hwmp-target-flags-regression.h"
#include "pmp-regression.h"

#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup dot11s-test
 *
 * @brief Dot11s Regression Suite
 */
class Dot11sRegressionSuite : public TestSuite
{
  public:
    Dot11sRegressionSuite()
        : TestSuite("devices-mesh-dot11s-regression", Type::SYSTEM)
    {
        // We do not use NS_TEST_SOURCEDIR variable here since mesh/test has
        // subdirectories
        SetDataDir(std::string("src/mesh/test/dot11s"));
        AddTestCase(new PeerManagementProtocolRegressionTest, TestCase::Duration::QUICK);
        AddTestCase(new HwmpSimplestRegressionTest, TestCase::Duration::QUICK);
        AddTestCase(new HwmpReactiveRegressionTest, TestCase::Duration::QUICK);
        AddTestCase(new HwmpProactiveRegressionTest, TestCase::Duration::QUICK);
        AddTestCase(new HwmpDoRfRegressionTest, TestCase::Duration::QUICK);
    }
} g_dot11sRegressionSuite; ///< the test suite
