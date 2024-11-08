/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Pavel Boyko <boyko@iitp.ru>
 */

#include "flame-regression.h"

#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup flame-test
 *
 * @brief Flame Regression Suite
 */
class FlameRegressionSuite : public TestSuite
{
  public:
    FlameRegressionSuite()
        : TestSuite("devices-mesh-flame-regression", Type::SYSTEM)
    {
        // We do not use NS_TEST_SOURCEDIR variable here since mesh/test has
        // subdirectories
        SetDataDir(std::string("src/mesh/test/flame"));
        AddTestCase(new FlameRegressionTest, TestCase::Duration::QUICK);
    }
} g_flameRegressionSuite; ///< the test suite
